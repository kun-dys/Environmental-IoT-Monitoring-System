#ifndef __DHT11_H
#define __DHT11_H
#include "cmsis_os.h"
#include "delay.h"
#include "gpio.h"

#define DHT11_HIGH     HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin,	GPIO_PIN_SET) //����ߵ�ƽ
#define DHT11_LOW      HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)//����͵�ƽ
#define DHT11_IO_IN      HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)//��ȡIO�ڵ�ƽ
                                                                                
/* ��DHT11��ȡ���ݣ�û��С������ */
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);

#endif

