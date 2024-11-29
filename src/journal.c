/*
 * journal.c
 *
 *  Created on: June 16, 2010
 *      Author: Nick Bokhan
 *
 */

#include "journal.h"
#include "usermemory.h"
#include "rtclock.h"
#include "typedef.h"
#include "snprintf.h"

#define JRNLSIZE 0x17*JSIZE
#define JSIZE 250

extern size_t strlen(const char * str);
extern char * strcat(char * destination, const char * source);
uint16 jrnlLength;



void reverse(char *str)
    {
    char c;
    int size=strlen(str)/2;
    for(int i=0;i<size;i++)
	{
	c=str[i];
	str[i]=str[strlen(str)-i-1];
	str[strlen(str)-i-1]=c;
	}
    }

/* itoa:  конвертируем n в символы в s */
void itoa1(int n, char s[])
    {
    int i=0;
    int sign = 0;

    if ((sign = n) < 0) /* записываем знак */
    {
    	n = -n; /* делаем n положительным числом */
    }

    do
	{ /* генерируем цифры в обратном порядке */
	s[i++] = n % 10 + '0'; /* берем следующую цифру */
	}
    while ((n /= 10) > 0); /* удаляем */
    if (sign < 0)
	s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    }

/* itoa:  конвертируем n в символы в s */
void itoa_n(int n, char s[], int nums)
    {
    int i, sign;

    if ((sign = n) < 0) /* записываем знак */
	n = -n; /* делаем n положительным числом */
    i = 0;
    do
	{ /* генерируем цифры в обратном порядке */
	s[i++] = n % 10 + '0'; /* берем следующую цифру */
	}
    while ((n /= 10) > 0); /* удаляем */

    while(i < nums)
    {
    	s[i++] = '0';
    }

    if (sign < 0)
	s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    }

int atoi1(char *c) {
      int res = 0;
      while (*c >= '0' && *c <= '9')
      {
        res = res * 10 + *c++ - '0';
      }
      return res;
    }

void GetTimeString(char *aBuf)
    {
	DATATIME dtl;

	rtcGetDataTime(&dtl);
	char conv_buf[8];
	itoa_n(dtl.Hour,conv_buf,2);
	strcpy(aBuf,conv_buf);
	strcat(aBuf,":");
	itoa_n(dtl.Min,conv_buf,2);
	strcat(aBuf,conv_buf);
	strcat(aBuf,":");
	itoa_n(dtl.Sec,conv_buf,2);
	strcat(aBuf,conv_buf);
    }

void GetDateString(char *aBuf)
    {
	DATATIME dtl;
	rtcGetDataTime(&dtl);
	char conv_buf[8];
	itoa_n(dtl.Data,conv_buf,2);
	strcpy(aBuf,conv_buf);
	strcat(aBuf,"/");
	itoa_n(dtl.Month,conv_buf,2);
	strcat(aBuf,conv_buf);
	strcat(aBuf,"/");
	itoa1(dtl.Years,conv_buf);
	strcat(aBuf,conv_buf);

    }

void GetTimeStringOff(char *aBuf)
    {
	DATATIME dtl;

	rtcGetDataTimeOff(&dtl);
	char conv_buf[8];
	itoa_n(dtl.Hour,conv_buf,2);
	strcpy(aBuf,conv_buf);
	strcat(aBuf,":");
	itoa_n(dtl.Min,conv_buf,2);
	strcat(aBuf,conv_buf);
	strcat(aBuf,":");
	itoa_n(dtl.Sec,conv_buf,2);
	strcat(aBuf,conv_buf);

    }

void GetDateStringOff(char *aBuf)
    {
	DATATIME dtl;
	rtcGetDataTimeOff(&dtl);
	char conv_buf[8];
	itoa_n(dtl.Data,conv_buf,2);
	strcpy(aBuf,conv_buf);
	strcat(aBuf,"/");
	itoa_n(dtl.Month,conv_buf,2);
	strcat(aBuf,conv_buf);
	strcat(aBuf,"/");
	itoa1(dtl.Years,conv_buf);
	strcat(aBuf,conv_buf);
    }




int JrnlClear()
    {
    jrnlLength = 1;
    uint32 buf[0x100]={0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
	    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0
    };

    portENTER_CRITICAL();
    for(int i=0;i<JSIZE;i++)
	{

	    MemSetWords((uint16)(0x2001+0x17*i), (uint16*)buf, 0x17);

	   // vTaskDelay(10);
	}

    MemSetWords(0x2000, &jrnlLength, 1);
    portEXIT_CRITICAL();
    JrnlWrite("Сброс журнала");
    return true;
    }

int JrnlWrite(char *aMsg)
    {
    uint16 testlen,wrLen;
    char buffer[64];
    char Date[16];
    char Time[16];
    BKP_WriteBackupRegister(BKP_DR7, 23);
    GetDateString(Date);
    GetTimeString(Time);

	char conv_buf[8];
	strcpy(buffer,"<");
	strcat(buffer,Date);
	strcat(buffer,"><");
	strcat(buffer,Time);
	strcat(buffer,"><");
	strcat(buffer,aMsg);
	int len = strlen(aMsg);
	while(len++ < 22)
	{
		strcat(buffer," ");
	}
	strcat(buffer,">");


    vTaskDelay(20);
    jrnlLength=GetJrnlLength();

    testlen =(strlen(buffer));

    if (jrnlLength > JRNLSIZE)
	{
	    JrnlClear();
	    return false;
	}
    vTaskDelay(20);
    BKP_WriteBackupRegister(BKP_DR7, 24);
    portENTER_CRITICAL();
    MemSetWords((uint16)(0x2000 + jrnlLength), (uint16*) (buffer), 0x17);
    portEXIT_CRITICAL();
    vTaskDelay(20);

    jrnlLength += 0x17;
    vTaskDelay(20);
    if (jrnlLength > JRNLSIZE)
   	{
	    jrnlLength=1;
   	}
    portENTER_CRITICAL();
    BKP_WriteBackupRegister(BKP_DR7, 25);
    bool x = MemSetWords(0x2000, &jrnlLength, 1);
    portEXIT_CRITICAL();
    BKP_WriteBackupRegister(BKP_DR7, 26);
    if(x)
	{
	return true;
	}else
	    {
	    return false;
	    }
    }


int JrnlWriteOff(char *aMsg)
    {
    uint16 testlen,wrLen;
    char buffer[64];
    char Date[16];
    char Time[16];

    GetDateStringOff(Date);
    GetTimeStringOff(Time);

	char conv_buf[8];
	strcpy(buffer,"<");
	strcat(buffer,Date);
	strcat(buffer,"><");
	strcat(buffer,Time);
	strcat(buffer,"><");
	strcat(buffer,aMsg);
	int len = strlen(aMsg);
	while(len++ < 22)
	{
		strcat(buffer," ");
	}
	strcat(buffer,">");

    vTaskDelay(20);
    jrnlLength=GetJrnlLength();

    testlen =(strlen(buffer));

    if (jrnlLength > JRNLSIZE)
	{
	    JrnlClear();
	    return false;
	}
    vTaskDelay(20);
    portENTER_CRITICAL();
    MemSetWords((uint16)(0x2000 + jrnlLength), (uint16*) (buffer), 0x17);
    portEXIT_CRITICAL();
    vTaskDelay(20);

    jrnlLength += 0x17;
    vTaskDelay(20);
    if (jrnlLength > JRNLSIZE)
   	{
	    jrnlLength=1;
   	}
    portENTER_CRITICAL();
    bool x = MemSetWords(0x2000, &jrnlLength, 1);
    portEXIT_CRITICAL();
    if(x)
	{
	return true;
	}else
	    {
	    return false;
	    }
    }


uint16 GetJrnlLength()
    {

	portENTER_CRITICAL();
	MemGetWords(0x2000, &jrnlLength, 1);
	portEXIT_CRITICAL();
    return jrnlLength;

    }

void JournalConf(int confadr)
{
	if (confadr==0x200)
	{JrnlWrite("Конфигурация изменена");}
		else if (confadr==0x500)
			{JrnlWrite("График осв. изменен");}
		else if (confadr==0x802)
			{JrnlWrite("График подсв. изменен");}
		else if (confadr==0xb04)
			{JrnlWrite("График иллюм. изменен");}
		else if (confadr==0xe06)
			{JrnlWrite("График энерг. изменен");}
		else if (confadr==0x1108)
			{JrnlWrite("График обогр. изменен");}
		else if (confadr==0x0)
			{JrnlWrite("Конф. модема изменена");}
		else if (confadr==0x90)
			{JrnlWrite("Конф. счетчик изменена");}
	}

