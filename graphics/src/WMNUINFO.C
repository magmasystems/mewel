/*===========================================================================*/
/*                                                                           */
/* File    : WMNUINFO.C                                                      */
/*                                                                           */
/* Purpose : Public functions which query a menu about its state.            */
/*                                                                           */
/* History :                                                                 */
/*    6/01/92 (maa) Separated out from winmenu.c                             */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"


/*===========================================================================*/
/*                                                                           */
/* Function : GetMenu()                                                      */
/*                                                                           */
/* Purpose  : Returns the handle of a window's menubar                       */
/*                                                                           */
/*===========================================================================*/
HMENU FAR PASCAL GetMenu(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? w->hMenu : NULLHWND;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetMenuItemCount()                                            */
/*                                                                          */
/* Purpose  : Determines the number of items in a menu.                     */
/*                                                                          */
/* Returns  : The number of items in the menu.                              */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL GetMenuItemCount(hMenu)
  HWND hMenu;
{
  MENU *m;

  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return -1;
  return m->nItems;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetMenuItemID()                                               */
/*                                                                          */
/* Purpose  : Given a menu and a position, returns the id of the menu item  */
/*            at that position.                                             */
/*                                                                          */
/* Returns  : The menu item id. If the menu item is a separator, returns 0. */
/*            If it's a popup, returns -1.                                  */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL GetMenuItemID(hMenu, pos)
  HMENU hMenu;
  int   pos;
{
  MENU      *m;
  LPMENUITEM mi;
  
  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return 0;
  if ((mi = (LPMENUITEM) MenuGetItem(m, pos, MF_BYPOSITION)) == NULL)
    return 0;

  /*
    Windows returns 0 for a separator and -1 for a popup
  */
  if (mi->flags & MF_SEPARATOR)
    return 0;
  if (mi->flags & MF_POPUP)
    return (UINT) -1;
  return mi->id;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetMenuState()                                                */
/*                                                                          */
/* Purpose  : Returns the state flags of a menu item.                       */
/*                                                                          */
/* Returns  : The state flags, or -1 if the item doesn't exist.             */
/*                                                                          */
/****************************************************************************/
UINT FAR PASCAL GetMenuState(hMenu, id, wFlags)
  HWND hMenu;
  UINT id;
  UINT wFlags;
{
  MENU      *m;
  LPMENUITEM mi;
  
  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return (UINT) -1;
  mi = MenuGetItem(m, id, wFlags);
  return (mi) ? mi->flags : (UINT) -1;
}


/*===========================================================================*/
/*                                                                           */
/* Function : GetMenuString()                                                */
/*                                                                           */
/* Purpose  : Copies the string of a menu item into a user buffer            */
/*                                                                           */
/* Returns  : The length of the string.                                      */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL GetMenuString(hMenu, id, buf, maxbuflen, wFlags)
  HWND hMenu;
  UINT id;
  LPSTR buf;
  int  maxbuflen;
  UINT wFlags;
{
  MENU      *m;
  LPMENUITEM mi;
  
  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return 0;

  if ((mi = MenuGetItem(m, id, wFlags)) != NULL && mi->text)
  {
    lstrncpy(buf, mi->text, maxbuflen-1);
    buf[maxbuflen-1] = '\0';
    return lstrlen(buf);
  }
  else
    return 0;
}


/*===========================================================================*/
/*                                                                           */
/* Function : GetSubMenu()                                                   */
/*                                                                           */
/* Purpose  : Given a position, returns the handle of the submenu at that    */
/*            position.                                                      */
/*                                                                           */
/* Returns  : The handle of the submenu, 0 if the submenu wasn't found.      */
/*                                                                           */
/*===========================================================================*/
HMENU FAR PASCAL GetSubMenu(hMenuBar, pos)
  HWND hMenuBar;
  int  pos;
{
  MENU     *m;
  LPMENUITEM mi;

  if ((m = _MenuHwndToStruct(hMenuBar)) == NULL)
    return NULLHWND;

  if ((mi = (LPMENUITEM) MenuGetItem(m, pos, MF_BYPOSITION)) != NULL &&
      (mi->flags & MF_POPUP))
    return (HWND) mi->id;
    
  return NULLHWND;
}

