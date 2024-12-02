/*
 * journal.h
 *
 *  Created on: 16.06.2010
 *      Author: nbohan
 */

#ifndef JOURNAL_H_
#define JOURNAL_H_
#include "typedef.h"
#include "error.h"

extern void itoa1(int n, char s[]);
extern int atoi1(char *c);
extern bool JrnlClear();
extern bool JrnlWrite(char *aMsg);
extern void JournalConf(int);
extern bool JrnlWriteOff(char *aMsg);

extern uint16 GetJrnlLength();
extern void GetDateString(char *aBuf);
extern void GetTimeString(char *aBuf);
extern void JournalConf(int confadr);


extern void CheckCUSignal();
extern void CheckCUDirect();
extern void CheckCUPower();
extern void CheckErrors();
extern void CheckPowerOn();

#endif /* JOURNAL_H_ */
