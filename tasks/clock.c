#include <stdlib.h>
#include <stdlib.h>
#include <stdarg.h>

#include "typedef.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart2.h"
#include "gprscon.h"
#include "rvnettcp.h"
#include "rtclock.h"

extern int counttick;
bool JrnlTime;
bool ResetTime;
bool ResetTimeEnd;

void vClockTask(void *pvParameters) {
	volatile int heapSize = 0;
	heapSize = xPortGetFreeHeapSize();
	heapSize += 0;
	while (1) {
		if (counttick >= 1000) {
			BKP_WriteBackupRegister(BKP_DR7, 3);
			UpdateTime();
			counttick = 0;
		}


		if ((JrnlTime)&&(!ResetTimeEnd))
			{
				JrnlWrite("Время изменено");
				JrnlTime = 0;

			}


		vTaskDelay(700);
	}
}
