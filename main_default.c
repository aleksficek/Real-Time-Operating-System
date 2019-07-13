/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// Declare TCB 
typedef struct{
  uint32_t *base;
	uint32_t *current;
	uint32_t *stack_pointer;
	
	uint8_t priority;
}tcb_t;

tcb_t TCBS[6];
uint8_t numTasks;

typedef struct Node_t{
	uint8_t task_num;
	struct Node_t *next;
}Node_t;

Node_t *schedule_array[6];

uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

uint32_t storeContext(void);
void restoreContext(uint32_t sp);

void PendSV_Handler(void) {
	
	
	// Some way of choosing next Task
	
	
	// Assume we start we TCB0
	
	// Access Registers 4 to 11 and push onto TCB1s stack
	
	int current_task = 0;
	int next_task = 1;
	
	
	// Just push/pop R4-R11 to/from stack and manipulate the stack pointer
	
	
																																																			// LOTS OF QUESTIONS HERE, CLARIFY
	TCBS[current_task].stack_pointer = (uint32_t *)storeContext(); // Returns value of stack pointer
	
	restoreContext((uint32_t)TCBS[next_task].stack_pointer);
}

//Function pointer to create task function
typedef void(*rtosTaskFunc_t)(void *args);

void task_create(rtosTaskFunc_t taskFunction, void *R0, uint8_t priority_)
{
	//Protects against more than 6 tasks being created
	if (numTasks > 5)
		return;
	
	//Set priority in tcb_t array
	TCBS[numTasks].priority = priority_;
	
	//Iterate down linked list, two cases
	Node_t* currNode = schedule_array[priority_];

	//Case 1: if this priority's linked list is empty, just insert)
	if (schedule_array[priority_] == NULL)
	{
    Node_t* newNode = (Node_t*)malloc(sizeof(Node_t));
    (*newNode).task_num = numTasks;
    (*newNode).next = NULL;

		schedule_array[priority_] = newNode;
	}
	//Case 2: if not empty
  else
  {
    while ((*currNode).next != NULL)
    {
      currNode = (*currNode).next;
    }
    //Now at last node
    
    //Set last node pointer to pointer of new node, and set its members
    Node_t* newNode = (Node_t*)malloc(sizeof(Node_t));
    (*newNode).task_num = numTasks;
    (*newNode).next = NULL;

    (*currNode).next = newNode;
  }
	
	//Initialize TCB members
	TCBS[numTasks].stack_pointer = TCBS[numTasks].base - 15;
	
	//Setting R0
	*(TCBS[numTasks].base - 7) = (uint32_t)R0;
	//Setting task function address
	*(TCBS[numTasks].base - 1) = (uint32_t)(*taskFunction);
	//Setting P0 to default value of 0x01000000 as specified in manual
	*(TCBS[numTasks].base) = (uint32_t)(0x01000000);

  numTasks++;
}

void initialization(void) {
	// Initialize TCB base address for its stack
	
	// Find address of main stack (first 32 bit value at 0x0 is base address)
	// Then go up for tcbs 54321 but substract instead (subtract by 2 kib, then 1 kib (800, then 400)
	uint32_t **mainstack = 0x0;
	uint32_t *mainstack_address = *mainstack;
	
	TCBS[5].base = (uint32_t *)(mainstack_address - 0x0800);
	TCBS[4].base = (uint32_t *)(mainstack_address - 0x1200);
	TCBS[3].base = (uint32_t *)(mainstack_address - 0x1600);
	TCBS[2].base = (uint32_t *)(mainstack_address - 0x2000);
	TCBS[1].base = (uint32_t *)(mainstack_address - 0x2400);
	TCBS[0].base = (uint32_t *)(mainstack_address - 0x2800);
	
	numTasks = 0;
	
	//Initialize schedule array to all point to NULL. Will be populated by task create function.
	for (int i=0; i<6; i++)
		schedule_array[i] = NULL;
	
	// Copy the main stack contents to process stack of new main() task and set the MSP to the main stack base address
	// Loop through each item and then save to next stack from mainstack_address - 0x8000
	
	uint32_t *MSP = (uint32_t *)__get_MSP();
	TCBS[0].current = TCBS[0].base;
	
	while (mainstack_address >= MSP) {
		*TCBS[0].current = *mainstack_address;
		TCBS[0].current--;
		mainstack_address--;
	}
	__set_MSP((uint32_t)mainstack_address);
	
	// __get_CONTROL(__set_CONTROL)
	__set_CONTROL(1 << 1);
	
	// TCB1.current may have decremented 1 too much
	__set_PSP(*TCBS[0].current);
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
