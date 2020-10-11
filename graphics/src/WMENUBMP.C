/*===========================================================================*/
/*                                                                           */
/* File    : WMENUBMP.C                                                      */
/*                                                                           */
/* Purpose : Routines to implement menus bitmaps and ownerdrawn items        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif


/*
  MenuGetItemDimensions()
    Internal function which retrieves the height and width of
    a menu item.
*/
DWORD PASCAL MenuGetItemDimensions(hMenu, lpMI)
  HMENU hMenu;
  LPMENUITEM lpMI;
{
  /*
    If we have an owner-drawn item, then use the WM_MEASUREITEM
    message to get the height and width
  */
  if (lpMI->flags & MF_OWNERDRAW)
  {
    MEASUREITEMSTRUCT mis;
    mis.CtlType = ODT_MENU;
    mis.itemID  = lpMI->id;
    mis.itemWidth = mis.itemHeight = 0;
    SendMessage(GetParent(hMenu), WM_MEASUREITEM, 0, (DWORD) &mis);    
    return MAKELONG(mis.itemWidth, mis.itemHeight);
  }

  /*
    If we have a bitmap, then get the bitmap handle from the
    low word of the text pointer. Then use GetObject to query
    the bitmap structure and return the height and width.
  */
  if (lpMI->flags & MF_BITMAP)
  {
    BITMAP  bm;
    HBITMAP hbm = (HBITMAP) FP_OFF(lpMI->text);
    if (hbm)
    {
      GetObject(hbm, sizeof(bm), (LPSTR) &bm);
      return MAKELONG(bm.bmWidth, bm.bmHeight);
    }
  }

  /*
    A regular menu item. Just return the height.
  */
  return MAKELONG(0, SysGDIInfo.tmHeightAndSpace);
}


BOOL FAR PASCAL SetMenuItemBitmaps(hMenu,idItem,uFlags,hbmUnchecked,hbmChecked)
  HMENU   hMenu;
  INT     idItem;
  UINT    uFlags;
  HBITMAP hbmUnchecked,
          hbmChecked;
{
  MENU       *m;
  LPMENUITEM lpMI;

  /*
    Get a pointer to the menu item structure
  */
  if ((m = _MenuHwndToStruct(hMenu)) == NULL)
    return FALSE;
  if ((lpMI = MenuGetItem(m, idItem, uFlags)) == NULL)
    return FALSE;

  /*
    Assign the bitmaps
  */
  lpMI->hbmChecked   = hbmChecked;
  lpMI->hbmUnchecked = hbmUnchecked;

  /*
    If both bitmaps are NULL, then revert to the system bitmaps
  */
  if (hbmUnchecked == NULL && hbmChecked == NULL)
    lpMI->hbmChecked = lpMI->hbmUnchecked = DEFAULT_MENU_CHECK;

  return TRUE;
}


DWORD FAR PASCAL GetMenuCheckMarkDimensions(void)
{
#if defined(MEWEL_GUI)
  /*
    Get the length and width of the standard checkmark
  */
  BYTE s[2];
  s[0] = (BYTE) WinGetSysChar(SYSCHAR_MENUCHECK);
  s[1] = '\0';
  return _GetTextExtent(s);
#else
  return MAKELONG(1, 1);
#endif
}

