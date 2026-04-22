#ifndef _BUTTON_DRV_H_
#define _BUTTON_DRV_H_
#include "esp_err.h"
#include "platform.h"

//按键回调函数
typedef void(*button_press_cb_t)(int gpio);

//获取电平函数
typedef int(*button_getleve_cb_t)(int gpio);

//按键配置结构体
typedef struct {
	int gpio_num;           //gpio号
	int active_level;       //按下的电平
	int long_press_time;    //长按时间
	button_getleve_cb_t getlevel_cb;  //获取电平的回调函数
	button_press_cb_t short_cb;   //短按回调函数
	button_press_cb_t long_cb;    //长按回调函数
} button_config_t;


typedef enum {
	BUTTON_RELEASE,             //按键没有按下
	BUTTON_PRESS,               //按键按下了，等待一点延时（消抖），然后触发短按回调事件，进入BUTTON_HOLD
	BUTTON_HOLD,                //按住状态，如果时间长度超过设定的超时计数，将触发长按回调函数，进入BUTTON_LONG_PRESS_HOLD
	BUTTON_LONG_PRESS_HOLD,     //此状态等待电平消失，回到BUTTON_RELEASE状态
} BUTTON_STATE;

typedef struct Button {
	button_config_t btn_cfg;    //按键配置
	BUTTON_STATE    state;      //当前状态
	int press_cnt;              //按下计数
	struct Button* next;        //下一个按键参数
} button_dev_t;

extern volatile uint16_t xl9555_button_level;

/** 设置按键事件
 * @param cfg   配置结构体
 * @return ESP_OK or ESP_FAIL
*/
esp_err_t button_event_set(button_config_t* cfg);
void button_init(void);


#endif
