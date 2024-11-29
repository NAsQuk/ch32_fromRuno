#include <stdlib.h>
#include <stdarg.h>

#include "typedef.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart2.h"
#include "gprscon.h"
#include "rvnettcp.h"
#include "usermemory.h"

#include "board.h"
#include "modem.h"
#include "led.h"

#define MODEM_DISABLE { GPIO_SetBits(GPIOB, GPIO_Pin_0);}  //инверсия SW ON
#define  MODEM_ENABLE {GPIO_ResetBits(GPIOB, GPIO_Pin_0); }

//#define MODEM_DISABLE { GPIO_ResetBits(GPIOB, GPIO_Pin_0);}  //для старого корпуса (версия 4)
//#define  MODEM_ENABLE {GPIO_SetBits(GPIOB, GPIO_Pin_0); }

#define CSQ_TIMEOUT_MS		( 30000)
#define GPRS_IDLE_TIMEOUT_MS      ( 60*4*1000) // 4 min idle and restart
#define GPRS_BUF_SIZE      		   256
//extern int CountAnswer;
//extern uint8 sio_buf2[1024];
//extern volatile uint16 bufsize2;

uint32 GprsIdleMSecCSQ;
uint32 GprsConnectionsAttempt;

int gprssize;
bool restart;
int FlagNoModem_Journal;
uint8 gprsbuf[GPRS_BUF_SIZE];

void GprsIdleIncMSec() {
	GprsIdleMSec++;
	GprsIdleMSecCSQ++;
}

void vGprsConTask(void *pvParameters) {
	(void) pvParameters;

	//vTaskDelay(500);
	GprsConnectionsAttempt = 0;
	int heapSize = 0;
	while (1) {
		GprsConnectionsAttempt++;
		LedConnect = 0;
		/*if (LedSIM)
		 {   LedAPN = 0;
		 LedNoModem = 0;

		 }*/
		/*else if (LedAPN)
		 {
		 LedNoModem = 0;

		 }*/
		if ((LedNoModem) || (LedSIM) || (LedAPN)) {
			//LedNoModem = 1;
			if (!LedNoModem_Journal) {
				JrnlWrite("GSM:ошибка адаптера");
				LedNoModem_Journal = 1;

			}

			GprsIdleMSec = 0;
			LedNoModem = 0;
			LedSIM = 0;
			LedAPN = 0;

			RAM.CSQ = 0;
			GprsIdleMSec = GPRS_IDLE_TIMEOUT_MS + 1;
			//MODEM_DISABLE;
		} else {
			//ModemInit();

			if (!LedNoModem) {
				ModemInit();
			}

			if ((!LedSIM) && (!LedAPN) && (!LedNoModem)
					&& (GprsIdleMSec < GPRS_IDLE_TIMEOUT_MS)) // если нет ошибок
					{
				LedConnect = 1;
				LedReg = 0;
				LedNoModem_Journal = 0;

			}

			if (LedReg) {
				GprsIdleMSec = 0;
				LedNoModem = 0;

				RAM.CSQ = 0;
				MODEM_DISABLE
				;

				break;
			}

		}
		while ((!LedSIM) && (!LedAPN) && (!LedNoModem)) {

			if (GprsIdleMSec <= GPRS_IDLE_TIMEOUT_MS) //если есть обмены, всё хорошо
			{
				BKP_WriteBackupRegister(BKP_DR7, 28);
				gprssize = ModemReceiveData(gprsbuf, GPRS_BUF_SIZE);
				heapSize = xPortGetFreeHeapSize();
				heapSize += 0;

				if (gprssize != 0) {
					LedReceive = 1;
					gprssize = RVnetTcpSlaveProcess(gprsbuf, gprssize, 1);
					if (DeviceID == 4) {
						gprsbuf[gprssize + 1] = '\r';
					}
					heapSize = xPortGetFreeHeapSize();
					heapSize += 0;
					if (gprssize != 0) {
						LedSend = 1;
						ModemSendData(gprsbuf, gprssize, numRD);
						heapSize = xPortGetFreeHeapSize();
						heapSize += 0;
						portENTER_CRITICAL();
						{
							GprsIdleMSec = 0;
							GprsIdleMSecCSQ = 0;

						}
						portEXIT_CRITICAL();
					}
				}
			}
			if (GprsIdleMSec > GPRS_IDLE_TIMEOUT_MS) //перегрузка (если нет обменов больше 4 мин)
			{

				RAM.CSQ = 0;

				GprsIdleMSec = 0;
				LedConnect = 0;
				LedReg = 0;

				if (DeviceID == 1) {
					//ATSTAT();

					if (numRD > 0) {
						if (!WaitClose(numRD))
							continue;
						numRD = 0;
						vTaskDelay(10);

						if (!WaitCloseServer())
							continue;
						vTaskDelay(10);
					}

					ModemWrite("AT+QPOWD\r\n");
					vTaskDelay(1500);

				} else if (DeviceID == 3) {
					if (numRD > 0) {
						if (!WaitClose(numRD))
							continue;
						numRD = 0;
						vTaskDelay(10);

						if (!WaitCloseServer())
							continue;
						vTaskDelay(10);
					}

					ModemWrite("AT+QPOWD\r\n");
					vTaskDelay(1500);

				}

				else if (DeviceID == 2) {
					ModemWrite("AT+QPOWD=0\r\n");
					vTaskDelay(1000);
				}

				MODEM_DISABLE
				;

				vTaskDelay(15000); //15сек

				GprsIdleMSec = 0;
				LedConnect = 0;
				LedReg = 0;
				LedSIM = 0;
				LedAPN = 0;

				break;
			}
			if (GprsIdleMSecCSQ > CSQ_TIMEOUT_MS) //проверка уровень сигнала
			{
				CSQ();
				//ATSTAT();
				GprsIdleMSecCSQ = 0;

			}
			vTaskDelay(1);
		}
		if ((LedNoModem) || (LedSIM) || (LedAPN)) {

			if ((!LedNoModem_Journal) && (FlagNoModem_Journal == 2)) {

				JrnlWrite("Ошибка адаптера");
				LedNoModem_Journal = 1;
			}
			if (FlagNoModem_Journal < 2) {
				FlagNoModem_Journal++;
			}
			GprsIdleMSec = 0;
			LedNoModem = 0;
			LedReg = 0;
			LedSIM = 0;
			LedAPN = 0;

			RAM.CSQ = 0;

			if (!restart) {
				MODEM_DISABLE
				;
				restart = 1;
			} else {
				restart = 0;
			}

			//vTaskDelay(15000); //15сек
			//break; // try to Init modem again
		}

		vTaskDelay(1000);
	}
}

