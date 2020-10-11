/*===========================================================================*/
/*                                                                           */
/* File    : WPLACEMT.C                                                      */
/*                                                                           */
/* Purpose : Implements the Get/SetWindowPlacement() functions               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#undef GetWindowPlacement
#undef SetWindowPlacement

BOOL WINAPI GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT FAR* lppl)
{
  POINT ptMin, ptMax;
  RECT  rcNormal;
  UINT  showCmd;
  DWORD ulFlags;
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
    return FALSE;
  ulFlags = w->flags;

  /*
    Get the show state
  */
  if (TEST_WS_HIDDEN(w))
    showCmd = SW_HIDE;
  else if (ulFlags & WS_MINIMIZE)
    showCmd = SW_MINIMIZE;
  else if (ulFlags & WS_MAXIMIZE)
    showCmd = SW_SHOWMAXIMIZED;
  else
    showCmd = (GetActiveWindow() == hWnd) ? SW_SHOW : SW_SHOWNOACTIVATE;

  /*
    Get the minimized position.
    If the window is already iconic, then get its current window origin.
    Otherwise, call _GetIconXY to calculate where the icon would appear.
  */
  if (showCmd == SW_SHOWMINIMIZED)
  {
    ptMin.x = w->rect.left;
    ptMin.y = w->rect.top;
  }
  else
  {
    int x, y, cx, cy;
    _GetIconXY(-1, hWnd, &x, &y, &cx, &cy);
    ptMin.x = x;  ptMin.y = y;  /* MWCOORD vs int */
  }


  /*
    Get the maximized position. This should be the dimensions of the screen.
    This is always 0,0 under MEWEL.
  */
  ptMax.x = ptMax.y = 0;

  /*
    Get the normal or restored position.
  */
  rcNormal = w->rect;
  if ((ulFlags & (WS_MINIMIZE | WS_MAXIMIZE)) && w->pRestoreInfo)
    rcNormal = ((PRESTOREINFO) w->pRestoreInfo)->rect;


  /*
    Fill in the fields
  */
  lppl->length           = sizeof(WINDOWPLACEMENT);
  lppl->flags            = 0;
  lppl->showCmd          = showCmd;
  lppl->ptMinPosition    = ptMin;
  lppl->ptMaxPosition    = ptMax;
  lppl->rcNormalPosition = rcNormal;

  return TRUE;
}


BOOL WINAPI SetWindowPlacement(HWND hWnd, CONST WINDOWPLACEMENT FAR* lppl)
{
  WINDOW *w;
  DWORD  ulFlags;
  int    cx, cy;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
    return FALSE;
  ulFlags = w->flags;

  /*
    lppl->flags is not used right now.
  */

  /*
    Process the rcNormalPosition value if the window is minimzed or maximized.
  */
  if ((ulFlags & (WS_MINIMIZE | WS_MAXIMIZE)) && w->pRestoreInfo)
    ((PRESTOREINFO) w->pRestoreInfo)->rect = lppl->rcNormalPosition;

  cy = w->rect.bottom - w->rect.top;
  cx = w->rect.right  - w->rect.left;
  if ((ulFlags & WS_MINIMIZE))
    MoveWindow(hWnd,lppl->ptMinPosition.x,lppl->ptMinPosition.y,cx,cy,FALSE);
  else if (ulFlags & WS_MAXIMIZE)
    MoveWindow(hWnd,lppl->ptMaxPosition.x,lppl->ptMaxPosition.y,cx,cy,FALSE);
  else
  {
    RECT r;
    r = lppl->rcNormalPosition;
    MoveWindow(hWnd, r.left, r.top, RECT_WIDTH(r), RECT_HEIGHT(r), FALSE);
  }

  return TRUE;
}

