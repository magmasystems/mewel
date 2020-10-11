/*===========================================================================*/
/*                                                                           */
/* File    : WINCARET.C                                                      */
/*                                                                           */
/* Purpose : Implements the Caret family of functions                        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if defined(XWINDOWS)
#define USE_BLINKING_CARET
#endif

static VOID PASCAL WinSetCursor(HWND,int,int);
static VOID CALLBACK InternalCaretCallback(HWND, UINT, UINT, DWORD);

static struct
{
  UINT    wBlinkTime;        /* Blink time */
  INT     iCaretVisibility;  /* Visibility count - Initially hidden */
  INT     nWidth, nHeight;   /* width and height */
  INT     X, Y;              /* Window-relative coordinates */
  HBRUSH  hBrush;            /* brush used to paint the caret */
  HBITMAP hBitmap;           /* bitmap used for the caret   */
  BOOL    bIgnoreTimer;
  INT     idTimer;
  INT     iHideCount;
} CaretInfo =
{
  750,  /* blink  */
  -1,   /* visibility */
  1,    /* width  */
  1,    /* height */
  0,    /* X      */
  0,    /* Y      */
  NULL, /* hBrush */
  NULL, /* hBitmap */
  FALSE,/* bIgnoreTimer */
  0,    /* idTimer */
  1,    /* iHideCount */
};


static struct
{
  CURSORINFO cursorInfo[1];
  int        sp;
} SavedCursorInfo;


VOID FAR PASCAL CreateCaret(hWnd, hBitmap, nWidth, nHeight)
  HWND    hWnd;
  HBITMAP hBitmap;
  int     nWidth, nHeight;
{
  (void) hBitmap;

  CaretInfo.nWidth = nWidth;
  CaretInfo.nHeight = nHeight;
  CaretInfo.X = CaretInfo.Y = -1;  /* -1 means that the pos hasn't been set */

  /*
    Destroy any currently owned caret, and reset the caret-related variables.
  */
  DestroyCaret();

#if defined(USE_BLINKING_CARET)
  CaretInfo.bIgnoreTimer = FALSE;
  CaretInfo.idTimer = SetTimer(NULL, 0, CaretInfo.wBlinkTime,
                               (TIMERPROC) InternalCaretCallback);
#endif

  CaretInfo.iHideCount = 1;
  CaretInfo.iCaretVisibility = -1;
  InternalSysParams.hWndCaret = hWnd;
}

VOID FAR PASCAL DestroyCaret(void)
{
  InternalSysParams.hWndCaret = NULLHWND;

#if defined(USE_BLINKING_CARET)
  if (CaretInfo.idTimer)
    KillTimer(NULL, CaretInfo.idTimer);
#endif

  if (CaretInfo.iCaretVisibility >= 0)
    VidHideCursor();
  CaretInfo.iCaretVisibility = -1;

  /*
    5/18/93 (maa)
      In case we destroy the caret while it is being saved, get rid of
      the saved cursor on the stack.
  */
  SavedCursorInfo.sp = 0;
}


VOID FAR PASCAL HideCaret(hWnd)
  HWND hWnd;
{
  /*
    Windows says that if hWnd is NULL, hide the caret only if it's owned
    by a window in the current task.
  */
  if (hWnd == NULLHWND && InternalSysParams.hWndCaret == NULL)
    return;

  /*
    Don't hide the caret if hWnd if not the window which owns the caret
  */
  if (hWnd != InternalSysParams.hWndCaret)
    return;

  /*
    Decrement the vis count. 
  */
#if defined(USE_BLINKING_CARET)
  CaretInfo.bIgnoreTimer++;
  VidHideCursor();
  CaretInfo.bIgnoreTimer--;
  CaretInfo.iHideCount++;
/*CaretInfo.iCaretVisibility--;*/
#else
  CaretInfo.iHideCount++;
  CaretInfo.iCaretVisibility--;
  WinUpdateCaret();
#endif
}

VOID FAR PASCAL ShowCaret(hWnd)
  HWND hWnd;
{
  /*
    Windows says that if hWnd is NULL, show the caret only if it's owned
    by a window in the current task.
  */
  if (hWnd == NULLHWND && InternalSysParams.hWndCaret == NULL)
    return;

  /*
    Don't show the caret if hWnd if not the window which owns the caret
  */
  if (hWnd != InternalSysParams.hWndCaret)
    return;

  /*
    Bump up the vis count. Show the cursor only if the viscount is 
    greater than zero.
  */
  if (CaretInfo.iHideCount)
    CaretInfo.iHideCount--;
#if !defined(USE_BLINKING_CARET)
  if (++CaretInfo.iCaretVisibility >= 0)
  {
    CaretInfo.iCaretVisibility = 0;  /* don't let it go above zero */
    WinUpdateCaret();
  }
#endif
}


VOID FAR PASCAL GetCaretPos(lpPoint)
  LPPOINT lpPoint;
{
  if (InternalSysParams.hWndCaret && lpPoint)
  {
    lpPoint->y = (MWCOORD) CaretInfo.Y;
    lpPoint->x = (MWCOORD) CaretInfo.X;
  }
}

VOID FAR PASCAL SetCaretPos(X, Y)
  INT X, Y;    /* logical coordinates */
{
  if (InternalSysParams.hWndCaret)
  {
#if defined(USE_BLINKING_CARET)
    CaretInfo.bIgnoreTimer++;
    if (CaretInfo.iCaretVisibility >= 0)
      VidHideCursor();
#endif
    CaretInfo.Y = Y;
    CaretInfo.X = X;
#if defined(USE_BLINKING_CARET)
    CaretInfo.bIgnoreTimer--;
#else
    WinUpdateCaret();
#endif
  }
}


UINT FAR PASCAL GetCaretBlinkTime(void)
{
  return CaretInfo.wBlinkTime;
}

VOID FAR PASCAL SetCaretBlinkTime(wMSeconds)
  UINT wMSeconds;
{
  CaretInfo.wBlinkTime = wMSeconds;

#if defined(USE_BLINKING_CARET)
  if (InternalSysParams.hWndCaret)
  {
    if (CaretInfo.idTimer)
      KillTimer(NULL, CaretInfo.idTimer);
    CaretInfo.idTimer = SetTimer(NULL, 0, CaretInfo.wBlinkTime, 
                                 (TIMERPROC) InternalCaretCallback);
  }
#endif
}

static VOID PASCAL WinSetCursor(hWnd, row, col)
  HWND hWnd;
  int  row, col;  /* client coordinates */
{
  WINDOW *w = WID_TO_WIN(hWnd);
  
  if (w && IsWindowVisible(hWnd) && WinIsPointVisible(hWnd, row, col))
  {
    /*
      Set the cursor position.
      10/15/92
        Prevent the caret from being drawn when the edit control is
        not yet visible and when it needs to be repainted anyway.
        Position the cursor only if the window is visible and there
        is no painting which needs to be done on the window. The
        painting logic will refresh the cursor.
    */
    if (IsRectEmpty(&w->rUpdate))
    {
      if (CaretInfo.iCaretVisibility >= 0)
#if defined(XWINDOWS)
        VidShowCursor();
#elif defined(MEWEL_GUI)
        VidSetCursorScanLines(0, CaretInfo.nHeight, CaretInfo.nWidth);
#else
        VidSetCursorMode(CaretInfo.nHeight > 1, CaretInfo.nWidth);
#endif
      VidSetPos(w->rClient.top + row, w->rClient.left + col);
      CLR_PROGRAM_STATE(STATE_SYNC_CARET);
    }
    else
    {
      VidHideCursor();
      SET_PROGRAM_STATE(STATE_SYNC_CARET);
    }
  }
  else
  {
    VidHideCursor();
  }
}


VOID FAR PASCAL WinUpdateCaret(void)
{
#if !defined(USE_BLINKING_CARET)
  if (InternalSysParams.hWndCaret && CaretInfo.iCaretVisibility >= 0)
  {
    if (CaretInfo.X != -1)
      WinSetCursor(InternalSysParams.hWndCaret, CaretInfo.Y, CaretInfo.X);
  }
  else
  {
    VidHideCursor();
  }
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : CaretPush() / CaretPop()                                      */
/*                                                                          */
/* Purpose  : Pushes and restores caret visibility info.                    */
/*                                                                          */
/* Returns  : TRUE if pushed, FALSE if not.                                 */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL CaretPush(yStart, yEnd)
  int yStart, yEnd;
{
#if defined(XWINDOWS)
  return FALSE;
#else
  /*
    Hide the caret
  */
  if ((CursorInfo.fState & CURSOR_VISIBLE)  &&
      CursorInfo.row >= yStart && CursorInfo.row <= yEnd &&
      SavedCursorInfo.sp < sizeof(SavedCursorInfo.cursorInfo) / sizeof(CURSORINFO))
  {
    memcpy((char *) &SavedCursorInfo.cursorInfo[SavedCursorInfo.sp],
           (char *) &CursorInfo, sizeof(CursorInfo));
    SavedCursorInfo.sp++;
    VidHideCursor();
    return TRUE;
  }
  else
    return FALSE;
#endif
}


VOID FAR PASCAL CaretPop(void)
{
#if !defined(XWINDOWS)
  if (SavedCursorInfo.sp > 0)
  {
    SavedCursorInfo.sp--;
    memcpy((char *) &CursorInfo,
           (char *) &SavedCursorInfo.cursorInfo[SavedCursorInfo.sp],
            sizeof(CursorInfo));
#if defined(XWINDOWS)
    VidShowCursor();
#else
    VidSetCursorScanLines(CursorInfo.startscan, CursorInfo.endscan, 
                          CursorInfo.nWidth);
#endif
  }
#endif
}


#if defined(USE_BLINKING_CARET)

static VOID PASCAL VidDoCursor(BOOL, BOOL);

INT FAR PASCAL VidHideCursor(void)
{
  CaretInfo.iCaretVisibility = -1;
  VidDoCursor(FALSE, FALSE);
  return TRUE;
}

INT FAR PASCAL VidShowCursor(void)
{
  VidDoCursor(TRUE, FALSE);
  return TRUE;
}

static VOID PASCAL VidDoCursor(BOOL bShowing, BOOL bFromCallback)
{
  HDC    hDC;
  RECT   r;

  /*
    If there is no window associated with the caret, return
  */
  if (InternalSysParams.hWndCaret == NULL)
    return;

  if (bFromCallback)
    CaretInfo.iCaretVisibility = (CaretInfo.iCaretVisibility == 0) ? -1 : 0;

  /*
    If we are trying to hide an already hidden caret, return
  */
#if 0
  if (CaretInfo.iCaretVisibility < 0 && !bShowing)
    return;
#endif

  /*
    If we are trying to show an already visible caret, return
  */
#if 0
  if (CaretInfo.iCaretVisibility >= 0 && bShowing)
    return;
#endif

  if (CaretInfo.hBrush == NULL)
#if 0
    CaretInfo.hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
#else
    CaretInfo.hBrush = CreateSolidBrush(RGB(128, 128, 128));
#endif

  SetRect(&r, CaretInfo.X, CaretInfo.Y,
          CaretInfo.X+CaretInfo.nWidth, CaretInfo.Y+CaretInfo.nHeight);

  hDC = GetDC(InternalSysParams.hWndCaret);
  SetROP2(hDC, R2_XORPEN);
  FillRect(hDC, &r, CaretInfo.hBrush);

#if 0
if (bFromCallback)
printf("CaretCallback - x %d, y %d, vis %d\n", CaretInfo.X, CaretInfo.Y, CaretInfo.iCaretVisibility);
#endif


  ReleaseDC(InternalSysParams.hWndCaret, hDC);
}


static VOID CALLBACK InternalCaretCallback(HWND hWnd, UINT message, 
                                           UINT idTimer, DWORD dwTime)
{
  if (CaretInfo.bIgnoreTimer)
    return;

  if (CaretInfo.iCaretVisibility >= 0 || CaretInfo.iHideCount == 0)
    VidDoCursor(TRUE, TRUE);
}

#endif

