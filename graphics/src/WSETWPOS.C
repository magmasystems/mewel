/*===========================================================================*/
/*                                                                           */
/* File    : WSETWPOS.C                                                      */
/*                                                                           */
/* Purpose : Implements the SetWindowPos() function                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


/*
  These are implemented in WZORDER.C
*/
#ifdef __cplusplus
extern "C" {
#endif
extern VOID FAR PASCAL _WinMoveToBottom(HWND);
extern VOID FAR PASCAL _WinMoveAfter(HWND, HWND);
extern HWND FAR PASCAL _WinTreeToTop(HWND);
#ifdef __cplusplus
}
#endif


#if (WINVER >= 0x030a)
INT
#else
VOID
#endif
FAR PASCAL SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, wFlags)
  HWND  hWnd;
  HWND  hWndInsertAfter;
  INT   x, y, cx, cy;
  UINT  wFlags;
{
#if defined(XWINDOWS)
  XWindowChanges Xchanges;
  int            Xchangemask = 0;
  BOOL           bDidMove = FALSE;
  BOOL           bDidSize = FALSE;
#endif

  WINDOW *w;
  DWORD  dwStyle;
  HWND   hParentToDraw;
  POINT  pt;
  BOOL   bDrawParent = FALSE;
  BOOL   bInvalidateWindow = FALSE;
  BOOL   bVisible;
  RECT   rOrig;
  RECT   rInvalid;
  RECT   rParentInvalid;
  INT    iOldHeight;
  INT    iOldWidth;
  BOOL   bNewSize = FALSE;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
#if (WINVER >= 0x030a)
    return 0;
#else
    return;
#endif

  rOrig          = w->rect;
  rInvalid       = rOrig;
  rParentInvalid = RectEmpty;
  dwStyle        = w->flags;
  bVisible       = (BOOL) ((dwStyle & WS_VISIBLE) != 0L);

  /*
    If we are moving a dialog box whose parent is smaller than the
    dialog, then we must refresh the underlying desktop window. So,
    do not retrieve the dialog box's owner, but the desktop window.
  */
  if (w->parent)
    hParentToDraw  = w->parent->win_id;
  else
    hParentToDraw  = (HWND) -1;

  /*
    Alter the Z-order? We definitely must if we are being called from
    SetActiveWindow or from BringWindowToTop.
  */
  if (!(wFlags & SWP_NOZORDER))
  {
    /*
      Assume that the entire window area is invalid after we changed
      the Z-order. This assumption is false only if we try to bring
      a window to the top which is already on the top.
    */
    rParentInvalid = rOrig;

#if defined(XWINDOWS)
    Xchangemask |= CWStackMode;
    Xchanges.stack_mode = (hWndInsertAfter == HWND_TOP) ? Above : Below;
#endif

    if (!hWndInsertAfter)
    {
      /*
        Move the window and all of its ancestors to the front of their
        sibling lists. _WinTreeToTop() returns the handle of the
        highest ancestor window which was shifted in the z-order list.
        It is NULL if no windows were shifted in the Z-order.
        However, we definitely want to refresh the parent if this 
        window is being moved or resized.
      */
      HWND hP;
      BOOL bNotChangingVismap;

      /*
        We are not changing what gets displayed in the parent if we
        are not moving nor resizing the window and we are not changing
        the visibility state of the window.
      */
      bNotChangingVismap = (BOOL) 
          ((wFlags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE) &&
            !(wFlags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)));

      hP = _WinTreeToTop(hWnd);
      /*
        If the window was already at the top of the parent's Z-order list,
        do not refresh the parent if we aren't changing the window in
        any other way. In that case, we do not need to set the invalid
        parent area.
      */
      if (hP == NULLHWND)
      {
        if (bNotChangingVismap)
        {
          hParentToDraw = (HWND) -1;
          rParentInvalid = RectEmpty;
        }
      }
      /*
        The window was actually brought to the top.
      */
      else if (hP != hWnd || bNotChangingVismap)
      {
        hParentToDraw = hP;
      }
    }
    else if (hWndInsertAfter == (HWND) 1)
    {
      /* Put at the bottom of the list */
      _WinMoveToBottom(hWnd);
    }
    else
    {
      /* Insert the window after window hWndInsertAfter in the window list */
      _WinMoveAfter(hWnd, hWndInsertAfter);
#if defined(XWINDOWS)
#if !111594
      Xchanges.sibling = WID_TO_WIN(hWndInsertAfter)->Xwindow;
      Xchangemask |= CWSibling;
#endif
#endif
    }
    bDrawParent = TRUE;
    if (!IsRectEmpty(&rParentInvalid))
      bInvalidateWindow = TRUE;
  }


  /*
    Move the window?
  */
  if (!(wFlags & SWP_NOMOVE))
  {
    BOOL bChangedPos;

    /*
      If it's a child window, translate the client-relative coords
      into absolute screen coords for the benefit of WinMove().
    */
    pt.x = (MWCOORD) x;
    pt.y = (MWCOORD) y;
#if !defined(MOTIF)
    /*
      9/29/94 (maa)
      In Motif, child widgets are based off of their parent's drawing area
    */
    if (dwStyle & WS_CHILD)
    {
      HWND hParent = GetParentOrDT(hWnd);
      ClientToScreen(hParent, &pt);
    }
#endif

    bChangedPos = (pt.x != rOrig.left || pt.y != rOrig.top);

    /*
      If we have a shell window which is a child (like an MDI child),
      then we must base the coordinates off of the parent window.
    */
#if defined(MOTIF)
    if (w->widgetShell && (dwStyle & WS_CHILD) && 
        !(w->dwXflags & WSX_USEMEWELSHELL))
    {
      Position parentX, parentY;
      int      currX, currY;

      HWND   hParent = GetParentOrDT(hWnd);
      WINDOW *wParent = WID_TO_WIN(hParent);
      Widget widgetParent;

      /*
        We can't use XtParent(), cause the parent of a shell is
        *always* the root window of the system.
      */
      if ((widgetParent = wParent->widgetDrawingArea) == NULL)
        widgetParent = w->widget;

      /*
        Set the screen-based coordinates of the shell.
      */
      XtTranslateCoords(widgetParent, 0, 0, &parentX, &parentY);
      pt.x += parentX;
      pt.y += parentY;

      /*
        If the desktop window has moved, then we definitely must
        move the child in order to keep the parent-child relationship
      */
      if (!bChangedPos)
      {
        XtVaGetValues(w->widgetShell, XmNx, &currX, XmNy, &currY, NULL);
        if ((currX & 0x0FFFF) != pt.x || (currY & 0x0FFFF) != pt.y)
          bChangedPos++;
      }
    }
#endif

    /*
      Send the WM_MOVE message if the position of the window is actually
      changing or if we are touching the window for the first time.
    */
    if (bChangedPos || (w->ulStyle & WIN_SEND_WMSIZE))
    {
#if defined(MOTIF)
#if 111594
      Xchanges.x = (Position) (pt.x);
      Xchanges.y = (Position) (pt.y);
#else
      Xchanges.x = (Position) (pt.x - InternalSysParams.wDesktop->rect.left);
      Xchanges.y = (Position) (pt.y - InternalSysParams.wDesktop->rect.top);
#endif
      Xchangemask |= CWX | CWY;
      bDidMove++;
#endif

#if !defined(MOTIF)
      WinMove(hWnd, pt.y, pt.x);
      /*
        Send the WM_MOVE message to the window in order to allow it to
        do some app-specific processing...
      */
      MEWELSend_WMMOVE(w);
#endif

      /*
        If we moved the window, then definitely redraw the parent.
        However, if we did not move the window, and we only executed
        this piece of code because the initial WM_MOVE message had
        to be sent, then the parent does not necessarily need to
        be refreshed.
      */
      if (bChangedPos && bVisible)
      {
        bDrawParent = TRUE;
        rParentInvalid = rOrig;
      }

      UnionRect(&rInvalid, &rOrig, &w->rect);
      bInvalidateWindow = TRUE;
    }
  }


  /*
    Resize the window?
  */
  if (!(wFlags & SWP_NOSIZE))
  {
    iOldHeight  = RECT_HEIGHT(w->rect);
    iOldWidth   = RECT_WIDTH(w->rect);
    bNewSize    = (BOOL) (cy != iOldHeight || cx != iOldWidth);

    /*
      Send the WM_SIZE message if the position of the window is actually
      changing or if we are touching the window for the first time.
    */
    if (bNewSize || (w->ulStyle & WIN_SEND_WMSIZE))
    {
#if defined(MOTIF)
      if (bNewSize)
      {
        Xchanges.width  = (Dimension) cx;
        Xchanges.height = (Dimension) cy;
        Xchangemask |= CWWidth | CWHeight;
      }
      bDidSize++;
#endif

#if !defined(MOTIF)
      WinSetSize(hWnd, cy, cx);
#endif

      /*
        If we resized the window, then definitely redraw the parent.
        However, if we did not resize the window, and we only executed
        this piece of code because the initial WM_SIZE message had
        to be sent, then the parent does not necessarily need to
        be refreshed.
      */
      if (bNewSize && bVisible)
      {
        bDrawParent = TRUE;
        /*
          If we just expanded the window, then do not refresh the
          parent. Otherwise, it's easier just to refresh the entire
          parent.
        */
        if (!(cy >= iOldHeight && cx >= iOldWidth))
          rParentInvalid = rOrig;
      }

      UnionRect(&rInvalid, &rInvalid, &w->rect);
      bInvalidateWindow = TRUE;

      /*
        Do not send the initial WM_SIZE message twice
      */
#if !defined(MOTIF)
      if ((w->ulStyle & WIN_SEND_WMSIZE))
        w->ulStyle &= ~WIN_SEND_WMSIZE;
#endif
    }
  }


  /*
    Alter the window's visibility?
  */
  if (wFlags & SWP_HIDEWINDOW)
  {
    _WinShowWindow(w, FALSE);

    /*
      We need to invalidate the parent where the original window was,
      but only if the window was originally visible.
    */
    if (bVisible)
    {
      bDrawParent = TRUE;
      UnionRect(&rParentInvalid, &rParentInvalid, &rOrig);
    }
  }


#if defined(XWINDOWS)
  /*
    X Windows geometry management
  */
    if (Xchangemask)
    {
      XMEWELSetWindowPos(w, Xchangemask, &Xchanges, FALSE);
#if !111594
      if (bDidMove)
      {
        WinMove(hWnd, pt.y, pt.x);
      }
      if (bDidSize)
      {
        WinSetSize(hWnd, cy, cx);
        UnionRect(&rInvalid, &rInvalid, &w->rect);
        if ((w->ulStyle & WIN_SEND_WMSIZE))
          w->ulStyle &= ~WIN_SEND_WMSIZE;
      }
#else
      /*
        We must make sure that a WM_SIZE message gets sent now, and
        not rely on the asynchronous nature of the Notify event to occur.
      */
      if (bDidSize && (w->ulStyle & WIN_SEND_WMSIZE))
      {
        XMEWELSendSizeMsg(hWnd);
      }
#endif
    }
#endif


  /*
    Show the window?
  */
  if (wFlags & SWP_SHOWWINDOW)
  {
    _WinShowWindow(w, TRUE);

    /*
      We need to invalidate the window, but only if the window was 
      originally not visible.
    */
    if (!bVisible)
      bInvalidateWindow = TRUE;
  }

  /*
    Activate the window? This will be true if we are being called from
    BringWindowToTop() or from SetActiveWindow().
  */
  if (!(wFlags & SWP_NOACTIVATE))
  {
    /*
      Windows only activates top-level windows. But MEWEL should
      activate normal and dialog windows (as in the Child Demo
      in MEWLDEMO).
    */
#if 80692
    HWND hParent = _WinGetRootWindow(hWnd);
    if (IsWindowVisible(hParent) && IsWindowEnabled(hParent))
      _WinActivate(hParent, FALSE);
#else
    /*
      This is the MEWEL way of doing things. But it breaks in something
      like zAPP, where you might have a pane child window inside of a
      MDI document window. The MDI doc should get activated, not the pane.
    */
    int idClass  = _WinGetLowestClass(w->idClass);
    if ((idClass == NORMAL_CLASS || idClass == DIALOG_CLASS) &&
        IsWindowVisible(hWnd) && IsWindowEnabled(hWnd))
      _WinActivate(hWnd, FALSE);
#endif
  }


  /*
    Refresh the frame? Do it if we specified the SWP_DRAWFRAME flags and
    if we are not refereshing anyway.
  */
  if (wFlags & SWP_DRAWFRAME)
  {
    w->ulStyle |= WIN_UPDATE_NCAREA;
    SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);
  }


  /*
    Update the visibility map
  */
  if (ShouldWeUpdateVismap(w))
    WinUpdateVisMap();

  /*
    Finally, refresh the screen.
  */
  if (hParentToDraw)
  {
    if (!bDrawParent)
      hParentToDraw = hWnd;
  }
  else
    hParentToDraw = _HwndDesktop;

  if (!IsRectEmpty(&rParentInvalid))
  {
    _WinAdjustRectForShadow(w, (LPRECT) &rParentInvalid);
    WinGenInvalidRects(hParentToDraw, (LPRECT) &rParentInvalid);
  }
  if (bInvalidateWindow)
  {
    InternalInvalidateWindow(hWnd, TRUE);
    /*
      5/18/93 (maa)
        If we are only showing a popup window, do not cause the windows
      under it to be invalid.
    */
    if ((wFlags & (SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER)) ==
                  (SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER) &&
         (dwStyle & WS_POPUP))
      ;
    else
    {
      rInvalid = w->rect;
      _WinAdjustRectForShadow(w, (LPRECT) &rInvalid);
      WinGenInvalidRects(hWnd, (LPRECT) &rInvalid);
    }
  }


#if (WINVER >= 0x030a)
    return 1;
#else
    return;
#endif
}



/*
  Only update the vismap if the window's parent is visible. This will
  save time if, for instance, we are doing ShowWindow()s on a bunch
  of controls whose parent is still not visible. Dialog boxes have
  NULL parents, so they need to updated always.
*/
BOOL FAR PASCAL ShouldWeUpdateVismap(w)
  WINDOW *w;
{
  WINDOW *wParent;

  for (wParent = w->parent;  wParent && wParent != InternalSysParams.wDesktop;
       wParent = wParent->parent)
    if (!(wParent->flags & WS_VISIBLE))
      return FALSE;
  if (wParent == NULL || wParent == InternalSysParams.wDesktop)
    return TRUE;
  else
    return FALSE;
}


INT FAR PASCAL MEWELSend_WMMOVE(w)
  WINDOW *w;
{
  RECT   r;
  WINDOW *wParent;

  /*
    WM_MOVE sends the coordinates of the client area. For child windows,
    the coordinates are relative to the client area of the parent.
  */
  r = w->rClient;
  if ((w->flags & WS_CHILD) || 
      ((wParent = w->parent) != NULL && wParent != InternalSysParams.wDesktop))
    WinScreenRectToClient(GetParent(w->win_id), (LPRECT) &r);
  return (INT) SendMessage(w->win_id, WM_MOVE, 0, MAKELONG(r.left, r.top));
}

