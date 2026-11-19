/*  Calculator app

	Author: Jason Tong and Ken Shan

	Description:
	This source file contains the implementation of the calculator app for the CEG3136 lab project. 
	
	The calculator app allows users to perform various mathematical operations through a touchpad 
	interface with numerical pads 0 to 9, and a SHIFT and NEXT pad. The inputs to the app is 
	taken from the touchpad and the results are displayed on an LCD screen, illuminated by a 
	separate backlight module.
*/

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "calc.h"
#include "display.h"
#include "touchpad.h"
#include <string.h>

#define NMAX 10 // Maximum number of operands
static Press_t operation; // Selected operation
static Entry_t operand[NMAX]; // Numeric operands
static bool done; // Operand entry complete
static int count; // Count of operands received
static Entry_t result; // Calculation result
static enum {MENU, PROMPT, ENTRY, RUN, SHOW, WAIT} state;

// Assembly subroutines to be called as functions
uint32_t Increment(uint32_t n);
uint32_t Decrement(uint32_t n);
uint32_t Add(uint32_t n1, uint32_t n2);
uint32_t Sub(uint32_t n1, uint32_t n2);
uint32_t Mul(uint32_t n1, uint32_t n2);
uint32_t Div(uint32_t n1, uint32_t n2);
uint32_t GCD(uint32_t n1, uint32_t n2);
uint32_t Factorial(uint32_t n);
uint32_t Fibonacci(uint32_t n);
void Sort(uint32_t array[], uint32_t size);
uint32_t Ave(uint32_t array[], uint32_t size);

// Table storing the display messages for the calculator app
static char *menuItems[] = {"1:+ 2:- 3:* 4:/", "5:GCD 6:FACT", "7:Fib 8:Srt 9:Av"};
int page = 0;

static int sortOffset = 0; // Tracks the starting index for displaying sorted elements

// Used to track the number of elements for sorting and averaging operations, which can 
// take variable number of inputs
static int numElems = -1; 

// Initialization
void Init_Calc (void) {
	DisplayEnable();
	TouchEnable();
}

// Runtime
void Task_Calc (void) {
	switch (state) {

	/*The MENU state is performed when first initializing or when finishing from a calculation, 
	where the operation and operands are reset, and to update the display for showing the menu 
	interface, which we programmed to be able to switch between 3 pages. */
	case MENU:
		// Prepare for a new operation
		operation = NONE;
		for (int i = 0; i < NMAX; i++)
			operand[i] = 0;
		count = 0;
		result = 0;
		// Display menu
		DisplayPrint(CALC, 0, "Calculator app %d",page+1);
		DisplayPrint(CALC, 1, menuItems[page]);
		state = PROMPT;
		break;

	/*The PROMPT state is when the application is waiting for the user to enter an operation to 
	perform or a number. If enough number of operands is entered, it can change to the RUN 
	state, or it could change to the ENTRY state if it needs more operands. 
	
	For operations allowing variable number of operands, we also require the user to first enter 
	a numElems value storing the number of operands the user intends to enter, which is added to 
	the array of operands before being overwritten once it has been read after re-entering the 
	ENTRY state.

	SHIFT and NEXT are also used in the PROMPT state to determine when the user wants to see 
	other operation options in the menu, which would trigger a temporary state change to MENU to 
	update the display. */
	case PROMPT:
		if (operation == NONE)
			operation = TouchInput(CALC);
		switch ((int)operation) {
		case 10:
			page -= 1;
			if (page == -1){
				page = 2;
			}
			state = MENU;
			break;
		case 11:
			page += 1;
			if (page == 3){
				page = 0;
			}
			state = MENU;
			break;
		case 6: // Factorial
		case 7: // Fibonacci
			if (count == 1)
				state = RUN;
			else {
				DisplayPrint(CALC, 0, "Enter a number:");
				state = ENTRY;
			}
			break;
		case 1: // addition
		case 2: // subtraction
		case 3: // multiplication
		case 4: // division
		case 5: // GCD
			if (count == 2)
				state = RUN;
			else {
				DisplayPrint(CALC, 0, "Enter a number:");
				state = ENTRY;
			}
			break;
		case 8: // Sort
		case 9: // Average
			// Condition to request from user a number of elements to consider
			if (numElems < 0){
				char msg[20];
				snprintf(msg, sizeof(msg), "Enter size(<%d):", NMAX+1);
				DisplayPrint(CALC, 0, msg);
				state = ENTRY;
			}
			// Condition to check if enough operands have been entered based on the intended number of elements
			else if(count == numElems){
				state = RUN;
				numElems = -1;
			}
			// Condition to prompt user for more operands
			else{
				char msg[20];
				snprintf(msg, sizeof(msg), "Enter number %d:", count);
				DisplayPrint(CALC, 0, msg);
				state = ENTRY;
			}
			break;
		default:
			operation = NONE;
			break;
		}
		break;

	/*The ENTRY state is mainly used for the software to wait until the user has finished 
	entering exactly one number, while also handling the logic of the difference between a 
	value for numElems being entered for sort and average, or just a standard operand. */
	case ENTRY:
		done = TouchEntry(CALC, &operand[count]); // Writes to operand[count] and sets done to true when NEXT is pressed
		DisplayPrint(CALC, 1, "%u", operand[count]);
		if (done) {
			// Condition to consider number of elements for sort or average
			if (numElems < 0 && operand[count] > 0 && operand[count] <= NMAX && ((int)operation == 8 || (int)operation == 9)){
				numElems = operand[count];
				operand[count] = 0;
			}
			// Condition for general numeric entries
			else{
				count++;
			}
			state = PROMPT;
		}
		break;

	/*The RUN state happens once per operations, and simply calculates the result (or updates 
	the values held in an array in the case of sort) before changing the state to SHOW.*/
	case RUN:
		DisplayPrint(CALC, 0, "Calculating...");
		DisplayPrint(CALC, 1, "");
		state = SHOW;
		switch ((int)operation) {
		case 1: result = Add(operand[0], operand[1]); break;
		case 2: result = Sub(operand[0], operand[1]); break;
		case 3: result = Mul(operand[0], operand[1]); break;
		case 4: result = Div(operand[0], operand[1]); break;
		case 5: result = GCD(operand[0], operand[1]); break;
		case 6: result = Factorial(operand[0]); break;
		case 7: result = Fibonacci(operand[0]); break;
		case 8: Sort(operand, count); break;
		case 9: result = Ave(operand, count); break;
		default: break;
		}
		break;

	/*The SHOW state is mainly used only to update the LCD display before entering the WAIT 
	state, with the exception of using sort, which we programmed to have the application stall 
	in the SHOW state so the user can switch between two sets of numbers using SHIFT and NEXT, 
	one representing the first 5 numbers in the sorted list, and the other representing the 
	next 5 numbers, while pressing any other touch pad other than SHIFT and NEXT will exit to 
	the WAIT state. 

	This is necessary in our implementation since we can have a maximum of 10 operands, but 
	showing an integer and a comma puts the minimum character count to 19, when the LCD only 
	has 16 cells.*/
	case SHOW:
		DisplayPrint(CALC, 0, "Result:");
		// Special display interface for result of sort operation
		if ((int)operation == 8) { 
			// Check for and record navigation inputs
			Press_t input = TouchInput(CALC);
			if (input == 10) { // Previous page
				if (sortOffset > 0) {
					sortOffset -= 5;
				}
				break; // Keep state in SHOW to refresh display
			}
			else if (input == 11) { // Next page
				if (sortOffset + 5 < count) {
					sortOffset += 5;
				}
				break; // Keep state in SHOW to refresh display
			}
			else if (input != NONE) {
				// Any other key press returns to MENU and resets offset
				sortOffset = 0;
				state = WAIT;
				break;
			}
			// Build a string for a window of elements, 5 elements at a time
			char msg[20] = "";
			int displayed = 0;
			for (int i = sortOffset; i < count && displayed < 5; i++) {
				char temp[10];
				if (displayed == 0) {
					snprintf(temp, sizeof(temp), "%lu", operand[i]);
				} else {
					snprintf(temp, sizeof(temp), ",%lu", operand[i]);
				}
				// Ensure the message buffer is not overflowed
				if (strlen(msg) + strlen(temp) < sizeof(msg)) {
					strcat(msg, temp);
					displayed++;
				} else {
					break;
				}
			}
			DisplayPrint(CALC, 1, msg);
			break;
		}
		// Special display format for result of average operation, one decimal place is considered with a radix point
		else if ((int)operation == 9){ 
			char msg[20];
			int integer = result / 10;
			int decimal = result % 10;
			snprintf(msg, sizeof(msg), "%d.%d", integer, decimal);
			DisplayPrint(CALC, 1, msg);
		}
		else{
			DisplayPrint(CALC, 1, "%u", result);
		}
		state = WAIT;
		break;

	/*The WAIT state is used to allow the user to see what the output of the operation was. It 
	exits to the MENU state for resetting once the user presses any touch pad.*/
	case WAIT:
		// Press any pad to return to the menu
		if (TouchInput(CALC) != NONE)
			state = MENU;
		break;
	}
}

