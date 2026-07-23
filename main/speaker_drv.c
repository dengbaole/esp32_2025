#include "speaker_drv.h"
#include "pca9557_drv.h"
#include "i2c_bus.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "speaker";

// ---- I2S 时钟引脚 ----
#define I2S_MCK_IO       (38)
#define I2S_BCK_IO       (14)
#define I2S_WS_IO        (13)

// ---- ES8311 参数 ----
#define ES8311_ADDR      (0x18)
#define ES8311_SAMPLE_RATE   (48000)

// ---- ES8311 寄存器 ----
#define ES8311_REG00_RESET    0x00
#define ES8311_REG01_CLK_MGR  0x01
#define ES8311_REG02_CLK_DIV  0x02
#define ES8311_REG03_ADC_OSR  0x03
#define ES8311_REG04_DAC_OSR  0x04
#define ES8311_REG05_DIV      0x05
#define ES8311_REG06_BCLK     0x06
#define ES8311_REG07_LRCK_H   0x07
#define ES8311_REG08_LRCK_L   0x08
#define ES8311_REG09_SDP_IN   0x09
#define ES8311_REG0A_SDP_OUT  0x0A
#define ES8311_REG0D_PWR_ANA  0x0D
#define ES8311_REG0E_PWR_PGA  0x0E
#define ES8311_REG12_PWR_DAC  0x12
#define ES8311_REG13_HP       0x13
#define ES8311_REG14_MIC      0x14
#define ES8311_REG17_ADC_GAIN 0x17
#define ES8311_REG1C_ADC_EQ   0x1C
#define ES8311_REG32_VOL      0x32
#define ES8311_REG37_DAC_EQ   0x37

static i2s_chan_handle_t tx_chan = NULL;
static i2c_master_dev_handle_t es8311_dev = NULL;

// ---- I2C 底层 ----
static esp_err_t es8311_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(es8311_dev, buf, 2, 100);
}

static esp_err_t es8311_read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(es8311_dev, &reg, 1, val, 1, 100);
}

// ---- ES8311 初始化（裸 I2C 寄存器）----
static void es8311_init(void)
{
    uint8_t tmp;

    // 1. 复位: 0x1F → 等待 → 0x00 → 0x80（上电）
    es8311_write_reg(ES8311_REG00_RESET, 0x1F);
    vTaskDelay(pdMS_TO_TICKS(20));
    es8311_write_reg(ES8311_REG00_RESET, 0x00);
    es8311_write_reg(ES8311_REG00_RESET, 0x80);

    // 2. 时钟源：MCLK 引脚输入，不反相，使能所有时钟
    es8311_write_reg(ES8311_REG01_CLK_MGR, 0x3F);

    // 3. 时钟分频系数（48kHz, 12.288MHz 查表）
    es8311_write_reg(ES8311_REG02_CLK_DIV, 0x00);  // pre_div=1, mult=1x
    es8311_write_reg(ES8311_REG03_ADC_OSR, 0x10);  // fs=ss, adc_osr=0x10
    es8311_write_reg(ES8311_REG04_DAC_OSR, 0x10);  // dac_osr=0x10
    es8311_write_reg(ES8311_REG05_DIV,     0x00);  // adc_div=1, dac_div=1

    // 4. BCLK 分频 + SCLK 不反相
    es8311_read_reg(ES8311_REG06_BCLK, &tmp);
    tmp = (tmp & 0xE0) | 0x03;   // bclk_div=4
    tmp &= ~BIT(5);               // sclk 不反相
    es8311_write_reg(ES8311_REG06_BCLK, tmp);

    // 5. LRCK 分频
    es8311_read_reg(ES8311_REG07_LRCK_H, &tmp);
    tmp = (tmp & 0xC0) | 0x00;   // lrck_h=0
    es8311_write_reg(ES8311_REG07_LRCK_H, tmp);
    es8311_write_reg(ES8311_REG08_LRCK_L, 0xFF); // lrck_l

    // 6. 音频格式：从模式 I2S, 16-bit
    es8311_read_reg(ES8311_REG00_RESET, &tmp);
    tmp &= 0xBF;  // Slave serial port
    es8311_write_reg(ES8311_REG00_RESET, tmp);
    es8311_write_reg(ES8311_REG09_SDP_IN,  (3 << 2));  // DAC 16-bit
    es8311_write_reg(ES8311_REG0A_SDP_OUT, (3 << 2));  // ADC 16-bit

    // 7. 上电序列
    es8311_write_reg(ES8311_REG0D_PWR_ANA, 0x01); // 模拟电路上电
    es8311_write_reg(ES8311_REG0E_PWR_PGA, 0x02); // PGA + ADC 调制器使能
    es8311_write_reg(ES8311_REG12_PWR_DAC, 0x00); // DAC 上电
    es8311_write_reg(ES8311_REG13_HP,      0x10); // 耳机输出使能

    // 8. ADC 旁路滤波器 + DAC 均衡器旁路
    es8311_write_reg(ES8311_REG1C_ADC_EQ, 0x6A);
    es8311_write_reg(ES8311_REG37_DAC_EQ, 0x08);

    // 9. 音量 70%
    es8311_write_reg(ES8311_REG32_VOL, (70 * 256 / 100) - 1); // = 178 = 0xB2

    // 10. 关闭数字 MIC，使能模拟 MIC + PGA 最大增益
    es8311_write_reg(ES8311_REG14_MIC, 0x1A);
    es8311_write_reg(ES8311_REG17_ADC_GAIN, 0xC8);

    ESP_LOGI(TAG, "ES8311 initialized (bare I2C)");
}

// ---- I2S 发送（标准 Philips 模式）----
static void i2s_tx_init(void)
{
    i2s_chan_config_t tx_conf = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_conf, &tx_chan, NULL));

    i2s_std_config_t std_conf = {
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .clk_cfg  = {
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .sample_rate_hz = ES8311_SAMPLE_RATE,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws   = I2S_WS_IO,
            .dout = SPK_I2S_DOUT,
            .din  = -1,  // 播放不需要输入
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_conf));
}

// ---- 播放文件 ----
esp_err_t speaker_drv_play(const char *filename)
{
    ESP_RETURN_ON_FALSE(tx_chan, ESP_FAIL, TAG, "speaker not initialized");

    char path[64];
    snprintf(path, sizeof(path), "/sdcard%s", filename);

    FILE *f = fopen(path, "rb");
    ESP_RETURN_ON_FALSE(f, ESP_FAIL, TAG, "failed to open %s", path);

    // 跳过 WAV 头（如果有的话）
    uint8_t header[44];
    fread(header, 1, 44, f);
    // 简单判断是否为 WAV（检查 "RIFF" 和 "WAVE" 标记）
    if (header[0] != 'R' || header[1] != 'I') {
        rewind(f);  // 不是 WAV，当 raw PCM 从头播放
    }

    ESP_LOGI(TAG, "Playing %s", path);

    static int16_t buf[4096];
    size_t bytes_read;

    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_chan), TAG, "i2s enable failed");

    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        size_t written = 0;
        ESP_ERROR_CHECK(i2s_channel_write(tx_chan, buf, bytes_read, &written, pdMS_TO_TICKS(1000)));
    }

    i2s_channel_disable(tx_chan);
    fclose(f);
    ESP_LOGI(TAG, "Playback done");
    return ESP_OK;
}

void speaker_drv_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    uint8_t reg = (vol == 0) ? 0 : (vol * 256 / 100) - 1;
    es8311_write_reg(ES8311_REG32_VOL, reg);
}

// ---- 对外接口 ----
void speaker_drv_init(void)
{
    i2s_tx_init();
    i2c_bus_init();
    es8311_dev = i2c_bus_add_device(ES8311_ADDR);
    es8311_init();
    pca9557_drv_set_bit(PCA9557_IO_PA_EN, 1);
    ESP_LOGI(TAG, "Speaker driver ready");
}

void speaker_drv_deinit(void)
{
    if (tx_chan) {
        i2s_channel_disable(tx_chan);
        i2s_del_channel(tx_chan);
        tx_chan = NULL;
    }
}
