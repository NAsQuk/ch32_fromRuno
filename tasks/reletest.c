#include <stdlib.h>
#include "reletest.h"
#include "usermemory.h"
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f10x_flash.h"
#include "usermemory.h"

void vReletestTask(void *pvParameters) {

	int heapSize = 0;

	while (1) {
		BKP_WriteBackupRegister(BKP_DR7, 70);
		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;
		if ((RAM.relays[0] & 1) != 0) {
			GPIO_SetBits(GPIO_RELE, P1);
		} else {
			GPIO_ResetBits(GPIO_RELE, P1);
		}
		if ((RAM.relays[0] & 2) != 0) {
			GPIO_SetBits(GPIO_RELE, P2);
		} else {
			GPIO_ResetBits(GPIO_RELE, P2);
		}
		if ((RAM.relays[0] & 4) != 0) {
			GPIO_SetBits(GPIO_RELE, P3);
		} else {
			GPIO_ResetBits(GPIO_RELE, P3);
		}


		DiskretSet(7, !(GPIO_ReadInputDataBit(GPIO_D6toD9, D8)));
		DiskretSet(6, !(GPIO_ReadInputDataBit(GPIO_D6toD9, D7)));
		DiskretSet(5, !(GPIO_ReadInputDataBit(GPIO_D6toD9, D6)));
		DiskretSet(4, !(GPIO_ReadInputDataBit(GPIO_D2toD5, D5)));
		DiskretSet(3, !(GPIO_ReadInputDataBit(GPIO_D2toD5, D4)));
		DiskretSet(2, !(GPIO_ReadInputDataBit(GPIO_D2toD5, D3)));
		DiskretSet(1, !(GPIO_ReadInputDataBit(GPIO_D2toD5, D2)));
		DiskretSet(0, !(GPIO_ReadInputDataBit(GPIO_D1, D1)));
		DiskretSet(10, !(GPIO_ReadInputDataBit(GPIO_D10toD11, D11)));
		DiskretSet(9, !(GPIO_ReadInputDataBit(GPIO_D10toD11, D10)));
		DiskretSet(8, !(GPIO_ReadInputDataBit(GPIO_D6toD9, D9)));

		DiskretsProcess();
		BKP_WriteBackupRegister(BKP_DR7, 72);
		vTaskDelay(2);
	}

}
