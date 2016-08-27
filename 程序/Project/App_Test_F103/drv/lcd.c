/*
 * ETM12880K1����(TL0350)
 * eleqian 2014-9-13
 */

#include "base.h"
#include "bitband.h"
#include "math.h"
#include "timebase.h"
#include "lcd.h"
#include "font.h"
#include <string.h>

/*-----------------------------------*/

// �򿪺�ʱʹ�ÿ��ٻ����㷨
#define LCD_FAST_DRAW_VLINE
#define LCD_FAST_DRAW_HLINE

// �򿪺�ʱʹ��λ������LCD����
#define LCD_BUF_BITBAND

/*-----------------------------------*/

// LCD����
static uint8_t gLCDBuf[LCD_PAGES][LCD_WIDTH][LCD_BITS_PER_PIXEL];

/*-----------------------------------*/
// LCD�˿ڲ�������
// ע����Ҫ�ͳ�ʼ������һ���޸�

// CS: PB12
#define LCD_CS_0()      GPIOB->BRR = GPIO_Pin_12
#define LCD_CS_1()      GPIOB->BSRR = GPIO_Pin_12

// RST: ϵͳRST��
#define LCD_RST_0()     //GPIOA->BRR = GPIO_Pin_4
#define LCD_RST_1()     //GPIOA->BSRR = GPIO_Pin_4

// RS: PB14
#define LCD_RS_0()      GPIOB->BRR = GPIO_Pin_14
#define LCD_RS_1()      GPIOB->BSRR = GPIO_Pin_14

// BL: PA8
#define LCD_BL_OFF()    GPIOA->BRR = GPIO_Pin_8
#define LCD_BL_ON()     GPIOA->BSRR = GPIO_Pin_8

/*-----------------------------------*/

// ��ʼ���˿�
static void LCD_HAL_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // SPI2����
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    // ʹ��SPI2
    SPI_Cmd(SPI2, ENABLE);

    // ����BL(PA8)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    LCD_BL_OFF();

    // ����CS(PB12)��RS(PB14)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    LCD_CS_1();
    LCD_RS_0();

    // ����SCK(PB13)��MOSI(PB15)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //LCD_RST_1();
}

/*-----------------------------------*/

// дLCD
#define LCD_Write(dat)    \
    do {    \
        while (!(SPI2->SR & SPI_I2S_FLAG_TXE));     \
        SPI2->DR = dat;    \
    } while (0)

#define LCD_WriteRAM(d)     LCD_Write(d)
#define LCD_WriteCmd(c)     LCD_Write(c)

// ��ʼд����
static void LCD_WriteBegin_RAM(void)
{
    LCD_RS_1();
    LCD_CS_0();
}

// ��ʼд����
static void LCD_WriteBegin_CMD(void)
{
    LCD_RS_0();
    LCD_CS_0();
}

// ����дLCD
static void LCD_WriteEnd(void)
{
    // �ȴ�SPI�������
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);
    //while (SPI2->SR & SPI_I2S_FLAG_BSY);
    LCD_CS_1();
}

/*-----------------------------------*/

// ��ʼ��
void LCD_Init(void)
{
    LCD_HAL_Config();

    //LCD_RST_0();            // �͵�ƽ��λ
    //delay_us(10);
    //LCD_RST_1();            // ��λ���
    //delay_us(10);

    //delay_us(10);
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0xe2);     // ��λ
    LCD_WriteEnd();
    delay_us(10);

    LCD_WriteBegin_CMD();

    LCD_WriteCmd(0xc8);     // ��ɨ��˳��(SHL=0xc0/0xc8)
    LCD_WriteCmd(0xa1);     // ��ɨ��˳��(ADC=0xa0/0xa1)

    LCD_WriteCmd(0x40);     // ��ʼRAM�У�0
    LCD_WriteCmd(0x00);

    LCD_WriteCmd(0x44);     // ��ʼCOM��COM24
    LCD_WriteCmd(0x18);

    LCD_WriteCmd(0x48);     // duty ratio��80
    LCD_WriteCmd(0x50);

    /*LCD_WriteCmd(0x4c);     // N-line inversion: K = 5
    LCD_WriteCmd(0x0e);*/

    LCD_WriteCmd(0xab);     // ��������

    LCD_WriteCmd(0x65);     // DC-DC��ѹ������4

    LCD_WriteCmd(0x26);     // �ֵ��Աȶȣ������÷�Χ0x20��0x27

    LCD_WriteCmd(0x81);     // ΢���Աȶ�
    LCD_WriteCmd(0x2f);     // �����÷�Χ0x00��0x3f

    LCD_WriteCmd(0x55);     // ƫѹ�ȣ�1/10(bias)

    LCD_WriteCmd(0x90);     // FRC��4��PWM��9

    LCD_WriteCmd(0x88);     // ��ռ�ձȣ�0/36
    LCD_WriteCmd(0x00);
    LCD_WriteCmd(0x89);
    LCD_WriteCmd(0x00);

    LCD_WriteCmd(0x8a);     // ����ռ�ձȣ�16/36
    LCD_WriteCmd(0x44);
    LCD_WriteCmd(0x8b);
    LCD_WriteCmd(0x44);

    LCD_WriteCmd(0x8c);     // ���ռ�ձȣ�27/36
    LCD_WriteCmd(0x99);
    LCD_WriteCmd(0x8d);
    LCD_WriteCmd(0x90);

    LCD_WriteCmd(0x8e);     // ��ռ�ձȣ�36/36
    LCD_WriteCmd(0x99);
    LCD_WriteCmd(0x8f);
    LCD_WriteCmd(0x99);

    LCD_WriteCmd(0x2c);     // ��ѹ����1(VC)
    delay_ms(20);
    LCD_WriteCmd(0x2e);     // ��ѹ����2(VR)
    delay_ms(5);
    LCD_WriteCmd(0x2f);     // ��ѹ����3(VF)
    delay_ms(5);

    LCD_WriteCmd(0xaf);     // ����ʾ

    //LCD_WriteCmd(0xa5);     // ȫ��

    LCD_WriteEnd();
}

/*-----------------------------------*/

// ���⿪��
void LCD_LightOnOff(BOOL isOn)
{
    if (isOn) {
        LCD_BL_ON();
    } else {
        LCD_BL_OFF();
    }
}

// ��ʾ����
void LCD_DisplayOnOff(BOOL isOn)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0xAE | (isOn ? 1 : 0));
    LCD_WriteEnd();
}

// �͹���ģʽ����
void LCD_PowerSaveOnOff(BOOL isOn)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0xA9 | (isOn ? 1 : 0));
    LCD_WriteEnd();
}

// ������ʼ��ʾ��(��ֱ����)
void LCD_SetStartLine(uint8_t line)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0x40 | (line & 0x3f));
    LCD_WriteEnd();
}

// ������ʾ��(x)
void LCD_SetColumn(uint8_t column)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0x10 | ((column >> 4) & 0x07));
    LCD_WriteCmd(column & 0x0f);
    LCD_WriteEnd();
}

// ������ʾҳ(y/8)
void LCD_SetPage(uint8_t page)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0xb0 | (page & 0x0f));
    LCD_WriteEnd();
}

// ͬʱ������ʾ�к���ʾҳ
void LCD_GotoXY(uint8_t column, uint8_t page)
{
    LCD_WriteBegin_CMD();
    LCD_WriteCmd(0x10 | ((column >> 4) & 0x07));
    LCD_WriteCmd(column & 0x0f);
    LCD_WriteCmd(0xb0 | (page & 0x0f));
    LCD_WriteEnd();
}

/*-----------------------------------*/

// ��ʾ��ɫͼƬ���߶ȱ���Ϊ8�ı���
// ������ͼƬָ�룬x��y/8��w��h/8
void LCD_DisplayPicture8_1Bpp(const void *pic, uint8_t x, uint8_t y8, uint8_t w, uint8_t h8)
{
    const uint8_t *dptr;
    size_t i, j;

    if ((y8 + h8) > LCD_PAGES) {
        return;
    }
    if ((x + w) > LCD_WIDTH) {
        return;
    }

    dptr = (const uint8_t *)pic;

    for (i = 0; i < h8; i++) {
        LCD_GotoXY(x, y8 + i);
        LCD_WriteBegin_RAM();
        for (j = 0; j < w; j++) {
            LCD_WriteRAM(*dptr);
            LCD_WriteRAM(*dptr++);
        }
        LCD_WriteEnd();
    }
}

// ��ʾ4ɫͼƬ���߶ȱ���Ϊ8�ı���
// ������ͼƬָ�룬x��y/8��w��h/8
void LCD_DisplayPicture8_2Bpp(const void *pic, uint8_t x, uint8_t y8, uint8_t w, uint8_t h8)
{
    const uint8_t *dptr;
    size_t i, j;

    if ((y8 + h8) > LCD_PAGES) {
        return;
    }
    if ((x + w) > LCD_WIDTH) {
        return;
    }

    dptr = (const uint8_t *)pic;

    for (i = 0; i < h8; i++) {
        LCD_GotoXY(x, y8 + i);
        LCD_WriteBegin_RAM();
        for (j = 0; j < w; j++) {
            LCD_WriteRAM(*dptr++);
            LCD_WriteRAM(*dptr++);
        }
        LCD_WriteEnd();
    }
}

/*-----------------------------------*/

// ��ʾ5*8Ӣ���ַ�
// ������Ascii�ַ���x��y/8
void LCD_DisplayAscii5x8(char ch, uint8_t x, uint8_t y8)
{
    const uint8_t *dptr;

    if (ch < ' ') {
        ch = ' ';
    } else if (ch > '~') {
        return;
    }

    dptr = &AsciiFont_5x8[(ch - ' ') * 5];

    LCD_DisplayPicture8_1Bpp(dptr, x, y8, 5, 1);
}

// ��ʾ�ָ�8���ַ���
// �������ַ�����x��y/8
void LCD_DisplayString8(const char *str, uint8_t x, uint8_t y8)
{
    while (*str != '\0') {
        LCD_DisplayAscii5x8(*str++, x, y8);
        x += 5;

        if (x >= LCD_WIDTH) {
            x = 0;
            y8++;
            if (y8 >= LCD_PAGES) {
                y8 = 0;
            }
        }
    }
}

/*-----------------------------------*/
// LCD����ģʽ

// ˢ��֡
void LCD_Refresh(void)
{
    size_t i, j;
    uint8_t *pBuf;

    pBuf = (uint8_t *)gLCDBuf;
    for (i = 0; i < LCD_PAGES; i++) {
        LCD_GotoXY(0, i);
        LCD_WriteBegin_RAM();
        for (j = 0; j < LCD_WIDTH; j++) {
            LCD_WriteRAM(*pBuf++);
            LCD_WriteRAM(*pBuf++);
        }
        LCD_WriteEnd();
    }
}

// ����
void LCD_Clear(void)
{
    memset(gLCDBuf, 0x0, sizeof(gLCDBuf));
}

// ���Ƶ�
void LCD_DrawPoint(uint8_t x, uint8_t y, uint8_t gray)
{
#ifdef LCD_BUF_BITBAND
    uintptr_t addr;
#else
    uint8_t *pBuf;
    uint8_t yBit;
#endif  //LCD_BUF_BITBAND

    // ��ʱȥ������߻���Ч��
    /*if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }*/

#ifdef LCD_BUF_BITBAND
    addr = BITBAND_SRAM((uintptr_t)gLCDBuf[y >> 3][x], y & 0x7);
    MEM_ADDR(addr) = gray >> 1;
    MEM_ADDR(addr + 32) = gray & 0x1;
#else
    yBit = y & 0x7;
    pBuf = &gLCDBuf[y >> 3][x][0];
    *pBuf = (*pBuf & ~(0x1 << yBit)) | ((gray >> 1) << yBit);
    pBuf++;
    *pBuf = (*pBuf & ~(0x1 << yBit)) | ((gray & 0x1) << yBit);
#endif //LCD_BUF_BITBAND
}

// ������ֱ��
void LCD_DrawVLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t gray)
{
#ifdef LCD_FAST_DRAW_VLINE
    // �����㷨���ϲ�ͬһ�ֽ���λ����
    uint8_t *pBuf;
    size_t remPixels;           // ʣ��Ҫ���Ƶ�pixel��
    size_t yBit;                // ���ֽڶ����bit��
    uint8_t pixel8mask;         // �����޸ĵ�bits��Ĥ
    uint8_t pixel8h, pixel8l;   // һ��8��pixel����ֵ

    if (y0 > y1) {
        uint8_t temp;

        temp = y1;
        y1 = y0;
        y0 = temp;
    }
    
    pixel8h = -(gray >> 1);
    pixel8l = -(gray & 0x1);
    remPixels = y1 - y0 + 1;
    pBuf = &gLCDBuf[y0 >> 3][x][0];

    // ���ƿ�ʼ�ķ��ֽڶ��벿��
    yBit = y0 & 0x7;
    if (yBit > 0) {
        pixel8mask = (0xff >> (8 - yBit));
        if (yBit + remPixels < 8) {
            pixel8mask |= (0xff << (yBit + remPixels));
            remPixels = 0;
        } else {
            remPixels -= (8 - yBit);
        }
        *pBuf = (*pBuf & pixel8mask) | (pixel8h & ~pixel8mask);
        *(pBuf + 1) = (*(pBuf + 1) & pixel8mask) | (pixel8l & ~pixel8mask);
        pBuf += LCD_WIDTH * LCD_BITS_PER_PIXEL;
    }

    if (remPixels > 0) {
        size_t numBytes = remPixels >> 3;

        // �����ֽڶ���Ĳ���
        if (numBytes > 0) {
            remPixels -= numBytes << 3;
            while (numBytes--) {
                *pBuf = pixel8h;
                *(pBuf + 1) = pixel8l;
                pBuf += LCD_WIDTH * LCD_BITS_PER_PIXEL;
            }
        }

        // ���ƽ����ķ��ֽڶ��벿��
        pixel8mask = (0xff << remPixels);
        *pBuf = (*pBuf & pixel8mask) | (pixel8h & ~pixel8mask);
        *(pBuf + 1) = (*(pBuf + 1) & pixel8mask) | (pixel8l & ~pixel8mask);
    }
#else //!LCD_FAST_DRAW_VLINE
    // ��׼�㷨
    if (y0 > y1) {
        uint8_t temp;

        temp = y1;
        y1 = y0;
        y0 = temp;
    }

    do {
        LCD_DrawPoint(x, y0++, gray);
    } while (y1 >= y0);
#endif //LCD_FAST_DRAW_VLINE
}

// ����ˮƽ��
void LCD_DrawHLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t gray)
{
#ifdef LCD_FAST_DRAW_HLINE
    // �����㷨���ϲ�ͬһҳ��λ����
    uint16_t *pBuf;
    size_t remPixels;
    size_t yBit;
    uint16_t pixelmask;
    uint16_t pixel;

    if (x0 > x1) {
        uint8_t temp;

        temp = x1;
        x1 = x0;
        x0 = temp;
    }

    remPixels = x1 - x0 + 1;
    pBuf = (uint16_t *)&gLCDBuf[y >> 3][x0][0];
    yBit = y & 0x7;
    pixelmask =  ~((0x1 << yBit) | (0x1 << (yBit + 8)));
    pixel = ((gray >> 1) << yBit) | ((gray & 0x1) << (yBit + 8));

    do {
        *pBuf = (*pBuf & pixelmask) | pixel;
        pBuf++;
    } while (remPixels-- > 0);
#else //!LCD_FAST_DRAW_HLINE
    // ��׼�㷨
    if (x0 > x1) {
        uint8_t temp;

        temp = x1;
        x1 = x0;
        x0 = temp;
    }

    do {
        LCD_DrawPoint(x0++, y, gray);
    } while (x1 >= x0);
#endif //LCD_FAST_DRAW_HLINE
}

// �����߶�
void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t gray)
{
    int8_t dx, dy;              // ֱ��x��,y���ֵ����
    int8_t dx_sym, dy_sym;      // x��,y����������Ϊ-1ʱ��ֵ����Ϊ1ʱ��ֵ����
    int8_t dx_2, dy_2;          // dx*2,dy*2ֵ���������ڼӿ������ٶ�
    int8_t di;                  // ���߱���

    dx = x1 - x0;
    dy = y1 - y0;

    if (dx < 0) {
        dx_sym = -1;
    } else if (dx > 0) {
        dx_sym = 1;
    } else {
        LCD_DrawVLine(x0, y0, y1, gray);
        return;
    }

    if (dy > 0) {
        dy_sym = 1;
    } else if (dy < 0) {
        dy_sym = -1;
    } else {
        LCD_DrawHLine(x0, x1, y0, gray);
        return;
    }

    dx = dx_sym * dx;
    dy = dy_sym * dy;

    dx_2 = dx * 2;
    dy_2 = dy * 2;

    if (dx >= dy) {
        di = dy_2 - dx;
        while (x0 != x1) {
            LCD_DrawPoint(x0, y0, gray);
            x0 += dx_sym;
            if (di < 0) {
                di += dy_2;
            } else {
                di += dy_2 - dx_2;
                y0 += dy_sym;
            }
        }
        LCD_DrawPoint(x0, y0, gray);
    } else {
        di = dx_2 - dy;
        while (y0 != y1) {
            LCD_DrawPoint(x0, y0, gray);
            y0 += dy_sym;
            if (di < 0) {
                di += dx_2;
            } else {
                di += dx_2 - dy_2;
                x0 += dx_sym;
            }
        }
        LCD_DrawPoint(x0, y0, gray);
    }
}

// ���ƾ��ο�
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    LCD_DrawHLine(x, x + w, y, gray);
    LCD_DrawHLine(x, x + w, y + h, gray);
    LCD_DrawVLine(x, y, y + h, gray);
    LCD_DrawVLine(x + w, y, y + h, gray);
}

// ����������
void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    uint8_t x1, y1;

    x1 = x + w - 1;
    y1 = y + h - 1;

    while (x <= x1) {
        LCD_DrawVLine(x, y, y1, gray);
        x++;
    }
}

// ͸����ʾ��ɫͼƬ���߶ȱ���Ϊ8�ı���
// ������ͼƬָ�룬x��y/8��w��h/8���Ҷ�
void LCD_DrawPicture8_1Bpp(const void *pic, uint8_t x, uint8_t y8, uint8_t w, uint8_t h8, uint8_t gray)
{
    const uint8_t *dptr;
    uint8_t *pbuf;
    uint8_t gray8h, gray8l;
    size_t i, j;

    if ((y8 + h8) > LCD_PAGES) {
        return;
    }
    if ((x + w) > LCD_WIDTH) {
        return;
    }

    gray8h = -(gray >> 1);
    gray8l = -(gray & 0x1);

    dptr = (const uint8_t *)pic;

    for (i = 0; i < h8; i++) {
        pbuf = &gLCDBuf[y8 + i][x][0];
        for (j = 0; j < w; j++) {
            *pbuf = (*pbuf & ~*dptr) | (gray8h & *dptr);
            pbuf++;
            *pbuf = (*pbuf & ~*dptr) | (gray8l & *dptr);
            pbuf++;
            dptr++;
        }
    }
}

// ���Ƶ�ɫͼƬ
// ������ͼƬָ�룬x��y��w��h�� �Ҷ�
void LCD_DrawPicture_1Bpp(const void *pic, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{

}

// ����4�Ҷ�ͼƬ
// ������ͼƬָ�룬x��y��w��h
void LCD_DrawPicture_2Bpp(const void *pic, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{

}

/*-----------------------------------*/

// �����ַ�(��͸������ģʽ)
// �������ַ���x��y�����壬�Ҷ�
void LCD_DrawChar(char ch, uint8_t x, uint8_t y, uint8_t font, uint8_t gray)
{
    const uint8_t *pFont;
#ifdef LCD_BUF_BITBAND
    uintptr_t addr;
#else
    uint8_t *pBuf;
    size_t yBit;
#endif //LCD_BUF_BITBAND
    uint8_t grayh, grayl;
    uint8_t chw, chh;
    uint8_t x1, y1;
    size_t i, j;
    uint8_t dot8;

    grayh = (gray >> 1);
    grayl = (gray & 0x1);

    switch (font) {
    case FONT_SIZE_3X6:
        chw = 3;
        chh = 8;
        pFont = &AsciiFont_3x6[(ch - 0x20) * 3];
        break;
    case FONT_SIZE_5X8:
        chw = 5;
        chh = 8;
        pFont = &AsciiFont_5x8[(ch - 0x20) * 5];
        break;
    default:
        return;
    }

    x1 = x + chw - 1;
    y1 = y + chh - 1;

    for (i = x; i <= x1; i++) {
        dot8 = *pFont++;
        for (j = y; j <= y1; j++) {
            if (dot8 & 0x1) {
#ifdef LCD_BUF_BITBAND
                addr = BITBAND_SRAM((uintptr_t)gLCDBuf[j >> 3][i], j & 0x7);
                MEM_ADDR(addr) = grayh;
                MEM_ADDR(addr + 32) = grayl;
#else
                yBit = j & 0x7;
                pBuf = (uint8_t *)gLCDBuf[j >> 3][i];
                *pBuf = (*pBuf & ~(0x1 << yBit)) | (grayh << yBit);
                pBuf++;
                *pBuf = (*pBuf & ~(0x1 << yBit)) | (grayl << yBit);
#endif //LCD_BUF_BITBAND
            }
            dot8 >>= 1;
        }
    }
}

// �����ַ���(��͸������ģʽ)
// �������ַ���x��y�����壬�Ҷ�
void LCD_DrawString(const char *str, uint8_t x, uint8_t y, uint8_t font, uint8_t gray)
{
    uint8_t dx, dy;

    switch (font) {
    case FONT_SIZE_3X6:
        dx = 4;
        dy = 8;
        break;
    case FONT_SIZE_5X8:
        dx = 6;
        dy = 8;
        break;
    default:
        return;
    }

    while (*str != '\0') {
        LCD_DrawChar(*str++, x, y, font, gray);
        x += dx;

        if (x >= LCD_WIDTH) {
            x = 0;
            y += dy;
            if (y >= LCD_HEIGHT) {
                y = 0;
            }
        }
    }
}
