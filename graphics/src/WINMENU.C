/*===========================================================================*/
/*                                                                           */
/* File    : WINMENU.C                                                       */
/*                                                                           */
/* Purpose : Routines to implement the menubar and pulldown menu system      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU
#define MULTI
#define SOFTCOST
#define INCLUDE_CURSES
#define NOGDI
#define NOGRAPHICS
#define NOKERNEL

#ifdef CARRYER
/*  July 12, 1991
  I've made the menus behave a bit more like Windows.
  When you click on a menubar item, none of the pulldowns are selected
  until you drag the mouse over them. When the mouse moves off all
  items (i.e. the border or a separator) all items are deselected.
  If you select via an Alt-key, then the first selectable item is
  hilighted.

  I also added a new sys-color SYSCLR_MENUSELHILIGHTSEL.
  This is the color of the selection character when the hilight is on
  an item. This lets you, for instance, make the whole text go to
  bright white when an item is selected (which i prefer), but also
  allows you to get the old effect by leaving SYSCLR_MENUSELHILITESEL
  the same as SYSCLR_MENUHILITESEL.
*/
#endif


#include "wprivate.h"
#include "window.h"

#define MM_SELECT		(WM_USER + 413)

/*
  WM_KILLFOCUS lParam values
*/
#define KILL_PARENT_FOCUS       0L
#define DONT_KILL_PARENT_FOCUS  1L

#ifdef __cplusplus
extern "C" {
#endif
extern int     FAR PASCAL MenuBarColToItem(MENU *,int);
extern int     FAR PASCAL MenuBarItemToCol(MENU *,int);
static INT         PASCAL MenuBarIDToPos(MENU *,int);
static INT         PASCAL MenuFindPrefix(MENU *,INT,INT);
static INT         PASCAL MenuGetMaxLen(MENU *);
static INT         PASCAL MenuGetPopupHeight(MENU *);
static INT         PASCAL MenuMove(MENU *,UINT);
extern int     FAR PASCAL MenuSelect(MENU *,int,int);
static INT         PASCAL _MenuGetFirstSelectableItem(MENU *);
extern WINDOW *FAR PASCAL _WinGetPrevPtr(WINDOW *);
static INT         PASCAL IsMouseInMenuParent(PWINDOW ,DWORD);
static WINDOW     *PASCAL GetMenubarFromPopup(WINDOW *);

extern INT     FAR PASCAL MenuRowToItem(MENU *, int);
extern VOID    FAR PASCAL MenuComputeItemCoords(MENU *);

#ifdef __cplusplus
}
#endif

/*
  Define this if we want to attempt inverting the highlited menu item
*/
#ifdef MEWEL_GUI
#define INVERT_ITEM
MENUINVERTINFO MenuInvertInfo =
{
  FALSE, -1, 0
};
#endif

#define GetSysMetrics(n)  (n)


/****************************************************************************/
/*                                                                          */
/* Function : CreateMenu()                                                  */
/*                                                                          */
/* Purpose  : Creates an empty menu window with no entries.                 */
/*                                                                          */
/* Returns  : The handle to the menu window.                                */
/*                                                                          */
/* Windows Compatibility Note : This should really return the handle to     */
/*    a menu resource. Windows stores menus as resources, and creates       */
/*    and hides a single window to display a popup.                         */
/*                                                                          */
/****************************************************************************/
HMENU FAR PASCAL CreateMenu(void)
{
  WINDOW *wMenu;
  MENU   *m;
  HWND   hMenu;
  
  /*
    Create the menubar window
  */
  hMenu = WinCreate(NULLHWND,
                    0,1,
                    IGetSystemMetrics(SM_CYMENU),VideoInfo.width,
                    NULL,
                    SYSTEM_COLOR,
                    /* WS_HIDDEN,*/ 0L,
                    NORMAL_CLASS, 0);

  /*
    Get a pointer to the window structure for the menu
  */
  if (hMenu == NULLHWND ||  (wMenu = WID_TO_WIN(hMenu)) == NULL)
    return NULLHWND;
  SET_WS_HIDDEN(wMenu);

  /*
    Note that the main window is the menu's parent, but the menu is *not*
    the main window's child. We do this so the menu bar can receive mouse
    messages on it's own.
  */
  WinSetWinProc(hMenu, MenuWndProc);
  wMenu->ulStyle |= WIN_IS_MENU;

  /*
    Fill in the fields of the MENU structure.
  */
  wMenu->pPrivate = emalloc(sizeof(MENU));
  m = (MENU *) wMenu->pPrivate;
  m->flags   |=  M_MENUBAR;
  m->iCurrSel = -1;
  m->hWnd     = hMenu;
  m->iLevel   = 0;

  return hMenu;
}


/****************************************************************************/
/*                                                                          */
/* Function : MenuWndProc()                                                 */
/*                                                                          */
/* Purpose  : The window procedure for the menu class                       */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LRESULT FAR PASCAL MenuWndProc(hMenu, message, wParam, lParam)
  HWND   hMenu;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  WINDOW *wMenu, *wParent;
  MENU   *m;
  LPMENUITEM mi;
  static BOOL bSelecting = FALSE;
  int    mouserow, mousecol;
  int    oldSel;
  int    index;
  DWORD  lParamOrig = lParam;

  /*
    Get a ptr to the menu window and structure
  */
  if ((wMenu = WID_TO_WIN(hMenu)) == NULL)
    return FALSE;
  m = (MENU *) wMenu->pPrivate;
  wParent = wMenu->parent;


  /*
    Client-area mouse messages should be converted back into screen coords
    for this winproc.
  */
  if (IS_WM_MOUSE_MSG(message))
    lParam = _UnWindowizeMouse(hMenu, lParam);


  /*
    Extract the window-based mouse coordinates
  */
  mouserow = HIWORD(lParam) - wMenu->rClient.top;
  mousecol = LOWORD(lParam) - wMenu->rClient.left;


  switch (message)
  {
    case WM_NCHITTEST :
      return HTCLIENT;

    case WM_PAINT  :
      InternalDrawMenuBar(hMenu);
      wMenu->rUpdate = RectEmpty;
      break;

    case WM_HELP    :
    case WM_COMMAND :
    case WM_SYSCOMMAND :
      /*
        WM_COMMAND is sent to the menubar winproc when the user presses
        an ALT+letter combo which corresponds to a menubar item. (lParam is 1).
        It is also sent by a popup or a subpopup when the user selects
        an item (lParam is 0).
      */
      if (HIWORD(lParam) == 0)
      {
        if (wMenu->parent)
          SendMessage(wMenu->parent->win_id, message, wParam, lParam);
        break;
      }

      if (IS_MENUBAR(m))
      {
        /* Get the 0-based index of the menubar item chosen */
        index = MenuBarIDToPos(m, wParam);

lbl_select_menubar_item:
        if (index < 0)
          break;
        m->flags &= ~M_NOSHOWPOPUPS;
        if (MenuSelect(m, index, TRUE) == LB_ERR)
          break;
        if (MenuBarShowPopup(hMenu, index) == FALSE)
          goto send_cmd;     /* no associated popup? Issue the menubar cmd */
        MenuRefresh(wMenu);
      }
      break;


    case WM_KEYDOWN :
      if (wParam < 0xFF)  /* wait until WM_CHAR time */
        break;
    case WM_CHAR    :
      /*
        We hit RETURN on a menu item. If it's a menubar item without
        an associated popup, then send the menu command.
      */
      if (wParam == VK_RETURN && DONT_SHOW_POPUPS(m))
      {
        index = m->iCurrSel;
        goto lbl_select_menubar_item;
      }

      /*
        If we hit RETURN on an item which has an associated popup, then
        show the popup. If we hit any other character, then show the
        popup or send the command.
      */
      if (wParam == VK_RETURN || MenuChar(wMenu, wParam) == MM_SELECT)
      {
        if (GetSubMenu(hMenu, m->iCurrSel))
          MenuBarShowPopup(hMenu, m->iCurrSel);
        else
          goto send_cmd;
      }
      break;


    case WM_SETFOCUS :
    {
      /*
        When we set focus to a menu item, we should remember the window
        which had focus before it, so that if the user presses the ESC
        key, focus is returned to the old window.
      */
      /*
        If we are not dragging the mouse (ie - the left button is down),
        then disregard the mouse movement as well.
      */
      SetCapture(hMenu);

      /*
        Send the WM_INITMENU message to the parent
      */
      if (wParent)
        SendMessage(wParent->win_id, WM_INITMENU, hMenu, 0L);

      /*
        Get the index of the first menu-item which we can select
        (ie - an enabled item), and move the selection bar to that item.
      */
#ifdef CARRYER
      /*
        If the mouse button is not down, do the above thingie.
      */
      if (!IsMouseLeftButtonDown())
#endif
      if ((index =  _MenuGetFirstSelectableItem(m)) >= 0)
        MenuSelect(m, index, TRUE);

      /*
        Save the handle of the window which previously had the focus,
        so we can later restore the focus to this window.
      */
      m->hOldFocus = (HWND) wParam;

      /*
        If we are about to display a hidden popup, then save the
        underlying screen so we can restore it later. Also, turn off
        the HIDDEN flag.
      */
      if (!IS_MENUBAR(m) && TEST_WS_HIDDEN(wMenu))
      {
        MenuSaveScreen(hMenu, TRUE);
        CLR_WS_HIDDEN(wMenu);
        wMenu->flags |= WS_VISIBLE;
#if defined(XWINDOWS) && !defined(MOTIF)
        XMapWindow(XSysParams.display, wMenu->Xwindow);
        _XFlushEvents();
#endif
        _WinUpdateVisMap(wMenu);
        InternalDrawMenuBar(hMenu);
      }

      /*
        If we moved up to the menubar by tapping the ALT key, then we
        send a WM_MENUSELECT message to the parent. We also refresh
        the menubar to show the highlighting. In addition, we set
        a flag telling the menu *not* to show the underlying popup.
      */
      if (SysEventInfo.lastMessage == WM_ALT && IS_MENUBAR(m) && 
          IsWindowEnabled(hMenu))
      {
        LPMENUITEM mi = MenuGetItem(m, 0, MF_BYPOSITION);
        m->flags |= M_NOSHOWPOPUPS;
        InternalDrawMenuBar(hMenu);
        if (wParent && mi)
          SendMessage(wParent->win_id, WM_MENUSELECT, mi->id, 
                                       MAKELONG(mi->flags, m->hWnd));
      }
      break;
    }

    case WM_KILLFOCUS :
      m->iCurrSel = -1; /* reset to -1 so MenuRefresh doesn't hilite */
#if 0   /* 3/29/93 (fred needham @ televoice) */
      m->flags &= ~M_NOSHOWPOPUPS;
#endif
      ReleaseCapture();
      InternalSysParams.hWndFocus = NULLHWND;
      bSelecting = FALSE;
      if (!IS_MENUBAR(m))
      {
        /*
          The popup is losing focus, so restore the screen underneath
        */
        SET_WS_HIDDEN(wMenu);
        wMenu->flags &= ~WS_VISIBLE;
#if defined(XWINDOWS) && !defined(MOTIF)
        XUnmapWindow(XSysParams.display, wMenu->Xwindow);
        _XFlushEvents();
#endif
        MenuSaveScreen(hMenu, FALSE);

        /*
          If we did something like hit the right arrow key while in a pulldown,
          then the parent window (ie - the menubar) should not lose its focus.
          (2) In addition, if we are in a sub-popup (the iLevel is 2 or more) &
          we click the mouse on the higher-level popup, we shouldn't kill
          the parent.
        */
        if (lParam == KILL_PARENT_FOCUS  && 
            wParent != NULL              &&
            IS_MENU(wParent)             &&
/* (2) */   (m->iLevel < 2 || !wParam || !(WinGetStyle(wParam) & WIN_IS_MENU)))
        {
          SendMessage(wParent->win_id, WM_KILLFOCUS, wParam, KILL_PARENT_FOCUS);
        }
        else
        {
          InternalSysParams.hWndFocus = wParent->win_id;
          if ((wParent->ulStyle & WIN_IS_MENU))
            bSelecting = TRUE;
        }

        /*
          In the event we have an independent popup menu or a system menu,
          send the final WM_MENUSELECT message to the parent.
        */
        if (wParent != NULL && !IS_MENU(wParent))
          goto send_final_menuselect;
      }
      else  /* kill focus is sent to the menubar */
      {
        MenuRefresh(wMenu);
        /*
          Return the focus to the old window.
          Be careful not to set the focus back to a popup
          (Use wParent as a temp var)
        */
        if (m->hOldFocus != NULLHWND && 
            (wParent = WID_TO_WIN(m->hOldFocus)) != (WINDOW *) NULL &&
            !(wParent->ulStyle & WIN_IS_MENU))
          SetFocus(m->hOldFocus);

        /*
          When we dismiss a menubar, we should send a WM_MENUSELECT message
          to the parent with lParam set to (-1,0). This gives the app an
          opportunity to erase any status messages.
        */
send_final_menuselect:
        if ((wParent = wMenu->parent) != NULL)
          SendMessage(wParent->win_id,WM_MENUSELECT,0,MAKELONG(((WORD)-1),0));
      }
      break;


    case WM_INITMENU      :
    case WM_INITMENUPOPUP :
    case WM_MENUSELECT    :
      /*
         We get here when the menubar has received a message
         from one of its subpopups. We want to pass that info on to the
         parent window.
      */
      SendMessage(wParent->win_id, message, wParam, lParam);
      break;


    case MM_SELECT :
      /*
        MM_SELECT is an internal MEWEL message which is sent by the
        MenuChar() when the selection is moved to a new item.
        wParam is the 0-based menu item to select.
      */

      /*
        Set the current selection to the item and send WM_MENUSELECT msg
      */
      index = (int) wParam;
      if (MenuSelect(m, index, TRUE) == LB_ERR)
        break;

      if (IS_MENUBAR(m))
      {
        /*
          Try showing the popup which is attached to the menubar item.
        */
        if (DONT_SHOW_POPUPS(m) || MenuBarShowPopup(hMenu, index) == FALSE)
        {
          /*
            We have a menubar item with no associated popup. Draw the
            hilited item, and send a WM_MENUSELECT message to the parent.
          */
          LPMENUITEM mi = MenuGetItem(m, index, MF_BYPOSITION);
          MenuRefresh(wMenu);
          if (wParent && mi)
            SendMessage(wParent->win_id, WM_MENUSELECT, 
                        mi->id, MAKELONG(mi->flags, m->hWnd));
          break;
        }
      }

      /*
        Refresh the popup
      */
      MenuRefresh(wMenu);
      bSelecting = TRUE;
      break;


    case WM_LBUTTONDBLCLK   :
    case WM_NCLBUTTONDBLCLK :
      /*
        If we are in a system menu and we double clicked on the icon,
        then post a SC_CLOSE message.
      */
      if (m->flags & M_SYSMENU)
      {
#ifdef MEWEL_GUI
        /*
          Double-clicking in the icon area will restore the icon.
        */
        if ((wParent->flags & WS_MINIMIZE) && GetMenuState(hMenu,SC_RESTORE,MF_BYCOMMAND) != (UINT) -1)
        {
          ReleaseCapture();
          bSelecting = FALSE;
          PostMessage(wParent->win_id, WM_SYSCOMMAND, SC_RESTORE, 0L);
        }
        else
        /*
          We should really take care of this in StdWindowWinProc() under
           the WM_NCHITTEST / HTSYSMENU case.
        */
        if (SendMessage(wParent->win_id,WM_NCHITTEST,0,lParam) == HTSYSMENU)
#else
        if (mouserow == -2 && mousecol == -1)
#endif
        {
          ReleaseCapture();
          bSelecting = FALSE;
          if (GetMenuState(hMenu,SC_CLOSE,MF_BYCOMMAND) != (UINT) -1)
            PostMessage(wParent->win_id, WM_SYSCOMMAND, SC_CLOSE, 0L);
        }
      }
      break;


    case WM_NCLBUTTONDOWN :
    case WM_LBUTTONDOWN   :
      m->flags &= ~M_NOSHOWPOPUPS;  /* A mouse will activate popups */

      /* Ignore clicks on the menu border... */
      if (!PtInRect(&wMenu->rClient, MAKEPOINT(lParam)))
      {
        /*
          If we released the mouse button outside of the popup, then
          get rid of the popup. If we released the button while on the
          menubar, then keep the popup...
        */
        BOOL bInParent = (wParent && IsMouseInMenuParent(wMenu,lParam));
        SendMessage(hMenu, WM_KILLFOCUS, 
                    (bInParent&&IS_MENU(wParent)) ? wParent->win_id : NULLHWND,
                    KILL_PARENT_FOCUS);

        /*
          Simulate a click on the menubar so that the menubar can 
          display the appropriate pulldown menu.
        */
        if (bInParent)
        {
          /*
            Transform the screen-relative coords in lParam into 
            parent-menu-relative coords.
          */
          lParamOrig = _WindowizeMouse(wParent->win_id, lParam);
          SendMessage(wParent->win_id, WM_LBUTTONDOWN, wParam, lParamOrig);
        }
        break;
      }

      if (IS_MENUBAR(m))
      {
        /*
          Find the menubar item which corresponds to the column we clicked on.
          Select the item and show the corresponding popup.
        */
        if ((index = MenuBarColToItem(m, mousecol)) < 0)
          break;
        SetCapture(hMenu);
        bSelecting = TRUE;
        goto lbl_select_menubar_item;
      }
      else /* a POPUP */
      {
        /*
          We pressed the button on a pulldown menu item. Select that item
          and refresh the pulldown in order to show the new highlighting.
        */
        oldSel = m->iCurrSel;

        /*
          Is the mouse above or below the popup?
        */
#if 112892 && defined(MEWEL_GUI)
        if ((index = MenuRowToItem(m, mouserow)) < 0)
          if (IS_SYSMENU(m))
            index = 0;
          else
            break;
#else
        if (mouserow < 0 || mouserow > m->nItems * SysGDIInfo.tmHeightAndSpace)
          break;
#endif
        /*
          Select the menu item which was clicked on
        */

#if 112892 && defined(MEWEL_GUI)
        if (MenuSelect(m, index, TRUE) == LB_ERR)
#else
        if (MenuSelect(m, mouserow/SysGDIInfo.tmHeightAndSpace, TRUE) == LB_ERR)
#endif
          break;
        /*
          If the selection changed, redraw the menu
        */
        if (m->iCurrSel != oldSel)
          MenuRefresh(wMenu);
        /*
          Capture the mouse
        */
        SetCapture(hMenu);
        bSelecting = TRUE;
      }
      break;


    case WM_NCLBUTTONUP :
    case WM_LBUTTONUP   :
      /*
        Ignore button-up messages if we aren't selecting. Stray button-up
        messages can occur if we are, for example, in a dialog box with the
        button down and we move up to the menubar and release the button.
      */
      if (bSelecting == FALSE)
      {
        /*
          If we clicked and released the mouse on a disabled menubar item,
          then we must relinquish the focus back to the previous focus.
        */
        if (InternalSysParams.hWndFocus == hMenu && IS_MENUBAR(m) && 
            MenuBarColToItem(m, mousecol) < 0)
          SetFocus(m->hOldFocus);
        break;
      }

      m->flags &= ~M_NOSHOWPOPUPS;

      /* Ignore clicks on the menu border... */
      if (!PtInRect(&wMenu->rClient, MAKEPOINT(lParam)))
      {
        /*
          If we released the mouse button outside of the popup, then
          get rid of the popup. If we released the button while on the
          menubar, then keep the popup...
        */
        if (wParent)
        {
          /*
            If we are in a system menu and we released the button on the
            system menu icon, then keep the system menu up.
          */
          if ((m->flags & M_SYSMENU) &&
#ifdef MEWEL_GUI
              SendMessage(wParent->win_id,WM_NCHITTEST,0,lParam) == HTSYSMENU)
#else
              mouserow==-2 && mousecol==-1)
#endif
            ReleaseCapture();
          else if (!IsMouseInMenuParent(wMenu, lParam))
            SendMessage(hMenu, WM_KILLFOCUS, 0, KILL_PARENT_FOCUS);
          else
            /*
              We're on the menubar. Keep the popup but release the capture
              so that a click outside the menu will kill the popup.
            */
#ifdef CARRYER
            /* hilight the first selectable item */
            if ((index =  _MenuGetFirstSelectableItem(m)) >= 0)
            {
              MenuSelect(m, index, TRUE);
              MenuRefresh(wMenu);
            }
#endif
            ReleaseCapture();
        }
      }
      else
      {
send_cmd:
        /*
          If we selected a menuitem which has an associated menu popup,
          show that child
        */
        if (!IS_MENUBAR(m)                                            &&
            (mi = MenuGetItem(m, m->iCurrSel, MF_BYPOSITION)) != NULL &&
            (mi->flags & MF_POPUP))
        {
           MenuBarShowPopup(hMenu, m->iCurrSel);
           SetCapture((HWND) mi->id);
           bSelecting = FALSE;
           break;
        }

        /* Save the current selection, cause KILLFOCUS resets it to -1 */
        oldSel = m->iCurrSel;
        SendMessage(hMenu, WM_KILLFOCUS, 0, KILL_PARENT_FOCUS);

        /*
          Get a pointer to the structure for the menu item
        */
        if ((mi = MenuGetItem(m, oldSel, MF_BYPOSITION)) == NULL ||
#ifdef CARRYER
            (mi->flags & (MF_DISABLED | MF_GRAYED)) || (oldSel < 0))
#else
            (mi->flags & (MF_DISABLED | MF_GRAYED)))
#endif
        {
          bSelecting = FALSE;
          break;
        }

        /*
          If we have a popup menu item, we send the WM_COMMAND msg to
          the grandparent window. If we have a menubar item that does
          not have an associated popup, we send the WM_COMMAND msg to
          the parent.
        */
        if (!IS_MENUBAR(m) && wMenu->parent->parent)
        {
          UINT idCmd;

          /*
            If the popup is tied directly to a window (and not a menubar),
            send that window the WM_COMMAND
          */
          wParent = (wMenu->parent->winproc == MenuWndProc) ?
                                       wMenu->parent->parent : wMenu->parent;

post_cmd:
          idCmd = mi->id;
          if (wMenu->parent->hSysMenu == hMenu) 
          {
            message = WM_SYSCOMMAND;
            /*
              If we are about to double-click on an iconic window, then
              restore the window.
            */
            if ((wParent->flags & WS_MINIMIZE) &&
                SysEventInfo.DblClickPendingMsg == WM_LBUTTONDBLCLK)
              idCmd = SC_RESTORE;
          }
          else if (mi->flags & MF_HELP)
            message = WM_HELP;
          else
            message = WM_COMMAND;
          PostMessage(wParent->win_id, message, idCmd, 0L);
        }
        else if (!(mi->flags & MF_POPUP) && wMenu->parent)
        {
          wParent = wMenu->parent;
          goto post_cmd;
        }
      }
/*    ReleaseCapture(); */
      bSelecting = FALSE;
      break;


    case WM_MOUSEMOVE :
      /* 
        Disregard if we are not selecting the menu
      */
      if (!bSelecting)
        break;

      /*
        If we are not dragging the mouse (ie - the left button is down),
        then disregard the mouse movement as well.
      */
#ifndef OS2
      if (!IsMouseLeftButtonDown())
        break;
#endif

      oldSel = m->iCurrSel;

      /*
        See if the mouse is still within the menu.
      */
      if (!PtInRect(&wMenu->rClient, MAKEPOINT(lParam)))
      {
#ifdef CARRYER
        m->iCurrSel = -1;
        MenuRefresh(wMenu);
#endif
        /*
          See if we dragged the mouse on the menubar while the popup had
          the focus. If not, disregard.
        */
        if (IS_MENUBAR(m) || 
#ifdef MEWEL_GUI
            HIWORD(lParam) < (wMenu->rect.top-IGetSystemMetrics(SM_CYMENU)))
#else
            HIWORD(lParam) != (wMenu->rect.top-1))
#endif
          break;
      }

      if (IS_MENUBAR(m))
      {
        /*
          Note - if we drag the mouse past a menubar item with no associated
          pulldown, *DO NOT* issue a command. (So this section of code can't
          be replaced with "goto lbl_select_menu_item;"
        */
        index = MenuBarColToItem(m, mousecol);
#ifdef CARRYER
        if (index < 0 )
          m->iCurrSel = -1;   /* deselect if we move off an item. */
        else if (index == m->iCurrSel)
          break;
#else
        if (index < 0 || index == m->iCurrSel)
          break;
#endif
        MenuSelect(m, index, TRUE);
        MenuBarShowPopup(hMenu, index);
      }
      else
      {
      	/* 
           A pulldown has the focus. We dragged the mouse in the pulldown
           or in the menubar.
        */
#ifdef MEWEL_GUI
#if 112892
        index = MenuRowToItem(m, mouserow);
        if (index == m->iCurrSel)
#else
        if (mouserow > m->nItems*VideoInfo.yFontHeight ||
            (mouserow >= m->iCurrSel*VideoInfo.yFontHeight &&
             mouserow < (m->iCurrSel+1)*VideoInfo.yFontHeight))
#endif
#else
        if (mouserow > m->nItems || mouserow == m->iCurrSel)
#endif
          break;

        if (mouserow < 0)
        { /*
            We dragged the mouse in the menubar
          */
          WINDOW *wBar = wMenu->parent;
          MENU   *mBar;

          if (wBar == NULL || !IS_MENU(wBar))
            break;
          mBar  = (MENU *) wBar->pPrivate;
          if (!IS_MENUBAR(mBar))
            break;

          index = MenuBarColToItem(mBar, LOWORD(lParam) - wBar->rClient.left);
          if (index >= 0 && index != mBar->iCurrSel)
          { /* 
              We dragged past the popup borders. The menubar will still
              have the focus.
            */
            SendMessage(hMenu, WM_KILLFOCUS, 0, DONT_KILL_PARENT_FOCUS);
            /*
              Only send a button down message to a menubar item which has a
              pulldown associated with it.
            */
            mi = MenuGetItem(mBar, index, MF_BYPOSITION);
            if (mi && (mi->flags & MF_POPUP))
            {
              lParamOrig = _WindowizeMouse(wBar->win_id, lParam);
              SendMessage(wBar->win_id, WM_LBUTTONDOWN, wParam, lParamOrig);
            }
            else
            {
              /*
                No pulldown associated with this menubar. Select & refresh.
              */
              oldSel = mBar->iCurrSel;
              InternalSysParams.hWndFocus = wBar->win_id;
              MenuSelect(mBar, index, TRUE);
#ifdef INVERT_ITEM
              MenuInvertInfo.bInverting = FALSE;
              MenuInvertInfo.iOldSel    = mBar->iCurrSel;
              MenuInvertInfo.hMenu      = wBar->win_id;
#endif
              MenuRefresh(wBar);
            }
          }
          break;
        }

#ifdef MEWEL_GUI
#if 112892
        if (MenuSelect(m, MenuRowToItem(m, mouserow), TRUE) == LB_ERR)
#else
        if (MenuSelect(m, mouserow / VideoInfo.yFontHeight, TRUE) == LB_ERR)
#endif
#else
        if (MenuSelect(m, mouserow, TRUE) == LB_ERR)
#endif
          break;
      }

#ifdef INVERT_ITEM
      /*
        Highlight the new selection in the pulldown menu
      */
      MenuInvertInfo.bInverting = TRUE;
      MenuInvertInfo.iOldSel    = oldSel;
      MenuInvertInfo.hMenu      = hMenu;
#endif
      MenuRefresh(wMenu);
#ifdef INVERT_ITEM
      MenuInvertInfo.bInverting = FALSE;
      MenuInvertInfo.iOldSel    = m->iCurrSel;
#endif
      break;



    case WM_NCACTIVATE :   /* no such thing as NCACTIVATE for menus */
      return TRUE;

    case WM_SYSCOLORCHANGE:
      if (wParam == SYSCLR_MENU || wParam == SYSCLR_MENUTEXT)
        InternalInvalidateWindow(hMenu, FALSE);
      break;


#if defined (USE_BITMAPS_IN_MENUS)
    case WM_DRAWITEM    :
    case WM_MEASUREITEM :
      return SendMessage(GetParent(hMenu), message, wParam, lParam);
#endif


    default :
      return StdWindowWinProc(hMenu, message, wParam, lParam);

  } /* switch */
  
  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function :  DestroyMenu()                                                */
/*                                                                          */
/* Purpose  :  Destroys an entire menu                                      */
/*                                                                          */
/* Returns  :  TRUE if the menu was deleted, FALSE if not.                  */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL DestroyMenu(hMenu)
  HWND hMenu;
{
  MENU     *m;
  LPMENUITEM mi;
  LIST     *p;
  WINDOW   *w;


  /*
    If we passed a handle to a normal window, use the window's menu handle
  */
  if (!IsWindow(hMenu))
    return FALSE;

  if (hMenu && !IS_MENU(WID_TO_WIN(hMenu)))
    hMenu = GetMenu(hMenu);

  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return FALSE;

  /*
    Go down the list of items and destroy all pulldown menus
  */
  for (p = m->itemList;  p;  p = p->next)
  {
    mi = (LPMENUITEM) p->data;
    if (mi->flags & MF_POPUP)
      DestroyMenu((HWND) mi->id);
    if (mi->text)
#if defined (USE_BITMAPS_IN_MENUS)
      if (!(mi->flags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
      MYFREE_FAR(mi->text);
  }

  /*
    Delete the menubar
  */
  ListFree(&m->itemList, TRUE);
  WinDelete(hMenu);

  /*
    If any windows have this menu associated with them, then we
      NULL out the hMenu field
  */
  for (w = InternalSysParams.WindowList;  w;  w = w->next)
    if (w->hMenu == hMenu)
    {
      w->hMenu = NULLHWND;
      _WinSetClientRect(w->win_id);
    }
    else if (w->hSysMenu == hMenu)
    {
      w->hSysMenu = NULLHWND;
    }
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : DrawMenuBar()                                                 */
/*                                                                          */
/* Purpose  : Draws a menubar or a pulldown menu. This function simply      */
/*            calls MenuRefresh() to do the dirty work.                     */
/*                                                                          */
/* Returns  : TRUE if the menu exists, FALSE if not.                        */
/*                                                                          */
/****************************************************************************/
static BOOL bFromDrawMenuBar = FALSE;

INT FAR PASCAL DrawMenuBar(HWND hMenu)
{
  INT rc;

  bFromDrawMenuBar = TRUE;
  rc = InternalDrawMenuBar(hMenu);
  bFromDrawMenuBar = FALSE;
  return rc;
}

INT FAR PASCAL InternalDrawMenuBar(HWND hMenu)
{
  WINDOW *wMenu = WID_TO_WIN(hMenu);

  if (wMenu == NULL)
    return FALSE;

  if (wMenu->hMenu)  /* we passed a handle to the window, not to the menu */
    wMenu = WID_TO_WIN(wMenu->hMenu);
  if (wMenu && IS_MENU(wMenu) && !TEST_WS_HIDDEN(wMenu))
  {
    /*
      4/19/93 (maa)
        Added test to see if parent was visible.
    */
    WINDOW *wParent = wMenu->parent;
    if (wParent && IsWindowVisible(wParent->win_id))
    {
#if defined(MOTIF)
      if (bFromDrawMenuBar)
        XMEWELInvalidateWindow(wMenu->win_id);
#else
      MenuRefresh(wMenu);
#endif
      wMenu->rUpdate = RectEmpty;
    }
  }
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* IsMenuBarAccel() - sees if an ALT key combo corresponds to a menubar item */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL IsMenuBarAccel(hMenu, key, pchAltKey, piPos)
  HWND hMenu;
  UINT key;
  int  *pchAltKey;
  int  *piPos;
{
  MENU     *m;
  LPMENUITEM mi;
  LIST     *p;
  int      iPos;

  /*
    Ignore non-ALT keys (keys below 128)
  */
#ifdef CDPLUS
  /* part of patch to allow CTRL to activate menus. */
  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
#else
  if (key < 128  ||  (m = _MenuHwndToStruct(hMenu)) == NULL)
#endif
    return -1;

  /*
    Ignore accel keys if the menubar is disabled
  */
  if (!IsWindowEnabled(hMenu))
    return -1;

  /*
    Find out what letter was pressed with the ALT key
  */
#ifdef INTERNATIONAL_MEWEL
  if ((*pchAltKey = key = (key & SPECIAL_CHAR) ? lang_upper ((key >> 8) & 0xFF)
                                               : AltKeytoLetter(key)) == 0)
#else
  if ((*pchAltKey = key = AltKeytoLetter(key)) == 0)
#endif
    return -1;

  /*
    Go down the list of menubar items and try matching the letter
  */
  for (iPos = 0, p = m->itemList;  p;  p = p->next, iPos++)
  {
    mi = (LPMENUITEM ) p->data;
    if (key == (UINT) lang_upper(mi->letter) &&
        !(mi->flags & (MF_DISABLED | MF_GRAYED)))
    {
      *piPos = iPos;
      return mi->id;
    }
  }

  return -1;
}


/*===========================================================================*/
/*                                                                           */
/* MenuBarColToItem() - given a column, returns the index of the menubar     */
/*                      item which occupies that column.                     */
/*                                                                           */
/*===========================================================================*/
#if !defined(MEWEL_GUI)
INT FAR PASCAL MenuBarColToItem(m, col)
  MENU *m;
  int  col;       /* this is 0-based from the menu, *not* screen coords */
{
  LIST     *p;
  LPMENUITEM mi;
  int  len, pos;
  int  index;

  index = 0;
  pos = GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM);

  /*
    Go through all of the items in the menubar
  */
  for (p = m->itemList;  p;  p = p->next, index++)
  {
    mi = (LPMENUITEM) p->data;

    /*
      Determine how many columns the menu text occupies
    */
    if (mi->text == NULL)
      len = 0;
    else
      len = lstrlen(mi->text) - (lstrchr(mi->text, HILITE_PREFIX) ? 1 : 0);

    /*
      Special processing for right-justified items
    */
    if (mi->flags & (MF_HELP | MF_RIGHTJUST))
    {
      WINDOW *wMenu = WID_TO_WIN(m->hWnd);
      int    iLastCol;

      if (!wMenu)
        continue;

      /*
        Calculate the last column of the menu
      */
      iLastCol = wMenu->rClient.right - wMenu->rClient.left - 1;

      /*
        Special processing for MDI restore icon
      */
      if (wMenu->ulStyle & MFS_RESTOREICON)
      {
        /*
          Did we click on the MDI restore icon?
        */
        if (col == iLastCol)
          return GetMenuItemCount(m->hWnd)-1;

        /*
          The MDI restore icon takes up 2 columns, so the item ends 2 columns
          from the right side.
        */
        iLastCol -= 2;
      }

      /*
        Use the new testing position below.
      */
      pos = iLastCol - len - GetSysMetrics(SM_CXBETWEENMENUITEMS);
    }

    /*
      See if the column falls between the start of the menu item and the
      end. If so, return the index of the item. Otherwise, advance the
      testing position to the next item.
    */
    if (col >= pos && col < pos + len)
      return (mi->flags & (MF_DISABLED | MF_GRAYED)) ? -1 : index;
    else
      pos += len + GetSysMetrics(SM_CXBETWEENMENUITEMS);
  }

  /*
    No menubar item corresponds to the mouse position
  */
  return -1;
}
#endif


/*===========================================================================*/
/*                                                                           */
/* MenuBarItemToCol() - given a menubar item index,returns the column        */
/*                      number where the item starts.                        */
/*                                                                           */
/*===========================================================================*/
#if !defined(MEWEL_GUI)
int FAR PASCAL MenuBarItemToCol(m, index)
  MENU *m;
  int  index;
{
  LIST *p;
  LPMENUITEM mi;
  int  i;
  int  col;


  /*
    Go through all of the menubar items. 
  */
  for (i = col = 0, p = m->itemList;  p && i < index;  p = p->next, i++)
  {
    mi = (LPMENUITEM ) p->data;
    if (mi->text)
    {
      int sLen = lstrlen(mi->text);
      if (lstrchr(mi->text, HILITE_PREFIX) != NULL)
        sLen--;
      col += sLen + GetSysMetrics(SM_CXBETWEENMENUITEMS);
    }
  }


  /*
    Special case for a right-justified menubar item.
  */
  if (p)
  {
    mi = (LPMENUITEM ) p->data;
    if (mi->flags & (MF_HELP | MF_RIGHTJUST))
    {
      WINDOW *wMenu = WID_TO_WIN(m->hWnd);
      int sLen = lstrlen(mi->text);
      if (lstrchr(mi->text, HILITE_PREFIX) != NULL)
        sLen--;

      col = wMenu->rClient.right - sLen;
      if ((mi->flags & MF_HELP) && (wMenu->ulStyle & MFS_RESTOREICON))
        col -= 2;
    }
  }

  return col + GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM);

}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : MenuBarIDToPos()                                              */
/*                                                                          */
/* Purpose  : Scans a menu, searching for an item with the desired id.      */
/*                                                                          */
/* Returns  : The 0-based position of the item. If the item wasn't found,   */
/*            returns -1.                                                   */
/*                                                                          */
/****************************************************************************/
static INT PASCAL MenuBarIDToPos(m, id)
  MENU *m;
  int  id;
{
  LIST *p;
  int  index;

  for (index = 0, p = m->itemList;  p;  p = p->next, index++)
    if (((LPMENUITEM ) p->data)->id == id)
      return index;
  return -1;
}

/*===========================================================================*/
/*                                                                           */
/* MenuBarShowPopup() -                                                      */
/*  Displays the popup menu which is associated with a menubar item          */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL MenuBarShowPopup(hBar, index)
  HWND hBar;   /* handle of the menubar */
  int  index;  /* 0-based index of menubar item */
{
  WINDOW   *wMenu = WID_TO_WIN(hBar);
  MENU     *m = (MENU *) wMenu->pPrivate;
  LPMENUITEM mi;
  LIST     *p;
  HWND     hPopup;

  if ((p = ListGetNth(m->itemList, index)) == NULL)
    return FALSE;
  mi = (LPMENUITEM ) p->data;

  if (mi->flags & MF_POPUP)
  {
    /* The menubar item has a popup associated with it. */
    hPopup = (HWND) mi->id;

    /*
      Tell the parent that we showed a popup...
    */
    if (wMenu->parent)
      SendMessage(wMenu->parent->win_id, WM_INITMENUPOPUP, hPopup,
                  MAKELONG(index, (mi->flags & MF_SYSMENU) ? 1 : 0));
    /*
      We don't want to use SetFocus here, as we don't want a KILLFOCUS
      message sent to the menubar... 
    */
    InternalSysParams.hWndFocus = hPopup;
    SendMessage(hPopup, WM_SETFOCUS, 0, 0L);
    return TRUE;
  }
  else
  {
    /*
      We have a menubar item which has no associated popup. We want to 
      make sure that the menubar has focus and receives the LEFT and RIGHT
      arrow keys that the user presses, so we merely set the focus to the
      menubar.
    */
    InternalSysParams.hWndFocus = hBar;
    return FALSE;  /* no popup associated with this menubar item */
  }
}

/*===========================================================================*/
/*                                                                           */
/* MenuChar() - processes a keyboard character which was pressed in a menu   */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL MenuChar(wMenu, key)
  WINDOW *wMenu;
  UINT    key;
{
  HMENU    hMenu = wMenu->win_id;
  HWND     hRoot;
  MENU     *m = (MENU *) wMenu->pPrivate;
  int      oldCurrSel, iCurrSel;
  LPMENUITEM mi;
  
  oldCurrSel = m->iCurrSel;

  /*
    If we are in a popup, and the user pressed the left or right keys,
    then we want to set focus back to the menu bar and let the menu
    bar move sideways...
  */
  if (!IS_MENUBAR(m) && (key == VK_LEFT || key == VK_RIGHT))
  {
    BOOL bInSubPopup;
    BOOL bKillSubPopups;
    WINDOW *wMenubar;
    WINDOW *wParent;

    /*
      See if the parent of the popup is a regular window. It can be
      if we are in a system menu or a trackable popup menu.
    */
    wParent = wMenu->parent;
    if (!wParent)
      return FALSE;

    /*
      The left/right arrow keys should have no effect on a system menu
    */
    if (wParent->hSysMenu == hMenu) 
      return FALSE;

    /*
      If we have a second-level (or greater) submenu attached to the current
      item, and we press the right arrow key, we want to set display that
      submenu.
    */
    if (key == VK_RIGHT && GetSubMenu(hMenu, oldCurrSel))
    {
      MenuBarShowPopup(hMenu, oldCurrSel);
      return TRUE;
    }

    /*
      If we are in a 1st level trackable popup menu, then return, since the
      left or right arrow keys shouldn't do anything.
    */
    if (!IS_MENU(wParent))
      return FALSE;

    /*
      Set 'm' to the parent's menu structure
    */
    m = (MENU *) wParent->pPrivate;

    /* Save curr menubar selection, since SETFOCUS resets it to 0 */
    oldCurrSel = m->iCurrSel;

    /*
      We are in a second-level popup if the parent of the popup
      if not a menubar.
    */
    bInSubPopup = (BOOL) !IS_MENUBAR(m);

    /*
      If we have no more sub-popups and we press the right arrow,
      we want to move to the next popup on the menubar.
    */
    bKillSubPopups = (BOOL) (key == VK_RIGHT && bInSubPopup &&
                   (wMenubar = GetMenubarFromPopup(wMenu)) != NULL);

    if (bKillSubPopups)
    {
      /*
        Save the index of the menubar item which is currently selected.
        Kill all of the popups, then move rightwards from the saved
        selection.
      */
      m = (MENU *) wMenubar->pPrivate;
      oldCurrSel = m->iCurrSel;
      SendMessage(hMenu, WM_KILLFOCUS, 0, KILL_PARENT_FOCUS);
      m->iCurrSel = oldCurrSel;
      m->iCurrSel = MenuMove(m, VK_RIGHT);
    }
    else
    {
      wMenubar = wParent;
      SendMessage(hMenu, WM_KILLFOCUS, 0, DONT_KILL_PARENT_FOCUS);
      m->iCurrSel = oldCurrSel;
      if (!bInSubPopup)
        m->iCurrSel = MenuMove(m, key);
    }
    if (wMenubar)
      SendMessage(wMenubar->win_id, MM_SELECT, m->iCurrSel, 0L);
    return TRUE;
  }
  else if (!IS_POPUP(m) && (key == VK_LEFT || key == VK_RIGHT))
  {
    m->iCurrSel = MenuMove(m, key);
    SendMessage(hMenu, MM_SELECT, m->iCurrSel, 0L);
    return TRUE;
  }

  switch (key)
  {
    case VK_LEFT :
    case VK_UP   :
    case VK_RIGHT:
    case VK_DOWN :
      if (DONT_SHOW_POPUPS(m))
      {
        PostMessage(hMenu, WM_KEYDOWN, VK_RETURN, 0L);
        return TRUE;
      }
    case VK_HOME :
    case VK_END  :
      MenuSelect(m, MenuMove(m, key), TRUE);
      break;

    case VK_F1   :
      if (wMenu->parent)
      {
        if ((mi = MenuGetItem(m, m->iCurrSel, MF_BYPOSITION)) != NULL)
          PostMessage(wMenu->parent->win_id, WM_HELP, mi->id, 0L);
      }
      break;


    case VK_ESCAPE :
do_escape:
      /*
        If we're in a second-level submenu, and we press ESC, back up to
        the previous submenu.
      */
      m = (MENU *) wMenu->parent->pPrivate;
      if (IS_MENU(wMenu->parent) && m && !IS_MENUBAR(m) && m->iLevel >= 1)
      {
        PostMessage(hMenu, WM_KEYDOWN, VK_LEFT, 0L);
        return TRUE;
      }

      SendMessage(hMenu, WM_KILLFOCUS, 0, KILL_PARENT_FOCUS);
      return TRUE;

    default   :
      if (key)
      {
        if ((iCurrSel = MenuFindPrefix(m, key, m->iCurrSel+1)) != LB_ERR ||
            (iCurrSel = MenuFindPrefix(m, key, 0)) != LB_ERR)
        {
select_item:
          MenuSelect(m, iCurrSel, TRUE);
          if (DONT_SHOW_POPUPS(m))
          {
            PostMessage(hMenu, WM_KEYDOWN, VK_RETURN, 0L);
            return TRUE;
          }
          /*
            If we selected a menu item which invokes a submenu,
            then we must change the highlite to go to that item.
          */
          if ((mi = MenuGetItem(m, iCurrSel, MF_BYPOSITION)) != NULL &&
               (mi->flags & MF_POPUP) && m->iCurrSel != oldCurrSel)
            MenuRefresh(wMenu);
          return MM_SELECT;
        }

        /*
          It's not a menubar item.
          Send a WM_MENUCHAR message to the menu owner as per MS-Windows.
        */
        if ((hRoot = WinGetMenuRoot(hMenu)) != NULLHWND)
        {
          DWORD ulRet = (DWORD) SendMessage(hRoot, WM_MENUCHAR, (WPARAM) key,
                                            MAKELONG(MF_POPUP, hMenu));
          if (ulRet == 0)
            MessageBeep(0);
          else if (HIWORD(ulRet) == 1)
            goto do_escape;
          else if (HIWORD(ulRet) > 1)
          {
            iCurrSel = LOWORD(ulRet);
            goto select_item;
          }
        }
      }
      break;
  }

  /*
    Refresh only if we've changed the selection or topline
  */
  if (m->iCurrSel != oldCurrSel)
  {
#ifdef INVERT_ITEM
    MenuInvertInfo.bInverting = TRUE;
    MenuInvertInfo.iOldSel    = oldCurrSel;
    MenuInvertInfo.hMenu      = hMenu;
#endif
    MenuRefresh(wMenu);
#ifdef INVERT_ITEM
    MenuInvertInfo.bInverting = FALSE;
    MenuInvertInfo.iOldSel    = m->iCurrSel;
#endif
  }

  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Function : MenuFindPrefix()                                               */
/*                                                                           */
/* Purpose  : Searches a menu from position 'start' in search of             */
/*            the next item with the specified prefix.                       */
/*                                                                           */
/* Returns  : The 0-based position of the matching item, LB_ERR if no        */
/*            matching item was found.                                       */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL MenuFindPrefix(m, prefix, start)
  MENU *m;
  INT  prefix;
  INT  start;
{
  LIST *p;
  LPMENUITEM mi;
  
  if (start >= m->nItems)
    return LB_ERR;

  /*
    Try to match the first character of the prefix with the "hot" letter
    character of each menu item if we are trying to match only 1 letter.
  */
  for (p = ListGetNth(m->itemList, start);  p;  p = p->next, start++)
  {
    mi = (LPMENUITEM) p->data;
    if (!(mi->flags & (MF_SEPARATOR | MF_DISABLED)))
      if (lang_upper((BYTE) prefix) == lang_upper((BYTE) mi->letter))
        return start;
  }

  return LB_ERR;
}


/*===========================================================================*/
/*                                                                           */
/* Function : MenuGetItem()                                                  */
/*                                                                           */
/* Purpose  : Retrieves a menu item by position or by control ID             */
/*                                                                           */
/* Returns  : A pointer to the menu item structure.                          */
/*                                                                           */
/*===========================================================================*/
LPMENUITEM FAR PASCAL MenuGetItem(m, index, flags)
  MENU *m;
  int  index;
  UINT flags;
{
  LIST *pNth, *p;
  LPMENUITEM mi;
  
  if (!(flags & MF_BYPOSITION))  /* search the menu for the proper id... */
  {
    for (p = m->itemList;  p;  p = p->next)
    {
      WINDOW *popw;
      mi = (LPMENUITEM ) p->data;
      /*
        We do not want to mistakenly compare the index with the submenu handle
      */
      if (mi->id == index && !(mi->flags & MF_POPUP))
        return mi;
      if ((mi->flags & MF_POPUP))    /* recurse on the sub-menu... */
      {
        if ((popw = WID_TO_WIN(mi->id)) != NULL &&
	(mi = MenuGetItem((MENU *) popw->pPrivate, index, flags)) != NULL)
          return mi;
      }	  
    }
    return (LPMENUITEM ) NULL;
  }
  else   /* MF_BYPOSITION */
  {
    /*
      Get the index'th item
    */
    if ((pNth = ListGetNth(m->itemList, index)) == NULL)
      return (LPMENUITEM ) NULL;
    return (LPMENUITEM ) pNth->data;
  }
}


/*===========================================================================*/
/*                                                                           */
/* Function : MenuGetMaxLen()                                                */
/*                                                                           */
/* Purpose  : Gets the length of the longest string in a menu so we can      */
/*            automatically size the menu properly                           */
/*                                                                           */
/* Returns  : The length of the longest string plus the space for the        */
/*            checkmark and blank.                                           */
/*                                                                           */
/*===========================================================================*/
#if 112892 && !defined(MEWEL_GUI)
static INT PASCAL MenuGetMaxLen(m)
  MENU *m;
{
  int      len, maxlen;
  LIST     *p;
  LPMENUITEM mi;
  
  for (maxlen = 0, p = m->itemList;  p;  p = p->next)
  {
    mi = (LPMENUITEM ) p->data;
    if ((mi->flags & MF_SEPARATOR))
      continue;

    /*
      Test the length of the current string
    */
    if ((len=lstrlen(mi->text)-(lstrchr(mi->text,HILITE_PREFIX)?1:0)) > maxlen)
      maxlen = len;

    /*
      Account for a tab
    */
    if (lstrchr(mi->text, '\t') != NULL)
    {
      len += 8;
      if (len > maxlen)
        maxlen = len;
    }
  }

  maxlen += 2;    /* + 2 to accomodate a space and checkmark */

#ifdef MEWEL_GUI
  return maxlen * VideoInfo.xFontWidth;
#else
  return maxlen;
#endif
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : MenuGetPopupHeight(MENU *)                                    */
/*                                                                          */
/* Purpose  : Calculates the height of a popup menu. This is important      */
/*            for owner-drawn menu items or menu items which are MF_BITMAP. */
/*                                                                          */
/* Returns  : The total height of the popup, not including borders.         */
/*                                                                          */
/****************************************************************************/
#if 112892 && !defined(MEWEL_GUI)
static INT PASCAL MenuGetPopupHeight(m)
  MENU *m;
{
  PLIST      pList;
  LPMENUITEM mi;
  INT        iHeight = 0;
  
  for (pList = m->itemList;  pList;  pList = pList->next)
  {
    mi = (LPMENUITEM) pList->data;
    /*
      We should send the WM_MEASUREITEM message for owner-drawn items.
      For bitmap items, we should figure out the height of the bitmap.
    */
    iHeight += mi->cyItem;
  }
  return iHeight;
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : MenuMove()                                                    */
/*                                                                          */
/* Purpose  : Given a menu and a direction, moves to the next selectable    */
/*            item in the menu.                                             */
/*                                                                          */
/* Returns  : The 0-based index of the new selection.                       */
/*                                                                          */
/****************************************************************************/
static INT PASCAL MenuMove(pMenu, direction)
  MENU *pMenu;      /* the current menu box */
  UINT direction;   /* _UP, _DOWN, _LEFT, _RIGHT */
{
  int        idxStart, idx;
  LPMENUITEM mi;

  /*
    Do a quick search to see if we have any valid items in the menu.
  */
  if (_MenuGetFirstSelectableItem(pMenu) < 0)
    return -1;

  /*
    Start searching at the current selection.
  */
  if ((idxStart = pMenu->iCurrSel) == -1)
    idxStart = 0;
  idx = idxStart;

  /*
    Simulate moving to the first item by starting at the last item and moving
    down. Likewise, simulate moving to the last item by starting at the first
    item and moving up.
  */
  if (direction == VK_HOME)
  {
    idxStart = idx = pMenu->nItems - 1;
    direction = VK_DOWN;
  }
  else if (direction == VK_END)
  {
    idxStart = idx = 0;
    direction = VK_UP;
  }

  for (;;)
  {
    /*
      Move to the next/prev menu item.
    */
    if (direction == VK_UP || direction == VK_LEFT)   /* move back */
      idx = (idx <= 0) ? pMenu->nItems - 1 : idx - 1;
    else                                              /* move forward */
      idx = (idx >= pMenu->nItems-1) ? 0 : idx + 1;
      
    /*
      Get around Borland codegen bug
    */
    if (idx >= pMenu->nItems)
      idx = pMenu->nItems-1;

    /*
      Return if we have wrapped around to the original searching point.
    */
    if (idx == idxStart)
      return idx;

    /*
      Get information about this menu item.
    */
    mi = (LPMENUITEM) ListGetNth(pMenu->itemList, idx)->data;

    /*
      If we are going across the menubar and we reach a disabled menubar
      item, then continue searching.
    */
    if (IS_MENUBAR(pMenu) && (mi->flags & (MF_DISABLED | MF_GRAYED)))
      continue;

    /*
      If we have an item which is not a separator, return.
    */
    if (!(mi->flags & MF_SEPARATOR))
      return idx;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : MenuSaveScreen()                                              */
/*                                                                          */
/* Purpose  : Saves and restores the area under a popup menu.               */
/*                                                                          */
/* Returns  : TRUE if saved, FALSE if not.                                  */
/*                                                                          */
/****************************************************************************/

/*
  Stack of screen saves for MenuSaveScreen()
*/
#define MAXSTACK  8
static int MenuSaveSP = -1;

typedef struct tagMenuSaveInfo
{
  HANDLE hSavedScreen;
  RECT  rSavedScreen;
  HMENU hMenu;
  BOOL  bFlipped;
  RECT  rOriginal;
} MENUSAVEINFO, *PMENUSAVEINFO;

MENUSAVEINFO _MenuSaveInfo[MAXSTACK];

extern INT FAR PASCAL WinAdjustPopupWindow(HWND, LPVOID);

INT FAR PASCAL MenuSaveScreen(HWND hWnd, BOOL bSave)
{
  HANDLE hScreen;
  RECT r;
  BOOL bFlipped;
  PMENUSAVEINFO pSaveInfo;
  int  iMenu;

  if (MenuSaveSP >= MAXSTACK)
    return FALSE;

  if (bSave)
  {
    /*
      Before we save the screen, we should make sure that all of the windows
      have been properly updated. Otherwise, the first _PeekMsg() below
      can cause the invalid windows to be refreshed, thereby making the
      saved area totally wrong.
    */
    if (TEST_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST))
      RefreshInvalidWindows(_HwndDesktop);
    pSaveInfo = &_MenuSaveInfo[++MenuSaveSP];
    bFlipped = FALSE;

    bFlipped = WinAdjustPopupWindow(hWnd, pSaveInfo);
    GetWindowRect(hWnd, &r);   /* get the new coordinates of the pulldown */

#ifndef MEWEL_GUI
    r.right += 2;  r.bottom++;   /* in case of shadow effect... */
#endif

    r.right = min(r.right, (int) VideoInfo.width);

    /*
      Possible bug - what if we switch modes between save and restore?
    */
    if (TEST_PROGRAM_STATE(STATE_NO_SAVEBITS))
      hScreen = NULL;
    else
      hScreen = _WinSaveRect(hWnd, &r);
    pSaveInfo->hSavedScreen = hScreen;
    pSaveInfo->rSavedScreen = r;
    pSaveInfo->hMenu        = hWnd;
    pSaveInfo->bFlipped     = bFlipped;
    if (hScreen == NULL)
      return FALSE;
  }
  else
  {
    if (MenuSaveSP < 0)
      return FALSE;

    pSaveInfo = &_MenuSaveInfo[MenuSaveSP--];
    hScreen = pSaveInfo->hSavedScreen;
    if (hScreen != NULL)
    {
      WinRestoreRect(hWnd, &pSaveInfo->rSavedScreen, hScreen);
      GlobalFree(hScreen);
    }

    if (pSaveInfo->bFlipped)
      WinMove(hWnd, pSaveInfo->rOriginal.top, pSaveInfo->rOriginal.left);

    WinUpdateVisMap();
    /*
      In case we just killed a submenu, we want to re-establish the
      visibility map for the higher level submenus. If we don't do this,
      then the highlight bar for the higher level submenus will never get
      refreshed.
    */
    for (iMenu = 0;  iMenu <= MenuSaveSP;  iMenu++)
      _WinUpdateVisMap(WID_TO_WIN(_MenuSaveInfo[iMenu].hMenu));

    if (!hScreen)
    {
      WinGenInvalidRects(_HwndDesktop, (LPRECT) &pSaveInfo->rSavedScreen);
      /*
        Refresh right away so the menu has the illusion of disappearing
        right away instead of waiting around until the refresh cycle.
      */
      RefreshInvalidWindows(_HwndDesktop);
    }
  }

  return TRUE;
}



/*
  We need to check the dimensions of the pulldown to make sure that
  it doesn't get clipped to the screen. If it does, then we move
  the pulldown so that it is displayed on top of the menubar
  or system menu).
*/
INT FAR PASCAL WinAdjustPopupWindow(hWnd, pSaveInfo)
  HWND   hWnd;
  LPVOID pSaveInfo;
{
  RECT r;

  GetWindowRect(hWnd, &r);     /* get the coordinates of the pulldown */

  /*
    Does any part of the menu/combolistbox lie off the screen?
  */
  if (r.bottom > (int) VideoInfo.length || r.top < 0 || 
      r.left < 0 || r.right > VideoInfo.width)
  {
    if (pSaveInfo)
      ((PMENUSAVEINFO) pSaveInfo)->rOriginal = r;

    /*
      Adjust the x coordinate
    */
    if (r.left < 0)
      OffsetRect(&r, -r.left, 0);
    else if (r.right > VideoInfo.width)
      OffsetRect(&r, VideoInfo.width - r.right, 0);

    /*
      Adjust the y coordinate
    */
    if (r.top < 0)
      OffsetRect(&r, 0, -r.top);
    else if (r.bottom > VideoInfo.length)
      OffsetRect(&r, 0, -RECT_HEIGHT(r));

    /*
      Move the window to the new location
    */
    WinMove(hWnd, r.top, r.left);

    return TRUE;
  }

  else
  {
    return FALSE;
  }
}



/*===========================================================================*/
/*                                                                           */
/* MenuSelect() - make item 'index' the currently selected menu item         */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL MenuSelect(m, index, bHilite)
  MENU     *m;      /* pointer to menu structure */
  int      index;		/* 0-based index of the menu item to select */
  int      bHilite; /* TRUE if we should highlite the item */
{
  LPMENUITEM mi;

  if (index < 0 || index >= m->nItems)
    return LB_ERR;
    
  if (bHilite)
  {
    mi = MenuGetItem(m, index, MF_BYPOSITION);
    if (!mi)
       return LB_ERR;
    if ((mi->flags & (MF_SEPARATOR)))   /* if this is a seperator */
    {                                   /* then deselect everything */
#ifdef CARRYER
      m->iCurrSel = -1;
#endif
      return LB_ERR;
    }

    m->iCurrSel = index;

    /* Send the WM_MENUSELECT msg only if we are in a pulldown */
    if (!IS_MENUBAR(m))
    {
      WINDOW *wMenu = WID_TO_WIN(m->hWnd);
      WINDOW *pw;

      /*
        We send the message if we are in a system menu or a pulldown.
        wMenu->parent->parent is the window that owns the menubar
      */
      if (wMenu && (pw = wMenu->parent) != NULL &&
          (pw->hSysMenu == m->hWnd || (pw = pw->parent) != NULL))
       SendMessage(pw->win_id,WM_MENUSELECT,mi->id,MAKELONG(mi->flags,m->hWnd));
    }
  }

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : SetMenu()                                                     */
/*                                                                          */
/* Purpose  : Attaches (or removes) a menubar to a window.                  */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL SetMenu(hWnd, hMenu)
  HWND  hWnd;
  HMENU hMenu;
{
  WINDOW *wParent = WID_TO_WIN(hWnd);
  WINDOW *wMenu   = WID_TO_WIN(hMenu);
  WINDOW *wChild;
  int    bReplaced;
  BOOL   bPopupMenu = FALSE;

  if (!wParent)
    return FALSE;

  /*
    See if the menu we are passing is valid...
  */
  if (hMenu && !wMenu)
    return FALSE;

  /*
    Make sure that we are really passing in a menu as the hMenu argument.
  */
  if (wMenu && !IS_MENU(wMenu))
    wMenu = (WINDOW *) NULL;

  /*
    bReplaced is
      0  the window did not have a menubar, and we are *not* setting one now
      1  the window has a menubar, and we are taking it away.
      2  replaced an existing menubar with another menubar
      3  the window did not have a menubar, and we are setting one now

      If bReplaced is 1 or 3, then we are modifying the client area of 
      the window. If it is 3, then we should move the top-level children
      down by the size of the menubar.
  */


  /*
    Hide the old menubar so it doesn't get any clicks
  */
  if (wParent->hMenu)
  {
    WINDOW *wOldMenu = WID_TO_WIN(wParent->hMenu);
    if (wOldMenu)
    {
      bPopupMenu = (BOOL) ((_MenuHwndToStruct(wParent->hMenu)->flags & M_POPUP) != 0);
      SET_WS_HIDDEN(wOldMenu);
      wOldMenu->flags &= ~WS_VISIBLE;
#if defined(XWINDOWS) && !defined(MOTIF)
      XUnmapWindow(XSysParams.display, wOldMenu->Xwindow);
      _XFlushEvents();
#endif
#if defined(MOTIF)
      XMEWELRemoveMenu(hWnd);
#endif
    }
    bReplaced = (hMenu == NULL) ? 1 : 2;
  }
  else  /* the window did not have a menubar to begin with */
  {
    /*
      If a window did not orinally have a menu, and we are not
      giving it a menu now, then don't call the vismap stuff.
    */
    bReplaced = (hMenu == NULL) ? 0 : 3;
  }

  /*
    Set the parent's hMenu field to the new value
  */
  wParent->hMenu = hMenu;

  /*
    Attach the new menubar to the parent, adjust all of its popups,
    and make it visible.
  */
  if (wMenu)
  {
    MENU *m = (MENU *) wMenu->pPrivate;
    wMenu->parent = wParent;        /* Attach uplink to parent.      */
    _MenuFixPopups(wParent, hMenu); /* Move to the correct position. */
    if (m->flags & M_MENUBAR)       /* Don't make popups visible yet.*/
    {
      CLR_WS_HIDDEN(wMenu);         /* Make sure the menu is visible now */
      wMenu->flags |= WS_VISIBLE;   /* Make sure the menu is visible now */
    }
    else
    {
      bPopupMenu = TRUE;
    }
    if (wParent->flags & WS_CLIP)   /* Inherit clipping from parent. */
      wMenu->flags |= WS_CLIP;
  }

  /*
    Adjust the overlapped window's client rectangle so that the
    menu can be put right above the client area.
  */
  if (!bPopupMenu)
    _WinSetClientRect(hWnd);

  /*
    If we add a menubar to the frame window after the frame's child
    windows are created, then we must slide the children down so that
    they start below the new menubar.
  */
#if !defined(MOTIF)
  if (bReplaced == 3 && !bPopupMenu)
    for (wChild = wParent->children;  wChild;  wChild = wChild->sibling)
    {
      WinMove(wChild->win_id,
              wChild->rect.top+IGetSystemMetrics(SM_CYMENU), wChild->rect.left);
    }
#endif

  /*
    Update the vismap only if we replaced or hid the original menu
  */
#if defined(MEWEL_TEXT)
  if (bReplaced)
  {
    /*
      Only update the vismap if the window's parent is visible. This will
      save time if, for instance, we are doing ShowWindow()s on a bunch
      of controls whose parent is still not visible. Dialog boxes have
      NULL parents, so they need to updated always.
    */
    if (wMenu == NULL || ShouldWeUpdateVismap(wMenu))
      WinUpdateVisMap();
  }
#endif

  /*
    If we did something to the menu, then we need to update the
    window's non-client area.
  */
  if (bReplaced >= TRUE && !bPopupMenu)
    InvalidateNCArea(hWnd);

#if defined(MOTIF) && !defined(USE_UIL_FOR_MENUS)
  if (bReplaced >= 2 && !bPopupMenu)
  {
    XSetMenu(hWnd, (LPCSTR) NULL, hMenu, bReplaced);
  }
#endif

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _MenuHwndToStruct()                                           */
/*                                                                          */
/* Purpose  : Given a window or a menu, returns a ptr to the MENU struct.   */
/*                                                                          */
/* Returns  : A pointer the the MENU structure.                             */
/*                                                                          */
/****************************************************************************/
MENU *FAR PASCAL _MenuHwndToStruct(hMenu)
  HWND hMenu;
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hMenu)) == (WINDOW *) NULL)
    return (MENU *) NULL;
  if (!IS_MENU(w))   /* did we pass the handle of the menu's owner? */
    return _MenuHwndToStruct(w->hMenu);
  return (MENU *) w->pPrivate;
}


/****************************************************************************/
/*                                                                          */
/* Function : _MenuFixPopups()                                              */
/*                                                                          */
/* Purpose  : Goes through a menu system and tidies up the origins and the  */
/*            sizes of the popups.                                          */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _MenuFixPopups(wParent, hBar)
  WINDOW *wParent;
  HWND   hBar;
{
#if !defined(MOTIF)
  HMENU  hPopup;
  MENU   *m, *mPopup;
  WINDOW *wMenu;
  WINDOW *wPopup;
  LIST   *p;
  INT    iItem;

#if defined(MEWEL_GUI)
  HDC    hDC;
#endif


  /*
    Get a pointer to the menu structure
  */
  if (!hBar || (wMenu = WID_TO_WIN(hBar)) == NULL)
    return;
  m = (MENU *) wMenu->pPrivate;

#if defined(MEWEL_GUI)
  /*
    In graphics mode, we must realize the system font so that the text
    extents get calculated correctly. We might have a proportional system
    font.
  */
  hDC = GetDC(NULL);
#endif


  /*
    If the menu is a menubar and is attached to a window, then we move
    the menubar to the upper-left corner of the window.
  */
  if (wParent && m->iLevel == 0)
  {
#if defined(MEWEL_GUI)
    int cxBorder  = 0;
    DWORD dwFlags = wParent->flags;

    if (!(dwFlags & WS_MAXIMIZE))
    {
      if (dwFlags & WS_THICKFRAME)
        cxBorder = IGetSystemMetrics(SM_CXFRAME);
      else if (HAS_BORDER(dwFlags))
        cxBorder = IGetSystemMetrics(SM_CXBORDER);
    }
#endif

    WinMove(hBar, wParent->rClient.top - IGetSystemMetrics(SM_CYMENU),
                  wParent->rClient.left);
    WinSetSize(hBar, IGetSystemMetrics(SM_CYMENU),
#if defined(MEWEL_GUI)
               WIN_WIDTH(wParent) - (2 * cxBorder));
#else
               WIN_CLIENT_WIDTH(wParent));
#endif

#if defined(MEWEL_GUI)
    MenuComputeItemCoords(m);
#endif
  }

  /*
    Go through all of the pulldowns (or submenus) and move them as well.
  */
  for (iItem = 0, p = m->itemList;  p;  p = p->next, iItem++)
  {
    LPMENUITEM mi = (LPMENUITEM ) p->data;

    /*
      Only deal with popups and submenus
    */
    if (!(mi->flags & MF_POPUP))
      continue;

    hPopup = (HMENU) mi->id;
    wPopup = WID_TO_WIN(hPopup);
    mPopup = (MENU *) wPopup->pPrivate;

    /*
      Calculate the size of the popup
    */
    MenuAdjustPopupSize(mPopup);

    /* 
      Account for the pulldown going past the right side of the menubar
    */
    if (m->iLevel == 0)    /* we're dealing with a popup of a menubar... */
    {
      int col = MenuBarItemToCol(m, iItem);
      int iMaxRight;
      int cxSlideLeft;

#if defined(USE_BITMAPS_IN_MENUS)
      BOOL bMaxMDIMenu = (BOOL) ((wMenu->ulStyle & MFS_RESTOREICON) != 0);
#endif

      /*
        First-level popups should start right below the menubar and
        right under the menubar string.
      */
#if defined(MEWEL_GUI)
      cxSlideLeft = SysGDIInfo.tmAveCharWidth + IGetSystemMetrics(SM_CXBORDER);
#if defined(USE_BITMAPS_IN_MENUS)
      if (iItem == 0 && bMaxMDIMenu)
        cxSlideLeft -= SysGDIInfo.tmAveCharWidth;
#endif
#else /* MEWEL_TEXT */
      cxSlideLeft = GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM);
#endif

      WinMove(hPopup, wMenu->rClient.top + IGetSystemMetrics(SM_CYMENU), 
                      wMenu->rClient.left + col - cxSlideLeft);

      /*
        Make sure that the right side of the popup does not go past
        the right side of the screen or the right side of the menubar.
      */
      if ((iMaxRight = VideoInfo.width) > wMenu->rect.right)
        iMaxRight = wMenu->rect.right;
      if (wPopup->rect.right > iMaxRight)
        OffsetRect(&wPopup->rect, iMaxRight - wPopup->rect.right, 0);
    }
    else
    {
      /* 
        We're dealing with a second (or greater) level submenu. We want
        to move it next to it's attached item.
      */
      WinMove(hPopup,
              wMenu->rect.top + (iItem * SysGDIInfo.tmHeightAndSpace),
              wMenu->rect.right);
    }

    /*
      Set the client area of the popup. Popups always have borders.
    */
    wPopup->rClient = wPopup->rect;
    InflateRect(&wPopup->rClient, -IGetSystemMetrics(SM_CXBORDER),
                                  -IGetSystemMetrics(SM_CYBORDER));

    /*
      Adjust the level of the popup
    */
    mPopup->iLevel = m->iLevel+1;

    /*
      Recurse on the popup so we can take care of submenus
    */
    _MenuFixPopups(NULL, hPopup);
  }

#if defined(MEWEL_GUI)
  ReleaseDC(NULL, hDC);
#endif

#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : MenuAdjustPopupSize()                                         */
/*                                                                          */
/* Purpose  : Calculates the proper size of a popup menu.                   */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL MenuAdjustPopupSize(m)
  MENU *m;
{
#if !defined(MOTIF)
  /*
    Set the new size of the popup. The width is set to the width of the
    longest string in the popup (plus space for a checkmark and a blank),
    plus the size of the borders.
    The height is set to the number of items in the popup plus the size
    of the borders.
  */
  if (!IS_MENUBAR(m))
#if defined(MEWEL_GUI)
    MenuComputeItemCoords(m);
#else
    WinSetSize(m->hWnd, 
               MenuGetPopupHeight(m) + 2*IGetSystemMetrics(SM_CYBORDER),
               MenuGetMaxLen(m)      + 2*IGetSystemMetrics(SM_CXBORDER));
#endif
#endif
}



/****************************************************************************/
/*                                                                          */
/* Function : _MenuGetFirstSelectableItem()                                 */
/*                                                                          */
/* Purpose  : Determines the first selectable menu item.                    */
/*                                                                          */
/* Returns  : The 0-based position of the first item, -1 if none.           */
/*                                                                          */
/****************************************************************************/
static INT PASCAL _MenuGetFirstSelectableItem(pMenu)
  MENU *pMenu;
{
  int  index;
  LIST *p;

  /*
    Find the first menu item which is not a separator.
  */
  for (index = 0, p = pMenu->itemList;  p;  p = p->next, index++)
    if (!(((LPMENUITEM) p->data)->flags & MF_SEPARATOR))
      break;
  if (p == NULL)  /* We went thru the whole loop, only finding disabled ones */
    return -1;
  return index;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsMouseMenuInParent()                                         */
/*                                                                          */
/* Purpose  : Returns TRUE if <row,col> is within a menu's parent's client  */
/*            area or within the menubar.                                   */
/*                                                                          */
/****************************************************************************/
static INT PASCAL IsMouseInMenuParent(wMenu, lParam)
  WINDOW *wMenu;
  DWORD  lParam;  /* screen-based coordinates */
{
  RECT  rClient;
  POINT pt;

  rClient = wMenu->parent->rClient;
  if (wMenu->parent->hMenu == wMenu->win_id)  /* Are we testing a menubar? */
    rClient.top -= IGetSystemMetrics(SM_CYMENU);  /* Include the menubar. */
  pt = MAKEPOINT(lParam);
  return PtInRect(&rClient, pt);
}


/****************************************************************************/
/*                                                                          */
/* Function : GetMenubarFromPopup(wMenu)                                    */
/*                                                                          */
/* Purpose  : Given a pointer to a menu structure, walks up the popup chain */
/*            in search of the menubar. For stand-alone popup menus, this   */
/*            will fail.                                                    */
/*                                                                          */
/* Returns  : The window structure of the menubar, NULL if no menubar.      */
/*                                                                          */
/****************************************************************************/
static WINDOW *PASCAL GetMenubarFromPopup(wMenu)
  WINDOW *wMenu;
{
  while (wMenu && IS_MENU(wMenu))
  {
    MENU *m = (MENU *) wMenu->pPrivate;
    if (IS_MENUBAR(m))
      return wMenu;
    else
      wMenu = wMenu->parent;
  }
  return (WINDOW *) NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsMenu(hMenu)                                                 */
/*                                                                          */
/* Purpose  : Win 3.1 function to determine if the handle points to a menu. */
/*                                                                          */
/* Returns  : TRUE if the window is a menu, FALSE if not.                   */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsMenu(hMenu)
  HMENU hMenu;
{
  WINDOW *w = WID_TO_WIN(hMenu);
  return (BOOL) (w != NULL && IS_MENU(w));
}

