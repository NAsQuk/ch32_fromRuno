#ifndef _RS485TASK
#define _RS485TASK

#define vRs485Task_STACK_SIZE		1000

extern void vRs485Task(void *pvParameters )  __attribute__((naked));


#endif
