/*===========================================================================*/
/*                                                                           */
/* File    : WINMOVE.C                                                       */
/*                                                                           */
/* Purpose : WinMove()                                                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static INT PASCAL _WinMove(HWND,int,int);


INT FAR PASCAL WinMove(hWnd, row, col)
  HWND hWnd;
  INT  row, col;   /* absolute screen coords */
{
  INT rc;

  rc = _WinMove(hWnd, row, col);

#if defined(MEWEL_TEXT)
  if (!TEST_PROGRAM_STATE(STATE_DEFER_VISMAP) && IsWindowVisible(hWnd))
    WinUpdateVisMap();
#endif

  return rc;
}


static INT PASCAL _WinMove(hWnd, row, col)
  HWND hWnd;
  INT  row, col;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *origw = w;

  /*
    Determine the old dimensions of the window
  */
  int    old_height;
  int    old_width;
  int    oldtop;
  int    oldleft;

  int    cxClient, cyClient;

  /*
    Null check
  */
  if (!w)
    return FALSE;

  /*
    Determine the old dimensions of the window
  */
  old_height = RECT_HEIGHT(w->rect);
  old_width  = RECT_WIDTH(w->rect);
  oldtop     = w->rect.top;
  oldleft    = w->rect.left;
  cxClient   = w->rClient.left;
  cyClient   = w->rClient.top;


  /*
    Set the window rect and the client rect
  */
  SetRect(&w->rect, col, row, col+old_width, row+old_height);
  _WinSetClientRect(hWnd);

  cxClient = w->rClient.left - cxClient;
  cyClient = w->rClient.top  - cyClient;


  /*
    Send the WM_MOVE message to the window in order to allow it to
    do some app-specific processing...
  */
#if !102792  /* moved to SetWindowPos() */
  SendMessage(hWnd, WM_MOVE, 0, MAKELONG(col, row));
#endif

  /*
    Recursively move all of the children
  */
#if defined(MOTIF) || defined(DECWINDOWS)
#else
  for (w = w->children;  w;  w = w->sibling)
  {
    int cx = (w->flags & WS_CHILD) ? cxClient : (col-oldleft);
    int cy = (w->flags & WS_CHILD) ? cyClient : (row-oldtop);
    _WinMove(w->win_id, w->rect.top + cy, w->rect.left + cx);
  }
#endif


  /*
     Make sure the menubar and system menu are moved correctly.
  */
#if defined(MEWEL_TEXT)
  if (origw->hMenu)
    _MenuFixPopups(origw, origw->hMenu);
#endif


  if (origw->hSysMenu)
    _WinMove(origw->hSysMenu, 
#ifdef MEWEL_GUI
             origw->rect.top+
               IGetSystemMetrics((origw->flags & WS_THICKFRAME && !IsZoomed(origw->win_id)) ? SM_CYFRAME : SM_CYBORDER) +
               IGetSystemMetrics(SM_CYSIZE),
             origw->rClient.left);
#else
             origw->rect.top+IGetSystemMetrics(SM_CYSIZE),
             origw->rect.left);
#endif


#ifdef USE_ICON_TITLE
  if (origw->flags & WS_MINIMIZE)
  {
    HWND hTitle = ((PRESTOREINFO) origw->pRestoreInfo)->hIconTitle;
    if (hTitle)
    {
      RECT r;  /* cause _WinDestroyInvalid messes up the rect arg */
      int cxHalfSpacing, sLen;

      sLen = lstrlen(origw->title) * VideoInfo.xFontWidth;
      if (sLen > IGetSystemMetrics(SM_CXICONSPACING))
        sLen = IGetSystemMetrics(SM_CXICONSPACING);
      cxHalfSpacing = (sLen - IGetSystemMetrics(SM_CXICON)) / 2;
      r = WID_TO_WIN(hTitle)->rect;
      if (IsWindowVisible(hWnd))
      {
        WinGenInvalidRects(GetParentOrDT(hWnd), &r);
        InvalidateRect(hTitle, NULL, TRUE);
      }
      _WinMove(hTitle, origw->rect.top + IGetSystemMetrics(SM_CYICON) + 4,
                       origw->rect.left - cxHalfSpacing);
                                          
    }
  }
#endif

  /*
    If we moved te window which has the caret, move the caret too.
  */
  if (InternalSysParams.hWndCaret == hWnd)
  {
    POINT pt;
    GetCaretPos(&pt);
    SetCaretPos(pt.x, pt.y);
  }

#if defined(XWINDOWS) && !defined(MOTIF)
  _XWinMove(hWnd, col, row);
#endif

  return TRUE;
}


/*==========================================================================*/
/*                                                                          */
/*                                MoveWindow                                */
/*  Usage:      Same as MS-WINDOWS.                                         */
/*  Input:      Same as MS-WINDOWS.                                         */
/*  Output:     Same as MS-WINDOWS.                                         */
/*  Desc:       This functions provides the MS-WINDOWS standard MoveWindow. */
/*              This can not simply be a macro because it has to handle     */
/*              dialog boxes in a special way.                              */
/*  Modifies:   None                                                        */
/*  Notes:      None                                                        */
/*==========================================================================*/
#if (WINVER >= 0x030a)
BOOL
#else
VOID 
#endif
FAR PASCAL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, 
                      BOOL fRepaint)
{
#if (WINVER >= 0x030a)
  return
#endif
  SetWindowPos(hWnd, (HWND) 0, X, Y, nWidth, nHeight, 
               (fRepaint) ? (SWP_NOZORDER | SWP_NOACTIVATE) 
                          : (SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
}

