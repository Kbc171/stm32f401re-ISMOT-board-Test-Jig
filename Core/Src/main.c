#include "main.h"
#include "adc.h"
#include "fatfs.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "st7735.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

#define MLX90614_ADDR          (0x5Au << 1)
#define SD_CS_HIGH()           HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)
#define SD_CS_LOW()            HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)

extern BYTE USER_GetCardType(void);

void SystemClock_Config(void);

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
} LedPin;

static const LedPin kLeds[] = {
    {LED1_GPIO_Port, LED1_Pin}, /* PA1 */
    {LED2_GPIO_Port, LED2_Pin}, /* PB10 */
    {LED3_GPIO_Port, LED3_Pin}, /* PB0 */
    {LED4_GPIO_Port, LED4_Pin}, /* PC1 */
};
static const char *kLedNames[] = { "PA1", "PB10", "PB0", "PC1" };

static const uint8_t kTm1637Seg[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

static uint8_t g_tft_ok;
static uint8_t g_i2c_ready;
static char g_sd_msg[22] = "SD ---";
static uint16_t g_sd_color = ST7735_WHITE;
static uint32_t g_sd_n;
static char g_btn_msg[12] = "--";
static char g_key_msg[4] = "-";
static char g_touch_msg[4] = "NO";

static void UART_Print(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void TFT_Text(uint16_t x, uint16_t y, const char *s, uint16_t color)
{
  if (!g_tft_ok)
  {
    return;
  }
  ST7735_WriteString(x, y, s, Font_7x10, color, ST7735_BLACK);
}

static void TFT_Cell(uint16_t x, uint16_t y, uint8_t cols, const char *s, uint16_t color)
{
  char buf[24];

  if (!g_tft_ok)
  {
    return;
  }
  if (cols >= sizeof(buf))
  {
    cols = (uint8_t)(sizeof(buf) - 1u);
  }
  snprintf(buf, sizeof(buf), "%-*.*s", (int)cols, (int)cols, s);
  TFT_Text(x, y, buf, color);
}

static void TFT_Status(const char *s, uint16_t color)
{
  TFT_Cell(72, 2, 12, s, color);
}

static void TFT_ShowSdStatus(void)
{
  TFT_Cell(36, 83, 17, g_sd_msg, g_sd_color);
}

static void TFT_ShowInput(void)
{
  char line[24];

  snprintf(line, sizeof(line), "BTN %-5s KEY %s", g_btn_msg, g_key_msg);
  TFT_Cell(4, 104, 22, line, ST7735_CYAN);
  snprintf(line, sizeof(line), "TOUCH %s", g_touch_msg);
  TFT_Cell(4, 115, 22, line, ST7735_MAGENTA);
}

static void Draw_Dashboard(void)
{
  if (!g_tft_ok)
  {
    return;
  }

  ST7735_FillScreen(ST7735_BLACK);

  ST7735_DrawRect(0, 0, 160, 13, ST7735_YELLOW);
  TFT_Text(3, 2, "TEST JIG", ST7735_YELLOW);

  ST7735_DrawRect(0, 13, 80, 34, ST7735_GREEN);
  TFT_Text(3, 15, "TEMP", ST7735_GREEN);
  TFT_Cell(3, 26, 10, "Tobj --.-C", ST7735_WHITE);
  TFT_Cell(3, 36, 10, "Tamb --.-C", ST7735_WHITE);

  ST7735_DrawRect(80, 13, 80, 34, ST7735_CYAN);
  TFT_Text(83, 15, "LIGHT", ST7735_CYAN);
  TFT_Cell(83, 26, 10, "--.-V", ST7735_WHITE);
  TFT_Cell(83, 36, 10, "--%", ST7735_WHITE);

  ST7735_DrawRect(0, 47, 80, 33, ST7735_MAGENTA);
  TFT_Text(3, 49, "COLOUR", ST7735_MAGENTA);
  TFT_Cell(3, 60, 10, "R- G- B-", ST7735_WHITE);
  TFT_Cell(3, 70, 10, "----", ST7735_WHITE);

  ST7735_DrawRect(80, 47, 80, 33, ST7735_ORANGE);
  TFT_Text(83, 49, "ROT", ST7735_ORANGE);
  TFT_Cell(83, 60, 10, "--%", ST7735_WHITE);

  ST7735_DrawRect(0, 80, 160, 21, ST7735_YELLOW);
  TFT_Text(4, 83, "SD", ST7735_YELLOW);
  TFT_ShowSdStatus();

  ST7735_DrawRect(0, 101, 160, 27, ST7735_CYAN);
  TFT_ShowInput();
}

static void FormatTempC(char *buf, size_t n, const char *tag, int32_t tenths)
{
  uint32_t a;

  if (tenths == -9999)
  {
    snprintf(buf, n, "%s ---.-C", tag);
    return;
  }

  a = (uint32_t)(tenths < 0 ? -tenths : tenths);
  snprintf(buf, n, "%s%s%lu.%luC", tag, (tenths < 0) ? "-" : "",
           (unsigned long)(a / 10u), (unsigned long)(a % 10u));
}

static void Beep(uint32_t ms)
{
  /* Active buzzer behind an optocoupler: needs a steady DC level.
   * Fast PWM does not get through the opto and the buzzer goes silent. */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
  HAL_Delay(ms);
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

static void LedsAll(GPIO_PinState state)
{
  for (unsigned i = 0; i < (sizeof(kLeds) / sizeof(kLeds[0])); i++)
  {
    HAL_GPIO_WritePin(kLeds[i].port, kLeds[i].pin, state);
  }
}

static void RgbSet(uint8_t r, uint8_t g, uint8_t b)
{
  HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void TM1637_Delay(void)
{
  for (volatile int i = 0; i < 200; i++)
  {
  }
}

static void TM1637_Start(void)
{
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
  TM1637_Delay();
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_RESET);
}

static void TM1637_Stop(void)
{
  HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
  TM1637_Delay();
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_RESET);
  TM1637_Delay();
  HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
  TM1637_Delay();
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
}

static void TM1637_WriteByte(uint8_t b)
{
  for (int i = 0; i < 8; i++)
  {
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, (b & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
    TM1637_Delay();
    b >>= 1;
  }
  HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
  TM1637_Delay();
  HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
  TM1637_Delay();
}

static void TM1637_DisplayNumber(int num)
{
  uint8_t d[4];
  d[0] = (uint8_t)((num / 1000) % 10);
  d[1] = (uint8_t)((num / 100) % 10);
  d[2] = (uint8_t)((num / 10) % 10);
  d[3] = (uint8_t)(num % 10);

  TM1637_Start();
  TM1637_WriteByte(0x40);
  TM1637_Stop();

  TM1637_Start();
  TM1637_WriteByte(0xC0);
  for (int i = 0; i < 4; i++)
  {
    TM1637_WriteByte(kTm1637Seg[d[i]]);
  }
  TM1637_Stop();

  TM1637_Start();
  TM1637_WriteByte(0x8F);
  TM1637_Stop();
}

static void TCS3200_SetColor(uint8_t s2, uint8_t s3)
{
  HAL_GPIO_WritePin(TCS_S2_GPIO_Port, TCS_S2_Pin, s2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TCS_S3_GPIO_Port, TCS_S3_Pin, s3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint32_t TCS3200_ReadFreq(uint32_t gate_ms)
{
  uint32_t count = 0;
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < gate_ms)
  {
    if (HAL_GPIO_ReadPin(TCS_OUT_GPIO_Port, TCS_OUT_Pin) == GPIO_PIN_SET)
    {
      while (HAL_GPIO_ReadPin(TCS_OUT_GPIO_Port, TCS_OUT_Pin) == GPIO_PIN_SET)
      {
        if ((HAL_GetTick() - start) >= gate_ms)
        {
          break;
        }
      }
      count++;
    }
  }
  if (gate_ms == 0u)
  {
    return 0;
  }
  return count * (1000u / gate_ms);
}

static void TCS3200_ReadRGB(uint32_t gate_ms, uint32_t *red, uint32_t *green, uint32_t *blue)
{
  HAL_GPIO_WritePin(TCS_OE_GPIO_Port, TCS_OE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TCS_S0_GPIO_Port, TCS_S0_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TCS_S1_GPIO_Port, TCS_S1_Pin, GPIO_PIN_RESET);

  TCS3200_SetColor(0, 0);
  *red = TCS3200_ReadFreq(gate_ms);
  TCS3200_SetColor(1, 1);
  *green = TCS3200_ReadFreq(gate_ms);
  TCS3200_SetColor(0, 1);
  *blue = TCS3200_ReadFreq(gate_ms);
}

static const char *ColourName(uint32_t r, uint32_t g, uint32_t b)
{
  if ((r + g + b) == 0u)
  {
    return "NONE";
  }
  if ((r >= g) && (r >= b))
  {
    return "RED";
  }
  if ((g >= r) && (g >= b))
  {
    return "GREEN";
  }
  return "BLUE";
}

static char Keypad_Read(void)
{
  static const char map[4][3] = {
      {'1', '2', '3'},
      {'4', '5', '6'},
      {'7', '8', '9'},
      {'*', '0', '#'}
  };
  GPIO_TypeDef *row_ports[4] = {KP_ROW0_GPIO_Port, KP_ROW1_GPIO_Port, KP_ROW2_GPIO_Port, KP_ROW3_GPIO_Port};
  uint16_t row_pins[4] = {KP_ROW0_Pin, KP_ROW1_Pin, KP_ROW2_Pin, KP_ROW3_Pin};
  GPIO_TypeDef *col_ports[3] = {KP_COL0_GPIO_Port, KP_COL1_GPIO_Port, KP_COL2_GPIO_Port};
  uint16_t col_pins[3] = {KP_COL0_Pin, KP_COL1_Pin, KP_COL2_Pin};

  for (int col = 0; col < 3; col++)
  {
    for (int c = 0; c < 3; c++)
    {
      HAL_GPIO_WritePin(col_ports[c], col_pins[c], GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(col_ports[col], col_pins[col], GPIO_PIN_RESET);
    HAL_Delay(1);
    for (int row = 0; row < 4; row++)
    {
      if (HAL_GPIO_ReadPin(row_ports[row], row_pins[row]) == GPIO_PIN_RESET)
      {
        for (int c = 0; c < 3; c++)
        {
          HAL_GPIO_WritePin(col_ports[c], col_pins[c], GPIO_PIN_SET);
        }
        return map[row][col];
      }
    }
  }
  for (int c = 0; c < 3; c++)
  {
    HAL_GPIO_WritePin(col_ports[c], col_pins[c], GPIO_PIN_SET);
  }
  return 0;
}

static uint16_t MLX90614_Read16(uint8_t reg)
{
  uint8_t data[3] = {0};
  if (HAL_I2C_Mem_Read(&hi2c3, MLX90614_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 2, 50) != HAL_OK)
  {
    return 0;
  }
  return (uint16_t)((data[1] << 8) | data[0]);
}

static int32_t MLX90614_TempTenths(uint8_t reg)
{
  uint16_t raw = MLX90614_Read16(reg);
  if (raw == 0u)
  {
    return -9999;
  }
  return (int32_t)raw / 5 - 2732;
}

static void Test_Leds(void)
{
  char line[48];

  UART_Print("[LEDS] discrete: PA1, PB10, PB0, PC1 (Led_check)\r\n");
  TFT_Status("LEDS chase", ST7735_YELLOW);
  for (int pass = 0; pass < 2; pass++)
  {
    for (unsigned i = 0; i < (sizeof(kLeds) / sizeof(kLeds[0])); i++)
    {
      LedsAll(GPIO_PIN_RESET);
      HAL_GPIO_WritePin(kLeds[i].port, kLeds[i].pin, GPIO_PIN_SET);
      snprintf(line, sizeof(line), "[LEDS] ON %s\r\n", kLedNames[i]);
      UART_Print(line);
      snprintf(line, sizeof(line), "LED %s ON", kLedNames[i]);
      TFT_Status(line, ST7735_GREEN);
      HAL_Delay(220);
    }
  }
  LedsAll(GPIO_PIN_RESET);
  UART_Print("[LEDS] all 4 discrete LEDs should have blinked\r\n");
  TFT_Status("LEDS done", ST7735_GREEN);
}

static void Test_Rgb(void)
{
  UART_Print("[RGB ] PC0=R  PC2=G  PC3=B (RGB project)\r\n");
  TFT_Status("RGB LED", ST7735_YELLOW);

  UART_Print("[RGB ] RED PC0\r\n");
  TFT_Status("RGB RED PC0", ST7735_RED);
  RgbSet(1, 0, 0);
  HAL_Delay(450);

  UART_Print("[RGB ] GREEN PC2\r\n");
  TFT_Status("RGB GREEN PC2", ST7735_GREEN);
  RgbSet(0, 1, 0);
  HAL_Delay(450);

  UART_Print("[RGB ] BLUE PC3\r\n");
  TFT_Status("RGB BLUE PC3", ST7735_CYAN);
  RgbSet(0, 0, 1);
  HAL_Delay(450);

  UART_Print("[RGB ] WHITE all\r\n");
  TFT_Status("RGB WHITE", ST7735_WHITE);
  RgbSet(1, 1, 1);
  HAL_Delay(450);

  RgbSet(0, 0, 0);
  UART_Print("[RGB ] done\r\n");
  TFT_Status("RGB done", ST7735_GREEN);
}

static void Test_Relays(void)
{
  UART_Print("[RLY1] PD2 ON (listen for click)\r\n");
  TFT_Status("RELAY1 PD2 ON", ST7735_YELLOW);
  HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_SET);
  HAL_Delay(700);
  HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_RESET);
  UART_Print("[RLY1] PD2 OFF\r\n");
  TFT_Status("RELAY1 PD2 OFF", ST7735_GREEN);
  HAL_Delay(300);

  UART_Print("[RLY2] PB12 ON (listen for click)\r\n");
  TFT_Status("RELAY2 PB12 ON", ST7735_YELLOW);
  HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_SET);
  HAL_Delay(700);
  HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_RESET);
  UART_Print("[RLY2] PB12 OFF\r\n");
  TFT_Status("RELAY2 PB12 OFF", ST7735_GREEN);
  HAL_Delay(200);
}

static void Test_Buzzer(void)
{
  UART_Print("[BUZZ] PA4 two beeps (DC through optocoupler)\r\n");
  Beep(250);
  HAL_Delay(120);
  Beep(250);
  UART_Print("[BUZZ] done\r\n");
}

static void Test_Clock(void)
{
  UART_Print("[CLK ] TM1637 8888 then 1234 on PA8/PC9\r\n");
  TM1637_DisplayNumber(8888);
  HAL_Delay(400);
  TM1637_DisplayNumber(1234);
  UART_Print("[CLK ] PASS (4-digit display should show 1234)\r\n");
}

static void Test_Tft(void)
{
  UART_Print("[TFT ] ST7735 SPI1 PA5/6/7 CS=PB6 DC=PA9 RST=PC7 landscape\r\n");
  ST7735_Init();
  g_tft_ok = 1;
  ST7735_FillRect(0, 0, 53, 128, ST7735_RED);
  ST7735_FillRect(53, 0, 54, 128, ST7735_GREEN);
  ST7735_FillRect(107, 0, 53, 128, ST7735_BLUE);
  HAL_Delay(400);
  Draw_Dashboard();
  UART_Print("[TFT ] PASS landscape dashboard\r\n");
}

static void Sd_Poll(void)
{
  FRESULT res;
  char dbg[48];
  uint8_t dummy = 0xFF;

  g_sd_n++;
  (void)f_mount(NULL, USERPath, 0);

  SD_CS_HIGH();
  for (int i = 0; i < 10; i++)
  {
    HAL_SPI_Transmit(&hspi3, &dummy, 1, 10);
  }

  res = f_mount(&USERFatFS, USERPath, 1);
  if (res == FR_OK)
  {
    uint8_t ctype = USER_GetCardType();

    res = f_open(&USERFile, "log.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (res == FR_OK)
    {
      snprintf(dbg, sizeof(dbg), "Hello from STM32F401! n=%lu\r\n",
               (unsigned long)g_sd_n);
      f_puts(dbg, &USERFile);
      f_close(&USERFile);
      snprintf(g_sd_msg, sizeof(g_sd_msg), "SD PASS #%lu", (unsigned long)g_sd_n);
      g_sd_color = ST7735_GREEN;
      snprintf(dbg, sizeof(dbg), "[LIVE] SD PASS n=%lu type=0x%02X\r\n",
               (unsigned long)g_sd_n, ctype);
      UART_Print(dbg);
    }
    else
    {
      snprintf(g_sd_msg, sizeof(g_sd_msg), "SD WR FAIL #%lu", (unsigned long)g_sd_n);
      g_sd_color = ST7735_RED;
      snprintf(dbg, sizeof(dbg), "[LIVE] SD write FAIL %d n=%lu\r\n",
               (int)res, (unsigned long)g_sd_n);
      UART_Print(dbg);
    }
  }
  else
  {
    snprintf(g_sd_msg, sizeof(g_sd_msg), "SD FAIL #%lu", (unsigned long)g_sd_n);
    g_sd_color = ST7735_RED;
    snprintf(dbg, sizeof(dbg), "[LIVE] SD mount FAIL (%d) n=%lu\r\n",
             (int)res, (unsigned long)g_sd_n);
    UART_Print(dbg);
  }

  TFT_ShowSdStatus();
}

static void Test_Sd(void)
{
  UART_Print("[SD  ] SPI3 PC10/11/12 CS=PA15 (SD_card project, live)\r\n");
  USER_SdSetVerbose(1);
  Sd_Poll();
  USER_SdSetVerbose(0);
}

static void Test_Adc(void)
{
  char line[64];
  uint32_t rot = ADC_ReadChannel(ADC_CHANNEL_0);
  uint32_t light = ADC_ReadChannel(ADC_CHANNEL_9);
  uint32_t rot_mv = rot * 3300u / 4095u;
  uint32_t light_mv = light * 3300u / 4095u;

  snprintf(line, sizeof(line), "[ROT ] PA0 ADC=%lu  %lu.%02luV  %lu%%\r\n",
           (unsigned long)rot,
           (unsigned long)(rot_mv / 1000u), (unsigned long)((rot_mv % 1000u) / 10u),
           (unsigned long)(rot * 100u / 4095u));
  UART_Print(line);
  snprintf(line, sizeof(line), "[LITE] PB1 ADC=%lu  %lu.%02luV  %lu%%\r\n",
           (unsigned long)light,
           (unsigned long)(light_mv / 1000u), (unsigned long)((light_mv % 1000u) / 10u),
           (unsigned long)(light * 100u / 4095u));
  UART_Print(line);
  snprintf(line, sizeof(line), "%lu.%02luV",
           (unsigned long)(light_mv / 1000u), (unsigned long)((light_mv % 1000u) / 10u));
  TFT_Cell(83, 26, 10, line, ST7735_WHITE);
  snprintf(line, sizeof(line), "%lu%%", (unsigned long)(light * 100u / 4095u));
  TFT_Cell(83, 36, 10, line, ST7735_WHITE);
  snprintf(line, sizeof(line), "%lu%%", (unsigned long)(rot * 100u / 4095u));
  TFT_Cell(83, 60, 10, line, ST7735_WHITE);
}

static void Test_Temp(void)
{
  char line[64];

  UART_Print("[TEMP] MLX90614 I2C3 PA8/PC9 addr 0x5A\r\n");
  HAL_GPIO_DeInit(TM1637_CLK_Port, TM1637_CLK_Pin);
  HAL_GPIO_DeInit(TM1637_DIO_Port, TM1637_DIO_Pin);
  MX_I2C3_Init();

  if (HAL_I2C_IsDeviceReady(&hi2c3, MLX90614_ADDR, 3, 50) != HAL_OK)
  {
    UART_Print("[TEMP] FAIL not found on I2C3\r\n");
    TFT_Cell(3, 26, 10, "Tobj FAIL", ST7735_RED);
    g_i2c_ready = 0;
    return;
  }

  g_i2c_ready = 1;
  int32_t amb = MLX90614_TempTenths(0x06);
  int32_t obj = MLX90614_TempTenths(0x07);
  FormatTempC(line, sizeof(line), "[TEMP] amb=", amb);
  UART_Print(line);
  UART_Print("\r\n");
  FormatTempC(line, sizeof(line), "[TEMP] obj=", obj);
  UART_Print(line);
  UART_Print("\r\n");
  FormatTempC(line, sizeof(line), "Tobj ", obj);
  TFT_Cell(3, 26, 10, line, ST7735_WHITE);
  FormatTempC(line, sizeof(line), "Tamb ", amb);
  TFT_Cell(3, 36, 10, line, ST7735_WHITE);
}

static void Test_Colour(void)
{
  char line[80];
  uint32_t red, green, blue;

  UART_Print("[COLR] TCS3200: hold a coloured object in front of the sensor\r\n");
  TCS3200_ReadRGB(80, &red, &green, &blue);
  snprintf(line, sizeof(line), "[COLR] R=%lu G=%lu B=%lu  seen=%s\r\n",
           (unsigned long)red, (unsigned long)green, (unsigned long)blue,
           ColourName(red, green, blue));
  UART_Print(line);
  snprintf(line, sizeof(line), "R%lu G%lu B%lu",
           (unsigned long)red, (unsigned long)green, (unsigned long)blue);
  TFT_Cell(3, 60, 10, line, ST7735_WHITE);
  TFT_Cell(3, 70, 10, ColourName(red, green, blue), ST7735_YELLOW);
}

static void Draw_LiveHeader(void)
{
  TFT_Status("LIVE", ST7735_GREEN);
  TFT_ShowSdStatus();
  TFT_ShowInput();
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI3_Init();
  MX_FATFS_Init();

  UART_Print("\r\n\r\n======== BOARD TEST JIG ========\r\n");
  UART_Print("STM32F401RE carrier - one-shot hardware check\r\n");
  UART_Print("Open this COM port at 115200 8N1\r\n\r\n");

  /* SD card first, same order as the working SD_card firmware */
  Test_Sd();

  MX_SPI1_Init();
  MX_ADC1_Init();

  Test_Clock();
  Test_Tft();
  TFT_ShowSdStatus();
  Test_Leds();
  Test_Rgb();
  Test_Relays();
  Test_Buzzer();
  Test_Adc();
  Test_Temp();
  Test_Colour();
  Draw_LiveHeader();

  UART_Print("\r\n[LIVE] press buttons, keypad, touch pad\r\n");
  UART_Print("[LIVE] rotate pot / cover light / point temp sensor\r\n");
  UART_Print("[LIVE] SD remounts and writes log.txt about once a second\r\n");
  UART_Print("================================\r\n");

  uint32_t last_status = 0;
  uint32_t last_sd = 0;
  char last_key = 0;
  uint32_t last_key_tick = 0;

  while (1)
  {
    char msg[96];

    if (HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin) == GPIO_PIN_SET)
    {
      UART_Print("[BTN ] DOWN\r\n");
      Beep(80);
      snprintf(g_btn_msg, sizeof(g_btn_msg), "DOWN");
      TFT_ShowInput();
      HAL_Delay(180);
    }
    if (HAL_GPIO_ReadPin(BTN_UP_GPIO_Port, BTN_UP_Pin) == GPIO_PIN_SET)
    {
      UART_Print("[BTN ] UP\r\n");
      Beep(80);
      snprintf(g_btn_msg, sizeof(g_btn_msg), "UP");
      TFT_ShowInput();
      HAL_Delay(180);
    }
    if (HAL_GPIO_ReadPin(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin) == GPIO_PIN_SET)
    {
      UART_Print("[BTN ] LEFT\r\n");
      Beep(80);
      snprintf(g_btn_msg, sizeof(g_btn_msg), "LEFT");
      TFT_ShowInput();
      HAL_Delay(180);
    }
    if (HAL_GPIO_ReadPin(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin) == GPIO_PIN_SET)
    {
      UART_Print("[BTN ] RIGHT\r\n");
      Beep(80);
      snprintf(g_btn_msg, sizeof(g_btn_msg), "RIGHT");
      TFT_ShowInput();
      HAL_Delay(180);
    }

    char key = Keypad_Read();
    if (key != 0 && (key != last_key || (HAL_GetTick() - last_key_tick) > 250u))
    {
      snprintf(msg, sizeof(msg), "[KEY ] %c\r\n", key);
      UART_Print(msg);
      Beep(40);
      snprintf(msg, sizeof(msg), "KEY %c", key);
      g_key_msg[0] = key;
      g_key_msg[1] = 0;
      TFT_ShowInput();
      last_key = key;
      last_key_tick = HAL_GetTick();
    }
    else if (key == 0)
    {
      last_key = 0;
    }

    uint8_t touched = (HAL_GPIO_ReadPin(TOUCH_GPIO_Port, TOUCH_Pin) == GPIO_PIN_SET);
    if ((touched && g_touch_msg[0] != 'Y') || (!touched && g_touch_msg[0] != 'N'))
    {
      snprintf(g_touch_msg, sizeof(g_touch_msg), "%s", touched ? "YES" : "NO");
      TFT_ShowInput();
    }

    if ((HAL_GetTick() - last_sd) >= 1000u)
    {
      Sd_Poll();
      last_sd = HAL_GetTick();
    }

    if ((HAL_GetTick() - last_status) >= 500u)
    {
      uint32_t rot = ADC_ReadChannel(ADC_CHANNEL_0);
      uint32_t light = ADC_ReadChannel(ADC_CHANNEL_9);
      uint32_t light_mv = light * 3300u / 4095u;
      uint32_t light_pct = light * 100u / 4095u;
      int32_t amb = -9999;
      int32_t obj = -9999;

      uint32_t red = 0, green = 0, blue = 0;

      if (g_i2c_ready)
      {
        amb = MLX90614_TempTenths(0x06);
        obj = MLX90614_TempTenths(0x07);
      }

      TCS3200_ReadRGB(40, &red, &green, &blue);

      FormatTempC(msg, sizeof(msg), "[LIVE] amb=", amb);
      UART_Print(msg);
      UART_Print("  ");
      FormatTempC(msg, sizeof(msg), "obj=", obj);
      UART_Print(msg);
      UART_Print("\r\n");
      snprintf(msg, sizeof(msg),
               "[LIVE] light ADC=%lu  %lu.%02luV  %lu%%  rot=%lu%%\r\n",
               (unsigned long)light,
               (unsigned long)(light_mv / 1000u),
               (unsigned long)((light_mv % 1000u) / 10u),
               (unsigned long)light_pct,
               (unsigned long)(rot * 100u / 4095u));
      UART_Print(msg);
      snprintf(msg, sizeof(msg), "[LIVE] colour R=%lu G=%lu B=%lu  %s\r\n",
               (unsigned long)red, (unsigned long)green, (unsigned long)blue,
               ColourName(red, green, blue));
      UART_Print(msg);

      FormatTempC(msg, sizeof(msg), "Tobj ", obj);
      TFT_Cell(3, 26, 10, msg, ST7735_WHITE);
      FormatTempC(msg, sizeof(msg), "Tamb ", amb);
      TFT_Cell(3, 36, 10, msg, ST7735_WHITE);
      snprintf(msg, sizeof(msg), "%lu.%02luV",
               (unsigned long)(light_mv / 1000u),
               (unsigned long)((light_mv % 1000u) / 10u));
      TFT_Cell(83, 26, 10, msg, ST7735_WHITE);
      snprintf(msg, sizeof(msg), "%lu%%", (unsigned long)light_pct);
      TFT_Cell(83, 36, 10, msg, ST7735_WHITE);
      snprintf(msg, sizeof(msg), "%lu%%",
               (unsigned long)(rot * 100u / 4095u));
      TFT_Cell(83, 60, 10, msg, ST7735_WHITE);
      snprintf(msg, sizeof(msg), "R%lu G%lu B%lu",
               (unsigned long)red, (unsigned long)green, (unsigned long)blue);
      TFT_Cell(3, 60, 10, msg, ST7735_WHITE);
      TFT_Cell(3, 70, 10, ColourName(red, green, blue), ST7735_YELLOW);

      last_status = HAL_GetTick();
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
