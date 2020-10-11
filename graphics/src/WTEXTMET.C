/*===========================================================================*/
/*                                                                           */
/* File    : WTEXTMET.C                                                      */
/*                                                                           */
/* Purpose : Implements GetTextMetrics(), GetTextExtent(), and GetCharWidth()*/
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#include "wgraphic.h"
#endif

/*
  Hook to GUIs and graphics engines
*/
typedef int (PASCAL *TEXTMETRICSPROC)(HDC, LPTEXTMETRIC);
TEXTMETRICSPROC lpfnGetTextMetricsHook = (TEXTMETRICSPROC) 0;

TEXTMETRIC DefaultTextMetrics =
{
  /* lpMetrics->tmHeight           */ 1,
  /* lpMetrics->tmAscent           */ 1,
  /* lpMetrics->tmDescent          */ 0,
  /* lpMetrics->tmInternalLeading  */ 0,
  /* lpMetrics->tmExternalLeading  */ 0,
  /* lpMetrics->tmAveCharWidth     */ 1,
  /* lpMetrics->tmMaxCharWidth     */ 1,
  /* lpMetrics->tmWeight           */ 0,
  /* lpMetrics->tmItalic           */ 0,
  /* lpMetrics->tmUnderlined       */ 0,
  /* lpMetrics->tmStruckOut        */ 0,
  /* lpMetrics->tmFirstChar        */ 0x00,
  /* lpMetrics->tmLastChar         */ 0xFF,
  /* lpMetrics->tmDefaultChar      */ 0x20,
  /* lpMetrics->tmBreakChar        */ 0x0A,
  /* lpMetrics->tmPitchAndFamily   */ 0,
  /* lpMetrics->tmCharSet          */ 0,
  /* lpMetrics->tmOverhang         */ 0,
  /* lpMetrics->tmDigitizedAspectX */ 0,
  /* lpMetrics->tmDigitizedAspectY */ 0,
};


/*==========================================================================*/
/*                             GetTextMetrics                               */
/* Usage:      Same as MS-WINDOWS.                                          */
/* Input:      Same as MS-WINDOWS.                                          */
/* Output:     Same as MS-WINDOWS.                                          */
/* Desc:       This functions provides an MS-WINDOWS compatible function    */
/*             for use under MEWEL.                                         */
/* Modifies:   None                                                         */
/* Notes:      None                                                         */
/*==========================================================================*/
BOOL FAR PASCAL GetTextMetrics(hdc, lpMetrics)
  HDC hdc;
  LPTEXTMETRIC lpMetrics;
{
  if (!lpMetrics)
    return FALSE;

  /*
    Any hooks to a GUI?
  */
  if (lpfnGetTextMetricsHook)
    return (*lpfnGetTextMetricsHook)(hdc, lpMetrics);

  lmemcpy((LPSTR) lpMetrics, (LPSTR) &DefaultTextMetrics, sizeof(TEXTMETRIC));
  return TRUE;
}


BOOL FAR PASCAL GetTextExtentPoint(hDC, s, l, lpSize)
  HDC hDC;      /* display context handle (not used - for Windows compat.) */
  LPCSTR s;     /* far pointer to string whose extent is to be computed    */
  int l;        /* length of string s                                      */
  SIZE FAR *lpSize;
    /* 
     * Compute the extent (length/width) of a string based on the following 
     * rules:
     *
     *   1) both the length and width of the null string are 0.
     *   2) a '\n' adds 1 to the length and 0 to the width.
     *   3) the longest line determines the width.
     *   4) the last line is not terminated with a newline.
     *   5) tab characters add only 1 to the width (i.e. no tab expansions).
     *
     *   examples:
     *     "ABCDEF\nGHI\nLMNO" is of length 3 and width 6
     *     "A\n" is of length 2 and width 1 (note length of 2)
     *     "A\nBCDE" is of length 2 and width 4
     */
{
  LPCSTR p;                     /* string work pointer */
  int w = 0;                    /* width of string     */
  int h = (*s != '\0');         /* height of string    */
  int lw = 0;                   /* line width (reset for each line - '\n')*/

#if defined(MEWEL_GUI) || defined(XWINDOWS)
  int *aiWidth;
  TEXTMETRIC tm;

#if !11593 || defined(XWINDOWS)
  /*
    Although this is not technically correct, it is a lot faster than
    getting the char widths.
  */
  DWORD dw = _GetTextExtent(s);
  w = LOWORD(dw);
  h = HIWORD(dw);
  goto bye;
#else
  if ((aiWidth = (int *) emalloc(256 * sizeof(int))) != NULL)
    GetCharWidth(hDC, (UINT) 0, (UINT) 255, aiWidth);
  GetTextMetrics(hDC, &tm);
#endif
#endif

  (void) hDC;

  for (p = s; l--; p++)         /* for each character in string */
  {
    if (*p == '\n')             /* if end of line      */
    {
      if (lw > w) w = lw;       /* set new widest line */
      lw = 0;                   /* reset line width    */
      h++;                      /* add one more line to height */
    }
    else                        /* else not end of line */
    {
#ifdef MEWEL_GUI
      if (aiWidth)
        lw += aiWidth[*p];      /* bump current line width */
      else
        lw += VideoInfo.xFontWidth;
#else
      lw++;                     /* bump current line width */
#endif
    }
  }
  if (lw > w) w = lw;           /* if last line is longest line */

#ifdef MEWEL_GUI
  h *= tm.tmHeight;
  if (aiWidth)
    MyFree(aiWidth);
#endif

bye:
  lpSize->cx = w;
  lpSize->cy = h;
  return TRUE;
}


DWORD FAR PASCAL GetTextExtent(hDC, s, len)
  HDC   hDC;    /* display context handle (not used - for Windows compat.) */
  LPCSTR s;     /* far pointer to string whose extent is to be computed    */
  INT   len;    /* length of string s                                      */
{
  SIZE size;

  if (GetTextExtentPoint(hDC, s, len, &size))
    return (DWORD) MAKELONG(size.cx, size.cy);
  else
    return 0L;
}

BOOL FAR PASCAL GetCharWidth(hDC, wFirstChar, wLastChar, lpBuffer)
  HDC   hDC;
  UINT  wFirstChar, wLastChar;
  LPINT lpBuffer;
{
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  LPHDC lphDC;
  char s[2];

  (void) hDC;

#if 40993
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Allocate the font width cache for the first time....
  */
  if (SysGDIInfo.lpFontWidthCache == NULL)
    SysGDIInfo.lpFontWidthCache = (LPINT) emalloc_far_noquit(256 * sizeof(INT));

  /*
    Bug waiting to happen -
      What happens if we create a font, select it, get the widths,
    and delete the font, then do the same thing over again with
    another logical font. The two different fonts can have the same
    'hFont' value, and so, the refreshig of the font cache would
    not be done for the second cache.
  */

  /*
    See if we need to refresh the font cache. We do if the font
    we are looking at is different from the last font we looked
    at. We use a very simple test of matching the new and old font handles.
  */
  if (SysGDIInfo.hFontCached != lphDC->hFont && SysGDIInfo.lpFontWidthCache)
  {
    INT   ch;
    LPINT lp;

    /*
      Go through all 256 ansi characters and get the width of each, and
      stick the width in the entry in the font width cache.
    */
    s[1] = '\0';
    for (lp = SysGDIInfo.lpFontWidthCache, ch = 0;  ch < 256;  ch++)
    {
      INT nWidth;
      s[0] = (BYTE) ch;
      nWidth = textwidth(s);
#if defined(META)
      if (nWidth == 0xFF)  /* Guard against a character  */
        nWidth = 0;        /*   which is not in the font */
#endif
      *lp++ = nWidth;
    }

    /*
      Save the handle of the cached font.
    */
    SysGDIInfo.hFontCached = lphDC->hFont;
  }

  /*
    Copy the widths over to the passed buffer.
  */
  if (SysGDIInfo.lpFontWidthCache)
  {
    if (wFirstChar == wLastChar) /* simple assignment if looking at one char */
    {
      *lpBuffer = SysGDIInfo.lpFontWidthCache[wFirstChar];
    }
    else
    {
      LPINT lp = &SysGDIInfo.lpFontWidthCache[wFirstChar];
      while (wFirstChar++ <= wLastChar)
        *lpBuffer++ = *lp++;
    }
    return TRUE;
  }

#endif /* 40993 */

  /*
    There is no font cache. So just enumerate the characters.
  */
  s[1] = '\0';
  while (wFirstChar <= wLastChar)
  {
    s[0] = wFirstChar++;
    *lpBuffer++ = textwidth(s);
  }


#elif defined(MEWEL_TEXT)

  (void) hDC;

  while (wFirstChar++ <= wLastChar)
    *lpBuffer++ = 1;
#endif

  return TRUE;
}

