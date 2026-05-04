#include "project.h"

static uint8_t LED_NUM[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90,
    0xBF, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E, 0x7F
};

#define SEG_S    0x92
#define SEG_U    0xC1
#define SEG_n    0xAB
#define SEG_I    0xF9
#define SEG_OFF  0xFF

#define STATE_NORMAL   0
#define STATE_INPUT    1
#define STATE_SUCCESS  2
#define STATE_DENIED   3

static volatile uint8_t disp_buf[8];
static volatile uint8_t disp_count = 2;
static volatile uint8_t led_counter = 0;
static volatile uint32_t blink_timer = 0;

static uint8_t state = STATE_NORMAL;
static uint8_t entered[3];
static uint8_t input_pos = 0;

static const uint8_t PASSWORD[3] = {1, 2, 3};

static void (*COLUMN_x_SetDriveMode[3])(uint8_t) = {
    COLUMN_0_SetDriveMode, COLUMN_1_SetDriveMode, COLUMN_2_SetDriveMode
};
static void (*COLUMN_x_Write[3])(uint8_t) = {
    COLUMN_0_Write, COLUMN_1_Write, COLUMN_2_Write
};
static uint8 (*ROW_x_Read[4])(void) = {
    ROW_0_Read, ROW_1_Read, ROW_2_Read, ROW_3_Read
};

static const int8_t KEYMAP[4][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
    {10, 0, 11}
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

    uint8_t show = 1;
    if (state == STATE_SUCCESS)
        show = (blink_timer / 200) % 2 == 0;
    else if (state == STATE_DENIED)
        show = (blink_timer / 500) % 2 == 0;

    if (show)
        FourDigit74HC595_sendOneDigit(led_counter, disp_buf[led_counter]);
    else
    {
        FourDigit74HC595_sendData(0xFF);
        FourDigit74HC595_sendData(0xFF);
        Pin_Latch_Write(1);
        Pin_Latch_Write(0);
    }

    led_counter++;
    if (led_counter >= disp_count)
        led_counter = 0;
}

static void setDisplay31(void)
{
    disp_buf[0] = 0x86; // E
    disp_buf[1] = 0xAB; // n
    disp_buf[2] = 0x87; // t
    disp_buf[3] = 0x86; // E
    disp_buf[4] = 0xAF; // r
    disp_count = 5;
}

static void setDisplaySuccess(void)
{
    disp_buf[0] = SEG_S;
    disp_buf[1] = SEG_U;
    disp_buf[2] = LED_NUM[13]; // C
    disp_buf[3] = LED_NUM[13]; // C
    disp_buf[4] = LED_NUM[15]; // E
    disp_buf[5] = SEG_S;
    disp_buf[6] = SEG_S;
    disp_count = 7;
}

static void setDisplayDenied(void)
{
    disp_buf[0] = LED_NUM[14]; // d
    disp_buf[1] = LED_NUM[15]; // E
    disp_buf[2] = SEG_n;       // n
    disp_buf[3] = SEG_I;       // I
    disp_buf[4] = LED_NUM[15]; // E
    disp_buf[5] = LED_NUM[14]; // d
    disp_count = 6;
}

static int8_t scanKeypad(void)
{
    for (int c = 0; c < 3; c++)
    {
        COLUMN_x_SetDriveMode[c](COLUMN_0_DM_STRONG);
        COLUMN_x_Write[c](0);
        for (int r = 0; r < 4; r++)
        {
            if (ROW_x_Read[r]() == 0)
            {
                COLUMN_x_SetDriveMode[c](COLUMN_0_DM_DIG_HIZ);
                return KEYMAP[r][c];
            }
        }
        COLUMN_x_SetDriveMode[c](COLUMN_0_DM_DIG_HIZ);
    }
    return -1;
}

int main(void)
{
    CyGlobalIntEnable;

    for (int c = 0; c < 3; c++)
        COLUMN_x_SetDriveMode[c](COLUMN_0_DM_DIG_HIZ);

    setDisplay31();

    Timer_Int_StartEx(Timer_Int_Handler2);
    Timer_Start();

    int8_t last_key = -1;

    for (;;)
    {
        int8_t key = scanKeypad();

        if (key != last_key && key != -1)
        {
            if (state == STATE_SUCCESS || state == STATE_DENIED)
            {
                if (key >= 0 && key <= 9)
                {
                    state = STATE_INPUT;
                    input_pos = 0;
                    for (int i = 0; i < 8; i++) disp_buf[i] = SEG_OFF;
                    disp_count = 3;
                    led_counter = 0;
                    entered[input_pos] = key;
                    disp_buf[input_pos] = LED_NUM[key];
                    input_pos++;
                }
            }
            else if (state == STATE_NORMAL)
            {
                if (key >= 0 && key <= 9)
                {
                    state = STATE_INPUT;
                    input_pos = 0;
                    for (int i = 0; i < 8; i++) disp_buf[i] = SEG_OFF;
                    disp_count = 3;
                    led_counter = 0;
                    entered[input_pos] = key;
                    disp_buf[input_pos] = LED_NUM[key];
                    input_pos++;
                }
            }
            else if (state == STATE_INPUT)
            {
                if (key >= 0 && key <= 9 && input_pos < 3)
                {
                    entered[input_pos] = key;
                    disp_buf[input_pos] = LED_NUM[key];
                    input_pos++;
                }
                else if (key == 11) // #
                {
                    uint8_t correct = (input_pos == 3);
                    if (correct)
                        for (int i = 0; i < 3; i++)
                            if (entered[i] != PASSWORD[i]) { correct = 0; break; }

                    blink_timer = 0;
                    led_counter = 0;

                    if (correct)
                    {
                        state = STATE_SUCCESS;
                        setDisplaySuccess();
                    }
                    else
                    {
                        state = STATE_DENIED;
                        setDisplayDenied();
                    }
                }
            }
        }

        last_key = key;
        CyDelay(20);
    }
}