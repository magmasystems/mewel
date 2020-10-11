/*===========================================================================*/
/*                                                                           */
/* File    : WGUIMENU.C                                                      */
/*                                                                           */
/* Purpose : Routines to implement the menubar and pulldown menu system      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#define GetSysMetrics(n)  (n)


extern VOID FAR PASCAL MenuComputeItemCoords(MENU *);

static VOID PASCAL MenuInvertItem(HDC, MENU *, LPMENUITEM);
static VOID PASCAL DrawMenuItem(LPMENUITEM,HDC,INT,INT,WINDOW*,BOOL,BOOL,INT);

#if defined (USE_BITMAPS_IN_MENUS)
static DWORD PASCAL GetMenuItemCheckMarkDimensions(LPMENUITEM);
#endif

/*
  Define USE_TABS if we want to try the tabbed-text method
*/
#define USE_TABS

/*
  This is the character which is used for submenus
*/
static char szSubMenu[2] = { '>', '\0' };


INT FAR PASCAL MenuRefresh(wMenu)
  WINDOW  *wMenu;
{
  RECT       r;
  HDC        hDC;
  LOGBRUSH   lb;
  MENU       *m;
  HWND       hMenu = wMenu->win_id;
  LIST       *p;
  INT        pos;
  INT        pixWidth = WIN_CLIENT_WIDTH(wMenu);
  LPMENUITEM mi;
  BOOL       bIsPopup;
  INT        cyMenuOffset;
  INT        yItem;

#if defined(USE_BITMAPS_IN_MENUS)
  BOOL       bMaxMDIMenu = (BOOL) ((wMenu->ulStyle & MFS_RESTOREICON) != 0);
#endif


  HilitePrefix = HILITE_PREFIX;

  /*
    Get a pointer to the menu structure
  */
  m = (MENU *) wMenu->pPrivate;
  bIsPopup = (BOOL) (!IS_MENUBAR(m));

  /*
    This is used to center the text vertically within a menu item's rectangle.
  */
  if (bIsPopup)
  {
#if defined(GX) || defined(BGI) || defined(MSC)
    cyMenuOffset = SysGDIInfo.tmExternalLeading >> 1;
#else
    cyMenuOffset = 2;
#endif
  }
  else
  {
    /*
      The number of pixels which the menubar string should be offset by
      is the writable space in the menubar (the size of the menu bar minus
      the top and bottom border width) divided by 2.
    */
    cyMenuOffset = (IGetSystemMetrics(SM_CYMENU) - 2 -
                                          SysGDIInfo.tmHeight) >> 1;
    if (cyMenuOffset < 0)
      cyMenuOffset = 0;
  }


  /*
    Get a DC for the menu.
  */
  MouseHide();
  hDC = GetWindowDC(hMenu);
  SetBkMode(hDC, TRANSPARENT);


  /*
    Incremental redrawing of a popup....
      Unhighlight the old selected item and highlight the new one.
  */
  if (bIsPopup && MenuInvertInfo.bInverting &&
      MenuInvertInfo.iOldSel != m->iCurrSel && MenuInvertInfo.hMenu == hMenu)
  {
    for (pos = 0;  pos <= 1;  pos++)
    {
      int  iSel = (pos == 0) ? MenuInvertInfo.iOldSel : m->iCurrSel;
      int  iSysBmp;

      if (iSel != -1)
      {
        mi = (LPMENUITEM) ListGetNth(m->itemList, iSel)->data;
        if (pos == 0)
          FillRect(hDC, &mi->rectItem, SysBrush[COLOR_MENU]);

        iSysBmp = 0;
#if defined(USE_BITMAPS_IN_MENUS)
        if (bMaxMDIMenu)
        {
          if (iSel == 0) 
            iSysBmp = SYSBMPT_MDISYSMENU;
          else if (iSel == m->nItems-1)
            iSysBmp = SYSBMPT_RESTORE;
        }
#endif

        DrawMenuItem(mi, hDC, mi->rectItem.top + cyMenuOffset, pixWidth,
                     wMenu, bIsPopup, (BOOL) (pos == 1),
                     iSysBmp);
      }
    }
    goto bye;
  }

  /*
    Set the text color and make it transparent.
    Set the brush we are going to erase with.
  */
  SelectObject(hDC, SysBrush[COLOR_MENU]);
  SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
  GetObject(SysBrush[COLOR_MENU], sizeof(lb), &lb);
  SetBkColor(hDC, lb.lbColor);

  MenuComputeItemCoords(m);

  /*
    Draw the enclosing rectangle and fill it.
  */
  WindowRectToPixels(hMenu, &r);
  if (bIsPopup)
  {
    Rectangle(hDC, 0, 0, r.right, r.bottom);
  }
  else  /* menubar */
  {
    FillRect(hDC, &r, SysBrush[COLOR_MENU]);
    MoveTo(hDC, 0, r.bottom-1);
    LineTo(hDC, r.right, r.bottom-1);
  }


  /*
    Draw each menu item
  */
  for (pos = 0, p = m->itemList;  p;  p = p->next, pos++)
  {
    int iSysBmp = 0;

    mi = (LPMENUITEM) p->data;
    yItem = mi->rectItem.top + cyMenuOffset;

#if defined(USE_BITMAPS_IN_MENUS)
    if (bMaxMDIMenu)
    {
      if (pos == 0) 
        iSysBmp = SYSBMPT_MDISYSMENU;
      else if (pos == m->nItems-1)
        iSysBmp = SYSBMPT_RESTORE;
    }
#endif

    DrawMenuItem(mi, hDC, yItem, pixWidth, wMenu, bIsPopup, 
                    (BOOL) (pos == m->iCurrSel || (mi->flags & MF_HILITE)),
                    iSysBmp);
  } /* for p */


bye:
  MouseShow();
  ReleaseDC(hMenu, hDC);
  HilitePrefix = '\0';
  return TRUE;
}


static VOID PASCAL DrawMenuItem(mi, hDC, yItem, pixWidth, wMenu, bIsPopup,
                                bHighlighted, iMaxMDIMenu)
  LPMENUITEM mi;
  HDC        hDC;
  INT        yItem;
  INT        pixWidth;
  WINDOW    *wMenu;
  BOOL       bIsPopup;
  BOOL       bHighlighted;
  INT        iMaxMDIMenu;
{
  LPSTR      pSlash;
  INT        iRightPos, iRightWidth;
  BOOL       bOwnerDrawn, bOwnerDrawnOrBitmap;
  UINT       fFlags;

  fFlags = mi->flags;

  /*
    Draw the separator line
  */
  if (fFlags & MF_SEPARATOR)
  {
    /*
      Center the separator within the item rectangle.
    */
    yItem = mi->rectItem.top + (RECT_HEIGHT(mi->rectItem) / 2);
    MoveTo(hDC, IGetSystemMetrics(SM_CXBORDER), yItem);
    LineTo(hDC, pixWidth, yItem);
    return;
  }


  /*
    See if we have an owner-drawn or bitmap menu item
  */
  bOwnerDrawn = (fFlags & MF_OWNERDRAW);
  bOwnerDrawnOrBitmap = bOwnerDrawn || (fFlags & MF_BITMAP);


  /*
    Figure out the attribute to draw the item with.
  */
  if (bHighlighted && !iMaxMDIMenu)
  {
    RECT r;
    r = mi->rectItem;
    if (!bIsPopup)
      InflateRect(&r, SysGDIInfo.tmAveCharWidth, 0);
    if (!bOwnerDrawn)
      FillRect(hDC, &r, SysBrush[COLOR_HIGHLIGHT]);
    SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
  }
  else
    SetTextColor(hDC, GetSysColor((fFlags & (MF_DISABLED | MF_GRAYED))
                                    ? COLOR_GRAYTEXT : COLOR_MENUTEXT));

  /*
    Do some work to right-justify the menu item which has a '\a' in it.
  */
  iRightPos = 0;
  if (bIsPopup && mi->iRightJustPos != 0 && !bOwnerDrawnOrBitmap)
  {
    pSlash = mi->text + mi->iRightJustPos;
    iRightWidth = LOWORD(_GetTextExtent(pSlash+2));
    iRightPos = pixWidth - iRightWidth - SysGDIInfo.tmAveCharWidth;
    if (iRightPos > 0)
      *pSlash = '\0';
  }

  /*
    Output the popup menuitem's text
  */
#if defined(USE_BITMAPS_IN_MENUS)
  if (bOwnerDrawn)
  {
    DRAWITEMSTRUCT dis;
    dis.CtlType    = ODT_MENU;
    dis.itemID     = mi->id;
    dis.hwndItem   = wMenu->win_id;
    dis.hDC        = hDC;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState  = 0;
    dis.itemData   = (DWORD) mi->text;
    if (fFlags & MF_CHECKED)
      dis.itemState |= ODS_CHECKED;
    if (fFlags & MF_GRAYED)
      dis.itemState |= ODS_GRAYED;
    if (fFlags & MF_DISABLED)
      dis.itemState |= ODS_DISABLED;
    if (bHighlighted)
      dis.itemState |= ODS_SELECTED;
    dis.rcItem     = mi->rectItem;
    SendMessage(GetParent(wMenu->win_id), WM_DRAWITEM, 0, (DWORD) &dis);
  }
  else if (bOwnerDrawnOrBitmap)
  {
    HBITMAP hbm = (HBITMAP) FP_OFF(mi->text);
    if (hbm)
      DrawBitmapToDC(hDC, 
                     mi->rectItem.left + 
                        (bIsPopup ? SysGDIInfo.tmAveCharWidth : 0), 
                     yItem, hbm, SRCCOPY);
  }
  else if (iMaxMDIMenu)
  {
    /*
      Draw the restore or system menu on the maximized MDI child's menubar.
      We tinker with the menu's WS_MAXIMIZE bit so that DrawSysTransparent
      will not offset the bitmap by the size of a frame.
    */
    wMenu->flags |= WS_MAXIMIZE;
    DrawSysTransparentBitmap(wMenu->win_id, hDC, iMaxMDIMenu);
    wMenu->flags &= ~WS_MAXIMIZE;
  }
  else
#endif

  {
#ifdef USE_TABS
  TabbedTextOut(hDC,
          mi->rectItem.left + (bIsPopup ? SysGDIInfo.tmAveCharWidth : 0), 
          yItem, mi->text, lstrlen(mi->text),
          0, NULL, 0);
#else
  TextOut(hDC,
          mi->rectItem.left + (bIsPopup ? SysGDIInfo.tmAveCharWidth : 0), 
          yItem, mi->text, lstrlen(mi->text));
#endif
  }

  /*
    Restore the '\a'
  */
  if (iRightPos > 0)
  {
    *pSlash = '\\';
    TextOut(hDC,
            mi->rectItem.right-SysGDIInfo.tmAveCharWidth-iRightWidth,
            yItem, pSlash+2, strlen(pSlash+2));
  }

  /*
    Draw the separator line for a menubar help item
  */
  if (!bIsPopup && (fFlags & (MF_HELP | MF_RIGHTJUST)))
  {
#if defined(USE_BITMAPS_IN_MENUS)
    if (iMaxMDIMenu == 0)
#endif
    {
    INT x = mi->rectItem.left - 4;
    MoveTo(hDC, x, 0);
    LineTo(hDC, x, IGetSystemMetrics(SM_CYMENU)-2);
    }
  }

  /*
    If the menu is a popup and it has a child, draw a pointer char...
  */ 
  if (bIsPopup && (fFlags & MF_POPUP) && (int)mi->id != (int)wMenu->win_id)
    TextOut(hDC, pixWidth - (int) LOWORD(_GetTextExtent(szSubMenu)), 
            yItem, (LPSTR) szSubMenu, 1);


  /*
    Display the checkmark in a checked popup menu item
  */
  if (!bOwnerDrawn)
  {
    if (fFlags & MF_CHECKED)
    {
#if defined(USE_BITMAPS_IN_MENUS)
      /*
        Draw the checked bitmap
      */
      if (mi->hbmChecked && mi->hbmChecked != DEFAULT_MENU_CHECK)
      {
        DrawBitmapToDC(hDC, IGetSystemMetrics(SM_CXBORDER), yItem, 
                       mi->hbmChecked, SRCCOPY);
      }
      else if (mi->hbmChecked == DEFAULT_MENU_CHECK)
#endif
      {
      BYTE s[2];
      s[0] = (BYTE) WinGetSysChar(SYSCHAR_MENUCHECK);
      s[1] = '\0';
      TextOut(hDC, IGetSystemMetrics(SM_CXBORDER), yItem, (LPSTR) &s, 1);
      }
    }
#if defined(USE_BITMAPS_IN_MENUS)
    /*
      Draw the "unchecked" bitmap
    */
    else if (mi->hbmUnchecked && mi->hbmUnchecked != DEFAULT_MENU_CHECK)
    {
      DrawBitmapToDC(hDC, IGetSystemMetrics(SM_CXBORDER), yItem, 
                     mi->hbmUnchecked, SRCCOPY);
    }
#endif
  }
}


INT FAR PASCAL MenuRowToItem(m, row)
  MENU *m;
  int  row;  /* client-based y coord of menu where mouse was clicked */
{
  LIST       *p;
  LPMENUITEM mi;
  int        index;

  for (index = 0, p = m->itemList;  p;  p = p->next, index++)
  {
    mi = (LPMENUITEM) p->data;
    if (row >= mi->rectItem.top && row < mi->rectItem.bottom)
      return index;
  }
  return -1;
}

INT FAR PASCAL MenuBarColToItem(m, col)
  MENU *m;
  int  col;
{
  LIST       *p;
  LPMENUITEM mi;
  int        index;

  for (index = 0, p = m->itemList;  p;  p = p->next, index++)
  {
    mi = (LPMENUITEM) p->data;
    if (col >= mi->rectItem.left && col < mi->rectItem.right)
      return (mi->flags & (MF_DISABLED | MF_GRAYED)) ? -1 : index;
  }
  return -1;
}

int FAR PASCAL MenuBarItemToCol(m, index)
  MENU *m;
  int  index;
{
  LIST      *pList;
  LPMENUITEM mi;

  if ((pList = ListGetNth(m->itemList, index)) == NULL)
    return 0;
  mi = (LPMENUITEM) pList->data;
  return mi->rectItem.left;
}


VOID FAR PASCAL MenuComputeItemCoords(m)
  MENU *m;
{
  RECT       rect;
  LIST       *p;
  LPMENUITEM mi;
  INT        sLen;
  BYTE       chSavePrefix;

  /*
    Set the HilitePrefix to '~' so that _GetTextExtent() can treat the
    tilde as a 0-width character.
  */
  chSavePrefix = HilitePrefix;
  HilitePrefix = HILITE_PREFIX;

  if (IS_MENUBAR(m))
  {
    BOOL bMaxMDIMenu = (BOOL) ((WID_TO_WIN(m->hWnd)->ulStyle & MFS_RESTOREICON) != 0);
    INT  iItem;

    SetRect(&rect,
            IGetSystemMetrics(SM_CXBORDER) + 
#if defined(USE_BITMAPS_IN_MENUS)
            (bMaxMDIMenu ? 0 : SM_CXBEFOREFIRSTMENUITEM),
#else
            SM_CXBEFOREFIRSTMENUITEM,
#endif
            IGetSystemMetrics(SM_CYBORDER),
            0,
            IGetSystemMetrics(SM_CYMENU) - IGetSystemMetrics(SM_CXBORDER));

    /*
      Go through all of the menubar items. 
    */
    for (iItem = 1, p = m->itemList;  p;  p = p->next, iItem++)
    {
      mi = (LPMENUITEM) p->data;

      if (mi->text)
      {
#if defined(USE_BITMAPS_IN_MENUS)
        if (bMaxMDIMenu && (iItem == 1 || iItem == m->nItems))
          sLen = 18;
        else if (mi->flags & (MF_OWNERDRAW | MF_BITMAP))
          sLen = LOWORD(MenuGetItemDimensions(m->hWnd, mi));
        else
#endif
          sLen = LOWORD(_GetTextExtent(mi->text));
        mi->wTextExtent = sLen;
        rect.right = rect.left + sLen;
        mi->rectItem = rect;

        /*
          Move the rectangle over by the length of the menu item plus
          the space between two menu items.
        */
        rect.left = rect.right + SM_CXBETWEENMENUITEMS;

        /*
          Take care of a right-justified menubar item
        */
        if (mi->flags & (MF_HELP | MF_RIGHTJUST))
        {
          WINDOW *wMenu = WID_TO_WIN(m->hWnd);
          MWCOORD  col = RECT_WIDTH(wMenu->rClient) - sLen - SysGDIInfo.tmAveCharWidth;

          /*
            If there is an MDI restore icon in this menubar, then shift
            the help item over a few spaces to the left in order to make
            room for the icon.
          */
          if ((mi->flags & MF_HELP) && bMaxMDIMenu)
#if defined(USE_BITMAPS_IN_MENUS)
            col -= 18;
#else
            col -= SM_RESTOREICONWIDTH;
#endif

#if defined(USE_BITMAPS_IN_MENUS)
          if (!(mi->flags & MF_HELP) && bMaxMDIMenu)
            col += SysGDIInfo.tmAveCharWidth;
#endif

          rect.left    = col;
          rect.right   = col + sLen;
          mi->rectItem = rect;
        }
      }
    } /* for p */
  }

  else /* m is a POPUP menu */
  {
    INT maxLen = 0;

#ifdef USE_TABS
    HDC hDC = GetDC(NULL);
#endif

    /*
      The left side of the item starts right after the border plus space for
      the checkmark (except in a separator)
    */
    SetRect(&rect,
            IGetSystemMetrics(SM_CXBORDER),
            IGetSystemMetrics(SM_CYBORDER),
            0,
            0);


    for (p = m->itemList;  p;  p = p->next)
    {
      mi = (LPMENUITEM) p->data;

      rect.bottom = rect.top + mi->cyItem;
      mi->rectItem = rect;
      rect.top = rect.bottom;

      if ((mi->flags & MF_SEPARATOR))
      {
        mi->wTextExtent = 0;
        mi->rectItem.left = IGetSystemMetrics(SM_CXBORDER);
        continue;
      }

#if defined(USE_BITMAPS_IN_MENUS)
      if (mi->flags & (MF_OWNERDRAW | MF_BITMAP))
        sLen = LOWORD(MenuGetItemDimensions(m->hWnd, mi));
      else
#endif

      if (mi->text)
      {
#ifdef USE_TABS
      sLen = LOWORD(GetTabbedTextExtent(hDC, mi->text, lstrlen(mi->text), 0, NULL));
#else
      sLen = LOWORD(_GetTextExtent(mi->text));
      /*
        Account for a tab
      */
      if (lstrchr(mi->text, '\t') != NULL)
        sLen += 7 * SysGDIInfo.tmAveCharWidth;
#endif
      }
      else
        sLen = 0;


#if defined(USE_BITMAPS_IN_MENUS)
      sLen += SysGDIInfo.tmAveCharWidth +
                  LOWORD(GetMenuItemCheckMarkDimensions(mi));
#else
      /* + 2 to accomodate a space and checkmark */
      sLen += 2 * SysGDIInfo.tmAveCharWidth;
#endif

      /* Make room for the submenu character */
      if ((mi->flags & MF_POPUP))
        sLen += LOWORD(_GetTextExtent(szSubMenu));

      mi->wTextExtent = sLen;
      maxLen = max(maxLen, sLen);
    }

    for (p = m->itemList;  p;  p = p->next)
    {
      mi = (LPMENUITEM) p->data;
      mi->rectItem.right = mi->rectItem.left + maxLen;
    }

#ifdef USE_TABS
    ReleaseDC(NULL, hDC);
#endif

    WinSetSize(m->hWnd, 
               rect.bottom + IGetSystemMetrics(SM_CYBORDER),
               maxLen + 2*IGetSystemMetrics(SM_CXBORDER));
  }

  HilitePrefix = chSavePrefix;
}



#if defined(USE_BITMAPS_IN_MENUS)

static DWORD PASCAL GetMenuItemCheckMarkDimensions(lpMI)
  LPMENUITEM lpMI;
{
  BITMAP  bm;
  HBITMAP hbm = lpMI->hbmChecked;

  if (hbm == DEFAULT_MENU_CHECK || hbm == NULL)
    return GetMenuCheckMarkDimensions();

  GetObject(hbm, sizeof(bm), (LPSTR) &bm);
  return MAKELONG(bm.bmWidth, bm.bmHeight);
}

#endif

