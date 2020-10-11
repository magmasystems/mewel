/*===========================================================================*/
/*                                                                           */
/* File    : WREFRESH.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  For the GUI version using a graphics engine which does not support
  region clipping, we need to refresh higher siblings whenever we
  draw into a possibly background window. Therefore, there are special
  tests in InvalidateNCArea() and WinGenInvalidRects() for this.
*/
#if defined(MEWEL_GUI) && !defined(META)
#if 0
#define INVALIDATE_SIBLINGS
#endif
#endif

#define SEND_SIZEMSG_BEFORE_PAINT   1

extern BOOL  FAR PASCAL _IsButtonClass(HWND, UINT);

static VOID PASCAL _WinGenInvalidRects(HWND, LPRECT, BOOL);
static VOID PASCAL _RefreshInvalidWindows(HWND);

#if SEND_SIZEMSG_BEFORE_PAINT
static VOID PASCAL UpdatePendingSizeMsg(HWND);
static VOID PASCAL SendSizeMessage(HWND);
#endif


VOID PASCAL RefreshInvalidWindows(hWnd)
  HWND hWnd;
{
  int    iDlg;
  BOOL   bCaretPushed;
#ifdef MEWEL_TEXT
  BOOL   bDidVirtual = FALSE;
#endif

  if (!TEST_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST))
    goto sync_caret;

  MouseHide();
  bCaretPushed = CaretPush(0, VideoInfo.length);

#ifdef MEWEL_TEXT
  if (!bVirtScreenEnabled)
    if (!VID_IN_GRAPHICS_MODE())
    {
      if (VirtualScreenEnable())
        bDidVirtual++;
    }
#endif

#if defined(XWINDOWS)
  _XFlushEvents();
#endif

#if SEND_SIZEMSG_BEFORE_PAINT
  UpdatePendingSizeMsg(hWnd);
#endif

  _RefreshInvalidWindows(hWnd);

  /*
    Now refresh any dialog boxes being shown
  */
  for (iDlg = 0;  iDlg < DlgStackSP;  iDlg++)
  {
#if SEND_SIZEMSG_BEFORE_PAINT
    UpdatePendingSizeMsg(_HDlgStack[iDlg]);
#endif
    _RefreshInvalidWindows(_HDlgStack[iDlg]);
  }

#ifdef MEWEL_TEXT
  if (bDidVirtual)
    VirtualScreenFlush();
#endif

  MouseShow();
  if (bCaretPushed)
    CaretPop();

sync_caret:
  if (TEST_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST | STATE_SYNC_CARET))
    WinUpdateCaret();
  CLR_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST | STATE_SYNC_CARET);
}


static VOID PASCAL _RefreshInvalidWindows(hWnd)
  HWND hWnd;
{
  WINDOW *w;

  if (hWnd == NULLHWND)
    return;

  /*
    If the window has an update region, then repaint it.
  */
  w = WID_TO_WIN(hWnd);
  UpdateWindow(hWnd);

  /*
    Update the scrollbars which are attached to the window
  */
  if (w->hSB[SB_HORZ])
    UpdateWindow(w->hSB[SB_HORZ]);
  if (w->hSB[SB_VERT])
    UpdateWindow(w->hSB[SB_VERT]);

  /*
    Recursively update the window's children, from bottom child upwards.
  */
  if (w->children)
  {
    /*
      For dialogs boxes, draw the children from first child to the last
    */
    if (IS_DIALOG(w))
    {
      for (w = w->children;  w;  w = w->sibling)
        _RefreshInvalidWindows(w->win_id);
    }
    else
    {
      /*
        Go to the end of the children list.
      */
      for (w = w->children;  w->sibling;  w = w->sibling)
        ;
      for (  ;  w;  w = w->prevSibling)
        _RefreshInvalidWindows(w->win_id);
    }
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : WinGenInvalidRects()                                          */
/*                                                                          */
/* Purpose  : Given a window, invalidates all windows which lie within      */
/*            a certain rectangular area.                                   */
/*                                                                          */
/* Returns  : Zilch.                                                        */
/*                                                                          */
/****************************************************************************/
VOID PASCAL WinGenInvalidRects(hWnd, lpRect)
  HWND   hWnd;
  LPRECT lpRect;
{
#if defined(USE_NATIVE_GUI)
  return;
#else

  int    iDlg;
  WINDOW *w;
  BOOL   bInvalidateSiblings = FALSE;


  /*
    10/20/92 (maa)
      When we are only refreshing a window which is overlapped by
    a higher sibling, then we must redraw the higher sibling too!
    This is because BGI has no region clipping.
  */
#if defined(INVALIDATE_SIBLINGS)
  bInvalidateSiblings = (BOOL) (hWnd != _HwndDesktop);
#endif

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL || !IsWindowVisible(hWnd))
    return;

  _WinGenInvalidRects(hWnd, lpRect, bInvalidateSiblings);

  /*
    Now refresh any dialog boxes being shown, but only if the hWnd
    we passed in was not a dialog box itself.
  */
  if (!IS_DIALOG(w))
  {
    for (iDlg = 0;  iDlg < DlgStackSP;  iDlg++)
      _WinGenInvalidRects(_HDlgStack[iDlg], lpRect, bInvalidateSiblings);
  }

  /*
    Invalidate any owned windows
  */
  if (hWnd != _HwndDesktop)
  {
    for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
      if (w->hWndOwner == hWnd)
        _WinGenInvalidRects(w->win_id, lpRect, bInvalidateSiblings);
  }
#endif /* USE_NATIVE_GUI */
}


#if !defined(USE_NATIVE_GUI)
static VOID PASCAL _WinGenInvalidRects(
  HWND   hWnd,
  LPRECT lpRect,  /* lpRect is in screen coordinates */
  BOOL   bInvalidateSiblings
)
{
  RECT   rIntersect, rClient, rScrClient;
  RECT   rScreen;
  WINDOW *w;
#if defined(INVALIDATE_SIBLINGS)
  WINDOW *wSibling;
#endif

  w = WID_TO_WIN(hWnd);

  /*
    1) See if the window is visible. It is not visible if its 'hidden'
       flag is on or if any one of its ancestors are not visible.
  */
  if (!IsWindowVisible(hWnd))
    return;

  /*
    2) Make sure that both the window rect and the clipping rect are
       bounded by the screen.
  */
  rScreen = SysGDIInfo.rectScreen;
  if (!IntersectRect(lpRect, lpRect, &rScreen))
    return;
  if (!IntersectRect(&rIntersect, &w->rect, &rScreen))
    return;

  if (_IsButtonClass(hWnd, PUSHBUTTON_CLASS))
    _WinAdjustRectForShadow(w, (LPRECT) &rIntersect);

  /*
    3) See if the window's rectangle lies within the obstructing rectangle.
  */
  if (!IntersectRect(&rIntersect, lpRect, &rIntersect))
    return;


  /*
    4) See if the window's non-client area needs to be redrawn
  */
  if (lpRect->left   < w->rClient.left  ||
      lpRect->top    < w->rClient.top   ||
      lpRect->right  > w->rClient.right ||
      lpRect->bottom > w->rClient.bottom)
  {
    w->ulStyle |= WIN_UPDATE_NCAREA;
    SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);
  }

  /*
    5) See if the window's client area is affected.
  */
  if (IntersectRect(&rScrClient, &rIntersect, &w->rClient))
  {
    /*
      Get the rectangle into client coords
    */
    rClient = rScrClient;
    WinScreenRectToClient(hWnd, &rClient);
#if defined(MEWEL_TEXT)
    if (HAS_SHADOWED_KIDS(w))
      _InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
    else
#endif
    _InvalidateRect(hWnd, (LPRECT) &rClient, TRUE);
    if (!IsIconic(hWnd) && w->children)
    {
      /*
        Go to the end of the children list.
      */
      WINDOW *wChild;

      /*
        This fixes a problem when an update region has another
        update region added to it. The children of the window should
        have their update regions modified based on the new TOTAL
        update region, not just by the new region. (Fred Needham, 4/4/94)
      */
      rClient = w->rClient;
      WinScreenRectToClient(hWnd, &rClient);
      IntersectRect(&rIntersect, &rClient, &w->rUpdate);
      WinClientRectToScreen(hWnd, &rIntersect);
      UnionRect(&rScrClient, &rScrClient, &rIntersect);

      for (wChild = w->children;  wChild->sibling;  wChild = wChild->sibling)
        ;
      for (  ;  wChild;  wChild = wChild->prevSibling)
        _WinGenInvalidRects(wChild->win_id,
#if 101092
               &rScrClient, FALSE);
#else
/*
  10/10/92 (maa)
    If a child popup lies partially over the parent and partially outside the
  parent, then the possibility exists that, when we redraw the parent, the
  part of the popup which lies inside the parent will be drawn over.
  On the minus side, a lot of extra redrawing is incurred. So, enable this
  piece of code if users complain about popups getting chopped off,
  especially after dragging dialog boxes and message boxes around the
  screen.
*/
               (wChild->flags & WS_POPUP) ? &rScreen : &rScrClient, FALSE);
#endif
    }
  }


  /*
    6) Possibly invalidate higher siblings
  */
#if defined(INVALIDATE_SIBLINGS)
  if (bInvalidateSiblings && (wSibling = w->prevSibling) != NULL)
  {
    UnionRect(lpRect, lpRect, &w->rect);
    while (wSibling)
    {
      if (IntersectRect(&rScreen, &wSibling->rect, lpRect))
      {
        _WinGenInvalidRects(wSibling->win_id, lpRect, FALSE);
        UnionRect(lpRect, lpRect, &wSibling->rect);
      }
      wSibling = wSibling->prevSibling;
    }
  }
#else
  (void) bInvalidateSiblings;
#endif
}
#endif /* USE_NATIVE_GUI */


/****************************************************************************/
/*                                                                          */
/* Function : InternalInvalidateWindow(hWnd, bErase)                        */
/*                                                                          */
/* Purpose  : Invalidates both a window's client area and the NC area.      */
/*                                                                          */
/* Returns  : Zilch.                                                        */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL InternalInvalidateWindow(HWND hWnd, BOOL bErase)
{
#if !defined(USE_NATIVE_GUI)
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != (WINDOW *) NULL)
  {
#if defined(MOTIF)
    if (!(w->dwXflags & WSX_USEMEWELSHELL))
      return;
#endif
    InvalidateRect(hWnd, (LPRECT) NULL, bErase);
    w->ulStyle |= WIN_UPDATE_NCAREA;
    SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);
  }
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : InvalidateNCArea(hWnd)                                        */
/*                                                                          */
/* Purpose  : Invalidates only a window's non-client area.                  */
/*                                                                          */
/* Returns  : Zilch.                                                        */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL InvalidateNCArea(hWnd)
  HWND hWnd;
{
#if !defined(USE_NATIVE_GUI)

#if defined(INVALIDATE_SIBLINGS)
  RECT   rNC[4], rTmp, rWindow;
  BOOL   abEmpty[4];
  INT    i;
  WINDOW *wSibling;
#endif

  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != (WINDOW *) NULL)
  {
    w->ulStyle |= WIN_UPDATE_NCAREA;
    SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);

#if defined(INVALIDATE_SIBLINGS)
    /*
      Go through all siblings which are higher in the Z-order. If they
      intersect the invalid rectangle, refresh them as well.

      First, get the 4 rectangles which constitute the difference between
      the client area and the non-client area.
    */
    rWindow = w->rect;
    RectTile(rWindow, w->rClient, rNC);
    for (i = 0;  i < 4;  i++)
      abEmpty[i] = IsRectEmpty(&rNC[i]);

    for (wSibling=w->prevSibling;  wSibling;  wSibling=wSibling->prevSibling)
    {
      if (TEST_WS_HIDDEN(wSibling))
        continue;
      for (i = 0;  i < 4;  i++)
      {
        /*
          If the child intersects any part of the non-client area,
          then invalidate that sibling.
        */
        if (!abEmpty[i] && IntersectRect(&rTmp, &rNC[i], &wSibling->rect))
        {
          WinGenInvalidRects(wSibling->win_id, &rWindow);
          return;
        }
      }
    }
#endif
  }

#endif /* USE_NATIVE_GUI */
}


/****************************************************************************/
/*                                                                          */
/* Function : UpdateWindow(hWnd)                                            */
/*                                                                          */
/* Purpose  : Repaint a window only if its update region is not empty.      */
/*                                                                          */
/* Returns  : Zilch.                                                        */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL UpdateWindow(hWnd)
  HWND hWnd;
{
  RECT   rUpdate;
  WINDOW *w;

  if (!IsWindowVisible(hWnd))
    return;

  if (GetUpdateRect(_HwndDesktop, &rUpdate, FALSE))  /* an update rect available! */
  {
    WINDOW *wChild;

    /*
      Go through the desktop's top-level children and see if they all
      obscure the desktop. If so, then do not refresh the desktop.
    */
    BOOL bUpdateDesktop = TRUE;
    for (wChild = InternalSysParams.wDesktop->children;  wChild;  wChild = wChild->sibling)
    {
      if (IsWindowVisible(wChild->win_id))
      {
        RECT rChild;
        rChild = wChild->rect;
        /*
          See if the child's rectangle completely covers the desktop's
          update area.
        */
        if (rChild.top    <= rUpdate.top    &&
            rChild.left   <= rUpdate.left   &&
            rChild.bottom >= rUpdate.bottom &&
            rChild.right  >= rUpdate.right)
        {
          bUpdateDesktop = FALSE;
          break;
        }
      }
    }
    if (bUpdateDesktop)
      SendMessage(_HwndDesktop, WM_PAINT, (WPARAM) -1, (DWORD) (LPSTR) &rUpdate);

    SetUpdateRect(_HwndDesktop, (LPRECT) NULL);  /* Validate the window */
  } /* end if desktop window needs refreshing */


  w = WID_TO_WIN(hWnd);

  /*
    If this is the first time that a window is being shown, send the
    winproc a WM_SIZE and WM_MOVE message.
  */
#if !SEND_SIZEMSG_BEFORE_PAINT
  if (w->ulStyle & WIN_SEND_WMSIZE)
  {
    w->ulStyle &= ~WIN_SEND_WMSIZE;
    MEWELSend_WMSIZE(w);
    MEWELSend_WMMOVE(w);
  }
#endif

  /*
    If the window's non-client area needs to be redrawn, send a WM_NCPAINT.
  */
  if (w->ulStyle & WIN_UPDATE_NCAREA)
  {
    SendMessage(hWnd, WM_NCPAINT, 0, 0L);
    w->ulStyle &= ~WIN_UPDATE_NCAREA;
  }


  /*
    Update the client area
  */
  if (GetUpdateRect(hWnd, &rUpdate, FALSE))  /* an update rect available! */
  {
    /*
      Send a WM_PAINT or WM_PAINTICON message
    */
    UINT wMsg;
    if ((w->flags & WS_MINIMIZE) && ClassIDToClassStruct(w->idClass)->hIcon)
      wMsg = WM_PAINTICON;
    else
      wMsg = WM_PAINT;
    SendMessage(hWnd, wMsg, (WPARAM) -1, (DWORD) (LPSTR) &rUpdate);
    SetUpdateRect(hWnd, (LPRECT) NULL);  /* Validate the window */
  }
}


/****************************************************************************/
/*                                                                          */
/*  Routines to manipulate a window's update region.                        */
/*    ValidateRect() - subtracts a rectangle                                */
/*    InvalidateRect() - adds a rectangle                                   */
/*    SetUpdateRect() - sets the entire update rectangle                    */
/*    GetUpdateRect() - retrieves the entire update rectangle               */
/*                                                                          */
/****************************************************************************/

VOID FAR PASCAL InvalidateRect(HWND hWnd, CONST RECT FAR *lpRect, BOOL bErase)
{
  RECT   rUpdate;
  WINDOW *w = WID_TO_WIN(hWnd);

  if (w)
  {
    _InvalidateRect(hWnd, lpRect, bErase);

    /*
      5/10/93 (maa)
      If we call InvalidateRect() on a window which has children
      (such as a dialog box), and the window does not clip the
      children, then we need to force the children to be refreshed.
      
      (Note : the logic below cannot be made part of _InvalidateRect
      because of the recursive calls between WinGen... and
      _InvalidateRect(). )
    */
    if (w->children && !(w->flags & WS_CLIPCHILDREN))
    {
      /*
        Get the window's update rectangle into client coordinates.
      */
      rUpdate = w->rUpdate;
      OffsetRect(&rUpdate, w->rClient.left, w->rClient.top);
      /*
        Invalidate all children who intersect the invalid area.
      */
      WinGenInvalidRects(hWnd, &rUpdate);
    }
  }
}

VOID FAR PASCAL _InvalidateRect(HWND hWnd, CONST RECT FAR *lpRect, BOOL bErase)
{
  RECT   rClient;
  RECT   rUpdate;

  GetClientRect(hWnd, &rClient);
  if (lpRect != NULL)    /* Constrain the invalid rect to the window client */
    IntersectRect(&rClient, &rClient, lpRect);
  if (GetUpdateRect(hWnd, &rUpdate, FALSE)) /* Already has an update rect? */
    UnionRect(&rClient, &rClient, &rUpdate);  /* Combine it with the new one.*/
  SetUpdateRect(hWnd, &rClient);

  if (bErase)
  {
    WINDOW *w = WID_TO_WIN(hWnd);
    if (w)
      w->ulStyle |= WIN_SEND_ERASEBKGND;
  }
}


VOID FAR PASCAL ValidateRect(hWnd, lpRect)
  HWND   hWnd;
  CONST RECT FAR *lpRect;
{
  RECT rClient;
  RECT rUpdate;
  RECT rTile[5];
  int  i;

  if (!GetUpdateRect(hWnd, &rUpdate, FALSE))  /* no update rect */
    return;

  if (lpRect == NULL)
  {
    SetUpdateRect(hWnd, (LPRECT) NULL);
    return;
  }

  GetClientRect(hWnd, &rClient);  /* constrain to client area */
  IntersectRect(&rClient, &rClient, lpRect);

  /*
    We "subtract" the validated rect from the existing update rect by tiling
    the update rect, and unioning all of the tiled parts.
  */
  RectTile(rUpdate, rClient, rTile);
  rUpdate = RectEmpty;
  for (i = 0;  i < 4;  i++)
    if (!IsRectEmpty(&rTile[i]))
      UnionRect(&rUpdate, &rUpdate, &rTile[i]);

  SetUpdateRect(hWnd, &rUpdate);
}


INT FAR PASCAL GetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase)
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) != NULL && !IsRectEmpty(&w->rUpdate))
  {
    /*
      Get the update rect. Erase the window if the user wants to.
    */
    if (lpRect == NULL)
      return TRUE;

    *lpRect = w->rUpdate;
    if (fErase)
    {
      HDC hDC = GetDC(hWnd);
      SendMessage(hWnd, 
                 ((w->flags & WS_MINIMIZE) && ClassIDToClassStruct(w->idClass)->hIcon)
                  ? WM_ICONERASEBKGND : WM_ERASEBKGND,
                 (WPARAM) hDC, 0L);
      ReleaseDC(hWnd, hDC);
    }
    return TRUE;
  }
  else
  {
    if (lpRect)
      *lpRect = RectEmpty;
    return FALSE;
  }
}


VOID FAR PASCAL SetUpdateRect(hWnd, lpRect)
  HWND   hWnd;
  LPRECT lpRect;
{
  /*
    Set and return w->rUpdate
  */
  WINDOW *w;
  if ((w = WID_TO_WIN(hWnd)) != NULL)
    if (lpRect)
    {
      w->rUpdate = *lpRect;
      SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);
    }
    else
      w->rUpdate = RectEmpty;
}


/****************************************************************************/
/*                                                                          */
/* Function : WinGenAncestorClippingRect                                    */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/


/*
  Define as a macro in order to speed things up
*/
#define RECTINTERSECT(r, r1, r2)  \
      RECTGEN(r, max(r1.top,r2.top), max(r1.left,r2.left), \
              min(r1.bottom,r2.bottom), min(r1.right,r2.right))

#define RECTISEMPTY(r)  (r.top >= r.bottom || r.left >= r.right)
#define RECTCONTAINSPOINT(r, row, col) \
          (col >= r.left && col < r.right && row >= r.top && row < r.bottom)


/*
  Turbo C++, and maybe some other compilers, will call a far function to
  copy a RECT structure. On the other hand, MSC will do it in-line. So,
  define a macro for a RECT copy operation.
*/
#if defined(__TURBOC__)
#define RECTCOPY(r1,r2)  RECTGEN(r1,(r2).top,(r2).left,(r2).bottom,(r2).right)
#else
#define RECTCOPY(r1,r2)  (r1) = (r2)
#endif


int FAR PASCAL WinGenAncestorClippingRect(hWnd, hDC, rReturn)
  HWND   hWnd;
  HDC    hDC;
  LPRECT rReturn;
{
#if defined(MOTIF)
  return FALSE;
#else

  WINDOW *w = WID_TO_WIN(hWnd);
  WINDOW *wParent;
  RECT   rClipping, rParent, rScreen;
  DWORD  dwFlags;
  BOOL   bIsMenu, bIsScrollbar;
  LPHDC  lphDC;

  if (!w)
    return FALSE;

  /*
    Generate a clipping rectangle which is the size of the screen. Then
    clip the window that we're drawing on to that.
  */
  rScreen = SysGDIInfo.rectScreen;
  RECTCOPY(rClipping, rScreen);

  dwFlags = w->flags;

  /*
    Get a pointer to the optional DC
  */
  if (hDC != 0 && hDC != (HDC) -1)
    lphDC = _GetDC(hDC);
  else
    lphDC = (LPHDC) NULL;

  /*
    If the window isn't visible and the DC is not a memory DC, return FALSE.
    If it's a memory DC, then there is probably a bitmap attached tp it.
  */
  if (!IsWindowVisible(hWnd) && (lphDC == NULL || !IS_MEMDC(lphDC)))
    return FALSE;

  /*
    Get some stuff into local vars for speed...
  */
  wParent = w->parent;
  bIsMenu = (BOOL) ((w->ulStyle & WIN_IS_MENU) != 0);
  bIsScrollbar = (BOOL) (w->idClass == SCROLLBAR_CLASS);


  /*
    Clip to the entire window if we are drawing a border, or else clip
    to the client area. 
  */
  if (bDrawingBorder || hDC == (HDC) -1 ||  hDC != 0 && IS_WINDOWDC(lphDC))
  {
    RECTCOPY(rParent, w->rect);
  }
  else
  {
    RECTCOPY(rParent, w->rClient);
  }
  RECTINTERSECT(rClipping, rClipping, rParent);

  /*
    If there is a clipping region in the device context, clip to that.
  */
  if (hDC != 0 && hDC != (HDC) -1)
  {
    RECT  rDC;
    RECTCOPY(rDC, lphDC->rClipping);
    RECTINTERSECT(rClipping, rClipping, rDC);
  }

  /*
    Certain windows should not be clipped to the parent (such as listbox
    portions of comboboxes)
  */
  if (w->ulStyle & WIN_NOCLIP_TO_PARENT)
    goto byebye;

#if 1
  /*
    1/20/92 (maa)
      Try letting popups roam free...
    Unfortunately, WinUpdateVisMap() puts menus *above* the popup
    windows
  */
  if (w->flags & WS_POPUP)
    goto byebye;
#endif

  /*
    A pulldown menu should not be clipped to the parent (a menubar or a higher
    level pulldown). Instead, it should be clipped to the screen.
    The same goes for a pulldown menu off a system menu.
  */
  if (bIsMenu && wParent &&
      ((wParent->flags & WS_SYSMENU) || (wParent->ulStyle & WIN_IS_MENU)))
    goto byebye;

  /*
    Go through all of the ancestors and clip to their client areas.
  */
  for (  ;  wParent;  wParent = wParent->parent)
  {
    /*
       A scrollbar should be clipped to its immediate parent's border, not
       client area.
       The same thing goes for menubars (since the client area starts
       below the menubar).
    */
    if (((bIsScrollbar && !(dwFlags&SBS_CTL)) || bIsMenu) && w->parent==wParent)
    {
      RECTCOPY(rParent, wParent->rect);
    }
    else
    {
      RECTCOPY(rParent, wParent->rClient);
    }
    RECTINTERSECT(rClipping, rClipping, rParent);
    if (RECTISEMPTY(rClipping))
    {
      RECTCOPY(*rReturn, RectEmpty);
      return FALSE;
    }

    if (wParent->ulStyle & WIN_NOCLIP_TO_PARENT)
      break;

#if 1
    if (wParent->flags & WS_POPUP)
      break;
#endif
  }


byebye:
  /*
    Do one final check to make sure that those "non-clipped" windows
    get clipped to the physical screen.
  */
  RECTINTERSECT(rClipping, rClipping, rScreen);
  if (RECTISEMPTY(rClipping))
  {
    RECTCOPY(*rReturn, RectEmpty);
    return FALSE;
  }

  RECTCOPY(*rReturn, rClipping);
  return TRUE;
#endif
}



#if SEND_SIZEMSG_BEFORE_PAINT
static VOID PASCAL UpdatePendingSizeMsg(hWnd)
  HWND hWnd;
{
  WINDOW *w;

  if (hWnd == NULLHWND)
    return;
  if (!IsWindowVisible(hWnd))
    return;

  SendSizeMessage(hWnd);

  w = WID_TO_WIN(hWnd);
  if (w->hSB[SB_HORZ])
    SendSizeMessage(w->hSB[SB_HORZ]);
  if (w->hSB[SB_VERT])
    SendSizeMessage(w->hSB[SB_VERT]);
  if (w->children)
  {
    for (w = w->children;  w;  w = w->sibling)
      UpdatePendingSizeMsg(w->win_id);
  }
}


static VOID PASCAL SendSizeMessage(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);

  /*
    If this is the first time that a window is being shown, send the
    winproc a WM_SIZE and WM_MOVE message.
  */
  if (w->ulStyle & WIN_SEND_WMSIZE)
  {
    w->ulStyle &= ~WIN_SEND_WMSIZE;
    MEWELSend_WMSIZE(w);
    MEWELSend_WMMOVE(w);
  }
}
#endif

