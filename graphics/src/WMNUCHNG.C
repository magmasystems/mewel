/*===========================================================================*/
/*                                                                           */
/* File    : WMNUCHNG.C                                                      */
/*                                                                           */
/* Purpose : Functions to change the state of a menu.                        */
/*           Includes the public functions ChangeMenu(), EnableMenuItem(),   */
/*           CheckMenuItem()                                                 */
/*                                                                           */
/* History :                                                                 */
/*     6/01/92 (maa)  Separated out from winmenu.c                           */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"


#ifdef __cplusplus
extern "C" {
#endif
static INT  PASCAL MenuDeleteItem(MENU *, INT, UINT);
static INT  PASCAL MenuInsertItem(MENU *, LPMENUITEM, UINT, UINT);
static LPSTR PASCAL MenuSaveString(LPCSTR);
extern VOID FAR PASCAL MenuComputeItemCoords(MENU *);
#ifdef __cplusplus
}
#endif

/*
  Mask used to AND-out undesireable bits in a menu item's 'flags' field
*/
#define MF_ATTRMASK \
(~(MF_APPEND|MF_CHANGE|MF_DELETE|MF_INSERT|MF_REMOVE|MF_BYPOSITION|MF_BYCOMMAND))

/*
  This variable is needed in order to tell ChangeMenu that we are dealing
  with a MF_OWNERDRAW menu item. This is because the value of MF_OWNERDRAW
  (which is 0x100) is the same value as MF_APPEND.
*/
static BOOL bChangingOwnerDrawnItem = FALSE;

BOOL bDeferMenuRecalc = FALSE;


/****************************************************************************/
/*                                                                          */
/* Function : AppendMenu()                                                  */
/*            DeleteMenu()                                                  */
/*            InsertMenu()                                                  */
/*            ModifyMenu()                                                  */
/*            RemoveMenu()                                                  */
/*                                                                          */
/* Purpose  : Windows 3.x high-level menu handling functions                */
/*                                                                          */
/* Returns  : The result from ChangeMenu()                                  */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DeleteMenu(hMenu, idChgItem, flags)
  HWND hMenu;
  UINT idChgItem;
  UINT flags;
{
  return ChangeMenu(hMenu, idChgItem, NULL, 0, flags | MF_DELETE);
}

BOOL FAR PASCAL RemoveMenu(hMenu, idChgItem, flags)
  HWND hMenu;
  UINT idChgItem;
  UINT flags;
{
  return ChangeMenu(hMenu, idChgItem, NULL, 0, flags | MF_REMOVE);
}

BOOL FAR PASCAL AppendMenu(hMenu, flags, idNewItem, newString)
  HWND   hMenu;
  UINT   idNewItem;
  LPCSTR newString;
  UINT   flags;
{
  BOOL   rc;

  if (flags & MF_OWNERDRAW)
    bChangingOwnerDrawnItem = TRUE;

  rc = ChangeMenu(hMenu, 0, newString, idNewItem, flags | MF_APPEND);

  bChangingOwnerDrawnItem = FALSE;
  return rc;
}

BOOL FAR PASCAL InsertMenu(hMenu, idChgItem, flags, idNewItem, newString)
  HWND   hMenu;
  UINT   idChgItem, idNewItem;
  LPCSTR newString;
  UINT   flags;
{
  BOOL   rc;

  if (flags & MF_OWNERDRAW)
    bChangingOwnerDrawnItem = TRUE;

  rc = ChangeMenu(hMenu, idChgItem, newString, idNewItem, flags | MF_INSERT);

  bChangingOwnerDrawnItem = FALSE;
  return rc;
}

BOOL FAR PASCAL ModifyMenu(hMenu, idChgItem, flags, idNewItem, newString)
  HWND   hMenu;
  UINT   idChgItem, idNewItem;
  LPCSTR newString;
  UINT   flags;
{
  BOOL   rc;

  if (flags & MF_OWNERDRAW)
    bChangingOwnerDrawnItem = TRUE;

  rc = ChangeMenu(hMenu, idChgItem, newString, idNewItem, flags | MF_CHANGE);

  bChangingOwnerDrawnItem = FALSE;
  return rc;
}


/****************************************************************************/
/*                                                                          */
/* Function : ChangeMenu()                                                  */
/*                                                                          */
/* Purpose  : Catch-all routine to append, insert, delete, and change       */
/*            menu items.                                                   */
/*                                                                          */
/* Returns  : TRUE if the item was changed successfully, FALSE if not.      */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL ChangeMenu(hMenu, idChgItem, newString, idNewItem, flags)
  HWND   hMenu;
  UINT   idChgItem, idNewItem;
  LPCSTR newString;
  UINT   flags;
{
  WINDOW *wMenu;
  MENU   *m;
  LPMENUITEM ti;
  LPSTR   s;
  UINT    oldFlags = 0;

  /*
    Get a pointer to the menu's window and to the menu structure itself.
  */
  if ((wMenu = WID_TO_WIN(hMenu)) == NULL)
    return FALSE;
  m = (MENU *) wMenu->pPrivate;

  /*
    MS Windows compatiblility - change '&' to '~'
  */
  if (newString)
#if defined(USE_BITMAPS_IN_MENUS)
    if (!(flags & MF_BITMAP) && !bChangingOwnerDrawnItem)
#endif
    _TranslatePrefix((LPSTR) newString);

  /*
    If we passed in the MF_POPUP flag, it means that we are adding a
    popup menu to either a menubar item or a popup
  */
  if (flags & MF_POPUP)
  {
    HWND   hPopup = (HWND) idNewItem;
    WINDOW *popw  = WID_TO_WIN(hPopup);
    MENU   *popm;
    BOOL   bMakingPopupOuttaMyself;

    if (popw == NULL)
      return FALSE;

    /*
      See if we are taking a menu template and transforming it into
      a popup. We usually do this when we create a floating menu.
    */
    bMakingPopupOuttaMyself = (BOOL) (hPopup == hMenu);

    /*
      Convert the menubar to a popup
    */
    popm  = (MENU *) popw->pPrivate;
    popm->iLevel = m->iLevel+1;

    /*
      The popup has a border & is hidden, and is never clipped.
    */
    popw->flags |= WS_BORDER;
    SET_WS_HIDDEN(popw);
    popw->flags &= ~(WS_VISIBLE | WS_CLIP);


    if (IS_MENUBAR(m))
    {
      /*
        We are making a popup out of a menu which we formerly considered
        to be a menubar (ie - something created by CreateMenu)
      */
      popm->flags &= ~M_MENUBAR;
      popm->flags |= M_POPUP;
      if (flags & MF_SYSMENU)
        popm->flags |= M_SYSMENU;
      if (!bMakingPopupOuttaMyself)
        popw->parent = wMenu;
      else
        MenuAdjustPopupSize(popm);
      if (!bDeferMenuRecalc)
        _MenuFixPopups(NULL, hMenu);
    }

    else  /* adding a nth-level submenu to a popup menu */
    {
      popm->flags &= ~M_MENUBAR;
      popw->parent = wMenu;
      if (!bDeferMenuRecalc)
        _MenuFixPopups(NULL, hPopup);
    }
  } /* if (flags & MF_POPUP) */


  /*
    See if we are changing, appending, or inserting
  */
  if (!(flags & (MF_DELETE | MF_REMOVE)))
  {
    /*
      Get a pointer to the menuitem structure (or create a new one).
    */
    if (!(flags & MF_CHANGE))  /* appending or inserting... */
    {
      if ((ti = (LPMENUITEM) EMALLOC_FAR(sizeof(MENUITEM))) == NULL)
        return FALSE;
      /*
         If the item we just added was a popup, then the id field
         will be the handle of the popup menu.
      */
      ti->flags = (flags & MF_POPUP) ? MF_POPUP : 0x0000;
    }
    else
    {
      if ((ti = MenuGetItem(m, idChgItem, flags)) == NULL)
        return FALSE;
      oldFlags = ti->flags;
    }

    /*
      Assign the proper attribute values to the item.
    */
    if (!(flags & MF_CHECKED))
      ti->flags &= ~MF_CHECKED;
    if (!(flags & MF_DISABLED))
      ti->flags &= ~MF_DISABLED;
    if (!(flags & MF_GRAYED))
      ti->flags &= ~MF_GRAYED;
#if defined(USE_BITMAPS_IN_MENUS)
    if (!bChangingOwnerDrawnItem)
      ti->flags &= ~MF_OWNERDRAW;
    if (!(flags & MF_BITMAP))
      ti->flags &= ~MF_BITMAP;
#endif
    if (flags & MF_SHADOW)
      wMenu->flags |= (WS_SHADOW | SHADOW_BOTRIGHT);

    ti->flags |= (flags & MF_ATTRMASK);
    if (bChangingOwnerDrawnItem)
      ti->flags |= MF_OWNERDRAW;
    ti->id = idNewItem;

    /*
      Now that we have all of the item flags figured out, get the 
      height of the item.
    */
#if defined(USE_BITMAPS_IN_MENUS)
    ti->cyItem = HIWORD(MenuGetItemDimensions(hMenu, ti));
#else
    ti->cyItem = SysGDIInfo.tmHeightAndSpace;
#endif
    /*
       A min value of 16 will space out the menu items nicely
    */
#if defined(MEWEL_GUI)
    if (ti->cyItem < 16)
      ti->cyItem = 16;
#endif


#if defined(USE_BITMAPS_IN_MENUS)
    if (!(flags & MF_CHANGE) || newString != NULL ||
                                (ti->flags & (MF_BITMAP | MF_OWNERDRAW)))
#else
    if (!(flags & MF_CHANGE) || newString != NULL)
#endif
    {
      if (!(flags & MF_CHANGE))              /* Don't do this if we */
      {                                      /*  are just changing. */
        ti->id = idNewItem;
#if defined(MOTIF) || defined(DECWINDOWS)
        if (wMenu->parent && wMenu->parent->hSysMenu == hMenu)
        {
          ;
        }
        else
        {
          if (ti->widget)
          {
            XtVaSetValues(ti->widget, XmNuserData, (caddr_t) idNewItem, NULL);
            if (ti->flags & MF_CHECKED)
              XtVaSetValues(ti->widget, XmNindicatorOn, TRUE, NULL);
            if (ti->flags & (MF_DISABLED | MF_GRAYED))
              XtVaSetValues(ti->widget, XmNsensitive, FALSE, NULL);
          }
        }
#endif
      }

      /*
        Take care of the menu item text
      */
#if defined(USE_BITMAPS_IN_MENUS)
      if (newString || (ti->flags & (MF_BITMAP | MF_OWNERDRAW)))
#else
      if (newString)
#endif
      {
        LPSTR pSlash;

        /*
          See if we have a right-justified menu item. We do if we have
          a string like "\aHelp"
        */
#if defined(USE_BITMAPS_IN_MENUS)
        if (!(ti->flags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
        if (newString[0] == '\\')    /* take care of '\a' */
          if (newString[1] == 'a')
          {
            newString += 2;
            ti->flags |= MF_RIGHTJUST;
          }

        /*
          Check for \a or a 8 in the middle of a menu item
        */
#if defined(USE_BITMAPS_IN_MENUS)
        if (!(ti->flags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
        if ((pSlash = lstrchr(newString, '\\')) != NULL && pSlash[1] == 'a' ||
            (pSlash = lstrchr(newString, '\b')) != NULL)
          ti->iRightJustPos = pSlash - (LPSTR) newString;


        /*
          Delete the old menu string and save the new one.
        */
        if (ti->text)
#if defined(USE_BITMAPS_IN_MENUS)
          if (!(oldFlags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
          MYFREE_FAR(ti->text);

#if defined(USE_BITMAPS_IN_MENUS)
        if (ti->flags & (MF_BITMAP | MF_OWNERDRAW))
        {
          ti->text   = (LPSTR) newString;
          ti->letter = '\0';
          ti->cyItem = HIWORD(MenuGetItemDimensions(hMenu, ti));
          if (ti->flags & MF_BITMAP)
            ti->hbmChecked = DEFAULT_MENU_CHECK;
        }
        else
        {
          ti->text = MenuSaveString(newString);
          /*
            Determine the 'hot-key' for this item. It's the character
            right after the hotkey-prefix, or the first letter in
            the menu.
          */
          if ((s = lstrchr(newString, HILITE_PREFIX)) != NULL && *++s)
            ti->letter = *s;
          else
            ti->letter = *newString;
        }
#else
        /*
          Kludge for MF_BITMAP
        */
        if (ti->flags & MF_BITMAP)
          newString = "<Bitmap>";
        ti->text = MenuSaveString(newString);

        /*
          Determine the 'hot-key' for this item. It's the character
          right after the hotkey-prefix, or the first letter in
          the menu.
        */
        if ((s = lstrchr((LPSTR) newString, HILITE_PREFIX)) != NULL && *++s)
          ti->letter = *s;
        else
          ti->letter = *newString;
#endif


#if defined(MOTIF) || defined(DECWINDOWS)
        if (ti->widget && (flags & MF_CHANGE))
        {
          BYTE   szBuf[80];
          LPSTR  s;
          LPCSTR t;
          for (s = szBuf, t = newString;  *t;  t++)
            if (*t != HILITE_PREFIX)
              *s++ = *t;
          *s = '\0';

          XtVaSetValues(ti->widget, 
                        XmNlabelString, XmStringCreateLtoR(szBuf, XmSTRING_DEFAULT_CHARSET),
                        XmNmnemonic, ti->letter,
                        NULL);

        }
#endif
      }
      else /* newString = NULL */
      {
        ti->flags |= MF_SEPARATOR;
        if (ti->text)
#if defined(USE_BITMAPS_IN_MENUS)
          if (!(oldFlags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
          MYFREE_FAR(ti->text);
        ti->text = NULL;
      }


      /*
        Insert the new item in the item-list
      */
      if (!(flags & MF_CHANGE))
        MenuInsertItem(m, ti, flags, idChgItem);

      /*
        Possibly readjust width of the popup and adjust the origin of
        the popup.
      */
      if (!IS_MENUBAR(m))
      {
        if (!bDeferMenuRecalc)
          MenuAdjustPopupSize(m);
      }
#if defined(MEWEL_GUI)
      /*
        If we are changing or inserting an item in a menubar, we must
        compute the rectItem structures of the menubar items again.
      */
      else
      {
        if (!bDeferMenuRecalc)
          MenuComputeItemCoords(m);
        InvalidateNCArea(GetParentOrDT(m->hWnd));
      }
#endif
      if (!bDeferMenuRecalc)
        _MenuFixPopups(NULL, hMenu);
    }

  } /* if (!(flags & (MF_DELETE | MF_REMOVE))) */

  else if (flags & (MF_DELETE | MF_REMOVE))
  {
    return MenuDeleteItem(m, idChgItem, flags);
  }


#if defined(MOTIF) || defined(DECWINDOWS)
  if (wMenu->parent && wMenu->parent->hSysMenu == hMenu)
  {
    XRefreshSysMenu(hMenu);
  }
#endif

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : CheckMenuItem()                                               */
/*                                                                          */
/* Purpose  : Checks or unchecks a specific menu item.                      */
/*                                                                          */
/* Returns  : The old check state for that item.                            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL CheckMenuItem(hMenu, idCheckItem, wCheck)
  HWND hMenu;
  UINT idCheckItem;
  UINT wCheck;
{
  MENU       *pMenu;
  LPMENUITEM lpMI;
  UINT       fOldState;

  pMenu = _MenuHwndToStruct(hMenu);
  if (pMenu != NULL && 
      (lpMI = MenuGetItem(pMenu, idCheckItem, wCheck)) != NULL)
  {
    fOldState   = (lpMI->flags & MF_CHECKED);
    lpMI->flags &= ~MF_CHECKED;
    lpMI->flags |= (wCheck & MF_CHECKED);
#if defined(MOTIF)
    if (lpMI->widget != (Widget) 0)
    {
      XtVaSetValues(lpMI->widget,
                    XmNset,         (wCheck & MF_CHECKED) ? TRUE : FALSE,
                    XmNindicatorOn, (wCheck & MF_CHECKED) ? TRUE : FALSE,
                    NULL);
    }
#endif    
    return fOldState;
  }
  else
    return (BOOL) -1;
}


/****************************************************************************/
/*                                                                          */
/* Function : EnableMenuItem().                                             */
/*                                                                          */
/* Purpose  : Enables or disabled a menu item.                              */
/*                                                                          */
/* Returns  : The old state of the menu item.                               */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL EnableMenuItem(hMenu, idEnableItem, wEnable)
  HWND hMenu;
  UINT idEnableItem;
  UINT wEnable;
{
  MENU       *pMenu;
  LPMENUITEM lpMI;
  UINT       fOldState;

  pMenu = _MenuHwndToStruct(hMenu);
  if (pMenu != NULL && 
      (lpMI = MenuGetItem(pMenu, idEnableItem, wEnable)) != NULL)
  {
    fOldState   = (lpMI->flags & (MF_DISABLED | MF_GRAYED));
    lpMI->flags &= ~(MF_DISABLED | MF_GRAYED);
    lpMI->flags |= (wEnable & (MF_DISABLED | MF_GRAYED));
#if defined(MOTIF)
    if (lpMI->widget != (Widget) 0)
      XtSetSensitive(lpMI->widget,
                     (wEnable & (MF_DISABLED|MF_GRAYED)) == 0);
#endif    
    return fOldState;
  }
  else
    return (BOOL) -1;
}


/*===========================================================================*/
/*                                                                           */
/* Function : MenuInsertItem()                                               */
/*                                                                           */
/* Purpose  : Inserts menuitem mi into menu m at index pos                   */
/*                                                                           */
/* Returns  : TRUE if successful, LB_ERRSPACE if not.                        */
/*                                                                           */
/*===========================================================================*/
static INT PASCAL MenuInsertItem(m, mitem, fFlags, idChgItem)
  MENU     *m;
  LPMENUITEM mitem;
  UINT     fFlags;
  UINT     idChgItem;  /* position or command id */
{
  LIST       *p, *pNth;
  LPMENUITEM mi;
  
  /*
    If we are appending by position, then force the position to be -1.
  */
  if (fFlags & (MF_APPEND | MF_RIGHTJUST))
  {
    idChgItem = (UINT) -1; 
    fFlags |= MF_BYPOSITION;
  }

  if (!(fFlags & MF_BYPOSITION))    /* it's MF_BYCOMMAND */
  {
    for (pNth = m->itemList;  pNth;  pNth = pNth->next)
    {
      mi = (LPMENUITEM) pNth->data;
      if ((mi->flags & MF_POPUP))    /* recurse on the sub-menu... */
      {
        WINDOW *popw;
        if ((popw = WID_TO_WIN(mi->id)) != NULL &&
             MenuInsertItem((MENU*)popw->pPrivate,mitem,fFlags,idChgItem)==TRUE)
          return TRUE;
      }
      else if ((UINT) mi->id == idChgItem)
      {
        if ((p = ListCreate((LPSTR) mitem)) == NULL)
          return LB_ERRSPACE;
        if (fFlags & (MF_APPEND | MF_RIGHTJUST))
        {
          if (pNth->next && !(fFlags & MF_RIGHTJUST))
            /*
              Append after this item
            */
            ListInsert(&m->itemList, p, pNth->next);
          else
            /*
              Append to the end of the menu
            */
            ListAdd(&m->itemList, p);
        }
        else
        {
          ListInsert(&m->itemList, p, pNth);
        }
        m->nItems++;
        return TRUE;
      }
    }
    /*
      Couldn't find the item ?
    */
    if (!pNth)
      return FALSE;
  }

  else /* MF_BYPOSITION */
  {
    if ((p = ListCreate((LPSTR) mitem)) == NULL)
      return LB_ERRSPACE;
    if ((int) idChgItem < 0 || (pNth = ListGetNth(m->itemList, idChgItem)) == NULL)
      ListAdd(&m->itemList, p);
    else
      ListInsert(&m->itemList, p, pNth);
    m->nItems++;
  }

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : MenuDeleteItem()                                              */
/*                                                                          */
/* Purpose  : Deletes a menu item from a menu based on the wFlags           */
/*                                                                          */
/* Returns  : TRUE if the item was deleted, FALSE if not.                   */
/*                                                                          */
/****************************************************************************/
static INT PASCAL MenuDeleteItem(m, pos, wFlags)
  MENU *m;
  INT  pos;
  UINT wFlags;
{
  LIST      *pNth;
  LPMENUITEM mi;
  UINT       miFlags;
  HWND       hPopup;
  
  if (!(wFlags & MF_BYPOSITION))    /* it's MF_BYCOMMAND */
  {
    for (pNth = m->itemList;  pNth;  pNth = pNth->next)
    {
      mi = (LPMENUITEM ) pNth->data;
      if ((mi->flags & MF_POPUP))    /* recurse on the sub-menu... */
      {
        WINDOW *popw;
        if ((popw = WID_TO_WIN(mi->id)) != NULL &&
             MenuDeleteItem((MENU *) popw->pPrivate, pos, wFlags) == TRUE)
          return TRUE;
      }
      else if (mi->id == pos)
        break;
    }
    /*
      Couldn't find the item ?
    */
    if (!pNth)
      return FALSE;
  }
  else /* MF_BYPOSITION */
  {
    if ((pNth = ListGetNth(m->itemList, pos)) == NULL)
      return FALSE;
    mi = (LPMENUITEM ) pNth->data;
  }

  /*
    Save the current flag setiings
  */
  miFlags = mi->flags;
  hPopup  = mi->id;

  /*
    Get rid of the menu string
  */
  if (mi->text != NULL)
#if defined(USE_BITMAPS_IN_MENUS)
    if (!(miFlags & (MF_BITMAP | MF_OWNERDRAW)))
#endif
    MYFREE_FAR(mi->text);

  /*
    One less item in the menu
  */
  m->nItems--;

  /*
    Delete the item from the list of items
  */
  ListDelete(&m->itemList, pNth);

  /*
    If we are destroying an entire popup menu, use DestroyMenu().
  */
  if (IS_MENUBAR(m))
  {
    if ((miFlags & MF_POPUP) && (wFlags & MF_DELETE))
      DestroyMenu(hPopup);
#if defined(MEWEL_GUI)
    MenuComputeItemCoords(m);
#endif
    _MenuFixPopups(NULL, m->hWnd);
  }
  else
  {
    MenuAdjustPopupSize(m);
  }
  return TRUE;
}


static LPSTR PASCAL MenuSaveString(lpStr)
  LPCSTR lpStr;
{
  LPSTR  lpNewStr;
  LPCSTR lpTab;
  int    iLen;
  int    iTabPos = 0;

  /*
    Figure out the initial # of chars to allocate (including the '\0')
  */
  iLen = lstrlen(lpStr) + 1;

  /*
    If we have a '\b' (ASCII 8) embedded in the middle of the string, then
    change it to '\a'. The Windows RC puts it in there.
  */
  if ((lpTab = lstrchr(lpStr, '\b')) != NULL)
  {
    iLen++;  /* increase the length by 1 for the addition of the 'a' */
    iTabPos = lpTab - lpStr;
  }

  /*
    Allocate memory for the string and copy it
  */
  if ((lpNewStr = (LPSTR) EMALLOC_FAR_NOQUIT(iLen)) != NULL)
  {
    if (iTabPos)
    {
      lstrncpy(lpNewStr, lpStr, iTabPos);
      lpNewStr[iTabPos]   = '\\';
      lpNewStr[iTabPos+1] = 'a';
      lstrcpy(lpNewStr + iTabPos + 2, lpStr + iTabPos + 1);
    }
    else
      lstrcpy(lpNewStr, lpStr);
  }

  return lpNewStr;
}


