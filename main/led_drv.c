#include "led_drv.h"

void led_init(void) {
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << GPIO_NUM_15),
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void led_breath_init(void) {
  // 初始化定时器
  ledc_timer_config_t ledc_timer = {
      .clk_cfg = LEDC_AUTO_CLK,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = LEDC_TIMER_12_BIT,
      .freq_hz = 5000,
  };
  ledc_timer_config(&ledc_timer);
  // 初始化pwm通道
  ledc_channel_config_t ledc_channel = {
      .channel = LEDC_CHANNEL_0,
      .duty = 0,
      .gpio_num = GPIO_NUM_15,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_sel = LEDC_TIMER_0,
  };
  ledc_channel_config(&ledc_channel);
  // 渐变
  ledc_fade_func_install(0);
}

void set_effect_1(void) {
  ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 2000);
  ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE);
  ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 2000);
  ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE);
}