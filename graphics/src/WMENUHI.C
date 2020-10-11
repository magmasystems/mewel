/*===========================================================================*/
/*                                                                           */
/* File    : WMENUHI.C                                                       */
/*                                                                           */
/* Purpose : Implements HiliteMenuItem()                                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif
extern LPMENUITEM FAR PASCAL MenuGetItem(LPMENU,int,UINT);
#ifdef __cplusplus
}
#endif


BOOL FAR PASCAL HiliteMenuItem(hWnd, hMenu, wID, wFlags)
  HWND  hWnd;
  HMENU hMenu;
  UINT  wID;
  UINT  wFlags;
{
  LPMENU m;
  LPMENUITEM mi;
  WINDOW *w;

  (void) hWnd;

  if ((w = WID_TO_WIN(hMenu)) == (WINDOW *) NULL)
    return FALSE;

  /*
    Get a pointer to the menu item data structure
  */
  m = (LPMENU) w->pPrivate;
  if ((mi = MenuGetItem(m, wID, wFlags)) == NULL)
    return FALSE;

  /*
    Manipulate the hilite bit
  */
  if (wFlags & MF_HILITE)
    mi->flags |= MF_HILITE;
  else
    mi->flags &= ~MF_HILITE;

  /*
    Refresh the menu
  */
  DrawMenuBar(hMenu);

  return TRUE;
}

