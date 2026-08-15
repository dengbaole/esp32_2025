// 摄像头驱动（去组件版）—— GC0308 + LCD_CAM + GDMA
//
// 分层：
//   gc0308_drv.c : 传感器层，只负责 I2C 寄存器配置（换其他 DVP 摄像头改这里）
//   camera_drv.c : 控制器层，只负责 DVP 信号接收和帧缓冲（对应 STM32 的 DCMI）
//
// 采集流程（参考 esp32-camera 组件在 ESP32-S3 上的 PSRAM RGB565 模式）：
//   1. VSYNC 中断到来 -> 启动/复位 LCD_CAM 和 GDMA，把描述符链指向一块 PSRAM 帧缓冲
//   2. GDMA 按 6 行(3840 字节)一个节点，环形 DMA 进 PSRAM，整帧 40 个节点
//   3. 下一个 VSYNC 中断到来 -> 停止 GDMA，同步 cache，回调应用层，再武装另一块缓冲
//
// 不依赖 esp32-camera 组件。managed_components/ 里的组件保留作学习对照，不参与编译。

#include "camera_drv.h"
#include "gc0308_drv.h"
#include "pca9557_drv.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_intr_alloc.h"
#include "esp_cache.h"
#include "esp_rom_gpio.h"
#include "esp_rom_sys.h"
#include "esp_private/gdma.h"
#include "esp_private/periph_ctrl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/ledc.h"
#include "driver/gpio.h"

#include "soc/lcd_cam_struct.h"
#include "soc/lcd_cam_reg.h"
#include "soc/gdma_struct.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_periph.h"
#include "soc/io_mux_reg.h"
#include "hal/dma_types.h"
#include "hal/cache_hal.h"
#include "hal/cache_ll.h"
#include "hal/clk_gate_ll.h"

#include <string.h>

static const char *TAG = "camera";

// ---- 1. 硬件引脚：DVP 8 位并口 ----
#define CAM_PIN_D0      16
#define CAM_PIN_D1      18
#define CAM_PIN_D2      8
#define CAM_PIN_D3      17
#define CAM_PIN_D4      15
#define CAM_PIN_D5      6
#define CAM_PIN_D6      4
#define CAM_PIN_D7      9
#define CAM_PIN_XCLK    5
#define CAM_PIN_PCLK    7
#define CAM_PIN_VSYNC   3
#define CAM_PIN_HREF    46

// ---- 2. 图像参数：QVGA RGB565 ----
#define CAM_FRAME_W       320
#define CAM_FRAME_H       240
#define CAM_LINE_BYTES    (CAM_FRAME_W * 2)                  // 一行 640 字节
#define CAM_FB_SIZE       (CAM_LINE_BYTES * CAM_FRAME_H)     // 一帧 153600 字节

// ---- 3. GDMA 描述符参数 ----
// 每个节点装 6 行（3840 字节）：既是行宽整数倍，又不超描述符 4095 上限
#define DMA_NODE_LINES     6
#define DMA_NODE_BYTES     (CAM_LINE_BYTES * DMA_NODE_LINES) // 3840
#define DMA_NODE_COUNT     (CAM_FB_SIZE / DMA_NODE_BYTES)    // 40 个节点
#define DMA_EOF_BYTES      (DMA_NODE_BYTES * 4)              // 每 15360 字节一次 EOF（中断未开，仅硬件事件）
#define FB_COUNT           2                                 // PSRAM 双缓冲

// ---- 4. 内部状态 ----
typedef enum {
    CAM_EVT_VSYNC = 0,
    CAM_EVT_STOP,
} cam_event_t;

static QueueHandle_t s_evt_queue = NULL;      // VSYNC/停止事件队列
static TaskHandle_t s_task = NULL;            // 采集任务句柄
static volatile bool s_running = false;
static volatile uint32_t s_evt_overflow = 0;
static camera_frame_cb_t s_frame_cb = NULL;

static uint8_t *s_fb[FB_COUNT] = {NULL, NULL};        // PSRAM 帧缓冲
static dma_descriptor_t *s_desc[FB_COUNT] = {NULL, NULL}; // 每帧一套描述符链
static gdma_channel_handle_t s_dma_chan = NULL;        // GDMA 通道句柄
static int s_dma_ch = -1;                              // GDMA 通道号
static bool s_capturing = false;                       // 仅采集任务使用

static void capture_stop(void);
static void capture_start(int fb);

// ---- 5. XCLK：LEDC 输出 24MHz 给摄像头 ----
static void xclk_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_1_BIT,   // 1 位分辨率：占空比 50%
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 24000000,
        .clk_cfg         = LEDC_AUTO_CLK,
        .deconfigure     = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num   = CAM_PIN_XCLK,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 1,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

// ---- 6. DVP 引脚路由：把物理引脚连到 LCD_CAM 外设信号 ----
static void dvp_pin_input(int pin, int signal, bool invert)
{
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_FLOATING);
    esp_rom_gpio_connect_in_signal(pin, signal, invert);
}

static void dvp_pins_init(void)
{
    const int data_pins[8] = {
        CAM_PIN_D0, CAM_PIN_D1, CAM_PIN_D2, CAM_PIN_D3,
        CAM_PIN_D4, CAM_PIN_D5, CAM_PIN_D6, CAM_PIN_D7,
    };
    for (int i = 0; i < 8; i++) {
        dvp_pin_input(data_pins[i], CAM_DATA_IN0_IDX + i, false);
    }
    dvp_pin_input(CAM_PIN_PCLK, CAM_PCLK_IDX, false);
    dvp_pin_input(CAM_PIN_VSYNC, CAM_V_SYNC_IDX, true);   // GC0308 VSYNC 需反相
    dvp_pin_input(CAM_PIN_HREF, CAM_H_ENABLE_IDX, false);
}

// ---- 7. LCD_CAM 外设：DVP 接收模式 ----
static void lcd_cam_dvp_init(void)
{
    // LCD_CAM 外设时钟；已被其他驱动开启时不要再次复位，避免影响别人
    if (!periph_ll_periph_enabled(PERIPH_LCD_CAM_MODULE)) {
        periph_ll_disable_clk_set_rst(PERIPH_LCD_CAM_MODULE);
        periph_ll_enable_clk_clear_rst(PERIPH_LCD_CAM_MODULE);
    }

    LCD_CAM.cam_ctrl.val = 0;
    LCD_CAM.cam_ctrl.cam_clkm_div_b = 0;
    LCD_CAM.cam_ctrl.cam_clkm_div_a = 0;
    LCD_CAM.cam_ctrl.cam_clkm_div_num = 160000000 / 24000000;
    LCD_CAM.cam_ctrl.cam_clk_sel = 3;               // XCLK 由 LEDC 提供，不用 LCD_CAM 内部时钟
    LCD_CAM.cam_ctrl.cam_stop_en = 0;
    LCD_CAM.cam_ctrl.cam_vsync_filter_thres = 4;    // VSYNC 滤波阈值
    LCD_CAM.cam_ctrl.cam_byte_order = 0;
    LCD_CAM.cam_ctrl.cam_bit_order = 0;
    LCD_CAM.cam_ctrl.cam_line_int_en = 0;
    LCD_CAM.cam_ctrl.cam_vs_eof_en = 0;             // EOF 由接收字节数控制，不由 VSYNC 控制

    LCD_CAM.cam_ctrl1.val = 0;
    LCD_CAM.cam_ctrl1.cam_rec_data_bytelen = DMA_NODE_BYTES - 1;
    LCD_CAM.cam_ctrl1.cam_line_int_num = 0;
    LCD_CAM.cam_ctrl1.cam_clk_inv = 0;
    LCD_CAM.cam_ctrl1.cam_vsync_filter_en = 1;
    LCD_CAM.cam_ctrl1.cam_2byte_en = 0;             // 8 位数据总线
    LCD_CAM.cam_ctrl1.cam_de_inv = 0;
    LCD_CAM.cam_ctrl1.cam_hsync_inv = 0;
    LCD_CAM.cam_ctrl1.cam_vsync_inv = 0;            // 反相已在 GPIO 矩阵层完成
    LCD_CAM.cam_ctrl1.cam_vh_de_mode_en = 0;        // 标准 DVP 模式

    LCD_CAM.cam_rgb_yuv.val = 0;                    // 不做硬件色彩转换

    LCD_CAM.cam_ctrl.cam_update = 1;
    LCD_CAM.cam_ctrl1.cam_start = 1;
}

// ---- 8. GDMA + PSRAM 帧缓冲初始化 ----
static esp_err_t gdma_init(void)
{
    esp_err_t ret;

    // GDMA 外设时钟；已有其他驱动（SPI/I2S）使用 GDMA 时只复位自己的通道
    if (!periph_ll_periph_enabled(PERIPH_GDMA_MODULE)) {
        periph_ll_disable_clk_set_rst(PERIPH_GDMA_MODULE);
        periph_ll_enable_clk_clear_rst(PERIPH_GDMA_MODULE);
    }

    // 分配 RX 通道（LCD_CAM 是 AHB 外设，数据方向对 GDMA 来说是 RX）
    gdma_channel_alloc_config_t alloc_cfg = {
        .direction = GDMA_CHANNEL_DIRECTION_RX,
    };
    ret = gdma_new_ahb_channel(&alloc_cfg, &s_dma_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "alloc GDMA channel failed: 0x%x", ret);
        return ret;
    }
    gdma_get_channel_id(s_dma_chan, &s_dma_ch);

    // 把 GDMA 通道接到 LCD_CAM 外设（触发源 CAM0）
    gdma_trigger_t trig = GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_CAM, 0);
    ret = gdma_connect(s_dma_chan, trig);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "connect GDMA to CAM failed: 0x%x", ret);
        return ret;
    }

    gdma_strategy_config_t strategy = {
        .owner_check = false,       // 不检查描述符 owner 位，中断/重启处理更简单
        .auto_update_desc = false,
    };
    gdma_apply_strategy(s_dma_chan, &strategy);

    // 允许 DMA 写 PSRAM，并开启 32 字节突发
    gdma_transfer_config_t transfer = {
        .max_data_burst_size = 32,
        .access_ext_mem = true,
    };
    gdma_config_transfer(s_dma_chan, &transfer);

    // 分配两帧 PSRAM 缓冲。3840 是 64 的整数倍，因此每个描述符节点也自然对齐
    for (int i = 0; i < FB_COUNT; i++) {
        s_fb[i] = heap_caps_aligned_alloc(64, CAM_FB_SIZE,
                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_fb[i]) {
            ESP_LOGE(TAG, "frame buffer %d alloc failed", i);
            return ESP_ERR_NO_MEM;
        }
    }

    // 分配两套 DMA 描述符（必须放内部 DMA 可访问内存），每套 40 个节点环形连接
    s_desc[0] = heap_caps_calloc(FB_COUNT * DMA_NODE_COUNT, sizeof(dma_descriptor_t),
                                 MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_desc[0]) {
        ESP_LOGE(TAG, "DMA descriptor alloc failed");
        return ESP_ERR_NO_MEM;
    }
    s_desc[1] = s_desc[0] + DMA_NODE_COUNT;

    for (int f = 0; f < FB_COUNT; f++) {
        for (int i = 0; i < DMA_NODE_COUNT; i++) {
            s_desc[f][i].dw0.size = DMA_NODE_BYTES;
            s_desc[f][i].dw0.length = 0;
            s_desc[f][i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
            s_desc[f][i].dw0.suc_eof = 0;
            s_desc[f][i].buffer = s_fb[f] + i * DMA_NODE_BYTES;
            s_desc[f][i].next = &s_desc[f][(i + 1) % DMA_NODE_COUNT];
        }
    }

    ESP_LOGI(TAG, "GDMA ch%d ready, fb=%p/%p", s_dma_ch, s_fb[0], s_fb[1]);
    return ESP_OK;
}

// ---- 9. VSYNC 中断：每个新帧开始时通知任务 ----
static void IRAM_ATTR vsync_isr(void *arg)
{
    uint32_t st = LCD_CAM.lc_dma_int_st.val;
    if (st == 0) {
        return;
    }
    LCD_CAM.lc_dma_int_clr.val = st;

    if (st & LCD_CAM_CAM_VSYNC_INT_ST_M) {
        cam_event_t evt = CAM_EVT_VSYNC;
        BaseType_t hpw = pdFALSE;
        if (xQueueSendFromISR(s_evt_queue, &evt, &hpw) != pdTRUE) {
            s_evt_overflow++;
        }
        if (hpw == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

static esp_err_t vsync_intr_init(void)
{
    esp_err_t ret = esp_intr_alloc_intrstatus(
        ETS_LCD_CAM_INTR_SOURCE,
        ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM,
        (uint32_t)&LCD_CAM.lc_dma_int_st.val,
        LCD_CAM_CAM_VSYNC_INT_ST_M,
        vsync_isr, NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VSYNC intr alloc failed: 0x%x", ret);
    }
    return ret;
}

// ---- 10. 采集启停（组件同款寄存器流程） ----
static void capture_stop(void)
{
    GDMA.channel[s_dma_ch].in.link.stop = 1;
}

static void capture_start(int fb)
{
    // 复位 CAM 和 GDMA 接收逻辑
    LCD_CAM.cam_ctrl1.cam_start = 0;
    LCD_CAM.cam_ctrl1.cam_reset = 1;
    LCD_CAM.cam_ctrl1.cam_reset = 0;
    LCD_CAM.cam_ctrl1.cam_afifo_reset = 1;
    LCD_CAM.cam_ctrl1.cam_afifo_reset = 0;
    GDMA.channel[s_dma_ch].in.conf0.in_rst = 1;
    GDMA.channel[s_dma_ch].in.conf0.in_rst = 0;

    // 每接收 15360 字节产生一次 EOF 事件；这里没开 EOF 中断，
    // RGB565+PSRAM 模式下只需 VSYNC 中断即可知道一整帧结束
    LCD_CAM.cam_ctrl1.cam_rec_data_bytelen = DMA_EOF_BYTES - 1;

    // 把 GDMA 描述符链指向当前帧缓冲并启动
    GDMA.channel[s_dma_ch].in.link.addr = ((uint32_t)&s_desc[fb][0]) & 0xFFFFF;
    GDMA.channel[s_dma_ch].in.link.start = 1;

    LCD_CAM.cam_ctrl.cam_update = 1;
    LCD_CAM.cam_ctrl1.cam_start = 1;

    // 组件同款技巧：制造一个 10us 的假 VSYNC 边沿，让 CAM 不用再等一整帧
    esp_rom_gpio_connect_in_signal(CAM_PIN_VSYNC, CAM_V_SYNC_IDX, false);
    esp_rom_delay_us(10);
    esp_rom_gpio_connect_in_signal(CAM_PIN_VSYNC, CAM_V_SYNC_IDX, true);
}

// DMA 写 PSRAM 后，CPU 再读前必须同步 cache，否则读到的是旧缓存
static void frame_cache_sync(uint8_t *buf)
{
    size_t line = cache_hal_get_cache_line_size(CACHE_LL_LEVEL_EXT_MEM, CACHE_TYPE_DATA);
    if (line == 0) {
        line = 32;
    }
    uintptr_t start = (uintptr_t)buf & ~(line - 1);
    size_t len = CAM_FB_SIZE + ((uintptr_t)buf - start);
    len = (len + line - 1) & ~(line - 1);
    esp_cache_msync((void *)start, len,
                    ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
}

// ---- 11. 采集任务：VSYNC 状态机 ----
static void capture_task(void *arg)
{
    int fb = 0;

    while (s_running) {
        cam_event_t evt;
        if (!xQueueReceive(s_evt_queue, &evt, portMAX_DELAY)) {
            continue;
        }

        if (evt == CAM_EVT_STOP) {
            break;
        }
        if (evt != CAM_EVT_VSYNC) {
            continue;
        }

        if (!s_capturing) {
            // 第一个 VSYNC：只武装，不收帧（避免收到半帧）
            capture_start(fb);
            s_capturing = true;
        } else {
            // 后续 VSYNC：上一帧已经写满，先停 DMA
            capture_stop();
            uint8_t *done = s_fb[fb];

            // 切换到另一块缓冲并立即武装，下一帧不会漏
            fb ^= 1;
            capture_start(fb);

            // 再把完成的帧交给应用层（LCD 显示）
            frame_cache_sync(done);
            if (s_frame_cb) {
                s_frame_cb((const uint16_t *)done, CAM_FRAME_W, CAM_FRAME_H);
            }
        }

        if (s_evt_overflow) {
            ESP_LOGW(TAG, "VSYNC event queue overflow: %lu", (unsigned long)s_evt_overflow);
            s_evt_overflow = 0;
        }
    }

    capture_stop();
    s_task = NULL;
    vTaskDelete(NULL);
}

// ---- 12. 对外 API（与原来 camera_drv.h 完全兼容） ----
void camera_drv_init(void)
{
    i2c_bus_init();

    // 摄像头上电：DVP_PWDN 拉低（低有效）
    pca9557_drv_set_bit(PCA9557_IO_DVP_PWDN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    xclk_init();
    dvp_pins_init();
    lcd_cam_dvp_init();

    esp_err_t ret = gdma_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GDMA init failed: 0x%x", ret);
        return;
    }

    s_evt_queue = xQueueCreate(4, sizeof(cam_event_t));
    if (!s_evt_queue) {
        ESP_LOGE(TAG, "event queue create failed");
        return;
    }

    ret = vsync_intr_init();
    if (ret != ESP_OK) {
        return;
    }

    ret = gc0308_drv_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GC0308 init failed: 0x%x", ret);
        return;
    }

    ESP_LOGI(TAG, "Camera ready (bare-metal driver)");
}

void camera_drv_start(camera_frame_cb_t on_frame)
{
    if (!s_evt_queue || s_dma_ch < 0) {
        ESP_LOGE(TAG, "camera not initialized");
        return;
    }
    if (s_task) {
        s_frame_cb = on_frame;  // 已在运行，只换回调
        return;
    }

    s_frame_cb = on_frame;
    s_running = true;
    s_capturing = false;
    s_evt_overflow = 0;
    xQueueReset(s_evt_queue);

    LCD_CAM.lc_dma_int_clr.cam_vsync_int_clr = 1;
    LCD_CAM.lc_dma_int_ena.cam_vsync_int_ena = 1;

    // 高优先级：VSYNC 来时要尽快停 DMA/武装下一帧
    xTaskCreatePinnedToCore(capture_task, "camera", 4096, NULL,
                            configMAX_PRIORITIES - 2, &s_task, 0);
    if (!s_task) {
        ESP_LOGE(TAG, "camera task create failed");
        s_running = false;
        return;
    }
    ESP_LOGI(TAG, "Preview started");
}

void camera_drv_stop(void)
{
    if (!s_task) {
        return;
    }

    s_running = false;
    LCD_CAM.lc_dma_int_ena.cam_vsync_int_ena = 0;
    capture_stop();

    cam_event_t evt = CAM_EVT_STOP;
    xQueueSend(s_evt_queue, &evt, 0);

    // 等任务自己退出；异常情况兜底强删
    for (int i = 0; i < 100 && s_task; i++) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (s_task) {
        vTaskDelete(s_task);
        s_task = NULL;
    }
    s_frame_cb = NULL;
}
