/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//Need to include context.c?

// Declare TCB 
typedef struct{
	//Bottom of task stack (highest address)
  uint32_t *base;
	//Temp pointer
	uint32_t *current;
	//Top of stack, could also be on top of pushed registers (lowest address)
	uint32_t *stack_pointer;
	
	uint8_t priority;
}tcb_t;

tcb_t TCBS[6];
uint8_t numTasks;
uint8_t currTask;
uint8_t next_task;

typedef struct Node_t{
	uint8_t task_num;
	struct Node_t *next;
}Node_t;

Node_t *schedule_array[6];

uint32_t msTicks = 0;

void SysTick_Handler(void) {
	// When context switch required
	if (!(msTicks % 1000)) {
		// Write 1 to PENDSVSET bit of ICSR
		SCB->ICSR |= (1 << 28);
	}
	msTicks++;
}

uint32_t storeContext(void);
void restoreContext(uint32_t sp);
uint8_t find_next_task();
uint8_t remove_front_node(uint8_t priority);
void add_node(uint8_t priority_, uint8_t taskNum);

void PendSV_Handler(void) {
	printf("\n\nPENDSV\n");								
	
	printf("LETS PRINT CURRTASK PRIORITY: %d\n", TCBS[currTask].priority);
	//Puts current task's node back
	add_node(TCBS[currTask].priority, currTask);
	
	for (int priority = 0; priority<6; priority++)
		{
			Node_t *currNode = schedule_array[priority];

			printf("Priority list %d:", priority);

			while (currNode != NULL)
			{
				printf("%d ", (*currNode).task_num);
				currNode = (*currNode).next;
			}

			printf("\n");
		}
	
	
	//Finds next task
	next_task = find_next_task();
	printf("Next Task: %d\n", next_task);
	
							// Some way of choosing next Task			
							// Assume we start we TCB0
							// Access Registers 4 to 11 and push onto TCB1s stack
	
	// Just push/pop R4-R11 to/from stack and manipulate the stack pointer
	
	//Pushes register contents onto current task's stack and updates its stack pointer																																												// LOTS OF QUESTIONS HERE, CLARIFY
	TCBS[currTask].stack_pointer = (uint32_t *)storeContext();

	//Pops new task's registers content (stored on its stack) into registers
	restoreContext((uint32_t)TCBS[next_task].stack_pointer);
	
	//Updates current task
	currTask = next_task;

	//Removes next task's node
	remove_front_node(TCBS[next_task].priority);
	
	//Reset PENDSVSET bit of ICSR to 0
	
	SCB->ICSR &= !(1 << 28);
}

//Function pointer to create task function
typedef void(*rtosTaskFunc_t)(void *args);

//Gets called in taks initialization and pre-emting
void add_node(uint8_t priority_, uint8_t taskNum)
{
	printf("Print the task Num: %d\n", taskNum);
	//Iterate down linked list
	Node_t* currNode = schedule_array[priority_];

	//Case 1: if this priority's linked list is empty, just insert)
	if (schedule_array[priority_] == NULL)
	{
    Node_t* newNode = (Node_t*)malloc(sizeof(Node_t));
    (*newNode).task_num = taskNum;
		printf("NEW NODE ADDED TASK NUM: %d\n", (*newNode).task_num);
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
    (*newNode).task_num = taskNum;
    (*newNode).next = NULL;

    (*currNode).next = newNode;
  }
}

void task_create(rtosTaskFunc_t taskFunction, void *R0, uint8_t priority_)
{
	//Protects against more than 6 tasks being created
	if (numTasks > 5)
		return;
	
	add_node(priority_, numTasks);
	
	//Initialize TCB members
	TCBS[numTasks].stack_pointer = TCBS[numTasks].base - 15;
	TCBS[numTasks].priority = priority_;
	
	//Setting R0
	*(TCBS[numTasks].base - 7) = (uint32_t)R0;
	//Setting task function address
	*(TCBS[numTasks].base - 1) = (uint32_t)(*taskFunction);
	//Setting P0 to default value of 0x01000000 as specified in manual
	*(TCBS[numTasks].base) = (uint32_t)(0x01000000);

  numTasks++;
}

//Returns task number of task removed, 0 if no ready tasks at priority, 
//-1 if invalid priority
uint8_t remove_front_node(uint8_t priority)
{
  uint8_t taskNumOfRemoved = 0;

  if (priority < 0 || priority > 5)
    return -1;

  if (schedule_array[priority] == NULL)
    return 0;
    
  taskNumOfRemoved = (*schedule_array[priority]).task_num;

  Node_t* secondNode = (*schedule_array[priority]).next;
  free(schedule_array[priority]);
  schedule_array[priority] = secondNode;

  numTasks--;

  return (uint8_t)taskNumOfRemoved;
}

//Return -1 if no available next task
uint8_t find_next_task()
{
  //Iterate down bit vector until find next available priority
  int next_priority = 5;
  while (schedule_array[next_priority] == NULL && next_priority >= 0) {
    next_priority--;
  }
  
  //If no available next task
  if (next_priority == -1)
	{
		printf("No available next task, ERROR");
    return -1;
	}

  //If available next task, return first task num of linked list.
  //Because available tasks are added at back of linked list
  //and ready tasks are removed at front of linked list,
  //tasks of equal priority that are ready are round robined
  return (uint8_t)(*schedule_array[next_priority]).task_num;
}

void initialization(void) {
	// Initialize TCB base address for its stack
	
	// Find address of main stack (first 32 bit value at 0x0 is base address)
	// Then go up for tcbs 54321 but substract instead (subtract by 2 kib, then 1 kib (800, then 400)
	uint32_t **mainstack = 0x0;
	//This used to copy over main stack to task 1 stack
	uint32_t *mainstack_address = *mainstack;
	//This used to remember where main stack base is
	uint32_t *mainstack_base = *mainstack;
	
	TCBS[5].base = (uint32_t *)(mainstack_address - 0x0800/4);
	TCBS[4].base = (uint32_t *)(mainstack_address - 0x1200/4);
	TCBS[3].base = (uint32_t *)(mainstack_address - 0x1600/4);
	TCBS[2].base = (uint32_t *)(mainstack_address - 0x2000/4);
	TCBS[1].base = (uint32_t *)(mainstack_address - 0x2400/4);
	TCBS[0].base = (uint32_t *)(mainstack_address - 0x2800/4);
	
	numTasks = 0;
	
	//Initialize schedule array to all point to NULL. Will be populated by task create function.
	for (int i=0; i<6; i++)
		schedule_array[i] = NULL;
	
	// Copy the main stack contents to process stack of new main() task and set the MSP to the main stack base address
	// Loop through each item and then save to next stack from mainstack_address - 0x8000
	
	uint32_t *MSP = (uint32_t *)__get_MSP();
	TCBS[0].current = TCBS[0].base;
	
	//Copy everything in main stack to task 1 stack
	while (mainstack_address >= MSP) {
		*TCBS[0].current = *mainstack_address;
		TCBS[0].current--;
		
		mainstack_address--;
		
	}
	
	TCBS[0].stack_pointer = TCBS[0].current + 1;
	
	//Set MSP to mainstack base address
	__set_MSP((uint32_t)mainstack_base);
	
	//Switches control from MSP to PSP
	__set_CONTROL(__get_CONTROL() | (1 << 1));
	
	//Set PSP to top of task 1 stack
	// TCB1.current may have decremented 1 too much
	__set_PSP((uint32_t)TCBS[0].stack_pointer);
	
	//Begin multithread by running task 0. Correct next task will be 
	//determined at next pre-empt in PendSV_Handler
	currTask = 0;
	
	//Set task 1 priority to 0, acts as idle task
	TCBS[0].priority = 0;
	add_node(TCBS[0].priority, numTasks);
	
	//Increment numtasks now that there is a task
	numTasks++;
}


void do_something(void *args) {
	while (1)
	{
		printf("\n\nTHIS IS TASK 2\n\n");
		for (int priority = 0; priority<6; priority++)
		{
			Node_t *currNode = schedule_array[priority];

			printf("Priority list %d:", priority);

			while (currNode != NULL)
			{
				printf("%d ", (*currNode).task_num);
				currNode = (*currNode).next;
			}

			printf("\n");
		}
		printf("\n");
	}
}


int main(void) {
	printf("\n=============================Starting...===================================\n\n");
	//Initialization creates task 0
	initialization();
	
	rtosTaskFunc_t task1 = &do_something;
	task_create(task1, NULL, 5);
 
	SysTick_Config(SystemCoreClock/1000);
	
	printf("Curr task after init: %d", currTask);
	
	uint32_t period = 1000; // 1s
	uint32_t prev = -period;
	
	while(true) {
		if((uint32_t)(msTicks - prev) >= period) {
			printf("\n\nTHIS IS TASK 1\n\n");
			printf("PSP: %d\n", __get_PSP());
			printf("MSP: %d\n\n", __get_MSP());
			prev += period;
		}
		
		printf("task0\n");
	}
		
}
