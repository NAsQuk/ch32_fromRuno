//********************************************************
//    Project      :
//    Version      :
//    Date         :
//    Author       : Kopachevsky Yuri
//    Company      : BEMN
//    Discription  :
//    Tools        : GNUARM compiller
//*********************************************************
#ifndef  _USERMEMORY
#define  _USERMEMORY
#include "typedef.h"
#include "error.h"

#define MEMUNITS_NUM    (6)
#include "memman.h"



//---------------------------------------------------------
// Flash Memory allocation
#define USERPROGRAMM_LOCATION		(0x08002000)


#define SIGNATURE_WORDS		(128)
#define SIGNATURE_LOCATION	(USERPROGRAMM_LOCATION-SIGNATURE_BYTES)
#define SIGNATURE_BYTES		(0x100)

#define DEVICE_CFG_WORDS	(0x0500)
#define DEVICE_CONST_WORDS	(0x0D00)
#define DEVICE_PROG_WORDS	(0x1600)



#define DEVICE_FLASHMEM_WORDS	(DEVICE_CFG_WORDS+DEVICE_CONST_WORDS+DEVICE_PROG_WORDS)

#define DEVICE_FLASHMEM_LOCATION	(0x08050000 - DEVICE_FLASHMEM_WORDS*2)

#define DEVICE_RAMMEM_WORDS 0x400

//SUBSTRUCTURES
typedef union
    {
	uint16 data[2];
	struct
	    {
		uint8 StartMin;
		uint8 StartHour;
		uint8 FinishMin;
		uint8 FinishHour;
	    };
    }__attribute__((packed)) TIMERANGE;

typedef union
    {
	uint16 data[2];
	struct
	    {
		uint8 StartDay;
		uint8 StartMonth;
		uint8 FinishDay;
		uint8 FinishMonth;
	    };
    }__attribute__((packed)) DATERANGE;

typedef union
    {
	//6 words
	uint16 data[6];
	struct
	    {
		uint16 grafNum; //����� ������� (1 � 4 ������ 1-4 ��� ��������, 5-8 � ������� � ���������)
		uint8 releNum; //����� ���� 1-8 (����� ���� ���� ������������ ������� 1-���, 0-����)
		uint8 discNum; //����� �������� �������� ��������� 1-44
		uint16 Mask[4]; //����� ���������������. (��������������� ���� ������ ���� �������� � ���������)
	    }__attribute__((packed));
    }__attribute__((packed)) ConfChannelControl;

//--------------------------------------------------------


//---------------------------------------------------------
typedef union
    {
	uint16 data[SIGNATURE_WORDS];
	struct
	    {
		char DeviceName[16];
		char Version[4];
		char Reserv1[12];
		char DataTime[20];// "yyyy.MM.dd HH.mm.ss" - string
		char Reserv2[12];
		uint32 Serial;
		uint32 ProgrammSize;
		uint16 ProgrammCRC;

	    };
    }__attribute__((packed)) PROGRAMM_SIGNATURE;
//========================================================
// ============== CONFIGURATION ==========================
typedef struct
    {
	uint16 brt;
	uint16 devaddr;
    }__attribute__((packed)) USARTCFG;
typedef struct
    {
	char adparam[64];
	char callnumber[64];
	char login[64];
	char password[64];
    }__attribute__((packed)) GPRSCFG;
typedef struct
    {
	ConfChannelControl confCU[8]; //0x30 words
	uint16 MaskSecurity[4]; //0x04 words
	uint16 MaskControl[4]; //0x04 words
	uint16 MaskPower[4]; //0x04 words
	uint16 SwitchTime; //0x01 words
	uint16 Years; //823D
	uint16 Month; //823E
	uint16 Data; //823F
 	uint16 Hour; //8240
	uint16 Min; //8241
	uint16 Sec; //8242
	uint16 MSec; //8243
	uint16 reserv[6]; //:40

	//uint16 JournalClear;
	//uint16 StopTime;
	//uint16 reserv[11]; //:40
    }__attribute__((packed)) LOGICACFG;
typedef union
    {
	uint8 directbytes[32];
	struct
	    {
		char addr[8];
		char pass[8];
		char param[8];
		uint16 locate;
		uint16 reserv[3];
	    }__attribute__((packed));
    }__attribute__((packed)) IEC61107GFG;

//=========================================================
//======= Flash Memory ====================================
typedef union
    {
	uint16 dwords[DEVICE_CFG_WORDS]; // direct words
	struct
	    {
		GPRSCFG gprs;//0x8000-0x80FF
		uint16  GSMtime;//8080
		uint16  typeadapt;
		uint16  reservXtra1[0x8090 - 0x8082];
		uint16  MeterType;//8090
		uint32  MeterNum;//8091
		uint16  reservXtra[0x8200 - 0x8093];
		/*uint16 reservXtra[0x8200 - 0x8080];*/

		LOGICACFG logica;//0x8200-0x8240
		USARTCFG usart0;
		USARTCFG usart1;
		USARTCFG usart2;
		uint16 reserv[0x4A];
		IEC61107GFG iec61107req[32];//0x200
		}__attribute__((packed));
    }__attribute__((packed)) DEVICEGFG;
typedef union
    {
	uint16 dwords[DEVICE_CONST_WORDS]; // direct words
	struct
	    {
/*#ifdef LIDACONF
		struct
		    {
			TIMERANGE Shedule[12][32];
			DATERANGE EconomyDate; //0x0C words:AA
		    }__attribute__((packed)) Graph[3];
		DATERANGE Schedule4; //0x02 words
#else*/
		struct
			{
			TIMERANGE Shedule[12][32];
			DATERANGE EconomyDate; //0x0C words:AA
			}__attribute__((packed)) Graph[4];
		DATERANGE Schedule4; //0x02 words
//#endif
		uint16 reserv3[0x1F8];
	    }__attribute__((packed));

    }__attribute__((packed)) DEVICECONST;

typedef union
    {
	uint16 dwords[DEVICE_PROG_WORDS]; // direct words
	struct
	    {
		uint16 head[128];
		uint16 programm[DEVICE_PROG_WORDS - 128];
	    }__attribute__((packed));
    }__attribute__((packed)) DEVICEPROG;
typedef union
    {
	uint16 dwords[DEVICE_FLASHMEM_WORDS];
	struct
	    {
		DEVICEGFG devcfg;
		DEVICECONST devconst;
		DEVICEPROG devprog;
	    };
    }__attribute__((packed)) FLASHMEM;

//=========================================================
//======= Ram   Memory ====================================
typedef union
    {
	uint16 dwords[DEVICE_RAMMEM_WORDS];//9
	struct
	    {
		uint16 LocalCommand[2];
		uint16 OutputCommand[2];
		uint8 LogicErrorFlags;
		uint8 ChannelCondition;
		uint16 ErrorAndConditionModule[8];
		uint16 CrcDamage;
		uint16 UIPparams[17];
		uint16 CSQ;
		uint16 Refresh;
		uint16 reserv1[0x1FD - 0x20];
		uint16 CommonCommand[2];

		uint16 diskrets[4];
		uint16 relays[4];
		char strings[16][16]; // IEC61107GFG Answers
		uint16 reserv2[0x300 - 0x288];


		//diagnostic and control block
		uint16 DiagnDevice[2];/*11bit*/
		uint16 reset;

		uint16 ErrorFuseChannel;
		uint16 ErrorChannel;
		uint16 ErrorDiscretLogic[4];
		uint16 rtc_corr;
		int16 koeff_corr;
		uint16 t25_corr;
		uint16 soft_prsc;
		uint16 deccor;
		uint16 temperature;
		uint16 ErrorChannelE;
		uint16 ErrorChannelF;
		uint16 reserv3[DEVICE_RAMMEM_WORDS - 0x310];

	    //uint16 reserv;

	    }__attribute__((packed));
    }__attribute__((packed)) RAMMEM;

extern void MemInit();
extern void CopyDataBytes(uint8 *, uint8 *, uint16);
extern RAMMEM RAM;
extern FLASHMEM *pxConfig;

extern int DeviceID;

#endif
