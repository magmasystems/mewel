/*===========================================================================*/
/*                                                                           */
/* File    : WINEVENT.H                                                      */
/*                                                                           */
/* Purpose : Header file for event-queue processing.                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

typedef struct _queue
{
  int  qhead,
       qtail;
  int  maxelements;
  int  nelements;
  int  element_width;
  PSTR qdata;
  unsigned long semQueueEmpty,
                semQueueFull,
                semQueueMutex;
} QUEUE;
extern QUEUE EventQueue;

#if defined(DOS) || defined(UNIX) || defined(VAXC)
#define DOSSEMSET(sem)         (*(sem)) = 1
#define DOSSEMCLEAR(sem)       (*(sem)) = 0
#define DOSSEMWAIT(sem,time)
#define DOSSEMREQUEST(sem,time)   DOSSEMSET(sem)
#endif

#ifdef OS2
#define DOSSEMSET(sem)          DosSemSet(sem)
#define DOSSEMCLEAR(sem)        DosSemClear(sem)
#define DOSSEMWAIT(sem,time)    DosSemWait(sem,time)
#define DOSSEMREQUEST(sem,time) DosSemRequest(sem,time)
#ifndef SEM_IMMEDIATE_RETURN
#define SEM_IMMEDIATE_RETURN  0L
#endif
#endif

extern QUEUE *FAR PASCAL InitQueue(QUEUE *);
extern VOID  FAR  PASCAL QueueAddData(QUEUE *,BYTE *);
extern INT   FAR  PASCAL QueueNextElement(QUEUE *,int);
extern INT   FAR  PASCAL QueuePrevElement(QUEUE *,int);
extern BYTE  *FAR PASCAL QueueGetData(QUEUE *,int);

/*
  Enable() / Disable() interrupts
*/
#if defined(DOS)
#if defined(__ZTC__)
#include <int.h>    /* interrupt package - for int_off() and int_on() */
#define enable()   int_on()
#define disable()  int_off()
#elif defined(MSC)
#define disable()  _disable()
#define enable()   _enable()
#endif

#else     /* !DOS */
#define disable()
#define enable()
#endif

#if defined(__DPMI32__) && !defined(__NTCONSOLE__)
#include "wincon.h"     /* make sure we include the local version */
#define __NTCONSOLE__

typedef struct tagNTConsoleInfo
{
  HANDLE hStdInput;
  HANDLE hStdOutput;
  DWORD  dwConsoleMode;
} NTCONSOLEINFO;

extern NTCONSOLEINFO NTConsoleInfo;
#endif

