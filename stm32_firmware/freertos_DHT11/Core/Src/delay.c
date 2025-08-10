#include "delay.h"
#include "tim.h"  // ���� htim2 �Ķ���
#include "cmsis_os.h"  // ���� delay_ms������� FreeRTOS��

extern TIM_HandleTypeDef htim2;

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);  // ���ü�����
    while (__HAL_TIM_GET_COUNTER(&htim2) < us); // �ȴ�������Ŀ��ֵ
}

void delay_ms(uint32_t ms)
{
	//osDelay(ms);	// ʹ�� FreeRTOS ����ʱ����
    //HAL_Delay(ms);  
	while(ms--)
    {
        delay_us(1000);  // ÿ����ʱ1ms
    }
}
