#include "project.h"

static uint8_t LED_NUM[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90,
    0xBF, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E, 0x7F
};

static void FourDigit74HC595_sendData(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        Pin_DO_Write((data & (0x80 >> i)) ? 1 : 0);
        Pin_CLK_Write(1);
        Pin_CLK_Write(0);
    }
}

static void FourDigit74HC595_sendOneDigit(uint8_t position, uint8_t digit, uint8_t dot)
{
    if (position >= 8)
    {
        FourDigit74HC595_sendData(0xFF);
        FourDigit74HC595_sendData(0xFF);
    }
    FourDigit74HC595_sendData(0xFF & ~(1 << position));
    if (dot)
        FourDigit74HC595_sendData(LED_NUM[digit] & 0x7F);
    else
        FourDigit74HC595_sendData(LED_NUM[digit]);
    Pin_Latch_Write(1);
    Pin_Latch_Write(0);
}

static volatile uint8 led_counter = 0;

CY_ISR(Timer_Int_Handler2)
{
    const uint8 digits[2] = {3, 1};
    FourDigit74HC595_sendOneDigit(led_counter, digits[led_counter], 0);
    led_counter++;
    if (led_counter > 1)
        led_counter = 0;
}

int main(void)
{
    CyGlobalIntEnable;
    Timer_Int_StartEx(Timer_Int_Handler2);
    Timer_Start();

    for (;;)
    {
    }
}