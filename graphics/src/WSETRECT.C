/*===========================================================================*/
/*                                                                           */
/* File    : WSETRECT.C                                                      */
/*                                                                           */
/* Purpose : _WinSetClientRect()                                             */
/*           Calculates the dimensions of the client area of a window.       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU
#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
static VOID PASCAL _WinCalcNCObjects(HWND);
#ifdef __cplusplus
}
#endif



/*
  _WinSetClientRect()
    Sets the client coordinates of a window. It is assumed that the
  window coords are set up already, and can be found in w->rect.
*/
INT FAR PASCAL _WinSetClientRect(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  RECT   rClient;

  if (!w)
    return FALSE;

  /*
    Start off with the client rect equal to the window rect.
    Then give the app a chance to much with the size. The default
    processing for WM_NCCALCSIZE is to call WinSetClientRect() below.
  */
  rClient = w->rect;
  SendMessage(hWnd, WM_NCCALCSIZE, 0, (LONG) (LPSTR) &rClient);
  w->rClient = rClient;

  /*
    After the client rectangle is calculated, we can adjust the
    coordinates of the non-client objects, like the scrollbars and
    the menubar.
  */
#if defined(MOTIF)
  _XGetRealCoords(w);
#else
  _WinCalcNCObjects(hWnd);
#endif

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : WinSetClientRect()                                            */
/*                                                                          */
/* Purpose  : Handles the WM_NCCALCSIZE message. Given a window rectangle,  */
/*            determines the window's client area.                          */
/*            Passes back the client area rect in 'lpRect' --- does not     */
/*            actually set the window's rClient field.                      */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: StdWindowWinProc() for default processing of the              */
/*             WM_NCCALCSIZE message.                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinSetClientRect(hWnd, lpRect)
  HWND   hWnd;
  LPRECT lpRect;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags;
  RECT   rClient;
  INT    iHeight;
  BOOL   bMaxMDIChild;
#ifdef MEWEL_TEXT
  int    idClass;
#endif

  if (!w)
    return;

  /*
    For Motif, only do the NC calculations if we have a window for
    which we are drawing the NC area ourselves.
  */
#if defined(MOTIF)
  if (!(w->dwXflags & WSX_USEMEWELSHELL))
    return;
#endif

  dwFlags = w->flags;

#if defined(MEWEL_GUI) || defined(MOTIF)
  /*
    If we are displaying real icons, then set the client rect to be the
    same as the window rect.
  */
  if (dwFlags & WS_MINIMIZE)
    return;
#endif

  rClient = *lpRect;

  /*
    See if the window is a maximized MDI child window. If so, we need to
    do special processing.
  */
  bMaxMDIChild = (IS_MDIDOC(w) && (dwFlags & WS_MAXIMIZE));


  /*
    If a window has a border, then its client area is inside the border...
  */
  if (HAS_BORDER(dwFlags))
  {
    /*
      The client rect should be the window rect minus the width of the
      various borders.
    */
#if defined(MEWEL_GUI) || defined(MOTIF)
    if (dwFlags & WS_THICKFRAME) /* thick frame? */
    {
#if 101394
      if ((dwFlags & WS_MAXIMIZE) && !bMaxMDIChild)
#else
      if ((dwFlags & WS_MAXIMIZE))
#endif
      {
        /*
          If we have a maximized MDI document window, do not include the
          caption, since it "takes on" the caption of the main window.
        */
        if (bMaxMDIChild)
          goto check_sbars;
      }
      else
        InflateRect((LPRECT) &rClient,
                    -IGetSystemMetrics(SM_CXFRAME),
                    -IGetSystemMetrics(SM_CYFRAME));
    }
    else
    {
      if (!(dwFlags & WS_MAXIMIZE))
        InflateRect((LPRECT) &rClient,
                    -IGetSystemMetrics(SM_CXBORDER),
                    -IGetSystemMetrics(SM_CYBORDER));
    }
    goto check_caption_and_sbars;
#else
    idClass = _WinGetLowestClass(w->idClass);

#if !defined(MEWEL_TEXT)
    if (bMaxMDIChild)
       goto check_sbars;
#endif

    iHeight = RECT_HEIGHT(rClient);
    if ((idClass != PUSHBUTTON_CLASS && idClass != BUTTON_CLASS) || 
        iHeight >= 3)
    {
      rClient.left++;
      rClient.right--;
    }
    if ((idClass != EDIT_CLASS && idClass != PUSHBUTTON_CLASS && idClass != BUTTON_CLASS) || 
        iHeight >= 3)
    {
      rClient.top++;
      rClient.bottom--;
    }
#endif
  }


  /*
    Account for non-bordered windows with scrollbars
  */
  else
  {
check_caption_and_sbars:
    if (HAS_CAPTION(dwFlags))  /* non-bordered window w/ caption bar */
      rClient.top += IGetSystemMetrics(SM_CYCAPTION);

check_sbars:
    if (WinHasScrollbars(hWnd, SB_BOTH))
    {
      HWND hHSB, hVSB;
      BOOL bVert, bHorz;

      WinGetScrollbars(hWnd, &hHSB, &hVSB);
      /*
        Don't use IsWindowVisible() here, because the parent window
        might not be shown yet. So, test TEST_WS_HIDDEN(wSB) to
        see if the scrollbars have been explictly hidden
      */
      bHorz = (BOOL) (hHSB != NULL && !TEST_WS_HIDDEN(WID_TO_WIN(hHSB)));
      bVert = (BOOL) (hVSB != NULL && !TEST_WS_HIDDEN(WID_TO_WIN(hVSB)));

      /*
        First fix the client rectangle
      */
      if (bHorz)
        rClient.bottom -= IGetSystemMetrics(SM_CYHSCROLL);
#ifdef MEWEL_TEXT
      else if (bMaxMDIChild && hHSB)
      {
        rClient.bottom++;
      }
#endif

      if (bVert)
        rClient.right  -= IGetSystemMetrics(SM_CXVSCROLL);
#ifdef MEWEL_TEXT
      else if (bMaxMDIChild && hVSB)
      {
        rClient.right++;
      }
#endif
    }
  }

  /*
    If we have a menubar, then the client starts below the menubar.
    Ignore the menubar if the window is iconized.
  */
  if (w->hMenu && !(dwFlags & WS_MINIMIZE))
    rClient.top += IGetSystemMetrics(SM_CYMENU);

  *lpRect = rClient;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinCalcNCObjects()                                           */
/*                                                                          */
/* Purpose  : Internal routine called by _WinSetClientRect() after the      */
/*            window's client rectangle has been calculated. This routine   */
/*            adjusts the positions and sizes of the objects in the         */
/*            non-client area - the scrollbars and the menubar.             */
/*                                                                          */
/* Returns  : Ntohing                                                       */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL _WinCalcNCObjects(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags;
  RECT   rClient;
  HMENU  hMenu;
#if defined(MEWEL_TEXT)
  BOOL   bMaxMDIChild;
#endif

  dwFlags = w->flags;

#if defined(MEWEL_TEXT)
  bMaxMDIChild = (IS_MDIDOC(w) && (dwFlags & WS_MAXIMIZE));
#endif

#if defined(MEWEL_GUI)
  /*
    If we are displaying real icons, then set the client rect to be the
    same as the window rect.
  */
  if (dwFlags & WS_MINIMIZE)
    return;
#endif

  rClient = w->rClient;

  if (WinHasScrollbars(hWnd, SB_BOTH))
  {
    HWND hHSB, hVSB;
    BOOL bVert, bHorz;

    WinGetScrollbars(hWnd, &hHSB, &hVSB);
    /*
      Don't use IsWindowVisible() here, because the parent window
      might not be shown yet. So, test TEST_WS_HIDDEN(wSB) to
      see if the scrollbars have been explictly hidden
    */
    /*
      10/6/92 (maa)
        We must move the scrollbars, regardless of if they are hidden
      or not. This is especially true when we move a dialog box which
      has a combobox with scrollbars. (The scrollbars are hidden if
      the listbox portion is hidden, so the scrollbars won't get moved
      when the listbox gets moved.)
    */
    bHorz = (BOOL) (hHSB != NULL);
    bVert = (BOOL) (hVSB != NULL);

    /*
      Fix the position of the scrollbars
    */
    if (bHorz)
    {
#if defined(MEWEL_TEXT)
      WinMove(hHSB, rClient.bottom, rClient.left+bMaxMDIChild);
      WinSetSize(hHSB, IGetSystemMetrics(SM_CYHSCROLL), RECT_WIDTH(rClient)-bMaxMDIChild);
#elif defined(MEWEL_GUI)
      WinMove(hHSB, rClient.bottom, rClient.left);
      WinSetSize(hHSB, IGetSystemMetrics(SM_CYHSCROLL), RECT_WIDTH(rClient));
#endif
    }
    if (bVert)
    {
#if defined(MEWEL_TEXT)
      WinMove(hVSB, rClient.top+bMaxMDIChild, rClient.right);
      WinSetSize(hVSB, RECT_HEIGHT(rClient)-bMaxMDIChild, IGetSystemMetrics(SM_CXVSCROLL));
#elif defined(MEWEL_GUI)
      WinMove(hVSB, rClient.top, rClient.right);
      WinSetSize(hVSB, RECT_HEIGHT(rClient), IGetSystemMetrics(SM_CXVSCROLL));
#endif
    }
  }

  /*
    Set the size of the window's menubar
  */
  if ((hMenu = w->hMenu) != NULL)
  {
    MENU *m = _MenuHwndToStruct(hMenu);

    /*
      Test the menu to make sure that it's not a trackable popup menu.
      This is because TrackPopupMenu() does the dirty trick of temporarily 
      setting the window's hMenu field to the trackable popup menu.
    */
    if (m != NULL && !(m->flags & M_POPUP))
    {
      WinSetSize(hMenu, IGetSystemMetrics(SM_CYMENU), RECT_WIDTH(rClient));
      _MenuFixPopups(w, hMenu);
    }
  }
}



/****************************************************************************/
/*                                                                          */
/* Function : GetWindowRect()                                               */
/*                                                                          */
/* Purpose  : Passes back the entire window rectangle in SCREEN coordinates */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL GetWindowRect(hWnd, lpRect)
  HWND hWnd;
  LPRECT lpRect;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  if (w)
#if defined(XWINDOWS)
    *lpRect = w->rWindowRoot;
#else
    *lpRect = w->rect;
#endif
  else
    *lpRect = RectEmpty;
}

/****************************************************************************/
/*                                                                          */
/* Function : WinGetClient()                                                */
/*                                                                          */
/* Purpose  : Returns the window's client rect in SCREEN coordinates        */
/*                                                                          */
/* Returns  : A RECT structure                                              */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinGetClient(hWnd, lpRect)
  HWND hWnd;
  LPRECT lpRect;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  if (w)
#if defined(XWINDOWS)
    *lpRect = w->rClientRoot;
#else
    *lpRect = w->rClient;
#endif
  else
    *lpRect = RectEmpty;
}


/****************************************************************************/
/*                                                                          */
/* Function : _WinAdjustRectForShadow()                                     */
/*                                                                          */
/* Purpose  : Given a rectangle, increases the size of the rectangle in     */
/*            accordance with the window's shadow style.                    */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _WinAdjustRectForShadow(w, lpRect)
  WINDOW *w;
  LPRECT lpRect;
{
#if defined(MEWEL_TEXT)
  if (w->flags & WS_SHADOW)
  {
    lpRect->bottom++;
    lpRect->right += 2;
  }
#else
  (void) w;
  (void) lpRect;
#endif
}

