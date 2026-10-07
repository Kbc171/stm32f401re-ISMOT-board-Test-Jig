#include "gpio.h"

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Idle levels */
  HAL_GPIO_WritePin(GPIOA, LED1_Pin | BUZZER_Pin | TCS_S2_Pin | TCS_S3_Pin | TFT_DC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, TM1637_CLK_Pin, GPIO_PIN_SET);
  /* PA15 CS starts low, same as the working SD_card project */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, LED2_Pin | LED3_Pin | TCS_OE_Pin | RELAY2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, TFT_CS_Pin | KP_COL0_Pin | KP_COL2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOC, LED4_Pin | RGB_R_Pin | RGB_G_Pin | RGB_B_Pin | TCS_S1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, TCS_S0_Pin | TFT_RST_Pin | KP_COL1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = LED1_Pin | TCS_S2_Pin | TCS_S3_Pin | TM1637_CLK_Pin;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TFT_DC_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(TFT_DC_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED2_Pin | LED3_Pin | TCS_OE_Pin |
                        KP_COL0_Pin | KP_COL2_Pin | RELAY2_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TFT_CS_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(TFT_CS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RELAY1_Pin;
  HAL_GPIO_Init(RELAY1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED4_Pin | RGB_R_Pin | RGB_G_Pin | RGB_B_Pin |
                        TCS_S0_Pin | TCS_S1_Pin | TFT_RST_Pin | KP_COL1_Pin |
                        TM1637_DIO_Pin;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Keypad rows: pulled up, active low */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Pin = KP_ROW0_Pin;
  HAL_GPIO_Init(KP_ROW0_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = KP_ROW1_Pin | KP_ROW2_Pin | KP_ROW3_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Buttons: pressed = high (matches working Push_buttons firmware) */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Pin = BTN_DOWN_Pin;
  HAL_GPIO_Init(BTN_DOWN_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = BTN_UP_Pin;
  HAL_GPIO_Init(BTN_UP_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BTN_LEFT_Pin | BTN_RIGHT_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Touch: pressed = high */
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Pin = TOUCH_Pin;
  HAL_GPIO_Init(TOUCH_GPIO_Port, &GPIO_InitStruct);

  /* TCS3200 frequency output */
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = TCS_OUT_Pin;
  HAL_GPIO_Init(TCS_OUT_GPIO_Port, &GPIO_InitStruct);
}
