#ifndef MODEM_H
#define MODEM_H
#include "typedef.h"
typedef enum
{
	MA_UNKNOWN = 0,
	MA_CALLREADY,
	MA_OK,
	MA_SERVEROK,
	MA_NORMALPOWERDOWN,
	MA_STATEIPSTATUS,
	MA_STATEIPCLOSE,
	MA_STATEIPINITIAL,
	MA_STATEIPGPRSACT,
	MA_STATEPDPDEACT,
	MA_CIPSENDREADY,
	MA_CONNECT,
	MA_NOCARRIER,
	MA_ERROR,
	MA_READY,
} MDEM_ANSWER;
#define AT_AMOUNT (MA_ERROR - 1)


typedef struct
{
	char *str;
	MDEM_ANSWER ma;
} AT_COMMAND;

#define PPP_USER                " "
#define PPP_PASS                " "
#define PPP_ADPARAM             "mingorsvet.velcom.by"
//#define PPP_USER              "bemn_7650654"
//#define PPP_PASS              "7650654"
//#define PPP_ADDSTR              "AT+CGDCONT=1,\"IP\",\"%s\"\r"
//#define PPP_CSTT 				"AT+QIREGAPP=\"%s\",\"%s\",\"%s\"\r"

//#define PPP_USER                ""
//#define PPP_PASS                ""
//#define PPP_ADDSTR              "AT+CGDCONT=1,\"IP\",\"lidazhkh.velcom.by\"\r"
extern int numRD;

extern void ModemInit();
extern int ModemReceiveData(uint8 *,int);
extern void ModemSendData(uint8 *, uint8, int);
extern void ModemWrite(char *);
extern bool WaitClose(int);
extern bool WaitCloseServer();
extern bool CSQ();

extern bool LedNoModem_Journal,LedSIM_Journal,LedAPN_Journal;
extern uint32 GprsIdleMSec;

#endif
