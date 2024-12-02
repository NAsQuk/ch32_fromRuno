/*
 * translator.c Контроллер
 *
 *  Created on: May 26, 2010
 *      Author: albert
 */
// It file for Main translator func
#include "typedef.h"
#include "usermemory.h"
#include "translator.h"
#include "rtclock.h"
#include "journal.h"
#include "crc.h"
#include "modem.h"
#include "led.h"
#include "board.h"

#define GPRS_IDLE_TIMEOUT_MS      ( 60*4*1000) // 4 min idle and restart

extern uint16 T1;
bool bPowerOn = false;
bool ChanelDirect[8], ChanelRepare[8], ChanelChangeDirect[8],
		ChanelChangeRepare[8];
volatile DATATIME timer;
uint8 prevLogicFlags = 0;
uint32 testSwitchRele = 0;
uint16 prevErrDiscretLogic[4] = { 0, 0, 0, 0 };
uint16 CountReset = 0;
int manualwork;


void Reset() {
	for (int i = 0; i < DEVICE_RAMMEM_WORDS; i++) {
		RAM.dwords[i] = 0;

	}
	JrnlWrite("Сброс устройства");

	GprsIdleMSec = GPRS_IDLE_TIMEOUT_MS+1;
	LedNoModem = 0;
	if (!LedSIM)
	{
		CSQ();
	}
}

int GetChannelBitSignal(int channelNum, uint16* ChannelData) {

	if (ChannelData[(int) (channelNum / 4)] & (1 << ((4 * channelNum) % 16))) {
		return 1;
	} else {
		return 0;
	}
}

int GetChannelBitRepare(int channelNum, uint16* ChannelData) {
	int temp;
#ifdef LIDACONF
	if (ChannelData[(int) (channelNum / 4)]
			& (1 << ((4* channelNum ) % 16 + 1)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
#else
	if (ChannelData[(int) (channelNum / 4)]
			& (1 << ((4 * channelNum) % 16 + 3))) {
		return 1;
	} else {
		return 0;
	}
#endif
}

int GetChannelBitDirect(int channelNum, uint16* ChannelData) {
	if (ChannelData[(int) (channelNum / 4)]
			& (1 << ((4 * channelNum) % 16 + 2))) {
		return 1;
	} else {
		return 0;
	}
}

int GetChannelBitReserv(int channelNum, uint16* ChannelData) {
#ifdef LIDACONF
	if (ChannelData[(int) (channelNum / 4)]
			& (1 << ((4* channelNum ) % 16 + 3)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
#else
	if (ChannelData[(int) (channelNum / 4)]
			& (1 << ((4 * channelNum) % 16 + 1)))
		return 1;
	else
		return 0;
#endif
}

void SetChannelBitSignal(int channelNum, uint16* ChannelData, int bit) {
	if (bit) {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						| (1 << (4 * channelNum) % 16);
	} else {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						& (~(1 << (4 * channelNum) % 16));
	}
}
int block[8];
void SetChannelBitRepare(int channelNum, uint16* ChannelData, int bit) {
#ifdef LIDACONF
	if (bit)
	{
		ChannelData[(int) (channelNum / 4)]
		= ChannelData[(int) (channelNum / 4)] | (1 << ((4* channelNum )
						% 16 + 1));
	}
	else
	{
		ChannelData[(int) (channelNum / 4)]
		= ChannelData[(int) (channelNum / 4)] & (~(1
						<< ((4* channelNum ) % 16 + 1)));
	}
#else
	if (bit) {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						| (1 << ((4 * channelNum) % 16 + 3));
		block[channelNum] = 1;
	} else {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						& (~(1 << ((4 * channelNum) % 16 + 3)));
		block[channelNum] = 0;
	}
#endif
}

void SetChannelBitDirect(int channelNum, uint16* ChannelData, int bit) {
	if (bit) {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						| (1 << ((4 * channelNum) % 16 + 2));
	} else {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						& (~(1 << ((4 * channelNum) % 16 + 2)));
	}
}

void SetChannelBitReserv(int channelNum, uint16* ChannelData, int bit) {
#ifdef LIDACONF
	if (bit)
	{
		ChannelData[(int) (channelNum / 4)]
		= ChannelData[(int) (channelNum / 4)] | (1 << ((4* channelNum )
						% 16 + 3));
	}
	else
	{
		ChannelData[(int) (channelNum / 4)]
		= ChannelData[(int) (channelNum / 4)] & (~(1
						<< ((4* channelNum ) % 16 + 3)));
	}
#else
	if (bit) {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						| (1 << ((4 * channelNum) % 16 + 1));
	} else {
		ChannelData[(int) (channelNum / 4)] =
				ChannelData[(int) (channelNum / 4)]
						& (~(1 << ((4 * channelNum) % 16 + 1)));
	}
#endif
}

int GetDiskretFromRam(int i) {

	return ((RAM.diskrets[i / 11] >> (i % 11)) & 0x1);
}
void SetRelayToRam(int i, int vol) {
	if (vol != 0) {
		RAM.relays[i / 16] |= (1 << (i % 16));
	} else {
		RAM.relays[i / 16] &= ~(1 << (i % 16));
	}
}
int CheckDiscretMask(uint16 *mask) {
	int rezult = 1;
	for (int i = 0; i < 4; i++) {
		if ((RAM.diskrets[i] & mask[i]) != (mask[i])) {
			rezult = 0;
			RAM.ErrorDiscretLogic[i] &= (~mask[i]);
			RAM.ErrorDiscretLogic[i] |= ((~(RAM.diskrets[i] & mask[i]))
					& mask[i]);
		} else {
			RAM.ErrorDiscretLogic[i] &= (~mask[i]);
		}
	}
	return rezult;
}

int CheckDiscretMaskNegative(uint16 *mask) {
	int rezult = 1;
	for (int i = 0; i < 4; i++) {
		if ((RAM.diskrets[i] & mask[i]) == 0) {
			RAM.ErrorDiscretLogic[i] &= (~mask[i]);
		} else {
			rezult = 0;
			RAM.ErrorDiscretLogic[i] &= (~mask[i]);
			RAM.ErrorDiscretLogic[i] |= ((RAM.diskrets[i] & mask[i]));
		}
	}
	return rezult;
}
void CheckCUSignal() {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	char buffer[256];
	char minbuf[32];
	for (int i = 0; i < 8; i++) {
		if (pxConfig->devcfg.logica.confCU[i].releNum == 0)
			continue;
		itoa1(i + 1, minbuf);
		uint8 TempReleInd = pxConfig->devcfg.logica.confCU[i].releNum - 1;
		if (GetChannelBitSignal(i, RAM.OutputCommand)
				!= ((RAM.relays[TempReleInd / 16] >> (TempReleInd % 16))
						& (0x0001))) {
			if (GetChannelBitSignal(i, RAM.OutputCommand) == 1) {
				strcpy(buffer, "Вкл. канала:");
				strcat(buffer, minbuf);

				JrnlWrite(buffer);
			} else {
				strcpy(buffer, "Выкл. канала:");
				strcat(buffer, minbuf);
				JrnlWrite(buffer);
			}
		}
	}
}
bool NeedWrite = false;
char message[30];
void CheckErrors() {

	uint8 diffMask;
	if (prevLogicFlags != RAM.LogicErrorFlags) {
		diffMask = (prevLogicFlags) ^ (RAM.LogicErrorFlags);

		for (int i = 0; i < 8; i++) {
			if (((diffMask >> i) & 0x01) == 0x01) {
				if (((RAM.LogicErrorFlags >> i) & 0x01) == 0x01) {
					switch (i) {
					case 0:
						strcpy(message, "Ош. питания");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 1:
						strcpy(message, "Ош. цеп. упр.");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 2:
						strcpy(message, "Дверь открыта");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 3:
						strcpy(message, "Ош. управл");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 4:
						strcpy(message, "Ош. предохр.");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					default:
						strcpy(message, "Неизв ошибка!");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					}
				} else {
					switch (i) {
					case 0:
						strcpy(message, "Норма питание");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 1:
						strcpy(message, "Норма цеп.упр.");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 2:
						strcpy(message, "Дверь закрыта");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 3:
						strcpy(message, "Норма упр.");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					case 4:
						strcpy(message, "Норма предохр.");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					default:
						strcpy(message, "Неизв норма");
						NeedWrite = true;
						CheckDiscretsErrors();
						break;
					}
				}
			}
		}
		prevLogicFlags = RAM.LogicErrorFlags;
	}
}

void CheckDiscretsErrors() {
	uint16 diffMask = 0;
	char buffer[64];
	char conv_buf[8];
	for (int it = 0; it < 4; it++)
		if (prevErrDiscretLogic[it] != RAM.ErrorDiscretLogic[it]) {
			diffMask = prevErrDiscretLogic[it] ^ RAM.ErrorDiscretLogic[it];

			for (int i = 0; i < 11; i++) {
				if ((diffMask >> i) & 0x0001 == 0x0001) {
					if ((RAM.ErrorDiscretLogic[it] >> i) & 0x0001 == 0x0001) {
						strcpy(buffer, "Д:");
						itoa1(i + 1, conv_buf);
						strcat(buffer, conv_buf);
						strcat(buffer, "(0):");
						strcat(buffer, message);
						JrnlWrite(buffer);
						NeedWrite = false;

					} else {
						strcpy(buffer, "Д:");
						itoa1(i + 1, conv_buf);
						strcat(buffer, conv_buf);
						strcat(buffer, "(1):");
						strcat(buffer, message);
						JrnlWrite(buffer);
						NeedWrite = false;
					}

				}

			}

			prevErrDiscretLogic[it] = RAM.ErrorDiscretLogic[it];
		}
	if (NeedWrite == true) {
		JrnlWrite(message);
		NeedWrite = false;
	}
	strcpy(message, "");
}

char minbuf[20];
char buffer[256];


void CheckPowerOn() {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	portENTER_CRITICAL();
	{
		if (bPowerOn == false) {
			bPowerOn = true;
			if (RCC->CSR & RCC_CSR_IWDGRSTF) {
				JrnlWrite("Сброс от стор. таймера");
				itoa1(T1, minbuf);
				strcpy(buffer, "Код ошибки=");
				strcat(buffer, minbuf);
				JrnlWrite(buffer);
				RAM.LocalCommand[0] = BKP_ReadBackupRegister(BKP_DR1);
				RAM.LocalCommand[1] = BKP_ReadBackupRegister(BKP_DR2);
			} else {
				//Запись в журнал - выключения контроллера
				JrnlWriteOff("Контроллер Выкл");
				//Запись в журнал - включения контроллера
				JrnlWrite("Контроллер Вкл");

				//RAM.LocalCommand[0] = BKP_ReadBackupRegister(BKP_DR1);//сохранение состояния по выключению питания
				//RAM.LocalCommand[1] = BKP_ReadBackupRegister(BKP_DR2);

			}

			//Инициализирование начальных значений
			for (int i = 0; i < 8; i++) {
				if (GetChannelBitDirect(i, RAM.OutputCommand) == 1) {
					ChanelDirect[i] = true;
					ChanelChangeDirect[i] = true;
				} else {

					ChanelDirect[i] = false;
					ChanelChangeDirect[i] = false;
				}
				if (GetChannelBitRepare(i, RAM.OutputCommand) == 1) {
					ChanelRepare[i] = true;
					ChanelChangeRepare[i] = true;
				} else {
					ChanelRepare[i] = false;
					ChanelChangeRepare[i] = false;
				}
			}
			///
		}

	}
	portEXIT_CRITICAL();
}

//--------------------------------------------------------------------------
void DoProgram() {

	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	DATATIME dtl, dt;
	bool reverseDay;
	char tmp_buf[8];

	if (RAM.reset != 0) {
		if (RAM.reset == 1)
		{
			Reset();
			prevLogicFlags = 0;
			for (int i = 0; i < 4; i++)
				{
				prevErrDiscretLogic[i] = 0;
				}
			}
		else if (RAM.reset == 2)
			{
			JrnlClear();
			}
		RAM.reset = 0;
	}

	/* int jrnlLength = GetJrnlLength();
		    if (jrnlLength == 0)
			{

			}*/

	BKP_WriteBackupRegister(BKP_DR7, 51);
	CheckPowerOn();

	uint16 u1 = 0;
	int DelayForStopTime;

	if (pxConfig->devcfg.logica.StopTime == 0x01) {
		vTaskDelay(2000);
		portENTER_CRITICAL();
		{
			uint16 u1 = 0;
			for (int i = 0; i < 100000; i++) {
			}
			I2C_Time_BufferWrite(0xFF, 0x0E, 1);
			MemSetWords((uint16) (0x823E), &u1, 0x1);
		}
		portEXIT_CRITICAL();
	}
	BKP_WriteBackupRegister(BKP_DR7, 52);
	if (pxConfig->devcfg.logica.SwitchTime * 1000 < atcGetCounter()) {

		if (CountReset == 0) {
			bool Direct = false;

			for (int i = 0; i < 8; i++) {
				if (GetChannelBitDirect(i, RAM.LocalCommand)) {
					Direct = true;
					break;
				}
			}
			if (Direct == true) {
				JrnlWrite("Авт.режим всех КУ");

			}
		}
		for (int i = 0; i < 8; i++) {
			if (!block[i]) {
				SetChannelBitDirect(i, RAM.LocalCommand, 0); //OutputCommand
			}
		}
		for (int i = 0; i < 8; i++) {
			if (GetChannelBitDirect(i, RAM.LocalCommand) == 1) {
				ChanelChangeDirect[i] = true;
			} else {
				ChanelChangeDirect[i] = false;
			}
			ChanelDirect[i] = ChanelChangeDirect[i];
		}

	}
	BKP_WriteBackupRegister(BKP_DR7, 53);
	if (pxConfig->devcfg.logica.SwitchTime * 1000 < atcGetCounter()) {
		for (int i = 0; i < 8; i++) {
			if (!block[i]) {
				uint16 tmpGrNum = pxConfig->devcfg.logica.confCU[i].grafNum;
				if (tmpGrNum > 0 && tmpGrNum < 9) {
					SetChannelBitDirect(i, RAM.LocalCommand, 0);
				}
			}
		}
		atcResetCounter();
	}

	rtcGetDataTime(&dt);
	rtcGetDataTime(&dtl);
	BKP_WriteBackupRegister(BKP_DR7, 54);
	if (CheckDiscretMask(pxConfig->devcfg.logica.MaskControl)) {

		RAM.LogicErrorFlags &= ~(1 << 3);
		CheckErrors();

	} else {

		RAM.LogicErrorFlags |= (1 << 3);
		CheckErrors();

	}


	if (CheckDiscretMask(pxConfig->devcfg.logica.MaskPower)) {

		RAM.LogicErrorFlags &= ~(1);
		CheckErrors();

	} else {

		RAM.LogicErrorFlags |= (1);
		CheckErrors();

	}

	BKP_WriteBackupRegister(BKP_DR7, 55);
	for (int i = 0; i < 8; i++) {
		if (GetChannelBitRepare(i, RAM.CommonCommand) == 1) {
			if (GetChannelBitRepare(i, RAM.OutputCommand) == 1)
				continue;
			SetChannelBitSignal(i, RAM.LocalCommand,GetChannelBitSignal(i, RAM.CommonCommand));
			SetChannelBitDirect(i, RAM.LocalCommand,GetChannelBitDirect(i, RAM.CommonCommand));
			SetChannelBitReserv(i, RAM.LocalCommand,GetChannelBitReserv(i, RAM.CommonCommand));
			SetChannelBitRepare(i, RAM.CommonCommand, 0); //insurance to make global command only once.
		}
	}
	BKP_WriteBackupRegister(BKP_DR7, 56);
	for (int i = 0; i < 8; i++) {
		if (pxConfig->devcfg.logica.confCU[i].releNum == 0)
			continue;

		SetChannelBitRepare(i, RAM.OutputCommand,GetChannelBitRepare(i, RAM.LocalCommand));
		SetChannelBitReserv(i, RAM.OutputCommand,GetChannelBitReserv(i, RAM.LocalCommand));

		if (GetChannelBitDirect(i, RAM.LocalCommand) == 1) {
			SetChannelBitDirect(i, RAM.OutputCommand, 1);

			SetChannelBitSignal(i, RAM.OutputCommand,
					GetChannelBitSignal(i, RAM.LocalCommand));

		} else {
			SetChannelBitDirect(i, RAM.OutputCommand, 0);
			if (GetChannelBitRepare(i, RAM.LocalCommand) == 1) {
				SetChannelBitSignal(i, RAM.OutputCommand, 0);
			}

		}

	}
	BKP_WriteBackupRegister(BKP_DR7, 57);
	//grafs check
	for (int i = 0; i < 8; i++) {
		uint16 tmpGrNum = pxConfig->devcfg.logica.confCU[i].grafNum;

		if (GetChannelBitRepare(i, RAM.OutputCommand)) {

			continue;
		}

		if (GetChannelBitDirect(i, RAM.OutputCommand)) {
			continue;
		}

		if (tmpGrNum > 0 && tmpGrNum < 5) {
			tmpGrNum--;
			reverseDay = false;
			uint8 tmp = 0;
			uint8 startHour =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].StartHour;
			uint8 startMin = pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month
					- 1][dt.Data - 1].StartMin;
			uint8 finishHour =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].FinishHour;
			uint8 finishMin =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].FinishMin;
			if (startHour * 60 + startMin > finishHour * 60 + finishMin) {
				reverseDay = true;
				tmp = startHour;
				startHour = finishHour;
				finishHour = tmp;

				tmp = startMin;
				startMin = finishMin;
				finishMin = tmp;
			}
			//-------------- graph 1-3 without economy-------------------
			if(!((finishHour==startHour)&(finishMin==startMin)))
			{
			if ((startHour * 60 + startMin <= (dt.Hour * 60 + dt.Min))
					&& (finishHour * 60 + finishMin > (dt.Hour * 60 + dt.Min))) {
				if (!reverseDay)
					SetChannelBitSignal(i, RAM.OutputCommand, 0);
				else
					SetChannelBitSignal(i, RAM.OutputCommand, 1);
				continue;
			} else {
				if (!reverseDay)
					SetChannelBitSignal(i, RAM.OutputCommand, 1);
				else
					SetChannelBitSignal(i, RAM.OutputCommand, 0);
				continue;
			}
			}
		}
		BKP_WriteBackupRegister(BKP_DR7, 58);
		if (tmpGrNum == 5) {
			tmpGrNum--;

			uint8 startMonth = pxConfig->devconst.Schedule4.StartMonth;
			uint8 startDate = pxConfig->devconst.Schedule4.StartDay;
			uint8 finishMonth = pxConfig->devconst.Schedule4.FinishMonth;
			uint8 finishDate = pxConfig->devconst.Schedule4.FinishDay;

			uint8 FnMonth =
					(finishMonth > startMonth) ?
							finishMonth : (finishMonth + 12);
			uint8 CurMonth =
					(dt.Month * 31 + dt.Data >= startMonth * 31 + startDate) ?
							dt.Month : (dt.Month + 12);
			//-------------- graph 4 heating-------------------
			if ((startMonth * 31 + startDate <= CurMonth * 31 + dt.Data)
					&& (FnMonth * 31 + finishDate > CurMonth * 31 + dt.Data)) {
				SetChannelBitSignal(i, RAM.OutputCommand, 1);
				continue;
			} else {
				SetChannelBitSignal(i, RAM.OutputCommand, 0);
				continue;
			}
		}
		if (tmpGrNum < 9 && tmpGrNum > 5) {
			tmpGrNum -= 6;
			uint8 startHour =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].StartHour;
			uint8 startMin = pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month
					- 1][dt.Data - 1].StartMin;
			uint8 finishHour =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].FinishHour;
			uint8 finishMin =
					pxConfig->devconst.Graph[tmpGrNum].Shedule[dt.Month - 1][dt.Data
							- 1].FinishMin;
			//-------------- graphs 1-3 with economy-------------------
			if(!((finishHour==startHour)&(finishMin==startMin)))
            {
			if (startHour * 60 + startMin <= dt.Hour * 60 + dt.Min
					&& finishHour * 60 + finishMin > dt.Hour * 60 + dt.Min) {
				SetChannelBitSignal(i, RAM.OutputCommand, 0);
				continue;
			} else {
				//date economy
				if ((pxConfig->devconst.Graph[tmpGrNum].EconomyDate.StartMonth
						* 31
						+ pxConfig->devconst.Graph[tmpGrNum].EconomyDate.StartDay
						<= dtl.Month * 31 + dtl.Data)
						&& (pxConfig->devconst.Graph[tmpGrNum].EconomyDate.FinishMonth
								* 31
								+ pxConfig->devconst.Graph[tmpGrNum].EconomyDate.FinishDay
								>= dtl.Month * 31 + dtl.Data)) {
					//time economy
					uint8 startHourEk =
							pxConfig->devconst.Graph[tmpGrNum].Shedule[dtl.Month
									- 1][31].StartHour;
					uint8 startMinEk =
							pxConfig->devconst.Graph[tmpGrNum].Shedule[dtl.Month
									- 1][31].StartMin;
					uint8 finishHourEk =
							pxConfig->devconst.Graph[tmpGrNum].Shedule[dtl.Month
									- 1][31].FinishHour;
					uint8 finishMinEk =
							pxConfig->devconst.Graph[tmpGrNum].Shedule[dtl.Month
									- 1][31].FinishMin;
					uint8 FnEKHour =
							(finishHourEk > startHourEk) ?
									finishHourEk : (finishHourEk + 24);
					uint8 CurHour =
							(dtl.Hour * 60 + dtl.Min
									>= startHourEk * 60 + startMinEk) ?
									dtl.Hour : (dtl.Hour + 24);

					if ((startHourEk * 60 + startMinEk <= CurHour * 60 + dtl.Min)
							&& (FnEKHour * 60 + finishMinEk
									> CurHour * 60 + dtl.Min)) {
						SetChannelBitSignal(i, RAM.OutputCommand, 1);
						continue;
					} else {
						SetChannelBitSignal(i, RAM.OutputCommand, 0);
						continue;
					}
				} else {
					SetChannelBitSignal(i, RAM.OutputCommand, 1);
					continue;
				}
			}
			/////
			}
		}

	}
	BKP_WriteBackupRegister(BKP_DR7, 59);
	//error and condition module for iFIX
	for (int i = 0; i < 4; i++) {
		RAM.ErrorAndConditionModule[i * 2] = RAM.ErrorDiscretLogic[i];
		RAM.ErrorAndConditionModule[i * 2 + 1] = RAM.diskrets[i];
	}

	//Сброс устройства
	CheckCUSignal();

	// send signals to rele
	uint8 tempReleMaskCounter = 0;
	BKP_WriteBackupRegister(BKP_DR7, 60);
	for (int i = 0; i < 8; i++) {
		if (pxConfig->devcfg.logica.confCU[i].releNum == 0)
			continue;
		uint8 TempReleInd = pxConfig->devcfg.logica.confCU[i].releNum - 1;
		SetRelayToRam(TempReleInd, GetChannelBitSignal(i, RAM.OutputCommand));

		tempReleMaskCounter |= (1 << TempReleInd);
	}
	//switch off unused relays
	for (int i = 0; i < 8; i++) {
		if ((tempReleMaskCounter & (1 << i)) == 0) {
			SetRelayToRam(i, 0);
		}
	}

	vTaskDelay(20);
	BKP_WriteBackupRegister(BKP_DR7, 61);
	//Get Channel Condition CU
	for (int i = 0; i < 8; i++) {

		if (pxConfig->devcfg.logica.confCU[i].discNum == 0)
			continue;
		if (GetDiskretFromRam(pxConfig->devcfg.logica.confCU[i].discNum - 1)
				== 0) {
			RAM.ChannelCondition &= (uint8) (~(1 << i));
		} else {
			RAM.ChannelCondition |= (uint8) (1 << i);
		}
	}
	BKP_WriteBackupRegister(BKP_DR7, 62);
	//Check Channel
	for (int i = 0; i < 8; i++) {

		if (pxConfig->devcfg.logica.confCU[i].discNum == 0) {
			RAM.ErrorChannel &= ~(1 << i);
			continue;
		}
		if (((RAM.ChannelCondition) & (1 << i)) >> i
				!= GetChannelBitSignal(i, RAM.OutputCommand)) {
			RAM.ErrorChannel |= (1 << i);
		} else {
			RAM.ErrorChannel &= ~(1 << i);
		}
	}

	if (RAM.ErrorChannel != 0) {
		RAM.LogicErrorFlags |= (1 << 1);
		CheckErrors();

	} else {
		RAM.LogicErrorFlags &= ~(1 << 1);
		CheckErrors();

	}


	BKP_WriteBackupRegister(BKP_DR7, 63);
	//Check Fuse on Channels
	for (int i = 0; i < 8; i++) {
		if (pxConfig->devcfg.logica.confCU[i].releNum == 0)
			continue;
		if (pxConfig->devcfg.logica.confCU[i].discNum == 0)
			continue;
		if (GetDiskretFromRam(pxConfig->devcfg.logica.confCU[i].discNum - 1)) {
			if (CheckDiscretMask(pxConfig->devcfg.logica.confCU[i].Mask)) {
				RAM.ErrorFuseChannel &= ~(1 << i);
			} else {
				RAM.ErrorFuseChannel |= (1 << i);
			}
		} else {
			if (CheckDiscretMaskNegative(
					pxConfig->devcfg.logica.confCU[i].Mask)) {
				RAM.ErrorFuseChannel &= ~(1 << i);
			} else {
				RAM.ErrorFuseChannel |= (1 << i);
			}
		}
	}
	BKP_WriteBackupRegister(BKP_DR7, 64);
	if (RAM.ErrorFuseChannel != 0) {
		RAM.LogicErrorFlags |= (1 << 4);
		CheckErrors();

	} else {
		RAM.LogicErrorFlags &= ~(1 << 4);
		CheckErrors();

	}


	//check Security
	if (CheckDiscretMask(pxConfig->devcfg.logica.MaskSecurity)) {
		RAM.LogicErrorFlags &= ~(1 << 2);
		CheckErrors();

	} else {
		RAM.LogicErrorFlags |= (1 << 2);
		CheckErrors();

	}


	BKP_WriteBackupRegister(BKP_DR7, 65);
	if ((pxConfig->devcfg.logica.SwitchTime) == 0xFFFF) {
		uint16 u1 = 0;
		for (int i = 0; i < 64; i++) {
			MemSetWords((uint16) (0x8200 + i), &u1, 0x1);
			vTaskDelay(1);
		}

	}

	if (pxConfig->devcfg.logica.SwitchTime > 1800
			|| pxConfig->devcfg.logica.SwitchTime == 0) {
		uint16 u1 = 300;
		portENTER_CRITICAL();
		{
			MemSetWords((uint16) (0x823C), &u1, 0x1);
		}
		portEXIT_CRITICAL();
	}

	SetCRC(&RAM.LogicErrorFlags, 18);

	BKP_WriteBackupRegister(BKP_DR7, 66);
	//Блок записи в журнал о ручном режиме, ремонтном режиме каналов
	for (int i = 0; i < 8; i++) {
		if (GetChannelBitDirect(i, RAM.OutputCommand) == 1) {
			ChanelChangeDirect[i] = true;
		} else {
			ChanelChangeDirect[i] = false;
		}
		if (GetChannelBitRepare(i, RAM.OutputCommand) == 1) {
			ChanelChangeRepare[i] = true;
		} else {
			ChanelChangeRepare[i] = false;
		}
	}
	char minbuf[20];
	char buffer[256];
	for (int i = 0; i < 8; i++) {
		if ((ChanelChangeDirect[i] == true) && (ChanelDirect[i] == false)) {
			itoa1(i + 1, minbuf);
			manualwork=1;
			strcpy(buffer, "Ручной режим.КУ:");
			strcat(buffer, minbuf);
			JrnlWrite(buffer);
			ChanelDirect[i] = ChanelChangeDirect[i];

		}
		if ((ChanelChangeDirect[i] == false) && (ChanelDirect[i] == true)) {
			itoa1(i + 1, minbuf);
			manualwork=0;
			strcpy(buffer, "Авто режим.КУ:");
			strcat(buffer, minbuf);
			JrnlWrite(buffer);
			ChanelDirect[i] = ChanelChangeDirect[i];

		}
		if ((ChanelChangeRepare[i] == true) && (ChanelRepare[i] == false)) {
			itoa1(i + 1, minbuf);
			strcpy(buffer, "Вкл. Рем. режим.КУ:");
			strcat(buffer, minbuf);
			JrnlWrite(buffer);
			ChanelRepare[i] = ChanelChangeRepare[i];

		}

		if ((ChanelChangeRepare[i] == false) && (ChanelRepare[i] == true)) {
			itoa1(i + 1, minbuf);
			strcpy(buffer, "Откл. Рем. режим.КУ:");
			strcat(buffer, minbuf);
			JrnlWrite(buffer);
			ChanelRepare[i] = ChanelChangeRepare[i];

		}

		BKP_WriteBackupRegister(BKP_DR1, RAM.LocalCommand[0]);
		BKP_WriteBackupRegister(BKP_DR2, RAM.LocalCommand[1]);
	}

}
