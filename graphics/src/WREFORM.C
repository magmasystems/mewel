/*===========================================================================*/
/*                                                                           */
/* File    : WREFORM.C                                                       */
/*                                                                           */
/* Purpose : Handles reformatting in edit controls                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "winedit.h"

#if defined(CH_WORDWRAP) && defined(EDIT_REFORMAT)
/*
   WORDWRAPPING ALGORITHM

   Loop until we reach the end of the text buffer.
     Loop until we reach the edge of the formatting rectangle
       If the character is a NEWLINE
         If the next character is not a WORDWRAP character, we are done.
         If the next character is a WORDWRAP character, delete the
           NEWLINE/WORDWRAP combo and insert a blank space in there
       else if the character is a blank
         record the position of the blank.
       else
         just move past the character.
      We are at the edge of the formatting rectangle. Move the pointer
        back to the last blank, get rid of the blank, and insert a
        NEWLINE/WORDWRAP combo there.


  Returns : The 0-based position in the buffer where the reformatting stopped.
            The only place where the return value is used is in
            _EditReformatAll().
*/

UINT FAR PASCAL _EditReformat(hEdit)
  HWND hEdit;
{
#if 52893 && defined(MEWEL_GUI)
  RECT   rInvalid;
  INT    nLinesDeleted = 0;
  INT    nLinesAdded = 0;
  INT    nLinesChanged;
#endif

  BUFFER *b;
  LPSTR pText;
  LPSTR pTextNl;
  int   iHeight;
  EPOS  iStartPos;
  EPOS  iCurrWidth;
  EPOS  iMaxWidth;
  LPSTR pLastBlankPos;
  BYTE  ch;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;
  if ((b->style & ES_AUTOHSCROLL) || !(b->style & ES_MULTILINE))
    return FALSE;

  /*
    Get the height and width of the buffer. Make sure we are dealing
    with a multiline edit field.
  */
  _EditGetDimensions(hEdit, (int *) &iHeight, (int *) &iMaxWidth);
  if (iHeight <= 1)
    return FALSE;

#if 0
  /*
    Wrap one char before the last pos in the window so that the window will
    not scroll horizontally.
  */
  iMaxWidth--;
#endif

  /*
    Find the beginning of the current line...
  */
  _EditIndexToLine(hEdit, b->iCurrPosition, &iStartPos);


#if 52893 && defined(MEWEL_GUI)
  GetClientRect(hEdit, (LPRECT) &rInvalid);
  rInvalid.top = b->cursor.row * (b->yFontHeight + b->yFontLeading);
#endif

  /*
    Get a ptr to the place to start scanning...
  */
  pText = b->szText + iStartPos;

  while (*pText)
  {
    pLastBlankPos = NULL;

    /*
      Should we really stop scanning for a blank at iMaxWidth?
      I think wrapping should occur even after a "long word".
      Since the original loop becomes confusing if you move the
      test for maxwidth inside, I split it up into two steps.
    */
    for (iCurrWidth=0, pTextNl = pText;  
         iCurrWidth < iMaxWidth && (ch = *pTextNl) != '\0';  
         iCurrWidth++, pTextNl++)
    {
      if (ch == CH_WORDWRAP && pTextNl[1] == CH_NEWLINE) /* found '\r\n' */
        goto bye;

      if (ch == CH_NEWLINE)  /* found '\n' */
      {
        /*
          If we have a hard return (\n) before the maxwidth
          of the line, then we do not need to wordwrap.
        */
        if (pTextNl[1] != CH_WORDWRAP)
          goto bye;

        /*
          We have a wordwrap combination (\n\r).
          Replace the NEWLINE with a blank and get rid of the WORDWRAP char.
          '\n\r' => ' '
        */
        *pTextNl = ' ';
        lmemcpy(pTextNl+1, pTextNl+2, b->nTotalBytes - (pTextNl - b->szText) - 2);
        b->szText[--b->nTotalBytes] = '\0';
        b->lastlnum--;
#if defined(MEWEL_GUI)
        nLinesDeleted++;
#else
        InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
#endif
      } /* end if (ch == NEWLINE) */
    } /* end for */

    /*
      Go through the text until we reach the max width of a line, and
      record the position of the last space character. The variable
      'pLastBlankPos' saves the position of place to wrap at.
    */
    for (iCurrWidth=0;  (ch = *pText) != '\0';  iCurrWidth++, pText++)
    {
      /*
        Record the position where we saw the blank which is closest to the
        max width of the line.
      */
      if (ch == ' ')
      {
        if (iCurrWidth < iMaxWidth-1 || !pLastBlankPos)
          pLastBlankPos = pText;
        if (iCurrWidth >= iMaxWidth)
          break;
      }
      else if (ch == CH_NEWLINE)
        break;
   }

    /*
      We are past the edge. Move back to the last blank pos, get rid of
      the blank, and insert the NEWLINE/WORDWRAP combo.
    */
    if (!pLastBlankPos)
      continue;

    /*
      If we reached the end of the editing buffer, or if the length of the
      line is shorter than the max width, then stop the reformatting...
    */
    if (iCurrWidth < iMaxWidth)
      break;
    /* 
      If the blank is in front of a newline, or at the end, don't wrap
    */
    if (!pLastBlankPos[1])
      break;
    if (pLastBlankPos[1] == CH_NEWLINE)
    {
      pText = pLastBlankPos + 2;
      if (*pText == CH_WORDWRAP)
        pText++;
      continue;
    }

    /*
      Insert \n\r wordwrapping combo
    */
    *pLastBlankPos++ = CH_NEWLINE;
    if (*pLastBlankPos == ' ')
    {
      *pLastBlankPos++ = CH_WORDWRAP;
    }
    /*
      Be careful...
      Insert the wrapping combo only if we won't overflow the edit
      buffer and we won't exceed any app-imposed limit.
    */
    else if (b->nTotalBytes < b->nTotalAllocated && 
             (!b->nMaxLimit || b->nTotalBytes < b->nMaxLimit))
    {
      /*
        Insert a CH_WORDWRAP character ('\r')
      */
      LPSTR s2;
      s2 = b->szText+b->nTotalBytes;
      lmemshr(pLastBlankPos+1, pLastBlankPos, s2-pLastBlankPos+1);
      *pLastBlankPos++ = CH_WORDWRAP;
      b->nTotalBytes++;
      /*
       If we are inserting a \r\n in the middle of the file, at a
       position which is before the current editing position, bump up
       the current position.
      */
      if ((EPOS) b->iCurrPosition >= (EPOS) (pLastBlankPos - b->szText))
        b->iCurrPosition++;
    }

    /*
      We added one more line to the edit buffer, so increment the line count.
    */
    b->lastlnum++;
#if defined(MEWEL_GUI)
    nLinesAdded++;
#else
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
#endif

    /*
      Start scanning again at the place where we wrapped.
    */
    pText = pLastBlankPos;

    /*
      Set horizontal scroll position back to 0, in case the window was
      scrolled horizontally (user typed a long word). If that moves the
      cursor out of visibility, it will be corrected by EditSetCursor.
    */
    b->iHscroll = 0;

  } /* end while (*pText) */


bye:
#if 52893 && defined(MEWEL_GUI)
  /*
    Invalidate part of the edit window if we added or deleted wordwrap
    indicators. If we added a different amount of lines than we deleted,
    then the rest of the edit window should be shifted up or down, so
    invalidate the rest of the window. If we added the same amount of
    lines as we deleted, then we only need to invalidate the changed
    lines.
  */
  if ((nLinesChanged = max(nLinesAdded, nLinesDeleted)) > 0)
  {
    if (nLinesAdded == nLinesDeleted)
      rInvalid.bottom = rInvalid.top +
                    ((nLinesChanged+1) * (b->yFontHeight + b->yFontLeading));
    InvalidateRect(hEdit, (LPRECT) &rInvalid, FALSE);
  }
#endif
  return TRUE;
}



VOID FAR PASCAL _EditReformatAll(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR pText;
  EPOS  iOrigPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return;
  if ((b->style & ES_AUTOHSCROLL) || !(b->style & ES_MULTILINE))
    return;

  iOrigPos = b->iCurrPosition;
  b->iCurrPosition = 0;

  while (_EditReformat(hEdit))
  {
    /* Find the next "hard" newline */
    pText = b->szText + b->iCurrPosition;
    while ((pText = lstrchr(pText, CH_NEWLINE)) != NULL)
      if (pText[1] == CH_WORDWRAP || pText[1] == CH_NEWLINE)
        pText++;
      else
        break;
    if (!pText)
      break;

    /* start reformatting after the newline */
    if (!*(++pText))
      break;
    b->iCurrPosition = pText - b->szText;
  }
  b->iCurrPosition = iOrigPos;
}

#endif /* defined(CH_WORDWRAP) && defined(EDIT_REFORMAT) */

