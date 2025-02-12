#ifndef _LED_DRV_H
#define _LED_DRV_H

#include "platform.h"

#define LED_GPIO GPIO_NUM_4




void led_init(void);
void led_on(void);
void led_off(void);


#endif