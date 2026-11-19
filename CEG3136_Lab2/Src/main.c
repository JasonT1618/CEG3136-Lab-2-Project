/*	Main program

	Author: Jason Tong and Ken Shan

	Description:
	This source file contains the main program for the CEG3136 lab project, as of lab 2. The 
	main function sets up the system, initializes the apps, and enters a continuous loop for 
	handling interactions with the user on a customized circuit board utilizing an 
	STM32L552ZE MCU.
*/

#include"display.h"
#include <stdint.h>
#include <stdio.h>

//header file for drives
#include "gpio.h"
#include "i2c.h"
#include "systick.h"
#include "touchpad.h"

//header file for apps
#include "alarm.h"
#include "calc.h"

static const Pin_t BlueLED = {GPIOB,7} ;	//Pin PB7 -> User LD2
static const Pin_t Button = {GPIOC,13} ;	//Pin PC13 -> User B1

static unsigned int iterationCount;

int main(void) {

	int i=0;
	if(i==0){
		ConfigureSysClk();

		printf("Welcome to %c%c%c%d!\n", 67, 69, 71, 0xC40);

		// Initialize apps
		Init_Alarm();
		Init_Calc();

		StartSysTick();

		while (1){
			// Run apps
			Task_Alarm();
			Task_Calc();

			// Housekeeping
			UpdateDisplay();
			ScanTouchpad();
			ServiceI2CRequests();

			WaitForSysTick(); // Wait for the next SysTick interrupt that occurs every 1ms
		}
	}
	else{ // Obsolete code for testing GPIO and SysTick in earlier labs
		ConfigureSysClk();

		printf("Welcome to %c%c%c%d!\n", 67, 69, 71, 0xC40);

		GPIO_Enable(BlueLED);
		GPIO_Mode(BlueLED,OUTPUT);

		GPIO_Enable(Button);
		GPIO_Mode(Button,INPUT);

		StartSysTick();

		while(1){
		DelayFor(500);

		//Turn on LED if button is pressed
		 if(!GPIO_Input(Button)){
		 	GPIO_Output(BlueLED, HIGH);
		 }
		 DelayFor(500);

		 //turn off LED
		 GPIO_Output(BlueLED, LOW);

		 iterationCount++;
		}
	}
}
