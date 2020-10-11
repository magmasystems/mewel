/*===========================================================================*/
/*                                                                           */
/*    10 Jun 96  RT   inserted #elif define(__DPMI16__) clauses              */
/*                       @ line 417 & line 462
/*                                                                           */
/*===========================================================================*/
/*===========================================================================*/
/*                                                                           */
/* File    : MOUINIT.C                                                       */
/*                                                                           */
/* Purpose :  Initialize the mouse                                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MOUSE
#define INCLUDE_CURSORS

#include "wprivate.h"
#include "window.h"

#if defined(__DPMI32__)
#include "winevent.h"
#endif


VOID FAR PASCAL MOUSECALL(p)
  MOUSEPARAMS *p;
{
#if defined(__NTCONSOLE__)
  TREGISTERS tr;
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = p->m1;
  tr.BX = p->m2;              /* BX must come last to avoid overwrite */
  tr.CX = p->m3;
  tr.DX = p->m4;
  RealIntr(0x33, &tr);
  p->m1 = tr.AX;
  p->m2 = tr.BX;
  p->m3 = tr.CX;
  p->m4 = tr.DX;
#elif defined(__DPMI16__)
  int foo;
  _AX = p->m1;
  _CX = p->m3;
  _DX = p->m4;
  _BX = p->m2;              /* BX must come last to avoid overwrite */
  geninterrupt(0x33);       /* call the mouse interrupt */
  foo   = _BX;              /* cause BX will be overwritten by LES BX,.. */
  p->m1 = _AX;
  p->m2 = foo;
  p->m3 = _CX;
  p->m4 = _DX;
#else
  union REGS r;
  r.x.ax = p->m1;
  r.x.bx = p->m2;
  r.x.cx = p->m3;
  r.x.dx = p->m4;
  INT86(0x33, &r, &r);      /* call the mouse interrupt */
  p->m1 = r.x.ax;
  p->m2 = r.x.bx;
  p->m3 = r.x.cx;
  p->m4 = r.x.dx;
#endif
}


/* Microsoft mouse function 0 */
int FAR PASCAL MOUSE_Init(bMouseTerminate) 
  BOOL bMouseTerminate;
{
  MOUSEPARAMS  p;
  unsigned     ver;
#if !defined(__NTCONSOLE__) && (defined(WAGNER_GRAPHICS) || defined(MEWEL_GUI))
  BYTE FAR *vp;
#endif

  (void) bMouseTerminate;

  ver = _osmajor;
#ifdef INSTANTC
  ver = 4;  /*DF LIMITATION OF INSTANTC */
#endif

  if (ver < 2)
	 return (0);                 /* need Dos 2.0 or higher to use these mouse
											* routines */
  memset((char *) &p, 0, sizeof(p));

  /*
	 Get around bug in Hercules
  */
#if !defined(__NTCONSOLE__) && (defined(WAGNER_GRAPHICS) || defined(MEWEL_GUI))
#ifdef WAGNER_GRAPHICS
  if (VID_IN_GRAPHICS_MODE() && !vidmode.mode)
#else
  if (VID_IN_GRAPHICS_MODE() && (VideoInfo.flags & 0x07) == MDA)  /* assume a hercules monitor */
#endif
  {
	 vp = (BYTE FAR *) 0x0400049L;
	 *vp = 6;
  }
  else
	 vp = NULL;
#endif

  if (ver >= 3)
	 MOUSECALL(&p);              /* status returned in tail->m1, if 0 then not
											* installed */
#if !defined(__NTCONSOLE__)
  else
  {                             /* its version 2 */
	 union  REGS  r;
	 struct SREGS segs;
	 r.h.ah = 0x35;              /* Function to get interrupt vector */
	 r.h.al = 0x33;              /* mouse interrupt number */
	 intdosx(&r, &r, &segs);
	 if (segs.es == 0 && r.x.bx == 0)
		p.m1 = 0;                 /* if vector points to 0000:0000, no mouse */
	 else
		MOUSECALL(&p);            /* initialize mouse */
  }
#endif

  /*
	 If there is a mouse, then tell it to allow mouse movement over the
	 entire screen.  Otherwise, the mouse won't be able to move beyond 80x25.
  */
  if (p.m1)
  {
	 int res;

	 res = p.m1;
	 memset((char *) &p, 0, sizeof(p));
	 p.m1 = 0x07;
#ifdef MEWEL_GUI
	 p.m4 = VideoInfo.width * ((VID_IN_GRAPHICS_MODE()) ? 1 : 8) - 1;
#else
	 p.m4 = VideoInfo.width * ((VID_IN_GRAPHICS_MODE()) ? FONT_WIDTH : 8) - 1;
#endif
	 MOUSECALL(&p);
	 memset((char *) &p, 0, sizeof(p));
	 p.m1 = 0x08;
#ifdef MEWEL_GUI
	p.m4 = VideoInfo.length * ((VID_IN_GRAPHICS_MODE()) ? 1 : 8) - 1;
#else
	 p.m4 = VideoInfo.length * ((VID_IN_GRAPHICS_MODE()) ? VideoInfo.yFontHeight : 8) - 1;
#endif
	 MOUSECALL(&p);
	 p.m1 = res;
  }

#if !defined(__NTCONSOLE__) && (defined(WAGNER_GRAPHICS) || defined(MEWEL_GUI))
 if (vp)
	*vp = 7;
#endif

  return p.m1;
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUPRESS.C                                                      */
/*                                                                           */
/* Purpose : Function to retrieve the button-press status of the mouse       */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_GetPress(btn, bnow, bcount, x, y)
  int  btn, 
		 *bnow,
		 *bcount,
		 *x,
		 *y;
{
  MOUSEPARAMS p;

  p.m1 = 5;
  p.m2 = btn;
  MOUSECALL(&p);
  *bnow = p.m1;             /* status of button now */
  *bcount = p.m2;           /* number of times pressed since last call to
									  * function */
  *x = p.m3;                /* current horizontal position */
  *y = p.m4;                /* current vertical position */
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUSTAT.C                                                       */
/*                                                                           */
/* Purpose : Function to retrieve the mouse status                           */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_GetStatus(button, curx, cury)
  int     *button;
  MWCOORD *curx, *cury;
{
#if defined(__NTCONSOLE__)
#if defined(MEWEL_GUI) || defined(MEWEL_TEXT)
  TREGISTERS tr;

  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 3;
  RealIntr(0x33, &tr);
  *button = tr.BX;
  *curx   = tr.CX;
  *cury   = tr.DX;

#else
  INPUT_RECORD eventBuf;
  DWORD        dwPendingEvent;

  static MWCOORD dwLastX = 0;
  static MWCOORD dwLastY = 0;
  static DWORD   dwLastButton = 0L;

  GetNumberOfConsoleInputEvents(NTConsoleInfo.hStdInput, &dwPendingEvent);
  if (dwPendingEvent == 0)
  {
copy_last_values:
	 *button = (int) dwLastButton;
	 *curx = dwLastX;
	 *cury = dwLastY;
	 return;
  }

  PeekConsoleInput(NTConsoleInfo.hStdInput, &eventBuf, 1, &dwPendingEvent);
  if (eventBuf.EventType == MOUSE_EVENT)
  {
	 ReadConsoleInput(NTConsoleInfo.hStdInput, &eventBuf, 1, &dwPendingEvent);
	 dwLastX = (MWCOORD) eventBuf.Event.MouseEvent.dwMousePosition.X;
	 dwLastY = (MWCOORD) eventBuf.Event.MouseEvent.dwMousePosition.Y;

	 /*
		Translate NT mouse button codes to MEWEL codes
	 */
	 dwLastButton = (eventBuf.Event.MouseEvent.dwButtonState & 
								(FROM_LEFT_1ST_BUTTON_PRESSED | 
								 RIGHTMOST_BUTTON_PRESSED     | 
								 FROM_LEFT_2ND_BUTTON_PRESSED));
  }
  goto copy_last_values;
#endif

#elif defined(__DPMI16__)
  int foo;

  _AX = 3;
  geninterrupt(0x33);       /* call the mouse interrupt */
  foo = _BX;                /* avoid BX overwrite ... LES BX,[bp+0E] */
  *button = foo;
  *curx   = _CX;
  *cury   = _DX;
#else
  union REGS r;
  r.x.ax = 3;
  INT86(0x33, &r, &r);      /* call the mouse interrupt */
  *button = r.x.bx;
  *curx   = r.x.cx;
  *cury   = r.x.dx;
#endif
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUSTPOS.C                                                      */
/*                                                                           */
/* Purpose : Function to set the position of the mouse                       */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_SetPos(x, y)
  int  x, y;
{
  MOUSEPARAMS p;

  p.m1 = 4;
  p.m3 = x;
  p.m4 = y;
  MOUSECALL(&p);
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUPEN.C                                                        */
/*                                                                           */
/* Purpose : Function to interface the mouse with the light pen              */
/*                                                                           */
/*===========================================================================*/
#ifdef NOTUSED
VOID FAR PASCAL MOUSE_LightPen(sw)
  int  sw;
{
  MOUSEPARAMS p;

  p.m1 = (sw ? 13 : 14);
  MOUSECALL(&p);
}
#endif

/*===========================================================================*/
/*                                                                           */
/* File    : MOUSPEED.C                                                      */
/*                                                                           */
/* Purpose : Function to set the speed of the mouse.                         */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_SetSpeed(x, y)
  int  x, y;
{
  MOUSEPARAMS p;

  p.m1 = 15;                /* function # */
  p.m3 = x;                 /* horizontal mickey/pixel ratio */
  p.m4 = y;                 /* vertical mickey/pixel ratio   */
  MOUSECALL(&p);
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUHIDE.C                                                       */
/*                                                                           */
/* Purpose : Function to hide the mouse cursor                               */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL MOUSE_HideCursor(void)          /* Msoft mouse function 2 */
{
#if defined(__NTCONSOLE__)
#if defined(MEWEL_GUI) || defined(MEWEL_TEXT)
  TREGISTERS tr;
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 2;
  RealIntr(0x33, &tr);
#else
  SetConsoleMode(NTConsoleInfo.hStdInput, 
					  NTConsoleInfo.dwConsoleMode & ~ENABLE_MOUSE_INPUT);
#endif

#else
  MOUSEPARAMS p;
  p.m1 = 2;
  MOUSECALL(&p);
#endif

  return TRUE;
}

/*===========================================================================*/
/*                                                                           */
/* File    :  MOUGTREL.C                                                     */
/*                                                                           */
/* Purpose :  Gets the button-release status of the mouse                    */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_GetRelease(btn, bnow, bcount, x, y)
  int  btn, *bnow, *bcount, *x, *y;
{
  MOUSEPARAMS p;

  p.m1 = 6;                 /* mouse call #6 */
  p.m2 = btn;               /* which button to check, 0=left,1=right */
  MOUSECALL(&p);
  *bnow = p.m1;             /* status of button now */
  *bcount = p.m2;           /* number of button releases since last call      */
  *x = p.m3;                /* horizontal position at last button release     */
  *y = p.m4;                /* vertical position at last release              */
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUGETMO.C                                                      */
/*                                                                           */
/* Purpose : Gets the motion of the mouse since the last call                */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_GetMotion(x, y)
  int  *x, *y;
{
  MOUSEPARAMS p;

  p.m1 = 11;                /* mouse function #11 */
  MOUSECALL(&p);
  *x = p.m3;                /* horizontal distance since last call */
  *y = p.m4;                /* vertical distance since last call   */
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUTHRES.C                                                      */
/*                                                                           */
/* Purpose : Sets the mouse threshhold speed                                 */
/*                                                                           */
/*===========================================================================*/
#ifdef NOTUSED
VOID FAR PASCAL MOUSE_SetThreshold(x)
  int  x;
{
  MOUSEPARAMS p;

  p.m1 = 19;                /* function 19 */
  p.m4 = x;                 /* speed threshold */
  MOUSECALL(&p);
}
#endif

/*===========================================================================*/
/*                                                                           */
/* File    : MOUSTSUB.C                                                      */
/*                                                                           */
/* Purpose : Sets the user-defined subroutine for mouse interrupts           */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_SetSubroutine(mask, func)
  int  mask;
  VOID (FAR * func) ();
{
#if defined(__NTCONSOLE__)
  (void) mask;
  (void) func;

//    10 Jun 96  RT   inserted #elif define(__DPMI16__) clause
#elif defined(__DPMI16__)
  WORD segFunc = FP_SEG(func);
  WORD offFunc = FP_OFF(func);

  asm
  {
	 push es
	 mov  ax, segFunc
	 mov  es, ax
	 mov  dx, offFunc
	 mov  ax, 12
	 mov  cx, mask
	 int  33h
	 pop  es
  }

#else
  union REGS r;
  struct SREGS sregs;

  sregs.es = FP_SEG(func);         /* segment of function */
  r.x.dx = FP_OFF(func);           /* offset of function  */
  r.x.cx = mask;                   /* condition call mask */
  r.x.ax = 12;                     /* mouse function 12   */
  
  INT86X(0x33, &r, &r, &sregs);    /* mouse function call */
#endif
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUCURS.C                                                       */
/*                                                                           */
/* Purpose : Functions which set the mouse cursor mode and shape             */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_SetGraphicsCursor(pCursorInfo, hspot, vspot, lpANDmask, lpXORmask)
  LPSYSCURSORINFO pCursorInfo;
  int    hspot, vspot;
  LPVOID lpANDmask;
  LPVOID lpXORmask;
{
#if defined(__NTCONSOLE__)
#pragma warn -par

//    10 Jun 96  RT   inserted #elif define(__DPMI16__) clause
#elif defined(__DPMI16__)
  WORD segFunc = FP_SEG(lpANDmask);
  WORD offFunc = FP_OFF(lpANDmask);

  (void) pCursorInfo;
  (void) lpXORmask;

  asm
  {
	 push es
	 mov  ax, segFunc
	 mov  es, ax
	 mov  dx, offFunc
	 mov  ax, 9
	 mov  bx, hspot
	 mov  cx, vspot
	 int  33h
	 pop  es
  }

#else
  struct SREGS sregs;
  union  REGS  r;

  (void) pCursorInfo;
  (void) lpXORmask;

  r.x.ax = 9;
  r.x.bx = hspot;               /* horizontal hot spot of cursor */
  r.x.cx = vspot;               /* vertical hot spot of cursor */
  segread(&sregs);

#if defined(WC386)
  {
  int far *fpANDmask = lpANDmask;  /* We need a true far pointer */
  r.x.dx   = FP_OFF(fpANDmask); /* offset of mask  */
  sregs.es = FP_SEG(fpANDmask); /* segment address of cursor mask */
  }
#else
  r.x.dx   = FP_OFF(lpANDmask); /* offset of mask  */
  sregs.es = FP_SEG(lpANDmask); /* segment address of cursor mask */
#endif

#if !defined(PL386) /* && !defined(__DPMI16__)*/
  INT86X(0x33, &r, &r, &sregs);
#endif
#endif
}


/* mouse function 10, set cursor mode.  Sets cursor to text or software
	mode, and type of cursor
*/
int FAR PASCAL MOUSE_SetCursorMode(int cursor, int scrnmask, int cursmask)
{
  MOUSEPARAMS p;

  p.m1 = 10;                /* mouse call #10 */
  p.m2 = cursor;            /* if 0, use software cursor; 1, use hardware
									  * cursor */
  p.m3 = scrnmask;          /* if software cursor, defines screen mask           */
  /* if hardware cursor, defines scan line start       */
  p.m4 = cursmask;          /* if software cursor, defines cursor mask,          */
  /* if hardware cursor, defines scan line stop.       */
  MOUSECALL(&p);
  return 0;
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUHIDER.C                                                      */
/*                                                                           */
/* Purpose : Hides the mouse cursor when it enters a certain region          */
/*                                                                           */
/*===========================================================================*/
/* This function hides the mouse if it is in the region when this function
	is called.  Afterwards your program must call show_cursor
	to show the cursor again.
*/
VOID FAR PASCAL MOUSE_ConditionalOff(int x1, int y1, int x2, int y2)
{
#if defined(__NTCONSOLE__)
  TREGISTERS tr;

  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 16;
  tr.CX = x1;
  tr.DX = y1;
  tr.SI = x2;
  tr.DI = y2;
  RealIntr(0x33, &tr);

#elif defined(__DPMI16__)
  _SI = x2;
  _DI = y2;
  _AX = 16;
  _CX = x1;
  _DX = y1;
  geninterrupt(0x33);       /* call the mouse interrupt */

#else
  union REGS r;
  r.x.si = x2;                  /* lower x screen coordinates */
  r.x.di = y2;                  /* lower y screen coordinates */
  r.x.ax = 16;                  /* mouse function 16          */
  r.x.cx = x1;                  /* upper x screen coordinates */
  r.x.dx = y1;                  /* upper y screen coordinates */
  INT86(0x33, &r, &r);          /* mouse interrupt */
#endif
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUSHOW.C                                                       */
/*                                                                           */
/* Purpose : Function to show the mouse cursor                               */
/*                                                                           */
/*===========================================================================*/
INT  FAR PASCAL MOUSE_ShowCursor(void)         /* Msoft mouse function 1 */
{
#if defined(__NTCONSOLE__)
#if defined(MEWEL_GUI) || defined(MEWEL_TEXT)
  TREGISTERS tr;
  memset((char *) &tr, 0, sizeof(tr));
  tr.AX = 1;
  RealIntr(0x33, &tr);
#else
  SetConsoleMode(NTConsoleInfo.hStdInput, 
					  NTConsoleInfo.dwConsoleMode | ENABLE_MOUSE_INPUT);
#endif

#else
  MOUSEPARAMS p;
  p.m1 = 1;
  MOUSECALL(&p);
#endif

  return TRUE;
}

/*===========================================================================*/
/*                                                                           */
/* File    : MOUBOUND.C                                                      */
/*                                                                           */
/* Purpose : Functions to set the horizontal and vertical bounds             */
/*                                                                           */
/*===========================================================================*/
VOID FAR PASCAL MOUSE_SetHBounds(l, r)        /* set left and right boundaries */
  int  l, r;
{
  MOUSEPARAMS p;

  p.m1 = 7;
  p.m3 = l;
  p.m4 = r;
  MOUSECALL(&p);
}

VOID FAR PASCAL MOUSE_SetVBounds(t, b)        /* set top and bottom bounds */
  int  t, b;
{
  MOUSEPARAMS p;

  p.m1 = 8;
  p.m3 = t;
  p.m4 = b;
  MOUSECALL(&p);
}

VOID FAR PASCAL MOUSE_ClipCursor(x1, y1, x2, y2)
  int  x1, y1, x2, y2;
{
  MOUSE_SetHBounds(x1, x2);       /* set left and right boundaries */
  MOUSE_SetVBounds(y1, y2);       /* set top and bottom bounds */
}

