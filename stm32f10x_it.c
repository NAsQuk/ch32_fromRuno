#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "I2CRoutines.h"

__IO uint8_t Tx_Idx1 = 0, Rx_Idx1 = 0;
__IO uint8_t Tx_Idx2 = 0, Rx_Idx2 = 0;

#ifdef I2CUSE
extern __IO uint32_t NumbOfBytes1;
extern __IO uint32_t NumbOfBytes2;
extern uint8_t Buffer_Rx1[];
extern uint8_t Buffer_Tx1[];
extern uint8_t Buffer_Rx2[];
extern uint8_t Buffer_Tx2[];

extern __IO uint32_t I2CDirection;
extern uint8_t Address;
#endif

void NMI_Handler(void) {

}

void HardFault_Handler(void) {

	NVIC_SystemReset();

}

void USB_LP_CAN1_RX0_IRQHandler() {
	USB_Istr();
}
;

void MemManage_Handler(void) {
	while (1) {
	}
}

void BusFault_Handler(void) {

	while (1) {
	}
}

void UsageFault_Handler(void) {

	while (1) {
	}
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
