#include <stdlib.h>
#include "logica.h"
#include "FreeRTOS.h"
#include "task.h"
#include "translator.h"
#include "gprscon.h"
#include "stm32f10x_flash.h"
#include "usermemory.h"
#include "rtclock.h"

void vLogicaTask(void *pvParameters) {

	int heapSize = 0;
	rtcInit();
	vTaskDelay(500);
	while (1) {
		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;
		BKP_WriteBackupRegister(BKP_DR7, 50);
		//JrnlClear();
		IWDG_ReloadCounter();
		DoProgram();
		vTaskDelay(20);
	}
}

