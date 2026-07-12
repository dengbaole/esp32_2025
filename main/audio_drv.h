#ifndef AUDIO_DRV_H
#define AUDIO_DRV_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 I2S + ES7210 音频编解码器
void audio_drv_init(void);

// 录制 WAV 音频到 SD 卡（需先挂载 sd_drv）
// filename: 文件名（如 "/RECORD.WAV"）
// duration_sec: 录音时长（秒）
esp_err_t audio_drv_record(const char *filename, int duration_sec);

// 释放音频资源
void audio_drv_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_DRV_H
