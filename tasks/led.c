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
#include "board.h"
bool LedReg, LedConnect, LedReceive, LedSend;
bool LedNoModem, LedSIM, LedAPN;

void vLedTask(void *pvParameters) {
	volatile int heapSize = 0;
	while (1) {
		//IWDG_SetReload(10000); ру
		IWDG_ReloadCounter();

		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;
		GPIO_SetBits(GPIO_LEDWORK, LEDWORK);

		if ((ResetTime)/*&&(!LedConnect)*/) {
			if (manualwork == 1) {
				GPIO_SetBits(GPIO_LEDWORK, LEDWORK);
				vTaskDelay(100);
				GPIO_ResetBits(GPIO_LEDWORK, LEDWORK);
				vTaskDelay(300);
			}
			if (manualwork == 0) {
				GPIO_SetBits(GPIO_LEDWORK, LEDWORK);
				vTaskDelay(2000);
				GPIO_ResetBits(GPIO_LEDWORK, LEDWORK);
				vTaskDelay(2000);
			}

		} else /*if (!LedConnect)*/
		{
			vTaskDelay(1000);
			GPIO_ResetBits(GPIO_LEDWORK, LEDWORK);
			vTaskDelay(3000);

		}
		if (LedReg) {
			GPIO_SetBits(GPIO_LED, LEDRED);
			GPIO_ResetBits(GPIO_LED, LEDGREEN);
			GPIO_ResetBits(GPIO_LEDWORK, LEDWORK); // для версии 4.49.0
			vTaskDelay(100);
			GPIO_SetBits(GPIO_LED, LEDGREEN);
			GPIO_SetBits(GPIO_LEDWORK, LEDWORK); // для версии 4.49.0
			vTaskDelay(100);
		}

		if (LedConnect) {
			if (LedSend) {

				GPIO_ResetBits(GPIO_LED, LEDRED);
				GPIO_SetBits(GPIO_LED, LEDGREEN);
				vTaskDelay(150);
				GPIO_ResetBits(GPIO_LED, LEDGREEN);
				GPIO_SetBits(GPIO_LED, LEDRED);
				GPIO_ResetBits(GPIO_LEDWORK, LEDWORK);
				LedSend = 0;
			} else if (LedReceive) {
				GPIO_ResetBits(GPIO_LED, LEDGREEN);
				GPIO_SetBits(GPIO_LED, LEDRED);
				vTaskDelay(150);
				GPIO_ResetBits(GPIO_LED, LEDGREEN);
				GPIO_SetBits(GPIO_LED, LEDRED);
				LedReceive = 0;

			} else {
				GPIO_ResetBits(GPIO_LED, LEDGREEN);
				GPIO_SetBits(GPIO_LED, LEDRED);
			}
			/*GPIO_ResetBits(GPIO_LEDWORK, LEDWORK);
			 vTaskDelay(50);
			 GPIO_SetBits(GPIO_LEDWORK, LEDWORK);
			 vTaskDelay(50); // для версии 4.49.0*/

		}

		else {

			if (LedNoModem) {
				GPIO_SetBits(GPIO_LED, LEDRED);
				vTaskDelay(1000);
				GPIO_ResetBits(GPIO_LED, LEDRED);
				vTaskDelay(250);
			} else if (LedSIM) {
				GPIO_SetBits(GPIO_LED, LEDRED);
				vTaskDelay(250);
				GPIO_ResetBits(GPIO_LED, LEDRED);
				vTaskDelay(1000);
			} else if (LedAPN) {
				GPIO_SetBits(GPIO_LED, LEDRED);
				vTaskDelay(250);
				GPIO_ResetBits(GPIO_LED, LEDRED);
				vTaskDelay(250);

			} else {
				GPIO_SetBits(GPIO_LED, LEDRED);
				GPIO_SetBits(GPIO_LED, LEDGREEN);
			}
		}

	}
}
