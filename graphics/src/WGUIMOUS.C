/*===========================================================================*/
/*                                                                           */
/* File    : WGUIMOUS.C                                                      */
/*                                                                           */
/* Purpose : Graphics-engine specific mouse routines which replace the       */
/*           routines found in WINMOUSE.C. These are needed in order to      */
/*           have a mouse in 256-color modes.                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/*  September 1993: GX Graphics support added by                             */
/*      Keith Nash                                                           */
/*      Nash Computing Services                                              */
/*      (501)758-3104                                                        */
/*      CSID 71021,2130                                                      */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
  
#define INCLUDE_MOUSE
#define INCLUDE_CURSORS
#if !defined(MEWEL_GUI)
#define MEWEL_GUI
#endif
  
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
  
#if defined(GNUGRX) || defined(USE_BCC2GRX)
#define GRX
#include <mousex.h>
#endif


/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_Init()                                                  */
/*                                                                          */
/* Purpose  : Initializes the mouse.                                        */
/*                                                                          */
/* Returns  : Non-zero if successful, 0 if no mouse can be used.            */
/*                                                                          */
/* Notes    : bMouseTerminate is TRUE when called from MouseTerminate.      */
/*            MouseTerminate is called when exiting the program, or         */
/*            by VidInitNewVideoMode when we change video modes.            */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL MOUSE_Init(BOOL bMouseTerminate)
{
  static BOOL bMouseInit = FALSE;

#if defined(META)
  static INT aiMouseType[] =
  {
    MsDriver, MsCOM1, MsCOM2, MoCOM1, MoCOM2
  };
  
  INT i;
  
  /*
    If we are being called from MouseTerminate(), then terminate the mouse.
  */
#ifndef TEST
  if (bMouseTerminate)
  {
    if (bMouseInit)
      mwStopMouse();
    return bMouseInit = FALSE;
  }
#endif
  
  /*
    Check for re-initialization
  */
  if (bMouseInit)
    return TRUE;

  for (i = 0;  i < sizeof(aiMouseType) / sizeof(aiMouseType[0]);  i++)
    if (mwQueryMouse(aiMouseType[i]) == 0)
      break;
  
  if (i == sizeof(aiMouseType) / sizeof(aiMouseType[0]))
    return bMouseInit = FALSE;
  
  mwInitMouse(aiMouseType[i]);
  mwTrackCursor(TRUE);
  return bMouseInit = TRUE;

#elif defined(GX)

  if (bMouseTerminate)
  {
    if (bMouseInit)
      grStopMouse();
    return bMouseInit = FALSE;
  }

  /*
    Check for re-initialization
  */
  if (bMouseInit)
    return TRUE;

  bMouseInit = (grInitMouse() == gxSUCCESS);
  if (bMouseInit)
  {
    grSetMouseMode(gxGRAPHICS);
    grDisplayMouse(grSHOW);
    grTrackMouse(grTRACK);
  }
  return bMouseInit;

#elif defined(GRX)

  if (bMouseTerminate)
  {
    if (bMouseInit)
      MouseUnInit();
    return bMouseInit = FALSE;
  }

  /*
    Check for re-initialization
  */
  if (bMouseInit)
    return TRUE;

  bMouseInit = MouseDetect();
  if (bMouseInit)
  {
    MouseInit();
    MouseDisplayCursor();
    MouseEventEnable(0, 1);
  }
  return bMouseInit;

#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_GetStatus()                                             */
/*                                                                          */
/* Purpose  : Queries the current mouse coordinates and button-press info.  */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_GetStatus(pButton, pX, pY)
  INT    *pButton;
  MWCOORD  *pX, *pY;
{
#if defined(META)

  INT x, y;
  mwReadMouse(&x, &y, pButton);
  *pX = (MWCOORD) x;
  *pY = (MWCOORD) y;

#elif defined(GX)

  INT x, y;
  grGetMousePos(&x, &y);
  *pButton = grGetMouseButtons();
  *pX = (MWCOORD) x;
  *pY = (MWCOORD) y;

#elif defined(GRX)

  MouseEvent mouseEv;
  MouseGetEvent(M_BUTTON_CHANGE | M_MOTION | M_POLL, &mouseEv);
  *pButton = mouseEv.buttons;
  *pX = (MWCOORD) mouseEv.x;
  *pY = (MWCOORD) mouseEv.y;

#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_SetPos()                                                */
/*                                                                          */
/* Purpose  : Moves the mouse cursor to the specified coordinates.          */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_SetPos(x, y)
  INT  x, y;
{
#if defined(META)

  mwTrackCursor(FALSE);
  mwMoveCursor(x, y);
  mwTrackCursor(TRUE);

#elif defined(GX)

  grSetMousePos(x, y);

#elif defined(GRX)

  MouseWarp(x, y);

#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MouseSetSpeed()                                               */
/*                                                                          */
/* Purpose  : Sets the mickey:pixel ratio                                   */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_SetSpeed(x, y)
  INT  x, y;
{
#if defined(META)

  (void) x;  (void) y;
  mwScaleMouse(1, 1);

#elif defined(GX)

  (void) x;  (void) y;

  /* No analogous GX Graphics mouse function - KN */

#elif defined(GRX)

  (void) x;  (void) y;

#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_HideCursor()                                            */
/*                                                                          */
/* Purpose  : Hides the mouse.                                              */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL MOUSE_HideCursor(void)
{
#if defined(META)
  mwHideCursor();
#elif defined(GX)
  grDisplayMouse(grHIDE);
#elif defined(GRX)
  MouseEraseCursor();
#endif
  return TRUE;
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_ShowCursor()                                            */
/*                                                                          */
/* Purpose  : Shows the mouse.                                              */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL MOUSE_ShowCursor(void)
{
#if defined(META)
  mwShowCursor();
#elif defined(GX)
  grDisplayMouse(grSHOW);
#elif defined(GRX)
  MouseDisplayCursor();
#endif
  return TRUE;
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_ClipCursor()                                            */
/*                                                                          */
/* Purpose  : Called by ClipCursor().                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_ClipCursor(x1, y1, x2, y2)
  INT  x1, y1, x2, y2;
{
#if defined(META)
  mwLimitMouse(x1, y1, x2, y2);
#elif defined(GX)
  grSetMouseBounds(x1, y1, x2, y2);
#elif defined(GRX)
  MouseSetLimits(x1, y1, x2, y2);
#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_SetGraphicsCursor()                                     */
/*                                                                          */
/* Purpose  : Sets the cursor shape.                                        */
/*                                                                          */
/****************************************************************************/
#pragma pack(1)
  
#if defined(META)
typedef struct tagCursorData
{
  cursor cursorData;
  BYTE   szImage[4*32];
} CURSORDATA;
  
static CURSORDATA foreMask =
{
  { 16, 16, 0, 0, 2, 1, 1 },
};
  
static CURSORDATA backMask =
{
  { 16, 16, 0, 0, 2, 1, 1 },
};

#elif defined(GX)

typedef struct tagCursorData
{
  BYTE szImage[128];
} CURSORDATA;

#endif
  
  
VOID FAR PASCAL MOUSE_SetGraphicsCursor(pCursorInfo,xSpot,ySpot,lpANDmask,lpXORmask)
  LPSYSCURSORINFO pCursorInfo;
  INT             xSpot, ySpot;
  LPVOID          lpANDmask;
  LPVOID          lpXORmask;
{
#if defined(META)
#if 1
  BYTE buf[128];
  int  i;
  int  nHeight   = pCursorInfo->nHeight;
  int  nRowBytes = pCursorInfo->nWidth / 8;
  int  iMaskSize = nRowBytes * nHeight;
  BOOL bFromResource = pCursorInfo->bFromResource;
 
  /*
    MEWEL stores the cursor bitmaps in the reverse-byte order which is
    needed by the INT 33H routines. So, we have byte 1, 0, 3, 2, 5, 4, ...
    Both the back and fore masks need to re-reverse the bytes so that
    they are in consecutive order. Also, the backMask needs to have
    all of its bits flipped to conform to the way that MetaWindows
    expects the masks to be.
  */

  /*
    AND mask
  */
  if (bFromResource)
  {
    LPSTR lpBuf = backMask.szImage + (nHeight-1)*nRowBytes;
    for (i = 0;  i < nHeight;  i++)
    {
      lmemcpy(lpBuf, (LPSTR) lpXORmask, nRowBytes);
      lpBuf -= nRowBytes;
      (LPSTR) lpXORmask += nRowBytes;
    }
#if 1
    lpBuf = backMask.szImage;
    for (i = 0;  i < iMaskSize;  i++)
      *lpBuf++ ^= 0xFF;
#endif
  }
  else
  {
    lmemcpy(buf, (LPSTR) lpANDmask, iMaskSize);
    for (i = 0;  i < iMaskSize;  i += 2)
    {
      BYTE ch1, ch2;
      ch1 = buf[i];
      ch2 = buf[i+1];
      ch1 ^= 0xFF;
      ch2 ^= 0xFF;
      buf[i]   = ch2;
      buf[i+1] = ch1;
    }
    lmemcpy(backMask.szImage, buf, iMaskSize);
  }
  
  /*
    XOR mask
  */
  if (bFromResource)
  {
    LPSTR lpBuf = foreMask.szImage + (nHeight-1)*nRowBytes;
    for (i = 0;  i < nHeight;  i++)
    {
      lmemcpy(lpBuf, (LPSTR) lpANDmask, nRowBytes);
      lpBuf -= nRowBytes;
      (LPSTR) lpANDmask += nRowBytes;
    }
#if 0
    lpBuf = foreMask.szImage;
    for (i = 0;  i < iMaskSize;  i++)
      *lpBuf++ ^= 0xFF;
#endif
  }
  else
  {
    lmemcpy(buf, (LPSTR) lpXORmask, iMaskSize);
    for (i = 0;  i < iMaskSize;  i += 2)
    {
      BYTE ch1, ch2;
      ch1      = buf[i];
      ch2      = buf[i+1];
      buf[i]   = ch2;
      buf[i+1] = ch1;
    }
    lmemcpy(foreMask.szImage, buf, iMaskSize);
  }

  backMask.cursorData.curWidth  = foreMask.cursorData.curWidth  = pCursorInfo->nWidth;
  backMask.cursorData.curHeight = foreMask.cursorData.curHeight = pCursorInfo->nHeight;
  backMask.cursorData.curRowBytes = foreMask.cursorData.curRowBytes = (pCursorInfo->nWidth / 8);

#else
  int iMaskSize = (pCursorInfo->nWidth / 8) * pCursorInfo->nHeight;
  lmemcpy(backMask.szImage, (LPSTR) lpANDmask,  iMaskSize);
  lmemcpy(foreMask.szImage, (LPSTR) lpXORmask,  iMaskSize);
#endif

  mwDefineCursor(0, xSpot, ySpot, (cursor *) &backMask, (cursor *) &foreMask);
  mwCursorStyle(0);
  (void) lpANDmask;


#elif defined(GX)
  CURSORDATA foreMask, backMask;
  int  i;
  
  int  iMaskSize = (pCursorInfo->nWidth / 8) * pCursorInfo->nHeight;

  lmemcpy(backMask.szImage, (LPSTR) lpANDmask, iMaskSize);
  lmemcpy(foreMask.szImage, (LPSTR) lpXORmask, iMaskSize);

  for (i = 0;  i < iMaskSize;  i++)
  {
    backMask.szImage[i] ^= 0xFF;
    foreMask.szImage[i] ^= 0xFF;
  }

  grSetMouseMask(grCUSER, xSpot, ySpot, (int *) &backMask, (int *) &foreMask);
  grSetMouseStyle(grCUSER,grBLACK);


#elif defined(GRX)

#if 0
  GrCursor *grCursor;

  grCursor = GrBuildCursor(buf, pCursorInfo->nWidth, pCursorInfo->nHeight,
                           xSpot, ySpot, NULL);
  if (grCursor)
    MouseSetCursor(grCursor);
#endif

#endif
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_SetCursorMode()                                         */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/****************************************************************************/
int FAR PASCAL MOUSE_SetCursorMode(iCursor, scrnmask, cursmask)
  int iCursor;
  int scrnmask;
  int cursmask;
{
  (void) iCursor;
  (void) scrnmask;
  (void) cursmask;
  return 0;
}
  
/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_SetHBounds()                                            */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_SetHBounds(l, r)        /* set left and right boundaries */
  int l, r;
{
#if defined(GX)
  int CurX1, CurY1, CurX2, CurY2;
  grGetMouseBounds(&CurX1, &CurY1, &CurX2, &CurY2);
  grSetMouseBounds(l,CurY1,r,CurY2);
#endif        
  (void) l;  (void) r;
}

/****************************************************************************/
/*                                                                          */
/* Function : MOUSE_SetVBounds()                                            */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MOUSE_SetVBounds(t, b)        /* set top and bottom bounds */
  int t, b;
{
#if defined(GX)
  int CurX1, CurY1, CurX2, CurY2;
  grGetMouseBounds(&CurX1, &CurY1, &CurX2, &CurY2);
  grSetMouseBounds(CurX1,t,CurX2,b);
#endif        
  (void) t;   (void) b;
}


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
#if defined(__NTCONSOLE__) || defined(__DPMI32__)
  (void) mask;
  (void) func;

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

/****************************************************************************/
/*                                                                          */
/* Test main                                                                */
/*                                                                          */
/****************************************************************************/

#if defined(TEST)
  
extern int _Cdecl getch(void);
  
main()
{
#if defined(META)
  mwInitGraphics(VGA640x480);
  mwSetDisplay(GrafPg0);
#endif
  
  if (MOUSE_Init())
  {
    MOUSE_SetSpeed(8, 8);           /* set mickey/pixel ratio to 1:1 */
    MOUSE_ShowCursor();
#if defined(META)
    mwCursorColor(7, 0);
#endif
  
    getch();
  
#if defined(META)
    mwStopMouse();
#endif
  }
  
#if defined(META)
  mwSetDisplay(TextPg0);
  mwStopGraphics();
#endif

  return 1;
}
#endif
  
