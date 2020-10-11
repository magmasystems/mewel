/*===========================================================================*/
/*                                                                           */
/* File    : WTABTEXT.C                                                      */
/*                                                                           */
/* Purpose : Contains the TabbedTextOut() & GetTabbedTextExtent() functions  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static LONG PASCAL _TabbedTextOut(HDC,INT,INT,LPCSTR,INT,INT,LPINT,INT,BOOL);


static LONG PASCAL _TabbedTextOut(
  HDC    hDC,
  INT    X, INT Y,
  LPCSTR lpStr,
  INT    nChars,
  INT    nTabStops,
  LPINT  lpnTabs,
  INT    nTabOrigin,
  BOOL   bOutput)
{
  TEXTMETRIC tm;
  LPHDC      lphDC;
  HBRUSH     hBrushBk = 0;   /* used for drawing blank space */
  RECT       r;
  LPSTR      lpSub;          /* start of substring to dump */
  UINT       wAlign;
  INT        nTabIncr;
  INT        idxTab = 0;     /* current index into the tab array */
  INT        cyHeight;
  INT        cxWidth,
             cxSubWidth,     /* pixel width of substring */
             cxOffset;
  BYTE       ch, chSave;

  /*
    hDC can be NULL if we are doing a fast GetTabbedTextExtent (such
    as doing an index-to-column calculation for an edit control).
  */
  if ((lphDC = _GetDC(hDC)) == NULL && bOutput)
    return FALSE;

  GetTextMetrics(hDC, &tm);

  if (nTabStops == 0 && lpnTabs == NULL)
  {
    nTabStops = 1;
    nTabIncr = 8 * tm.tmAveCharWidth;
  }
  else if (nTabStops == 1)
  {
    nTabIncr = *lpnTabs;
  }
  else
  {
    nTabIncr = -1;
  }
  if (nTabIncr == 0)
    nTabIncr = 1;


  /*
    Adjust the starting position of the text as-per the text alignment 
    which is stored in the passed DC.
  */
  if (bOutput)
  {
    wAlign = lphDC->wAlignment;
    /*
      If TA_UPDATECP, then use the current position stored in the DC
    */
    if (wAlign & TA_UPDATECP)
    {
      X = lphDC->ptPen.x;
      Y = lphDC->ptPen.y;
    }
  }

  cyHeight = tm.tmHeight;  /* + tm.tmExternalLeading ??? */
  cxSubWidth = 0;
  cxWidth = cxOffset = X - nTabOrigin;

  /*
    If we are writing to the left of the physical screen, then start
    with width off at 0.
  */
  if (cxWidth < 0)
    cxWidth = cxOffset = 0;

#if 0
  /*
    Convert logical to screen coords
  */
  if (bOutput)
  {
    POINT pt;
    pt.x = X;
    pt.y = Y;
    LPtoSP(hDC, (LPPOINT) &pt, 1);
    X = pt.x;
    Y = pt.y;
  }
#endif


  for (lpSub = (LPSTR) lpStr;  nChars-- > 0 && (ch = *lpStr) != '\0';  lpStr++)
  {
    /*
      If the character is not a tab, just bump up the width of the
      entire string and of the substring.
    */
    if (ch != '\t')
    {
#if defined(MEWEL_GUI) || defined(MOTIF)
      INT w;
      GetCharWidth(hDC, ch, ch, &w);
      cxSubWidth += w;
#else
      cxSubWidth++;
#endif
      continue;
    }

    /*
      Dump the substring up until the tab.
    */
    if (bOutput)
    {
      chSave = *lpStr;
      * (LPSTR) lpStr = '\0';
      TextOut(hDC, X, Y, lpSub, lpStr - lpSub);
      * (LPSTR) lpStr = chSave;
    }
    cxWidth += cxSubWidth;
    X += cxSubWidth;

    /*
      Figure out the width of the tabstop. Put it into cxSubWidth.
    */
    if (nTabIncr == -1)
    {
      /*
        We have an array of tabstops.
      */
      INT  i;

      /*
        Search the tab array for the first tab stop which is greater
        than or equal to the current string extent.
      */
      for (i = idxTab;  i < nTabStops;  i++)
        if (lpnTabs[i] >= cxWidth)
          break;

      /*
        If we have run out of tab stops, use a tab of 1
      */
      if (i >= nTabStops)
      {
        nTabIncr = 1;
        cxSubWidth = 1;
      }
      else
      {
        /*
          Get the number of spaces
        */
        cxSubWidth = lpnTabs[i] - cxWidth;

        /*
          Advance the current index into the tab array to the proper
          position.
        */
        idxTab = i+1;
      }
    }
    else
    {
      /*
        Starting at the current X position, figure out the next highest
        multiple of nTabIncr.
      */
      cxSubWidth = (cxWidth+nTabIncr) - ((cxWidth+nTabIncr)%nTabIncr) - cxWidth;
    }


    /*
      Output the blank spaces
      Use a brush which is set to the background color to draw the blanks.
    */
    if (bOutput && lphDC->wBackgroundMode == OPAQUE)
    {
      if (hBrushBk == NULL)
        hBrushBk = CreateSolidBrush(GetBkColor(hDC));
      SetRect(&r, X, Y, X+cxSubWidth, Y+cyHeight);
      FillRect(hDC, &r, hBrushBk);
    }

    /*
      Bump up the width of the entire string and of X. Start a new substring.
    */
    cxWidth += cxSubWidth;
    X += cxSubWidth;
    cxSubWidth = 0;
    lpSub = (LPSTR) lpStr + 1;
  }

  /*
    Dump the remaining portion of the string
  */
  if (lpStr - lpSub > 0)
  {
    if (bOutput)
    {
      chSave = *lpStr;
      * (LPSTR) lpStr = '\0';
      TextOut(hDC, X, Y, lpSub, lpStr - lpSub);
      * (LPSTR) lpStr = chSave;
    }
    cxWidth += cxSubWidth;
  }

  /*
    Get rid of any objects we created
  */
  if (hBrushBk)
    DeleteObject(hBrushBk);

  /*
    Update the current position
  */
  if (bOutput && (wAlign & TA_UPDATECP))
    lphDC->ptPen.x += cxWidth;

  /*
    Return the height and width
  */
  cxWidth -= cxOffset;
  return MAKELONG(cxWidth, cyHeight);
}


LONG FAR PASCAL TabbedTextOut(hDC, X, Y, lpStr, nChars, 
                           nTabStops, lpnTabs, nTabOrigin)
  HDC    hDC;
  INT    X, Y;
  LPCSTR lpStr;
  INT    nChars;
  INT    nTabStops;
  LPINT  lpnTabs;
  INT    nTabOrigin;
{
  return _TabbedTextOut(hDC, X, Y, lpStr, nChars, 
                       nTabStops, lpnTabs, nTabOrigin, TRUE);
}

DWORD FAR PASCAL GetTabbedTextExtent(hDC, lpString, nChars, nTabStops, lpnTabs)
  HDC    hDC;
  LPCSTR lpString;
  INT    nChars;
  INT    nTabStops;
  LPINT  lpnTabs;
{
  return _TabbedTextOut(hDC, 0, 0, lpString, nChars, 
                       nTabStops, lpnTabs, 0, FALSE);
}

