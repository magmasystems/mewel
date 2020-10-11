/*===========================================================================*/
/*                                                                           */
/* File    : WINTIME.C                                                       */
/*                                                                           */
/* Purpose : Routines to retrieve the DOS and system (BIOS) times            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_DOSINFOSEG

#include "wprivate.h"
#include "window.h"

#if defined(UNIX)
#include <sys/types.h>
#if defined(sco)
#include <sys/timeb.h>
#endif
#if defined(sunos)
#include <sys/time.h>
#endif
#include <time.h>
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#endif
#endif /* UNIX */

#if defined(DOS) || defined(OS2) || defined(VAXC)
#include <time.h>
#endif

#define BIOSTODMAX  1573039L

extern int           FAR PASCAL DosGetTime(int *,int *,int *);
extern unsigned long FAR PASCAL BIOSGetTime(void);
extern unsigned long FAR PASCAL BIOSSetTime(int);


int FAR PASCAL DosGetTime(h, m, s)                     /* get DOS system time */
  int *h, *m, *s;
{
#ifdef OS2
  DATETIME dt;
  DosGetDateTime(&dt);
  *h = dt.hours;
  *m = dt.minutes;
  *s = dt.seconds;
#endif

#if defined(DOS) && !defined(MEWEL_32BITS)
  union REGS r;
  r.x.ax = 0x2C00;
  intdos(&r, &r);
  *h = r.x.cx >> 8;     /* Hours */
  *m = r.x.cx & 0x00FF; /* Minutes */
  *s = r.x.dx >> 8;     /* Seconds */
#endif

#if defined(UNIX) || defined(VAXC) || defined(MEWEL_32BITS)

#if defined(UNIX)
  long now;
#else
  time_t now;
#endif

  struct tm *tm;

  time(&now);
  tm = localtime(&now);
  *h = tm->tm_hour;
  *m = tm->tm_min;
  *s = tm->tm_sec;
#endif

  return 1;
}


/* BIOSGetTime - return the current PC time */
unsigned long FAR PASCAL BIOSGetTime(void)
{
#ifdef OS2
  PGINFOSEG g;
  PLINFOSEG l;
  SEL gdt, ldt;

  DosGetInfoSeg((PSEL) &gdt, (PSEL) &ldt);
  g = MAKEPGINFOSEG(gdt);
  return g->msecs;
#endif


#ifdef DOS
#if defined(__DPMI32__)
  _AX = 0;
  geninterrupt(0x1A);
  return ((unsigned long) _CX << 16) | _DX;
#elif defined(__HIGHC__) || (defined(PL386) && defined(__WATCOMC__))
  FARADDR biostod = GetBIOSAddress(0x6C);
  return * (unsigned long TRUEFAR *) biostod;
#elif defined(WC386) || defined(__DPMI16__)
  FARADDR biostod = GetBIOSAddress(0x6C);
  return * (unsigned long far *) biostod;
#elif !defined(EXTENDED_DOS)
  /* Set up a pointer into BIOS Data Area */
  volatile unsigned long far *biostod = (unsigned long far *) MK_FP(0x40,0x6C);
  return *biostod;
#else
  /* * This can kill the BIOS TOD Rollover flag -- Do not use!! */
  union REGS in, out;
  in.h.ah = 0;
  INT86(0x1A, &in, &out);
  return ((unsigned long) out.x.cx << 16) | out.x.dx;
#endif
#endif


#ifdef UNIX
  /* Return time in milliseconds. Relative to program start to avoid overflow */
#ifdef sunos
  struct timeval tv;
  static struct timezone tz = { 0, 0 };
#endif /* sunos */
  static long start = 0;

  if (start == 0)
    (void) time(&start);
#ifdef sunos
  gettimeofday(&tv, &tz);
  return (tv.tv_sec - start) * 1000 + tv.tv_usec / 1000;
#else
  return (time((long *) 0) - start) * 1000;
#endif /* sunos */
#endif /* UNIX */


#ifdef VAXC
  /* Return time in milliseconds. Relative to program start to avoid overflow */
  static time_t start = 0;
  struct timeb tv;

  if (start == 0)
    (void) time(&start);
  (void) ftime(&tv);              /* ftime should be obsolete by gettimeofday */
  return (tv.time - start) * 1000 + tv.millitm;
#endif /* VAXC */
}


/* BIOSSetTime - returns the PC time n secs from now */
unsigned long FAR PASCAL BIOSSetTime(secs)
  int  secs;    /* the amount of time to wait for */
{
#if defined(UNIX)||defined(VAXC)
  return BIOSGetTime() + secs;

#else
  unsigned long  finaltime = 0L, currtime;

  /* Currtime is used to get the # of clock ticks (18.2 * 10) */
  for (currtime = 182;  secs;  currtime <<= 1)
  {
    if (secs & 1) finaltime += currtime;
    secs >>= 1;
  }

  /* Round up to the nearest increment of clock ticks */
  finaltime = (finaltime + 5) / 10;
  finaltime += BIOSGetTime();
  if (finaltime > BIOSTODMAX)
      finaltime -= BIOSTODMAX;
  return finaltime;
#endif
}


#if !defined(__DPMI32__)
DWORD FAR PASCAL GetTickCount(void)
{
  return GetCurrentTime();
}
#endif

DWORD FAR PASCAL GetCurrentTime(void)
{
  /*
    ms = ticks / 18.2 * 1000
       = tick*10 / 182 * 1000
  */
  DWORD dwTicks = clock();

#if defined(__BORLANDC__)
#if (__BORLANDC__ >= 0x400)
  /*
    clock() returns milliseconds in BC++ 4.0, but CLK_TICK is defined
    to be 1000.0. See comment below about floating pt library.
  */
  return dwTicks;
#else
  /*
    We don't want to drag in the BC floating pt lib, so instead of
    using CLK_TCK (which is defined to be 18.2), use 55ms.
  */
  return dwTicks * 55L;
#endif

#elif defined(__HIGHC__) || defined(__GNUC__) || defined(sunos)
  return (dwTicks / CLOCKS_PER_SEC) * 1000L;

#else
  return (dwTicks * 1000L) / CLK_TCK;
#endif
}

DWORD WINAPI GetTimerResolution(void)  /* USER.14 */
{
  /*
    Returns the number of microseconds in one timer tick. The PC timer
    gives off a tick every 54.945 millisecond.
  */
  return 54945L;
}

