#ifndef IMU_DRV_H
#define IMU_DRV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// QMI8658 姿态数据结构
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyr_x;
    int16_t gyr_y;
    int16_t gyr_z;
    float AngleX;
    float AngleY;
    float AngleZ;
} t_sQMI8658;

// I2C 引脚配置
#define IMU_I2C_SDA       GPIO_NUM_1
#define IMU_I2C_SCL       GPIO_NUM_2

// 初始化 IMU（I2C + QMI8658）
void imu_drv_init(void);

// 读取加速度计数据并计算姿态角
void imu_drv_read_angle(t_sQMI8658 *p);

#ifdef __cplusplus
}
#endif

#endif // IMU_DRV_H
