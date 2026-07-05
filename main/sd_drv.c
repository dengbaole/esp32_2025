#include "sd_drv.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"

static const char *TAG = "sd_drv";

static sdmmc_card_t *card = NULL;

esp_err_t sd_drv_init(void)
{
    esp_err_t ret;

    // SDMMC 主机配置
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // SDMMC 卡槽配置：1-bit 模式 + 指定引脚 + 内部上拉
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk   = SD_CLK_GPIO;
    slot_config.cmd   = SD_CMD_GPIO;
    slot_config.d0    = SD_D0_GPIO;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // FAT 挂载配置：挂载失败时自动格式化
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                   &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 打印 SD 卡信息
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
    return ESP_OK;
}

void sd_drv_deinit(void)
{
    if (card) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
        card = NULL;
        ESP_LOGI(TAG, "SD card unmounted");
    }
}
