/*===========================================================================*/
/*                                                                           */
/* File    : WINSIZE.C                                                       */
/*                                                                           */
/* Purpose : WinSetSize() changes the size of a window. It sends the         */
/*           WM_SIZE message to the window proc.                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

INT FAR PASCAL WinSetSize(hWnd, height, width)
  HWND   hWnd;
  int    height, width;
{
  RECT   rectOrig;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwClassStyle;
  int    rc;

  /*
    This variable will be used as a semaphore so that we don't recurse
    on a case like this :

     case WM_SIZE :
       SetWindowPos(hWnd, .....);
  */
  static HWND hWndSizing = NULLHWND;

  if (!w)
    return FALSE;

  /*
    Figure out the new coordinates of the bottom right corner
  */
  w->rect.right  = w->rect.left + width;
  w->rect.bottom = w->rect.top  + height;
  _WinSetClientRect(hWnd);

  /*
    Save the window rectangle
  */
  rectOrig = w->rect;

  /*
    Inform the window proc that the size has changed
  */
  if (hWndSizing != hWnd)
  {
    hWndSizing = hWnd;
    rc = MEWELSend_WMSIZE(w);
    hWndSizing = NULLHWND;
  }
  else
    rc = TRUE;


  /*
    Do this again in case the user changed the size in response to the WM_SIZE
  */
  if (!EqualRect(&w->rect, &rectOrig))
    _WinSetClientRect(hWnd);


  if (w->idClass == SCROLLBAR_CLASS || IS_MENU(w))
  {
#if defined(XWINDOWS) && !defined(MOTIF)
    if (w->idClass == SCROLLBAR_CLASS)
      _XWinResize(hWnd, width, height);
#endif
    return rc;
  }

#if defined(XWINDOWS) && !defined(MOTIF)
  _XWinResize(hWnd, width, height);
#endif

  /*
    Redraw if the class style is CS_REDRAW. Actually, we should post paint
    messages to the parent window.
  */
#if !defined(MOTIF)
  dwClassStyle = GetClassStyle(hWnd);
  if (dwClassStyle & (CS_VREDRAW | CS_HREDRAW))
    InvalidateRect(GetParentOrDT(hWnd), &w->rect, TRUE);
#endif

  /*
    Update the visibility map
  */
  if (IsWindowVisible(hWnd))
    WinUpdateVisMap();

  return rc;
}


INT FAR PASCAL MEWELSend_WMSIZE(w)
  WINDOW *w;
{
  HWND hWnd = w->win_id;

  /*
    WM_SIZE sends the width and height of the client area in lParam,
    and the size code in wParam.
  */
  return (INT)
  SendMessage(hWnd, WM_SIZE,
              IsZoomed(hWnd) ? SIZEFULLSCREEN
                             : ((IsIconic(hWnd)) ? SIZEICONIC : SIZENORMAL),
              MAKELONG(RECT_WIDTH(w->rClient), RECT_HEIGHT(w->rClient)));
}

