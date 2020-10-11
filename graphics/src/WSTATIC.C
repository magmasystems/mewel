/*===========================================================================*/
/*                                                                           */
/* File    : WSTATIC.C                                                       */
/*                                                                           */
/* Purpose : Routines to create, draw, and handle the static window class.   */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_LISTBOX
#define INCLUDE_COMBOBOX
#define INCLUDE_CURSES

#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
static VOID PASCAL StaticDraw(HWND);
static VOID PASCAL StaticDrawBorder(HWND);
#ifdef __cplusplus
}
#endif


HWND FAR PASCAL StaticCreate(hParent,row1,col1,row2,col2,szText,attr,dwStyle,id,hInst)
  HWND  hParent;
  int   row1, col1, row2, col2;
  LPSTR szText;
  COLOR attr;
  DWORD dwStyle;
  INT   id;
  HINSTANCE hInst;
{
  WINDOW *w;
  HWND   hWnd;
  UINT   idClass;

  /*
    Make sure that frames have the border style set
  */
  if (dwStyle & SS_FRAME)
    dwStyle |= WS_BORDER;

  /*
    Create the window structure
  */
  if ((hWnd = WinCreate(hParent,
                        row1,col1,row2,col2,
                        ((dwStyle & SS_ICON) == SS_ICON) ? NULL : szText,
                        attr, 
                        dwStyle | WS_CLIPSIBLINGS /* | WS_GROUP */,
                        STATIC_CLASS,
                        id)) == NULLHWND)
    return NULLHWND;

  w = WID_TO_WIN(hWnd);
  idClass = w->idClass;
  
  /*
    Depending on the style of the static control, give the static a
    new MEWEL-ized base class.
  */
  if (dwStyle & SS_TEXT) 
  {
    idClass = TEXT_CLASS;
    /*
      Kludge - if we have a static text with SS_NOPREFIX, undo the
      translation of the ampersands into tildes.
    */
    if ((dwStyle & SS_NOPREFIX))
    {
      for (szText = w->title;  *szText;  szText++)
        if (*szText == HILITE_PREFIX)
          *szText = '&';
    }
  }
  else if (dwStyle & SS_FRAME)
    idClass = FRAME_CLASS;
  else if (dwStyle & SS_BOX)
    idClass = BOX_CLASS;
  else if ((dwStyle & SS_ICON) == SS_ICON)
  {
#if (MODEL_NAME == MODEL_MEDIUM)
    /*
      Kludge for PSTR<->LPSTR truncation of IDI_xxx
    */
    UINT iTitle = (UINT) FP_OFF(szText);
    if (iTitle >= 32512 && iTitle <= 32516)
      w->hIcon = LoadIcon(NULL, MK_FP(0, iTitle));
    else
#endif
    w->hIcon = LoadIcon(hInst, szText);
  }

  w->idClass = idClass;
  return hWnd;
}


/*===========================================================================*/
/*                                                                           */
/* File    : STDRAW.C                                                        */
/*                                                                           */
/* Purpose : Routines to draw a static control                               */
/*                                                                           */
/*===========================================================================*/
VOID PASCAL StaticDraw(hWnd)
  HWND   hWnd;
{
#if !defined(USE_NATIVE_GUI)
  WINDOW *w;
  INT    idClass;
  DWORD  dwFlags;
  COLOR  oldHilitePrefixAttr = HilitePrefixAttr;
  BOOL   bPartOfCombo;

#if defined(MEWEL_TEXT)
  COLOR  attr;
#endif

#ifdef MEWEL_GUI
  TEXTMETRIC tm;
  RECT   pixRect;
  HDC    hDC;
  HFONT  hFont, hOldFont;
#endif


  if ((w = WID_TO_WIN(hWnd)) == NULL || !IsWindowVisible(hWnd))
    return;

  dwFlags = w->flags;

  if (!(dwFlags & SS_NOPREFIX))
    HilitePrefix = HILITE_PREFIX;

#ifdef MEWEL_TEXT
  if ((attr = w->attr) == SYSTEM_COLOR)
    HilitePrefixAttr = WinQuerySysColor(hWnd, SYSCLR_WINDOWPREFIXHIGHLIGHT);

  if (_PrepareWMCtlColor(hWnd, CTLCOLOR_STATIC, 0))
  {
    attr = w->attr;
    HilitePrefixAttr = MAKE_ATTR(
          GET_FOREGROUND(WinQuerySysColor(hWnd, SYSCLR_WINDOWPREFIXHIGHLIGHT)),
          GET_BACKGROUND(attr));
  }
#endif


  idClass = _WinGetLowestClass(w->idClass);

  /*
    If the class is 0 (ie - normal window), then this routine was probably
    reached by calling CallWindowProc() on a subclass of a static text
    control (ie - the way that zAPP handles control window creation).
    So, if the class is 0, check the style bits for SS_xxxx.
  */
  if (idClass == NORMAL_CLASS || idClass == STATIC_CLASS)
  {
    if (dwFlags & SS_TEXT)
      idClass = TEXT_CLASS;
    else if (dwFlags & SS_FRAME)
      idClass = FRAME_CLASS;
    else if (dwFlags & SS_BOX)
      idClass = BOX_CLASS;
  }

  if (idClass == TEXT_CLASS)
  {
#ifndef MEWEL_GUI
    WinClear(hWnd);
#endif

    /*
      bPartOfCombo is 1 if the static text control is part of a combobox.
      It is set to 2 if it also has the input focus and it is not
      ownerdrawn (the owner-drawn control will take care of its own
      selection drawing).
    */
    bPartOfCombo = (BOOL) (w->parent &&
                      _WinGetLowestClass(w->parent->idClass) == COMBO_CLASS);
    if (bPartOfCombo && InternalSysParams.hWndFocus == hWnd &&
        !(w->parent->flags & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)))
      bPartOfCombo++;

#ifdef MEWEL_TEXT
    /*
      Determine the color of the control.
    */
    if (attr == SYSTEM_COLOR)
      attr = WinQuerySysColor(NULLHWND, 
        ((dwFlags & WS_DISABLED) != 0L || 
         (bPartOfCombo && (w->parent->flags & WS_DISABLED)))
               ? SYSCLR_DISABLEDBUTTON : SYSCLR_WINDOWTEXT);

    /*
      If the static control is disabled, then make the "hotkey" the
      same color as the rest of the control.
    */
    if (dwFlags & WS_DISABLED)
      HilitePrefixAttr = attr;
#endif

    /*
      Special processing for static fields which are part of combo boxes.
      Draw borders and draw highlighted if it has the focus.
    */
#ifndef MEWEL_GUI
    if (bPartOfCombo)
    {
      COLOR  attr2;

      bDrawingBorder++;
      WinPutc(hWnd, 0, 0, WinGetSysChar(SYSCHAR_EDIT_LBORDER), attr);

      if (InternalSysParams.hWndFocus == hWnd)
      {
        attr2 = MAKE_HIGHLITE(attr);
        HIGHLITE_ON();
      }
      else
        attr2 = attr;
      WinPuts(hWnd, 0, 1, (w->title && w->title[0]) ? w->title : " ", attr2);
      HIGHLITE_OFF();

      WinPutc(hWnd, 0, RECT_WIDTH(w->rect)-1,
                          WinGetSysChar(SYSCHAR_EDIT_RBORDER), attr);
      bDrawingBorder--;
    }
    else
#endif
    {
      LPSTR pOrigTitle = w->title;
      LPSTR s = pOrigTitle ? pOrigTitle : " ";
      int  row;
      int  sLen   = lstrlen(s);
      int  iWidth = RECT_WIDTH(w->rClient);
      int  iHeight= RECT_HEIGHT(w->rClient);
      int  nCharsPerLine;
      int  cxChar, cyChar;

#ifdef MEWEL_GUI
      int   x;
      int   xOffset = 0;
      int   yOffset = 0;
      int   sLen2;
      HBRUSH hBr;

      /*
        Get a window DC for the static control.
      */
      hDC = GetWindowDC(hWnd);
      WindowRectToPixels(hWnd, (LPRECT) &pixRect);

      /*
        Set the current font
      */
      if ((hFont = (HFONT) SendMessage(hWnd, WM_GETFONT, 0, 0L)) != NULL)
        hOldFont = SelectObject(hDC, hFont);

      /*
        Determine the height and width of a single character
      */
      GetTextMetrics(hDC, &tm);

      /*
        Set the proper color attributes for writing
      */
      SetBkMode(hDC, TRANSPARENT);
      hBr = (HBRUSH) SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WPARAM) hDC,
                                 MAKELONG(hWnd, CTLCOLOR_STATIC));
      SelectObject(hDC, hBr);

      /*
        Erase the control's area
      */
      SendMessage(hWnd, WM_ERASEBKGND, (WPARAM) hDC, 0L);
#endif

      /*
        Get the character height and width. 
      */
#ifdef MEWEL_GUI
      cyChar = tm.tmHeight + tm.tmExternalLeading;
      cxChar = tm.tmAveCharWidth;
#else
      cyChar = 1;
      cxChar = 1;
#endif


      /*
        Draw the border and the shadow.... yes, multi-line static controls can 
        have these....
      */
      w->title = NULL;  /* so a title doesn't get drawn on the border */
#ifdef MEWEL_GUI
      if (dwFlags & WS_BORDER)
      {
        if (SysGDIInfo.fFlags & GDISTATE_NO_BEVELS)
        {
          HBRUSH hOldBr = SelectObject(hDC, GetStockObject(NULL_BRUSH));
          Rectangle(hDC, pixRect.left, pixRect.top, pixRect.right, pixRect.bottom);
          SelectObject(hDC, hOldBr);
        }
        else
          DrawBeveledBox(hWnd, hDC, (LPRECT) &pixRect, 1, 1,
               (InternalSysParams.hWndFocus == hWnd), 
               (bPartOfCombo == 2) ? SysBrush[COLOR_HIGHLIGHT] : 
                                     _GetDC(hDC)->hBrush);
        InflateRect(&pixRect, -1, -1);

        /*
          If we have a border, create a margin around it for the text
        */
        xOffset = 4;
        if (bPartOfCombo)
        {
          if (DrawOwnerDrawnStatic(hWnd))
            goto end_of_text_drawing;
          if (bPartOfCombo == 2)
            SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
          if ((yOffset = (RECT_HEIGHT(pixRect) - tm.tmHeight) / 2) < 0)
            yOffset = 0;
        }
        else
          yOffset = 4;
        iWidth  -= xOffset;
        iHeight -= yOffset;
      }
#else
      WinDrawBorder(hWnd, 0);
#endif
      w->title = pOrigTitle;
      WinDrawShadow(hWnd);

      /*
        Adjust the string length if it contains a hot-key character
      */
      if (lstrchr(s, HILITE_PREFIX) != NULL)
        sLen--;

      /*
        Determine how many characters can fit on one line.
      */
      nCharsPerLine = iWidth / cxChar;


      /*
        Write out the string. If we have a multi-line text field, break
        up the lines.
      */
      for (row = 0;  row < iHeight;  row += cyChar)
      {
        BYTE  chSave;
        LPSTR pchSave;
        LPSTR pchSpace;  /* ptr to current space(blank) code */

        pchSave  = NULL;
        pchSpace = NULL;

        /*
          See if the string to write is longer than the width of the static
          control. If it is, we have to find a place to wordwrap.
          Also wrap if the line has an embedded newline character.
        */
        if (nCharsPerLine < sLen || lstrchr(s, '\n'))
        {
          for (pchSave = s;  pchSave <= s + nCharsPerLine;  pchSave++)
          {
            if (*pchSave == ' ' || *pchSave == '\n')
              pchSpace = pchSave;    /* save ptr to current word break */
            if (*pchSave == '\n')
              break;                 /* end of newline found */
          }
          if (pchSpace != NULL)      /* if not NULL, then use as end-of-line */
            pchSave = pchSpace;
          if (*pchSave != ' ' && *pchSave != '\n')
            pchSave = s + nCharsPerLine;
          chSave = *pchSave;
          /*
            Do not word wrap if we are at the last line of the static control.
            Instead, leave it to the WinPuts() to clip the string.
          */
          if (row < iHeight-cyChar)
            *pchSave = '\0';
        }

#ifdef MEWEL_GUI
        sLen2 = (int) LOWORD(_GetTextExtent(s));

        /*
          Determine the x coordinate where the output should begin
          Note - we should really use GetTextExtent() here
        */
        if ((dwFlags & 0x0F) == SS_CENTER)
          x = (pixRect.right - sLen2 - xOffset) / 2;
        else if ((dwFlags & 0x0F) == SS_RIGHT)
        {
          x = (pixRect.right - sLen2 - xOffset);
          xOffset = 0;
        }
        else
          x = 0;

        /*
          Output the static text at the appropriate location
        */
        TextOut(hDC, x + xOffset, row + yOffset, s, lstrlen(s));
#else
        if ((dwFlags & 0x0F) == SS_CENTER)
          WinPutsCenter(hWnd, row, s, attr);
        else if ((dwFlags & 0x0F) == SS_RIGHT)
          WinPutsRight(hWnd, row, s, attr);
        else
          WinPuts(hWnd, row, 0, s, attr);
#endif

        if (pchSave)
        {
          sLen -= (pchSave - s);
          *(s = pchSave) = chSave;

          /*
            If we hit a space or newline, then skip over it.
          */
          if (chSave == ' ' || chSave == '\n')
          {
            sLen--;
            s++;
          }
        }
        else
          break;
      } /* end for */

#ifdef MEWEL_GUI
end_of_text_drawing:
      if (hFont)
        SelectObject(hDC, hOldFont);
      ReleaseDC(hWnd, hDC);
#endif

    } /* end if (wParent == COMBO) */
  } /* end if (idClass == TEXT_CLASS) */

  else if (idClass == FRAME_CLASS)
  {
    WinDrawBorder(hWnd, 0);
  }
  else if (idClass == BOX_CLASS)
  {
#ifdef MEWEL_GUI
    HBRUSH hBrush = 0;
#endif

    /*
      We need to do the testing of the type of frame here instead of in
      the window creation routine because the programmer might switch
      style bits in the middle of the app.
    */
    dwFlags &= 0x0FL;  /* so we can distinguish the various box styles */
    if (dwFlags >= (SS_BLACKRECT & 0x0FL) && 
        dwFlags <= (SS_WHITERECT & 0x0FL))
    {
#if defined(DOS) || defined(OS2)
      w->fillchar = 219;   /* the fill char is a solid box */
#endif
      if (dwFlags == (SS_BLACKRECT & 0x0FL))
      {
        w->attr = MAKE_ATTR(BLACK, BLACK);
#ifdef MEWEL_GUI
        hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
#endif
      }
      else if (dwFlags == (SS_WHITERECT & 0x0FL))
      {
        w->attr = MAKE_ATTR(WHITE, WHITE);
#ifdef MEWEL_GUI
        hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
#endif
      }
      else if (dwFlags == (SS_GRAYRECT & 0x0FL))
      {
        w->attr = MAKE_ATTR(INTENSE(BLACK), WHITE);
#ifdef MEWEL_GUI
        hBrush = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
#endif
      }
    }

#ifdef MEWEL_GUI
    if (hBrush)
    {
      hDC = GetDC(hWnd);
      WindowRectToPixels(hWnd, (LPRECT) &pixRect);
      FillRect(hDC, (LPRECT) &pixRect, hBrush);
      DeleteObject(hBrush);
      ReleaseDC(hWnd, hDC);
    }
    else
#endif
    WinFillRect(hWnd, (HDC) 0, &w->rClient, w->fillchar, w->attr);
  }
  else if (w->hIcon)
  {
    HDC hDC = GetDC(hWnd);
    DrawIcon(hDC, 0, 0, w->hIcon);
    ReleaseDC(hWnd, hDC);
  }

  HilitePrefix = '\0';
  HilitePrefixAttr = oldHilitePrefixAttr;
#endif /* NATIVE_GUI */
}


static VOID PASCAL StaticDrawBorder(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  DWORD  dwFlags = (w->flags & 0x0FL);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
  HPEN   hPen = 0,
         hOldPen;
  DWORD  rgbPen = 0L;

  if      (dwFlags == (SS_BLACKFRAME & 0x0FL))
    rgbPen = GetSysColor(COLOR_WINDOWFRAME);
  else if (dwFlags == (SS_WHITEFRAME & 0x0FL))
    rgbPen = GetSysColor(COLOR_WINDOW);
  else if (dwFlags == (SS_GRAYFRAME & 0x0FL))
    rgbPen = GetSysColor(COLOR_BACKGROUND);

  if (rgbPen != 0L)
    hPen = CreatePen(PS_SOLID, 1, rgbPen);

  if (dwFlags >= (SS_BLACKFRAME & 0x0FL) && 
      dwFlags <= (SS_WHITEFRAME & 0x0FL))
  {
    HDC hDC = GetWindowDC(hWnd);
    if (hPen)
      hOldPen = SelectObject(hDC, hPen);
    SelectObject(hDC, GetStockObject(NULL_BRUSH));
    Rectangle(hDC, 0, 0, RECT_WIDTH(w->rect), RECT_HEIGHT(w->rect));
    if (hPen)
    {
      SelectObject(hDC, hOldPen);
      DeleteObject(hPen);
    }
    ReleaseDC(hWnd, hDC);
  }


#if !defined(MOTIF)
  else   /* not a SS_BLACK/WHITE/GRAYFRAME, a regular groupbox */
  {
    RECT r;
    HDC  hDC;
    TEXTMETRIC tm;

    WindowRectToPixels(hWnd, (LPRECT) &r);
    /*
      If we have a title, we want it to be written in the middle
      of the top line of the frame.
    */
#ifdef ZAPP
    {
    HPEN oldPen, boxPen;
    int x1, y1, x2, y2;

    hDC = GetWindowDC(hWnd);

    x1 = r.left;
    y1 = r.top;
    x2 = r.right-2;
    y2 = r.bottom-2;		  

    if (w->title && w->title[0])
      y1 += 4;
    boxPen = CreatePen(PS_SOLID, 1, RGB(0xff,0xff,0xff));
    oldPen = SelectObject(hDC,boxPen);
    MoveTo(hDC,x1,y1);
    LineTo(hDC,x2-1,y1);
    LineTo(hDC,x2-1,y2-1);	/* draw first rectangle white */
    LineTo(hDC,x1,y2-1);
    LineTo(hDC,x1,y1);
    SelectObject(hDC,oldPen);
    DeleteObject(boxPen);
    boxPen = CreatePen(PS_SOLID,1,RGB(0x40,0x40,0x40));
    oldPen = SelectObject(hDC, boxPen);
    MoveTo(hDC,x2,y1+1);
    LineTo(hDC,x2,y2);
    LineTo(hDC,x1-1,y2);
    MoveTo(hDC,x1+1,y2-2);
    LineTo(hDC,x1+1,y1+1);
    LineTo(hDC,x2-2,y1+1);
    SelectObject(hDC,oldPen);
    DeleteObject(boxPen);

    if (w->title && w->title[0])
    {
      int    textWidth;
      int    tPosX,tPosY;
      HBRUSH textBrush,oldBrush;

      tPosX = x1+8;
      tPosY = y1;
      textWidth = LOWORD(_GetTextExtent(w->title));

      boxPen = CreatePen(PS_SOLID,1,RGB(0x80,0x80,0x80));
      oldPen = SelectObject(hDC,boxPen);

      MoveTo(hDC,tPosX-2,tPosY);
      LineTo(hDC,tPosX+textWidth+1,tPosY);
      MoveTo(hDC,tPosX-2,tPosY+1);
      LineTo(hDC,tPosX+textWidth+1,tPosY+1);

      SelectObject(hDC,oldPen);
      DeleteObject(boxPen);

      textBrush=CreateSolidBrush(RGB(0x80,0x80,0x80));
      oldBrush = SelectObject(hDC,textBrush);
      SetBkMode(hDC,TRANSPARENT);

      TextOut(hDC,tPosX+1,0,w->title,lstrlen(w->title));

      SetBkMode(hDC,OPAQUE);
      SelectObject(hDC, oldBrush);
      DeleteObject(textBrush);
    }

    ReleaseDC(hWnd, hDC);
    }

#else	  	

    hDC = GetWindowDC(hWnd);

    /*
      The top of the frame is drawn at the midpoint of the title.
    */
    if (w->title && w->title[0])
    {
      GetTextMetrics(hDC, &tm);
      r.top += (tm.tmHeight + tm.tmExternalLeading) / 2;
    }

    /*
      Draw the frame.
    */
    if (SysGDIInfo.fFlags & GDISTATE_NO_BEVELS)
    {
      HPEN hOldPen = SelectObject(hDC, SysPen[SYSPEN_WINDOWFRAME]);
      HBRUSH hOldBr = SelectObject(hDC, GetStockObject(NULL_BRUSH));
      Rectangle(hDC, r.left, r.top, r.right, r.bottom);
      SelectObject(hDC, hOldPen);
      SelectObject(hDC, hOldBr);
    }
    else
      DrawBeveledBox(hWnd,hDC,(LPRECT)&r,1,1,FALSE,GetStockObject(NULL_BRUSH));

    /*
      Draw the title starting a little to the right of the upper-left corner
    */
    if (w->title && w->title[0])
    {
      HBRUSH hBr;
      hBr = (HBRUSH) SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WPARAM) hDC,
                           MAKELONG(hWnd, CTLCOLOR_STATIC));
      SelectObject(hDC, hBr);
      TextOut(hDC, tm.tmAveCharWidth, 0, w->title, lstrlen(w->title));
    }

    ReleaseDC(hWnd, hDC);

#endif /* ZAPP */

  }
#endif /* MOTIF */


#else

  COLOR  attr;
  HWND   hParent;
  WINDOW *wParent;

  /*
    The background of the frame is the foreground of the parent.
  */
  hParent = GetParentOrDT(hWnd);
  wParent = WID_TO_WIN(hParent);
  if (wParent->attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hParent) & 0xF0;
  else
    attr = wParent->attr & 0xF0;

  /*
    Determine the foreground color of the rectangle
  */
  if      (dwFlags == (SS_BLACKFRAME & 0x0FL))
    attr |= GET_FOREGROUND(WinQuerySysColor(0, SYSCLR_WINDOWFRAME));
  else if (dwFlags == (SS_WHITEFRAME & 0x0FL))
    attr |= GET_BACKGROUND(WinQuerySysColor(0, SYSCLR_WINDOW));
  else if (dwFlags == (SS_GRAYFRAME & 0x0FL))
    attr |= GET_BACKGROUND(WinQuerySysColor(0, SYSCLR_BACKGROUND));
  else  /* groupbox or regular bordered static */
  {
    if (w->attr == SYSTEM_COLOR)
      attr |= (WinGetClassBrush(hWnd) & 0x0F);
    else
      attr = w->attr;
  }

  if (dwFlags >= (SS_BLACKFRAME & 0x0FL) && 
      dwFlags <= (SS_WHITEFRAME & 0x0FL))
  {
    /*
      If there is a title, then temporarily remove it.
    */
    LPSTR pszSaveTitle = w->title;
    /*
      KLUDGE here and an inconsistancy. BS_GROUPBOX in MEWEL is the
      same thing as SS_BLACKFRAME. So, assume that SS_BLACKFRAMEs are
      GROUPBOXes and print the damn title!
    */
    if (dwFlags != (SS_BLACKFRAME & 0x0FL))
      w->title = NULL;
    WinDrawFrame(hWnd, attr, 0);
    w->title = pszSaveTitle;
  }
  else   /* not a SS_BLACK/WHITE/GRAYFRAME, a regular groupbox */
  {
    WinDrawFrame(hWnd, attr, 0);
  }

#endif /* MEWEL_GUI */
}


/*===========================================================================*/
/*                                                                           */
/* Function: StaticWinProc()                                                 */
/*                                                                           */
/* Purpose : Winproc for the static class                                    */
/*                                                                           */
/*===========================================================================*/
LRESULT FAR PASCAL StaticWinProc(hWnd, message, wParam, lParam)
  HWND   hWnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  WINDOW *w;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
    return -1;

  switch (message)
  {
#if !defined(USE_NATIVE_GUI)
    case WM_NCHITTEST   :
      /*
        If the mouse wasn't on the frame's border, then the frame is 
        "transparent".
      */
#ifdef TELEVOICE
      /*
        Televoice wants all statics to be transparent
      */
      return HTTRANSPARENT;
#else
      if (IsFrameClass(w))
      {
        int row, col;
        row = HIWORD(lParam);
        col = LOWORD(lParam);
        if (row != w->rect.top  && row != w->rect.bottom-1 &&
            col != w->rect.left && col != w->rect.right-1)
          return HTTRANSPARENT;
      }
      return HTNOWHERE;
#endif


    case WM_NCPAINT  :
      if (!(w->flags & WS_BORDER))
        break;
      /* fall through */
    case WM_PAINT    :
      StaticDraw(hWnd);
      w->rUpdate = RectEmpty;
      w->ulStyle &= ~WIN_UPDATE_NCAREA;
      break;
#endif /* NATIVE_GUI */
      

    case WM_BORDER     :
      /*
        We need to draw frames in a special way. They need to have the
        background color of their parent window.
      */
      if (IsFrameClass(w))
      {
        StaticDrawBorder(hWnd);
        return TRUE;
      }
      else  /* not a frame */
        goto call_dwp;


    case WM_GETDLGCODE :
      return DLGC_STATIC;

    case STM_GETICON :
      return w->hIcon;

    case STM_SETICON :
    {
      HICON hOldIcon = w->hIcon;
      w->hIcon = wParam;
      InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
      return hOldIcon;
    }

    case WM_DESTROY :
      if (w->hIcon)
      {
        DestroyIcon(w->hIcon);
        w->hIcon = NULL;
      }
      break;

    default :
call_dwp:
      return StdWindowWinProc(hWnd, message, wParam, lParam);
  }
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : IsFrameClass(w)                                               */
/*                                                                          */
/* Purpose  : Internal function which tests a window to see if it is a      */
/*            frame-style window. This test is required by various          */
/*            visibility calculation routines in order to see if underlying */
/*            windows are visible.                                          */
/*                                                                          */
/* Returns  : TRUE if the window is a frame class, false if not.            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL IsFrameClass(w)
  WINDOW *w;
{
  INT   idClass;
  DWORD dwStyle;

  /*
    A FRAME window?
  */
  if ((idClass = _WinGetLowestClass(w->idClass)) == FRAME_CLASS)
    return TRUE;

  dwStyle = w->flags;

  /*
    Test the various styles of STATIC
  */
  if (idClass == STATIC_CLASS && (dwStyle & SS_FRAME))
    return TRUE;

  /*
    A GROUPBOX window?
  */
  if (idClass == BUTTON_CLASS && (dwStyle & BS_GROUPBOX))
    return TRUE;

  /*
    Not a frame window
  */
  return FALSE;
}

