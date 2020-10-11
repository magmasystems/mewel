/*===========================================================================*/
/*                                                                           */
/* File    : WINZOOM.C                                                       */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

BOOL bIsaWindowZoomed = FALSE;    /* TRUE if there's a zoomed window */

static BOOL PASCAL WinRestore(HWND);

#ifdef USE_ICON_TITLE
static HWND PASCAL WinCreateIconTitle(HWND);
#endif


BOOL FAR PASCAL WinZoom(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  MINMAXINFO MinMax;
  HWND   hOldFocus;
  DWORD  dwFlags;
  PRESTOREINFO pRestoreInfo;

  /*
    Make sure that the window has min-max capabilities
  */
  if (!w)
    return FALSE;
  dwFlags = w->flags;

  /*
    If the window is an icon, see if we're allowed to open it.
  */
  if (IsIconic(hWnd))
    if (!SendMessage(hWnd, WM_QUERYOPEN, 0, 0L))
      return FALSE;

  /*
    If it's iconic, then restore the window first.
  */
  if (IsIconic(hWnd))
    WinRestore(hWnd);


  /*
    If the window who has the focus is an edit window, send a WM_KILLFOCUS
    message so the cursor and highlighting are hidden.
  */
  if ((hOldFocus = InternalSysParams.hWndFocus) != NULLHWND)
    if (!IsChild(hWnd, InternalSysParams.hWndFocus))
      SetFocus(hWnd);
    else
      SendMessage(hOldFocus, WM_KILLFOCUS, 0, 0L);

  /*
    If the window is already zoomed, then restore it and return.
  */
  if (dwFlags & WS_MAXIMIZE)
    return WinRestore(hWnd);

  /*
    There is special zoom processing for MDI document windows.
  */
  if (IS_MDIDOC(w))
    return (BOOL) SendMessage(GetParent(hWnd), WM_MDIMAXIMIZE, hWnd, 0L);

  /*
    If there's a zoomed window already, we can't zoom a second one!
  */
  if (bIsaWindowZoomed)
    goto bye;

  /*
    Get a pointer to the restore-info structure...
  */
  if ((pRestoreInfo = (PRESTOREINFO) emalloc(sizeof(RESTOREINFO))) == NULL)
    return FALSE;
  w->pRestoreInfo = (PSTR) pRestoreInfo;

  /*
    Save the old window dimensions & turn on the maximize bit
  */
  pRestoreInfo->rect = w->rect;
  pRestoreInfo->ulOldFlags = w->flags;
  bIsaWindowZoomed = TRUE;
  w->flags |= WS_MAXIMIZE;
  w->flags &= ~WS_MINIMIZE;

  /*
    We want the maximized window to be as large as its root's client
    area. If it's a top-level window, then we can make it as big as the
    screen.
  */
  if (!w->parent)
  {
    MinMax.ptMaxSize.x = SysGDIInfo.rectScreen.right;
    MinMax.ptMaxSize.y = SysGDIInfo.rectScreen.bottom;
  }
  else
  {
    WINDOW *wParent;
    /*
      Go up until we hit the root window... then take its client area
    */
    for (wParent = w->parent;
         wParent->parent && wParent->parent != InternalSysParams.wDesktop;
         wParent = wParent->parent)
      ;
    MinMax.ptMaxSize.x = wParent->rClient.right  - wParent->rClient.left;
    MinMax.ptMaxSize.y = wParent->rClient.bottom - wParent->rClient.top;
  }

  /*
    Give the app a chance to impose its window coordinates on us....
  */
  SendMessage(hWnd, WM_GETMINMAXINFO, 0, (DWORD) (LPSTR) &MinMax);

  /*
    Move and resize the window...
  */
  w->ulStyle |= WIN_SEND_WMSIZE; /* 4/7/93(maa)- force WinMove & WinSetSize */
  SetWindowPos(hWnd, NULLHWND,
               0, 0, MinMax.ptMaxSize.x, MinMax.ptMaxSize.y,
               SWP_SHOWWINDOW | SWP_NOZORDER);
  /*
    Calculate the new non-client and client area
  */
  InternalInvalidateWindow(hWnd, TRUE);

bye:
  /*
    If the window who has the focus is an edit window, send a WM_SETFOCUS
    message so the cursor and highlighting are made visible again.
    Note - if we shrunk the window, then the edit control may not be
    visible.
  */
  if (hOldFocus && (hOldFocus == hWnd || IsChild(hWnd, hOldFocus)))
  {
    InternalSysParams.hWndFocus = hOldFocus;
    SendMessage(hOldFocus, WM_SETFOCUS, 0, 0L);
    WinRefreshActiveTitleBar(hOldFocus);
  }

  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function :  WinMinimize()                                                */
/*                                                                          */
/* Purpose  :  "Iconizes" a window... ie, reduced it to its minimum size.   */
/*                                                                          */
/* Returns  :  TRUE if minimized, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL WinMinimize(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *wParent;
  RECT   rMin;
  INT    x, y, cx, cy;
  HWND   hOldFocus;
  PRESTOREINFO pRestoreInfo;
  DWORD  dwFlags;

  if (!w)
    return FALSE;

  dwFlags = w->flags;

  /*
    If the window is already an icon, restore it and return.
  */
  if (dwFlags & WS_MINIMIZE)
    return WinRestore(hWnd);

  /*
    If the window is maximized, then restore it first
  */
  if (dwFlags & WS_MAXIMIZE)
    WinRestore(hWnd);

  /*
    If the window who has the focus is an edit window, send a WM_KILLFOCUS
    message so the cursor and highlighting are hidden.
  */
  if ((hOldFocus = InternalSysParams.hWndFocus) != NULLHWND)
    if (!IsChild(hWnd, InternalSysParams.hWndFocus))
      SetFocus(hWnd);
    else
      SendMessage(hOldFocus, WM_KILLFOCUS, 0, 0L);
  
  if ((wParent = w->parent) == (WINDOW *) NULL)
    wParent = InternalSysParams.wDesktop;

  /*
    Get a pointer to the restore-info structure...
  */
  if ((pRestoreInfo = (PRESTOREINFO) emalloc(sizeof(RESTOREINFO))) == NULL)
    return FALSE;
  w->pRestoreInfo = (PSTR) pRestoreInfo;

  /*
    Save the old window dimensions & turn on the minimize bit
  */
  pRestoreInfo->rect = w->rect;
  pRestoreInfo->ulOldFlags = dwFlags;

  w->flags |= WS_MINIMIZE;

#if defined(MOTIF) && 0
  {
  extern VOID PASCAL _XIconify(Widget);
  _XIconify(w->widget);
  return TRUE;
  }
#endif

  /*
    Determine the coordinates where the icon should be placed within the
    parent window.
  */
  _GetIconXY(-1, hWnd, &x, &y, &cx, &cy);

  /*
    Give the app a chance to impose its window coordinates on us....
  */
  SetRect(&rMin, x, y, x+cx, y+cy);

  /*
    Do the moving and sizing. Place the iconic window on the bottom
    of the window stack. Since the active window is now iconic,
    activate the first top-level window.
    Windows Compatibility Note : We should also hide the owned popups.
  */
#if !defined(MOTIF)
  if (dwFlags & WS_CHILD)
    WinScreenRectToClient(wParent->win_id, (LPRECT) &rMin);
#endif
#if defined(MOTIF)
  if (w->widgetDrawingArea)
  {
    XtUnmanageChild(w->widgetDrawingArea);
    XSysParams.ulFlags |= XFLAG_DONT_RESIZE_DRAWINGAREA;
  }
#endif
  SetWindowPos(hWnd, (HWND) 1, rMin.left, rMin.top,
               RECT_WIDTH(rMin), RECT_HEIGHT(rMin),
               SWP_NOACTIVATE | SWP_SHOWWINDOW);
#if defined(MOTIF)
  XSysParams.ulFlags &= ~XFLAG_DONT_RESIZE_DRAWINGAREA;
#endif

  if (GetActiveWindow() == hWnd)
    _WinActivateFirstWindow();

#ifdef USE_ICON_TITLE
  pRestoreInfo->hIconTitle = WinCreateIconTitle(hWnd);
#endif

  return TRUE;
}


static BOOL PASCAL WinRestore(hWnd)
  HWND hWnd;
{
  WINDOW       *w = WID_TO_WIN(hWnd);
  RECT         rOrig;
  PRESTOREINFO pRestoreInfo;
  BOOL         bIconic;
  HWND         hOldFocus;

  /*
    Make sure the window is minimized or maximized.
  */
  if (!(w->flags & (WS_MINIMIZE | WS_MAXIMIZE)))
    return FALSE;

  /*
    First of all, see if we're allowed to restore the icon
  */
  if ((bIconic = IsIconic(hWnd)) == TRUE)
    if (!SendMessage(hWnd, WM_QUERYOPEN, 0, 0L))
      return FALSE;

  /*
    Get the original window area
  */
  pRestoreInfo = (PRESTOREINFO) w->pRestoreInfo;
  rOrig = pRestoreInfo->rect;


  /*
    If we are unzooming a window, then turn off the global flag which
    tells MEWEL that there is a zoomed window on the screen.
  */
  if (!bIconic)
    bIsaWindowZoomed = FALSE;

#ifdef USE_ICON_TITLE
  WinDestroyIconTitle(w);
#endif

  /*
    Get rid of the maximize and minimize bits
  */
  w->flags = pRestoreInfo->ulOldFlags;
  w->flags &= ~(WS_MAXIMIZE | WS_MINIMIZE);

  /*
    Move the window back to its original position.
  */
#if !defined(MOTIF)
  if (w->flags & WS_CHILD)
    WinScreenRectToClient(GetParentOrDT(hWnd), (LPRECT) &rOrig);
#endif

  w->ulStyle |= WIN_SEND_WMSIZE; /* 4/7/93(maa)- force WinMove & WinSetSize */
#if defined(MOTIF)
  if (w->widgetDrawingArea && bIconic)
  {
    XtManageChild(w->widgetDrawingArea);
    XSysParams.ulFlags |= XFLAG_DONT_RESIZE_DRAWINGAREA;
  }
#endif
  SetWindowPos(hWnd, NULLHWND,
               rOrig.left, rOrig.top, RECT_WIDTH(rOrig), RECT_HEIGHT(rOrig),
               SWP_SHOWWINDOW);
#if defined(MOTIF)
  XSysParams.ulFlags &= ~XFLAG_DONT_RESIZE_DRAWINGAREA;
#endif

  /*
    Calculate the new non-client and client area
  */
  InternalInvalidateWindow(hWnd, TRUE);

  /*
    Windows Compatibility Note :
      We should show owned popups
  */

  /*
    If the window who has the focus is an edit window, send a WM_SETFOCUS
    message so the cursor and highlighting are made visible again.
    Note - if we shrunk the window, then the edit control may not be
    visible.
  */
  hOldFocus = InternalSysParams.hWndFocus;
  if (hOldFocus && IsChild(hWnd, hOldFocus) /* && 
                   WinGetClass(hOldFocus) == EDIT_CLASS */ )
  {
    SendMessage(hOldFocus, WM_SETFOCUS, 0, 0L);
    WinRefreshActiveTitleBar(hOldFocus);
  }

  MyFree(pRestoreInfo);
  w->pRestoreInfo = NULL;
  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : _GetIconXY()                                                  */
/*                                                                          */
/* Purpose  : Given a window, calculates its icon position in the parent.   */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _GetIconXY(idxIcon, hWnd, x, y, cx, cy)
  INT    idxIcon;    /* 0-based index of icon, -1 doing the last icon */
  HWND   hWnd;
  INT    *x,  *y;
  INT    *cx, *cy;
{
  HWND   hParent;
  WINDOW *wChild, *wParent;
#ifdef ICON_ARRANGE_HORIZONTALLY
  INT    nIconsPerRow;
#else
  INT    nIconsPerColumn;
#endif
  RECT   rcParent;

  /*
    Get a pointer to the iconic window's parent
  */
  hParent = GetParentOrDT(hWnd);
  wParent = WID_TO_WIN(hParent);

  /*
    If an index of -1 is passed, then we must figure out the index of the
    icon. This is the number of currently iconized windows minus 1.
  */
  if (idxIcon == -1)
  {
    for (idxIcon=0, wChild=wParent->children;  wChild;  wChild=wChild->sibling)
      if (wChild->flags & WS_MINIMIZE)
        idxIcon++;
    if (idxIcon > 0)
      idxIcon--;  /* make it 0-based */
  }

  /*
    Get the area in the parent where the icons will fit into.
  */
  rcParent = wParent->rClient;

#ifdef ICON_ARRANGE_HORIZONTALLY
  /*
    Some of you users might want to arrange the icons across the bottom
    of the screen instead of on the right side. If you do, then just
    #define the constant above.
  */
  nIconsPerRow = RECT_WIDTH(rcParent) / IGetSystemMetrics(SM_CXICONSPACING);
  *x = (idxIcon % nIconsPerRow) * IGetSystemMetrics(SM_CXICONSPACING);
  *y = rcParent.bottom -
        ((idxIcon / nIconsPerRow + 1) * IGetSystemMetrics(SM_CYICONSPACING);
#else
  nIconsPerColumn = RECT_HEIGHT(rcParent) / IGetSystemMetrics(SM_CYICONSPACING);
  *x = rcParent.right - 
        (idxIcon / nIconsPerColumn + 1) * IGetSystemMetrics(SM_CXICONSPACING);
  *y = rcParent.bottom -
        (idxIcon % nIconsPerColumn + 1) * IGetSystemMetrics(SM_CYICONSPACING);
#endif

  /*
    Determine the height and width of the icon.
  */
  *cx = ICONWIDTH;
  *cy = ICONHEIGHT;

#if defined(MOTIF) && 0
  /*
    For MOTIF, keep window at the same origin, or else MWM might do
    funny things.
  */
  wChild = WID_TO_WIN(hWnd);
  *x = wChild->rWindowRoot.left;
  *y = wChild->rWindowRoot.top;
#endif
}


#ifdef USE_ICON_TITLE

static LONG FAR PASCAL IconTitleWndProc(HWND, UINT, WPARAM, LPARAM);

static HWND PASCAL WinCreateIconTitle(hWnd)
  HWND hWnd;
{
  static BOOL bFirstTime = TRUE;

  TEXTMETRIC tm;
  HWND   hWndTitle;
  RECT   r;
  INT    cxHalfSpacing, sLen, nLines;
  WINDOW *w;
  HDC    hDC;
#ifdef NOTYET
  HFONT  hOldFont;
#endif


  /*
    Register the icon title window class
  */
  if (bFirstTime)
  {
    WNDCLASS *pWC = (WNDCLASS *) emalloc(sizeof(WNDCLASS));
    if (pWC)
    {
      pWC->lpfnWndProc   = (WINPROC *) IconTitleWndProc;
      pWC->lpszClassName = (LPSTR) "#32772";
      pWC->cbWndExtra    = sizeof(HWND);
      RegisterClass(pWC);
    }
    MyFree(pWC);
    bFirstTime++;
  }

  w = WID_TO_WIN(hWnd);

  /*
    Get a DC for the font operations.
    We want to draw the icon window with the small font.
  */
  hDC = GetDC(hWnd);
#ifdef NOTYET
  hOldFont = SelectObject(hDC, InternalSysParams.hSmallFont);
#endif
  GetTextMetrics(hDC, &tm);

  /*
    Calculate the amount of spacing we need to adjust the icon title
    by. For instance, in this example, cxHalfSpacing is 20.

                   32 (SM_CXICON)
                 ------
        <- 20 -> |    | <- 20 ->
                 ------
        ------------------------
        ------------------------
                   72 (SM_CXICONSPACING)

  */

  sLen = LOWORD(GetTextExtent(hDC, w->title, lstrlen(w->title)));
  if (sLen > IGetSystemMetrics(SM_CXICONSPACING))
  {
    nLines = sLen / IGetSystemMetrics(SM_CXICONSPACING) + 1;
    sLen = IGetSystemMetrics(SM_CXICONSPACING);
  }
  else
    nLines = 1;

  cxHalfSpacing = (sLen - IGetSystemMetrics(SM_CXICON)) / 2;

  /*
    Use DrawText() to figure out the proper height of the icon title.
  */
  SetRect(&r, w->rect.left - cxHalfSpacing,
              w->rect.top + IGetSystemMetrics(SM_CYICON) + 4, 
              w->rect.left - cxHalfSpacing + sLen,
              w->rect.top + IGetSystemMetrics(SM_CYICON) + 4 +
                 (nLines * tm.tmHeight));
  DrawText(hDC, w->title, -1, &r,
               DT_CALCRECT | DT_CENTER | DT_VCENTER | DT_WORDBREAK);

#ifdef NOTYET
  SelectObject(hDC, hOldFont);
#endif
  ReleaseDC(hWnd, hDC);

#if !defined(MOTIF)
  if (w->hWndOwner)
    WinScreenRectToClient(w->hWndOwner, &r);
#endif

  hWndTitle = CreateWindow("#32772",
                           w->title,
                           WS_CHILD | WS_CLIPSIBLINGS,
                           r.left,
                           r.top,
                           RECT_WIDTH(r),
                           RECT_HEIGHT(r)+2, /* 2 for spacing before & after */
                           w->hWndOwner,
                           0,
                           0,
                           NULL);

  if (hWndTitle)
  {
    /*
      Flag the window as being an icon title and link it to the
      icon via the hWndOwner field.
    */
    w = WID_TO_WIN(hWndTitle);
    w->ulStyle |= WIN_IS_ICONTITLE;
    w->hWndOwner = hWnd;
    SetWindowWord(hWndTitle, 0, (WORD) hWnd);  /* set the parent ptr */
    ShowWindow(hWndTitle, SW_SHOWNOACTIVATE);
  }
  return hWndTitle;
}


static LRESULT FAR PASCAL 
IconTitleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HWND  hParent = (HWND) GetWindowWord(hWnd, 0);

  switch (message)
  {
    case WM_PAINT :
    {
      PAINTSTRUCT ps;
      RECT        r;
      WINDOW      *w = WID_TO_WIN(hWnd);
      int         sLen = lstrlen(w->title);
#ifdef NOTYET
      HFONT  hOldFont;
#endif

      BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
      GetClientRect(hWnd, &r);
#ifdef NOTYET
      hOldFont = SelectObject(ps.hdc, InternalSysParams.hSmallFont);
#endif
      SetBkMode(ps.hdc, TRANSPARENT);
      DrawText(ps.hdc, w->title, sLen, &r, 
               DT_CENTER | DT_VCENTER | DT_WORDBREAK);
#ifdef NOTYET
      SelectObject(ps.hdc, hOldFont);
#endif
      EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
      return TRUE;
    }

    case WM_ERASEBKGND :
    {
      RECT   r;
      HBRUSH hBrush;

      GetClientRect(hWnd, (LPRECT) &r);

      /*
        If the icon is the active window, use the caption color to
        fill the text background.
      */
      if (InternalSysParams.hWndActive == hWnd || 
          InternalSysParams.hWndActive == hParent)
        hBrush = SysBrush[COLOR_ACTIVECAPTION];
      else
      {
        WINDOW *wParent = WID_TO_WIN(hParent);
        hBrush = SysBrush[IS_MDIDOC(wParent) ? COLOR_APPWORKSPACE : COLOR_WINDOW];
      }
      FillRect((HDC) wParam, (LPRECT) &r, hBrush);
      return TRUE;
    }


    /*
      Send all keyboard messages to the icon's window proc.
      Send all command messages to the icon's window proc.
    */
    case WM_CHAR       :
    case WM_KEYDOWN    :
    case WM_KEYUP      :
    case WM_SYSKEYUP   :
    case WM_SYSCOMMAND :
    case WM_COMMAND    :
      return CallWindowProc((FARPROC) GetClassLong(hParent, GWL_WNDPROC),
                            hParent, message, wParam, lParam);

    case WM_LBUTTONUP  :
      WinActivateSysMenu(hParent);
      return TRUE;

    default :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);
  }
}

#endif


#if defined(USE_ICON_TITLE)
VOID FAR PASCAL WinDestroyIconTitle(w)
  WINDOW *w;
{
  PRESTOREINFO pRestoreInfo;

  pRestoreInfo = (PRESTOREINFO) w->pRestoreInfo;
  if (pRestoreInfo->hIconTitle)
  {
    RECT rTitle;

    GetWindowRect(pRestoreInfo->hIconTitle, &rTitle);
    WinGenInvalidRects(_HwndDesktop, &rTitle);
    _WinDestroy(pRestoreInfo->hIconTitle);
    pRestoreInfo->hIconTitle = NULLHWND;
  }
}
#endif

