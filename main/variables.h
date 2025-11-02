#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/adc.h"

/* Default WiFi credentials */
#define DEFAULT_SSID "WIFI-ESP"
#define DEFAULT_PASSWORD "87654321"

/* ST7735 connected Pins*/
#define TFT_CS          5
#define TFT_RST        21 
#define TFT_DC         22

/* ESP32 touch pin*/
#define TOUCH_PAD_GPIO4_CHANNEL TOUCH_PAD_NUM0 

/* MQ-2 ADC Pin*/
#define MQ2_ADC_CHANNEL ADC1_CHANNEL_4

/* TFT message buffer parameters*/
#define TFT_MSG_SIZE 128
#define TFT_QUEUE_LENGTH 5

/* TCP config*/
#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3
#define CONFIG_EXAMPLE_IPV4         1

/* Display TCP message size*/
#define TEXT_TCP_X 0
#define TEXT_TCP_Y 0
#define TEXT_TCP_WIDTH 128
#define TEXT_TCP_HEIGHT 80

/* Display MQ2 message size*/
#define TEXT_MQ2_X 0
#define TEXT_MQ2_Y 80
#define TEXT_MQ2_WIDTH 128
#define TEXT_MQ2_HEIGHT 80

#ifdef __cplusplus
extern "C" {
#endif

/** Table for all message types
 * 
 * MSG_TCP = TCP messages
 * MSG_MQ2 = MQ2 sensor messages
 * 
*/
typedef enum {
    MSG_TCP,
    MSG_MQ2
} display_msg_type_t;

typedef struct {
    display_msg_type_t type;
    union {
        char tcp_msg[TFT_MSG_SIZE];
        struct { float lpg, co, smoke; } mq2;
    } data;
} display_msg_t;

/** @brief Handle for message Buffer */
extern QueueHandle_t displayQueue;

#ifdef __cplusplus
}
#endif



