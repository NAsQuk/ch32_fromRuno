/*******************************************************************************
 * Project            : STM32 MINI Digital Picture Frame

 * File Name          : main.c
 * Author             : Martin Thomas, main-skeleton based on code from the
 *                      STMicroelectronics MCD Application Team
 * Version            : see VERSION_STRING below
 * Date               : see VERSION_STRING below
 * Description        : Main program body for the SD-Card tests
 ********************************************************************************
 * License: BSD
 *******************************************************************************/
//ѕривет///
#define VERSION_STRING "V1.0.0 24.03.2011"
/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include "board.h"
#include "hwinit.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gprscon.h"
#include "logica.h"
#include "reletest.h"
#include "rs485.h"
#include "usbtask.h"
#include "led.h"
#include "clock.h"
#include "I2CRoutines.h"
#include "usermemory.h"
#include "rtclock.h"
#include "stm32f10x_iwdg.h"

int CountAnswer = 0;

/* Private function prototypes -----------------------------------------------*/
void GPIO_Configuration(void);
void IWD_Start(void);
int initMK;
uint16_t T1;
/* Public functions -- -------------------------------------------------------*/

/////////////////////////////////////////////////////////////////
#define vLogicaTask_PRIORITY            ( tskIDLE_PRIORITY + 4 )
#define vRs485Task_PRIORITY            ( tskIDLE_PRIORITY + 3 )
#define vUSBTask_PRIORITY            ( tskIDLE_PRIORITY + 1 )
#define vReletestTask_PRIORITY            ( tskIDLE_PRIORITY + 5 )
#define vGprsConTask_PRIORITY           ( tskIDLE_PRIORITY + 5 )
#define vLedTask_PRIORITY           ( tskIDLE_PRIORITY)
#define vClockTask_PRIORITY           ( tskIDLE_PRIORITY+2)
/////////////////////////////////////////////////////////////////
int counttick = 0;

int main(void) {
	manualwork = 0;
	// онфигурирование микроконтроллера
	uint8_t *TestByte;
	TestByte = (uint8_t*) 0xE00FFFD0;
	if (*TestByte == 7) {
		initMK = 1; //GD
	} else {
		initMK = 0; //STM32
	}
	GPIO_Configuration();
	hwInit();
	MemInit();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);
	T1 = BKP_ReadBackupRegister(BKP_DR7);
	BKP_WriteBackupRegister(BKP_DR7, 2);
	IWD_Start();

	// Turn on/off LED(s)
	xTaskCreate(vLogicaTask, (const signed portCHAR * const ) "Logica",
			vLogicaTask_STACK_SIZE, NULL, vLogicaTask_PRIORITY,
			(xTaskHandle* ) NULL);

	xTaskCreate(vRs485Task, (const signed portCHAR * const ) "RS485",
			vRs485Task_STACK_SIZE, NULL, vRs485Task_PRIORITY,
			(xTaskHandle* ) NULL);

	xTaskCreate(vReletestTask, (const signed portCHAR * const ) "Reletest",
			vReletestTask_STACK_SIZE, NULL, vReletestTask_PRIORITY,
			(xTaskHandle* ) NULL);

	xTaskCreate(vUSBTask, (const signed portCHAR * const ) "USB",
			vUSBTask_STACK_SIZE, NULL, vUSBTask_PRIORITY, (xTaskHandle* ) NULL);

	xTaskCreate(vGprsConTask, (const signed portCHAR * const ) "GPRS",
			vGprsConTask_STACK_SIZE, NULL, vGprsConTask_PRIORITY,
			(xTaskHandle* ) NULL);

	xTaskCreate(vLedTask, (const signed portCHAR * const ) "LED",
			vLedTask_STACK_SIZE, NULL, vLedTask_PRIORITY, (xTaskHandle* ) NULL);

	xTaskCreate(vClockTask, (const signed portCHAR * const ) "CLOCK",
			vClockTask_STACK_SIZE, NULL, vClockTask_PRIORITY,
			(xTaskHandle* ) NULL);

	//Start the scheduler.
	vTaskStartScheduler();

	while (1) {

	};

	return 0;
}

void vApplicationIdleHook(void) {

}

void vApplicationTickHook(void) {
	counttick++;
	GprsIdleIncMSec();
	atcIncrementCounter();
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask,
		signed portCHAR *pcTaskName) {
	BKP_WriteBackupRegister(BKP_DR7, 1);
	while (1)
		;
}

void GPIO_Configuration(void) {

	GPIO_InitTypeDef GPIO_InitStructure;
	/* Enable GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* Enable GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	/* Enable GPIOB clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_I2C1,ENABLE);
	/* Configure I2C1 pins: SCL and SDA ----------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//===================== RS232 =========================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//===================== End of RS232 =========================
	/* Relays and Led */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6
			| GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//DS1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//DS4 DS5 DS6 DS7
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8 | GPIO_Pin_7
			| GPIO_Pin_6 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//DS8 DS9 DS10 DS11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_12 | GPIO_Pin_2
			| GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_2);
	GPIO_SetBits(GPIOC, GPIO_Pin_3);

}

void IWD_Start(void) {
// разрешаем доступ к регистам IWDG
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
// устанавливаем предделитель тактировани€ вач дога
	IWDG_SetPrescaler(IWDG_Prescaler_256);
// устанавливаем "до куда считать"
	IWDG_SetReload(0x0fff);
// перезагружаем IWDG
	IWDG_ReloadCounter();
// запускаем IWDG
	IWDG_Enable();
}

