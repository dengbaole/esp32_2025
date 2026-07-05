#ifndef SD_DRV_H
#define SD_DRV_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// SDMMC 引脚（1-bit 模式）
#define SD_CLK_GPIO     GPIO_NUM_47
#define SD_CMD_GPIO     GPIO_NUM_48
#define SD_D0_GPIO      GPIO_NUM_21

// 挂载路径
#define SD_MOUNT_POINT  "/sdcard"

// 初始化 SD 卡（挂载 FAT 文件系统）
esp_err_t sd_drv_init(void);

// 卸载 SD 卡
void sd_drv_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // SD_DRV_H
