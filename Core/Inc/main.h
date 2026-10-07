/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* Nucleo / ST-Link */
#define B1_Pin                 GPIO_PIN_13
#define B1_GPIO_Port           GPIOC
#define USART_TX_Pin           GPIO_PIN_2
#define USART_TX_GPIO_Port     GPIOA
#define USART_RX_Pin           GPIO_PIN_3
#define USART_RX_GPIO_Port     GPIOA
#define TMS_Pin                GPIO_PIN_13
#define TMS_GPIO_Port          GPIOA
#define TCK_Pin                GPIO_PIN_14
#define TCK_GPIO_Port          GPIOA

/* Discrete LEDs from Led_check (not the RGB pins) */
#define LED1_Pin               GPIO_PIN_1
#define LED1_GPIO_Port         GPIOA
#define LED2_Pin               GPIO_PIN_10
#define LED2_GPIO_Port         GPIOB
#define LED3_Pin               GPIO_PIN_0
#define LED3_GPIO_Port         GPIOB
#define LED4_Pin               GPIO_PIN_1
#define LED4_GPIO_Port         GPIOC

/* RGB LED from RGB project: PC0=R, PC2=G, PC3=B */
#define RGB_R_Pin              GPIO_PIN_0
#define RGB_R_GPIO_Port        GPIOC
#define RGB_G_Pin              GPIO_PIN_2
#define RGB_G_GPIO_Port        GPIOC
#define RGB_B_Pin              GPIO_PIN_3
#define RGB_B_GPIO_Port        GPIOC

/* Buzzer */
#define BUZZER_Pin             GPIO_PIN_4
#define BUZZER_GPIO_Port       GPIOA

/* Relays */
#define RELAY1_Pin             GPIO_PIN_2
#define RELAY1_GPIO_Port       GPIOD
#define RELAY2_Pin             GPIO_PIN_12
#define RELAY2_GPIO_Port       GPIOB

/* Navigation buttons */
#define BTN_DOWN_Pin           GPIO_PIN_5
#define BTN_DOWN_GPIO_Port     GPIOC
#define BTN_UP_Pin             GPIO_PIN_6
#define BTN_UP_GPIO_Port       GPIOC
#define BTN_LEFT_Pin           GPIO_PIN_8
#define BTN_LEFT_GPIO_Port     GPIOB
#define BTN_RIGHT_Pin          GPIO_PIN_9
#define BTN_RIGHT_GPIO_Port    GPIOB

/* 3x4 keypad: rows = inputs, cols = outputs */
#define KP_ROW0_Pin            GPIO_PIN_10
#define KP_ROW0_GPIO_Port      GPIOA
#define KP_ROW1_Pin            GPIO_PIN_15
#define KP_ROW1_GPIO_Port      GPIOB
#define KP_ROW2_Pin            GPIO_PIN_14
#define KP_ROW2_GPIO_Port      GPIOB
#define KP_ROW3_Pin            GPIO_PIN_13
#define KP_ROW3_GPIO_Port      GPIOB
#define KP_COL0_Pin            GPIO_PIN_3
#define KP_COL0_GPIO_Port      GPIOB
#define KP_COL1_Pin            GPIO_PIN_4
#define KP_COL1_GPIO_Port      GPIOC
#define KP_COL2_Pin            GPIO_PIN_5
#define KP_COL2_GPIO_Port      GPIOB

/* Touch */
#define TOUCH_Pin              GPIO_PIN_8
#define TOUCH_GPIO_Port        GPIOC

/* TCS3200 colour sensor */
#define TCS_OE_Pin             GPIO_PIN_7
#define TCS_OE_GPIO_Port       GPIOB
#define TCS_S0_Pin             GPIO_PIN_14
#define TCS_S0_GPIO_Port       GPIOC
#define TCS_S1_Pin             GPIO_PIN_15
#define TCS_S1_GPIO_Port       GPIOC
#define TCS_S2_Pin             GPIO_PIN_11
#define TCS_S2_GPIO_Port       GPIOA
#define TCS_S3_Pin             GPIO_PIN_12
#define TCS_S3_GPIO_Port       GPIOA
#define TCS_OUT_Pin            GPIO_PIN_4
#define TCS_OUT_GPIO_Port      GPIOB

/* TM1637 clock (GPIO bit-bang; later reused as I2C3) */
#define TM1637_CLK_Pin         GPIO_PIN_8
#define TM1637_CLK_Port        GPIOA
#define TM1637_DIO_Pin         GPIO_PIN_9
#define TM1637_DIO_Port        GPIOC

/* ST7735 TFT */
#define TFT_CS_Pin             GPIO_PIN_6
#define TFT_CS_GPIO_Port       GPIOB
#define TFT_DC_Pin             GPIO_PIN_9
#define TFT_DC_GPIO_Port       GPIOA
#define TFT_RST_Pin            GPIO_PIN_7
#define TFT_RST_GPIO_Port      GPIOC

/* SD card SPI3 */
#define SD_CS_Pin              GPIO_PIN_15
#define SD_CS_GPIO_Port        GPIOA

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
