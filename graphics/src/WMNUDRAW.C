/*===========================================================================*/
/*                                                                           */
/* File    : WMNUDRAW.C                                                      */
/*                                                                           */
/* Purpose : Routines to draw a menu                                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_MENU
#define INCLUDE_CURSES
#define NOKERNEL

#include "wprivate.h"
#include "window.h"
#ifdef MEWEL_GUI
#include "wgraphic.h"
#endif

#define GetSysMetrics(n)  (n)


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

/*
  Structure which records info about disabled menubar items.
*/
typedef struct tagDisabledMenubarItemInfo
{
  LPSTR pszItem;
  INT   iColumn;
  UINT  idxPos;
} DISABLEDMENUBARITEMINFO;



/*===========================================================================*/
/*                                                                           */
/* MenuRefresh() -                                                           */
/*   This function draws a menu (popup or pulldown)                          */
/*                                                                           */
/*===========================================================================*/
#ifndef MEWEL_GUI
INT FAR PASCAL MenuRefresh(wMenu)
  WINDOW  *wMenu;
{
  MENU     *m;
  LPMENUITEM mi;
  HWND     hMenu = wMenu->win_id;
  LIST     *p;
/*
  First off, we want enough of a buffer that can handle 160-column screens.

  *Second*, and *much* more important...  This buffer was being allocated
  as the maximum screen width plus one.  But, this buffer contained
  unprintable characters (i.e. hilites), which could push the data being
  placed into the buffer past the screen width.  So, THE STACK WAS BEING
  OVERWRITTEN!!!  Not a good thing...

  Allocating 191 bytes is a hack.  We aren't bothering to calculate a real
  required length - we're just fixing it for our case (with a little
  extra space for safety.
*/
#if (defined(sunos) || defined(VAXC))
  char    buf[BUFSIZ];
#else
  char    buf[160+1+30];
#endif
  LPSTR   pItem;
  int     itemLen, hiliteCol;
  int     offset, hiliteCount;
  LPMENUITEM miHilite;
  DISABLEDMENUBARITEMINFO DMII[16],
                          *pDMII = DMII;
  int      pos, col;
  BOOL     bHasBorder = (BOOL) ((wMenu->flags & WS_BORDER) != 0L);
  int      width = WIN_CLIENT_WIDTH(wMenu);
  COLOR    attr, a, saveAttr;
  COLOR    oldHilitePrefixAttr;
  PSTR     szBlank = (PSTR) " ";

  BOOL   bHidMouse =  MouseHideIfInRange(wMenu->rect.top, wMenu->rect.bottom);

  HilitePrefix = HILITE_PREFIX;
  oldHilitePrefixAttr = HilitePrefixAttr;

  /*
    Get a pointer to the menu structure
  */
  m = (MENU *) wMenu->pPrivate;
  pos = col = 0;
  saveAttr = attr = wMenu->attr;
  if (attr == SYSTEM_COLOR)
    wMenu->attr = attr = WinQuerySysColor(wMenu->win_id, SYSCLR_MENU);

  /*
    Draw the border and the shadow
  */
  if (bHasBorder)
    WinDrawBorder(hMenu, 0);      /* a single-line border */
  WinDrawShadow(hMenu);

  /*
    If we have a menubar, clear the top line of the window
  */
  if (IS_MENUBAR(m))
  {
    col = GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM);
    hiliteCount = 0;
  }

  /*
    Initialize the menubar to all blanks
  */
  memset(buf, ' ', sizeof(buf));
  miHilite = NULL;
  hiliteCount = hiliteCol = 0;

  /*
    Draw each menu item
  */
  for (p = m->itemList;  p;  p = p->next, pos++)
  {
    /*
      See if we're drawing past the right border. If so, get outta here!
    */
    if (col - hiliteCount >= width)
      break;

    mi = (LPMENUITEM ) p->data;

    /*
      Figure out the attribute to draw the item with.
    */
    if (bUseSysColors)
    {
      if (mi->flags & (MF_DISABLED | MF_GRAYED))
      {
        a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUGRAYEDTEXT);
        if (!IS_MENUBAR(m))
        {
          if (pos == m->iCurrSel)
          {
            a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUGRAYEDHILITESEL);
            HIGHLITE_ON();
          }
          HilitePrefixAttr = a;
        }
      }
      else
      {
        if (pos == m->iCurrSel)
        {
          a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUHILITESEL);
          HIGHLITE_ON();
        }
        else
          a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUTEXT);
        /* Set HilitePrefixAttr to SYSCLR_MENUHILITEITEM */
        HilitePrefixAttr =  WinQuerySysColor(NULLHWND,SYSCLR_MENUHILITETEXT);
      }
    }
    else  /* not using the system colors */
    {
      a = (pos == m->iCurrSel) ? MAKE_HIGHLITE(attr) : attr;
      if (mi->flags & (MF_DISABLED | MF_GRAYED))
        a ^= 0x08;
    }


    /*
      Get the length of the item and a ptr to the text.
    */
    if ((mi->flags & MF_SEPARATOR))
    {
      itemLen = offset = 0;
    }
    else
    {
      pItem = mi->text;
      itemLen = lstrlen(pItem);
      offset = 0;
      /*
        See if it has a prefix char.
        If so, we have one more hilited entry, so bump up hiliteCount.
      */
      if (mi->letter && lstrchr(pItem, HILITE_PREFIX))
      {
        hiliteCount++;
        offset = 1;
      }
    }


    if (IS_MENUBAR(m))
    {
      if (pos == m->iCurrSel)
      {
        /*
          We are about to process the menubar entry which is highlighted.
          Record the item structure and figure out the starting column
          for the highlight.
        */
        miHilite = mi;
        if (mi->flags & (MF_HELP | MF_RIGHTJUST))
        {
          hiliteCol = width - itemLen /*- offset*/;
          if ((mi->flags & MF_HELP) && (wMenu->ulStyle & MFS_RESTOREICON))
            hiliteCol -= 2;
        }
        else
          hiliteCol = col - max((hiliteCount-offset), 0);
      }

      if (mi->flags & (MF_HELP | MF_RIGHTJUST))
      {
        /*
          Figure out the right-justified column where the help item should
          appear.
          Copy the item to the appropriate spot in the output buffer, and
          move the pointer to the next spot to fill.
        */
        col = (width-1) - itemLen - offset + hiliteCount;
        if ((mi->flags & MF_HELP) && (wMenu->ulStyle & MFS_RESTOREICON))
          col -= 2;
        buf[col++] = SysBoxDrawingChars[0][2];
        lmemcpy(buf+col, pItem, itemLen);
        col += itemLen - offset;
      }

      else /* not drawing a help item */
      {
        /*
          Record a disabled menubar item
        */
        if (mi->flags & (MF_DISABLED | MF_GRAYED))
        {
          pDMII->pszItem = pItem;
          pDMII->iColumn = col - max(0, hiliteCount-1);
          pDMII->idxPos  = pos;
          pDMII++;
        }

        /*
          Copy the item to the appropriate spot in the output buffer, and
          move the pointer to the next spot to fill.
        */
#ifdef DUMB_CURSES
        /* Show a '-' character in front of disabled menu items */
        if (mi->flags & (MF_DISABLED | MF_GRAYED))
          buf[col - 1] = '-';
#endif
        lmemcpy(buf+col, pItem, itemLen);
        col += itemLen + GetSysMetrics(SM_CXBETWEENMENUITEMS);
      }
    }

    else /* we are drawing a pulldown item */
    {
       /*
         First, erase the bad part of the pulldown row.
       */

      if (mi->flags & MF_SEPARATOR)
      {
        char szSeparator[65];
        memset(szSeparator, SysBoxDrawingChars[0][0], width);
        szSeparator[width] = '\0';
        WinPuts(hMenu, pos, 0, szSeparator, a);
      }
      else
      {
        /*
          We have a regular string for the menu item. Output it.
          First, erase the bad part of the pulldown row.
        */
        WinEraseEOL(hMenu, pos, itemLen+1 - offset, a);
        lstrcpy(buf, mi->text);

        /*
          Do some work to right-justify the menu item
        */
        if (mi->iRightJustPos != 0)
        {
          LPSTR pSlash = mi->text + mi->iRightJustPos;
          int  iStrWidth = lstrlen(pSlash)-2;  /* don't include the \a */
          int  iRightPos = width - iStrWidth;
          if (iRightPos > 0)
          {
            /*
              Clear out the part of the string buffer which is to the
              right of the right-justified string (including the '\0').
              Then copy the right-justified part into the proper position.
            */
            memset(buf+mi->iRightJustPos, ' ', sizeof(buf)-mi->iRightJustPos);
            lstrncpy(buf+iRightPos, pSlash+2, iStrWidth);
            buf[iRightPos+iStrWidth] = '\0';
          }
        }

        if (pos == m->iCurrSel)        /* get rid of the char hiliting */
        {
#if 0
          LPSTR pPrefix = lstrchr(buf, HILITE_PREFIX);
          if (pPrefix)
            bytedel(pPrefix, 1, 0);
          HilitePrefixAttr =  a;
#else
#ifdef CARRYER
          if (mi->flags & (MF_DISABLED | MF_GRAYED)) 
            HilitePrefixAttr = a;
          else
            HilitePrefixAttr =
              (bUseSysColors) ?  WinQuerySysColor(NULLHWND, SYSCLR_MENUHILITESEL)
                        :  (a & 0xF0) | (HilitePrefixAttr & 0x0F);
#else
          HilitePrefixAttr =  (a & 0xF0) | (HilitePrefixAttr & 0x0F);
#endif
#endif
        }  

        /*
          Leave column 1 for checkmarks
        */
#ifdef DUMB_CURSES
	/* Show a '-' character in front of disabled menu items */
        if (mi->flags & (MF_DISABLED | MF_GRAYED))
          WinPuts(hMenu, pos, 0, "-", a);
        else
#endif
        WinPuts(hMenu, pos, 0, szBlank, a);
#ifdef MICROPORT
        {
        char *hilite_loc = strchr(buf, HILITE_PREFIX);
        WinPuts(hMenu, pos, 1, buf, a);
        /* don't worry about whether it's selected or not...
         * (selected items are all inverse anyway...)
        */
        HIGHLITE_ON();
        WinPutc(hMenu, pos, 1+hilite_loc-buf, *(hilite_loc+1), a);
        HIGHLITE_OFF();
        }
#else
#ifdef aix
	/* for the currently selected item, forcefully switch into reverse
	 *   text mode.
	 */
        if (pos == m->iCurrSel)
        {
          a = 0x70; /* reverse */
          /* clear the entire line to get the full bar effect */
          WinEraseEOL(hMenu, pos, 0, a);
        }		
#endif
        WinPuts(hMenu, pos, 1, buf, a);
#endif

#ifdef aix
        { /* allows us to define a local var. */
           /* find the prefix in the item text and work on that only */
           char *hilite_loc = strchr(buf, HILITE_PREFIX);

           /* use reverse if currently selected, else bold */
           a = (pos == m->iCurrSel) ? 0x70 : 0x08;
           WinPutc(hMenu, pos, 1+hilite_loc-buf, *(hilite_loc+1), a);
         }
#endif


        /*
          If the menu is a popup and it has a child, draw a pointer char...
        */ 
        if ((mi->flags & MF_POPUP) && (UINT) mi->id != wMenu->win_id)
          WinPutc(hMenu, pos, width-1, WinGetSysChar(SYSCHAR_SUBMENU), a);
        if (mi->flags & MF_CHECKED)
          WinPutc(hMenu, pos, 0, WinGetSysChar(SYSCHAR_MENUCHECK), a);
        /*
           If it was selected, restore the character hilite...
        */
        if (pos == m->iCurrSel && bUseSysColors) 
          HilitePrefixAttr = WinQuerySysColor(NULLHWND,SYSCLR_MENUHILITETEXT);
        else
          HilitePrefixAttr = oldHilitePrefixAttr;
      }
    }

    HIGHLITE_OFF();

  } /* end for (p) */


  /*
    We are done filling the menubar virtual buffer, so output it for real.
  */
  if (IS_MENUBAR(m))
  {
    buf[col+1] = '\0';
    WinPuts(hMenu, 0, 0, buf, attr);
    WinEraseEOL(hMenu, 0, col+1-hiliteCount, attr);

    /*
      If an entry is selected, output it in reverse video. Note that the
      entry's prefix char should not be highlighted as this looks bad.
    */
    if (miHilite)
    {
      a = 
        (bUseSysColors) ?  WinQuerySysColor(NULLHWND, SYSCLR_MENUHILITESEL)
                        :   MAKE_HIGHLITE(attr);
#ifdef CARRYER
      HilitePrefixAttr =
                 (bUseSysColors) ?  WinQuerySysColor(NULLHWND, SYSCLR_MENUSELHILITESEL)
                        :  (a & 0xF0) | (HilitePrefixAttr & 0x0F);
#else
      HilitePrefixAttr =  (a & 0xF0) | (HilitePrefixAttr & 0x0F);
#endif

      WinPuts(hMenu, 0, hiliteCol, miHilite->text, a);
      /*
        Highlite one space before and one space after the item
      */
      WinPuts(hMenu, 0, hiliteCol-1, szBlank, a);
      col = hiliteCol + lstrlen(miHilite->text);
      if (lstrchr(miHilite->text, HILITE_PREFIX))
        col--;
      WinPuts(hMenu, 0, col, szBlank, a);
    }

    /*
      Time to output all of the disabled menubar items. See if we recorded
      any (we did if the pMDII pointer was bumped up past the MDII array).
    */
    if (pDMII != DMII)
    {
      while (--pDMII >= DMII)
      {
        if (bUseSysColors)
        {
          a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUGRAYEDTEXT);
          if (pDMII->idxPos == (UINT) m->iCurrSel)
            a =  WinQuerySysColor(NULLHWND, SYSCLR_MENUHILITESEL);
          HilitePrefixAttr = a;
        }
        else  /* not using the system colors */
          a = ((pDMII->idxPos == (UINT) m->iCurrSel) ? MAKE_HIGHLITE(attr) : attr) ^ 0x08;
  
        WinPuts(hMenu, 0, pDMII->iColumn, pDMII->pszItem, a);
      }
    }

  }

  if (bHidMouse)
    MouseShow();

  HilitePrefix = '\0';
  HilitePrefixAttr = oldHilitePrefixAttr;
  wMenu->attr = saveAttr;
  return TRUE;
}
#endif



#if !112892 && defined(MEWEL_GUI)

/*
  Define this if we want to attempt inverting the highlited menu item
*/
#if 1
#define INVERT_ITEM
#endif


INT FAR PASCAL MenuRefresh(wMenu)
  WINDOW  *wMenu;
{
  char       buf[160+1+30], *pItem;
  MENU       *m;
  HWND       hMenu = wMenu->win_id;
  LIST       *p;
  int        itemLen, hiliteCol;
  int        offset, hiliteCount;
  LPMENUITEM mi;
  LPMENUITEM miHilite;
  DISABLEDMENUBARITEMINFO DMII[16],
                          *pDMII = DMII;
  int      pos, col;
  int      width = WIN_CLIENT_WIDTH(wMenu);
  int      pixWidth = width;

  HDC      hDC;
  LOGBRUSH lb;
  int      yFontHeight;
  int      cyMenuOffset;
  int      xHelpCol = 0;
  RECT     r;


  HilitePrefix = HILITE_PREFIX;

  /*
    Get a pointer to the menu structure
  */
  m    = (MENU *) wMenu->pPrivate;
  col  = 0;

  /*
    Get a DC for the menu.
  */
  hDC = GetWindowDC(hMenu);

  yFontHeight = SysGDIInfo.tmHeightAndSpace;
  /*
    This is used to center the text vertically within a menu item's rectangle.
  */
#if defined(GX) || defined(__TURBOC__)
  cyMenuOffset = SysGDIInfo.tmExternalLeading >> 1;
#else
  cyMenuOffset = 2;
#endif


  /*
    Incremental redrawing of a popup....
      Unhighlight the old selected item and highlight the new one.
  */
#ifdef INVERT_ITEM
  if (!IS_MENUBAR(m) && MenuInvertInfo.bInverting &&
      MenuInvertInfo.iOldSel != m->iCurrSel && MenuInvertInfo.hMenu == hMenu)
  {
    for (pos = 0;  pos <= 1;  pos++)
    {
      int iSel = (pos == 0) ? MenuInvertInfo.iOldSel : m->iCurrSel;
      if (iSel != -1)
      {
        SetRect(&r, IGetSystemMetrics(SM_CXBORDER), iSel*yFontHeight,
                    width+1, (iSel+1) * yFontHeight);
        InvertRect(hDC, (LPRECT) &r);
      }
    }
    goto bye;
  }
#endif

  /*
    Set the text color and make it transparent.
    Set the brush we are going to erase with.
  */
  SetBkMode(hDC, TRANSPARENT);
  SelectObject(hDC, SysBrush[COLOR_MENU]);
  SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
  GetObject(SysBrush[COLOR_MENU], sizeof(lb), &lb);
  SetBkColor(hDC, lb.lbColor);

  /*
    Draw the enclosing rectangle
  */
  WindowRectToPixels(hMenu, &r);
  Rectangle(hDC, 0, 0, r.right, r.bottom);

  /* Make the column character based */
  if (IS_MENUBAR(m))
    col = GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM) / SysGDIInfo.tmAveCharWidth;

  /*
    Initialize the menubar to all blanks
  */
  memset(buf, ' ', sizeof(buf));
  miHilite = NULL;
  hiliteCount = hiliteCol = 0;
  width /= SysGDIInfo.tmAveCharWidth;  /* make it char based */


  /*
    Draw each menu item
  */
  for (pos = 0, p = m->itemList;  p;  p = p->next, pos++)
  {
    /*
      See if we're drawing past the right border. If so, get outta here!
    */
    if (col - hiliteCount >= width)
      break;

    mi = (LPMENUITEM) p->data;

    /*
      Figure out the attribute to draw the item with.
    */
    SetTextColor(hDC, GetSysColor((mi->flags & (MF_DISABLED | MF_GRAYED))
                                    ? COLOR_GRAYTEXT : COLOR_MENUTEXT));

    /*
      Get the length of the item and a ptr to the text.
    */
    if ((mi->flags & MF_SEPARATOR))
    {
      itemLen = offset = 0;
    }
    else
    {
      pItem   = mi->text;
      itemLen = strlen(pItem);
      offset  = 0;
      /*
        See if it has a prefix char.
        If so, we have one more hilited entry, so bump up hiliteCount.
      */
      if (mi->letter && strchr(pItem, HILITE_PREFIX))
      {
        hiliteCount++;
        offset = 1;
      }
    }


    if (IS_MENUBAR(m))
    {
      if (pos == m->iCurrSel || (mi->flags & MF_HILITE))
      {
        /*
          We are about to process the menubar entry which is highlighted.
          Record the item structure and figure out the starting column
          for the highlight.
        */
        miHilite = mi;
        if (mi->flags & (MF_HELP | MF_RIGHTJUST))
          hiliteCol = width - itemLen - offset + 1;
        else
          hiliteCol = col - max((hiliteCount-offset), 0);
      }

      if (mi->flags & (MF_HELP | MF_RIGHTJUST))
      {
        /*
          Figure out the right-justified column where the help item should
          appear.
          Copy the item to the appropriate spot in the output buffer, and
          move the pointer to the next spot to fill.
        */
        col = (width-1) - itemLen - offset + hiliteCount;
        xHelpCol = col++;
        memcpy(buf+col, pItem, itemLen);
        col += itemLen - offset;
      }

      else /* not drawing a help item */
      {
        /*
          Record a disabled menubar item
        */
        if (mi->flags & (MF_DISABLED | MF_GRAYED))
        {
          pDMII->pszItem = pItem;
          pDMII->iColumn = col - (hiliteCount-offset);
          pDMII->idxPos  = pos;
          pDMII++;
        }

        /*
          Copy the item to the appropriate spot in the output buffer, and
          move the pointer to the next spot to fill.
        */
        memcpy(buf+col, pItem, itemLen);
        col += itemLen;
        col += GetSysMetrics(SM_CXBETWEENMENUITEMS) / SysGDIInfo.tmAveCharWidth;
      }
    }

    else /* we are drawing a pulldown item */
    {
      /*
        Get the y coordinate of the menuitem.
      */
      int yItem = pos*yFontHeight + cyMenuOffset;

      /*
        First, erase the bad part of the pulldown row.
      */
      if (mi->flags & MF_SEPARATOR)
      {
        /*
          Center the separator within the item rectangle.
        */
        int y = pos*yFontHeight + (yFontHeight/2);
        MoveTo(hDC, IGetSystemMetrics(SM_CXBORDER), y);
        LineTo(hDC, pixWidth, y);
      }
      else
      {
        /*
          We have a regular string for the menu item. Output it.
          First, erase the bad part of the pulldown row.
        */
        strcpy(buf, mi->text);

#ifndef INVERT_ITEM
        if (pos == m->iCurrSel)
        {
          RECT r2;
          SetRect(&r2, IGetSystemMetrics(SM_CXBORDER), yItem - cyMenuOffset,
                       pixWidth, yItem - cyMenuOffset + yFontHeight - 1);
          SetTextColor(hDC, GetSysColor(COLOR_MENUHILITETEXT));
          FillRect(hDC, (LPRECT) &r2, SysBrush[COLOR_HIGHLIGHT]);
        }
#endif

        /*
          Do some work to right-justify the menu item
        */
        if (mi->iRightJustPos != 0)
        {
          LPSTR pSlash = mi->text + mi->iRightJustPos;
          int  iStrWidth = lstrlen(pSlash)-2;  /* don't include the \a */
          int  iRightPos = width - iStrWidth - 1;
          if (iRightPos > 0)
          {
            /*
              Clear out the part of the string buffer which is to the
              right of the right-justified string (including the '\0').
              Then copy the right-justified part into the proper position.
            */
            memset(buf+mi->iRightJustPos, ' ', sizeof(buf)-mi->iRightJustPos);
            lstrncpy(buf+iRightPos, pSlash+2, iStrWidth);
            buf[iRightPos+iStrWidth] = '\0';
          }
        }

        /*
          Output the popup menuitem's text
        */
        TextOut(hDC,
                SysGDIInfo.tmAveCharWidth + IGetSystemMetrics(SM_CXBORDER),
                yItem, buf, strlen(buf));

        /*
          If the menu is a popup and it has a child, draw a pointer char...
        */ 
        if ((mi->flags & MF_POPUP) && (int) mi->id != (int) wMenu->win_id)
        {
          BYTE ch = (BYTE) WinGetSysChar(SYSCHAR_SUBMENU);
#if defined(MSC) && !defined(GX) && !defined(META)
          ch = '>';  /* the regular submenu char 16 is not available */
#endif
          TextOut(hDC, (width-1)*SysGDIInfo.tmAveCharWidth,yItem,(LPSTR)&ch,1);
        }


        /*
          Display the checkmark in a checked popup menu item
        */
        if (mi->flags & MF_CHECKED)
        {
          BYTE ch = (BYTE) WinGetSysChar(SYSCHAR_MENUCHECK);
#if defined(MSC) && !defined(GX) && !defined(META)
          ch = 150;  /* checkmark in the MSC graphics font set */
#endif
          TextOut(hDC, IGetSystemMetrics(SM_CXBORDER), yItem, (LPSTR) &ch, 1);
        }

        /*
          Highlight the currently selected menu item
        */
#ifdef INVERT_ITEM
        if (pos == m->iCurrSel)
        {
          RECT r2;
          SetRect(&r2, IGetSystemMetrics(SM_CXBORDER), yItem - cyMenuOffset,
                       pixWidth + 1, (yItem - cyMenuOffset) + yFontHeight);
          InvertRect(hDC, (LPRECT) &r2);
        }
#endif
      }
    } /* end pulldown */
  } /* end for (p) */


  /*
    We are done filling the menubar virtual buffer, so output it for real.
  */
  if (IS_MENUBAR(m))
  {
    buf[col+1] = '\0';
    SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
    TextOut(hDC, 0, cyMenuOffset, buf, strlen(buf));

    /*
      Draw the help item separator, if any
    */
    if (xHelpCol)
    {
      int x = (xHelpCol-1) * SysGDIInfo.tmAveCharWidth - 
                    GetSysMetrics(SM_CXBEFOREFIRSTMENUITEM);
      MoveTo(hDC, x, 0);
      LineTo(hDC, x, IGetSystemMetrics(SM_CYMENU));
    }

    /*
      If an entry is selected, output it in reverse video. Note that the
      entry's prefix char should not be highlighted as this looks bad.
    */
    if (miHilite)
    {
      col = hiliteCol + strlen(miHilite->text);
      if (strchr(miHilite->text, HILITE_PREFIX))
        col--;

      SetRect(&r, (hiliteCol-1) * SysGDIInfo.tmAveCharWidth, 2,
                  (col + 1) * SysGDIInfo.tmAveCharWidth, yFontHeight - 1);
#ifdef INVERT_ITEM
      InvertRect(hDC, (LPRECT) &r);
#else
      SetTextColor(hDC, GetSysColor(COLOR_MENUHILITETEXT));
      FillRect(hDC, (LPRECT) &r, SysBrush[COLOR_HIGHLIGHT]);
      TextOut(hDC, hiliteCol*SysGDIInfo.tmAveCharWidth, cyMenuOffset,
               miHilite->text, strlen(miHilite->text));
#endif
    }

    /*
      Time to output all of the disabled menubar items. See if we recorded
      any (we did if the pMDII pointer was bumped up past the MDII array).
    */
    if (pDMII != DMII)
    {
      SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
      while (--pDMII >= DMII)
        TextOut(hDC, pDMII->iColumn*SysGDIInfo.tmAveCharWidth, cyMenuOffset,
                pDMII->pszItem, strlen(pDMII->pszItem));
    }
  } /* end if MENUBAR */


bye:
  ReleaseDC(hMenu, hDC);

  HilitePrefix = '\0';
  return TRUE;
}

#endif

