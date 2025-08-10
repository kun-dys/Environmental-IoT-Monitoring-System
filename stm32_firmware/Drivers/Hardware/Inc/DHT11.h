#ifndef __DHT11_H
#define __DHT11_H
#include "cmsis_os.h"
#include "delay.h"
#include "gpio.h"

#define DHT11_HIGH     HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin,	GPIO_PIN_SET) //输出高电平
#define DHT11_LOW      HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)//输出低电平
#define DHT11_IO_IN      HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)//读取IO口电平
                                                                                
/* 从DHT11读取数据，没有小数部分 */
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);

#endif

