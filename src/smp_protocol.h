#ifndef SMP_PROTOCOL_H
#define SMP_PROTOCOL_H
#include "typedef.h"
// Main protocol symbols
#define SYM_ST318		0xC0

#define GET_OPTION		    0x04
#define GET_INFO		    0x06
#define GET_DATA_SINGLE		0x0A
#define GET_ENERGY			0x01
#define GET_ENFULL 			0x02
#define GET_POW				0x0D
#define GET_CUR 			0x16
#define GET_VOLT			0x18
#define GET_ABC				0x1C
#define GET_A				0x04
#define GET_B				0x08
#define GET_C				0x10
#define GET_TIME			0x01

extern uint16 AskVolt(uint8* pxBuf, uint32 MeterNumber);
extern uint16 AskCur(uint8* pxBuf, uint32 MeterNumber);
extern uint16 AskPow(uint8* pxBuf, uint32 MeterNumber);
extern uint16 AskNum(uint8* pxBuf,uint32 MeterNumber);
extern uint16 AskTime(uint8* pxBuf,uint32 MeterNumber);
extern uint16 AskEnerge(uint8* pxBuf318,uint32 MeterNumber);

extern bool ParseAnswerNum(uint8 *buf, char *dsn);
extern bool ParseAnswerData(char *buf,  char *dsn1,char *dsn2);
extern bool ParseAnswerVolt(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/);
extern bool ParseAnswerCur(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/);
extern bool ParseAnswerPow(char *buf, char *dsn1/*,char *dsn2,char *dsn3*/);
extern bool ParseAnswerEnerge(char *buf, char *dsn1);


#endif
