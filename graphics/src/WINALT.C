/*===========================================================================*/
/*                                                                           */
/* File    : WINALT.C                                                        */
/*                                                                           */
/* Purpose : This is a simple front-end to the low-level int 9H keyboard     */
/*           handler. It's purpose is to detect if the ALT key was tapped    */
/*           in order to set the focus to the menubar. If the ALT key was    */
/*           tapped without any intervening keys, then the variable          */
/*           'nAltKeyPresses' will be incremented. MEWEL checks this         */
/*           variable in its event loop, and if an ALT key has been tapped,  */
/*           a WM_ALT message is generated.                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/*#define TEST*/

#include "wprivate.h"
#include "window.h"

#if defined(MSC) || defined(__WATCOMC__)
#include <conio.h>  /* for inp() */
#endif

#ifdef __TURBOC__
#if !defined(_disable)
#define _disable()              disable()
#define _enable()               enable()
#endif
#define _chain_intr(f)          ((*f)())
/*
#define inp(port)               inportb(port)
#define outp(port, c)           outportb(port, c)
*/
#define _dos_getvect(i)         getvect(i)
#define _dos_setvect(i, f)      setvect(i, f)
#endif

#ifdef DOS286X
#ifdef _dos_setvect
#undef _dos_setvect
#endif
#define _dos_setvect(i, f)
#endif


void Int9Init(void);
void Int9Terminate(void);

#ifdef ZORTECH
#include <int.h>
extern int cdecl FAR NewInt9(struct INT_DATA *pd);
#ifdef TEST
#include <disp.h>
#define cprintf disp_printf
#endif
#else /* */

#if defined(__WATCOMC__) && defined(__386__)
void __interrupt __far NewInt9();
#elif defined(MSC)
void interrupt FAR CDECL NewInt9();
#else
void interrupt FAR       NewInt9();
#endif

#if defined(MSC) && !defined(__TSC__)
(interrupt FAR CDECL *OldInt9)();
#endif
#if defined(__TURBOC__)
void interrupt (*OldInt9)();
#endif
#if defined(__WATCOMC__)
void (__interrupt __far *OldInt9)();
#endif
#if defined(__TSC__)
void (interrupt *OldInt9)();
#endif
#endif /* */


#ifdef TEST
static int FAR Int9Done = 0;    /* This must be declared FAR since, when it's
                                   accessed in the interrupt routine, the
                                   DS reg is not the same as our data segment.
                                */
#endif


#define SC_ALT      56          /* scan code for ALT key */
#define TOGGLE_SHIFT_MASK (INS_SHIFT | CAP_SHIFT | SCL_SHIFT | NUM_SHIFT)

#define BIOS_KBD_SHIFTSTATE (* (char  FAR *) 0x0000417)
#define BIOS_KBD_STARTQUEUE (* (short FAR *) 0x000041A)
#define BIOS_KBD_ENDQUEUE   (* (short FAR *) 0x000041C)

struct
{
  char bAltPressed;
  char bInterveningKeys;
  int  pStartQueue,
       pEndQueue;
} AltInfo;

char MEWEL_NoInt9 = 0;
static char bDidInt9 = FALSE;


/*
  Int9Init()
    Saves the old Int 9H address and sets the new one
*/
void Int9Init(void)
{
#if defined(EXTENDED_DOS) || defined(__ZTC__)
  MEWEL_NoInt9 = 1;
#endif

  /*
    If the app doesn't want to intercept int 9, or if the interrupt handler
    is already installed, then just return.
  */
  if (MEWEL_NoInt9 || bDidInt9)
    return;

  /*
    Hook the interrupt
  */
#ifdef ZORTECH
  int_intercept(0x09, NewInt9, 512);
#else /* */
  OldInt9 = _dos_getvect(0x09);
  _disable();
  _dos_setvect(0x09, NewInt9);
  _enable();
#endif /* */

  /*
    Initialize the AltInfo array and tell MEWEL that we got here already.
  */
  AltInfo.bAltPressed = 0;
  AltInfo.bInterveningKeys = 0;
  bDidInt9++;
}


/*
  Int9Terminate()
    Restores the original Int 9H address
*/
void Int9Terminate(void)
{
  /*
    Make sure that we hooked the interrupt
  */
  if (MEWEL_NoInt9 || !bDidInt9)
    return;

  /*
    Restore the old INT 9 handler
  */
#ifdef ZORTECH
  int_restore(0x09);
#else /* */
  _disable();
  _dos_setvect(0x09, OldInt9);
  _enable();
#endif /* */
}


#ifdef TEST
static int ScanCodeTable[] =
{
  -1
};
#endif

#if defined(ZORTECH)
int cdecl FAR NewInt9(struct INT_DATA *pd)
#endif /* */
#if defined(MSC) && !defined(ZORTECH) && !defined(__TSC__)
void interrupt cdecl FAR NewInt9(es,ds,di,si,bp,sp,bx,dx,cx,ax,ip,cs,flags)
  unsigned es, ds, di, si, bp, sp, bx, dx, cx, ax, ip, cs, flags;
#endif
#if defined(__TURBOC__) || defined(__TSC__)
void interrupt NewInt9(unsigned int bp,unsigned int di,unsigned int si,
               unsigned int ds,unsigned int es,unsigned int dx,unsigned int cx,
               unsigned int bx,unsigned int ax,unsigned int ip,unsigned int cs,
               unsigned int flags)
#endif
#if defined(__WATCOMC__)
void __interrupt __far NewInt9(union INTPACK reg)
#endif
{
  unsigned c, shift;
#ifdef TEST
  unsigned i;
  union REGS r;
#endif


#if defined(__WATCOMC__)
  (void) reg.x.eax;
#endif
#if (defined(MSC) && !defined(ZORTECH) && !defined(__TSC__)) || defined(__TURBOC__)
  (void) es;  (void) ds;  (void) si;  (void) di;  (void) bp;
  (void) bx;  (void) dx;  (void) cx;  (void) ax;  (void) ip;  (void) cs;
  (void) flags;
#if !defined(__TURBOC__)
  (void) sp;
#endif
#endif

  /* Get the scan code into c */
  c = inp(0x60);

  /*
    If it's an extended key (scan code is 224), ignore it
  */
  if (c == 0xE0)
    goto bye;

  /*
    Check if we have the ALT key.
  */
  if ((c & 0x7F) == SC_ALT)
  {
    if (c & 0x80)
    {
      /*
        The ALT key was released
      */
      shift = BIOS_KBD_SHIFTSTATE;
      if (AltInfo.bAltPressed == 1 && AltInfo.bInterveningKeys == 0 &&
          (shift & ~TOGGLE_SHIFT_MASK) == ALT_SHIFT)
      {
#ifdef TEST
        cprintf("ALT KEY PRESSED and RELEASED\r\n",1, 2);
#else
        SysEventInfo.nAltKeyPresses++;
#endif
      }
      AltInfo.bAltPressed = 0;
    }
    else
    {
      /*
        The ALT key was pressed
      */
      if (AltInfo.bAltPressed == 0)
      {
        AltInfo.pStartQueue = BIOS_KBD_STARTQUEUE;
        AltInfo.pEndQueue   = BIOS_KBD_ENDQUEUE;
        AltInfo.bInterveningKeys = 0;
      }
      AltInfo.bAltPressed++;
    }
  }
  else
    AltInfo.bInterveningKeys = 1;

  if (c & 0x80)   /* a released key --- can it! */
    goto bye;

#ifdef FILTER
  /* Check to see if it's a scan code that we're interested in */
  for (k = ScanCodeTable;  *k != -1;  k++)  ;
  if (*k == -1)
    goto bye;
#endif

#ifdef TEST
  /* If it's still a key that we are interested in, check the shift status */
  r.h.ah = 0x02;
  int86(0x16, &r, &r);
  shift = r.h.al;
#endif

#ifdef HANDLE_IT
/*
  If we want to handle the interrupt ourselves, execute the following code
*/
  outp(0x61, (i = inp(0x61)) | 0x80);  /* set the ack bit & send to the 8255 */
  outp(0x61, i & 0x7F);                /* clr the ack bit & send to the 8255 */
  outp(0x20, 0x20);                    /* send EOI to the 8259 int controllr */
#endif

#ifdef TEST
  cprintf("scan code [%d]  shift code [%x]\r\n", c, shift);
  if (c == 1)
    Int9Done++;
#endif
  
  /*
    Let the old Int 9H handle the action
  */
bye:
#ifdef ZORTECH
  return (0);
#else
  _chain_intr(OldInt9);
#endif /* */

}


#ifdef TEST
main()
{
#ifdef ZORTECH
  disp_open();
#endif /* */
  Int9Init();
  while (!Int9Done) 
    ;
  Int9Terminate();
#ifdef ZORTECH
  disp_close();
#endif /* */
}
#endif /* TEST */

