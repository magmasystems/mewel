/*===========================================================================*/
/*                                                                           */
/* File    : WGDITEXT.C                                                      */
/*                                                                           */
/* Purpose : Contains the graphics version of TextOut()                      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#define INCLUDE_MOUSE  

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#define USE_XVERTEXT
#if defined(XWINDOWS) && defined(USE_XVERTEXT)
#include "rotated.h"
#endif


/*
  Merge together the various text alignment flags from the different engines
*/
#if defined(META)
#define LEFT_TEXT     alignLeft
#define CENTER_TEXT   alignCenter
#define RIGHT_TEXT    alignRight
#define TOP_TEXT      alignTop
#define BOTTOM_TEXT   alignBaseline
#elif defined(GX)
#if defined(USE_GX_TEXT)
#define LEFT_TEXT     txLEFT
#define CENTER_TEXT   txCENTER
#define RIGHT_TEXT    txRIGHT
#define TOP_TEXT      txTOP
#define BOTTOM_TEXT   txBOTTOM
#else
#define LEFT_TEXT     grTLEFT
#define CENTER_TEXT   grTCENTER
#define RIGHT_TEXT    grTRIGHT
#define TOP_TEXT      grTTOP
#define BOTTOM_TEXT   grTBOTTOM
#endif
#elif !defined(BGI)
#define LEFT_TEXT     0
#define CENTER_TEXT   1
#define RIGHT_TEXT    2
#define TOP_TEXT      0
#define BOTTOM_TEXT   1
#endif


BOOL  FAR PASCAL _GTextOut(HDC hDC, INT x, INT y, PSTR pText, int cchText);
static VOID PASCAL EngineTextOut(PSTR, INT, INT, INT, INT, INT *);
static VOID PASCAL EngineFillBackground(LPRECT, COLOR);

/****************************************************************************/
/*                                                                          */
/* Function : _GTextOut()                                                   */
/*                                                                          */
/* Purpose  : Outputs a strings in graphics mode starting at point <x,y>.   */
/*            It uses the current font, current text and background color,  */
/*            current transparency mode, and current text alignment.        */
/*                                                                          */
/* Returns  : TRUE if the string was written, FALSE if not.                 */
/*                                                                          */
/* Note     : Transparency is not implemented in the underlying graphics    */
/*            engines.                                                      */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL _GTextOut(HDC hDC, INT x, INT y, PSTR pText, int cchText)
{
  TEXTMETRIC tm;
  int   aiHotPos[32];
  PSTR  pSave;
  BYTE  chSave;
  LPHDC lphDC;
  POINT pt;
  UINT  wAlign;
  DWORD dwSize;
  int   hJust = 0, vJust = 0;
  int   fg, bg;
  int   nPrefix;
  int   cyHeight;
  int   cxWidth;
  int   ySaveTop;
  RECT  r;


  /*
    Set up the viewport
  */
  if ((lphDC = GDISetup(hDC)) == NULL)
    return 0;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  /*
    See if we have any characters to write
  */
  if (!cchText || !pText)
    return TRUE;

#if defined(XWINDOWS)
  if (lphDC->gc == NULL)
    return 0;
#endif


#if !92192
  /*
    Set the current font
    Note (9/21)
      Let the SelectObject(hDC, hFont) handle the font realization. Of course,
    this breaks if we are using two separate DC's, each one with a different
    font selected into it, but let's ignore that for now.
  */
  RealizeFont(hDC);
#endif

  /*
    Get the text metrics for this font
  */
  GetTextMetrics(hDC, &tm);

  /*
     Save the cchText'th character, write the string, and restore the char.
  */
  pSave = pText + cchText;
  if ((chSave = *pSave) != '\0')
    *pSave = '\0';

  pText    = _GExpandTabs(pText, (INT *) &cchText, aiHotPos, &nPrefix);
#if defined(XWINDOWS)
  dwSize   = GetTextExtent(hDC, pText, cchText);
#else
  dwSize   = _GetTextExtent(pText);
#endif
  cxWidth  = LOWORD(dwSize);
  cyHeight = HIWORD(dwSize);

  /*
    Convert the current RGB color to a DOS attribute
  */
  fg = RGBtoAttr(hDC, lphDC->clrText);
  bg = RGBtoAttr(hDC, lphDC->clrBackground);

  /*
    Get the current text alignment
  */
  wAlign = lphDC->wAlignment;


  /*
    Adjust the starting x coordinate if centered or right justified.
  */
  if (wAlign & TA_CENTER)
    hJust = CENTER_TEXT;
  else if (wAlign & TA_RIGHT)
    hJust = RIGHT_TEXT;
  else
    hJust = LEFT_TEXT;

  /*
    Note : TA_BASELINE is not implemented
  */
  if (wAlign & TA_BOTTOM)
    vJust = BOTTOM_TEXT;
  else
    vJust = TOP_TEXT;


  /*
    Update the current position if the string is right or left justified
  */
  if (wAlign & TA_UPDATECP)
  {
    /*
      If TA_UPDATECP, then use the current position stored in the DC
    */
    x = lphDC->ptPen.x;
    y = lphDC->ptPen.y;
    if (wAlign & TA_RIGHT)            /* right */
      lphDC->ptPen.x -= (MWCOORD) cxWidth;
    else if (!(wAlign & TA_CENTER))   /* left */
      lphDC->ptPen.x += (MWCOORD) cxWidth;
  }


  /*
    Convert the logical points to screen points
  */
  pt.x = (MWCOORD) x;  pt.y = (MWCOORD) y;
  GrLPtoSP(hDC, (LPPOINT) &pt, 1);
  _XLogToPhys(lphDC, (MWCOORD *) &pt.x, (MWCOORD *) &pt.y);

  /*
    Borland's BGI cannot print a line of text if the y origin
    of the text lies partially above the clipping region. So,
    we need to manually adjust the clipping region so that
    it begins on a y coordinate which is a multiple of the
    character height. For instance, if the clipping region
    begins on pixel 418, and the character height is 16,
    then we need to adjust the clipping region so it starts
    at 416.
  */
  ySaveTop  = lphDC->rClipping.top;

  /*
    If the y coordinate is slightly negative and is less than the height
    of a character and if adjusting the clipping region will not overwrite
    the top of the window, then adjust the clipping region slightly.
  */
  if (pt.y < 0 && abs(pt.y) < cyHeight && 
      lphDC->rClipping.top > lphDC->ptOrg.y)
  {
#if defined(BGI)
    if (SysGDIInfo.CurrFontID == DEFAULT_FONT)
    {
      lphDC->rClipping.top += pt.y;
      pt.y = 0;
    }
#else
    ;
#endif
  }


  /*
    Set the viewport to the clipping rectangle
  */
  if (!_GraphicsSetViewport(hDC))
    return FALSE;


  /*
    Set the text alignment
  */
#if defined(META)
#if 32493
  mwTextAlign(LEFT_TEXT, TOP_TEXT);
#else
  mwTextAlign(hJust, vJust);
#endif

#elif defined(GX)
#if 0
  grSetTextJustify(hJust, vJust);
#else
#if defined(USE_GX_TEXT)
  txSetAlign(hJust, txTOP);
#else
  grSetTextJustify(hJust, TOP_TEXT);
#endif
#endif

#elif defined(BGI)
#if 0
  /*
    12/18/92 (maa)
    Commented this out because all of the various OffsetRect's below
    set the actual origin for the justified text.
  */
  settextjustify(hJust, vJust);
#else
  (void) hJust;    (void) vJust;
#endif
#endif


#if defined(GX)
#if defined(USE_GX_TEXT)
  txSetFace(txGetFace() | txTRANS);
#else
  grSetTextStyle(GX_DEFAULT_FONT, grTRANS);
#endif
#endif


  /*
    Create a rectangle 'r' which has the viewport-relative coordinates
    which the text should be drawn at.
  */
  SetRect((LPRECT) &r, pt.x, pt.y, pt.x + cxWidth, pt.y + cyHeight);
  /*
    Adjust the rectangle if we have a non-left and non-top alignment
  */
  if (wAlign & TA_CENTER)
    OffsetRect((LPRECT) &r, -cxWidth / 2, 0);
  else if (wAlign & TA_RIGHT)
    OffsetRect((LPRECT) &r, -(cxWidth-1), 0);
  if (wAlign & TA_BOTTOM)
    OffsetRect((LPRECT) &r, 0, -(cyHeight-1));

  /*
    Expand the viewport to include the underline
  */
  if (nPrefix)
    r.bottom++;

  /*
    Hide the mouse
  */
  MOUSE_ConditionalOffDC(lphDC, r.left, r.top, r.right, r.bottom);

#ifdef USE_REGIONS
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hRgnVis)
  {
    RECT    r3, rTmp;
    PREGION pRegion;
    INT     i;

    /*
      Convert the viewport-based rectangle to a screen-based rectangle
      because all of the rectangles in the region are in screen coords.
    */
    OffsetRect(&r, lphDC->rClipping.left, lphDC->rClipping.top);
    if (!IntersectRect(&r, &r, &SysGDIInfo.rectLastClipping))
      goto end1;

    pRegion = (PREGION) LOCKREGION(lphDC->hRgnVis);

    /*
      Go through all of the visible regions in the window. For each region,
      see if the area which we are about to draw intersects with the region.
    */
    for (i = pRegion->nRects - 1;  i >= 0;  i--)
    {
      if (IntersectRect(&rTmp, &pRegion->rects[i], &r))
      {
        INT cxFromVP, cyFromVP;

        /*
          rTmp is expressed in screen coordinates
          Clip the text to the pjysical screen.
        */
        SetRect(&r3, rTmp.left, rTmp.top, 
                     SysGDIInfo.cxScreen-1, SysGDIInfo.cyScreen-1);
        if (!IntersectRect(&r3, &r3, &rTmp))
          continue;
        _setviewport(r3.left, r3.top, r3.right, r3.bottom);
#if defined(GX) && defined(USE_GX_TEXT)
        txSetViewPort(r3.left, r3.top, r3.right, r3.bottom);
        txSetClipRegion(r3.left, r3.top, r3.right, r3.bottom);
        txSetClipping(txCLIP);
#endif

        /*
          If we are writing to the left of the viewport, then we
          need to write with a negative viewport-relative coordinate.
        */
        cxFromVP = (pt.x + lphDC->rClipping.left) - r3.left;
        cyFromVP = (pt.y + lphDC->rClipping.top ) - r3.top;

        /*
          Get 'r3' back to viewport-relative coordinates.
        */
        r3 = r;
        OffsetRect(&r3, -rTmp.left, -rTmp.top);

        /*
          For a stroked font, if we are drawing the lower part of the font,
          then reset the clipping so that the y coordinate is negative.
          This way, the top half will be clipped off the viewport.
        */
        if (pt.y < 0)
          r3.top += pt.y;

        /*
          If we are writing in opaue mode, then fill the background with
          the background color.
        */
        if (!IS_PRTDC(lphDC))
          if (lphDC->wBackgroundMode == OPAQUE)
            EngineFillBackground(&r3, bg);
#if defined(META)
          else
            mwRasterOp(xREPx);
#endif

        /*
          Output the text in the foreground color.
        */
        _setcolor(fg);
#if defined(GX) && defined(USE_GX_TEXT)
        txSetColor(fg, 0);
#endif

        /*
          Check to see if we are starting the drawing to the left of
          the viewport.
          We do this trick with setting pt.x to a negative value when
          we write horizontally scrolled listbox strings.
        */
        if (cxFromVP < 0 && r3.left >= 0)
          r3.left += cxFromVP;
        if (cyFromVP < 0 && r3.top  >= 0)
          r3.top  += cyFromVP;

        EngineTextOut(pText, r3.left, r3.top, cyHeight, nPrefix, aiHotPos);
      }
    }

    UNLOCKREGION(lphDC->hRgnVis);
    _setviewport(SysGDIInfo.rectLastClipping.left, 
                 SysGDIInfo.rectLastClipping.top, 
                 SysGDIInfo.rectLastClipping.right, 
                 SysGDIInfo.rectLastClipping.bottom);
#if defined(GX) && defined(USE_GX_TEXT)
    txSetViewPort(SysGDIInfo.rectLastClipping.left, 
                  SysGDIInfo.rectLastClipping.top, 
                  SysGDIInfo.rectLastClipping.right, 
                  SysGDIInfo.rectLastClipping.bottom);
    txSetClipRegion(SysGDIInfo.rectLastClipping.left, 
                    SysGDIInfo.rectLastClipping.top, 
                    SysGDIInfo.rectLastClipping.right, 
                    SysGDIInfo.rectLastClipping.bottom);
    txSetClipping(txCLIP);
#endif
  }
  else
#endif /* USE_REGIONS */
  {

#if !defined(XWINDOWS)
    if (!IS_PRTDC(lphDC))
      if (lphDC->wBackgroundMode == OPAQUE)
        EngineFillBackground(&r, bg);
#if defined(META)
      else
        mwRasterOp(xREPx);
#endif
#endif

#if defined(XWINDOWS)
    {
#if defined(USE_XVERTEXT)
    LPOBJECT lpObj = _ObjectDeref(lphDC->hFont);
    int      nEscapement = lpObj->uObject.uLogFont.lfEscapement;
    XFontStruct *xfs = (XFontStruct *) lpObj->lpExtra;
#endif

    /*
      XDrawString aligns the baseline of the characters with the origin.
      So, we need to increase y so that the top of the characters are
      at the origin.
    */
#if defined(USE_XVERTEXT)
    if (nEscapement && xfs)
    {
      /*
        Special cases for sideways text used by tabbed dialogs...
      */
      switch (nEscapement)
      {
        case 900  :
          r.left  += tm.tmAscent;
          break;
        case -900 :
          r.left  -= tm.tmAscent;
          break;
      }
    }
    else
#endif
    r.top += tm.tmAscent;

    XSetForeground(XSysParams.display, lphDC->gc, fg);
    if (lphDC->wBackgroundMode == OPAQUE)
    {
      XSetBackground(XSysParams.display, lphDC->gc, bg);
#if defined(USE_XVERTEXT)
      if (nEscapement && xfs)
      XRotDrawImageString(XSysParams.display,
                       xfs, ((float) nEscapement) / 10.0,
                       lphDC->drawable, lphDC->gc,
                       r.left, r.top, pText);
      else
#endif
      XDrawImageString(XSysParams.display, lphDC->drawable, lphDC->gc,
                       r.left, r.top, pText, cchText);
    }
    else
    {
#if defined(USE_XVERTEXT)
      if (nEscapement && xfs)
      XRotDrawString(XSysParams.display,
                       xfs, ((float) nEscapement) / 10.0,
                       lphDC->drawable, lphDC->gc,
                       r.left, r.top, pText);
      else
#endif
      XDrawString(XSysParams.display, lphDC->drawable, lphDC->gc,
                  r.left, r.top, pText, cchText);
    }

    if (tm.tmUnderlined)
    {
      XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc, 
                r.left, r.top+1, r.right, r.top+1);

    }

    if (tm.tmStruckOut)
    {
      int yLine = r.top - (tm.tmAscent >> 1);
      XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc, 
                r.left, yLine, r.right, yLine);
    }

    if (nPrefix)
    {
      while (nPrefix-- > 0)
      {
        BYTE chLetter[2];
        PSTR pszUnderline = pText + aiHotPos[nPrefix];
        INT  cxPrefix, x2;
        SIZE size;

        /*
          Find out the width of the character to be underlined, and
          find out the x coordinate of that character.
        */
        chLetter[0]   = *pszUnderline;
        chLetter[1]   = '\0';

        GetTextExtentPoint(hDC, chLetter, 1, &size);
        cxPrefix = size.cx;
        if (aiHotPos[nPrefix] == 0)
          x2 = r.left;
        else
        {
          GetTextExtentPoint(hDC, pText, aiHotPos[nPrefix]+1, &size);
          x2 = r.left + size.cx;
        }
        XDrawLine(XSysParams.display, lphDC->drawable, lphDC->gc, 
                  x2, r.top+1, x2+cxPrefix, r.top+1);
      }
    }
    }

#else /* !XWINDOWS */

#if defined(GX) && defined(USE_GX_TEXT)
    {
    RECT r3;
    r3 = lphDC->rClipping;
    txSetViewPort(r3.left, r3.top, r3.right, r3.bottom);
    txSetClipRegion(r3.left, r3.top, r3.right, r3.bottom);
    txSetClipping(txCLIP);
    txSetColor(fg, 0);
    }
#endif

    _setcolor(fg);
    EngineTextOut(pText,pt.x,pt.y,cyHeight,nPrefix,aiHotPos);

#if defined(GX) && defined(USE_GX_TEXT)
    txSetViewPort(SysGDIInfo.rectLastClipping.left, 
                  SysGDIInfo.rectLastClipping.top, 
                  SysGDIInfo.rectLastClipping.right, 
                  SysGDIInfo.rectLastClipping.bottom);
    txSetClipRegion(SysGDIInfo.rectLastClipping.left, 
                    SysGDIInfo.rectLastClipping.top, 
                    SysGDIInfo.rectLastClipping.right, 
                    SysGDIInfo.rectLastClipping.bottom);
    txSetClipping(txCLIP);
#endif
#endif
  }


end1:
  /*
    Restore the mouse.
  */
  MOUSE_ShowCursorDC();


  /*
    Restore the nulled-out character
  */
  if (chSave)
    *pSave = chSave;
  lphDC->rClipping.top  = (MWCOORD) ySaveTop;

#if defined(META)
  /*
    In Metawindows, if we have opaque text, reset the raster op
  */
  if (lphDC->wBackgroundMode == TRANSPARENT)
    RealizeROP2(hDC);
#endif

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : EngineTextOut()                                               */
/*                                                                          */
/* Purpose  : Graphics-engine specific text-out                             */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
#if !defined(XWINDOWS)
static VOID PASCAL EngineTextOut(pText, x, y, cyHeight, nPrefix, aiHotPos)
  PSTR pText;
  INT  x, y;
  INT  cyHeight;
  INT  nPrefix;
  INT  aiHotPos[];
{
  INT  x2, y2, n;

  /*
    BGI does not write the entire string if we start writing
    to the left of the viewport. This is only true for the
    bitmap fonts, not the stroked fonts.
  */
  outtextxy(x, y, pText);

  /*
    Underline each of the highlighted characters
  */
  for (n = nPrefix;  n-- > 0;  )
  {
    BYTE chLetter[2];
    BYTE chSave;
    PSTR pszUnderline = pText + aiHotPos[n];
    INT  cxPrefix;

    /*
      Find out the width of the character to be underlined, and
      find out the x coordinate of that character.
    */
    chLetter[0]   = *pszUnderline;
    chLetter[1]   = '\0';
    chSave        = *pszUnderline;
    *pszUnderline = '\0';

#if defined(GX) && !defined(USE_GX_TEXT)
    cxPrefix  = SysGDIInfo.tmAveCharWidth;
    x2 = x + lstrlen(pText) * SysGDIInfo.tmAveCharWidth;
#else
    cxPrefix = textwidth(chLetter);
    x2 = x + textwidth(pText);
#endif
    *pszUnderline = chSave;

    /*
      Determine the y coordinate where the underline will be drawn
    */
#if defined(META)
    y2 = y + cyHeight + 0;
#elif defined(GX)
    y2 = y + cyHeight - 1;
#elif defined(MSC)
    y2 = y + cyHeight + 0;
#else
    y2 = y + cyHeight + 1;
#endif

    /*
      Draw the underline
    */
    _moveto(x2, y2);
    _lineto(x2 + cxPrefix - 1, y2);
  }
}
#endif /* XW */


#if !defined(XWINDOWS)
static VOID PASCAL EngineFillBackground(lpRect, bg)
  LPRECT lpRect;
  COLOR  bg;
{
  /*
    Normalize the passed rectangle from Windows-type coords to the
    coords used by the graphics engines.
  */
#if !defined(META)
  lpRect->right--;
  lpRect->bottom--;
#endif

#if defined(META)
  mwRasterOp(zREPz);
  mwBackColor((bg == INTENSE(WHITE)) ? -1 : bg);
#elif defined(GX)
  grSetFillStyle(grFSOLID, bg, grOPAQUE);
  grDrawRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, grFILL);
  grSetBkColor(bg);
#elif defined(MSC)
  _setcolor(bg);
  _setfillmask("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
  _rectangle(_GFILLINTERIOR, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
#elif defined(BGI)
  setfillstyle(SOLID_FILL, bg);
  bar(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
#endif
}
#endif /* XW */


/****************************************************************************/
/*                                                                          */
/* Function : _GExpandTabs()                                                */
/*                                                                          */
/* Purpose  : Internal function which, given a string, detabs it and also   */
/*            checks for various "hotspot" points within the line (such as  */
/*            a string which represents a menubar).                         */
/*                                                                          */
/* Returns  : A pointer to the static buffer which contains the new string. */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL _GExpandTabs(lpTabbedText, pnChars, piHotPos, pnHotSpots)
  LPSTR lpTabbedText;
  INT   *pnChars;
  INT   *piHotPos;
  INT   *pnHotSpots;
{
  static PSTR buf = NULL;
  static INT  iBufSize = 0;

  PSTR pBuf;
  INT  nSpaces, nHotSpots;
  INT  nLen;
  BYTE ch;

  nHotSpots = 0;
  nLen = lstrlen(lpTabbedText);

  /*
    See if we have to allocate the initial scratch buffer or if we
    need to expand the size of the current buffer.
  */
  if (iBufSize < nLen-1 || buf == NULL)
  {
    /*
      Free the old buffer cause we have to alloc a new one
    */
    if (buf != NULL)
      MyFree(buf);

    /*
      Determine the new size of the temp buffer. 256 bytes for spaces
      plus 1 for the null byte.
    */
    iBufSize = nLen + 256 + 1;

    /*
      Alloc a new buffer
    */
    buf = emalloc(iBufSize);
    if (buf == NULL)
    {
      iBufSize = 0;
      return lpTabbedText;
    }
  }

  /*
    Go through the string and detab it.
  */
  pBuf = buf;
  while ((ch = *pBuf++ = *lpTabbedText++) != '\0')
  {
    /*
      We got a tab. Expand it to spaces.
    */
    if (ch == '\t')
    {
      pBuf--;
      nSpaces = 8 - ((pBuf - buf) & 0x07);
      memset(pBuf, ' ', nSpaces);
      pBuf += nSpaces;
    }
    /*
      If we have a 'hot letter', then record the hotspot position
      in the position array.
    */
    else if (ch == HILITE_PREFIX && *lpTabbedText != '\0')
    {
      pBuf--;
      if (piHotPos)
        *piHotPos++ = (pBuf - buf);
      nHotSpots++;
    }
  }

  *pnChars = (pBuf - buf - 1);
  *pnHotSpots = nHotSpots;
  return buf;
}

