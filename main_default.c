/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


// Declare TCB 
typedef struct{
  uint32_t *base;
	uint32_t *current;
}tcb_t;

tcb_t TCB0;
tcb_t TCB1;
tcb_t TCB2;
tcb_t TCB3;
tcb_t TCB4;
tcb_t TCB5;

uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

void initialization(void) {
	// Initialize TCB base address for its stack
	
	// Find address of main stack (first 32 bit value at 0x0 is base address)
	// Then go up for tcbs 54321 but substract instead (subtract by 2 kib, then 1 kib (800, then 400)
	
	uint32_t *mainstack = 0x0;
	uint32_t mainstack_address = *mainstack;
	
	TCB5.base = (uint32_t *)(mainstack_address - 0x0800);
	TCB4.base = (uint32_t *)(mainstack_address - 0x1200);
	TCB3.base = (uint32_t *)(mainstack_address - 0x1600);
	TCB2.base = (uint32_t *)(mainstack_address - 0x2000);
	TCB1.base = (uint32_t *)(mainstack_address - 0x2400);
	TCB0.base = (uint32_t *)(mainstack_address - 0x2800);
	
	
	// Copy the main stack contents to process stack of new main() task and set the MSP to the main stack base address
	// Loop through each item and then save to next stack from mainstack_address - 0x8000
	int counter = 0;
	uint32_t MSP = __get_MSP();
	TCB1.current = TCB1.base;
	
	while (mainstack_address > MSP) {
		*TCB1.current = *mainstack;
		TCB1.current--;
		mainstack_address--;
	}
}

int main(void) {
 
	SysTick_Config(SystemCoreClock/1000);
	printf("\nStarting...\n\n");
	
	uint32_t period = 1000; // 1s
	uint32_t prev = -period;
	while(true) {
		if((uint32_t)(msTicks - prev) >= period) {
			printf("tick ");
			prev += period;
		}
	}
		
}
