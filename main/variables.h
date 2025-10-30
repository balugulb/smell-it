#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


/* ST7735 connected Pins*/
#define TFT_CS          5
#define TFT_RST        21 
#define TFT_DC         22

/* ESP32 touch pin*/
#define TOUCH_PAD_GPIO4_CHANNEL TOUCH_PAD_NUM0 

/* TFT message buffer parameters*/
#define TFT_MSG_SIZE 128
#define TFT_QUEUE_LENGTH 5

/* TCP config*/
#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3
#define CONFIG_EXAMPLE_IPV4         1

/* TFT text field parameters*/
#define TEXT_X 20
#define TEXT_Y 12
#define TEXT_W 88
#define TEXT_H 24


#ifdef __cplusplus
extern "C" {
#endif


/** @brief Handle for message buffer */
extern QueueHandle_t tftQueue;

/** @brief Init msg queue */
void init_tft_queue();

#ifdef __cplusplus
}
#endif