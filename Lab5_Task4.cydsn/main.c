#include "project.h"

static uint8_t LED_NUM[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90
};

static volatile uint8_t disp_buf[4];
static volatile uint8_t led_counter = 0;
static volatile uint32_t ms_counter = 0;

static volatile uint8_t minutes = 5;
static volatile uint8_t seconds = 0;

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

static void updateDisplay(void)
{
    disp_buf[0] = LED_NUM[minutes / 10];
    disp_buf[1] = LED_NUM[minutes % 10] & 0x7F;
    disp_buf[2] = LED_NUM[seconds / 10];
    disp_buf[3] = LED_NUM[seconds % 10];
}

CY_ISR(Timer_Int_Handler2)
{
    ms_counter++;

    if (ms_counter >= 1000)
    {
        ms_counter = 0;

        if (minutes == 0 && seconds == 0)
        {
            // відлік закінчено — мигаємо 0000
        }
        else
        {
            if (seconds == 0)
            {
                minutes--;
                seconds = 59;
            }
            else
            {
                seconds--;
            }
        }
        updateDisplay();
    }

    FourDigit74HC595_sendOneDigit(led_counter, disp_buf[led_counter]);

    led_counter++;
    if (led_counter >= 4)
        led_counter = 0;
}

int main(void)
{
    CyGlobalIntEnable;

    updateDisplay();

    Timer_Int_StartEx(Timer_Int_Handler2);
    Timer_Start();

    for (;;)
    {
    }
}