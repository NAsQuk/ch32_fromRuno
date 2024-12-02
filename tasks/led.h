#ifndef LED_H_
#define LED_H_

#define vLedTask_STACK_SIZE 200

extern void vLedTask(void *pvParameters);

extern bool LedReg,LedConnect,LedReceive,LedSend;
extern bool LedNoModem,LedSIM,LedAPN;

#endif
