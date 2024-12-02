#include <stdlib.h>
#include <stdlib.h>
#include <stdarg.h>

#include "typedef.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart2.h"
#include "gprscon.h"
#include "rvnettcp.h"
#include "usermemory.h"

bool LedReg, LedConnect, LedReceive, LedSend, LedFLerr;
bool LedNoModem, LedSIM, LedAPN, LedGPS;
//bool ResetTime;
bool Flag_start;
extern bool Star_prog;

void vLedTask(void *pvParameters) {
	(void) pvParameters;

	volatile int heapSize = 0;

	while (1) {

		IWDG_ReloadCounter();

		//IWDG_SetReload(10000);
		// IWDG_ReloadCounter();

		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;

		///CheckLED();
		if (RAM.Refresh == 1) {
			GPIO_ResetBits(GPIO_Led, Led1_2);
			GPIO_SetBits(GPIO_Led, Led1_1);
			GPIO_ResetBits(GPIO_Led, Led2_1);
			GPIO_ResetBits(GPIO_Led, Led2_2);
			GPIO_ResetBits(GPIO_Led, Led3_2);
			GPIO_ResetBits(GPIO_Led3, Led3_1);

			/*vTaskDelay(250);
			 GPIO_ResetBits(GPIO_Led, Led1_2);
			 vTaskDelay(250);*/
		}

		if (Star_prog) {
			GPIO_ResetBits(GPIO_Led, Led1_1);
			GPIO_SetBits(GPIO_Led, Led1_2);
			vTaskDelay(250);
			GPIO_SetBits(GPIO_Led, Led1_1);
			GPIO_ResetBits(GPIO_Led, Led1_2);
			vTaskDelay(250);
		} else if (!Flag_start) {
			GPIO_ResetBits(GPIO_Led, Led1_1);
			GPIO_SetBits(GPIO_Led, Led1_2);
			GPIO_ResetBits(GPIO_Led, Led2_1);
			GPIO_SetBits(GPIO_Led, Led2_2);
			GPIO_ResetBits(GPIO_Led3, Led3_1);
			GPIO_SetBits(GPIO_Led, Led3_2);
			vTaskDelay(1000);
			GPIO_ResetBits(GPIO_Led, Led1_2);
			GPIO_SetBits(GPIO_Led, Led1_1);
			GPIO_ResetBits(GPIO_Led, Led2_2);
			GPIO_SetBits(GPIO_Led, Led2_1);
			GPIO_ResetBits(GPIO_Led, Led3_2);
			GPIO_SetBits(GPIO_Led3, Led3_1);
			vTaskDelay(1000);
			GPIO_ResetBits(GPIO_Led, Led1_1);
			GPIO_SetBits(GPIO_Led, Led1_2);
			GPIO_ResetBits(GPIO_Led, Led2_1);
			GPIO_ResetBits(GPIO_Led, Led2_2);
			GPIO_ResetBits(GPIO_Led, Led3_2);
			GPIO_ResetBits(GPIO_Led3, Led3_1);
			Flag_start = 1;
		} else {

			if (RAM.DiagnDevice[0] != 0) {
				GPIO_ResetBits(GPIO_Led, Led1_2);
				GPIO_SetBits(GPIO_Led, Led1_1);		//моргает красный
				vTaskDelay(250);
				GPIO_ResetBits(GPIO_Led, Led1_1);
				vTaskDelay(250);

			} else if (RAM.LogicErrorFlags != 0) {
				GPIO_ResetBits(GPIO_Led, Led1_2);
				GPIO_SetBits(GPIO_Led, Led1_1);		//красный
			} else {
				GPIO_ResetBits(GPIO_Led, Led1_1);
				GPIO_SetBits(GPIO_Led, Led1_2);   //зеленый

			}

			/*	if(ResetTime)
			 {
			 vTaskDelay(1000);
			 GPIO_ResetBits(GPIO_Led, Led1_2);

			 vTaskDelay(1000);
			 }
			 else
			 {
			 vTaskDelay(1000);
			 GPIO_ResetBits(GPIO_Led, Led1_2);

			 vTaskDelay(3000);

			 }*/

			/*if ((LedReg)||(LedSIM)||(LedAPN))
			 {
			 GPIO_SetBits(GPIO_Led, Led2_2);
			 GPIO_ResetBits(GPIO_Led, Led2_1);

			 }*/

			if (LedConnect) {
				if (LedSend) {
					GPIO_ResetBits(GPIO_Led, Led2_2);
					GPIO_SetBits(GPIO_Led, Led2_1);
					vTaskDelay(150);
					GPIO_ResetBits(GPIO_Led, Led2_1);
					GPIO_ResetBits(GPIO_Led, Led2_2);
					LedSend = 0;
				} else if (LedReceive) {
					GPIO_ResetBits(GPIO_Led, Led2_1);
					GPIO_SetBits(GPIO_Led, Led2_2);
					vTaskDelay(150);
					GPIO_ResetBits(GPIO_Led, Led2_1);
					GPIO_SetBits(GPIO_Led, Led2_2);
					LedReceive = 0;

				} else {
					GPIO_ResetBits(GPIO_Led, Led2_1);
					GPIO_ResetBits(GPIO_Led, Led2_2);
				}

			} else if (LedGPS) {
				GPIO_ResetBits(GPIO_Led, Led2_1);
				GPIO_SetBits(GPIO_Led, Led2_2);
				vTaskDelay(150);
				GPIO_SetBits(GPIO_Led, Led2_1);
				GPIO_ResetBits(GPIO_Led, Led2_2);
				vTaskDelay(150);
			}

			/*if (LedFLerr)
			 {
			 GPIO_ResetBits(GPIO_Led, Led2_2);
			 GPIO_SetBits(GPIO_Led, Led2_1);

			 }*/
		}

	}
}
