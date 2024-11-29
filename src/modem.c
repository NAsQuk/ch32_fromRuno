/*
 * modem.c
 *
 *  Created on: Jan 18, 2010
 *      Author: albert
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "modem.h"
#include "usart2.h"
#include "typedef.h"
#include "usermemory.h"
#include "led.h"
#include "rtclock.h"
#include "journal.h"

#define MODEM_SIO_BUFF_SIZE 1024
#define MODEM_DISABLE { GPIO_SetBits(GPIOB, GPIO_Pin_0);}  //инверсия SW ON
#define  MODEM_ENABLE {GPIO_ResetBits(GPIOB, GPIO_Pin_0); }

//#define MODEM_DISABLE { GPIO_ResetBits(GPIOB, GPIO_Pin_0);}  //для старого корпуса (вер 4.49.0)
//#define  MODEM_ENABLE {GPIO_SetBits(GPIOB, GPIO_Pin_0); }

#define GPRS_IDLE_TIMEOUT_MS      ( 60*4*1000) // 4 min idle and restart

uint32 GprsIdleMSec;

//#define CTS_ENABLE { GPIO_SetBits(GPIOB, GPIO_Pin_13);}
//#define CTS_DISABLE { GPIO_ResetBits(GPIOB, GPIO_Pin_13);}

uint8 sio_buf[MODEM_SIO_BUFF_SIZE];
uint8 sio_buf2[MODEM_SIO_BUFF_SIZE];
uint8 sio_buf3[MODEM_SIO_BUFF_SIZE];
uint8 sio_buf4[MODEM_SIO_BUFF_SIZE];

#define SIOFIFOSIZE 1024
uint8 sio_fifo[SIOFIFOSIZE];
//uint8 sio_buf[64];
volatile int bufpoint; //, bufsize;
volatile uint16_t bufsize;
volatile uint16_t bufsize2, bufsize3, bufsize4;

extern bool FlagGSMtime;
extern bool FlagGPStime;

int numRD; //IDсервера
int i_OPEN;
int i_Select;

bool LedNoModem_Journal = 0;
bool LedSIM_Journal = 0;
bool LedAPN_Journal = 0;
int flag_first = 0;
#define STRBUFLEN 256

char strbuf[STRBUFLEN];
#define MODEM_PORT (&COM0)

const AT_COMMAND atset[AT_AMOUNT] = { "+PBREADY", MA_CALLREADY, "OK", MA_OK,
		"SERVER OK", MA_SERVEROK, "NORMAL POWER DOWN", MA_NORMALPOWERDOWN,
		"STATE: IP STATUS", MA_STATEIPSTATUS, "STATE: IP CLOSE",
		MA_STATEIPCLOSE, "STATE: IP INITIAL", MA_STATEIPINITIAL,
		"STATE: IP GPRSACT", MA_STATEIPGPRSACT, "STATE: PDP DEACT",
		MA_STATEPDPDEACT, "> ", MA_CIPSENDREADY, "CONNECT", MA_CONNECT,
		"NO CARRIER", MA_NOCARRIER, "ERROR", MA_ERROR, "READY", MA_READY };

bool IsLiteral(char ch) {
	if ((ch >= ' ') && (ch <= '~')) {
		return true;
	}
	return false;
}

void InitSioBuf() {
	bufpoint = 0;
	bufsize = 0;
}

bool PopFromBuf(char *ch) {
	if (bufpoint >= bufsize) {
		return false;
	}
	*ch = sio_buf[bufpoint];
	bufpoint++;
	return true;
}

void FillBufEx(char *abuf, int16_t *aSize) {
	InitSioBuf();
	uint16_t time = 4000;
	uint16_t counter = 0;

	while (ReceivedMsg.flag == false && counter < time) {
		counter++;
		vTaskDelay(1);
	}

	if (ReceivedMsg.flag == true) {
		for (int i = 0; i < ReceivedMsg.size; i++) {
			abuf[i] = ReceivedMsg.buffer[i];
		}
		*aSize = ReceivedMsg.size;
		ReceivedMsg.flag = false;
		ReceivedMsg.size = 0;
		//TIM_Cmd(TIM5, ENABLE);
	}
	//vTaskDelay(1000);
	//bufsize = uart2Read(sio_buf,256);

}

void FillBuf() {
	InitSioBuf();
	uint16_t time = 1500;
	uint16_t counter = 0;

	while (ReceivedMsg.flag == false && counter < time) {
		counter++;
		vTaskDelay(1);
	}

	if (ReceivedMsg.flag == true) {
		for (int i = 0; i < ReceivedMsg.size; i++) {
			sio_buf[i] = ReceivedMsg.buffer[i];
		}
		bufsize = ReceivedMsg.size;
		ReceivedMsg.size = 0;
		ReceivedMsg.flag = false;
		//TIM_Cmd(TIM5, ENABLE);
	}
	//vTaskDelay(1000);
	//bufsize = uart2Read(sio_buf,256);

}

bool ReadString(char *str, int size) {
	char ch;
	if (bufpoint >= bufsize) {
		FillBuf();
	}
	do // remove non symbolic data
	{
		if (PopFromBuf(&ch) == false) {
			return false;
		}
	} while (!IsLiteral(ch));
	do {
		if (IsLiteral(ch)) {
			*str = ch;
			str++;
		} else // packet end
		{
			*str = 0; //  add end of string
			return true;
		}
	} while (PopFromBuf(&ch) == true);
	return false;
}

MDEM_ANSWER GetModemAnswer(char *s) {
	int i;
	if (*s == 0) {
		return MA_UNKNOWN;
	}
	for (i = 0; i < AT_AMOUNT; i++) {
		if (strcmp(atset[i].str, s) == 0) {
			return (atset[i].ma);
		}
	}
	return MA_UNKNOWN;
}

MDEM_ANSWER ReadModem() {
	if (ReadString(strbuf, STRBUFLEN) == false) {
		return MA_UNKNOWN;
	}
	return GetModemAnswer((char*) strbuf);
}
/* ----------------------- Start implementation -----------------------------*/
void ModemWrite(char *str) {

	uint32 i = 0;
	char *tmpstr = str;
	while (*str) {
		str++;
		i++;
		if (i == 62)
			break;
	}
	//AT91F_USART_SEND(MODEM_PORT, (uint8*) tmpstr, i);
	uart2Write((uint8*) tmpstr, i);
}
bool WaitAnsver(MDEM_ANSWER ma, int i) {
	//int i = 10;

	while (ReadModem() != ma) {
		if (i == 0) {
			return false;
		}
		i--;
	}
	//	dbgmessage("Ok\n");
	return true;
}

bool WaitCallReady() {
	int i = 0;
	dbgmessage("Waiting for modem.");
	while (ReadModem() != MA_CALLREADY) {
		vTaskDelay(200);
		if (i > 7) {
			return false;
		}
		i++;
	};
	return true;
}
bool WaitOk() {
	vTaskDelay(100);
	ModemWrite("AT\r\n");
	return WaitAnsver(MA_OK, 30);
}

bool WaitAtd() {
	vTaskDelay(100);
	//ModemWrite("ATZ\r\n");
	ModemWrite("AT&D0\r\n");  //заменить на установку скорости 115200?
	return WaitAnsver(MA_CALLREADY, 10);
	//return WaitAnsver(MA_OK, 30);
}
bool WaitAtd1() {
	vTaskDelay(100);
	//ModemWrite("ATZ\r\n");
	ModemWrite("AT&D0\r\n");  //заменить на установку скорости 115200?
	return WaitAnsver(MA_OK, 10);
	//return WaitAnsver(MA_OK, 30);
}

bool WaitAte() {
	vTaskDelay(100);
	ModemWrite("AT+SIGNAL?\r\n"); //0v1
	return WaitAnsver(MA_OK, 30);

}
// уровень сигнала
bool CSQ() {
	ModemWrite("AT+CSQ\r\n");
	//ModemWrite("AT+IPR?\r\n");// узнать скорость
	vTaskDelay(100);
	bufsize2 = uart2Read(sio_buf2, 256);
	uint16 CSQ = 0, i1 = 0, i2 = 0;
	if (bufsize2 != 0) {
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == '+') && (sio_buf2[i + 1] == 'C')
					&& (sio_buf2[i + 2] == 'S') && (sio_buf2[i + 3] == 'Q')
					&& (sio_buf2[i + 4] == ':')) {
				char c1, c2;
				c1 = sio_buf2[i + 7];
				c2 = sio_buf2[i + 6];
				CSQ = atoi1(&c2);
				RAM.CSQ = CSQ;
				if (RAM.CSQ == 99) {
					/*	char buffer[256];
					 char minbuf[32];
					 itoa(RAM.CSQ, minbuf);
					 strcpy(buffer, "GSM нет сигнала = ");
					 strcat(buffer, minbuf);
					 JrnlWrite(buffer);*/
					return false;
				}
				return true;
			}
		}

	}
}

bool GSM_clock() {
	char buffer[3];
	buffer[2] = 0;
	DATATIME dtC;
	int temp_zone;
	int count = 0;

	vTaskDelay(300);
	while (count < 50) {
		ModemWrite("AT+CCLK?\r\n");
		vTaskDelay(100);
		bufsize2 = uart2Read(sio_buf2, 256);
		if (bufsize2 > 24)
			break;
		count++;
		vTaskDelay(100);
	}
	if (bufsize2 != 0) {
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == '+') && (sio_buf2[i + 1] == 'C')
					&& (sio_buf2[i + 2] == 'C') && (sio_buf2[i + 3] == 'L')
					&& (sio_buf2[i + 4] == 'K') && (sio_buf2[i + 5] == ':')) {
				buffer[0] = sio_buf2[i + 8];
				buffer[1] = sio_buf2[i + 9];
				dtC.Years = atoi1(buffer);

				buffer[0] = sio_buf2[i + 11];
				buffer[1] = sio_buf2[i + 12];
				dtC.Month = atoi1(buffer);

				buffer[0] = sio_buf2[i + 14];
				buffer[1] = sio_buf2[i + 15];
				dtC.Data = atoi1(buffer);

				buffer[0] = sio_buf2[i + 17];
				buffer[1] = sio_buf2[i + 18];
				dtC.Hour = atoi1(buffer);

				buffer[0] = sio_buf2[i + 20];
				buffer[1] = sio_buf2[i + 21];
				dtC.Min = atoi1(buffer);

				buffer[0] = sio_buf2[i + 23];
				buffer[1] = sio_buf2[i + 24];
				dtC.Sec = atoi1(buffer);

				if ((DeviceID == 3) || (DeviceID == 1)) {
					buffer[0] = sio_buf2[i + 26];
					buffer[1] = sio_buf2[i + 27];
					temp_zone = atoi1(buffer);

					if (temp_zone == 12) {
						dtC.Hour = dtC.Hour + 3;
					}
				}
			}
		}
		dtC.Years += 2000;

		rtcSetDataTime(&dtC);

		FlagGSMtime = 1;
		return true;

	}

}

void ATSTAT() {
	if ((DeviceID == 2)) {
		ModemWrite("at+qistat\r\n");
		vTaskDelay(10);
		bufsize2 = uart2Read(sio_buf2, 256);
		if (bufsize2 != 0) {
			for (int i = 0; i < bufsize2; i++) {
				if ((sio_buf2[i] == 'I') && (sio_buf2[i + 1] == 'N')
						&& (sio_buf2[i + 2] == 'I') && (sio_buf2[i + 3] == 'T')
						&& (sio_buf2[i + 4] == 'I') && (sio_buf2[i + 5] == 'A')
						&& (sio_buf2[i + 6] == 'L')) {
					/*char buffer[256];
					 char minbuf[32];
					 itoa(RAM.CSQ, minbuf);
					 strcpy(buffer, "GSM обрыв = ");
					 strcat(buffer, minbuf);
					 JrnlWrite(buffer);*/
				}

			}
		}
	}

}

bool ATCPIN() {
	vTaskDelay(200);
	ModemWrite("AT+CPIN?\r\n");
	vTaskDelay(200);
	bufsize2 = uart2Read(sio_buf2, 256);
	if (bufsize2 != 0) {
		for (int i = 2; i < bufsize2; i++) {
			if ((sio_buf2[i] == '+') && (sio_buf2[i + 1] == 'C')
					&& (sio_buf2[i + 2] == 'P') && (sio_buf2[i + 3] == 'I')
					&& (sio_buf2[i + 4] == 'N') && (sio_buf2[i + 5] == ':')
					&& (sio_buf2[i + 6] == ' ') && (sio_buf2[i + 7] == 'R')
					&& (sio_buf2[i + 8] == 'E') && (sio_buf2[i + 9] == 'A')
					&& (sio_buf2[i + 10] == 'D') && (sio_buf2[i + 11] == 'Y')) {
				if (LedSIM_Journal) {
					LedSIM_Journal = 0;
					JrnlWrite("GSM:сим-карта ОК");
				}

				LedSIM = 0;
				return true;

			}
		}
	}

	return false;

}

bool ATCREG() {
	vTaskDelay(100);
	ModemWrite("AT+CREG?\r\n");
	vTaskDelay(100);
	bufsize2 = uart2Read(sio_buf2, 256);
	if (bufsize2 != 0) {
		for (int i = 2; i < bufsize2; i++) {
			if ((sio_buf2[i] == '+') && (sio_buf2[i + 1] == 'C')
					&& (sio_buf2[i + 2] == 'R') && (sio_buf2[i + 3] == 'E')
					&& (sio_buf2[i + 4] == 'G') && (sio_buf2[i + 5] == ':')
					&& (sio_buf2[i + 6] == ' ') && (sio_buf2[i + 7] == '0')
					&& (sio_buf2[i + 8] == ',') && (sio_buf2[i + 9] == '1')) {
				return true;
			}
		}
	}

	return false;

}

bool ATOPEN() {
	vTaskDelay(100);
	ModemWrite("AT+QIOPEN=1,1,\"TCP LISTENER\",\"127.0.0.1\",0,4444,0\r\n");
	vTaskDelay(100);
	bufsize2 = uart2Read(sio_buf2, 256);
	if (bufsize2 != 0) {
		for (int i = 2; i < bufsize2; i++) {
			if ((sio_buf2[i] == 'O') && (sio_buf2[i + 1] == 'K')
					&& (sio_buf2[i + 6] == '+') && (sio_buf2[i + 7] == 'Q')
					&& (sio_buf2[i + 8] == 'I') && (sio_buf2[i + 9] == 'O')
					&& (sio_buf2[i + 10] == 'P') && (sio_buf2[i + 11] == 'E')
					&& (sio_buf2[i + 12] == 'N') && (sio_buf2[i + 13] == ':')
					&& (sio_buf2[i + 14] == ' ') && (sio_buf2[i + 15] == '1')
					&& (sio_buf2[i + 16] == ',') && (sio_buf2[i + 17] == '0'))

					{
				i_OPEN = 0;
				return true;
			}
			if ((sio_buf2[i] == 'O') && (sio_buf2[i + 1] == 'K')
					&& (sio_buf2[i + 6] == '+') && (sio_buf2[i + 7] == 'Q')
					&& (sio_buf2[i + 8] == 'I') && (sio_buf2[i + 9] == 'O')
					&& (sio_buf2[i + 10] == 'P') && (sio_buf2[i + 11] == 'E')
					&& (sio_buf2[i + 12] == 'N') && (sio_buf2[i + 13] == ':')
					&& (sio_buf2[i + 14] == ' ') && (sio_buf2[i + 15] == '1')
					&& (sio_buf2[i + 16] == ',') && (sio_buf2[i + 17] == '5')
					&& (sio_buf2[i + 18] == '6') && (sio_buf2[i + 19] == '3'))

					{
				if (!WaitCloseServer()) //если ответ 1,563 то закрыть tcp
					continue;
				i_OPEN = 5;
				return false;
			}

		}

	}
	vTaskDelay(5000);
	return false;
}

bool WaitClose(int IDs) {

	char buffer[256];
	char minbuf[32];

	itoa1(IDs, minbuf);
	strcpy(buffer, "AT+QICLOSE="); // закрыть TCPсоединение
	strcat(buffer, minbuf);
	strcat(buffer, "\r\n");
	ModemWrite(buffer);
	vTaskDelay(100);
	return WaitAnsver(MA_OK, 1000);

}

bool WaitCloseServer() {
	ModemWrite("AT+QICLOSE=1\r\n"); // закрыть TCPсервер
	vTaskDelay(100);
	return WaitAnsver(MA_OK, 1000);
}

bool WaitGprsTime() {
	if (flag_first == 0) {
		if ((DeviceID == 1) || (DeviceID == 3)) {
			vTaskDelay(100);
			ModemWrite("AT+CTZR=2\r\n");
			if (WaitAnsver(MA_OK, 50) == false)
				return false;

			vTaskDelay(100);
			ModemWrite("AT+CTZU=1\r\n");
			if (WaitAnsver(MA_OK, 50) == false)
				return false;

			flag_first = 2;
			LedNoModem = 0;
			return true;
		} else {
			vTaskDelay(100);
			ModemWrite("AT+CTZU=3\r\n");
			if (WaitAnsver(MA_OK, 30) == false)
				return false;

			vTaskDelay(100);
			ModemWrite("AT+QNITZ=1\r\n");
			if (WaitAnsver(MA_OK, 30) == false)
				return false;

			flag_first = 2;
			LedNoModem = 0;
			return true;

		}

	}

}

//======================================================
bool WaitGprsConnectUC20() {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	char myIp[17];
	char strsipstart[256];
	char buf[64];
	//char header[] = "IPD";
	int packSize;
	int headerSize;
	//FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	int i;

	vTaskDelay(100);
	ModemWrite("AT+QIDEACT=1\r\n");    // деактивация
	/*if (WaitAnsver(MA_OK,10) == false)
	 return false;*/
	vTaskDelay(4000); //40c

	//vTaskDelay(1000);
//	ModemWrite("AT+QIMODE=0\r\n");  //режим
//	if (WaitAnsver(MA_OK,10) == false)
//				return false;

	vTaskDelay(100);
	BKP_WriteBackupRegister(BKP_DR7, 27);

	//LedAPN = 1;
//	if (strlen(pxConfig->devcfg.gprs.login) > 16
//			|| strlen(pxConfig->devcfg.gprs.password) > 16
//			|| strlen(pxConfig->devcfg.gprs.adparam) > 32) {
//		strcpy(buf, "AT+QICSGP=1,1,\""); //AT+QICSGP=1,1,"vmi.velcom.by"," "," ",1
//		strcat(buf, PPP_ADPARAM);
//		strcat(buf, "\",\"");
//		strcat(buf, PPP_USER);
//		strcat(buf, "\",\"");
//		strcat(buf, PPP_PASS);
//		strcat(buf, "\"\,1\r\n");
//	} else {
	strcpy(buf, "AT+QICSGP=1,1,\"");
	strcat(buf, pxConfig->devcfg.gprs.adparam);
	strcat(buf, "\",\"");
	strcat(buf, pxConfig->devcfg.gprs.login);
	strcat(buf, "\",\"");
	strcat(buf, pxConfig->devcfg.gprs.password);
	strcat(buf, "\"\,1\r\n");
	//}
	vTaskDelay(100);
	ModemWrite(buf);
	if (WaitAnsver(MA_OK, 10) == false) {
		if (!LedAPN_Journal) {
			JrnlWrite("GSM:ошибка инициал.");
			LedAPN_Journal = 1;
		}
		return false;
	}
	LedAPN = 0;
	vTaskDelay(100);
	ModemWrite("AT+CMGF=1\r\n");
	if (WaitAnsver(MA_OK, 10) == false)
		return false;
	vTaskDelay(100);
	ModemWrite("AT+CNMI=2,2,0,0,0\r\n");
	if (WaitAnsver(MA_OK, 10) == false)
		return false;
	vTaskDelay(100);

	ModemWrite("AT+QIACT=1\r\n"); // активация
	//vTaskDelay(100);
	if (WaitAnsver(MA_OK, 4000) == false)
		return false;
	vTaskDelay(100);

	i_OPEN = 5;

	while (!ATOPEN()) // открытие и старт TCPсервера
	{
		if (i_OPEN == 0) {
			break;
		}
		i_OPEN--;
	}

	i_OPEN = 0;

	ModemWrite("AT+QILOCIP\r\n"); //локальный IP
	WaitAnsver(MA_UNKNOWN, 10);
	vTaskDelay(100);

	/*ModemWrite("AT+QISERVER=0\r\n"); // сервер  (=0 - это TCP сервер)
	 vTaskDelay(100);
	 if (WaitAnsver(MA_OK,50) == false)
	 return false;
	 vTaskDelay(100);


	 ModemWrite("AT+QISRVC=2\r\n"); //роль соединения
	 if (WaitAnsver(MA_OK,10) == false)
	 return false;*/

	//vTaskDelay(200);
	if (pxConfig->devcfg.GSMtime == 1)  //если включена синхронизация времени
			{

		GSM_clock();
		FlagGSMtime = 0;

	}

	return true;
}
//======================================================
bool WaitGprsConnectUC15() {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	char myIp[17];
	char strsipstart[256];
	char buf[64];
	//char header[] = "IPD";
	int packSize;
	int headerSize;
	//FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	int i;

	vTaskDelay(100);
	ModemWrite("AT+QIDEACT=1\r\n");    // деактивация
	/*if (WaitAnsver(MA_OK,10) == false)
	 return false;*/
	vTaskDelay(4000); //40c

	//vTaskDelay(1000);
//		ModemWrite("AT+QIMODE=0\r\n");  //режим
//		if (WaitAnsver(MA_OK,10) == false)
//					return false;

	vTaskDelay(100);
	BKP_WriteBackupRegister(BKP_DR7, 27);

	//LedAPN = 1;
	if (strlen(pxConfig->devcfg.gprs.login) > 16
			|| strlen(pxConfig->devcfg.gprs.password) > 16
			|| strlen(pxConfig->devcfg.gprs.adparam) > 32) {
		strcpy(buf, "AT+QICSGP=1,1,\""); //AT+QICSGP=1,1,"vmi.velcom.by"," "," ",1
		strcat(buf, PPP_ADPARAM);
		strcat(buf, "\",\"");
		strcat(buf, PPP_USER);
		strcat(buf, "\",\"");
		strcat(buf, PPP_PASS);
		strcat(buf, "\"\,1\r\n");
	} else {
		strcpy(buf, "AT+QICSGP=1,1,\"");
		strcat(buf, pxConfig->devcfg.gprs.adparam);
		strcat(buf, "\",\"");
		strcat(buf, pxConfig->devcfg.gprs.login);
		strcat(buf, "\",\"");
		strcat(buf, pxConfig->devcfg.gprs.password);
		strcat(buf, "\"\,1\r\n");
	}
	vTaskDelay(100);
	ModemWrite(buf);
	if (WaitAnsver(MA_OK, 10) == false) {
		if (!LedAPN_Journal) {
			JrnlWrite("GSM:ошибка инициал.");
			LedAPN_Journal = 1;
		}
		return false;
	}
	LedAPN = 0;
	vTaskDelay(100);
	ModemWrite("AT+CMGF=1\r\n");
	if (WaitAnsver(MA_OK, 10) == false)
		return false;
	vTaskDelay(100);
	ModemWrite("AT+CNMI=2,2,0,0,0\r\n");
	if (WaitAnsver(MA_OK, 10) == false)
		return false;
	vTaskDelay(100);

	ModemWrite("AT+QIACT=1\r\n"); // активация
	vTaskDelay(100);
	if (WaitAnsver(MA_OK, 4000) == false)
		return false;
	vTaskDelay(100);

	i_OPEN = 5;

	while (!ATOPEN()) // открытие и старт TCPсервера
	{
		if (i_OPEN == 0) {
			break;
		}
		i_OPEN--;
	}

	i_OPEN = 0;

	ModemWrite("AT+QILOCIP\r\n"); //локальный IP
	WaitAnsver(MA_UNKNOWN, 10);
	vTaskDelay(100);

	/*	ModemWrite("AT+QISERVER=0\r\n"); // сервер  (=0 - это TCP сервер)
	 vTaskDelay(100);
	 if (WaitAnsver(MA_OK,50) == false)
	 return false;
	 vTaskDelay(100);


	 ModemWrite("AT+QISRVC=2\r\n"); //роль соединения
	 if (WaitAnsver(MA_OK,10) == false)
	 return false;*/

	//vTaskDelay(200);
	if (pxConfig->devcfg.GSMtime == 1)  //если включена синхронизация времени
			{

		GSM_clock();
		FlagGSMtime = 0;

	}

	return true;
}
//======================================================
bool WaitGprsConnectM66() {
	char myIp[17];
	char strsipstart[256];
	char buf[64];
	char header[] = "IPD";
	int packSize;
	int headerSize;
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	int i;
	//		vTaskDelay(100);
	//		BKP_WriteBackupRegister(BKP_DR7, 31);
	//		if (!WaitAte())
	//			continue;
	//		vTaskDelay(100);
	//		BKP_WriteBackupRegister(BKP_DR7, 32);
	vTaskDelay(100);
	ModemWrite("AT+QIDEACT\r\n");    // деактивация
	vTaskDelay(4000);
	/*if (WaitAnsver(MA_OK,400) == false)
	 return false;
	 vTaskDelay(100);*/
	ModemWrite("AT+QIMODE=0\r\n");  //режим
	if (WaitAnsver(MA_OK, 50) == false)
		return false;

//	if (strlen(pxConfig->devcfg.gprs.login) > 16
//			|| strlen(pxConfig->devcfg.gprs.password) > 16
//			|| strlen(pxConfig->devcfg.gprs.adparam) > 32) {
//		strcpy(buf, "AT+CGDCONT=1,\"IP\",\""); //AT+CGDCONT=1,IP,"vmi.velcom.by"," "," ",1
//		strcat(buf, PPP_ADPARAM);
//		strcat(buf, "\"\r\n");
//	} else {
//		strcpy(buf, "AT+CGDCONT=1,\"IP\",\"");
//		strcat(buf, pxConfig->devcfg.gprs.adparam);
//		strcat(buf, "\"\r\n");
//	}
	/*strcpy(buf, "AT+CGDCONT=1,\"IP\",\"");
	 strcat(buf, PPP_ADPARAM);
	 strcat(buf, "\"\r");*/
//	vTaskDelay(100);
//	ModemWrite(buf);
	if (WaitAnsver(MA_OK, 50) == false)
		return false;

//LedAPN = 1;
	if (strlen(pxConfig->devcfg.gprs.login) > 16
			|| strlen(pxConfig->devcfg.gprs.password) > 16
			|| strlen(pxConfig->devcfg.gprs.adparam) > 32) {
		strcpy(buf, "AT+QIREGAPP=\"");
		strcat(buf, PPP_ADPARAM);
		strcat(buf, "\",\"");
		strcat(buf, PPP_USER);
		strcat(buf, "\",\"");
		strcat(buf, PPP_PASS);
		strcat(buf, "\"\r\n");
	} else {
		strcpy(buf, "AT+QIREGAPP=\"");
		strcat(buf, pxConfig->devcfg.gprs.adparam);
		strcat(buf, "\",\"");
		strcat(buf, pxConfig->devcfg.gprs.login);
		strcat(buf, "\",\"");
		strcat(buf, pxConfig->devcfg.gprs.password);
		strcat(buf, "\"\r\n");
	}
	vTaskDelay(100);
	ModemWrite(buf);
	if (WaitAnsver(MA_OK, 50) == false) {
		/*	if (!LedAPN_Journal)
		 {
		 JrnlWrite("GSM:ошибка инициал.");
		 LedAPN_Journal=1;
		 }*/
		return false;
	}
	vTaskDelay(200);
	LedAPN = 0;
	ModemWrite("AT+CMGF=1\r\n");
	if (WaitAnsver(MA_OK, 50) == false)
		return false;
	vTaskDelay(200);
	ModemWrite("AT+QIPROMPT=2\r\n");
	if (WaitAnsver(MA_OK, 50) == false)
		return false;
	vTaskDelay(200);
	ModemWrite("AT+QIHEAD=1\r\n");
	if (WaitAnsver(MA_OK, 50) == false)
		return false;

	vTaskDelay(200);
	ModemWrite("AT+QIACT\r\n"); // активация
	vTaskDelay(100);
	if (WaitAnsver(MA_OK, 1500) == false)
		return false;
	vTaskDelay(500);

	ModemWrite("AT+QICSGP=1,\"CMNET\"\r\n");
	if (WaitAnsver(MA_OK, 50) == false)
		return false;
	vTaskDelay(200);
	ModemWrite("AT+QILPORT=\"TCP\",\"4444\"\r\n");
	if (WaitAnsver(MA_OK, 50) == false)
		return false;
	vTaskDelay(200);
	ModemWrite("AT+QILOCIP\r\n");
	WaitAnsver(MA_UNKNOWN, 50);
	vTaskDelay(200);
	ModemWrite("AT+QISERVER\r\n"); // сервер  (=0 - это TCP сервер)
	vTaskDelay(200);
	if (WaitAnsver(MA_OK, 50) == false)
		return false;
	vTaskDelay(200);

	ModemWrite("AT+QISRVC=2\r\n"); //роль соединения
	if (WaitAnsver(MA_OK, 50) == false)
		return false;

	/*if (pxConfig->devcfg.GSMtime == 1)  //если включена синхронизация времени





	 {

	 GSM_clock();
	 FlagGSMtime = 0;

	 }*/

	return true;
}
bool WaitGprsConnectN715() {
	char myIp[17];
	char strsipstart[256];
	char buf[64];
	char header[] = "IPD";
	int packSize;
	int headerSize;
	int fl = 0, con = 10;
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	int i;

	vTaskDelay(100);
	ModemWrite("AT+XISP=0\r\n");   // выбор TCP
	vTaskDelay(100);
	if (WaitAnsver(MA_OK, 400) == false)
		return false;
	vTaskDelay(100);

	if (strlen(pxConfig->devcfg.gprs.login) > 16
			|| strlen(pxConfig->devcfg.gprs.password) > 16
			|| strlen(pxConfig->devcfg.gprs.adparam) > 32) {
		strcpy(buf, "AT+CGDCONT=1,\"IP\",\""); //AT+CGDCONT=1,IP,"vmi.velcom.by"," "," ",1
		strcat(buf, PPP_ADPARAM);
		strcat(buf, "\"\r\n");
	} else {
		strcpy(buf, "AT+CGDCONT=1,\"PPP\",\"");
		strcat(buf, pxConfig->devcfg.gprs.adparam);
		strcat(buf, "\"\r\n");
	}

	vTaskDelay(100);

	while (1) {
		ModemWrite(buf);
		vTaskDelay(100);
		bufsize2 = uart2Read(sio_buf2, 256);
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == 'O') && (sio_buf2[i + 1] == 'K')) {
				fl = 1;
				break;
			}

		}
		con--;
		if (!con)
			return false;
		else if (fl)
			break;
	}
	fl = 0;
	vTaskDelay(200);
	while (1) {
		vTaskDelay(100);
		ModemWrite("AT+XIIC=1\r\n");
		vTaskDelay(100);
		bufsize2 = uart2Read(sio_buf2, 256);
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == 'O') && (sio_buf2[i + 1] == 'K')) {
				fl = 1;
				break;
			}

		}
		con--;
		if (!con)
			return false;
		if (fl)
			break;
	}

	fl = 0;
	con = 15;
	while (1) {
		vTaskDelay(100);
		ModemWrite("AT+XIIC?\r\n");
		vTaskDelay(100);
		bufsize2 = uart2Read(sio_buf2, 256);
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == 'O') && (sio_buf2[i + 1] == 'K')) {
				fl = 1;
				break;
			}

		}
		if (fl)
			break;
	}

	vTaskDelay(200);
	fl = 0;

	while (1) {
		ModemWrite("AT+TCPLISTEN=4444\r\n");
		vTaskDelay(200);
		bufsize2 = uart2Read(sio_buf2, 256);
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == '0') && (sio_buf2[i + 1] == ',')
					&& (sio_buf2[i + 2] == 'O') && (sio_buf2[i + 3] == 'K')) {
				fl = 1;
				break;
			}
			if ((sio_buf2[i] == 'L') && (sio_buf2[i + 1] == 'i')
					&& (sio_buf2[i + 2] == 's') && (sio_buf2[i + 3] == 't')) {
				fl = 1;
				break;
			}

		}
		con--;
		if (!con)
			return false;
		if (fl)
			break;
	}
	vTaskDelay(200);
	fl = 0;
	con = 15;
	while (1) {
		vTaskDelay(200);
		ModemWrite("AT+IPSTATUS=0\r\n");
		bufsize2 = uart2Read(sio_buf2, 256);
		for (int i = 0; i < bufsize2; i++) {
			if ((sio_buf2[i] == 'S') && (sio_buf2[i + 1] == 'R')
					&& (sio_buf2[i + 2] == 'V')) {
				fl = 1;
				break;
			}

		}
		con--;
		if (!con)
			return false;
		if (fl)
			break;
	}

	return true;
}
//==========================================================================
bool SelectModem() {
	int count = 0;

	vTaskDelay(100);
	while (count < 50) {
		ModemWrite("AT+GMM\r\n");
		bufsize4 = uart2Read(sio_buf4, 256);
		if (bufsize4 > 4)
			break;
		count++;
		vTaskDelay(100);
	}
	if (bufsize4 != 0) {
		for (int i = 0; i < bufsize4; i++) {
			if ((sio_buf4[i] == 'U') && (sio_buf4[i + 1] == 'C')
					&& (sio_buf4[i + 2] == '2') && (sio_buf4[i + 3] == '0')) //UC20
					{
				DeviceID = 1;

				/*if (!WaitGprsTime())
				 continue;
				 if (flag_first==2)
				 {

				 break;
				 }
				 else
				 {*/
				if (!WaitGprsConnectUC20()) // подключение UC20
					continue;
				vTaskDelay(50);
				return true;
				//}
			} else if ((sio_buf4[i] == 'U') && (sio_buf4[i + 1] == 'C')
					&& (sio_buf4[i + 2] == '1') && (sio_buf4[i + 3] == '5')) //UC15
					{
				//if (DeviceID != 3)

				//{
				DeviceID = 3;
				/*if (!WaitGprsTime())
				 continue;
				 if (flag_first==2)
				 {
				 flag_first = 1;
				 i_Select = 0;
				 break;
				 }
				 else
				 {*/
				if (!WaitGprsConnectUC15()) // подключение UC15
					continue;
				vTaskDelay(50);
				return true;
				//	}
				/*	}
				 else
				 {
				 int i_GprsCon = 50;

				 while (!WaitGprsConnectUC15())
				 {
				 if (i_GprsCon == 0)
				 {
				 break;
				 }
				 i_GprsCon--;
				 }// подключение UC15

				 i_GprsCon = 0;

				 vTaskDelay(50);
				 return true;
				 }*/

			} else if ((sio_buf4[i] == 'Q')
					&& (sio_buf4[i + 1] == 'u')  			//Quectel_M66
					&& (sio_buf4[i + 2] == 'e') && (sio_buf4[i + 3] == 'c')
					&& (sio_buf4[i + 4] == 't') && (sio_buf4[i + 5] == 'e')
					&& (sio_buf4[i + 6] == 'l') && (sio_buf4[i + 7] == '_')
					&& (sio_buf4[i + 8] == 'M')/*&&(sio_buf4[i + 9] == '6')
					 && (sio_buf4[i + 10] == '6')*/) // в 4.49.0 убрать М66, чтоб подтягивался М12 тоже
					{
				DeviceID = 2;
				/*if (!WaitGprsTime())
				 continue;
				 if (flag_first==2)
				 {
				 break;
				 }
				 else
				 {*/

				/*int i_GprsCon = 50;

				 while (!WaitGprsConnectM66()) // подключение M66
				 {
				 if (i_GprsCon == 0)
				 {
				 break;
				 }
				 i_GprsCon--;
				 }
				 i_GprsCon = 0;*/
				if (!WaitGprsConnectM66()) // подключение M66
					continue;

				vTaskDelay(50);
				return true;
				//}
			} else if ((sio_buf4[i] == 'N') && (sio_buf4[i + 1] == '7')
					&& (sio_buf4[i + 2] == '1') && (sio_buf4[i + 3] == '5')) //UC15
					{
				DeviceID = 4;
				if (!WaitGprsConnectN715()) // подключение M66
					continue;

				vTaskDelay(50);
				return true;

			}
		}
	}

	return false;

}

void ModemInit() {

	/*USART_DeInit(USART3);
	 DMA_DeInit(DMA1_Channel2);
	 DMA_DeInit(DMA1_Channel3);*/
	uart2Init(115200);

	USART2TIMConfigure(50);

//------------------------------------------------------------
	BKP_WriteBackupRegister(BKP_DR7, 30);
	while (1) {
		int heapSize = 0;
		heapSize = xPortGetFreeHeapSize();
		heapSize += 0;

		LedNoModem = 1;
		if (initMK == 1) {
			MODEM_ENABLE
			;
			vTaskDelay(4000);
			MODEM_DISABLE
			;
			vTaskDelay(5000);
			MODEM_ENABLE
			;
			vTaskDelay(10000);
		} else {
			MODEM_ENABLE
			;
			vTaskDelay(3000);
			MODEM_DISABLE
			;
			vTaskDelay(2500);
			MODEM_ENABLE
			;
			vTaskDelay(5000);
		}

		//LedNoModem = 1;

		if (!WaitAtd()) {
			if (!WaitAtd1())
				continue;
		}

		if (!WaitOk())
			continue;
		vTaskDelay(100);

		LedNoModem = 0;

		int i_CPIN = 50;

		vTaskDelay(100);
		BKP_WriteBackupRegister(BKP_DR7, 32);

		while (!ATCPIN()) // проверка сим-карты
		{
			if (i_CPIN == 0) {
				if (!LedSIM_Journal) {
					JrnlWrite("GSM:ошибка сим-карты");
					LedSIM_Journal = 1;
				}
				LedSIM = 1;
				// break;
			}
			i_CPIN--;
		}
		if (!LedSIM) {
			i_CPIN = 0;

			LedReg = 1;
			vTaskDelay(50);

			while (!CSQ())
				continue; // уровень сигнала

			vTaskDelay(50);
			BKP_WriteBackupRegister(BKP_DR7, 33);

			int i_CREG = 60;

			while (!ATCREG()) // проверка регистрации в сети
			{
				if (i_CREG == 0) {
					break;
				}
				i_CREG--;
			}

			i_CREG = 0;

			vTaskDelay(50);

			i_Select = 100;

			while (!SelectModem()) {

				if (i_Select == 0) {
					break;
				}
				i_Select--;
			}
			i_Select = 0;

			vTaskDelay(50);
		}

		BKP_WriteBackupRegister(BKP_DR7, 34);
		return;
	}
}
//=========================================================================
int ModemReceiveData(uint8 *buf, int maxsize) {

	int packSize;
	int headerSize;
	int i;
	int res = 0;

	BKP_WriteBackupRegister(BKP_DR7, 35);
	if (DeviceID == 4) {
		//ModemWrite("+TCPRECV: 1,12\r\n");
	}
	FillBuf();
	if (bufsize == 0)
		return 0;

	int k = 0;
	//-------------UC15-----------------------
	//-------------UC20-----------------------
	if ((DeviceID == 1) || (DeviceID == 3)) {

		if ((bufsize > 0)) {
			for (i = 0; i < bufsize; i++) {
				if ((sio_buf[i] == '+') && (sio_buf[i + 1] == 'Q')
						&& (sio_buf[i + 2] == 'I') && (sio_buf[i + 3] == 'U')
						&& (sio_buf[i + 4] == 'R') && (sio_buf[i + 5] == 'C')
						&& (sio_buf[i + 6] == ':') && (sio_buf[i + 7] == ' ')
						&& (sio_buf[i + 8] == '"') && (sio_buf[i + 9] == 'r')
						&& (sio_buf[i + 10] == 'e') && (sio_buf[i + 11] == 'c')
						&& (sio_buf[i + 12] == 'v')
						&& (sio_buf[i + 13] == '"')) {
					numRD = 0; //ID сервер

					while (sio_buf[i + 15] != '\r') {
						numRD = numRD * 10 + sio_buf[i + 15] - '0';
						i++;
					}

					char buffer[256];
					char minbuf[32];
					for (int k = 0; k <= 32; k++) {
						minbuf[k] = '0';
					}

					itoa1(numRD, minbuf);
					strcpy(buffer, "AT+QIRD="); // для чтения входящего сообщения
					strcat(buffer, minbuf);
					strcat(buffer, "\r\n");
					ModemWrite(buffer);
					vTaskDelay(10);
					k = 0;
					break;
				}
			}
			for (i = 0; i < bufsize; i++) { //само входящее сообщение (нету заголовка IPD) +QRD:<len>\r<data>
				if ((sio_buf[i] == '+') && (sio_buf[i + 1] == 'Q')
						&& (sio_buf[i + 2] == 'I') && (sio_buf[i + 3] == 'R')
						&& (sio_buf[i + 4] == 'D') && (sio_buf[i + 5] == ':')) {
					k = 1;
					while (sio_buf[i + 7] != '\r') {
						res = res * 10 + sio_buf[i + 7] - '0'; //<len>
						i++;
					}
					headerSize = i + 9;

					break;
				}
			}
		}

		if (k == 0)
			return 0;

		BKP_WriteBackupRegister(BKP_DR7, 36);
		char *cmpbuf = "+QIRD";
		char *strbufsize[16];
		int cmpPoint = 0;
		int strbufsizePoint = 0;

		packSize = res;

		//headerSize++;
		while (bufsize < (packSize + headerSize)) {

			break;
			int tmpsize;
			FillBufEx((uint8*) (&sio_buf[bufsize]), tmpsize);
//********************************************************

			if (tmpsize == 0) {
				break;
			}
			bufsize += tmpsize;
		}
		i = 0;

		bufsize = res;

		while (bufsize) {
			if (i >= maxsize) {
				return 0;
			}
			*buf = sio_buf[i + headerSize];
			buf++;
			i++;
			bufsize--;
		}

		return packSize;
	}
	//-------------N715-----------------------
	else if (DeviceID == 4) {
		char header[] = "IPD";
		int numIPD = 0;
		int k = 1;

		for (i = 0; i < 3; i++) {
			if (sio_buf[i] != header[i]) {
				for (i = 0; i < bufsize; i++) {
					if ((sio_buf[i] == 'P') && (sio_buf[i + 1] == 'R')
							&& (sio_buf[i + 2] == 'E')
							&& (sio_buf[i + 3] == 'C')
							&& (sio_buf[i + 4] == 'V')
							&& (sio_buf[i + 5] == '(')
							&& (sio_buf[i + 6] == 'S')
							&& (sio_buf[i + 7] == ')')
							&& (sio_buf[i + 8] == ':')
							&& (sio_buf[i + 9] == ' ')
							&& (sio_buf[i + 11] == ',')) {
						numIPD = i + 12;
						numRD = sio_buf[i + 10] - 48;
						k = 1;
						break;
					} else
						k = 0;
				}
			}

		};

		if (k == 0)
			return 0;
		if (numIPD != 0) {
			for (i = 0; i < numIPD + 40; i++)
				sio_buf[i] = sio_buf[i + numIPD];
		}
		int len_pack = 0;
		while (sio_buf[len_pack] != ',') {
			len_pack++;
		}

		BKP_WriteBackupRegister(BKP_DR7, 36);
		char *cmpbuf = "IPD";
		char strbufsize[16];
		int cmpPoint = 0;
		int strbufsizePoint = 0;

		headerSize = 0;
		for (int k = 0; k <= 16; k++) {
			strbufsize[k] = '0';
		}
		while (sio_buf[headerSize] != ',') {

			strbufsize[strbufsizePoint++] = sio_buf[headerSize];
			headerSize++;
		}
		int it = 0;
		int res = 0;
		while (strbufsize[it] >= '0' && strbufsize[it] <= '9') {
			res = res * 10 + (strbufsize[it] - '0');
			it++;
			if (it == headerSize)
				break;
		}

		packSize = res;

		headerSize++;

		while (bufsize < (packSize + headerSize)) {

			break;
			int tmpsize;
			FillBufEx((uint8*) (&sio_buf[bufsize]), tmpsize);
			//********************************************************

			if (tmpsize == 0) {
				break;
			}
			bufsize += tmpsize;
		}
		i = 0;

		bufsize -= headerSize;
		bufsize -= numIPD;

		while (bufsize) {
			if (i >= maxsize) {
				return 0;
			}
			*buf = sio_buf[i + headerSize];
			buf++;
			i++;
			bufsize--;
		}

		return packSize;
	}
	//-------------M66-----------------------
	else if (DeviceID == 2) {
		char header[] = "IPD";
		int numIPD = 0;
		int k = 1;

		for (i = 0; i < 3; i++) {
			if (sio_buf[i] != header[i]) {
				for (i = 0; i < 50; i++) {
					if ((sio_buf[i] == 'I') && (sio_buf[i + 1] == 'P')
							&& (sio_buf[i + 2] == 'D')) {
						numIPD = i;
						k = 1;
						break;
					} else
						k = 0;
				}
			}

		};
		if (k == 0)
			return 0;
		if (numIPD != 0) {
			for (i = 0; i < numIPD + 40; i++)
				sio_buf[i] = sio_buf[i + numIPD];
		}

		BKP_WriteBackupRegister(BKP_DR7, 36);
		char *cmpbuf = "IPD";
		char *strbufsize[16];
		int cmpPoint = 0;
		int strbufsizePoint = 0;

		headerSize = 0;
		while (sio_buf[headerSize] != ':') {
			if (cmpPoint < 3) {
				if (cmpbuf[cmpPoint] == sio_buf[headerSize]) {
					cmpPoint++;
				}
			} else {
				strbufsize[strbufsizePoint++] = sio_buf[headerSize];
			}
			headerSize++;
		}
		int it = 0;
		int res = 0;
		while (strbufsize[it] >= '0' && strbufsize[it] <= '9') {
			res = res * 10 + strbufsize[it] - '0';
			it++;
		}

		packSize = res;

		headerSize++;
		while (bufsize < (packSize + headerSize)) {

			break;
			int tmpsize;
			FillBufEx((uint8*) (&sio_buf[bufsize]), tmpsize);
			//********************************************************

			if (tmpsize == 0) {
				break;
			}
			bufsize += tmpsize;
		}
		i = 0;

		bufsize -= headerSize;

		while (bufsize) {
			if (i >= maxsize) {
				return 0;
			}
			*buf = sio_buf[i + headerSize];
			buf++;
			i++;
			bufsize--;
		}

		return packSize;
	}

}

//=========================================================================+QILOCIP
void ModemSendData(uint8 *buf, uint8 size, int IDserv) {
	BKP_WriteBackupRegister(BKP_DR7, 37);
	char *tempbuf[6];
	char tempbuf1[2];
	char tempbuf2[4];
	char strCipsend[20];

	//-------------UC15-----------------------
	//-------------UC20-----------------------
	if ((DeviceID == 1) || (DeviceID == 3))

	{
		strcpy(strCipsend, "AT+QISEND=");
		itoa1(IDserv, tempbuf1);
		itoa1(size, tempbuf2);
		strcat(strCipsend, tempbuf1);
		strcat(strCipsend, ",");
		strcat(strCipsend, tempbuf2);
		strcat(strCipsend, "\r\n>");
		ModemWrite(strCipsend);
		vTaskDelay(10);
		//BKP_WriteBackupRegister(BKP_DR7, 38);
		uart2Write(buf, size);
		//uart2Write(buf, size);
	}
	//-------------N715-----------------------
	else if (DeviceID == 4) {
		//	ModemWrite("\r\n");
		strcpy(strCipsend, "AT+TCPSENDS=");
		itoa1(numRD, tempbuf1);
		itoa1(size, tempbuf2);
		strcat(strCipsend, tempbuf1);
		strcat(strCipsend, ",");
		strcat(strCipsend, tempbuf2);
		strcat(strCipsend, "\r\n");
		ModemWrite(strCipsend);
		if (WaitAnsver(MA_CIPSENDREADY, 2) == false)
			return;
		BKP_WriteBackupRegister(BKP_DR7, 38);
		uart2Write(buf, size);
		//	uart2Write("\n", 1);
		//-------------M66-----------------------
	} else if (DeviceID == 2) {
		strcpy(strCipsend, "AT+QISEND=");
		itoa1(size, tempbuf);
		strcat(strCipsend, tempbuf);
		strcat(strCipsend, "\r");
		ModemWrite(strCipsend);
		vTaskDelay(50);
		BKP_WriteBackupRegister(BKP_DR7, 38);
		uart2Write(buf, size);
	}

}

