#ifndef RTCLOCK_H
#define RTCLOCK_H
#include "typedef.h"

typedef struct
{
  uint16 Years;
  uint16 Month;
  uint16 Data;
  uint16 Day;
  uint16 Hour;
  uint16 Min;
  uint16 Sec;
  uint16 MSec;

}__attribute__((packed)) DATATIME;


extern unsigned char month_day_table[];


extern void IncrementRTC(void);
extern void UpdateTime(void);

extern void rtcGetDataTime(DATATIME *dt);
extern void rtcGetLocalDataTime(DATATIME *dt);
extern void rtcSetDataTime(DATATIME *dt);
extern void rtcSetLocalDataTime(DATATIME *dt);
extern void rtcIncrementSoftClockMs(void);


extern void rtcGetiFixDateTime(char *dt);
extern void rtcSetiFixDateTime(char *dt);

extern void GetDateStringLM(char *buf);
extern void GetDateStringLD(char *buf);

extern bool ResetTime;
extern bool ResetTimeEnd;
extern bool JrnlTime;
#endif

