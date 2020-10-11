/********************************************************************/
/* BREAK.C - handles the settings of the CTRL-BREAK flag            */
/*    (these routines should be present in Lattice already)         */
/*                                                                  */
/* Copyright (C) 1986  Marc Adler    All Rights Reserved            */
/********************************************************************/
#define INCLUDE_OS2
#define INCL_DOS
#define INCL_DOSSIGNALS

#include "wprivate.h"
#include "window.h"

#ifdef DOS286X
typedef void (_interrupt _far *PIHANDLER)(REGS16 regs);
USHORT APIENTRY DosSetRealProtVec(USHORT int_no, PIHANDLER prot_new_ptr,
		                  REALPTR real_new_ptr, 
			          PIHANDLER _far *prot_old_ptrp,
			          REALPTR _far *real_old_ptrp);
USHORT APIENTRY DosSetPassToProtVec(USHORT int_no, PIHANDLER prot_new_ptr,
			            PIHANDLER _far *prot_old_ptrp,
			            REALPTR _far *real_old_ptrp);

PIHANDLER OldInt1BProt;
REALPTR   OldInt1BReal;
PIHANDLER OldInt23Prot;
REALPTR   OldInt23Real;
#endif

int  FAR PASCAL getbrk(void);
void FAR PASCAL rstbrk(void);
void FAR PASCAL setbrk(void);

volatile char BrkFound = 0;


/* Getbrk - returns the current CTRL-BREAK setting */
int FAR PASCAL getbrk()
{
#ifdef DOS
  union REGS r;

  r.x.ax = 0x3300;
  intdos(&r, &r);
  return((int) r.h.dl);
#else
  return 0;
#endif
}

/* Rstbrk - disable CTRL-BREAK */
void FAR PASCAL rstbrk()
{
#ifdef DOS
  union REGS r;

  r.x.ax = 0x3301;
  r.h.dl = 0;
  intdos(&r, &r);
#endif
}

/* Setbrk - enable CTRL-BREAK */
void FAR PASCAL setbrk()
{
#ifdef DOS
  union REGS r;

  r.x.ax = 0x3301;
  r.h.dl = 1;
  intdos(&r, &r);
#endif
}

#ifdef DOS

#ifdef __TURBOC__
#if !defined(_disable)
#define _disable()              disable()
#define _enable()               enable()
#endif
#define _chain_intr(f)          ((*f)())
#define _dos_getvect(i)         getvect(i)
#define _dos_setvect(i, f)      setvect(i, f)
#endif

#ifdef ZORTECH
#include <int.h>
extern int cdecl FAR CtrlBreakHandler(struct INT_DATA *pd);
#else /* !ZORTECH */

#if defined(MSC)
void interrupt FAR CDECL CtrlBreakHandler();
#else
void interrupt FAR       CtrlBreakHandler();
#endif

#if defined(MSC) && !defined(__TSC__)
(interrupt FAR CDECL *OldInt1B)();
(interrupt FAR CDECL *OldInt23)();
#endif

#if defined(__TURBOC__)
void interrupt (*OldInt1B)();
void interrupt (*OldInt23)();
#endif

#if defined(__WATCOMC__)
#include <conio.h>         /* for inp() */
void (interrupt FAR *OldInt1B)();
void (interrupt FAR *OldInt23)();
#endif

#if defined(__TSC__)
void (interrupt *OldInt1B)();
void (interrupt *OldInt23)();
#endif

#endif /* ZORTECH */



VOID FAR CDECL int23ini(LPSTR pBrkSemaphore)
{
  (void) pBrkSemaphore;

#if defined(__TSC__)
  return;
#endif

#if defined(ZORTECH)
  int_intercept(0x23, CtrlBreakHandler, 512);
  int_intercept(0x1B, CtrlBreakHandler, 512);
#elif defined (DOS286X)
  DosSetPassToProtVec(0x1B, CtrlBreakHandler, &OldInt1BProt, &OldInt1BReal);
  DosSetPassToProtVec(0x23, CtrlBreakHandler, &OldInt23Prot, &OldInt23Real);
#else
  OldInt23 = _dos_getvect(0x23);
  OldInt1B = _dos_getvect(0x1B);
  _disable();
  _dos_setvect(0x23, CtrlBreakHandler);
  _dos_setvect(0x1B, CtrlBreakHandler);
  _enable();
#endif
}

VOID FAR CDECL int23res(void)
{
#if defined(__TSC__)
  return;
#endif

#if defined(ZORTECH)
  int_restore(0x23);
  int_restore(0x1B);
#elif defined(DOS286X)
  DosSetRealProtVec(0x1B, OldInt1BProt, OldInt1BReal, NULL, NULL);
  DosSetRealProtVec(0x23, OldInt23Prot, OldInt23Real, NULL, NULL);
#else
  _disable();
  _dos_setvect(0x23, OldInt23);
  _dos_setvect(0x1B, OldInt1B);
  _enable();
#endif
}


#if defined(ZORTECH)
int cdecl FAR CtrlBreakHandler(struct INT_DATA *pd)
#endif /* */
#if defined(MSC) && !defined(ZORTECH) && !defined(__TSC__)
void interrupt cdecl FAR CtrlBreakHandler(es,ds,di,si,bp,sp,bx,dx,cx,ax,ip,cs,flags)
  unsigned es, ds, di, si, bp, sp, bx, dx, cx, ax, ip, cs, flags;
#endif
#if defined(__TURBOC__) || defined(__TSC__)
void interrupt CtrlBreakHandler(unsigned int bp,unsigned int di,unsigned int si,
               unsigned int ds,unsigned int es,unsigned int dx,unsigned int cx,
               unsigned int bx,unsigned int ax,unsigned int ip,unsigned int cs,
               unsigned int flags)
#endif
#if defined(__WATCOMC__)
void interrupt CtrlBreakHandler(union INTPACK r)
#endif
{
#if (defined(MSC) && !defined(ZORTECH) && !defined(__TSC__)) || defined(__TURBOC__)
  (void) es;  (void) ds;  (void) si;  (void) di;  (void) bp;
  (void) bx;  (void) dx;  (void) cx;  (void) ax;  (void) ip;  (void) cs;
  (void) flags;
#if !defined(__TURBOC__)
  (void) sp;
#endif
#endif

  BrkFound = 1;
#if defined(__WATCOMC__)
  (void) r.x.eax;
#endif
#if defined(ZORTECH)
  return 1;
#endif
}

#endif /* DOS */



#ifdef OS2

PFNSIGHANDLER Old_CTRLC_Handler;
PFNSIGHANDLER Old_CTRLBREAK_Handler;
/*
  Note - the IBM OS/2 SDK doesn't define the FAR PASCAL keyword for a sig handler
*/
extern void FAR PASCAL OS2_MEinterrupt();
USHORT        Prev_CTRLC_Action;
USHORT        Prev_CTRLBREAK_Action;

VOID FAR CDECL int23ini(LPSTR pBrkSemaphore)
{
  char   is_OS2;

  (void) pBrkSemaphore;

  DosGetMachineMode(&is_OS2);

  DosSetSigHandler(OS2_MEinterrupt, &Old_CTRLC_Handler,
                    (PUSHORT) &Prev_CTRLC_Action,
                    SIGA_ACCEPT, SIG_CTRLC);
  if (is_OS2)
    DosSetSigHandler(OS2_MEinterrupt, &Old_CTRLBREAK_Handler,
                      (PUSHORT) &Prev_CTRLBREAK_Action,
                      SIGA_ACCEPT, SIG_CTRLBREAK);
}

void cdecl int23res()
{
  PFNSIGHANDLER foo;
  USHORT baz;

  DosSetSigHandler(Old_CTRLC_Handler, &foo,
                       (PUSHORT) &baz,
                   Prev_CTRLC_Action, SIGA_IGNORE);
  DosSetSigHandler(Old_CTRLBREAK_Handler, &foo,
           &baz, Prev_CTRLBREAK_Action, SIGA_ACKNOWLEDGE);
}


/*
  Note - the IBM OS/2 SDK doesn't define the FAR PASCAL keyword for a sig handler
*/
void FAR PASCAL OS2_MEinterrupt(sigarg, signum)
  unsigned sigarg, signum;
{
  PFNSIGHANDLER foo;
  USHORT   baz;
  extern   FAR PASCAL MEinterrupt();
  char     is_OS2;

  DosGetMachineMode(&is_OS2);

  DosSetSigHandler((PFNSIGHANDLER) 0L,  &foo,
                            (PUSHORT) &baz,
                            SIGA_ACKNOWLEDGE, signum);
  if (!is_OS2)
  {
    DosSetSigHandler(OS2_MEinterrupt, &foo,
                     (PUSHORT) &baz, SIGA_ACCEPT, SIG_CTRLC);
    DosSetSigHandler(OS2_MEinterrupt, &foo,
                     (PUSHORT) &baz, SIGA_ACCEPT, SIG_CTRLBREAK);
  }
/*MEinterrupt(0);*/
}

#endif /* OS2 */


#if defined(UNIX)||defined(VAXC)
#ifdef aix
#include "signal.h"  /* ads - must copy and modify signal.h */
#else
#include <signal.h>
#endif
extern MEWELinterrupt();

VOID FAR CDECL int23ini(LPSTR pBrkSemaphore)
{
  extern UnixMEWELinterrupt();

  (void) pBrkSemaphore;

  signal(SIGINT,  UnixMEWELinterrupt);
  signal(SIGTERM, UnixMEWELinterrupt);
}

VOID FAR CDECL int23res(void)
{
  signal(SIGINT,  SIG_DFL);
  signal(SIGTERM, SIG_DFL);
}

UnixMEWELinterrupt(sig)
  int sig;
{
  signal(sig, UnixMEWELinterrupt);
  return MEWELinterrupt(0);
}

MEWELinterrupt(code)
  int code;
{
#if 0
  if (MessageBox("Interrupt received!", "Continue application?", NULL,
                 "Warning", MB_YESNO) == 7 /* IDNO */)
  {
    PostQuitMessage(code);
  }
#endif
  return 0;
}

#endif

