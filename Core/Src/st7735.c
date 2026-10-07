#include "st7735.h"
#include "spi.h"

#define CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)

#define DC_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET)
#define DC_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET)

#define RST_LOW()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET)
#define RST_HIGH() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET)

/* MADCTL: MY | MV | BGR = landscape, BGR order (same BGR as working portrait 0xC8) */
#define ST7735_MADCTL_LANDSCAPE  0xA8

static void ST7735_SendCmd(uint8_t cmd)
{
    DC_LOW();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    CS_HIGH();
}

static void ST7735_SendData(uint8_t data)
{
    DC_HIGH();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, 10);
    CS_HIGH();
}

static void ST7735_BeginWrite(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t cmd;
    uint8_t caset[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };
    uint8_t raset[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };

    CS_LOW();

    cmd = 0x2A;
    DC_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    HAL_SPI_Transmit(&hspi1, caset, 4, 10);

    cmd = 0x2B;
    DC_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    HAL_SPI_Transmit(&hspi1, raset, 4, 10);

    cmd = 0x2C;
    DC_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
}

static void ST7735_EndWrite(void)
{
    CS_HIGH();
}

void ST7735_Init(void)
{
    // Hardware reset sequence
    RST_HIGH();
    HAL_Delay(10);
    RST_LOW();
    HAL_Delay(10);
    RST_HIGH();
    HAL_Delay(120);

    // Software Reset
    ST7735_SendCmd(0x01);
    HAL_Delay(150);

    // Sleep Out
    ST7735_SendCmd(0x11);
    HAL_Delay(120);

    // Frame Rate Control (Normal mode)
    ST7735_SendCmd(0xB1);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Frame Rate Control (Idle mode)
    ST7735_SendCmd(0xB2);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Frame Rate Control (Partial mode)
    ST7735_SendCmd(0xB3);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Display Inversion Control
    ST7735_SendCmd(0xB4);
    ST7735_SendData(0x07);

    // Power Control 1
    ST7735_SendCmd(0xC0);
    ST7735_SendData(0xA2);
    ST7735_SendData(0x02);
    ST7735_SendData(0x84);

    // Power Control 2
    ST7735_SendCmd(0xC1);
    ST7735_SendData(0xC5);

    // Power Control 3
    ST7735_SendCmd(0xC2);
    ST7735_SendData(0x0A);
    ST7735_SendData(0x00);

    // Power Control 4
    ST7735_SendCmd(0xC3);
    ST7735_SendData(0x8A);
    ST7735_SendData(0x2A);

    // Power Control 5
    ST7735_SendCmd(0xC4);
    ST7735_SendData(0x8A);
    ST7735_SendData(0xEE);

    // VCOM Control 1
    ST7735_SendCmd(0xC5);
    ST7735_SendData(0x0E);

    // Memory Access Control — landscape (160x128)
    ST7735_SendCmd(0x36);
    ST7735_SendData(ST7735_MADCTL_LANDSCAPE);

    // Pixel Format Set (16-bit color)
    ST7735_SendCmd(0x3A);
    ST7735_SendData(0x05);

    // Gamma Set
    ST7735_SendCmd(0x26);
    ST7735_SendData(0x01);

    // Positive Gamma Correction
    ST7735_SendCmd(0xE0);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x1A);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x18);
    ST7735_SendData(0x2F);
    ST7735_SendData(0x28);
    ST7735_SendData(0x20);
    ST7735_SendData(0x22);
    ST7735_SendData(0x1F);
    ST7735_SendData(0x1B);
    ST7735_SendData(0x23);
    ST7735_SendData(0x37);
    ST7735_SendData(0x00);
    ST7735_SendData(0x07);
    ST7735_SendData(0x02);
    ST7735_SendData(0x10);

    // Negative Gamma Correction
    ST7735_SendCmd(0xE1);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x1B);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x17);
    ST7735_SendData(0x33);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x29);
    ST7735_SendData(0x2E);
    ST7735_SendData(0x30);
    ST7735_SendData(0x30);
    ST7735_SendData(0x39);
    ST7735_SendData(0x3F);
    ST7735_SendData(0x00);
    ST7735_SendData(0x07);
    ST7735_SendData(0x03);
    ST7735_SendData(0x10);

    // Column Address Set (landscape width 160)
    ST7735_SendCmd(0x2A);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x9F);

    // Row Address Set (landscape height 128)
    ST7735_SendCmd(0x2B);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x7F);

    // Normal Display Mode On
    ST7735_SendCmd(0x13);
    HAL_Delay(10);

    // Display ON
    ST7735_SendCmd(0x29);
    HAL_Delay(10);
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillRect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data[2];

    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
    {
        return;
    }

    data[0] = (uint8_t)(color >> 8);
    data[1] = (uint8_t)color;
    ST7735_BeginWrite(x, y, x, y);
    HAL_SPI_Transmit(&hspi1, data, 2, 10);
    ST7735_EndWrite();
}

void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint8_t buf[160];
    uint16_t n = 0;
    const uint8_t *glyph;

    if ((x + font.width) > ST7735_WIDTH || (y + font.height) > ST7735_HEIGHT)
    {
        return;
    }
    if (ch >= 'a' && ch <= 'z')
    {
        ch = (char)(ch - 32);
    }
    if (ch < 0x20 || ch > 0x5F)
    {
        ch = '?';
    }

    glyph = font.data + ((uint8_t)ch - 0x20) * font.width;

    for (uint8_t row = 0; row < font.height; row++)
    {
        for (uint8_t col = 0; col < font.width; col++)
        {
            uint16_t pix = (glyph[col] & (1u << row)) ? color : bgcolor;
            buf[n++] = (uint8_t)(pix >> 8);
            buf[n++] = (uint8_t)pix;
        }
    }

    ST7735_BeginWrite(x, y, (uint16_t)(x + font.width - 1u),
                      (uint16_t)(y + font.height - 1u));
    HAL_SPI_Transmit(&hspi1, buf, n, 50);
    ST7735_EndWrite();
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    while (*str)
    {
        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
}

void ST7735_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0)
    {
        return;
    }
    ST7735_FillRect(x, y, w, 1, color);
    ST7735_FillRect(x, (uint16_t)(y + h - 1u), w, 1, color);
    ST7735_FillRect(x, y, 1, h, color);
    ST7735_FillRect((uint16_t)(x + w - 1u), y, 1, h, color);
}

void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t data[2];
    uint32_t pixels;

    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
    {
        return;
    }
    if ((x + w) > ST7735_WIDTH)
    {
        w = (uint16_t)(ST7735_WIDTH - x);
    }
    if ((y + h) > ST7735_HEIGHT)
    {
        h = (uint16_t)(ST7735_HEIGHT - y);
    }
    if (w == 0 || h == 0)
    {
        return;
    }

    data[0] = (uint8_t)(color >> 8);
    data[1] = (uint8_t)color;
    pixels = (uint32_t)w * (uint32_t)h;

    ST7735_BeginWrite(x, y, (uint16_t)(x + w - 1u), (uint16_t)(y + h - 1u));
    for (uint32_t i = 0; i < pixels; i++)
    {
        HAL_SPI_Transmit(&hspi1, data, 2, 10);
    }
    ST7735_EndWrite();
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* image)
{
    if (x + w > ST7735_WIDTH) w = ST7735_WIDTH - x;
    if (y + h > ST7735_HEIGHT) h = ST7735_HEIGHT - y;
    
    ST7735_BeginWrite(x, y, (uint16_t)(x + w - 1u), (uint16_t)(y + h - 1u));

    for (uint32_t i = 0; i < ((uint32_t)w * (uint32_t)h); i++)
    {
        uint16_t pixel = image[i];
        uint8_t data[2] = { (uint8_t)(pixel >> 8), (uint8_t)pixel };
        HAL_SPI_Transmit(&hspi1, data, 2, 10);
    }

    ST7735_EndWrite();
}
