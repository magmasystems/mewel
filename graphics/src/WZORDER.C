/*===========================================================================*/
/*                                                                           */
/* File    : WZORDER.C                                                       */
/*                                                                           */
/* Purpose : This file contains all of the functions which deal with         */
/*           Z-Order and the window tree.                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


/*===========================================================================*/
/*                                                                           */
/* File    : WINTOTOP.C                                                      */
/*                                                                           */
/* Purpose : WinBringToTop() function                                        */
/*                                                                           */
/*===========================================================================*/

static BOOL PASCAL _WMakeTopSibling(PWINDOW);


int FAR PASCAL BringWindowToTop(hWnd)
  HWND hWnd;
{
  SetWindowPos(hWnd, NULLHWND, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  return TRUE;
}


HWND FAR PASCAL _WinTreeToTop(hWnd)
  HWND hWnd;
{
  WINDOW *wNewTop;
  WINDOW *w;
  WINDOW *wParent;
  WINDOW *wHighestToRedraw = NULL;

#if defined(MOTIF) || defined(DECWINDOWS)
#if !111594
  _XBringWindowToTop(hWnd);
#endif
  return _HwndDesktop;
#endif

  /*
    Do not rearrange the desktop window nor a minimized window.
  */
  if ((wNewTop = WID_TO_WIN(hWnd)) == NULL || wNewTop == InternalSysParams.wDesktop ||
                                             (wNewTop->flags & WS_MINIMIZE))
    return NULLHWND;

  /*
    Climb all the way up the window's ancestor tree.
    We rearrange the sibling list so that every window in the ancestor list 
    is the first child of its parent.
  */
  for (w = wNewTop;  w && w != InternalSysParams.wDesktop;  w = wParent)
  {
    /*
      wHighestParent will contain the highest window which we will have to
      redraw. We need to redraw it if the z-order has changed.
    */
    if ((wParent = w->parent) != NULL && wParent->children != w)
    {
      WINDOW *wSibling;
      /*
        We should only need to redraw the actual window which is being
        brought to the top, and not any other window.
      */
      for (wSibling=w->prevSibling;  wSibling;  wSibling=wSibling->prevSibling)
      {
        /*
          Don't move the window in front of its owned windows
        */
        if (wSibling->hWndOwner == hWnd)
          continue;
        if (IsWindowVisible(wSibling->win_id))
        {
          wHighestToRedraw = w;  /* refresh only if higher sibs are visible */
          break;
        }
      }
    }
    _WMakeTopSibling(w);
  }

  return wHighestToRedraw ? wHighestToRedraw->win_id : NULLHWND;
}


int FAR PASCAL WinToTop(hWnd)
  HWND hWnd;
{
  WINDOW *w, *wP;

  if ((w = WID_TO_WIN(hWnd)) == NULL || (w->flags & WS_MINIMIZE))
    return FALSE;

  /*
    Don't rearrange siblings if the window is a dialog control because
    that will screw up the tabbing sequence.
  */
  if ((wP = w->parent) == NULL || !IS_DIALOG(wP))
    if (_WMakeTopSibling(w))
      WinUpdateVisMap();

  return TRUE;  /* TRUE == we restacked */
}


static BOOL PASCAL _WMakeTopSibling(wNewTop)
  WINDOW *wNewTop;
{
  WINDOW *w, *prevw, *wParent;
  WINDOW **ppSibling;

  if ((wParent=wNewTop->parent) == NULL || (w=wParent->children) == wNewTop)
    return FALSE;

  /*
    Do not make a menu part of the sibling list
  */
  if (IS_MENU(wNewTop))
    return FALSE;

  /*
    Unlink from the sibling list
  */
  for (prevw = w;  w;  prevw = w, w = w->sibling)
    if (w == wNewTop)
    {
      prevw->sibling = wNewTop->sibling;
      if (w->sibling)
        w->sibling->prevSibling = prevw;
      break;
    }

  /*
    Make it the new highest child (after all of the popups, unless the
    window itself is a popup).
  */
  ppSibling = &wParent->children;
  w = wParent->children;
  prevw = NULL;

  /*
    A topmost window is automatically placed at the head of the child list.
  */
  if (!(wNewTop->dwExtStyle & WS_EX_TOPMOST))
  {
    /*
      If we have a popup window, we must span all topmost windows
    */
    if ((wNewTop->flags & WS_POPUP))
    {
      while (w != NULL && (w->dwExtStyle & WS_EX_TOPMOST))
      {
        ppSibling = &w->sibling;
        prevw = w;
        w = w->sibling;
      }
    }

    /*
      If we have a non-popup window, span all popups.
    */
    if (!(wNewTop->flags & WS_POPUP))
    {
      while (w != NULL && (w->flags & WS_POPUP))
      {
        ppSibling = &w->sibling;
        prevw = w;
        w = w->sibling;
      }
    }
  }

  wNewTop->sibling = w;
  if (w)
    w->prevSibling = wNewTop;
  *ppSibling = wNewTop;
  wNewTop->prevSibling = prevw;

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinMoveToBottom(hWnd)                                        */
/*                                                                          */
/* Purpose  : Moves a window to the bottom of the sibling list.             */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: SetWindowPos() when hWndAfter is (HWND) 1.                    */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinMoveToBottom(hWnd)
  HWND hWnd;
{
  PWINDOW w;
  PWINDOW wToMove;
  PWINDOW wParent;

  if ((wToMove = WID_TO_WIN(hWnd)) == (PWINDOW) NULL)
    return;

  wParent = wToMove->parent;

  /*
    See if the window to move is the only child of the parent or the
    last child already. If so, then return.... there's nothing to do!
  */
  if (wToMove->sibling == NULL)
    return;
  
  /*
    Now unlink it from the parent and put it back at the end of the
    sibling list
  */
  _WinUnlinkChildFromParent(wToMove);
  for (w = wParent->children;  w && w->sibling;  w = w->sibling)
    ;
  w->sibling = wToMove;
  wToMove->sibling = (WINDOW *) NULL;
  wToMove->prevSibling = w;
  WinUpdateVisMap();
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinMoveAfter(hWnd)                                           */
/*                                                                          */
/* Purpose  : Moves a window after another in the sibling list.             */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: SetWindowPos() when hWndAfter is a non-zero window handle.    */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinMoveAfter(hWnd, hWndAfter)
  HWND hWnd;
  HWND hWndAfter;
{
  PWINDOW w;
  PWINDOW wToMove, wAfter;

  wToMove = WID_TO_WIN(hWnd);

  if (hWndAfter == HWND_TOPMOST)
  {
    _WMakeTopSibling(wToMove);
    return;
  }

  wAfter  = WID_TO_WIN(hWndAfter);

  /*
    Make sure that hWndAfter is a sibling of hWnd
  */
  for (w = wToMove->parent->children;  w && w != wAfter;  w = w->sibling)
    ;
  if (w == wAfter)
  {
    /*
      Put wToMove after wAfter on the sibling list.
    */
    _WinUnlinkChildFromParent(wToMove);
    wToMove->sibling = wAfter->sibling;
    wToMove->prevSibling = wAfter;
    if (wToMove->sibling)
      wToMove->sibling->prevSibling = wToMove;
    wAfter->sibling = wToMove;
    WinUpdateVisMap();
  }
}




/****************************************************************************/
/*                                                                          */
/* Function : GetWindow, GetTopWindow, GetNextWindow                        */
/*                                                                          */
/* Purpose  : Queries the z-order list in search of info about a window.    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL GetWindow(hWnd, wCmd)
  HWND hWnd;
  UINT wCmd;
{
  WINDOW *w, *w2, *wPrev;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return NULLHWND;

  switch (wCmd)
  {
    case GW_HWNDFIRST :
      if (hWnd == _HwndDesktop || !w->parent)
        return NULLHWND;
      /*
        We were passed a child window - find the first sibling
      */
      return w->parent->children->win_id;
      
    case GW_HWNDLAST  :
      if (hWnd == _HwndDesktop || !w->parent)
        return NULLHWND;
      /*
        We were passed a child window - find the last sibling
      */
      for (w2 = w->parent->children;  w2;  wPrev = w2, w2 = w2->sibling)
        ;
      return wPrev->win_id;

    case GW_HWNDPREV  :
      return (w->prevSibling) ? w->prevSibling->win_id : NULLHWND;

    case GW_HWNDNEXT  :
      return (w->sibling) ? w->sibling->win_id : NULLHWND;

    case GW_OWNER     :
      return (IS_DIALOG(w)) ? w->hWndOwner : NULLHWND;

    case GW_CHILD     :
      return (w->children) ? w->children->win_id : NULLHWND;

    default           :
      return NULLHWND;
  }
}


HWND FAR PASCAL GetTopWindow(hWnd)
  HWND hWnd;
{
  if (hWnd == NULL) 
    hWnd = _HwndDesktop;
  return GetWindow(hWnd, GW_CHILD);
}


HWND FAR PASCAL GetNextWindow(hWnd, wFlag)
  HWND hWnd;
  UINT wFlag;
{
  return GetWindow(hWnd, wFlag);
}


/****************************************************************************/
/*                                                                          */
/* Function : SetParent(hWnd, hNewParent)                                   */
/*                                                                          */
/* Purpose  : Attaches a window (hWnd) to a new parent (hNewParent).        */
/*                                                                          */
/* Returns  : The handle of the old parent.                                 */
/*                                                                          */
/* Called by: ComboBoxWndProc to attach/detach a listbox to a combo.        */
/*            Also called by TrackPopupMenu()                               */
/*                                                                          */
/****************************************************************************/
HWND FAR PASCAL SetParent(hWnd, hNewParent)
  HWND hWnd;
  HWND hNewParent;
{
  WINDOW *w   = WID_TO_WIN(hWnd);
  HWND   hOldParent = (w && w->parent) ? w->parent->win_id : NULLHWND;
  
  if (w && hNewParent != hOldParent)
  {
    /* Make sure the old parent doesn't point to this window anymore */
    if (hOldParent)
      _WinUnlinkChildFromParent(w);
    if ((w->parent = (hNewParent) ? WID_TO_WIN(hNewParent) : NULL) != 0)
      _WinAddChild(hNewParent, hWnd);
    w->hWndOwner = hNewParent;  /* set the owner */
  }

#if defined(XWINDOWS)
  if (!(w->ulStyle & LBS_IN_COMBOBOX) && !IS_MENU(w) && hNewParent && w->widget)
  {
    WINDOW *wParent = WID_TO_WIN(hNewParent);
    Widget widgetParent = wParent->widget;
    if (wParent->widgetDrawingArea)
      widgetParent = wParent->widgetDrawingArea;
    if (XtWindow(w->widget) && XtWindow(widgetParent))
      XReparentWindow(XSysParams.display, XtWindow(w->widget), 
                      XtWindow(widgetParent), 0, 0);
    }
#endif

  return hOldParent;
}


HWND FAR PASCAL GetParentOrDT(hWnd)
  HWND hWnd;
{
  return ((hWnd = GetParent(hWnd)) == NULL) ? _HwndDesktop : hWnd;
}

HWND FAR PASCAL GetParent(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  HWND   hOwner = NULLHWND;

  if (w && (hOwner = w->hWndOwner) == NULLHWND)
  {
    WINDOW *wParent = w->parent;
    if (wParent && wParent->win_id != _HwndDesktop)
      hOwner = wParent->win_id;
  }
  return hOwner;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinAddChild(hWnd, hChild)                                    */
/*                                                                          */
/* Purpose  : Adds a child (hChild) to a parent's (hWnd) child list.        */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: SetParent and WinCreate                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinAddChild(hWnd, hChild)
  HWND hWnd;
  HWND hChild;
{
  WINDOW *wParent = WID_TO_WIN(hWnd);
  WINDOW *wChild  = WID_TO_WIN(hChild);
  BOOL   bIsDlg;
  
  /*
    Link the window to its parent and owner
    If the window is a popup window (and not a child popup) or an icon
    title window, then make its parent the desktop window. Otherwise, 
    the owner and the parent are the same.
  */
  if ((wChild->flags & (WS_POPUP | WS_CHILD)) == WS_POPUP)
    wParent = InternalSysParams.wDesktop;
  wChild->parent    = wParent;
  wChild->hWndOwner = (hWnd == _HwndDesktop) ? NULL : hWnd;  /* set the owner */

  /*
    See if we are attaching the child to a dialog box. If so, then we
    need to attach the child to the *end* of the sibling list, because
    this will make the tabbing order correct.
  */
#ifdef ZAPP
  bIsDlg = TRUE;  /* always add children to end of sibling list */
#else
  bIsDlg = IS_DIALOG(wParent);
#endif

  /*
    Give it no siblings
  */
  wChild->sibling = (WINDOW *) NULL;
  wChild->prevSibling = (WINDOW *) NULL;

  /*
    If we have a window's vertical or horizontal scrollbar, then
    do not attach it to the sibling list.
  */
  if (wChild->idClass == SCROLLBAR_CLASS && !(wChild->flags & SBS_CTL))
    return;

  /*
    Attach the window to the parent's list of children. It goes at
    the end of the list, which means that it has the lowest Z-order.
    However, popups go at the head of the list...
  */
  if (!wParent->children)
    wParent->children = wChild;
  else if (!bIsDlg || (wChild->flags & WS_POPUP) ||
                      (wChild->dwExtStyle & WS_EX_TOPMOST))
  {
    if ((wChild->sibling = wParent->children) != NULL)
      wChild->sibling->prevSibling = wChild;
    wParent->children = wChild;
  }
  else
  {
    WINDOW *w;
    for (w = wParent->children;  w->sibling;  w = w->sibling)
      ;
    w->sibling = wChild;
    wChild->prevSibling = w;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinUnlinkChildFromParent(wChild)                             */
/*                                                                          */
/* Purpose  : Internal function which severs a child from the sibling list. */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: _WinDestroy, _WinMoveToBottom, _WinMoveAfter, SetParent       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinUnlinkChildFromParent(wChild)
  WINDOW *wChild;
{
  WINDOW *wParent = wChild->parent;
  WINDOW *cw, *prevw;

  if (!wParent)
    return;

  /*
    Unlink the child from the sibling list
  */
  for (prevw = cw = wParent->children;  cw;  prevw = cw, cw = cw->sibling)
  {
    if (cw == wChild)
    {
      if (wChild == wParent->children)
      {
        if ((wParent->children = wChild->sibling) != NULL)
          wChild->sibling->prevSibling = NULL;
      }
      else
      {
        if ((prevw->sibling = wChild->sibling) != NULL)
          wChild->sibling->prevSibling = prevw;
      }
      break;
    }
  }

  wChild->sibling = (WINDOW *) NULL;
  wChild->prevSibling = (WINDOW *) NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsChild()                                                     */
/*                                                                          */
/* Purpose  : Tests a two windows to see if the second is a child of the    */
/*            first.                                                        */
/*                                                                          */
/* Returns  : TRUE if the hChild is a decendant of hParent, FALSE if not.   */
/*                                                                          */
/****************************************************************************/
static BOOL PASCAL _WinIsChild(PWINDOW, PWINDOW);

BOOL FAR PASCAL IsChild(hParent, hChild)
  HWND hParent;
  HWND hChild;
{
  WINDOW *wParent, *wChild;

  if ((wParent = WID_TO_WIN(hParent)) == (WINDOW *) NULL  ||
      (wChild  = WID_TO_WIN(hChild))  == (WINDOW *) NULL)
    return FALSE;

  return _WinIsChild(wParent, wChild);
}


static BOOL PASCAL _WinIsChild(wParent, w)
  WINDOW *wParent;
  WINDOW *w;
{
  register WINDOW *wChild;
  HWND hWnd = w->win_id;

  /*
    Test for the non-client controls which are not true children.
  */
  if (wParent->hSB[0]==hWnd || wParent->hSB[1]==hWnd || wParent->hMenu==hWnd)
    return TRUE;

  for (wChild = wParent->children;  wChild;  wChild = wChild->sibling)
  {
    /*
      If this child is the window in question, then it is a true child of
      wParent.
    */
    if (wChild == w)
      return TRUE;

    /*
      Test for the non-client controls which are not true children.
    */
    if (wChild->hSB[0]==hWnd || wChild->hSB[1]==hWnd || wChild->hMenu==hWnd)
      return TRUE;

    /*
      Recurse on the grandchildren
    */
    if (wChild->children && _WinIsChild(wChild, w))
      return TRUE;
  }
  return FALSE;
}

