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
	USART_InitTypeDef  USART_InitStructure;
	uint16 usbsize;
	uint8 usbbuf[256];
	//uint8 usbbufsizes[100];
	struct CONTENTS stru[100];
	uint16 usbbufsizesCounter=0;
	bool st = false;
	/* The parameters are not used. */
	(void) pvParameters;

	  Set_USBClock();
	  USB_Interrupts_Config();
	  USB_Init();
	  int heapSize = 0;
	  while (1)
	  {
			heapSize = xPortGetFreeHeapSize();
							heapSize +=0;
		  usbsize = UsbReceiveData(usbbuf,256);

		  if(usbsize > 0)
		  {

			  stru[usbbufsizesCounter].buf[0]=usbbuf[0];
			  stru[usbbufsizesCounter].buf[1]=usbbuf[1];
			  stru[usbbufsizesCounter].buf[2]=usbbuf[2];
			  stru[usbbufsizesCounter].buf[3]=usbbuf[3];
			  stru[usbbufsizesCounter].size = usbsize;
			  usbbufsizesCounter++;
			  if(usbbufsizesCounter>=100) usbbufsizesCounter = 0;
			  portENTER_CRITICAL();
			  usbsize = RVnetSlaveProcess(usbbuf,usbsize,1);
			  portEXIT_CRITICAL();
			  if(usbsize > 0)
			  {
				  UsbSendData(usbbuf,usbsize);
			  }
		  }
		  vTaskDelay(20);
	  }
}

