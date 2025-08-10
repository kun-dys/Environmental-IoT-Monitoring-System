#include "delay.h"
#include "tim.h"  // 包含 htim2 的定义
#include "cmsis_os.h"  // 用于 delay_ms（如果用 FreeRTOS）

extern TIM_HandleTypeDef htim2;

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);  // 重置计数器
    while (__HAL_TIM_GET_COUNTER(&htim2) < us); // 等待计数到目标值
}

void delay_ms(uint32_t ms)
{
	//osDelay(ms);	// 使用 FreeRTOS 的延时函数
    //HAL_Delay(ms);  
	while(ms--)
    {
        delay_us(1000);  // 每次延时1ms
    }
}
