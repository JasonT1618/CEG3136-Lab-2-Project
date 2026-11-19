// General-purpose input/output (GPIO) driver

/* 	Provided for the CEG3136 lab project, added to by Jason Tong and Ken Shan

	Functions whose definition we created: 
	GPIO_Enable(), GPIO_PortEnable(), GPIO_Mode(), GPIO_Config(), GPIO_AltFunc(), GPIO_Input(),
	GPIO_Output(), GPIO_Toggle().
*/

#include "gpio.h"

#include "mcu.h"

// --------------------------------------------------------
// Initialization
// --------------------------------------------------------

// Enable the specified GPIO pin
void GPIO_Enable (Pin_t pin) {
	GPIO_PortEnable(pin.port);
}

// Enable the peripheral clock for a GPIO port
void GPIO_PortEnable (Port_t port) {
	uint32_t *regAddr;  // Register address
	uint32_t  regVal;   // Register value

	regAddr = (uint32_t *)0x4002104C; 	// Address of AHB2 Enable Register from the Reset and Clock Control (RCC) circuitry (RCC_AHB2ENR)
	regVal   = *regAddr;
	int portNum = ((uint32_t)(port) - (uint32_t)GPIOA) /0x400;	// Get the port index from address
	regVal  |= 0b1 << portNum;
	*regAddr = regVal;
}

// Set the operating mode of a GPIO pin
void GPIO_Mode (Pin_t pin, PinMode_t mode) {
	uint32_t *regAddr;  // Register address
	uint32_t  regVal;   // Register value

	int portAddr = ((uint32_t)(pin.port) - (uint32_t)GPIOA);	// Address offset of the port
	regAddr  = (uint32_t *)(0x42020000 + portAddr);  // Address of GPIOx_MODER
	regVal   = *regAddr;                // Read from register
	regVal  &= ~(0b11 << 2*pin.index);           // Clear corresponding bits with mask
	regVal  |=  mode << 2*pin.index;             // Set new value
	*regAddr = regVal;                  // Write back to register
}

//configure additional setting for a GPIO pin
void GPIO_Config(Pin_t pin, PinType_t ot, PinSpeed_t osp, PinPUPD_t pupd){
	pin.port->OTYPER = (pin.port->OTYPER & ~(0b1 << pin.index)) | (ot << pin.index);
	pin.port->OSPEEDR = (pin.port->OSPEEDR & ~(0b11 << 2*pin.index)) | (osp << 2*pin.index);
	pin.port->PUPDR = (pin.port->PUPDR & ~(0b11 << 2*pin.index)) | (pupd << 2*pin.index);
}

//Select which alternate function is to be used in ALTFUNC mode
void GPIO_AltFunc(Pin_t pin, int af){
	if(pin.index <= 7 && pin.index >= 0){
		pin.port->AFR[0] = (pin.port->AFR[0] & ~(0b1111 << 4*pin.index)) | (af << 4*pin.index);
	}
	else if(pin.index <= 15 && pin.index >= 8){
		pin.port->AFR[1] = (pin.port->AFR[1] & ~(0b1111 << 4*pin.index)) | (af << 4*pin.index);
	}
}

// --------------------------------------------------------
// Pin observation and control
// --------------------------------------------------------

// Observe the state of an input pin
PinState_t GPIO_Input (const Pin_t pin) {
	uint32_t *regAddr;  // Register address
	uint32_t  regVal;   // Register value

	int portAddr = ((uint32_t)(pin.port) - (uint32_t)GPIOA);	// Address offset of the port
	regAddr = (uint32_t *)(0x42020010+portAddr);  // Address of GPIOx_IDR
	regVal  = *regAddr;                // Read from register

	return (regVal >> pin.index) & 0x1;
}

// Control the state of an output pin
void GPIO_Output (Pin_t pin, const PinState_t state) {
	uint32_t *regAddr;  // Register address
	uint32_t  regVal;   // Register value

	int portAddr = ((uint32_t)(pin.port) - (uint32_t)GPIOA);	// Address offset of the port
	regAddr  = (uint32_t *)(0x42020018+portAddr);  // Address of ODR
	if(state==1){ // Set BSRR = state in bit pin.index to set ODR to state
		regVal   = 0b1 << pin.index;	// Use bit set registers if state to set is 1
	}
	else{
		regVal   = 0b1 << (pin.index+16);	// Use bit reset registers if state to set is 0
	}
	*regAddr = regVal;                  // Write to register
}

// Toggle the state of an output pin
void GPIO_Toggle (Pin_t pin) {
	uint32_t *regAddr;  // Register address
	uint32_t  regVal;   // Register value

	int portAddr = ((uint32_t)(pin.port) - (uint32_t)GPIOA);	// Address offset of the port
	regAddr  = (uint32_t *)(0x42020014+portAddr);  // Address of GPIOx_BSRR
	regVal  = *regAddr;        		// Read from register
	regVal ^= 0b1 << pin.index;		// Toggle the bit corresponding the the pin's index
	*regAddr = regVal;              // Write to register

}

// --------------------------------------------------------
// Interrupt handling
// --------------------------------------------------------

// Array of callback function pointers
// Bits 0 to 15 (each can select one port GPIOA to GPIOH)
// Rising and falling edge triggers for each
static void (*callbacks[16][2]) (void);

// Register a function to be called when an interrupt occurs
void GPIO_Callback(Pin_t pin, void (*func)(void), PinEdge_t edge)
{
	callbacks[pin.index][edge] = func;

	// Enable interrupt generation
	if (edge == RISE)
	  EXTI->RTSR1 |= 1 << pin.index;
	else     // FALL
	  EXTI->FTSR1 |= 1 << pin.index;

	int portNum = ((uint32_t)pin.port - (uint32_t)GPIOA) / 0x400;
	EXTI->EXTICR[pin.index / 4] |= portNum << 8*(pin.index % 4);
	EXTI->IMR1 |= 1 << pin.index;

	// Enable interrupt vector
	NVIC->IPR[EXTI0_IRQn + pin.index] = 0;
	__COMPILER_BARRIER();
	NVIC->ISER[(EXTI0_IRQn + pin.index) / 32] = 1 << ((EXTI0_IRQn + pin.index) % 32);
	__COMPILER_BARRIER();
}

// Interrupt handler for all GPIO pins
void GPIO_IRQHandler (int i) {
	// Clear pending IRQ
    NVIC->ICPR[(EXTI0_IRQn + i) / 32] = 1 << ((EXTI0_IRQn + i) % 32);

    // Detect rising edge
	if (EXTI->RPR1 & (1 << i)) {
		EXTI->RPR1 = (1 << i);  // Service interrupt
		callbacks[i][RISE]();   // Invoke callback function
	}

    // Detect falling edge
	if (EXTI->FPR1 & (1 << i)) {
		EXTI->FPR1 = (1 << i);  // Service interrupt
		callbacks[i][FALL]();   // Invoke callback function
	}
}

// Dispatch all GPIO IRQs to common handler function
void  EXTI0_IRQHandler() { GPIO_IRQHandler( 0); }
void  EXTI1_IRQHandler() { GPIO_IRQHandler( 1); }
void  EXTI2_IRQHandler() { GPIO_IRQHandler( 2); }
void  EXTI3_IRQHandler() { GPIO_IRQHandler( 3); }
void  EXTI4_IRQHandler() { GPIO_IRQHandler( 4); }
void  EXTI5_IRQHandler() { GPIO_IRQHandler( 5); }
void  EXTI6_IRQHandler() { GPIO_IRQHandler( 6); }
void  EXTI7_IRQHandler() { GPIO_IRQHandler( 7); }
void  EXTI8_IRQHandler() { GPIO_IRQHandler( 8); }
void  EXTI9_IRQHandler() { GPIO_IRQHandler( 9); }
void EXTI10_IRQHandler() { GPIO_IRQHandler(10); }
void EXTI11_IRQHandler() { GPIO_IRQHandler(11); }
void EXTI12_IRQHandler() { GPIO_IRQHandler(12); }
void EXTI13_IRQHandler() { GPIO_IRQHandler(13); }
void EXTI14_IRQHandler() { GPIO_IRQHandler(14); }
void EXTI15_IRQHandler() { GPIO_IRQHandler(15); }
