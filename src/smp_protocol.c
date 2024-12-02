#include "smp_protocol.h"
#include <time.h>
#include "rtclock.h"

#define MAX_POW 65.535
#define MAX_CUR 131.072
#define MAX_VOLT 655.35

//--crc16/bypass
const uint16 Crc16Tabl[256] = { 0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E,
		0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
		0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
		0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041, 0x80C3,
		0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5,
		0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5, 0x80AF,
		0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093, 0x0096, 0x009C, 0x8099,
		0x0088, 0x808D, 0x8087, 0x0082, 0x8183, 0x0186, 0x018C, 0x8189, 0x0198,
		0x819D, 0x8197, 0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,
		0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4,
		0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
		0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173,
		0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123, 0x0126,
		0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132, 0x0110, 0x8115, 0x811F,
		0x011A, 0x810B, 0x010E, 0x0104, 0x8101, 0x8303, 0x0306, 0x030C, 0x8309,
		0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
		0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E,
		0x0374, 0x8371, 0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347,
		0x0342, 0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
		0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2, 0x83A3,
		0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2, 0x0390, 0x8395,
		0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381, 0x0280, 0x8285, 0x828F,
		0x028A, 0x829B, 0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9,
		0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8,
		0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE,
		0x02C4, 0x82C1, 0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257,
		0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
		0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213,
		0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202

};

uint16 SetCRC16(uint8 *pcBlock, uint16 len) {
	uint16 crc = 0x0000;

	while (len--)
		crc = (crc << 8) ^ Crc16Tabl[(crc >> 8) ^ *pcBlock++];

	*pcBlock = crc >> 8;
	*(pcBlock + 1) = crc;

	return crc;
}

//���������� ��� ����
uint16 AskVoltA(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_VOLT;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_A;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskVoltB(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_VOLT;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_B;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskVoltC(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_VOLT;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_C;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}
//���� ��� ����
uint16 AskCurA(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_CUR;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_A;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskCurB(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_CUR;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_B;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskCurC(uint8 *pxBuf318, uint32 MeterNumber)

{
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_CUR;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_C;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}
//�������� ��� ����
uint16 AskPowA(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_POW;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_A;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskPowB(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_POW;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_B;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskPowC(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_POW;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_C;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}
//�����, ��� ������ ������ �� ���������
uint16 AskNum(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;
	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = 0x03;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = 0x04;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

uint16 AskEnerge(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_DATA_SINGLE;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_ENERGY;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[10] = GET_ENFULL;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[11] = 0x00;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);

}

//����-�����
uint16 AskTime(uint8 *pxBuf318, uint32 MeterNumber) {
	uint16 crc318;
	uint8 tmpbuff318[64];
	unsigned short lenBuf = 0;
	*pxBuf318 = SYM_ST318;

	pxBuf318++;
	*pxBuf318 = tmpbuff318[0] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[1] = MeterNumber;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[2] = MeterNumber >> 8;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[3] = MeterNumber >> 16;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[4] = MeterNumber >> 24;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[5] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[6] = GET_INFO;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[7] = GET_OPTION;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[8] = 0x00;
	pxBuf318++;
	lenBuf++;

	*pxBuf318 = tmpbuff318[9] = GET_TIME;
	pxBuf318++;
	lenBuf++;

	crc318 = SetCRC16(tmpbuff318, lenBuf);

	*pxBuf318 = crc318 >> 8;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = crc318;
	pxBuf318++;
	lenBuf++;
	*pxBuf318 = SYM_ST318;

	return (lenBuf + 2);
}

void new_stof(char *str1, float f, char size)

{

	char pos = 0;  // position in string

	char len = 0;  // length of decimal part of result

	char curr[3];  // temp holder for next digit

	int value = 0;  // decimal digit(s) to convert
	int value2 = 0;  // decimal digit(s) to convert

	value = (int) f;  // truncate the floating point number
	itoa1(value, str1); // this is kinda dangerous depending on the length of str
	// now str array has the digits before the decimal

	if (f < 0)  // handle negative numbers
			{
		f *= -1;
		value *= -1;
	}

	len = strlen(str1);  // find out how big the integer part was
	pos = len;  // position the pointer to the end of the integer part
	str1[pos++] = '.';  // add decimal point to string

	value2 = value;

	while (pos < (size + len + 1))  // process remaining digits
	{

		f = f - (float) value2;  // hack off the whole part of the number
		f *= 10;  // move next digit over
		value2 = (int) f;  // get next digit
		itoa1(value2, curr); // convert digit to string
		str1[pos++] = *curr; // add digit to result string and increment pointer
		// str1[pos++] = *curr; // add digit to result string and increment pointer
		// str1[pos++] = *(curr+1); // add digit to result string and increment pointer
		// str1[pos++] = *(curr+2); // add digit to result string and increment pointer
	}

}

uint32* Decode(uint8 *buf, uint8 bufsize) {
	uint32 result[bufsize];
	int num = 0;
	uint32 ch;
	for (int i = 0; i < bufsize; i++) {
		result[i] = 0;
		do {
			ch = (*buf & 0x7F); // ������ ��������� 7 ���
			result[i] += ch << (num * 7); //��������� � ����������
			num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
		} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
	}
	return result;
}

bool ParseAnswerNum(uint8 *buf, char *dsn) {
	uint32 *tempNum;
	uint32 temp;

	if (*buf == SYM_ST318) {
		buf = buf + 16;

		if (*buf == GET_OPTION) {
			buf++;
			tempNum = Decode(buf, 1);
			temp = *tempNum;
			itoa1(temp, dsn);
		} else
			return false;
	} else
		return false;

	return true;

}

unsigned char month_day_table2[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30,
		31 };

int GetYearsFromSecondsFrom2000(uint32 timeSec) {
	uint16 predvYear = (uint32) (timeSec / 31536000/*31556926*/); //31536000 Seconds in leap year
	if (predvYear
			!= (timeSec + ((uint32) (predvYear / 4) + 1) * 86400)
					/ 31536000/*31556926*/) //86400 Seconds in one day
		predvYear++;
	//predvYear = predvYear+12;//2012���
	return predvYear;
}

void TimeString(char *aBuf, DATATIME dtl) {
	//DATATIME dtl;

	//rtcGetDataTime(&dtl);
	char conv_buf[8];
	if ((dtl.Hour < 24) && (dtl.Min < 60) && (dtl.Sec < 60)) {
		itoa_n(dtl.Hour, conv_buf, 2);
		strcpy(aBuf, conv_buf);
		strcat(aBuf, ":");
		itoa_n(dtl.Min, conv_buf, 2);
		strcat(aBuf, conv_buf);
		strcat(aBuf, ":");
		itoa_n(dtl.Sec, conv_buf, 2);
		strcat(aBuf, conv_buf);
	}
}

void DateString(char *aBuf, DATATIME dtl) {
	//DATATIME dtl;
	//rtcGetDataTime(&dtl);
	char conv_buf[16];
	if ((dtl.Day < 8) && (dtl.Data < 32) && (dtl.Month < 13)) {
		itoa_n(dtl.Day, conv_buf, 2);
		strcpy(aBuf, conv_buf);
		strcat(aBuf, ".");
		itoa_n(dtl.Data, conv_buf, 2);
		strcat(aBuf, conv_buf);
		strcat(aBuf, ".");
		itoa_n(dtl.Month, conv_buf, 2);
		strcat(aBuf, conv_buf);
		strcat(aBuf, ".");
		itoa1(dtl.Years, conv_buf);
		strcat(aBuf, conv_buf);
	}

}

bool ParseAnswerData(char *buf, char *dsn1, char *dsn2) {
	DATATIME dtsmp;
	uint32 tempNum[1];
	uint32 timeSec;
	int lYear;

	if (*buf == SYM_ST318) {
		buf = buf + 10;

		if (*buf == GET_TIME) {
			buf++;
			//	tempNum[0] = *(Decode(buf, 1));
			uint32 result[1];
			int num = 0;
			uint32 ch;
			for (int i = 0; i < 1; i++) {
				result[i] = 0;
				do {
					ch = (*buf & 0x7F); // ������ ��������� 7 ���
					result[i] += ch << (num * 7); //��������� � ����������
					num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
				} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
			}
			timeSec = result[0];

			dtsmp.Sec = timeSec % 60;
			dtsmp.Min = (timeSec / 60) % 60;
			dtsmp.Hour = (timeSec / (60 * 60)) % 24;
			dtsmp.Day = ((timeSec / (60 * 60 * 24))) % 7;	//1- �����������
			//dtsmp.Years = GetYearsFromSecondsFrom2000(timeSec);
			int16 daysCurYear = timeSec / (60 * 60 * 24) + 1;		//01/01/2012
			dtsmp.Years = 2012;						    //2012 ��� �� ���������
			while (true) {
				lYear = leap_year(dtsmp.Years);
				if ((dtsmp.Years % 4 == 0) && (lYear == 1)) {
					if (daysCurYear > 366) {
						daysCurYear = daysCurYear - 366;
						dtsmp.Years++;
					} else {
						break;
					}

				} else if (daysCurYear > 365) {
					daysCurYear = daysCurYear - 365;
					dtsmp.Years++;
				} else {
					break;
				}
			}

			for (int i = 0; i < 12; i++) {
				if (daysCurYear
						> (month_day_table2[i] + ((i == 1) ? lYear : 0))) {
					daysCurYear -= month_day_table2[i] + ((i == 1) ? lYear : 0);
				} else {
					dtsmp.Month = i + 1;
					dtsmp.Data = daysCurYear;
					break;
				}
			}

			DateString(dsn1, dtsmp);
			TimeString(dsn2, dtsmp);

			//itoa(temp, dsn);
		} else
			return false;
	}

	return true;
}

bool ParseAnswerVolt(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/) {
	uint32 *tempNum;
	float temp = 0.0;
	//uint8 *tmpbuff;

	if (*buf == SYM_ST318) {
		buf = buf + 10;

		if (*buf == GET_VOLT) {
			buf++;
			//tempNum = Decode(buf, 1);
			uint32 result[1];
			int num = 0;
			uint32 ch;
			for (int i = 0; i < 1; i++) {
				result[i] = 0;
				do {
					ch = (*buf & 0x7F); // ������ ��������� 7 ���
					result[i] += ch << (num * 7); //��������� � ����������
					num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
				} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
			}
			temp = (result[0]) / 100.0;

			if (temp > MAX_VOLT) {
				return false;
			}
			/*temp[1] = *(tempNum+1)/100.0;
			 temp[2] = *(tempNum+2)/100.0;*/
			new_stof(dsn1, temp, 3);
			/*new_stof(dsn2, temp[1], 3);
			 new_stof(dsn3, temp[2], 3);*/

		} else
			return false;
	} else
		return false;

	return true;
}

bool ParseAnswerCur(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/) {
	uint32 *tempNum;
	float temp = 0.0;

	if (*buf == SYM_ST318) {
		buf = buf + 10;

		if (*buf == GET_CUR) {
			buf++;
			uint32 result[1];
			int num = 0;
			uint32 ch;
			for (int i = 0; i < 1; i++) {
				result[i] = 0;
				do {
					ch = (*buf & 0x7F); // ������ ��������� 7 ���
					result[i] += ch << (num * 7); //��������� � ����������
					num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
				} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
			}

			temp = result[0] / 1000.0;

			if (temp > MAX_CUR) {
				return false;
			}
			/*temp[1] = *(tempNum+1)/100.0;
			 temp[2] = *(tempNum+2)/100.0;*/
			new_stof(dsn1, temp, 3);
			/*new_stof(dsn2, temp[1], 3);
			 new_stof(dsn3, temp[2], 3);*/

		} else
			return false;
	} else
		return false;

	return true;
}

bool ParseAnswerPow(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/) {
	uint32 *tempNum;
	float temp = 0.0;

	if (*buf == SYM_ST318) {
		buf = buf + 10;

		if (*buf == GET_POW) {
			buf++;
			uint32 result[1];
			int num = 0;
			uint32 ch;
			for (int i = 0; i < 1; i++) {
				result[i] = 0;
				do {
					ch = (*buf & 0x7F); // ������ ��������� 7 ���
					result[i] += ch << (num * 7); //��������� � ����������
					num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
				} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
			}

			temp = result[0] / 1000.0;

			if (temp > MAX_POW) {
				return false;
			}
			/*temp[1] = *(tempNum+1)/100.0;
			 temp[2] = *(tempNum+2)/100.0;*/
			new_stof(dsn1, temp, 3);
			/*new_stof(dsn2, temp[1], 3);
			 new_stof(dsn3, temp[2], 3);*/

		} else
			return false;
	} else
		return false;

	return true;
}

bool ParseAnswerEnerge(char *buf, char *dsn1) {
	uint32 *tempNum;
	float temp;

	if (*buf == SYM_ST318) {
		buf = buf + 10;

		if (*buf == GET_ENERGY) {
			buf++;
			//	tempNum = Decode(buf, 1);
			uint32 result[1];
			int num = 0;
			uint32 ch;
			for (int i = 0; i < 1; i++) {
				result[i] = 0;
				do {
					ch = (*buf & 0x7F); // ������ ��������� 7 ���
					result[i] += ch << (num * 7); //��������� � ����������
					num++; // ����� num+=7;, ����� ������ (num*7) ����� ������ num
				} while ((*(buf++) & 0x80)); //������� ������ ������� ���� lbf
			}

			temp = result[0] / 10000.0;
			new_stof(dsn1, temp, 3);

		} else
			return false;

	} else
		return false;
	return true;
}

