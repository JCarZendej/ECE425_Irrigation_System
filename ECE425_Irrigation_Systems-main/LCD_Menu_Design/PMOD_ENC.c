/**
 * @file PMOD_ENC.c
 * @brief Source code for the PMOD_ENC driver.
 *
 * This file contains the function definitions for the PMOD_ENC driver.
 * It uses GPIO to interface with the PMOD ENC (Rotary Encoder) module.
 *
 * The PMOD ENC module (rotary encoder) uses the following pinout:
 *  - PMOD ENC Pin 1 (A)        <-->  Tiva LaunchPad Pin PD0
 *  - PMOD ENC Pin 2 (B)        <-->  Tiva LaunchPad Pin PD1
 *  - PMOD ENC Pin 3 (BTN)      <-->  Tiva LaunchPad Pin PD2
 *  - PMOD ENC Pin 4 (SWT)      <-->  Tiva LaunchPad Pin PD3
 *  - PMOD ENC Pin 5 (GND)      <-->  Tiva LaunchPad GND
 *  - PMOD ENC Pin 6 (VCC)      <-->  Tiva LaunchPad 3.3V
 *
 * @note The EduBase push buttons (EduBase_Button_Init) should not be used with the
 * PMOD ENC module because both drivers share the same pins (PD0 - PD3).
 * 
 * @note For more information regarding the PMOD ENC module, refer to the Reference Manual
 * Link: https://reference.digilentinc.com/reference/pmod/pmodenc/reference-manual
 *
 * @author Aaron Nanas
 *
 */
 
#include "PMOD_ENC.h"

void PMOD_ENC_Init(void)
{

	SYSCTL->RCGCGPIO |= 0x08;
	
	GPIOD->DIR &= ~PMOD_ENC_ALL_PINS_MASK;
	
	GPIOD->AFSEL &= ~PMOD_ENC_ALL_PINS_MASK;
	
	GPIOD->DEN |= PMOD_ENC_ALL_PINS_MASK;
}

uint8_t PMOD_ENC_Get_State(void)
{
  uint8_t state = GPIOD->DATA & PMOD_ENC_ALL_PINS_MASK;
	return state;
}

int PMOD_ENC_Get_Rotation(uint8_t state, uint8_t last_state)
{
	if((state & PMOD_ENC_PIN_A_MASK) != 0 && (last_state & PMOD_ENC_PIN_A_MASK) == 0)
	{
		if((state & PMOD_ENC_PIN_B_MASK) != 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return 0;
	}	
}

uint8_t PMOD_ENC_Button_Read(uint8_t state)
{
  return state & PMOD_ENC_BUTTON_MASK;
}

uint8_t PMOD_ENC_Switch_Read(uint8_t state)
{
  return state & PMOD_ENC_SWITCH_MASK;
}
