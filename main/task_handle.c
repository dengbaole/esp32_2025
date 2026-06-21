
#include "platform.h"

#define NUM0_BIT BIT0
#define NUM1_BIT BIT1
#define KEY_BIT BIT2

static const char* TAG = "APP";
static EventGroupHandle_t test_event;

// 按键状态结构体
typedef struct {
  int press_count;                 // 按键次数
  int key_pressed;                 // 按键状态：0未按下，1按下中
  TickType_t press_time;           // 按键按下时间
  TickType_t release_time;         // 按键释放时间
  esp_timer_handle_t reset_timer;  // 定时器句柄
} key_state_t;

static key_state_t key_state = {0};

// 定时器回调函数：清空按键次数
static void reset_press_count_callback(void* arg) {
  key_state.press_count = 0;
  ESP_LOGI(TAG, "Press count reset to 0 after 0.4s release");
}

void key_handle(void* pvParameters) {
  // 配置GPIO
  gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                           .mode = GPIO_MODE_INPUT,
                           .pin_bit_mask = (1ULL << GPIO_NUM_0),
                           .pull_down_en = 0,
                           .pull_up_en = 1};

  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "GPIO config failed");
    vTaskDelete(NULL);
  }

  // 创建定时器用于0.4秒后重置
  const esp_timer_create_args_t timer_args = {
      .callback = &reset_press_count_callback, .arg = NULL, .dispatch_method = ESP_TIMER_TASK, .name = "reset_timer"};

  ret = esp_timer_create(&timer_args, &key_state.reset_timer);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Timer creation failed");
    vTaskDelete(NULL);
  }

  TickType_t last_debounce_time = 0;
  const TickType_t debounce_delay = pdMS_TO_TICKS(20);

  while (1) {
    int level = gpio_get_level(GPIO_NUM_0);
    if (level == 0 && key_state.key_pressed == 0) {
      // 检测到下降沿（按键按下）
      TickType_t current_time = xTaskGetTickCount();
      if ((current_time - last_debounce_time) > debounce_delay) {
        key_state.key_pressed = 1;
        key_state.press_time = current_time;

        // 如果有正在运行的定时器，停止它
        esp_timer_stop(key_state.reset_timer);

        key_state.press_count++;
        if (key_state.press_count > 5) {
          key_state.press_count = 5;
        }

        ESP_LOGI(TAG, "Key pressed %d times", key_state.press_count);
        xEventGroupSetBits(test_event, KEY_BIT);

        last_debounce_time = current_time;
      }
    } else if (level == 1 && key_state.key_pressed == 1) {
      // 按键释放
      key_state.key_pressed = 0;
      key_state.release_time = xTaskGetTickCount();

      // 启动定时器，0.4秒后清空按键次数
      esp_timer_start_once(key_state.reset_timer, 400000);  // 400ms = 400000微秒
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// 或者使用FreeRTOS定时器方案（更简单，不需要esp_timer）
void key_handle_simple(void* pvParameters) {
  gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                           .mode = GPIO_MODE_INPUT,
                           .pin_bit_mask = (1ULL << GPIO_NUM_0),
                           .pull_down_en = 0,
                           .pull_up_en = 1};

  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "GPIO config failed");
    vTaskDelete(NULL);
  }

  int press_count = 0;
  int key_pressed = 0;
  TickType_t last_release_time = 0;
  const TickType_t reset_delay = pdMS_TO_TICKS(400);  // 0.4秒

  while (1) {
    int level = gpio_get_level(GPIO_NUM_0);

    if (level == 0 && key_pressed == 0) {
      // 按键按下
      key_pressed = 1;

      press_count++;
      if (press_count > 5) {
        press_count = 5;
      }

      ESP_LOGI(TAG, "Key pressed %d times", press_count);
      xEventGroupSetBits(test_event, KEY_BIT);

      // 添加消抖延迟
      vTaskDelay(pdMS_TO_TICKS(20));
    } else if (level == 1 && key_pressed == 1) {
      // 按键释放
      key_pressed = 0;
      last_release_time = xTaskGetTickCount();
    }

    // 检查是否需要重置按键次数
    if (!key_pressed && press_count > 0) {
      TickType_t current_time = xTaskGetTickCount();
      if ((current_time - last_release_time) > reset_delay) {
        // 按键释放超过0.4秒，清空次数
        press_count = 0;
        ESP_LOGI(TAG, "Press count reset to 0 after 0.4s release");
        // 为了确保不会重复触发，这里增加一个标志
        last_release_time = current_time;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
t_sQMI8658 QMI8658;  // 定义QMI8658结构体变量
void task_handle(void* pvParameters) {
  ESP_ERROR_CHECK(bsp_i2c_init());                // 初始化I2C总线
  ESP_LOGI(TAG, "I2C initialized successfully");  // 输出I2C初始化成功的信息
  qmi8658_init();
  while (1) {
    qmi8658_fetch_angleFromAcc(&QMI8658);  // 获取XYZ轴的倾角
    // 输出XYZ轴的倾角
    ESP_LOGI(TAG, "angle_x = %.1f  angle_y = %.1f angle_z = %.1f", QMI8658.AngleX, QMI8658.AngleY, QMI8658.AngleZ);
    xEventGroupSetBits(test_event, NUM0_BIT);
    vTaskDelay(pdMS_TO_TICKS(1000));

    xEventGroupSetBits(test_event, NUM1_BIT);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task_handle2(void* pvParameters) {
  EventBits_t ev;

  while (1) {
    ev = xEventGroupWaitBits(test_event, NUM0_BIT | NUM1_BIT | KEY_BIT,
                             pdTRUE,   // 清除所有等待的事件位
                             pdFALSE,  // 不需要所有位同时到达
                             portMAX_DELAY);

    if (ev & NUM0_BIT) {
      ESP_LOGI(TAG, "Received NUM0_BIT");
    }
    if (ev & NUM1_BIT) {
      ESP_LOGI(TAG, "Received NUM1_BIT");
    }
    if (ev & KEY_BIT) {
      ESP_LOGI(TAG, "Received KEY_BIT");
    }
  }
}

void task_init(void) {
  // test_event = xEventGroupCreate();
  // if (test_event == NULL) {
  //     ESP_LOGE(TAG, "Failed to create event group!");
  //     return;
  // }

  // ESP_LOGI(TAG, "Creating tasks...");

  // xTaskCreate(task_handle, "task_handle", 4096, NULL, 5, NULL);
  // xTaskCreate(task_handle2, "task_handle2", 4096, NULL, 5, NULL);
  // // 使用简单版本的按键处理函数
  // xTaskCreate(key_handle_simple, "key_handle", 4096, NULL, 6, NULL);

  // ESP_LOGI(TAG, "Tasks created successfully");
}