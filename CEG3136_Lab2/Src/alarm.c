/*	alarm.c
  
	Author: Jason Tong and Ken Shan

	Description:
	This source file contains the implementation of the alarm system app for the CEG3136 lab 
	project.

	The alarm app can be summarized by 3 states: DISARMED, ARMED, and TRIGGERED. 
	
	The system starts in the DISARMED state. Here, no LEDs are illuminated, and the user can 
	trigger transition into the ARMED state by pressing a button for a short duration.

	The ARMED state has green and blue LEDs alternate illumination every second. It can 
	transition back to DISARMED from a long press, or triggering a motion detector will cause 
	transition to the TRIGGERED state.

	In the TRIGGERED state, the red LED is illuminated and a buzzer will be turned on. It can 
	transition back to the ARMED state from a short button press, or to the DISARMED state from
	a long button press.

	Short press is defined as holding the button for less than 2 seconds before release, while
	long press is defined as holding the button for 3 seconds without needing to have released
	the button.

	Each state transition will also produce a status message on the data console that includes
	the current system time, the new state, and the reason for the transition.

	The button is not debounced from hardware, so rapid fluctuations in signal within a 50ms 
	interval after the initial rising edge is ignored.
*/

#include"display.h"
#include <stdio.h>
#include <math.h>
#include "alarm.h"
#include "systick.h"
#include "gpio.h"
#include <stdbool.h>

// GPIO pins
static const Pin_t BlueLED = {GPIOB,7} ;	//Pin PB7 -> User LD2
static const Pin_t RedLED = {GPIOA,9} ;		//Pin PA9 -> User LD1
static const Pin_t GreenLED = {GPIOC,7} ;	//Pin PC7 -> User LD3
static const Pin_t Button = {GPIOC,13} ;	//Pin PC13 -> User B1
static const Pin_t Motion = {GPIOB,9} ;		//Pin PB9
static const Pin_t EStop = {GPIOB,2} ;		//Pin PB2
static const Pin_t Buzzer = {GPIOA,0} ;		//Pin PA0

static enum {DISARMED, ARMED, TRIGGERED} state;

// Constants
#define BOUNCE_DETECT 50	// 50ms
#define BLINK_PERIOD 1000 	// 1 second
#define SHORT_PRESS 2000	// 2 seconds
#define LONG_PRESS 3000		// 3 seconds
// Variables
static bool flag= false;
static int timePress;		// Time of the last rising edge from button
static int timeRelease;		// Time of the last falling edge from button
static int timeLastTransition;	// Time of the last state transition
static int timeBlink;		// Time of the last button toggle

// Declare interrupt callback functions
static void CallbackMotionDetect();
static void CallbackButtonPress();
static void CallbackButtonRelease();

// Initialization code
void Init_Alarm (void) {
	GPIO_PortEnable(GPIOA);
	GPIO_PortEnable(GPIOB);
	GPIO_PortEnable(GPIOC);
	//LEDs
	GPIO_Enable(BlueLED);
	GPIO_Mode(BlueLED,OUTPUT);
	GPIO_Enable(RedLED);
	GPIO_Mode(RedLED,OUTPUT);
	GPIO_Enable(GreenLED);
	GPIO_Mode(GreenLED,OUTPUT);
	GPIO_Enable(Buzzer);
	GPIO_Mode(Buzzer,OUTPUT);
	//Buttons
	GPIO_Enable(Button);
	GPIO_Mode(Button,INPUT);
	GPIO_Enable(Motion);
	GPIO_Mode(Motion,INPUT);
	GPIO_Enable(EStop);
	GPIO_Mode(EStop,INPUT);
	//Initial state
	GPIO_Output(BlueLED,LOW);
	GPIO_Output(RedLED,LOW);
	GPIO_Output(GreenLED,LOW);
	GPIO_Output(Buzzer,LOW);
	// Initialize interrupt handlers
	GPIO_Callback(Motion,CallbackMotionDetect,RISE);
	GPIO_Callback(EStop,CallbackButtonPress,RISE);
	GPIO_Callback(EStop,CallbackButtonRelease,FALL);

	timeLastTransition = TimeNow();

	// Initialize and set LCD display and backlight
	DisplayEnable();
	DisplayColor(ALARM, WHITE);
	DisplayPrint(ALARM,0,"DISARMED");

	printf("Alarm started, entering DISARMED state at time %u\n", TimeNow());
}

// Task code (state machine)
void Task_Alarm (void) {
	switch (state) {
	case DISARMED:
		// reset all LED
		GPIO_Output(BlueLED,LOW);
		GPIO_Output(RedLED,LOW);
		GPIO_Output(GreenLED,LOW);

		// Transition from DISARMED to ARMED
		// Pressed and released in less than 2s but more than 50ms
		// State transition did not already occur from the last press considered for redundancy
		if ( TimePassedSince(timePress) - TimePassedSince(timeRelease) < SHORT_PRESS
				&& TimePassedSince(timePress) - TimePassedSince(timeRelease) > BOUNCE_DETECT
				&& TimePassedSince(timeLastTransition) > TimePassedSince(timePress)) { 
			// set state to ARMED and update the timeLastTransition
			state = ARMED;
			printf("ARMED state at time %u\n", TimeNow());
			timeLastTransition = TimeNow();
			flag = false; // Clears flag from previous triggers
			GPIO_Output(BlueLED, HIGH);  // Keeps blue and green LED out of phase

			//Display
			DisplayColor(ALARM, YELLOW);
			DisplayPrint(ALARM, 0, "ARMED");
		}
		break;
	case ARMED:
		// Blink the Green and Blue LED
		if(TimePassedSince(timeBlink) >= BLINK_PERIOD){ // Toggle the green and blue LEDs whenever blink period is reached
			GPIO_Toggle(GreenLED);
			GPIO_Toggle(BlueLED);
			timeBlink = TimeNow();
		}

		// Transition condition from ARMED to DISARMED
		// Pressed for 3s and last press time is newer than the last release time (still holding)
		if(TimePassedSince(timePress) >= LONG_PRESS
				&& TimePassedSince(timeLastTransition) > TimePassedSince(timePress)
				&& TimePassedSince(timeRelease)> TimePassedSince(timePress))
		{ 
			// set state to DISARMED and update the timeLastTransition
			state = DISARMED;
			printf("DISARMED state at time %u, from ARMED\n", TimeNow());
			timeLastTransition = TimeNow();

			//Display
			DisplayColor(ALARM, WHITE);
			DisplayPrint(ALARM, 0, "DISARMED");
		}
		// Transition condition from ARMED to DISARMED
		// Motion detection boolean flag is triggered and given a 0.5s grace period after entering ARMED
		else if(flag && TimePassedSince(timeLastTransition) > 500){
			// Motion is detected and e-stop button is not being pressed, gives a 0.5s buffer time before TRIGGERED state can be transitioned to
			state = TRIGGERED;
			printf("TRIGGERED state at time %u, from ARMED\n", TimeNow());
			timeLastTransition = TimeNow();

			// SET RedLED and REST the BlueLED and GreenLED
			GPIO_Output(RedLED,HIGH);
			GPIO_Output(BlueLED,LOW);
			GPIO_Output(GreenLED,LOW);

			//Display
			DisplayColor(ALARM, RED);
			DisplayPrint(ALARM, 0, "TRIGGERED");
		}
		break;
	case TRIGGERED:
		// 4 different buzzer frequencies alternating in a 4 second period. The 1st second has 
		// f=1/10Hz, 2nd with f=1/5Hz, 3rd with f=1/3Hz, and 4th being off.
		if ((TimePassedSince(timeLastTransition) % 10 == 0 && (TimePassedSince(timeLastTransition) / 1000) % 4 == 0)
			|| (TimePassedSince(timeLastTransition) % 5 == 0 && (TimePassedSince(timeLastTransition) / 1000) % 4 == 1)
			|| (TimePassedSince(timeLastTransition) % 3 == 0 && (TimePassedSince(timeLastTransition) / 1000) % 4 == 2)){
			GPIO_Output(Buzzer,HIGH);
		}
		else {
			GPIO_Output(Buzzer,LOW);
		}

		// Transition condition from TRIGGERED to DISARMED
		// Pressed for 3s and not considering a press that occured before the state change
		if(TimePassedSince(timePress) >= LONG_PRESS
				&& TimePassedSince(timeLastTransition) > TimePassedSince(timePress)){ 
			// set state to DISARMED and update the timeLastTransition
			state = DISARMED;
			printf("DISARMED state at time %u\n", TimeNow());
			timeLastTransition = TimeNow();
			flag = false;
			GPIO_Output(RedLED,LOW);
			GPIO_Output(Buzzer,LOW);

			//Display
			DisplayColor(ALARM, WHITE);
			DisplayPrint(ALARM, 0, "DISARMED");
		}
		// Transition from TRIGGERED to ARMED
		// Pressed and released in less than 2s but more than 50ms
		// Press time newer than state change to TRIGGERED time considered for redundancy
		else if ( TimePassedSince(timePress) - TimePassedSince(timeRelease) < SHORT_PRESS
				&& TimePassedSince(timePress) - TimePassedSince(timeRelease) > BOUNCE_DETECT
				&& TimePassedSince(timeLastTransition) > TimePassedSince(timePress)) { 
			// set state to ARMED and update the timeLastTransition
			state = ARMED;
			printf("ARMED state at time %u\n", TimeNow());
			timeLastTransition = TimeNow();

			flag = false;
			GPIO_Output(RedLED,LOW);
			GPIO_Output(BlueLED,HIGH); // Keeps blue and green LED out of phase
			GPIO_Output(Buzzer,LOW);

			// Display
			DisplayColor(ALARM, YELLOW);
			DisplayPrint(ALARM, 0, "ARMED");
		}
		break;
	}
}

void CallbackMotionDetect (void) {
	// flag for motion detector and set when motion is detected
	flag=true;
}

void CallbackButtonPress (void) {
	// detect the time of press for e-stop button, with 50ms software debouncing time
	if(TimePassedSince(timePress) > BOUNCE_DETECT)
		timePress = TimeNow();
}

void CallbackButtonRelease (void) {
	// detect the time of release for e-stop button, with 50ms software debouncing time
	if(TimePassedSince(timePress) > BOUNCE_DETECT)
		timeRelease = TimeNow();
}
