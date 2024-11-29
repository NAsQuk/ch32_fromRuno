#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usermemory.h"
//#include "adc.h"
//#include "pwmcapture.h"
#include "iec61107.h"
#include "smp_protocol.h"
#include "rvnet.h"
#include "uart.h"
//#include "serial.h"

#include "stm32f10x.h"
//#include "usb_lib.h"
//#include "usb_desc.h"
//#include "hw_config.h"
//#include "usb_pwr.h"

char rs485buf[256];
char tmpbuff[128];
RVNET_DATATYPE rs485size;
IDENTIFIER id;
uint32 MeterNumber;

bool AskConunter();
bool AskConunter318();

void ClearUIPData()
{
	for(int i=0;i<16;i++)
	{
		RAM.UIPparams[i] = 0;
		for(int j=0;j<16;j++)
			{
				RAM.strings[i][j] = 0;
			}
	}
}

void ClearBuffer(int size)
{
  for(int i=0;i<size;i++)
  {
	  rs485buf[i]=0xFF;
  }
}
uint16 ConvertUIP(char* string)
    {

    return (atof(string) / 400) * 65536;
    }


double new_atof(char* s)
{
 double sign = 1;
 double val = 0;
 int d = 0;
 if (s) while (((*s<'0' || *s>'9') && *s!='-' && *s!='+' && *s!='.') && *(++s));
 else return 0.0;
 sign*=(*s == '-' && ++s) ? -1.0 : ((*s == '+' && ++s),1.0);
 while ((*s >= '0' && *s <= '9') || (*s == '.' && d < 1))
 {
   val += *s == '.' ? (++d, 0.0) : d > 0 ? ((++d, val*=10.0), *s-'0') :(val*=10.0, *s-'0');
   ++s;
 }
 double den = 1;
 if(d > 0) while(--d) den*=10.0;
 return (val*sign)/den;
}

void MakeiFixUIP()
    {

    RAM.UIPparams[0] = (uint16) (new_atof(&RAM.strings[8][0]) * 65535 / 400);
    RAM.UIPparams[1] = (uint16) (new_atof(&RAM.strings[9][0]) * 65535 / 400);
    RAM.UIPparams[2] = (uint16) (new_atof(&RAM.strings[10][0]) * 65535 / 400);

    //current
    RAM.UIPparams[3] = (uint16) (new_atof(&RAM.strings[11][0]) * 65535 / 100);
    RAM.UIPparams[4] = (uint16) (new_atof(&RAM.strings[12][0]) * 65535 / 100);
    RAM.UIPparams[5] = (uint16) (new_atof(&RAM.strings[13][0]) * 65535 / 100);
    //Power
    RAM.UIPparams[6] = (uint16) (new_atof(&RAM.strings[5][0]) * 65535 / 40);
    RAM.UIPparams[7] = (uint16) (new_atof(&RAM.strings[6][0]) * 65535 / 40);
    RAM.UIPparams[8] = (uint16) (new_atof(&RAM.strings[7][0]) * 65535 / 40);

    RAM.UIPparams[9] = ((uint32)(new_atof(&RAM.strings[4][0])*4294967296/999999))&0xFFFF;
    RAM.UIPparams[10] = (((uint32)(new_atof(&RAM.strings[4][0])*4294967296/999999))>>16)&0xFFFF;

    RAM.UIPparams[11] = ((uint32)(new_atof(&RAM.strings[15][0])*4294967296/999999))&0xFFFF;
    RAM.UIPparams[12] = (((uint32)(new_atof(&RAM.strings[15][0])*4294967296/999999))>>16)&0xFFFF;

   // RAM.UIPparams[13] = ((uint32)(new_atof(&RAM.strings[16][0])*4294967296/60000))&0xFFFF;
  //  RAM.UIPparams[14] = (((uint32)(new_atof(&RAM.strings[16][0])*4294967296/60000))>>16)&0xFFFF;

//    RAM.UIPparams[13] = ((uint32)(new_atof(&RAM.strings[16][0])*4294967296/999999))&0xFFFF;
//    RAM.UIPparams[14] = (((uint32)(new_atof(&RAM.strings[16][0])*4294967296/999999))>>16)&0xFFFF;
//
//   // RAM.UIPparams[15] = ((uint32)(new_atof(&RAM.strings[17][0])*65535/2000));
//    RAM.UIPparams[15] = ((uint32)(new_atof(&RAM.strings[17][0])*4294967296/999999))&0xFFFF;
//    RAM.UIPparams[16] = (((uint32)(new_atof(&RAM.strings[17][0])*4294967296/999999))>>16)&0xFFFF;

    }

//extern uint8_t buffer_out[VIRTUAL_COM_PORT_DATA_SIZE];

/* Private variables ---------------------------------------------------------*/
 USART_InitTypeDef USART_InitStructure;



void DMA_Configuration(void);

//uint32_t T1, oldT1, cntT1;
//uint32_t T2, oldT2, cntT2;


/*
uint8_t SerialGetBuffer(xComPortHandle pxPort, uint8_t* aBuffer)
{
	uint8_t count = 0;
	uint8_t bufvar = 0;
	while (xSerialGetChar(pxPort,&bufvar,50) == pdTRUE)
	{
		aBuffer[count++] = bufvar;
		vTaskDelay(10);
	}
	return count;
}*/


/*
void writeBuffer(uint8_t* buffer,uint8_t  size)
{
	for(int i=0;i<size;i++)
	{
		USART_SendData (uart4, buffer++);
	}
}*/

void processBuffer7to8(char* buf,uint16 size)
{
	for(int i=0;i<size;i++)
	{
		buf[i]=buf[i]&0x7F;
	}
}

void processBuffer8to7(char* buf,uint16 size)
{
	int count=0;
	for(int i=0;i<size;i++)
	{
		count = 0;
		count += (buf[i]&0x01) ? 1 : 0;
		count += (buf[i]&0x02) ? 1 : 0;
		count += (buf[i]&0x04) ? 1 : 0;
		count += (buf[i]&0x08) ? 1 : 0;
		count += (buf[i]&0x10) ? 1 : 0;
		count += (buf[i]&0x20) ? 1 : 0;
		count += (buf[i]&0x40) ? 1 : 0;

		buf[i]=buf[i]|(count%2)<<7;
	}
}

bool SendReseive()
    {
	int exit_counter = 0;
    //  vTaskDelay(50);
	int sendsize = rs485size;
	while(exit_counter++ <5)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_12);
	   // AT91F_PIO_SetOutput(AT91C_BASE_PIOA, LED_485);
		vTaskDelay(5);

		processBuffer8to7(rs485buf,sendsize);
		uart3Write ((uint8*) rs485buf, sendsize);
		vTaskDelay(30);

		//AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, LED_485);

		GPIO_ResetBits(GPIOC, GPIO_Pin_12);

		vTaskDelay(1000);

		rs485size = uart3Read((uint8*) rs485buf, 256/*, 3000*/);

    if (rs485size != 0) break;
    }
    if (rs485size == 0)
    {   vTaskDelay(2500);
    	return false;
    }

    processBuffer7to8(rs485buf,rs485size);
    vTaskDelay(50);
  //  AT91F_PIO_SetOutput(AT91C_BASE_PIOA, LED_485);
    vTaskDelay(100);
    //

    return true;
    }

bool AskParam(char *paramName, char *value)
    {
    value[0] = 0;
    rs485size = iecProcVarReqPacket(rs485buf, paramName, 32);
    if (!SendReseive())
	return false;

    uint8 boolX = iecProcVarAnswerPacket(rs485buf, value, 128);
    if(boolX == false)
	{
    	return false;
	}

    return true;
    }

bool AskParamArg(char *paramName, char *value,char *arg)
    {
    value[0] = 0;
    rs485size = iecProcVarReqPacketArg(rs485buf, paramName, 32,arg);
    if (!SendReseive())
	return false;
    uint8 boolX = iecProcVarAnswerPacket(rs485buf, value, 128);
    if(boolX == false)
   	{
       	return false;
   	}
    return true;
    }
char* ParseAnswer(char *buf, char *dsn)
    {
    while ( *buf != '(')
	buf++;
    buf++;
    while (*buf && *buf != ')')
	{
	*dsn = *buf;
	dsn++;
	buf++;
	}
    *dsn = 0;
    return buf;
    }
bool AskConunter()
    {
	MakeiFixUIP();
    char *pxBuf;
    char *tmpbuffX;
    // Find Device
    rs485size = iecProcReqPacket(rs485buf, "", 10);
    if (!SendReseive())
    	return false;
    uint8 boolX = iecIndProc(rs485buf, &id);
    if (false==boolX)
    {
    	return false;
    }

    // Password & security

	rs485size = iecOptionsAckPacket(rs485buf, '0', '5', '1');
	if (!SendReseive())
		return false;

	boolX = iecProcAddrAnswer(rs485buf, &RAM.strings[0][0], 128);
	if (false==boolX)
	{
		return false;
	}

    if (!AskParam("DATE_", tmpbuff))
    		return false;
    ParseAnswer(tmpbuff, &RAM.strings[1][0]);

    if (!AskParam("TIME_", tmpbuff))
    		return false;
    ParseAnswer(tmpbuff, &RAM.strings[2][0]);


	if (!AskParam("SNUMB", tmpbuff))
			return false;
	ParseAnswer(tmpbuff, &RAM.strings[3][0]);

    if (!AskParam("ET0PE", tmpbuff))
    		return false;
    ParseAnswer(tmpbuff, &RAM.strings[4][0]);

    if (!AskParam("POWPP", tmpbuff))
    		return false;
    pxBuf = tmpbuff;
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[5][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[6][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[7][0]);

    if (!AskParam("VOLTA", tmpbuff))
    		return false;
    pxBuf = tmpbuff;
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[8][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[9][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[10][0]);

    if (!AskParam("CURRE", tmpbuff))
    		return false;
    pxBuf = tmpbuff;
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[11][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[12][0]);
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[13][0]);

    if (!AskParam("FREQU", tmpbuff))
    		return false;
    pxBuf = tmpbuff;
    pxBuf = ParseAnswer(pxBuf, &RAM.strings[14][0]);

    if (!AskParam("ET0PE", tmpbuff))
    		return false;
    ParseAnswer(tmpbuff, &RAM.strings[15][0]);


    /*GetDateStringLM(tmpbuffX);
    if (!AskParamArg("ENMPE", tmpbuff,tmpbuffX))
         	return false;
    ParseAnswer(tmpbuff, &RAM.strings[16][0]);

    GetDateStringLD(tmpbuffX);
    if (!AskParamArg("ENDPE", tmpbuff,tmpbuffX))
          	return false;
    ParseAnswer(tmpbuff, &RAM.strings[17][0]);*/

    MakeiFixUIP();

    rs485size = iecProcExitPacket(rs485buf);
    GPIO_SetBits(GPIOC, GPIO_Pin_12);
    vTaskDelay(5);
    processBuffer8to7(rs485buf,rs485size);
    uart3Write ((uint8*) rs485buf, rs485size);
    vTaskDelay(10);
    GPIO_ResetBits(GPIOC, GPIO_Pin_12);
    vTaskDelay(500);

    return true;
    }

//------------------------------------------
bool SendReseive318(uint8* buffer, uint16 buffersize)
{
	//int bufsize318 = 0;
	//uint8 buffer318[bufsize318];
	int exit_counter = 0;
	rs485size = 0;

		/*while(exit_counter++ <5)
		{*/
		if  (rs485size == 0)
		{

			GPIO_SetBits(GPIOC, GPIO_Pin_12);
			vTaskDelay(5);

		    uart3Write (buffer, buffersize);

		    vTaskDelay(30);

		    //AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, LED_485);

		    GPIO_ResetBits(GPIOC, GPIO_Pin_12);

		    vTaskDelay(100);

			rs485size = uart3Read((uint8*) rs485buf, 256/*, 3000*/);


		   if (rs485size != 0)
		   {  //vTaskDelay(200);
			   return true;
		   	   }
		   else
			   {	vTaskDelay(2500);
			   	   return false;
			   }
			}


}


bool AskConunter318()
{
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	uint8* buffer318;
	uint16 buffersize318;

	//MakeiFixUIP();

	MeterNumber = pxConfig->devcfg.MeterNum;

	for(int i=0;i<6;i++)
					{
				    RAM.strings[0][i] = '7';
					}

/*	buffersize318 = AskNum(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
		return false;

	ParseAnswerNum(rs485buf, &RAM.strings[3][0]);*/

	itoa1(MeterNumber, &RAM.strings[3][0]); //серийный номер счетчика


	buffersize318 = AskTime(rs485buf,MeterNumber);
		if (!SendReseive318(rs485buf, buffersize318))
				return false;

	ParseAnswerData(rs485buf, &RAM.strings[1][0], &RAM.strings[2][0]);
//
	buffersize318 = AskVoltA(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerVolt(rs485buf, &RAM.strings[8][0]);

	buffersize318 = AskVoltB(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerVolt(rs485buf, &RAM.strings[9][0]);

	buffersize318 = AskVoltC(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerVolt(rs485buf,  &RAM.strings[10][0]);
//
	buffersize318 = AskCurA(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerCur(rs485buf, &RAM.strings[11][0]);

	buffersize318 = AskCurB(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerCur(rs485buf, &RAM.strings[12][0]);

	buffersize318 = AskCurC(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerCur(rs485buf,  &RAM.strings[13][0]);

//
	buffersize318 = AskPowA(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerPow(rs485buf, &RAM.strings[5][0]);

	buffersize318 = AskPowB(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerPow(rs485buf, &RAM.strings[6][0]);

	buffersize318 = AskPowC(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerPow(rs485buf, &RAM.strings[7][0]);
//

	buffersize318 = AskEnerge(rs485buf,MeterNumber);
	if (!SendReseive318(rs485buf, buffersize318))
			return false;

	ParseAnswerEnerge(rs485buf,  &RAM.strings[15][0]);
	ParseAnswerEnerge(rs485buf,  &RAM.strings[4][0]);

	MakeiFixUIP();
	return true;
}


void vRs485Task(void *pvParameters)
{
	FLASHMEM *pxConfig = (FLASHMEM*) DEVICE_FLASHMEM_LOCATION;
	//vTaskDelay(500);
	int bufsize = 512;
	uint8 buffer[bufsize];
	int MeterType = 0;
	/*GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);*/

	MeterType = pxConfig->devcfg.MeterType;

	if ((MeterType == 1)||(MeterType == 0xFFFF)) // CE301BY
	{
		/*uart3Init(4800);*/uart3Init(9600);
	} else
		if (MeterType == 2) // CE318BY
			{
			for(int i=0;i<6;i++)
				{
			    RAM.strings[0][i] = '7';
				}
				uart3Init(4800);
			}
		else
		{
			USART_DeInit(UART4);
		}

	//xComPortHandle porthandle =  xSerialPortInitMinimal(9600,1024);
	/*buffer[0]='W';
	buffer[1]='A';
	buffer[2]='R';
	buffer[3]='E';
	buffer[4]='F';*/

	GPIO_ResetBits(GPIOC, GPIO_Pin_12);
	int heapSize = 0;

		while (1)
			{

			heapSize = xPortGetFreeHeapSize();
			heapSize +=0;


			while ((pxConfig->devcfg.MeterType == 1)||(pxConfig->devcfg.MeterType == 0xFFFF)) // CE301BY
				{

				IWDG_ReloadCounter();

				if (MeterType != pxConfig->devcfg.MeterType) //если поменялась конфигурация
							{
								MeterType = pxConfig->devcfg.MeterType;

									if ((MeterType == 1)||(MeterType == 0xFFFF)) // CE301BY
									{
										ClearUIPData();

										/*uart3Init(4800);*/uart3Init(9600);

									}
										else break;
									GPIO_ResetBits(GPIOC, GPIO_Pin_12);

							}
				if (AskConunter())
			    	{
					vTaskDelay(100);

			    	}
				else
					{
				//ClearUIPData();
					IWDG_ReloadCounter();
					rs485size = iecProcExitPacket(rs485buf);
					GPIO_SetBits(GPIOC, GPIO_Pin_12);
					vTaskDelay(5);
					processBuffer8to7(rs485buf,rs485size);
					uart3Write ((uint8*) rs485buf, rs485size);
					vTaskDelay(30);
					GPIO_ResetBits(GPIOC, GPIO_Pin_12);
					vTaskDelay(500);
					}
				}

					while (pxConfig->devcfg.MeterType == 2) // CE318BY
									{
						IWDG_ReloadCounter();

						if (MeterType != pxConfig->devcfg.MeterType) //если поменялась конфигурация
									{
										MeterType = pxConfig->devcfg.MeterType;

												if (MeterType == 2) // CE318BY
													{
													    ClearUIPData();
													    for(int i=0;i<6;i++)
													    				{
													    			    RAM.strings[0][i] = '7';
													    				}
														uart3Init(4800);
													}
												else break;
											GPIO_ResetBits(GPIOC, GPIO_Pin_12);
									}
										if (AskConunter318())
									    	{
											vTaskDelay(1000);
									    	}else
									    	{vTaskDelay(1000);}

									}
			while ((pxConfig->devcfg.MeterType != 1)&&(pxConfig->devcfg.MeterType != 2)) // CE318BY
				{
				if (MeterType != pxConfig->devcfg.MeterType) //если поменялась конфигурация
												{
													MeterType = pxConfig->devcfg.MeterType;
													ClearUIPData();
												}

				vTaskDelay(10);
				}
			}
}
