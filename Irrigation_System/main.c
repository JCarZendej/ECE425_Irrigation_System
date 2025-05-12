/**
 * @file main.c
 * @brief DHT22 Temperature and Humidity Sensor Display on EduBase LCD
 *
 * TM4C123G LaunchPad + EduBase Board + DHT22 Sensor + Encoder
 *
 * Author: Juan Zendejas
 */
 
#include "TM4C123GH6PM.h"
#include "SysTick_Delay.h"
#include "EduBase_LCD.h"
#include "PMOD_ENC.h"
#include "Timer_0A_Interrupt.h"
#include "math.h"

#define DHT22_PIN 0x02     // PE1
#define LED_RED   0x02     // PF1 (Humidity alert LED)
#define LED_BLUE  0x04     // PF2 (Test valve LED)

// Menu state
#define MENU_MAX 2
static int main_menu_counter = 0;
static int prev_menu_counter = -1;
static uint8_t state = 0;
static uint8_t last_state = 0;
static uint8_t pmod_enc_btn_pressed = 0;

// DHT22 globals
uint8_t humidity_int, humidity_dec, temp_int, temp_dec, checksum;

void GPIO_Init(void);
void DHT22_Begin(void);
uint8_t DHT22_ReadByte(void);
void PMOD_ENC_Task(void);
void Display_Menu(int index);
void Execute_Selection(int index);

// === GPIO Setup ===
void GPIO_Init(void)
{
    SYSCTL->RCGCGPIO |= (1 << 4) | (1 << 5); // Enable Port E and Port F
    while ((SYSCTL->PRGPIO & ((1 << 4) | (1 << 5))) == 0);

    GPIOE->AFSEL &= ~DHT22_PIN;
    GPIOE->AMSEL &= ~DHT22_PIN;
    GPIOE->DIR |= DHT22_PIN;
    GPIOE->DEN |= DHT22_PIN;

    GPIOF->DIR |= (LED_RED | LED_BLUE);
    GPIOF->DEN |= (LED_RED | LED_BLUE);
    GPIOF->DATA &= ~(LED_RED | LED_BLUE); // All LEDs off initially
}

// === DHT22 Timing ===
void DHT22_Begin(void)
{
    GPIOE->DIR |= DHT22_PIN;
    GPIOE->DATA &= ~DHT22_PIN;
    SysTick_Delay1ms(3);
    GPIOE->DATA |= DHT22_PIN;
    SysTick_Delay1us(30);
    GPIOE->DIR &= ~DHT22_PIN;
}

uint8_t DHT22_ReadByte(void)
{
    uint8_t i, result = 0;
    for (i = 0; i < 8; i++)
    {
        uint32_t timeout = 10000;
        while ((GPIOE->DATA & DHT22_PIN) == 0 && timeout--)
            SysTick_Delay1us(1);

        SysTick_Delay1us(35);
        result <<= 1;
        if (GPIOE->DATA & DHT22_PIN)
            result |= 1;

        timeout = 10000;
        while ((GPIOE->DATA & DHT22_PIN) && timeout--)
            SysTick_Delay1us(1);
    }
    return result;
}

// === Menu Display ===
void Display_Menu(int index)
{
    EduBase_LCD_Clear_Display();
    EduBase_LCD_Set_Cursor(0, 0);
    switch (index)
    {
        case 0:
            EduBase_LCD_Display_String("[>] View Temp");
            break;
        case 1:
            EduBase_LCD_Display_String("[>] View RH");
            break;
        case 2:
            EduBase_LCD_Display_String("[>] Test Valve");
            break;
    }
}

// === Button Press Handler ===
void Execute_Selection(int index)
{
    uint16_t raw_humidity, raw_temp;
    float humidity, temp_c;

    DHT22_Begin();

    // Sync response
    uint32_t timeout = 10000;
    while ((GPIOE->DATA & DHT22_PIN) && timeout--) SysTick_Delay1us(1);
    timeout = 10000;
    while (!(GPIOE->DATA & DHT22_PIN) && timeout--) SysTick_Delay1us(1);
    timeout = 10000;
    while ((GPIOE->DATA & DHT22_PIN) && timeout--) SysTick_Delay1us(1);

    humidity_int = DHT22_ReadByte();
    humidity_dec = DHT22_ReadByte();
    temp_int = DHT22_ReadByte();
    temp_dec = DHT22_ReadByte();
    checksum = DHT22_ReadByte();

    raw_humidity = (humidity_int << 8) | humidity_dec;
    raw_temp = (temp_int << 8) | temp_dec;

    humidity = raw_humidity / 10.0;
    if (raw_temp & 0x8000)
    {
        raw_temp &= 0x7FFF;
        temp_c = -1 * (raw_temp / 10.0);
    }
    else
    {
        temp_c = raw_temp / 10.0;
    }

    EduBase_LCD_Clear_Display();
    if (index == 0) {
        EduBase_LCD_Set_Cursor(0, 0);
        EduBase_LCD_Display_String("Temp: ");
        EduBase_LCD_Display_Double(temp_c);
        EduBase_LCD_Display_String(" C");
        GPIOF->DATA &= ~LED_BLUE; // Ensure blue LED is off
    } else if (index == 1) {
        EduBase_LCD_Set_Cursor(0, 0);
        EduBase_LCD_Display_String("Humidity: ");
        EduBase_LCD_Display_Double(humidity);
        EduBase_LCD_Display_String("%");
        if (humidity < 40.0)
            GPIOF->DATA |= LED_RED;  // RH alert
        else
            GPIOF->DATA &= ~LED_RED;
    } else if (index == 2) {
        EduBase_LCD_Set_Cursor(0, 0);
        EduBase_LCD_Display_String("Testing Valve...");
        GPIOF->DATA |= LED_BLUE;
        SysTick_Delay1ms(2000);
        GPIOF->DATA &= ~LED_BLUE;
    }
}

// === Rotary Encoder Task ===
void PMOD_ENC_Task(void)
{
    state = PMOD_ENC_Get_State();
    if (PMOD_ENC_Button_Read(state) && !PMOD_ENC_Button_Read(last_state))
        pmod_enc_btn_pressed = 1;

    main_menu_counter += PMOD_ENC_Get_Rotation(state, last_state);
    if (main_menu_counter < 0) main_menu_counter = 0;
    if (main_menu_counter > MENU_MAX) main_menu_counter = MENU_MAX;
    last_state = state;
}

int main(void)
{
    SysTick_Delay_Init();
    EduBase_LCD_Init();
    EduBase_LCD_Enable_Display();
    EduBase_LCD_Clear_Display();
    GPIO_Init();

    PMOD_ENC_Init();
    Timer_0A_Interrupt_Init(&PMOD_ENC_Task);

    while (1)
    {
        if (prev_menu_counter != main_menu_counter)
        {
            Display_Menu(main_menu_counter);
            prev_menu_counter = main_menu_counter;
        }

        if (pmod_enc_btn_pressed)
        {
            pmod_enc_btn_pressed = 0;
            Execute_Selection(main_menu_counter);
            SysTick_Delay1ms(2000);
            Display_Menu(main_menu_counter);
        }
    }
}