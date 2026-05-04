#include "project.h"

static uint8_t LED_NUM[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90,
    0xBF, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E, 0x7F
};

#define SEG_n    0xAB
#define SEG_I    0xF9
#define SEG_OFF  0xFF

static volatile uint8_t disp_buf[8];
static volatile uint8_t disp_count = 8;
static volatile uint8_t led_counter = 0;
static volatile uint32_t blink_timer = 0;

static const uint8_t SCROLL_SYMS[4] = {0xAB, 0xF9, 0x88, 0x88};
static void FourDigit74HC595_sendData(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        Pin_DO_Write((data & (0x80 >> i)) ? 1 : 0);
        Pin_CLK_Write(1);
        Pin_CLK_Write(0);
    }
}

static void FourDigit74HC595_sendOneDigit(uint8_t position, uint8_t segment_code)
{
    FourDigit74HC595_sendData(0xFF & ~(1 << position));
    FourDigit74HC595_sendData(segment_code);
    Pin_Latch_Write(1);
    Pin_Latch_Write(0);
}

CY_ISR(Timer_Int_Handler2)
{
    blink_timer++;

    uint8_t offset = (blink_timer / 500) % 4;
    disp_buf[0] = SCROLL_SYMS[(4 - offset) % 4];
    disp_buf[1] = SCROLL_SYMS[(5 - offset) % 4];
    disp_buf[2] = SCROLL_SYMS[(6 - offset) % 4];
    disp_buf[3] = SCROLL_SYMS[(7 - offset) % 4];

    FourDigit74HC595_sendOneDigit(led_counter, disp_buf[led_counter]);

    led_counter++;
    if (led_counter >= disp_count)
        led_counter = 0;
}

int main(void)
{
    CyGlobalIntEnable;

        disp_buf[4] = 0x88;
        disp_buf[5] = SEG_OFF;
        disp_buf[6] = SEG_OFF;
        disp_buf[7] = SEG_OFF;


    Timer_Int_StartEx(Timer_Int_Handler2);
    Timer_Start();

    for (;;)
    {
    }
}