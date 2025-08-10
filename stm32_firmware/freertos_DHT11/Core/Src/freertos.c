/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include "delay.h"
#include "DHT11.h"
#include "esp32.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
#define INIT_DONE_FLAG  (1 << 0)

typedef struct {
    uint8_t temp;
    uint8_t humi;
} DHT11_Data_t;
/* USER CODE END Variables */
/* Definitions for DHT11Task */
osThreadId_t DHT11TaskHandle;
const osThreadAttr_t DHT11Task_attributes = {
  .name = "DHT11Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for MqttSendTask */
osThreadId_t MqttSendTaskHandle;
const osThreadAttr_t MqttSendTask_attributes = {
  .name = "MqttSendTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for InitTask */
osThreadId_t InitTaskHandle;
const osThreadAttr_t InitTask_attributes = {
  .name = "InitTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for dht11Queue */
osMessageQueueId_t dht11QueueHandle;
const osMessageQueueAttr_t dht11Queue_attributes = {
  .name = "dht11Queue"
};
/* Definitions for Mutex */
osMutexId_t MutexHandle;
const osMutexAttr_t Mutex_attributes = {
  .name = "Mutex"
};
/* Definitions for xInitEventGroup */
osEventFlagsId_t xInitEventGroupHandle;
const osEventFlagsAttr_t xInitEventGroup_attributes = {
  .name = "xInitEventGroup"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDHT11Task(void *argument);
void StartSendTask(void *argument);
void StartInitTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of Mutex */
  MutexHandle = osMutexNew(&Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of dht11Queue */
  dht11QueueHandle = osMessageQueueNew (8, sizeof(DHT11_Data_t), &dht11Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of DHT11Task */
  DHT11TaskHandle = osThreadNew(StartDHT11Task, NULL, &DHT11Task_attributes);

  /* creation of MqttSendTask */
  MqttSendTaskHandle = osThreadNew(StartSendTask, NULL, &MqttSendTask_attributes);

  /* creation of InitTask */
  InitTaskHandle = osThreadNew(StartInitTask, NULL, &InitTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of xInitEventGroup */
  xInitEventGroupHandle = osEventFlagsNew(&xInitEventGroup_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDHT11Task */
/**
* @brief Function implementing the DHT11Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDHT11Task */
void StartDHT11Task(void *argument)
{
  /* USER CODE BEGIN StartDHT11Task */
  /* Infinite loop */
    // 等待 InitTask 设置完成标志
    osEventFlagsWait(xInitEventGroupHandle,
                     INIT_DONE_FLAG,
                     osFlagsWaitAny,
                     osWaitForever);
	
    DHT11_Data_t dht11_data;
    for(;;)
    {
        if (DHT11_Read_Data(&dht11_data.temp, &dht11_data.humi))
        {
            osMessageQueuePut(dht11QueueHandle, &dht11_data, 0, 0);
        }
		else
		{
		  printf("Sensor read failed\n");
		}
		osDelay(1000);
    }
  /* USER CODE END StartDHT11Task */
}

/* USER CODE BEGIN Header_StartSendTask */
/**
* @brief Function implementing the MqttSendTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSendTask */
void StartSendTask(void *argument)
{
  /* USER CODE BEGIN StartSendTask */
  /* Infinite loop */
    // 等待 InitTask 设置完成标志
    osEventFlagsWait(xInitEventGroupHandle,
                     INIT_DONE_FLAG,
                     osFlagsWaitAny,
                     osWaitForever);
	DHT11_Data_t received_data;
	for(;;)
	{
		if (osMessageQueueGet(dht11QueueHandle, &received_data, NULL, osWaitForever) == osOK)
		{
			char payload[128];
			snprintf(payload, sizeof(payload), "{\\\"temp\\\":%d\\, \\\"humi\\\":%d}", received_data.temp, received_data.humi);
			//printf("MQTT Publish: %s\r\n", payload);
			ESP32_MQTT_Publish(MQTT_TOPIC_PUB, payload);
			osDelay(2000);
		}
	}

  /* USER CODE END StartSendTask */
}

/* USER CODE BEGIN Header_StartInitTask */
/**
* @brief Function implementing the InitTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInitTask */
void StartInitTask(void *argument)
{
  /* USER CODE BEGIN StartInitTask */
  /* Infinite loop */
	ESP32_Init();
	osDelay(1);
	ESP32_ConnectWiFi();
	osDelay(1);
	ESP32_MQTT_ConfigUser();
	ESP32_MQTT_ConnectServer();

	printf("ESP32 Init Done\r\n");

    // 设置事件标志，通知其他任务
    osEventFlagsSet(xInitEventGroupHandle, INIT_DONE_FLAG);

	vTaskDelete(NULL); // 初始化完成后自毁
  /* USER CODE END StartInitTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

