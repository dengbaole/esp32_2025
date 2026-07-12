#include "audio_drv.h"
#include "format_wav.h"
#include "driver/i2s_tdm.h"
#include "driver/i2s_std.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "audio_drv";

// ---- 硬件引脚 ----
#define I2C_NUM            (0)
#define I2C_SDA_IO         (1)
#define I2C_SCL_IO         (2)

#define I2S_MCK_IO         (38)
#define I2S_BCK_IO         (14)
#define I2S_WS_IO          (13)
#define I2S_DI_IO          (12)

// ---- 音频参数 ----
#define I2S_SAMPLE_RATE    (48000)
#define I2S_SAMPLE_BITS    (I2S_DATA_BIT_WIDTH_16BIT)
#define I2S_CHAN_NUM       (2)
#define I2S_TDM_SLOT_MASK  (I2S_TDM_SLOT0 | I2S_TDM_SLOT1)

// ---- ES7210 参数（I2C 地址 0x41）----
#define ES7210_ADDR        (0x41)

// ---- ES7210 寄存器地址 ----
#define ES7210_REG00_RESET          0x00
#define ES7210_REG02_MAINCLK        0x02
#define ES7210_REG03_MASTER_CLK     0x03
#define ES7210_REG04_LRCK_DIVH      0x04
#define ES7210_REG05_LRCK_DIVL      0x05
#define ES7210_REG06_POWER_DOWN     0x06
#define ES7210_REG07_OSR            0x07
#define ES7210_REG08_MODE_CONFIG    0x08
#define ES7210_REG09_TIME_CTRL0     0x09
#define ES7210_REG0A_TIME_CTRL1     0x0A
#define ES7210_REG11_SDP1           0x11
#define ES7210_REG12_SDP2           0x12
#define ES7210_REG1B_ADC1_DB        0x1B
#define ES7210_REG1C_ADC2_DB        0x1C
#define ES7210_REG1D_ADC3_DB        0x1D
#define ES7210_REG1E_ADC4_DB        0x1E
#define ES7210_REG20_HPF34_2        0x20
#define ES7210_REG21_HPF34_1        0x21
#define ES7210_REG22_HPF12_2        0x22
#define ES7210_REG23_HPF12_1        0x23
#define ES7210_REG40_ANALOG         0x40
#define ES7210_REG41_MIC12_BIAS     0x41
#define ES7210_REG42_MIC34_BIAS     0x42
#define ES7210_REG43_MIC1_GAIN      0x43
#define ES7210_REG44_MIC2_GAIN      0x44
#define ES7210_REG45_MIC3_GAIN      0x45
#define ES7210_REG46_MIC4_GAIN      0x46
#define ES7210_REG47_MIC1_POWER     0x47
#define ES7210_REG48_MIC2_POWER     0x48
#define ES7210_REG49_MIC3_POWER     0x49
#define ES7210_REG4A_MIC4_POWER     0x4A
#define ES7210_REG4B_MIC12_BIASPW   0x4B
#define ES7210_REG4C_MIC34_BIASPW   0x4C

#define SD_MOUNT_POINT     "/sdcard"

static i2s_chan_handle_t rx_chan = NULL;

// ---- I2C 底层读写（与 QMI8658 模式完全一致）----
static esp_err_t es7210_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_NUM, ES7210_ADDR,
                                      buf, sizeof(buf),
                                      1000 / portTICK_PERIOD_MS);
}

// ---- I2C 初始化（与 IMU 共用总线）----
static void i2c_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &conf));
    esp_err_t ret = i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C already installed by IMU, skip");
    }
}

// ---- I2S 初始化（TDM 模式，仅接收）----
static void i2s_init(void) {
    i2s_chan_config_t rx_conf = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_conf, NULL, &rx_chan));

    i2s_tdm_config_t tdm_conf = {
        .slot_cfg = I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_SAMPLE_BITS, I2S_SLOT_MODE_STEREO, I2S_TDM_SLOT_MASK),
        .clk_cfg  = {
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .sample_rate_hz = I2S_SAMPLE_RATE,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws   = I2S_WS_IO,
            .dout = -1,
            .din  = I2S_DI_IO,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(rx_chan, &tdm_conf));
}

// ---- ES7210 初始化（裸写寄存器，无外部组件依赖）----
static void es7210_init(void) {
    // 1. 软件复位
    es7210_write_reg(ES7210_REG00_RESET, 0xFF);
    es7210_write_reg(ES7210_REG00_RESET, 0x32);

    // 2. 上电初始化时间
    es7210_write_reg(ES7210_REG09_TIME_CTRL0, 0x30);
    es7210_write_reg(ES7210_REG0A_TIME_CTRL1, 0x30);

    // 3. 高通滤波器（ADC1-4）
    es7210_write_reg(ES7210_REG23_HPF12_1, 0x2A);
    es7210_write_reg(ES7210_REG22_HPF12_2, 0x0A);
    es7210_write_reg(ES7210_REG21_HPF34_1, 0x2A);
    es7210_write_reg(ES7210_REG20_HPF34_2, 0x0A);

    // 4. I2S 格式：标准 I2S + 16bit
    es7210_write_reg(ES7210_REG11_SDP1, 0x00 | 0x60);  // I2S fmt + 16bit
    // TDM 模式使能
    es7210_write_reg(ES7210_REG12_SDP2, 0x02);

    // 5. 模拟电源 + VMID
    es7210_write_reg(ES7210_REG40_ANALOG, 0xC3);

    // 6. MIC 偏置电压 2.87V
    es7210_write_reg(ES7210_REG41_MIC12_BIAS, 0x70);
    es7210_write_reg(ES7210_REG42_MIC34_BIAS, 0x70);

    // 7. MIC 增益 30dB（10 | 0x10 = 0x1A）
    es7210_write_reg(ES7210_REG43_MIC1_GAIN, 10 | 0x10);
    es7210_write_reg(ES7210_REG44_MIC2_GAIN, 10 | 0x10);
    es7210_write_reg(ES7210_REG45_MIC3_GAIN, 10 | 0x10);
    es7210_write_reg(ES7210_REG46_MIC4_GAIN, 10 | 0x10);

    // 8. MIC1-4 上电
    es7210_write_reg(ES7210_REG47_MIC1_POWER, 0x08);
    es7210_write_reg(ES7210_REG48_MIC2_POWER, 0x08);
    es7210_write_reg(ES7210_REG49_MIC3_POWER, 0x08);
    es7210_write_reg(ES7210_REG4A_MIC4_POWER, 0x08);

    // 9. 采样率 48kHz, MCLK=256×48k=12.288MHz
    es7210_write_reg(ES7210_REG07_OSR, 0x20);
    es7210_write_reg(ES7210_REG02_MAINCLK, 0xC1);   // adc_div=1, doubler=1, dll=1
    es7210_write_reg(ES7210_REG04_LRCK_DIVH, 0x01);
    es7210_write_reg(ES7210_REG05_LRCK_DIVL, 0x00);

    // 10. 关闭 DLL
    es7210_write_reg(ES7210_REG06_POWER_DOWN, 0x04);

    // 11. MIC1-4 偏置 + ADC + PGA 电源使能
    es7210_write_reg(ES7210_REG4B_MIC12_BIASPW, 0x0F);
    es7210_write_reg(ES7210_REG4C_MIC34_BIASPW, 0x0F);

    // 12. 使能芯片
    es7210_write_reg(ES7210_REG00_RESET, 0x71);
    es7210_write_reg(ES7210_REG00_RESET, 0x41);

    // 13. ADC 音量 0dB（191 = 0xBF）
    es7210_write_reg(ES7210_REG1B_ADC1_DB, 0xBF);
    es7210_write_reg(ES7210_REG1C_ADC2_DB, 0xBF);
    es7210_write_reg(ES7210_REG1D_ADC3_DB, 0xBF);
    es7210_write_reg(ES7210_REG1E_ADC4_DB, 0xBF);

    ESP_LOGI(TAG, "ES7210 initialized (bare I2C)");
}

// ---- 录音 ----
esp_err_t audio_drv_record(const char *filename, int duration_sec) {
    ESP_RETURN_ON_FALSE(rx_chan, ESP_FAIL, TAG, "audio not initialized");

    uint32_t byte_rate = I2S_SAMPLE_RATE * I2S_CHAN_NUM * (I2S_SAMPLE_BITS / 8);
    uint32_t wav_size = byte_rate * duration_sec;

    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(wav_size, I2S_SAMPLE_BITS, I2S_SAMPLE_RATE, I2S_CHAN_NUM);

    char path[64];
    snprintf(path, sizeof(path), SD_MOUNT_POINT "%s", filename);
    ESP_LOGI(TAG, "Recording to %s (%ds)", path, duration_sec);

    FILE *f = fopen(path, "w");
    ESP_RETURN_ON_FALSE(f, ESP_FAIL, TAG, "failed to open file");

    if (fwrite(&wav_header, sizeof(wav_header_t), 1, f) != 1) {
        fclose(f);
        return ESP_FAIL;
    }

    size_t written = 0;
    static int16_t buf[4096];
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx_chan), TAG, "i2s enable failed");

    while (written < wav_size) {
        if (written % byte_rate < sizeof(buf)) {
            ESP_LOGI(TAG, "Recording: %d/%ds", (int)(written / byte_rate) + 1, duration_sec);
        }
        size_t bytes_read = 0;
        esp_err_t ret = i2s_channel_read(rx_chan, buf, sizeof(buf), &bytes_read,
                                          pdMS_TO_TICKS(1000));
        if (ret != ESP_OK) break;
        if (fwrite(buf, bytes_read, 1, f) != 1) break;
        written += bytes_read;
    }

    i2s_channel_disable(rx_chan);
    fclose(f);
    ESP_LOGI(TAG, "Recording done: %s", path);
    return ESP_OK;
}

void audio_drv_init(void) {
    i2s_init();
    i2c_init();
    es7210_init();
    ESP_LOGI(TAG, "Audio driver ready");
}

void audio_drv_deinit(void) {
    if (rx_chan) {
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
}
