/*===========================================================================*/
/*                                                                           */
/* File    :  WSYSMENU.C                                                     */
/*                                                                           */
/* Purpose :  Handles the processing of system menus for normal windows      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif
static VOID PASCAL SetSysMenuState(HWND, HWND);
#ifdef __cplusplus
}
#endif

HMENU FAR PASCAL GetSystemMenu(
  HWND hWnd,
  BOOL bMode)  /* should be 0 for MS Windows compatibility */
{
  WINDOW *w = WID_TO_WIN(hWnd);
  if (bMode && w)
  {
    if (w->hSysMenu)
    {
      DestroyMenu(w->hSysMenu);
      w->hSysMenu = NULLHMENU;
    }
    WinCreateSysMenu(hWnd);
  }

  return (w && w->hSysMenu) ? w->hSysMenu : NULLHMENU;
}


/****************************************************************************/
/*                                                                          */
/*  _WinFindAndInvokeSysMenu()                                              */
/*    Starting with a window, finds the first ancestor with a system menu.  */
/*  If a window is found, the system menu is invoked.                       */
/*                                                                          */
/*  Called by :                                                             */
/*   DispatchMessage(), when the ALT+SPACE key is pressed                   */
/*   StdWindowWinProc(), when SC_KEYMENU and LOWORD(lParam) == ' '          */
/*   2 places in winmdi.c when ALT+'-' is pressed                           */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL _WinFindandInvokeSysMenu(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  
  if (!w)
    return FALSE;

  /*
    March up the tree starting at this window and find the first window
    which has a system menu and is not disabled.
  */
  while (w && (w->hSysMenu==NULLHMENU || !IsWindowEnabled(w->win_id)))
  {
    w = WID_TO_WIN(w->hWndOwner);
  }

  if (!w || !IsWindowEnabled(w->win_id))
    return FALSE;
  /*
    Hot damn! We found a window! Now activate it's system menu.
  */
  WinActivateSysMenu(w->win_id);
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/*  WinCreateSysMenu()                                                      */
/*    Creates a system menu and attaches it to window hWnd.                 */
/*                                                                          */
/****************************************************************************/
HMENU FAR PASCAL WinCreateSysMenu(hWnd)
  HWND hWnd;
{
  WINDOW *w;
  HMENU  hPop;   /* handle of the system menu */
  DWORD  dwFlags;
  BOOL   bNonChangableDlg;
  BOOL   bIsMDIDoc;

  extern BOOL bDeferMenuRecalc;


  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return NULLHMENU;

  /*
    Create the menu and fill in the default entries
  */
  hPop = CreateMenu();
  if (hPop)
    WID_TO_WIN(hPop)->parent = w;
  dwFlags = w->flags;

#if defined(ZAPP)
  bNonChangableDlg = !(!strcmp(WinGetClassName(hWnd), "DIALOGFRAMECL") &&
                      (dwFlags & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) == 0L);
#else
  bNonChangableDlg = !(IS_DIALOG(w) &&
                      (dwFlags & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) == 0L);
#endif

  /*
    If we are dealing with an MDI document window, then force the window
    to have all of the caption-bar decorations.
  */
  if ((bIsMDIDoc = IS_MDIDOC(w)) == TRUE)
    dwFlags |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_THICKFRAME;


#if defined(MOTIF)
  /*
    Only do the sysmenu if we are simulating our own NC decorations
  */
  if (!(w->dwXflags & WSX_USEMEWELSHELL))
  {
    /*
      Leave the window with an empty system menu...
    */
    w->hSysMenu = hPop;
    w->flags |= WS_SYSMENU;
    return hPop;
  }
#endif

#ifdef INTERNATIONAL_MEWEL
  WinGetIntlMenuStrings();
#endif

  bDeferMenuRecalc = TRUE;

  if (dwFlags & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX))
    ChangeMenu(hPop, 0, SysStrings[SYSSTR_RESTORE],   SC_RESTORE, MF_APPEND);
  if (dwFlags & WS_CAPTION)
    ChangeMenu(hPop, 0, SysStrings[SYSSTR_MOVE],      SC_MOVE,    MF_APPEND);
  if (dwFlags & WS_THICKFRAME)
    ChangeMenu(hPop, 0, SysStrings[SYSSTR_SIZE],      SC_SIZE,    MF_APPEND);
  if (dwFlags & WS_MINIMIZEBOX)
    ChangeMenu(hPop, 0, SysStrings[SYSSTR_MINIMIZE],  SC_MINIMIZE,MF_APPEND);
  if (dwFlags & WS_MAXIMIZEBOX)
    ChangeMenu(hPop, 0, SysStrings[SYSSTR_MAXIMIZE],  SC_MAXIMIZE,MF_APPEND);

  if (bNonChangableDlg || 
      (dwFlags & (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)))
    ChangeMenu(hPop, 0, NULL, 0, MF_APPEND | MF_SEPARATOR);

  ChangeMenu(hPop, 0,
           bIsMDIDoc ? SysStrings[SYSSTR_CLOSEMDI] : SysStrings[SYSSTR_CLOSE],
           SC_CLOSE,   MF_APPEND);

  /*
    Change the menu into a popup system menu
  */
  ChangeMenu(hPop, hPop, NULL, hPop, MF_POPUP | MF_SYSMENU | MF_CHANGE);

  /*
    Attach the system menu to the window and move it to the upper left corner
  */
  w->hSysMenu = hPop;
  w->flags |= WS_SYSMENU;

#if defined(MEWEL_GUI)
  {
  INT cx, cy;
  if (dwFlags & WS_MAXIMIZE)
    cx = cy = 0;
  else
  {
    BOOL bHasFrame = (BOOL) ((dwFlags & WS_THICKFRAME) != 0L);
    cy = IGetSystemMetrics(bHasFrame ? SM_CYFRAME : SM_CYBORDER);
    cx = IGetSystemMetrics(bHasFrame ? SM_CXFRAME : SM_CXBORDER);
  }
  WinMove(hPop,
          w->rect.top  + cy + IGetSystemMetrics(SM_CYCAPTION),
          w->rect.left + cx);
  }
#elif defined(MEWEL_TEXT)
  WinMove(hPop, w->rect.top+1, w->rect.left);
#endif

  EnableMenuItem(hPop, SC_RESTORE, MF_BYCOMMAND | MF_DISABLED);

  bDeferMenuRecalc = FALSE;
  return hPop;
}


/****************************************************************************/
/*                                                                          */
/*  WinActivateSysMenu()                                                    */
/*    Invokes a system menu and gets the user choice.                       */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL WinActivateSysMenu(hWnd)
  HWND hWnd;
{
  WINDOW *w, *wPop;
  MSG    msg;
  HMENU  hPop;
  RECT   r;
  HWND   hOldFocus = InternalSysParams.hWndFocus;
  UINT   rc;
  POINT  pt;

  /*
    Get a pointer to the window and to its system menu
  */
  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;
  if ((hPop = w->hSysMenu) == NULLHMENU)
    return FALSE;
  wPop = WID_TO_WIN(hPop);

  /*
    Draw the system menu pulldown and set the focus to it
  */
  SetSysMenuState(hWnd, hPop);

#if defined(MOTIF)
  return XMEWELActivateSysMenu(w);
#else

  SetFocus(hPop);
  SetCapture(hPop);

  /*
    Simulate a mousedown on the first item. This will place the cursor
    at the first item and will activate the mouse.
  */
  WinGetClient(hPop, &r);
  SendMessage(hPop, WM_LBUTTONDOWN, 0, MAKELONG(0, 0));

  while ((rc = _PeekMessage(&msg)) != WM_SYSCOMMAND)
  {
    if (msg.message == WM_QUIT)
      return TRUE;

    if (rc == FALSE)
    {
      SendMessage(hWnd, WM_ENTERIDLE, MSGF_MENU, (DWORD) hPop);
      continue;
    }

    /*
      If we pressed the mouse button outside of the system menu, then
      get outta here. Otherwise, we probably did something like click
      on the system menu icon, move the mouse to the desired entry, then
      clicked the button again.
    */
    if (msg.message == WM_NCLBUTTONDOWN)
      msg.message = WM_LBUTTONDOWN;
    else if (msg.message == WM_LBUTTONDOWN)
      msg.lParam = _UnWindowizeMouse(hPop, msg.lParam);

    if (msg.message == WM_LBUTTONDOWN)
    {
      HWND hW;
      INT  wHitCode;
      pt = MAKEPOINT(msg.lParam);
      GetWindowRect(hWnd, &r);

      /*
        Break if we didn't click on the system menu or the system menu icon.
      */
      if ((hW = DetermineClickOwner(pt.y, pt.x, &wHitCode)) != hPop &&
#ifdef MEWEL_GUI
          (hW != hWnd || pt.y < r.top                                || 
                         pt.y >= r.top + IGetSystemMetrics(SM_CYSIZE) ||
                         pt.x < r.left                               ||
                         pt.x >= r.left + IGetSystemMetrics(SM_CXSIZE)))
#else
          (hW != hWnd || r.top != pt.y || r.left != pt.x))
#endif
        break;
      else
        SetCapture(hPop);
    }

    _WinGetMessage(&msg);

    if (msg.message == WM_NCLBUTTONDOWN)
      msg.message = WM_LBUTTONDOWN;
    if (msg.message == WM_LBUTTONDOWN)
      rc = TRUE;

    if (msg.hwnd == hPop     ||
        msg.hwnd == NULLHMENU || msg.hwnd == hWnd ||
        msg.message == WM_TIMER)
    {
      DispatchMessage(&msg);
      if (msg.message == WM_ALT)
         break;
      if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        break;
      if (msg.message == WM_LBUTTONDOWN && InternalSysParams.hWndCapture != hPop)
        break;
      if (TEST_WS_HIDDEN(wPop))
        break;
    }
  }

  ReleaseCapture();

#if 0
  if (msg.message != WM_ALT)
#endif
    SetFocus(hOldFocus ? hOldFocus : hWnd);
  SendMessage(hOldFocus ? hOldFocus : hWnd, WM_SETFOCUS, hPop, 0L);

  if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
    return FALSE;
  return TRUE;
#endif
}


static VOID PASCAL SetSysMenuState(hWnd, hSysMenu)
  HWND hWnd;
  HWND hSysMenu;
{
  BOOL  bIconic;
  BOOL  bZoomed;
  DWORD dwFlags;

  dwFlags = WinGetFlags(hWnd);
  bZoomed = (BOOL) ((dwFlags & WS_MAXIMIZE) != 0L);
  bIconic = (BOOL) ((dwFlags & WS_MINIMIZE) != 0L);


  EnableMenuItem(hSysMenu, SC_RESTORE,  MF_BYCOMMAND | 
              ((bIconic || bZoomed) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

  EnableMenuItem(hSysMenu, SC_MINIMIZE, MF_BYCOMMAND |
              ((!bIconic && ((dwFlags & WS_MINIMIZEBOX) != 0L))
                       ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

  EnableMenuItem(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND |
              ((!bZoomed && ((dwFlags & WS_MAXIMIZEBOX) != 0L))
                       ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

  EnableMenuItem(hSysMenu, SC_MOVE,     MF_BYCOMMAND |
              ((!bZoomed && HAS_CAPTION(dwFlags)) || bIconic)
                       ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));

  EnableMenuItem(hSysMenu, SC_SIZE,     MF_BYCOMMAND |
              ((!bIconic && !bZoomed && ((dwFlags & WS_THICKFRAME) != 0L))
                       ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

  EnableMenuItem(hSysMenu, SC_CLOSE,    MF_BYCOMMAND |
              ((!(GetClassStyle(hWnd) & CS_NOCLOSE))
                       ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));


#if 0
  /*
    Get rid of the separator if we have one and do not need one.
  */
  nItems = GetMenuItemCount(hSysMenu);
  if (nItems && (GetMenuState(hSysMenu,nItems-1,MF_BYPOSITION) & MF_SEPARATOR))
  {
    DeleteMenu(hSysMenu, nItems-1, MF_BYPOSITION);
  }
#endif
}


#ifdef INTERNATIONAL_MEWEL
VOID FAR PASCAL WinGetIntlMenuStrings(void)
{
  static BOOL MenuTextRead = FALSE;

  /*
    The internationalization scheme loads the system menu strings
    from the resource file.
  */
  if (MewelCurrOpenResourceFile != -1 && !MenuTextRead)
  {
    BYTE szBuf[64];
    int  nb;

    MenuTextRead = TRUE;

    for (nb = 0;  nb < INTL_MENU_COUNT;  nb++)
      if (LoadString(MewelCurrOpenResourceFile, INTL_RESTORE + nb, szBuf,
                     sizeof(szBuf)-1) > 0)
        SysStrings[nb] = lstrsave(szBuf);
  }
}
#endif

