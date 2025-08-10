#include "esp32.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"

extern osMutexId_t MutexHandle;

extern UART_HandleTypeDef huart1;
#define ESP32_UART &huart1

#define RX_BUFFER_SIZE 512
static char rx_buffer[RX_BUFFER_SIZE];
static uint16_t rx_index = 0;

void ESP32_SendCommand(const char *cmd)
{
	osMutexAcquire(MutexHandle, osWaitForever);

	char full_cmd[256];
	snprintf(full_cmd, sizeof(full_cmd), "%s\r\n", cmd);
	HAL_UART_Transmit(ESP32_UART, (uint8_t *)full_cmd, strlen(full_cmd), HAL_MAX_DELAY);

	osMutexRelease(MutexHandle);
}

bool ESP32_WaitFor(const char *expected, uint32_t timeout_ms)
{
    osMutexAcquire(MutexHandle, osWaitForever);

    memset(rx_buffer, 0, RX_BUFFER_SIZE);
    rx_index = 0;

    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < timeout_ms)
    {
        uint8_t ch;
        if (HAL_UART_Receive(ESP32_UART, &ch, 1, 10) == HAL_OK)
        {
            if (rx_index < RX_BUFFER_SIZE - 1)
            {
                rx_buffer[rx_index++] = ch;
                rx_buffer[rx_index] = '\0';

                if (strstr(rx_buffer, expected)) {
                    printf("[ESP32 Response]:\r\n%s\r\n", rx_buffer);
                    osMutexRelease(MutexHandle);
                    return true;
                }
            }
        }
    }

    // timeout，打印当前已接收内容
    printf("[ESP32 Timeout Response]:\r\n%s\r\n", rx_buffer);
    osMutexRelease(MutexHandle);
    return false;
}


void ESP32_Init(void)
{
	ESP32_SendCommand("AT+RST");
	if (!ESP32_WaitFor("ready", 1000)) {
		printf("ESP32 Reset failed\r\n");
	}
	
	ESP32_SendCommand("ATE0");
	if (!ESP32_WaitFor("OK", 1000)) {
		printf("ESP32 ATE0 failed\r\n");
	}
	
//	ESP32_SendCommand("AT+CWMODE=1");  // Station 模式
//	if (!ESP32_WaitFor("OK", 1000)) {
//		printf("Set mode failed\r\n");
//	}
}

void ESP32_ConnectWiFi(void)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ESP32_WIFI_SSID, ESP32_WIFI_PASS);
	printf("%s\r\n", cmd);

	ESP32_SendCommand(cmd);
	if (!ESP32_WaitFor("OK", 2000)) {
		printf("WiFi connect failed\r\n");
	}
}

void ESP32_MQTT_ConfigUser(void)
{
    char cmd[256];
    // 构造用户配置指令
    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"" MQTT_CLIENT_ID "\",\"" MQTT_USERNAME "\",\"" MQTT_PASSWORD "\",0,0,\"\"");
    printf("Sending MQTT user config: %s\r\n", cmd);
    
    ESP32_SendCommand(cmd);
    // 等待配置成功响应，延长超时时间至3秒
    if (ESP32_WaitFor("OK", 3000)) {
        printf("MQTT user config success\r\n");
    } else {
        printf("MQTT user config failed\r\n");
    }
}

// 连接到MQTT服务器
void ESP32_MQTT_ConnectServer(void)
{
    char cmd[256];
    // 构造服务器连接指令
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,0", MQTT_BROKER, MQTT_PORT);
    printf("Sending MQTT connect: %s\r\n", cmd);
    
    ESP32_SendCommand(cmd);
    // 等待连接成功响应，网络操作超时设为5秒
    if (ESP32_WaitFor("OK", 3000)) {
        printf("MQTT connect server success\r\n");
    } else {
        printf("MQTT connect server failed\r\n");
    }
}

void ESP32_MQTT_Publish(const char *topic, const char *payload)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",0,0", topic, payload);
	printf("MQTT Publish: %s\r\n", cmd);
	ESP32_SendCommand(cmd);
	if (ESP32_WaitFor("OK", 2000)) {
        printf("MQTT Publish success\r\n");
    } else {
        printf("MQTT Publish failed\r\n");
    }
}
