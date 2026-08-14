#include "imu_drv.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"

static const char *TAG = "imu_drv";

// ---- QMI8658 寄存器 ----
enum qmi8658_reg {
    QMI8658_WHO_AM_I,       // 0x00
    QMI8658_REVISION_ID,    // 0x01
    QMI8658_CTRL1,          // 0x02
    QMI8658_CTRL2,          // 0x03
    QMI8658_CTRL3,          // 0x04
    QMI8658_CTRL4,          // 0x05
    QMI8658_CTRL5,          // 0x06
    QMI8658_CTRL6,          // 0x07
    QMI8658_CTRL7,          // 0x08
    QMI8658_CTRL8,          // 0x09
    QMI8658_CTRL9,          // 0x0A
    QMI8658_AX_L = 0x35,    // 加速度 X 低字节
    QMI8658_AX_H,           // 0x36
    QMI8658_AY_L,           // 0x37
    QMI8658_AY_H,           // 0x38
    QMI8658_AZ_L,           // 0x39
    QMI8658_AZ_H,           // 0x3A
    QMI8658_GX_L,           // 0x3B
    QMI8658_GX_H,           // 0x3C
    QMI8658_GY_L,           // 0x3D
    QMI8658_GY_H,           // 0x3E
    QMI8658_GZ_L,           // 0x3F
    QMI8658_GZ_H,           // 0x40
    QMI8658_STATUS0 = 0x2E, // 数据状态
    QMI8658_RESET = 0x60,   // 复位
};

#define QMI8658_ADDR  0x6A
static i2c_master_dev_handle_t qmi8658_dev = NULL;

// ---- I2C 底层读写 ----
static esp_err_t qmi8658_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(qmi8658_dev, &reg, 1, data, len, 100);
}

static esp_err_t qmi8658_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(qmi8658_dev, buf, 2, 100);
}

// ---- QMI8658 初始化 ----

static void qmi8658_init(void) {
    uint8_t id = 0;
    int retry;

    // 等待芯片就绪（带超时，避免死循环）
    for (retry = 0; retry < 50; retry++) {
        if (qmi8658_read_reg(QMI8658_WHO_AM_I, &id, 1) == ESP_OK && id == 0x05)
            break;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (id != 0x05) {
        ESP_LOGE(TAG, "WHO_AM_I failed: 0x%02x", id);
        return;
    }

    // 复位 + 配置（写回验证 + 重试，提升上电时序稳定性）
    for (retry = 0; retry < 3; retry++) {
        qmi8658_write_reg(QMI8658_RESET, 0xb0);
        vTaskDelay(pdMS_TO_TICKS(50));  // 复位后多等一会，等内部就绪

        // 注意：CTRL2/CTRL3 的 bit7 必须为 0！
        //   CTRL2 bit7=0 使能内部高速振荡器（=1 会禁用，导致不采样）
        //   CTRL3 bit7=0 禁用陀螺仪自检（=1 会进入自检模式）
        qmi8658_write_reg(QMI8658_CTRL1, 0x40);  // 地址自动递增 + Little-Endian
        qmi8658_write_reg(QMI8658_CTRL7, 0x03);  // 使能 Acc + Gyr
        qmi8658_write_reg(QMI8658_CTRL2, 0x15);  // Acc: ±4g, 250Hz, 高速振荡器使能
        qmi8658_write_reg(QMI8658_CTRL3, 0x55);  // Gyr: ±512dps, 250Hz, 无自检

        // 读回验证配置是否写入成功
        uint8_t c1 = 0, c7 = 0, c2 = 0, c3 = 0;
        qmi8658_read_reg(QMI8658_CTRL1, &c1, 1);
        qmi8658_read_reg(QMI8658_CTRL7, &c7, 1);
        qmi8658_read_reg(QMI8658_CTRL2, &c2, 1);
        qmi8658_read_reg(QMI8658_CTRL3, &c3, 1);
        if (c1 == 0x40 && c7 == 0x03 && c2 == 0x15 && c3 == 0x55) {
            ESP_LOGI(TAG, "QMI8658 OK (attempt %d)", retry + 1);
            return;
        }
        ESP_LOGW(TAG, "config mismatch c1=0x%02x c7=0x%02x c2=0x%02x c3=0x%02x, retry %d",
                 c1, c7, c2, c3, retry + 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGE(TAG, "QMI8658 init failed after retries");
}

// ---- 读取原始数据 ----
// 注意：QMI8658 的 DRDY 就绪位在读取 STATUS0 后会清除，
//       所以 STATUS0 只能读一次判断，随后直接读数据，不能再读 STATUS0。

static void qmi8658_read_acc_gyr(t_sQMI8658 *p) {
    int16_t buf[6] = {0};

    if (qmi8658_read_reg(QMI8658_AX_L, (uint8_t *)buf, 12) != ESP_OK)
        return;
    p->acc_x = buf[0];
    p->acc_y = buf[1];
    p->acc_z = buf[2];
    p->gyr_x = buf[3];
    p->gyr_y = buf[4];
    p->gyr_z = buf[5];
}

// ---- 加速度计计算姿态角 ----

void imu_drv_read_angle(t_sQMI8658 *p) {
    float temp;
    uint8_t status = 0;
    static uint32_t stall = 0;

    // 检查数据就绪（只读一次 STATUS0，随后直接读数据）
    if (qmi8658_read_reg(QMI8658_STATUS0, &status, 1) == ESP_OK && (status & 0x03)) {
        stall = 0;
        qmi8658_read_acc_gyr(p);
    } else {
        if (++stall >= 5) {  // 连续 5 秒无数据
            ESP_LOGW(TAG, "QMI8658 data stalled (STATUS0=0x%02x), re-init...", status);
            qmi8658_init();
            stall = 0;
        }
    }

    // AngleX: X 轴与水平面的夹角
    temp = (float)p->acc_x / sqrtf((float)p->acc_y * p->acc_y +
                                    (float)p->acc_z * p->acc_z);
    p->AngleX = atanf(temp) * 57.29578f;

    // AngleY: Y 轴与水平面的夹角
    temp = (float)p->acc_y / sqrtf((float)p->acc_x * p->acc_x +
                                    (float)p->acc_z * p->acc_z);
    p->AngleY = atanf(temp) * 57.29578f;

    // AngleZ: Z 轴与重力方向的夹角
    temp = sqrtf((float)p->acc_x * p->acc_x +
                 (float)p->acc_y * p->acc_y) / (float)p->acc_z;
    p->AngleZ = atanf(temp) * 57.29578f;
}

// ---- 初始化入口 ----

void imu_drv_init(void) {
    i2c_bus_init();
    qmi8658_dev = i2c_bus_add_device(QMI8658_ADDR);
    qmi8658_init();
}
