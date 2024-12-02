#include "rtclock.h"
#include "stm32f10x_i2c.h"
#include "board.h"
#include "usermemory.h"
#include "led.h"
#include "journal.h"

#define RETRY_NUMBER 3000

DATATIME RTClock[2];
bool iFixLocal;

//bool ResetTime = 0;
//bool JrnlTime = 0;
//bool ResetTimeEnd = 0;
FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;

unsigned int RTCCorrector = 0;

uint32 atcTickCounter;
uint32 atcCounterHZ;
bool switcher = true;
bool FlagGSMtime = 0;
bool FlagGPStime = 0;

void rtcUpdateLocalDT();

bool I2C_Time_BufferWrite(u8 *pBuffer, u16 WriteAddr, u16 NumByteToWrite);
bool I2C_Time_BufferRead(u8 *pBuffer, u16 ReadAddr, u16 NumByteToRead);

//----------------------------------

unsigned char month_day_table[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30,
		31 };

uint16 month_day_tableAcc[] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304,
		334, 375 };

unsigned char leap_year(unsigned int test_year);
//?????????? 1, ???? ??? test_year - ??????????
unsigned char leap_year(unsigned int test_year) {
	if (((test_year % 4 == 0) && (test_year % 100)) || (test_year % 400 == 0))
		return 1;
	else
		return 0;
}
unsigned int at91_bcd_int(unsigned char bcd) {
	return ((bcd & 0x0F) + (((bcd & 0xF0) >> 4) * 10));

}

unsigned int GetDayOfWeek(DATATIME *dt) {
	unsigned char Mounths[] = { 5, 1, 1, 4, 6, 2, 4, 0, 3, 5, 1, 3 };

	int DayOfWeek = (dt->Years + (int) (dt->Years / 4) + Mounths[dt->Month - 1]
			+ dt->Data);

	if (leap_year(dt->Years) == 1 && dt->Month < 2)
		DayOfWeek = (DayOfWeek - 1) % 7;
	else
		DayOfWeek = (DayOfWeek) % 7;
	if (DayOfWeek == 0)
		DayOfWeek = 7;
	return DayOfWeek;

}
unsigned char at91_int_bcd(unsigned int value) {
	char tmp[2];
	tmp[1] = (value % 10) + 0;
	value /= 10;
	tmp[0] = (value % 10) + 0;
	return ((tmp[1] & 0x0F) | ((tmp[0] & 0x0F) << 4));
}

u8 data[7];
u8 dataoff[7];

void UpdateTime(void) {

	if (ResetTime) {
		BKP_WriteBackupRegister(BKP_DR7, 4);
		if (!I2C_Time_BufferRead(data, 0, 7))
			return;
		RTClock[0].Sec = at91_bcd_int(data[0]);
		RTClock[0].Min = at91_bcd_int(data[1]);
		RTClock[0].Hour = at91_bcd_int(data[2]);
		RTClock[0].Day = at91_bcd_int(data[3]);
		RTClock[0].Data = at91_bcd_int(data[4]);
		RTClock[0].Month = at91_bcd_int(data[5]);
		RTClock[0].Years = (at91_bcd_int(data[6]) + 2000);

		if ((RTClock[0].Hour == 12) && (pxConfig->devcfg.GSMtime == 1)
				&& (!FlagGSMtime) && (LedConnect)) {
			GSM_clock();
			//FlagGSMtime = 1;
		}
		/*if ((RTClock[0].Hour == 12)&&(pxConfig->devcfg.GSMtime == 2)&&(!FlagGPStime)&&(LedGPS))
		 {
		 //GPS_clock();
		 //FlagGPStime = 1;
		 }*/
		if (((RTClock[0].Hour == 23) && (RTClock[0].Min == 59)
				&& (RTClock[0].Sec == 59) && ((FlagGSMtime) || (FlagGPStime)))
				|| (pxConfig->devcfg.GSMtime == 0)) {
			FlagGSMtime = 0;
			FlagGPStime = 0;

		}
		uint8_t SetDataTime[6] = { at91_int_bcd(RTClock[0].Sec), at91_int_bcd(
				RTClock[0].Min), at91_int_bcd(RTClock[0].Hour), at91_int_bcd(
				RTClock[0].Day), at91_int_bcd(RTClock[0].Data), at91_int_bcd(
				RTClock[0].Month) };
		I2C_Time_BufferWrite(SetDataTime, 7, 6);

		ResetTimeEnd = 0;
	}

}

bool rtcInit(void) {

	BKP_WriteBackupRegister(BKP_DR7, 18);
	uint8_t buffWr;
	u16 u16_WaitForOscSource;
	int counter = 10;
	buffWr = 156;
	while (!I2C_Time_BufferWrite(buffWr, 14, 1)) {
		if (counter-- <= 0)
			return false;
	}
	vTaskDelay(5);

	//for(u16_WaitForOscSource=0;u16_WaitForOscSource<5000;u16_WaitForOscSource++);
	counter = 10;
	buffWr = 184;
	while (!I2C_Time_BufferWrite(buffWr, 15, 1)) {
		if (counter-- <= 0)
			return false;
	}
	vTaskDelay(5);

	//for(u16_WaitForOscSource=0;u16_WaitForOscSource<5000;u16_WaitForOscSource++);
	counter = 10;
	while (!I2C_Time_BufferRead(data, 0, 7)) {
		if (counter-- <= 0)
			return false;
	}


	BKP_WriteBackupRegister(BKP_DR7, 19);
	RTClock[0].Sec = at91_bcd_int(data[0]);
	RTClock[0].Min = at91_bcd_int(data[1]);
	RTClock[0].Hour = at91_bcd_int(data[2]);
	RTClock[0].Day = at91_bcd_int(data[3]);
	RTClock[0].Data = at91_bcd_int(data[4]);
	RTClock[0].Month = at91_bcd_int(data[5]);
	RTClock[0].Years = (at91_bcd_int(data[6]) + 2000);
	vTaskDelay(20);
	ResetTime = 1;
	if (RTClock[0].Years == 2000) {
		RTClock[0].Sec = 255;
		RTClock[0].Min = 255;
		RTClock[0].Hour = 255;
		RTClock[0].Day = 255;
		RTClock[0].Data = 255;
		RTClock[0].Month = 255;
		RTClock[0].Years = 255;
		ResetTime = 0;
	}

	BKP_WriteBackupRegister(BKP_DR7, 20);
	if (!I2C_Time_BufferRead(dataoff, 7, 6))
		return false;
	return true;
}

void rtcGetDataTimeOff(DATATIME *dt) {
	dt->MSec = 0;
	dt->Sec = at91_bcd_int(dataoff[0]);
	dt->Min = at91_bcd_int(dataoff[1]);
	dt->Hour = at91_bcd_int(dataoff[2]);
	dt->Day = at91_bcd_int(dataoff[3]);
	dt->Data = at91_bcd_int(dataoff[4]);
	dt->Month = at91_bcd_int(dataoff[5]);
	dt->Years = RTClock[0].Years;

}
void rtcGetDataTime(DATATIME *dt) {
	dt->MSec = RTClock[0].MSec;
	dt->Sec = RTClock[0].Sec;
	dt->Min = RTClock[0].Min;
	dt->Hour = RTClock[0].Hour;
	dt->Day = RTClock[0].Day;
	dt->Data = RTClock[0].Data;
	dt->Month = RTClock[0].Month;
	dt->Years = RTClock[0].Years;

}

void rtcGetLocalDataTime(DATATIME *dt) {
	rtcUpdateLocalDT();
	dt->MSec = RTClock[1].MSec;
	dt->Sec = RTClock[1].Sec;
	dt->Min = RTClock[1].Min;
	dt->Hour = RTClock[1].Hour;
	dt->Day = RTClock[1].Day;
	dt->Data = RTClock[1].Data;
	dt->Month = RTClock[1].Month;
	dt->Years = RTClock[1].Years;
}

void rtcUpdateLocalDT() {
	int shift = 0; //our shift amount
	if (RTClock[0].Month > 3 && RTClock[0].Month < 10) {
		shift = 1;
	}
	if (RTClock[0].Month == 3 && RTClock[0].Data > 24 && RTClock[0].Day == 7
			&& RTClock[0].Hour > 1) {
		shift = 1;
	}

	if (RTClock[0].Month == 3 && RTClock[0].Data > 24 && RTClock[0].Day != 7
			&& (7 - RTClock[0].Day) > (31 - RTClock[0].Data)) {
		shift = 1;
	}

	if (RTClock[0].Month == 10
			&& ((RTClock[0].Data <= 24)
					|| (RTClock[0].Data > 24 && RTClock[0].Day != 7
							&& (7 - RTClock[0].Day) <= (31 - RTClock[0].Data)))) {
		shift = 1;
	}

	if (RTClock[0].Month == 10 && RTClock[0].Data > 24 && RTClock[0].Day == 7
			&& RTClock[0].Hour < 1) {
		shift = 1;
	}

	RTClock[1].MSec = RTClock[0].MSec;
	RTClock[1].Sec = RTClock[0].Sec;
	RTClock[1].Min = RTClock[0].Min;
	RTClock[1].Hour = RTClock[0].Hour + shift;
	if (RTClock[1].Hour > 23) {
		RTClock[1].Hour = 0;
	} else {
		shift = 0;
	}

	RTClock[1].Day = RTClock[0].Day + shift;
	if (RTClock[1].Day > 7) {
		RTClock[1].Day = 1;
	}
	RTClock[1].Data = RTClock[0].Data + shift;
	if (shift && RTClock[1].Data > month_day_table[RTClock[0].Month - 1]) {
		RTClock[1].Data = 1;
	} else {
		shift = 0;
	}
	RTClock[1].Month = RTClock[0].Month + shift;
	RTClock[1].Years = RTClock[0].Years;

}

void rtcSetDataTime(DATATIME *dt) {
	uint8_t count = 0;
	DATATIME tempClk;

	if ((dt->Years > 2100) | (dt->Years < 2010) | (dt->Month < 1)
			| (dt->Month > 12) | (dt->Data < 1) | (dt->Data > 31)
			| (dt->Hour < 0) | (dt->Hour > 23) | (dt->Min < 0) | (dt->Min > 59)
			| (dt->Sec < 0) | (dt->Sec > 59))
		return;

	rtcGetDataTime(&tempClk);

	if (((abs(tempClk.Min - (dt->Min)) > 10) || (tempClk.Hour != dt->Hour)
			|| (tempClk.Data != dt->Data) || (tempClk.Month != dt->Month)
			|| (tempClk.Years != dt->Years))) {
		JrnlTime = 1;
		ResetTimeEnd = 1;
	}

	uint8_t SetDataTime[7] = { at91_int_bcd(dt->Sec), at91_int_bcd(dt->Min),
			at91_int_bcd(dt->Hour), at91_int_bcd(dt->Day), at91_int_bcd(
					dt->Data), at91_int_bcd(dt->Month), at91_int_bcd(
					dt->Years - 2000) };
	while (!I2C_Time_BufferWrite(SetDataTime, 0, 7)) {
		count++;
		if (count > 10)
			break;
	}
	ResetTime = 1;

}

void rtcSetLocalDataTime(DATATIME *dt) {
	dt->Day = GetDayOfWeek(dt);
	int shift = 0; //our shift amount

	if (dt->Month > 3 && dt->Month < 10) {
		shift = 1;
	}
	if (dt->Month == 3 && dt->Data > 24 && dt->Day == 7 && dt->Hour > 1) {
		shift = 1;
	}

	if (dt->Month == 3 && dt->Data > 24 && dt->Day != 7
			&& (7 - dt->Day) > (31 - dt->Data)) {
		shift = 1;
	}

	if (dt->Month == 10
			&& ((dt->Data <= 24)
					|| (dt->Data > 24 && dt->Day != 7
							&& (7 - dt->Day) <= (31 - dt->Data)))) {
		shift = 1;
	}

	if (dt->Month == 10 && dt->Data > 24 && dt->Day == 7 && dt->Hour < 1) {
		shift = 1;
	}

	RTClock[0].MSec = dt->MSec;
	RTClock[0].Sec = dt->Sec;
	RTClock[0].Min = dt->Min;

	if (shift == 1) {
		if (dt->Hour == 0) {
			RTClock[0].Hour = 23;
		} else {
			RTClock[0].Hour = dt->Hour - shift;
			shift = 0;
		}
	} else {
		RTClock[0].Hour = dt->Hour;
	}

	if (shift == 1) {
		if (dt->Day == 1) {
			RTClock[0].Day = 7;
		} else {
			RTClock[0].Day = dt->Day - shift;
		}
	} else {
		RTClock[0].Day = dt->Day;
	}

	if (shift == 1) {
		if (dt->Data == 1) {
			RTClock[0].Data = month_day_table[dt->Month - 2];
		} else {
			RTClock[0].Data = dt->Data - shift;
			shift = 0;
		}
	} else {
		RTClock[0].Data = dt->Data;
	}

	RTClock[0].Month = dt->Month - shift;
	RTClock[0].Years = dt->Years;
}

//-----------------------------------------------------------

void itoaFix(int n, char s[]) {
	n = n % 100;

	s[0] = n / 10 + 48;
	s[1] = n % 10 + 48;
}

void rtcGetiFixDateTime(char *dt) {
	char buffer[2];
	DATATIME dtC[2];

	dt[0] = 48;

	dt[1] = 49;

	rtcGetDataTime(dtC);

	itoaFix(dtC->Years, buffer);
	dt[2] = buffer[0];
	dt[3] = buffer[1];
	itoaFix(dtC->Month, buffer);
	dt[4] = buffer[0];
	dt[5] = buffer[1];
	itoaFix(dtC->Data, buffer);
	dt[6] = buffer[0];
	dt[7] = buffer[1];
	itoaFix(dtC->Hour, buffer);
	dt[8] = buffer[0];
	dt[9] = buffer[1];
	itoaFix(dtC->Min, buffer);
	dt[10] = buffer[0];
	dt[11] = buffer[1];
	itoaFix(dtC->Sec, buffer);
	dt[12] = buffer[0];
	dt[13] = buffer[1];
	itoaFix(dtC->MSec, buffer);
	dt[14] = buffer[0];
	dt[15] = buffer[1];
}

void rtcSetiFixDateTime(char *dt) {
	char buffer[3];
	buffer[2] = 0;
	DATATIME dtC;

	if (dt[1] == 49)
		iFixLocal = true;
	else
		iFixLocal = false;

	buffer[0] = dt[2];
	buffer[1] = dt[3];
	dtC.Years = atoi(buffer);

	buffer[0] = dt[4];
	buffer[1] = dt[5];
	dtC.Month = atoi(buffer);

	buffer[0] = dt[6];
	buffer[1] = dt[7];
	dtC.Data = atoi(buffer);

	buffer[0] = dt[8];
	buffer[1] = dt[9];
	dtC.Hour = atoi(buffer);

	buffer[0] = dt[10];
	buffer[1] = dt[11];
	dtC.Min = atoi(buffer);

	buffer[0] = dt[12];
	buffer[1] = dt[13];
	dtC.Sec = atoi(buffer);

	buffer[0] = dt[14];
	buffer[1] = dt[15];
	dtC.MSec = atoi(buffer);

	if (dtC.Years > 70)
		dtC.Years += 1900;
	else
		dtC.Years += 2000;

	if (iFixLocal)
		rtcSetLocalDataTime(&dtC);
	else
		rtcSetDataTime(&dtC);
}

//-------------------------------------------------------------
void atcRebaseCounter() {
	atcCounterHZ = atcTickCounter;
}

void GetDateStringLM(char *aBuf) {

	DATATIME dtl;
	char tempBuf[16];
	char overalBuff[32];
	rtcGetDataTime(&dtl);
	overalBuff[0] = '\0';
	int month = dtl.Month;
	int year = dtl.Years;
	if (year > 2000)
		year -= 2000;
	if (month == 1) {
		month = 12;
		if (year != 0)
			year--;
	} else {
		month--;
	}

	if (month < 10)
		strcat((char*) overalBuff, "0");
	itoa(month, tempBuf);
	strcat(strcat((char*) overalBuff, (char*) tempBuf), ".");
	if (year < 10)
		strcat((char*) overalBuff, "0");
	itoa(year, tempBuf);
	strcat(overalBuff, tempBuf);

	CopyDataBytes((uint8*) overalBuff, (uint8*) aBuf, strlen(overalBuff) + 1);
}

void GetDateStringLD(char *aBuf) {
	DATATIME dtl;
	char tempBuf[16];
	char overalBuff[32];
	rtcGetDataTime(&dtl);
	overalBuff[0] = '\0';
	int month = dtl.Month;
	int year = dtl.Years;
	int data = dtl.Data;
	if (year > 2000)
		year -= 2000;

	if (data == 1) {
		data = month_day_table[(month + 10) % 12]
				+ ((month == 3) ? ((uint8) leap_year(year)) : (0));

		if (month == 1) {
			month = 12;
			if (year != 0)
				year--;
		} else {
			month--;
		}
	} else {
		data--;
	}

	if (data < 10)
		strcat((char*) overalBuff, "0");
	itoa(data, tempBuf);
	strcat(strcat((char*) overalBuff, (char*) tempBuf), ".");
	if (month < 10)
		strcat((char*) overalBuff, "0");
	itoa(month, tempBuf);
	strcat(strcat((char*) overalBuff, (char*) tempBuf), ".");
	if (year < 10)
		strcat((char*) overalBuff, "0");
	itoa(year, tempBuf);
	strcat((char*) overalBuff, (char*) tempBuf);

	CopyDataBytes((uint8*) overalBuff, (uint8*) aBuf, strlen(overalBuff) + 1);

}

/////////////////////Р§С‚РµРЅРёРµ С‡Р°СЃРѕРІ РїРѕ I2C///////////////////////////////////////////////////////////////////////////////
bool I2C_Time_BufferRead(u8 *pBuffer, u16 ReadAddr, u16 NumByteToRead) {
	if (initMK == 1) {
		int counter = RETRY_NUMBER;
		vu16 SR_Tmp;
		/* While the bus is busy */
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 5);
		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 6);
		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		BKP_WriteBackupRegister(BKP_DR7, 7);
		/*while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}*/
		  while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR))
		  {
		  	  if(counter-- <= 0) return I2C_EE_Relaunch();
		    }

		  /* Read I2C1 SR1 register*/
		  SR_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);



		/* Clear EV6 by setting again the PE bit */
		I2C_Cmd(I2C1, ENABLE);
		BKP_WriteBackupRegister(BKP_DR7, 8);
		I2C_SendData(I2C1, ReadAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 9);
		/* Send STRAT condition a second time */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 9);
		/* Send EEPROM address for read */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Receiver);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 10);
		/* While there is data to be read */
		counter = RETRY_NUMBER;
		while (NumByteToRead) {
			if (NumByteToRead == 1) {
				/* Disable Acknowledgement */
				I2C_AcknowledgeConfig(I2C1, DISABLE);

				/* Send STOP Condition */
				I2C_GenerateSTOP(I2C1, ENABLE);
			}
			BKP_WriteBackupRegister(BKP_DR7, 11);
			/* Test on EV7 and clear it */
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				/* Read a byte from the EEPROM */
				*pBuffer = I2C_ReceiveData(I2C1);

				/* Point to the next location where the byte read will be saved */
				pBuffer++;

				/* Decrement the read bytes counter */
				NumByteToRead--;

			}
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 12);
		/* Enable Acknowledgement to be ready for another reception */
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		return true;

	} else {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 5);
		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 6);
		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		BKP_WriteBackupRegister(BKP_DR7, 7);
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}

		/* Clear EV6 by setting again the PE bit */
		I2C_Cmd(I2C1, ENABLE);
		BKP_WriteBackupRegister(BKP_DR7, 8);
		I2C_SendData(I2C1, ReadAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 9);
		/* Send STRAT condition a second time */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 9);
		/* Send EEPROM address for read */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Receiver);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 10);
		/* While there is data to be read */
		counter = RETRY_NUMBER;
		while (NumByteToRead) {
			if (NumByteToRead == 1) {
				/* Disable Acknowledgement */
				I2C_AcknowledgeConfig(I2C1, DISABLE);

				/* Send STOP Condition */
				I2C_GenerateSTOP(I2C1, ENABLE);
			}
			BKP_WriteBackupRegister(BKP_DR7, 11);
			/* Test on EV7 and clear it */
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				/* Read a byte from the EEPROM */
				*pBuffer = I2C_ReceiveData(I2C1);

				/* Point to the next location where the byte read will be saved */
				pBuffer++;

				/* Decrement the read bytes counter */
				NumByteToRead--;

			}
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 12);
		/* Enable Acknowledgement to be ready for another reception */
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		return true;
	}
}

bool I2C_Time_BufferWrite(u8 *pBuffer, u16 WriteAddr, u16 NumByteToWrite) {
	if (initMK == 1) {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */
		/*while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}*/
		BKP_WriteBackupRegister(BKP_DR7, 13);
		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 14);
		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		/*while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}*/
		  while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR))
		  {
		  	  if(counter-- <= 0) return I2C_EE_Relaunch();
		    }

		BKP_WriteBackupRegister(BKP_DR7, 15);
		/* Send the EEPROM's internal address to write to */
		//  I2C_SendData(I2C1, (WriteAddr>>8) & 0xFF);
		//  counter = RETRY_NUMBER;
		//  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		//  {
		//	if(counter-- <= 0) return I2C_EE_Relaunch();
		//  }
		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 16);
		/* While there is data to be written */
		while (NumByteToWrite--) {
			/* Send the current byte */
			I2C_SendData(I2C1, *pBuffer);

			/* Point to the next byte to be written */
			pBuffer++;

			/* Test on EV8 and clear it */
			counter = RETRY_NUMBER;
			while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
				if (counter-- <= 0)
					return I2C_EE_Relaunch();
			}
		}
		BKP_WriteBackupRegister(BKP_DR7, 17);
		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;

	} else {
		int counter = RETRY_NUMBER;
		/* While the bus is busy */
	/*	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}*/
		BKP_WriteBackupRegister(BKP_DR7, 13);
		/* Send START condition */
		I2C_GenerateSTART(I2C1, ENABLE);

		/* Test on EV5 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 14);
		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C1, 0xD0, I2C_Direction_Transmitter);

		/* Test on EV6 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 15);
		/* Send the EEPROM's internal address to write to */
		//  I2C_SendData(I2C1, (WriteAddr>>8) & 0xFF);
		//  counter = RETRY_NUMBER;
		//  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		//  {
		//	if(counter-- <= 0) return I2C_EE_Relaunch();
		//  }
		I2C_SendData(I2C1, WriteAddr & 0xFF);

		/* Test on EV8 and clear it */
		counter = RETRY_NUMBER;
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (counter-- <= 0)
				return I2C_EE_Relaunch();
		}
		BKP_WriteBackupRegister(BKP_DR7, 16);
		/* While there is data to be written */
		while (NumByteToWrite--) {
			/* Send the current byte */
			I2C_SendData(I2C1, *pBuffer);

			/* Point to the next byte to be written */
			pBuffer++;

			/* Test on EV8 and clear it */
			counter = RETRY_NUMBER;
			while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
				if (counter-- <= 0)
					return I2C_EE_Relaunch();
			}
		}
		BKP_WriteBackupRegister(BKP_DR7, 17);
		/* Send STOP condition */
		I2C_GenerateSTOP(I2C1, ENABLE);
		return true;
	}
}

void atcIncrementCounter() {
	atcTickCounter++;
}
extern CountReset;
void atcResetCounter() {
	CountReset++;
	atcTickCounter = 0;
}
u32 atcGetCounter() {
	return atcTickCounter;
}
