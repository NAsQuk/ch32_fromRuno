#include "usermemory.h"

#include "rtclock.h"
#include "journal.h"
#include "stm32f10x_flash.h"
#include "i2c_ee.h"
#include "mios32_iic.h"
#include "logica.h"

#define __at(x)

#define EEPROM_A1A0   0
#define EEPROM_DADR   (0x50|EEPROM_A1A0)
#define MIOS32_IIC_BS_ADDR_BASE 0xA0

extern size_t strlen(const char *str);
RAMMEM RAM;
int tmpPointer = 0;
int tmpLen = 0;
bool EEPROM_working = false;
uint16 bigFlashBuff[0x400];
extern uint16 CountReset;

//FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
int DeviceID;

bool GetRam(uint16, uint16*, uint16);
bool SetRam(uint16, uint16*, uint16);
bool GetEeprom(uint16, uint16*, uint16);
bool SetEeprom(uint16, uint16*, uint16);
bool GetRtc(uint16, uint16*, uint16);
bool SetRtc(uint16, uint16*, uint16);
bool GetFlash(uint16, uint16*, uint16);
bool SetFlash(uint16, uint16*, uint16);
bool Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength);
bool GetSignature(uint16, uint16*, uint16);

//==============================================================================
void MemInit() {

	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	//EEPROM INIT
	//I2C_EE_Init();
	I2C_Configuration();
	//MIOS32_IIC_Init(0);
	//Flash_Init();
	FLASH_Unlock();
	FLASH_SetLatency(FLASH_Latency_1);

	int i;
	MemManInit();
	for (i = 0; i < (DEVICE_RAMMEM_WORDS); i++) {
		RAM.dwords[i] = 0;
	}

	memunit[0].startaddr = 0x0000;
	memunit[0].endaddr = DEVICE_RAMMEM_WORDS;
	memunit[0].GetMem = GetRam;
	memunit[0].SetMem = SetRam;

#if(DEVICE_RAMMEM_WORDS!=0x400)
#error Check useer memory address table !!!
#endif

	memunit[4].startaddr = 0x0400;
	memunit[4].endaddr = 0x480;
	memunit[4].GetMem = GetSignature;
	//	memunit[1].SetMem = SetRam;

	memunit[1].startaddr = 0x1000;
	memunit[1].endaddr = 0x1031;
	memunit[1].GetMem = GetRtc;
	memunit[1].SetMem = SetRtc;

	memunit[2].startaddr = 0x2000;
	memunit[2].endaddr = 0x3677;
	memunit[2].GetMem = GetEeprom;
	memunit[2].SetMem = SetEeprom;

	memunit[3].startaddr = 0x8000;
	memunit[3].endaddr = 0x10000;
	memunit[3].GetMem = GetFlash;
	memunit[3].SetMem = SetFlash;

}

//==============================================================================
bool GetRam(uint16 addr, uint16 *pxDsn, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	atcResetCounter();
	CountReset = 0;
	CopyDataBytes((uint8*) &RAM.dwords[addr], (uint8*) pxDsn, len * 2);
	return true;
}
//==============================================================================
bool SetRam(uint16 addr, uint16 *pxSrc, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	atcResetCounter();
	CountReset = 0;
	CopyDataBytes((uint8*) pxSrc, (uint8*) &RAM.dwords[addr], len * 2);
	return true;
}
//==============================================================================
bool GetSignature(uint16 addr, uint16 *pxDsn, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	char bufferSign[64];
	char *DeviceName;
	//setDeviceName
	if (DeviceID == 1) {
		DeviceName = "RUNO UC20       ";
	} else if (DeviceID == 2) {
		DeviceName = "RUNO M66        ";
	} else if (DeviceID == 3) {
		DeviceName = "RUNO UC15       ";
	} else if (DeviceID == 4) {
		DeviceName = "RUNO N715       ";
	} else {
		DeviceName = "RUNO           ";
	}
	uint8 version[4] = { 0, 17, 6, 8 };/*под новый процессор GD32F103 12 версия мигание светодиода*/
	char *DateVersion = "18/09/2024"; // ошибки часов нет
	CopyDataBytes((uint8*) DeviceName, (uint8*) bufferSign,
			strlen(DeviceName) + 1);
	CopyDataBytes((uint8*) version, (uint8*) bufferSign + 0x10, 4);
	CopyDataBytes((uint8*) DateVersion, (uint8*) bufferSign + 0x20,
			strlen(DateVersion) + 1);
	CopyDataBytes((uint8*) bufferSign + addr * 2, (uint8*) pxDsn, len * 2);
	return true;

}
//==============================================================================
bool GetEeprom(uint16 addr, uint16 *pxDsn, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	uint8 charBuf[256];
	uint8 counter = 0;

	uint8 curRead = 0;
	while (curRead < len) {
		counter = 0;
		while (!I2C_EE_BufferRead(charBuf + curRead * 2, (addr + curRead) * 2,
				((len - curRead > 0x10) ? 0x10 : (len - curRead)) * 2)) {
			if (counter++ >= 10)
				return false;
		}
		curRead += 0x10;
	}

	CopyDataBytes((uint8*) charBuf, (uint8*) pxDsn, len * 2);
	return true;
}
//==============================================================================
bool SetEeprom(uint16 addr, uint16 *pxSrc, uint16 len) {
	uint8 charBuf[256];
	uint8 counter = 0;

	uint8 curRead = 0;

	if ((pxSrc[0] == 0) && (addr == 0)) {
		return false;
	} else {

		while (!I2C_EE_BufferWrite((uint8*) pxSrc, addr * 2, len * 2)) {
			if (counter++ >= 10)
				return false;
		}
		vTaskDelay(10);
		while (curRead < len) {
			counter = 0;
			while (!I2C_EE_BufferRead(charBuf + curRead * 2,
					(addr + curRead) * 2,
					((len - curRead > 0x10) ? 0x10 : (len - curRead)) * 2)) {
				if (counter++ >= 10)
					return false;
			}

			curRead += 0x10;
		}
		//vTaskDelay(10);
		return Buffercmp(charBuf, (uint8*) pxSrc, len * 2);
	}
}

//==============================================================================
bool GetRtc(uint16 addr, uint16 *pxDsn, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	DATATIME temprtc;

	char iFixstr[16];

	tmpPointer = addr;
	tmpLen = len;
	int tmp = 0;
	int wr = 0;

	if (tmpPointer >= 0 && tmpPointer < 0x08 && tmpLen > 0) {
		rtcGetDataTime(&temprtc);
		tmp = ((tmpPointer + tmpLen) > 8) ? ((8 - tmpPointer)) : (tmpLen);

		CopyDataBytes((uint8*) ((uint8*) (&temprtc) + tmpPointer * 2),
				(uint8*) (pxDsn) + wr * 2, tmp * 2);
		tmpPointer += tmp;
		wr += tmp;
		tmpLen -= tmp;
	}
	if (tmpPointer >= 0x08 && tmpPointer < 0x10 && tmpLen > 0) {
		rtcGetDataTime(&temprtc);
		//rtcGetLocalDataTime(&temprtc);
		tmp = ((tmpPointer + tmpLen) > 0x10) ? ((0x10 - tmpPointer)) : (tmpLen);
		CopyDataBytes((uint8*) ((uint8*) (&temprtc) + (tmpPointer - 0x08) * 2),
				(uint8*) (pxDsn) + wr * 2, tmp * 2);
		tmpPointer += tmp;
		wr += tmp;
		tmpLen -= tmp;
	}
	if (tmpPointer >= 0x20 && tmpPointer < 0x30 && tmpLen > 0) {
		rtcGetiFixDateTime(iFixstr);
		tmp = ((tmpPointer + tmpLen) > 0x30) ? ((0x30 - tmpPointer)) : (tmpLen);
		CopyDataBytes((uint8*) ((uint8*) (&iFixstr) + (tmpPointer - 0x20) * 2),
				(uint8*) (pxDsn) + wr * 2, tmp * 2);
		tmpPointer += tmp;
		wr += tmp;
		tmpLen -= tmp;
	}

	atcResetCounter();
	CountReset = 0;
	return true;
}
//==============================================================================
bool SetRtc(uint16 addr, uint16 *pxSrc, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	DATATIME temprtc;

	char iFixstr[16];

	if (addr >= 0x20) {
		rtcGetiFixDateTime(iFixstr);
		CopyDataBytes((uint8*) pxSrc, (uint8*) iFixstr, len * 2);
		rtcSetiFixDateTime(iFixstr);
	} else if (addr < 0x08) {
		rtcGetDataTime(&temprtc);
		CopyDataBytes((uint8*) pxSrc, (uint8*) ((uint8*) (&temprtc) + addr * 2),
				((len + addr > 0x08) ? (8 - addr) : (len)) * 2);
		rtcSetDataTime(&temprtc);
	} else if (addr >= 0x08 && addr < 0x10) {
		rtcGetDataTime(&temprtc);
		//rtcGetLocalDataTime(&temprtc);
		CopyDataBytes((uint8*) pxSrc,
				(uint8*) ((uint8*) (&temprtc) + (addr - 0x08) * 2),
				((len + addr > 0x10) ? (0x10 - addr) : (len)) * 2);
		rtcSetDataTime(&temprtc);
		//rtcSetLocalDataTime(&temprtc);
	}

	atcResetCounter();
	CountReset = 0;
	return true;
}
//==============================================================================

//==============================================================================
bool GetFlash(uint16 addr, uint16 *pxDsn, uint16 len) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	void *pxFlash = (void*) (DEVICE_FLASHMEM_LOCATION);
	//pxFlash = (void*) ((int) pxFlash + addr * 2);
	CopyDataBytes(((uint8*) pxFlash) + addr * 2, (uint8*) pxDsn, len * 2);
	atcResetCounter();
	CountReset = 0;
	return true;
}
//==============================================================================
bool SetFlash(uint16 addr, uint16 *pxSrc, uint16 len) {

	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;

	uint16 tempadr = addr;
	uint16 page = addr / 0x400;
	uint16 pageEnd = (addr + len) / 0x400;
	uint16 pageOffset = addr % 0x400;
	uint16 pageEndOffset = (addr + len) % 0x400;
	FLASH_Status status;
	//vTaskDelay(100);
	FLASH_Unlock();
	if (page != pageEnd) {

		//=================================first part
		CopyDataBytes(
				(uint8*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
				(uint8*) bigFlashBuff, 0x800);
		CopyDataBytes((uint8*) pxSrc, (uint8*) (&bigFlashBuff[pageOffset]),
				(0x400 - pageOffset) * 2);

		while (FLASH_ErasePage(DEVICE_FLASHMEM_LOCATION + 0x800 * page)
				!= FLASH_COMPLETE) {
			status = FLASH_GetStatus();
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			vTaskDelay(20);
		}

		WriteFlash((void*) (bigFlashBuff),
				(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
				0x800);
		vTaskDelay(100);

		while (FLASH_GetStatus() != FLASH_COMPLETE) {
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			WriteFlash((void*) (bigFlashBuff),
					(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
					0x800);
			vTaskDelay(100);
		}
		//=================================Second part
		CopyDataBytes(
				(uint8*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * pageEnd),
				(uint8*) bigFlashBuff, 0x800);
		CopyDataBytes(((uint8*) pxSrc) + (len - pageEndOffset) * 2,
				(uint8*) (&bigFlashBuff[0]), pageEndOffset * 2);

		while (FLASH_ErasePage(DEVICE_FLASHMEM_LOCATION + 0x800 * pageEnd)
				!= FLASH_COMPLETE) {
			status = FLASH_GetStatus();
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			vTaskDelay(20);
		}

		WriteFlash((void*) (bigFlashBuff),
				(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * pageEnd),
				0x800);
		vTaskDelay(100);
		while (FLASH_GetStatus() != FLASH_COMPLETE) {
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			WriteFlash((void*) (bigFlashBuff),
					(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION)
							+ 0x800 * pageEnd), 0x800);
			vTaskDelay(100);
		}
	} else {
		CopyDataBytes(
				(uint8*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
				(uint8*) bigFlashBuff, 0x800);
		CopyDataBytes((uint8*) pxSrc, (uint8*) (&bigFlashBuff[pageOffset]),
				len * 2);

		while (FLASH_ErasePage(DEVICE_FLASHMEM_LOCATION + 0x800 * page)
				!= FLASH_COMPLETE) {
			status = FLASH_GetStatus();
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			vTaskDelay(20);
		}

		WriteFlash((void*) (bigFlashBuff),
				(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
				0x800);
		vTaskDelay(100);
		while (FLASH_GetStatus() != FLASH_COMPLETE) {
			FLASH_ClearFlag(
					FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			WriteFlash((void*) (bigFlashBuff),
					(void*) ((uint8*) (DEVICE_FLASHMEM_LOCATION) + 0x800 * page),
					0x800);
			vTaskDelay(100);
		}
	}
	FLASH_Lock();
	atcResetCounter();
	JournalConf(tempadr);
	return true;
}

bool Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength) {
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	while (BufferLength--) {
		if (*pBuffer1 != *pBuffer2) {
			return false;
		}

		pBuffer1++;
		pBuffer2++;
	}

	return true;
}

