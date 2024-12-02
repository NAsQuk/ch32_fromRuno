#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
//#include "adc.h"
//#include "pwmcapture.h"
#include "rvnet.h"
#include "uart.h"
#include "crc.h"
#include "stm32f10x.h"
//#include "usb_lib.h"
//#include "usb_desc.h"
//#include "hw_config.h"
//#include "usb_pwr.h"


extern __IO uint32_t count_out;

__IO uint8_t PrevXferComplete = 1;
//extern uint8_t buffer_out[VIRTUAL_COM_PORT_DATA_SIZE];

/* Private variables ---------------------------------------------------------*/


struct CONTENTS
{
	uint8 buf[5];
	uint16 size;
};


void vUSBTask(void *pvParameters)
{
	//usb
	vTaskDelay(100);
	uint16 usbsize = 256;
	uint8 usbbuf[usbsize];

	int heapSize = 0;
	/* The parameters are not used. */
	(void) pvParameters;

	uart1Init(115200);
	vTaskDelay(50);

	while (1) {
		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;

		usbsize = uart1Read(usbbuf, 256);
		vTaskDelay(50);

		if (usbsize > 0) {
			portENTER_CRITICAL();
			usbsize = RVnetSlaveProcess(usbbuf, usbsize, 1);
			portEXIT_CRITICAL();
			vTaskDelay(15); // чтобы не тормозил usb

			if (usbsize > 0) {
				uart1Write(usbbuf, usbsize);
				vTaskDelay(15);
			}
		}
	//	vTaskDelay(100);
	}
}

