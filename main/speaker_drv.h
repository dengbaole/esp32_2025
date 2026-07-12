#ifndef SPEAKER_DRV_H
#define SPEAKER_DRV_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2S 播放引脚（与录音共用 MCLK=38/BCK=14/WS=13，数据独立 DOUT=45）
#define SPK_I2S_DOUT    GPIO_NUM_45

// 初始化扬声器（ES8311 + I2S TX + PCA9557 PA使能）
void speaker_drv_init(void);

// 播放 WAV/PCM 文件
// filename: SD 卡上的文件路径，如 "/canon.pcm"
esp_err_t speaker_drv_play(const char *filename);

// 设置音量 0-100
void speaker_drv_set_volume(int vol);

// 释放资源
void speaker_drv_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
