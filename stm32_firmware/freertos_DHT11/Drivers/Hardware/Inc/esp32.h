#ifndef __ESP32_H
#define __ESP32_H

#include "main.h"
#include "usart.h"
#include <stdbool.h>

// MQTT ÅäÖÃ
#define ESP32_WIFI_SSID     "OnePlus"
#define ESP32_WIFI_PASS     "15966944488"

#define MQTT_BROKER         "broker.emqx.io"
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "stm32_client_test"
#define MQTT_USERNAME       "kundys"
#define MQTT_PASSWORD       "liuyaokun1028"
#define MQTT_TOPIC_PUB      "stm32topic"

// º¯ÊýÉùÃ÷
void ESP32_Init(void);
void ESP32_ConnectWiFi(void);
void ESP32_MQTT_ConfigUser(void);
void ESP32_MQTT_ConnectServer(void);
void ESP32_MQTT_Publish(const char *topic, const char *payload);
void ESP32_SendCommand(const char *cmd);
bool ESP32_WaitFor(const char *expected, uint32_t timeout_ms);

#endif
