/*===========================================================================*/
/*                                                                           */
/* File    : WLINETO.C                                                       */
/*                                                                           */
/* Purpose : MoveTo() and LineTo() commands for MS Windows compat            */
/*                                                                           */
/* History : 8/13/90 (maa) hacked up                                         */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  Hook to GUIs and graphics engines
*/
typedef int (FAR PASCAL *LINETOPROC)(HDC, INT, INT);

LINETOPROC lpfnLineToHook = (LINETOPROC) 0;
LINETOPROC lpfnMoveToHook = (LINETOPROC) 0;


DWORD FAR PASCAL MoveTo(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  POINT pt;

  if (MoveToEx(hDC, x, y, (POINT FAR *) &pt))
    return MAKELONG(pt.x, pt.y);
  else
    return 0L;
}

BOOL FAR PASCAL MoveToEx(hDC, x, y, lppt)
  HDC hDC;
  INT x, y;
  POINT FAR *lppt;
{
  LPHDC lphDC;

  /*
    Get a pointer to the DC
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Validate the window
  */
  if (!IsWindow(lphDC->hWnd))
    return FALSE;

  /*
    Save the old pen position for the returned POINT
  */
  if (lppt)
  {
    lppt->x = lphDC->ptPen.x;
    lppt->y = lphDC->ptPen.y;
  }

  /*
    Assign the new logical coordinates to the pen position
  */
  lphDC->ptPen.x = x;
  lphDC->ptPen.y = y;

  /*
    Hook to other GUIs?
  */
  if (lpfnMoveToHook)
    (*lpfnMoveToHook)(hDC, x, y);

  return TRUE;
}


BOOL FAR PASCAL LineTo(hDC, x, y)
  HDC hDC;
  INT x, y;
{
  HPEN   hPen;
  BYTE   chPen;
  LOGPEN lp;
  POINT  ptScreen;
  COLOR  attr, attrWin;
  WINDOW *w;
  int    rc = FALSE;
  LPHDC  lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0L;

  if (!IsWindow(lphDC->hWnd))
    return FALSE;

  if (lpfnLineToHook)
    return (*lpfnLineToHook)(hDC, x, y);

  LPtoSP(hDC, &ptScreen, 1);
  ScreenToClient(lphDC->hWnd, &ptScreen);
  hPen = lphDC->hPen;

  if (hPen && hPen != NULL_PEN)
  {
    GetObject(hPen, sizeof(lp), (LPSTR) &lp);
    if (lp.lopnStyle != PS_NULL)
    {
      w = WID_TO_WIN(lphDC->hWnd);
      if ((attrWin = w->attr) == SYSTEM_COLOR)
        attrWin = WinGetClassBrush(w->win_id);
      attr = RGBtoAttr(hDC, lp.lopnColor);
      attr = (attrWin & 0xF0) | (attr & 0x0F);
    }
    else
      return FALSE;
  }
  else
    return FALSE;


  /*
    Draw a vertical line?
  */
  if (lphDC->ptPen.x == x)
  {
    int  yFrom, yTo;
    char s[2];

    yFrom = min(y, lphDC->ptPen.y);
    yTo   = max(y, lphDC->ptPen.y);
    s[0]  = SysPenDrawingChars[lp.lopnStyle][1];
    s[1]  = '\0';

    while (yFrom <= yTo)
      _WinPuts(lphDC->hWnd, hDC, yFrom++, lphDC->ptPen.x, s, attr, 1, FALSE);

    rc = TRUE;
  }

  /*
    Draw a horizontal line?
  */
  else if (lphDC->ptPen.y == y)
  {
    int  iWidth, xFrom, xTo;
    char szLineBuf[133];

    xFrom = min(x, lphDC->ptPen.x);
    xTo   = max(x, lphDC->ptPen.x);
    iWidth = min(xTo - xFrom + 1, sizeof(szLineBuf)-2);

    chPen = SysPenDrawingChars[lp.lopnStyle][0];
    memset(szLineBuf, chPen, iWidth);
    szLineBuf[iWidth] = '\0';

    _WinPuts(lphDC->hWnd, hDC, lphDC->ptPen.y, xFrom, szLineBuf, attr, iWidth, FALSE);
    rc = TRUE;
  }

  MoveTo(hDC, x, y);
  return rc;
}

