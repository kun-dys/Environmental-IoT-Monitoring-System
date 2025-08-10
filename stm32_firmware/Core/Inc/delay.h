#ifndef __DELAY_H__
#define __DELAY_H__

#include "main.h"

void delay_us(uint16_t us);
void delay_ms(uint32_t ms); // 可使用 osDelay 或 HAL_Delay

#endif
