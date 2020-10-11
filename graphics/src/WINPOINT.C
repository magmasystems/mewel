/*===========================================================================*/
/*                                                                           */
/* File    : WINPOINT.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif
static BOOL PASCAL _WinContainsPoint(WINDOW *, POINT, BOOL);
static HWND PASCAL _WindowFromPoint(HWND, POINT);
#ifdef __cplusplus
}
#endif


/*===========================================================================*/
/*                                                                           */
/* Purpose : WindowFromPoint()                                               */
/*  Returns the window handle of the top-most window which covers <row,col>  */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL WindowFromPoint(pt)
  POINT pt;  /* screen-based coordinate */
{
  return _WindowFromPoint(_HwndDesktop, pt);
}

static HWND PASCAL _WindowFromPoint(hWnd, pt)
  HWND  hWnd;
  POINT pt;  /* screen-based coordinate */
{
  WINDOW *w;
  HWND   hChild;

  if (!IsWindowVisible(hWnd))
    return NULLHWND;

  w = WID_TO_WIN(hWnd); 
  if (PtInRect(&w->rect, pt))
  {
    if (PtInRect(&w->rClient, pt))
      for (w = w->children;  w;  w = w->sibling)
        if ((hChild = _WindowFromPoint(w->win_id, pt)) != NULLHWND)
          return hChild;
    return hWnd;
  }

  return NULLHWND;
}

/*===========================================================================*/
/*                                                                           */
/* Purpose : WinChildWindowFromPoint()                                       */
/*                                                                           */
/*===========================================================================*/
HWND FAR PASCAL ChildWindowFromPoint(hParent, pt)
  HWND hParent;
  POINT pt;
{
  WINDOW *w = WID_TO_WIN(hParent);

  if (!w)
    return NULLHWND;

  /*
    Convert the client coords to screen coords
  */
  ClientToScreen(hParent, &pt);

  /*
    If the point is not in the entire window, return NULL
  */
  if (!PtInRect((LPRECT) &w->rect, pt))
    return NULLHWND;

  /*
    We have a special case if part of a dropdown combo lies outside its
    parent dialog box, and we click on the portion of the dropdown which
    is outside of the dialog. Without this test, the handle of the
    dialog box will never be returned.
  */
#if defined(MEWEL_GUI)
  if (InternalSysParams.hWndFocus)
  {
    WINDOW *wFocus = WID_TO_WIN(InternalSysParams.hWndFocus);
    if (wFocus && (wFocus->ulStyle & LBS_IN_COMBOBOX) &&
        PtInRect((LPRECT) &wFocus->rect, pt))
      return InternalSysParams.hWndFocus;
  }
#endif

  /*
    Go through all of the first-level children in Z-order.
  */
  if (PtInRect((LPRECT) &w->rClient, pt))
  {
    for (w = w->children;  w;  w = w->sibling)
#if 0
      /*
        Note - Windows does not recurse on the grandchildren.
      */
      if (w->children && 
          (hWnd = ChildWindowFromPoint(w->win_id, pt)) != NULLHWND)
        return NULLHWND;
      else
#endif
      if (_WinContainsPoint(w, pt, FALSE))
        return w->win_id;
  }

  /*
    The point is in the non-client area of the passed window.
  */
#if 0
  return hWnd;
#else
  return NULLHWND;
#endif
}

/*===========================================================================*/
/*                                                                           */
/* _WinContainsPoint()                                                       */
/*   Low level function which tests if a window contains point <row,col>.    */
/*                                                                           */
/*===========================================================================*/
static BOOL PASCAL _WinContainsPoint(WINDOW *w, POINT pt, BOOL bTestFrame)
     /* bTestFrame is TRUE if we test the inside of a frame */
{
  RECT r;

  if (!IsWindowVisible(w->win_id))
    return FALSE;

  /*
    Even before we test for clipping, see if the point to test is in the
    candidate window (or on the border of a frame window).
  */
  r = w->rect;
  if (!PtInRect((LPRECT) &r, pt))
    return FALSE;

  /*
    The point is in the rectangle. If we want to test for frames, then
    see if the point is inside the frame. If it is, then return FALSE,
    cause the window in the area inside of the frame owns the point.
  */
  if (bTestFrame)
  {
    if (IsFrameClass(w))
      if (pt.y != r.top  && pt.y != r.bottom && 
          pt.x != r.left && pt.x != r.right)
        return FALSE;
  }

#ifdef MEWEL_GUI
  return TRUE;
#else
  if (WinVisMap[pt.y * VideoInfo.width + pt.x] == w->win_id)
    return TRUE;
  else
    return FALSE;
#endif
}

