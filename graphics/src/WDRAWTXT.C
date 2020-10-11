#include "wprivate.h"
#include "window.h"


/********************************************************************\
*                                                                    *
* TEMPLATE:     <^func^>                                             *
*                                                                    *
* NAME:         DrawText                                             *
* DESCRIPTION:  A MEWEL based version of the Windows function.       *
*               Implementation is on a feature by feature basis, as  *
*               needed.                                              *
* PARAMETERS:   hdc:          handle to the device context.          *
*               lpszString:   pointer to the string to output.       *
*               nCount:       length of the string.                  *
*               pRect:        pointer to the RECT structure.         *
*               wFormat:      format flags for output style.         *
* RETURNS:      void.                                                *
* NOTES:        none.                                                *
* SIDE EFFECTS: none.                                                *
*                                                                    *
\********************************************************************/
int FAR PASCAL DrawText(hdc, lpszString, nCount, lpRect, wFormat)
  HDC    hdc;
  LPCSTR lpszString;
  int    nCount;
  LPRECT lpRect;
  UINT   wFormat;
{
  INT             acnWidth[256];
  INT             cx,
                  cy,
                  cnyMax,
                  cnxMax,
                  cnxMaxLineWidth,
                  cnyLine,
                  cnyTotal,
                  cnxLine,
                  cnxTotal,
                  nTabSize,
                  nLineBegin;
  TEXTMETRIC      tm;
  int             i;
  LPHDC           lphdc;
  PSTR            pcBuf;


  if ((lphdc = _GetDC(hdc)) == NULL)
    return 0;

  /*
   * Disallow DT_WORDBREAK if DT_SINGLELINE was specified.
   */
  if (wFormat & DT_SINGLELINE)
    wFormat &= ~DT_WORDBREAK;

  /*
   * Get the tab expansion width and turn off the high byte of
   * wFormat so other flags will not be mistakenly processed.
   */
  if (wFormat & DT_EXPANDTABS)
  {
    if (wFormat & DT_TABSTOP)
    {
      nTabSize = HIBYTE(wFormat) ? HIBYTE(wFormat) : 1;
      wFormat &= 0x00FF;
    }
    else
      nTabSize = 8;
  }
  else
    nTabSize = 1;


  /*
   * Set width and height of the given rectangle.
   */
  cnxMax = lpRect->right  - lpRect->left;
  cnyMax = lpRect->bottom - lpRect->top;
  cnxTotal = 0;
  cnyTotal = 0;

  /*
    For DT_CALCRECT, we need to find the longest line
  */
  cnxMaxLineWidth = 0;

  /*
    Get the height of the font and the width of each character.
    Note : When using a proportional font, the GetCharWidth function
    can take a long-ish amount of time.
  */
  GetTextMetrics(hdc, (LPTEXTMETRIC) &tm);
  GetCharWidth(hdc, 0, 255, acnWidth);

  /*
   * Establish the vertical starting coordinate and dimension.
   */
  cnyLine = tm.tmHeight;
  if (wFormat & DT_EXTERNALLEADING)
    cnyLine += tm.tmExternalLeading;

  cy = lpRect->top;
  if (wFormat & DT_SINGLELINE)
  {
    if (wFormat & DT_BOTTOM)
      cy += cnyMax - cnyLine;
    else if (wFormat & DT_VCENTER)
      cy += (cnyMax - cnyLine) / 2;
  }

  /*
   * Copy the string to a local buffer, formatting in-process.
   */
  if (nCount == -1)
    nCount = lstrlen((LPSTR) lpszString);

  if ((pcBuf = emalloc(nCount + 8)) == NULL)
    return (0);

  cnxLine = 0;
  for (i = nLineBegin = 0;  i <= nCount;  i++, lpszString++)
  {
    /*
     * Copy the current character.
     */
    pcBuf[i] = *lpszString;
    if (pcBuf[i])
      cnxLine += acnWidth[pcBuf[i]];

    if (pcBuf[i] == '\t')
    {
      int cnTabWidth = GetTabDistance(i - nLineBegin, nTabSize);

      /*
       * If the tab size is greater than one then expand the local
       * buffer and add extra spaces.
       */
      if (cnTabWidth > 0)
      {
        nCount += cnTabWidth;
        if ((pcBuf = (PSTR) realloc(pcBuf, nCount + 1)) == NULL)
          return (0);
        memset(&pcBuf[i], ' ', cnTabWidth + 1);
        i += cnTabWidth;
        cnxLine += cnTabWidth * acnWidth[' '];
      }
      else
        pcBuf[i] = ' ';

    }
    else if ((wFormat & DT_WORDBREAK) && pcBuf[i] == ' ')
    {
      LPCSTR lpc;
      int   x;

      /*
       * Look ahead to see if the maximum length will be reached
       * before the next space, newline or null character.
       */
      lpc = lpszString + sizeof(char);

      for (x = 1; *lpc && strchr(" \t", *lpc); lpc++)
      {
        if (*lpc == '\t')
          x += GetTabDistance(i + x - nLineBegin, nTabSize);
      }

      for (  ;  *lpc && !strchr(" \t\r\n", *lpc);  x += acnWidth[*lpc], lpc++)
        ;

      if (cnxLine + x > cnxMax)
        pcBuf[i] = '\n';
    }

    if (pcBuf[i] == '\n' || pcBuf[i] == '\r')
    {
      /*
       * Terminate the line or translate newline to space.
       */
      cnxLine -= acnWidth[pcBuf[i]];
      if (wFormat & DT_SINGLELINE)
      {
        pcBuf[i] = ' ';
        cnxLine += acnWidth[' '];
      }
      else
        pcBuf[i] = 0;

      /*
        Get the max line width we've seen so far
      */
      cnxMaxLineWidth = max(cnxMaxLineWidth, cnxLine);

      /*
        6/1/91 (maa) - don't print a blank line if we have a \n following
      */
      if (*lpszString == '\r' && lpszString[1] == '\n')
      {
        lpszString++;
        pcBuf[++i] = '\0';
      }
    }
    else if (pcBuf[i] == '&')
    {
      /*
        If we are drawing a string with an embedded '&', then turn it
        into a tilde
      */
      pcBuf[i] = HILITE_PREFIX;
    }

    if (pcBuf[i] == '\0' || i >= nCount)
    {
      /*
       * Establish the horizontal starting coordinate and dimension.
       */
      cx = lpRect->left;
      if (wFormat & DT_RIGHT)
        cx += cnxMax - cnxLine;
      else if (wFormat & DT_CENTER)
        cx += (cnxMax - cnxLine) / 2;

      if (cnxLine > cnxTotal)
        cnxTotal = cnxLine;

      cnyTotal += cnyLine;

      /*
       * Output the string.
       */
      if (!(wFormat & DT_CALCRECT) && cy >= 0)
      {
#if defined(MOTIF) || defined(DECWINDOWS) || defined(MEWEL_GUI)
        (void) lphdc;
        TextOut(hdc, cx, cy, &pcBuf[nLineBegin], strlen(&pcBuf[nLineBegin]));
#else
        _WinPuts(lphdc->hWnd,hdc,cy,cx,&pcBuf[nLineBegin],lphdc->attr,-1,TRUE);
#endif
      }

      /*
       * Prepare for a subsequent line.
       */
      if (i < nCount)
      {
        nLineBegin = i + 1;
        cnxLine = 0;
        cy += cnyLine;
      }
    } /* end if (pcBuf[i] == 0 || i == nCount || pcBuf[i] == '\n') */
  } /* end for (i...) */

  /*
   * Free the local text buffer.
   */
  MyFree(pcBuf);

  /*
   * Resize the passed rectangle to bound the text.
   */
  if (wFormat & DT_CALCRECT)
  {
    if (cnyTotal == cnyLine)
      lpRect->right  = lpRect->left + (MWCOORD) cnxTotal;
    else
    {
      cnxMaxLineWidth = max(cnxMaxLineWidth, cnxMax);
      lpRect->right  = lpRect->left + (MWCOORD) cnxMaxLineWidth;
    }
    lpRect->bottom = lpRect->top + (MWCOORD) cnyTotal;
  }

  /*
   * Return the total height of the text.
   */
  return (cnyTotal);
}
