/*===========================================================================*/
/*                                                                           */
/* File    : WINCLICK.C                                                      */
/*                                                                           */
/* Purpose : DetermineClickOwner()                                           */
/*                                                                           */
/* History :                                                                 */
/*           6/17/91 (maa) Major gutting to take advantage of VisMap.        */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#if 0
#define DEBUG
#endif

#include "wprivate.h"
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef MEWEL_GUI
static HWND PASCAL _DetermineClickOwner(HWND, POINT, INT *);
#endif
extern VOID PASCAL WriteDebugString(PSTR);
#ifdef __cplusplus
}
#endif


/*
  This is like WinPointToID(), except that some special processing is done
  so that STATIC controls and DISABLED windowss don't get click messages.
*/
HWND FAR PASCAL DetermineClickOwner(row, col, piHitCode)
  INT row, col;    /* absolute screen coords of mouse  */
  INT *piHitCode;  /* pointer to returned HT_xxx value */
{
  HWND   hWnd;
#if defined(MEWEL_GUI)
  POINT  pt;
#endif

  *piHitCode = HTERROR;

  /*
    If a window has the mouse capture set, then all mouse clicks should
    go to that window. So return that window handle.
  */
  if (InternalSysParams.hWndCapture != NULLHWND &&
      IsWindow(InternalSysParams.hWndCapture))
  {
    *piHitCode = HTCLIENT;
    return InternalSysParams.hWndCapture;
  }

#ifdef MEWEL_GUI
  /*
    If we clicked on a menubar item, thus exposing a pulldown, and then
    released the mouse button on the menubar, the pulldown is still visible
    and is still the current focus window. So, in this case, all mouse
    actions should be directed towards the pulldown.
  */
  if (InternalSysParams.hWndFocus && !InternalSysParams.hWndSysModal)
  {
    WINDOW *w = WID_TO_WIN(InternalSysParams.hWndFocus);
    if (w && IS_MENU(w))
    {
      *piHitCode = HTCLIENT;
      return InternalSysParams.hWndFocus;
    }
  }
#endif

#ifdef MEWEL_GUI
  /*
    Find the top-level window which owns the mouse event.
  */
  pt.x = col;  pt.y = row;
  if ((hWnd = ChildWindowFromPoint(_HwndDesktop, pt)) == NULLHWND)
  {
    hWnd = _HwndDesktop;
#ifdef DEBUG
//  MEWELWriteDebugString("ChildWindowFromPoint returns NULL");
#endif
  }


#if 1
  /*
    maa (10/6/92)
      I don't know why, but if we take out this sprintf statement, the
    result from this function seems to be off some of the time. The
    presence of this sprintf function seems to synchronize things!!!!
    This is a bug waiting to happen....
  */
  {
  char buf[32];
  sprintf(buf, "ChildWinFromPoint returns %d", hWnd);
/*MEWELWriteDebugString(buf);*/
  }
#endif


#else
  if ((hWnd = VisMapPointToHwnd(row, col)) == NULLHWND)
    return NULLHWND;
#endif

  /*
    If we clicked over a hidden or disabled window, it's an error.
  */
  if (!IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd))
  {
#ifdef DEBUG
//  MEWELWriteDebugString("!Visible || !Enabled");
#endif
    *piHitCode = HTERROR;
    return _HwndDesktop;
  }

  /*
    Now that we know which top-level window the mouse was clicked over,
    narrow it down to which child.
  */
#ifdef MEWEL_GUI
  hWnd = _DetermineClickOwner(hWnd, pt, piHitCode);
#else
  if ((*piHitCode = (INT)SendMessage(hWnd,WM_NCHITTEST,0,MAKELONG(col,row))) ==
                                                                    HTNOWHERE)
    return NULLHWND;
#endif

  /*
    If there is a system-modal window around (like a message box), then
    force the mouse events to go to either the system-modal window or
    one of its children.
  */
  if (InternalSysParams.hWndSysModal)
  {
    if (!IsChild(InternalSysParams.hWndSysModal, hWnd))
      return InternalSysParams.hWndSysModal;
  }

  return hWnd;
}


#ifdef MEWEL_GUI

static HWND PASCAL _DetermineClickOwner(hWnd, pt, piHitCode)
  HWND  hWnd;
  POINT pt;
  INT   *piHitCode;
{
  DWORD  dwFlags;
  HWND   hChild;
  INT    wHitCode;
  WINDOW *w = WID_TO_WIN(hWnd);

  dwFlags = w->flags;

  if ((dwFlags & WS_DISABLED) || !(dwFlags & WS_VISIBLE))
    return NULLHWND;

  if (!PtInRect(&w->rect, pt))
    return NULLHWND;

  /*
    If the mouse is in the window's client area, then see if it is
    in one of the window's children. (Do this recursively).
    If the mouse in not in the window's client area, then it can still
    be in a child which is a popup window.
  */
  if (!(dwFlags & WS_MINIMIZE))
  {
    BOOL bInClient = PtInRect(&w->rClient, pt);
    for (w = w->children;  w;  w = w->sibling)
    {
      if (bInClient || (w->flags & WS_POPUP))
      {
        if ((hChild = _DetermineClickOwner(w->win_id, pt, piHitCode)) != NULL)
        {
          hWnd = hChild;
          break;
        }
      }
    }
  }

  /*
    Send the WM_NCHITTEST message to the window in order to try to narrow
    down which part of the window the mouse was clicked in.
  */
  *piHitCode = wHitCode =
    (INT) SendMessage(hWnd, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));

  switch (wHitCode)
  {
    case HTTRANSPARENT :
      return NULLHWND;
    case HTMENU        :
      return GetMenu(hWnd);
#if 0
    /*
      Don't process HTSYSMENU, since MEWEL sends the message directly
      to the parent window itself.
    */
    case HTSYSMENU     :
      return GetSystemMenu(hWnd, FALSE);
#endif
    case HTVSCROLL     :
    case HTHSCROLL     :
    {
      HWND hVSB, hHSB;
      WinGetScrollbars(hWnd, &hHSB, &hVSB);
      *piHitCode = HTCLIENT;
      return (wHitCode == HTVSCROLL) ? hVSB : hHSB;
    }
    default            :
      return hWnd;
  }
}

#endif /* MEWEL_GUI */


/*
x #define HTNOWHERE	    0
x #define HTCLIENT      1
x #define HTCAPTION	    2
x #define HTSYSMENU	    3
x #define HTSIZE        4
x #define HTMENU        5
x #define HTHSCROLL     6
x #define HTVSCROLL	    7
x #define HTMINBUTTON   8
x #define HTMAXBUTTON   9
x #define HTLEFT        10
x #define HTRIGHT       11
x #define HTTOP         12
x #define HTTOPLEFT	    13
x #define HTTOPRIGHT    14
x #define HTBOTTOM      15
x #define HTBOTTOMLEFT  16
x #define HTBOTTOMRIGHT 17
x #define HTBORDER      18
*/

#define NUMHITRECTS  (HTBORDER - HTNOWHERE + 1)


/****************************************************************************/
/*                                                                          */
/* Function : DoNCHitTest(hWnd, pt)                                         */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : The HT_xxx code corresponding to the passed point.            */
/*                                                                          */
/* Called by: The 'case WM_NCHITTEST' code in StdWindowWinProc().           */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL DoNCHitTest(HWND hWnd, POINT pt)
{
  LPRECT lprWindow;
  int    i, idClass;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  ulFlags;

  ulFlags = w->flags;

#if defined(MOTIF)
  /*
    Minimized windows always return HTCAPTION.
  */
  if (ulFlags & WS_MINIMIZE)
    return HTCAPTION;
#endif

  /*
    See if the point lies within the window. If not, return HTNOWHERE.
  */
#if defined(MOTIF)
  if (!PtInRect(&w->rWindowRoot, pt))
#else
  if (!PtInRect(&w->rect, pt))
#endif
    return HTNOWHERE;

  /*
    Disabled windows return HTERROR.
  */
  if (!IsWindowEnabled(hWnd))
    return HTERROR;


#if defined(MEWEL_GUI) || defined(MOTIF)
  /*
    Minimized windows always return HTCAPTION.
  */
  if (ulFlags & WS_MINIMIZE)
    return HTCAPTION;
#endif

  /*
    Special processing for Frames, Statics and scrollbars
  */
  idClass = _WinGetLowestClass(w->idClass);
  if (idClass == SCROLLBAR_CLASS)
    return HTCLIENT;
  else if (idClass == FRAME_CLASS || idClass == STATIC_CLASS)
    return HTTRANSPARENT;

  /*
    Calculate all of the various rectangular areas of the window. Then
    test the point to see if it is inside a rectangle. If so, return the
    HT_xxx code which corresponds to that rectangle.
  */
  lprWindow = WinCalcWindowRects(hWnd);
  for (i = 0;  i < NUMHITRECTS;  i++)
    if (PtInRect(&lprWindow[i], pt))
    {
#ifdef MEWEL_TEXT
      /*
        We want to be able to drag an icon around if we click on the
        client area. So, simulate clicking on the caption.
      */
      if (i == HTCLIENT && (ulFlags & WS_MINIMIZE))
        return HTCAPTION;
#endif
      return i;
    }

  return HTNOWHERE;


#ifdef OLD_MEWEL_TEXT_CODE
  if ((ulFlags & WS_THICKFRAME) && !(ulFlags & WS_MINIMIZE))
  {
    r = w->rect;
    if (pt.y == r.top)
    {
      if (pt.x == r.left)
        return (ulFlags & WS_SYSMENU) ? HTCLIENT : HTTOPLEFT;
      if (pt.x == r.right)
        return (ulFlags & WS_MAXIMIZEBOX) ? HTCLIENT : HTTOPRIGHT;
      return HAS_CAPTION(ulFlags) ? HTCLIENT : HTTOP;
    }

    if (pt.y == r.bottom)
      return (pt.x == r.right) ? HTBOTTOMRIGHT :
             (pt.x == r.left)  ? HTBOTTOMLEFT  : HTBOTTOM;
    if (pt.x == r.left)
      return HTLEFT;
    if (pt.x == r.right)
      return HTRIGHT;

    /*
      If we *really* wanted to be MS Windows compatible, we can
      further isolate where on the border the mouse was hit,
      and return messages like HTTOP, HTBOTTOM, HTZOOM, HTVSCROLL, etc
    */
  }
  return HTCLIENT;
#endif

}



LPRECT FAR PASCAL WinCalcWindowRects(hWnd)
  HWND  hWnd;
{
  RECT   rWindow;
  int    i, bMax;
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags;
  HWND   hVSB, hHSB;
  BOOL   bVert, bHorz;
  int    cxFrame, cyFrame;

  static RECT arWindow[NUMHITRECTS];


  /*
    See if the point lies within the window. If not, return HTNOWHERE.
  */
#if defined(MOTIF)
  rWindow = w->rWindowRoot;
#else
  rWindow = w->rect;
#endif
  dwFlags = w->flags;

  /*
    Set all rectangles to an empty rectangle.
  */
  for (i = 0;  i < NUMHITRECTS;  i++)
    arWindow[i] = RectEmpty;

  /*
    Get the client area rectangle
  */
#if defined(MOTIF)
  arWindow[HTCLIENT] = w->rClientRoot;
#else
  arWindow[HTCLIENT] = w->rClient;
#endif


  /*
    Calculate the frame size
  */
#if defined(MEWEL_GUI) || defined(MOTIF)
  if (!(dwFlags & WS_MAXIMIZE) && (dwFlags & WS_THICKFRAME))
#else
  if ((dwFlags & WS_THICKFRAME))
#endif
  {
    cxFrame = IGetSystemMetrics(SM_CXFRAME);
    cyFrame = IGetSystemMetrics(SM_CYFRAME);
  }
  else
  {
    cxFrame = cyFrame = 0;
  }


  /*
    In the caption?
  */
  if (HAS_CAPTION(dwFlags))
  {
    SetRect(&arWindow[HTCAPTION],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.left  + cxFrame,
            rWindow.top   + cyFrame,
            rWindow.right - cxFrame,
            rWindow.top   + cyFrame + IGetSystemMetrics(SM_CYCAPTION));
#else
            rWindow.left,
            rWindow.top,
            rWindow.right,
            rWindow.top + IGetSystemMetrics(SM_CYCAPTION));
#endif

    /*
      We need to subtract out the sysmenu, minimize, and maximize icons
    */
    if ((dwFlags & WS_SYSMENU) && w->hSysMenu)
    {
      SetRect(&arWindow[HTSYSMENU],
#if defined(MEWEL_GUI) || defined(MOTIF)
              rWindow.left + cxFrame,
              rWindow.top  + cyFrame,
              rWindow.left + cxFrame + IGetSystemMetrics(SM_CXSIZE),
              rWindow.top  + cyFrame + IGetSystemMetrics(SM_CYSIZE));
#else
              rWindow.left,
              rWindow.top,
              rWindow.left + IGetSystemMetrics(SM_CXSIZE),
              rWindow.top  + IGetSystemMetrics(SM_CYSIZE));
#endif
      arWindow[HTCAPTION].left = arWindow[HTSYSMENU].right;
    }

    if (dwFlags & WS_MAXIMIZEBOX)
    {
      SetRect(&arWindow[HTMAXBUTTON],
#if defined(MEWEL_GUI) || defined(MOTIF)
              rWindow.right - cxFrame - IGetSystemMetrics(SM_CXSIZE),
              rWindow.top   + cyFrame,
              rWindow.right - cxFrame,
              rWindow.top   + cyFrame + IGetSystemMetrics(SM_CYSIZE));
#else
              rWindow.right - IGetSystemMetrics(SM_CXSIZE),
              rWindow.top,
              rWindow.right,
              rWindow.top   + IGetSystemMetrics(SM_CYSIZE));
#endif
      arWindow[HTCAPTION].right -= IGetSystemMetrics(SM_CXSIZE);
      bMax = 2;
    }
    else
      bMax = 1;

    if ((dwFlags & WS_MINIMIZEBOX) && !(dwFlags & WS_MINIMIZE))
    {
      SetRect(&arWindow[HTMINBUTTON],
#if defined(MEWEL_GUI) || defined(MOTIF)
              rWindow.right - cxFrame - (bMax * IGetSystemMetrics(SM_CXSIZE)),
              rWindow.top   + cyFrame,
              rWindow.right - cxFrame - ((bMax-1) * IGetSystemMetrics(SM_CXSIZE)),
              rWindow.top   + cyFrame + IGetSystemMetrics(SM_CYSIZE));
#else
              rWindow.right - ((bMax) * IGetSystemMetrics(SM_CXSIZE)),
              rWindow.top,
              rWindow.right - ((bMax-1) * IGetSystemMetrics(SM_CXSIZE)),
              rWindow.top   + IGetSystemMetrics(SM_CYSIZE));
#endif
      arWindow[HTCAPTION].right -= IGetSystemMetrics(SM_CXSIZE);
    }
  }  

  /*
    Calculate the menubar rectangle. Notice that we use rWindow as the
    base of the right side instead of using rClient. This is because we
    do not want to take a vertical scrollbar into account.
  */
  if (w->hMenu)
  {
    GetWindowRect(w->hMenu, &arWindow[HTMENU]);
  }


  /*
    Try the scrollbars
  */
  bVert = bHorz = FALSE;
  WinGetScrollbars(hWnd, &hHSB, &hVSB);
  if (hVSB)
  {
    w = WID_TO_WIN(hVSB);
    if (w->flags & WS_VISIBLE)
    {
      arWindow[HTVSCROLL] = w->rect;
      bVert = TRUE;
    }
  }
  if (hHSB)
  {
    w = WID_TO_WIN(hHSB);
    if (w->flags & WS_VISIBLE)
    {
      arWindow[HTHSCROLL] = w->rect;
      bHorz = TRUE;
    }
  }

  /*
    If it's in the area between the ends of the horz and vertical 
    scrollbars, and if the window does not have a frame, then
    it's in the GROWBOX
  */
  if (bVert && bHorz && cxFrame == 0)
  {
    SetRect(&arWindow[HTGROWBOX],
            rWindow.right  - cxFrame - IGetSystemMetrics(SM_CXVSCROLL),
            rWindow.bottom - cyFrame - IGetSystemMetrics(SM_CYHSCROLL),
            rWindow.right  - cxFrame,
            rWindow.bottom - cyFrame);
  }

  /*
    Try the various frame areas
  */
#if defined(MEWEL_GUI) || defined(MOTIF)
  if (cxFrame)
  {
#else
  if (cxFrame || HAS_BORDER(dwFlags))
  {
    cxFrame = cyFrame = 0;
#endif
    SetRect(&arWindow[HTTOPLEFT],
            rWindow.left,
            rWindow.top,
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.left + cxFrame*2,
            rWindow.top  + cyFrame*2);
#else
            rWindow.left + 1,
            rWindow.top  + 1);
#endif

    SetRect(&arWindow[HTTOPRIGHT],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.right - cxFrame*2,
            rWindow.top,
            rWindow.right,
            rWindow.top  + cyFrame*2);
#else
            rWindow.right - 1,
            rWindow.top,
            rWindow.right,
            rWindow.top  + 1);
#endif

    SetRect(&arWindow[HTTOP],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.left + cxFrame*2,
            rWindow.top,
            rWindow.right - cxFrame*2,
            rWindow.top  + cyFrame);
#else
            rWindow.left + 1,
            rWindow.top,
            rWindow.right - 1,
            rWindow.top + 1);
#endif

    SetRect(&arWindow[HTBOTTOMLEFT],
            rWindow.left,
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.bottom - cyFrame*2,
            rWindow.left   + cxFrame*2,
#else
            rWindow.bottom - 1,
            rWindow.left   + 1,
#endif
            rWindow.bottom);

    SetRect(&arWindow[HTBOTTOMRIGHT],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.right  - cxFrame*2,
            rWindow.bottom - cyFrame*2,
#else
            rWindow.right  - 1,
            rWindow.bottom - 1,
#endif
            rWindow.right,
            rWindow.bottom);

    SetRect(&arWindow[HTBOTTOM],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.left + cxFrame*2,
            rWindow.bottom - cyFrame,
            rWindow.right - cxFrame*2,
            rWindow.bottom);
#else
            rWindow.left + 1,
            rWindow.bottom - 1,
            rWindow.right - 1,
            rWindow.bottom);
#endif

    SetRect(&arWindow[HTLEFT],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.left,
            rWindow.top + cyFrame*2,
            rWindow.left + cxFrame,
            rWindow.bottom - cyFrame*2);
#else
            rWindow.left,
            rWindow.top + 1,
            rWindow.left + 1,
            rWindow.bottom - 1);
#endif

    SetRect(&arWindow[HTRIGHT],
#if defined(MEWEL_GUI) || defined(MOTIF)
            rWindow.right - cxFrame,
            rWindow.top + cyFrame*2,
            rWindow.right,
            rWindow.bottom - cyFrame*2);
#else
            rWindow.right - 1,
            rWindow.top + 1,
            rWindow.right,
            rWindow.bottom - 1);
#endif
  }


  return arWindow;
}

