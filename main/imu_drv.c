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

    // 等待芯片就绪
    qmi8658_read_reg(QMI8658_WHO_AM_I, &id, 1);
    while (id != 0x05) {
        vTaskDelay(pdMS_TO_TICKS(100));
        qmi8658_read_reg(QMI8658_WHO_AM_I, &id, 1);
    }
    ESP_LOGI(TAG, "QMI8658 OK");

    // 复位 + 配置
    qmi8658_write_reg(QMI8658_RESET, 0xb0);
    vTaskDelay(pdMS_TO_TICKS(10));
    qmi8658_write_reg(QMI8658_CTRL1, 0x40);  // 地址自动递增
    qmi8658_write_reg(QMI8658_CTRL7, 0x03);  // 使能 Acc + Gyr
    qmi8658_write_reg(QMI8658_CTRL2, 0x95);  // Acc: ±4g, 250Hz
    qmi8658_write_reg(QMI8658_CTRL3, 0xd5);  // Gyr: ±512dps, 250Hz
}

// ---- 读取原始数据 ----

static void qmi8658_read_acc_gyr(t_sQMI8658 *p) {
    uint8_t status;
    int16_t buf[6];

    qmi8658_read_reg(QMI8658_STATUS0, &status, 1);
    if (status & 0x03) {  // 加速度计或陀螺仪数据就绪
        qmi8658_read_reg(QMI8658_AX_L, (uint8_t *)buf, 12);
        p->acc_x = buf[0];
        p->acc_y = buf[1];
        p->acc_z = buf[2];
        p->gyr_x = buf[3];
        p->gyr_y = buf[4];
        p->gyr_z = buf[5];
    }
}

// ---- 加速度计计算姿态角 ----

void imu_drv_read_angle(t_sQMI8658 *p) {
    float temp;
    qmi8658_read_acc_gyr(p);

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
