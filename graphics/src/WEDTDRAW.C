/*===========================================================================*/
/*                                                                           */
/* File    : WEDTDRAW.C                                                      */
/*                                                                           */
/* Purpose : Draws an edit control.                                          */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOKERNEL
#define INCLUDE_CURSES

#include "winedit.h"

/*
  Televoice specific changes
*/
#ifdef TELEVOICE
COLOR wEditBackGround      = 0xFFFF,
      wEditFocusBackGround = 0xFFFF;
#endif

/****************************************************************************/
/*                                                                          */
/* Function : EditShow()                                                    */
/*                                                                          */
/* Purpose  : Routine responsible for rendering an edit control and its     */
/*            contents on the screen.                                       */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: EditWinProc() in response to a WM_PAINT message.              */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EditShow(hEdit, hDC, lprcUpdate)
  HWND hEdit;
  HDC  hDC;
  LPRECT lprcUpdate;
{
  BUFFER *b;
  WINDOW *pWnd;
  INT    row, height;
  INT    iLine, iLast;
  INT    yTotalHeight;
  EPOS   iPos;
  EPOS   startMark, endMark;
  LPSTR  szLine;
  COLOR  attrRev, attrNorm;
  BOOL   bSelected;
  UINT   iLocalHscroll;
  HBRUSH hBrush;
  INT    yOffset, xOffset;
  HFONT  hFont, hOldFont;


  /*
    Get the window and buffer pointers
  */
  if ((pWnd = WID_TO_WIN(hEdit)) == NULL)
    return;
  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return;

  /*
    Make sure it's visible...
  */
  if (!IsWindowVisible(hEdit))
    return;


  /*
    Local vars to speed access...
  */
  iLocalHscroll = b->iHscroll;

  /*
    Figure out the color which we will use for the edit control.
  */
  attrNorm = pWnd->attr;
  if (attrNorm == SYSTEM_COLOR)
    attrNorm = WinGetClassBrush(hEdit);

  /*
    Get the font to draw with
  */
  if ((hFont = (HFONT) SendMessage(hEdit, WM_GETFONT, 0, 0L)) != NULL)
    hOldFont = SelectObject(hDC, hFont);

  /*
    If a non-bordered, single-line edit control has the focus, then
    show the contents in reverse video.
  */
#ifdef NEVER
  if ((b->fFlags & STATE_HASFOCUS) &&
      !(pWnd->flags & (WS_BORDER | ES_MULTILINE)))
    attrNorm = MAKE_HIGHLITE(attrNorm);
#endif


#ifdef TELEVOICE
  /*
    Televoice-specific change...
      Use different backgrounds for edit controls.
  */
  if (!(pWnd->flags & WS_DISABLED))
  {
    if (wEditBackGround != 0xFFFF)
      attrNorm = (attrNorm & 0xF) | (wEditBackGround & 0xF0);
    if ((b->fFlags & STATE_HASFOCUS) &&
        !(pWnd->flags & (/* WS_BORDER | */ES_MULTILINE)))
    {
      if (wEditFocusBackGround != 0xFFFF)
        attrNorm = (attrNorm & 0xF) | (wEditFocusBackGround);
    }
  }
#endif


  /*
    If there is a selected region in the edit control, then
      1) Make sure that the starting pos is less than the ending pos
      2) Determine the atribute to use for the highlighting
  */
  if ((bSelected = (BOOL) (b->fFlags & STATE_SOMETHING_SELECTED)) != FALSE)
  {
    if ((startMark = b->iposStartMark) > (endMark = b->iposEndMark))
      startMark ^= endMark, endMark ^= startMark, startMark ^= endMark;


#ifdef TELEVOICE
    /*
      Televoice-specific change...
        If the edit control has something selected but does not have the
      input focus, then do not highlight the selection.
    */
    if (!(b->fFlags & STATE_HASFOCUS))
      attrRev = attrNorm;
    else
      attrRev = (pWnd->attr != 1000) ? /* always true */
#else
    attrRev = (pWnd->attr == SYSTEM_COLOR) ? 
#endif
        WinQuerySysColor(NULL,SYSCLR_EDITSELECTION) : MAKE_HIGHLITE(attrNorm);
  }


  /*
    Get a pointer to the text where we will begin painting. Determine the
    last line to paint and the row to start painting at.
  */
  yTotalHeight = b->yFontHeight + b->yFontLeading;
  row   = lprcUpdate->top / yTotalHeight;
  iLine = (row == b->cursor.row) ? b->currlinenum : b->topline + row;
  iLast = iLine +  (lprcUpdate->bottom-lprcUpdate->top) / yTotalHeight;
  if (iLast > b->lastlnum)
    iLast = b->lastlnum;
  szLine = (LPSTR) _EditGetLine(hEdit, iLine, &iPos);

  /*
    Get the height of the control in number of lines.
  */
  height = lprcUpdate->bottom / yTotalHeight + 1;
  if (height > b->nLinesInWindow)
    height = b->nLinesInWindow;

  /*
    Calculate the offset from the top and the left of the client area 
    to start drawing the text at.
  */
  if (pWnd->flags & ES_MULTILINE)
    yOffset = 0;    /* Windows top-justifies the text */
  else
    yOffset = ((pWnd->rClient.bottom-pWnd->rClient.top) - b->yFontHeight) / 2;
  xOffset = b->xFontWidth / 2;  /* start to the right of the left border */

  /*
    Erase the background of the invalid part of the edit control
  */
  hBrush = CreateSolidBrush(AttrToRGB(GET_BACKGROUND(attrNorm)));
  FillRect(hDC, lprcUpdate, hBrush);
  DeleteObject(hBrush);

  /*
    Set the foreground color to the proper value.
  */
  SetTextColor(hDC, AttrToRGB(GET_FOREGROUND(attrNorm)));
  SetBkColor(hDC, AttrToRGB(GET_BACKGROUND(attrNorm)));


  /*
    Special handling for password fields
  */
  if (pWnd->flags & ES_PASSWORD)
  {
    /*
      If there is a password char defined, fill the edit control with it.
    */
    if (b->chPassword)
    {
      UINT iLen;
      PSTR szPassword = (PSTR) emalloc((iLen = lstrlen(szLine + iLocalHscroll)) + 1);
      memset(szPassword, b->chPassword, iLen);
      TextOut(hDC, xOffset, yOffset, szPassword, iLen);
      MyFree(szPassword);
    }
    goto bye;
  }  /* endif (ES_PASSWORD) */


  /*
    If we have an empty string and the user did an EM_SETSEL, then
    the code below will highlite the first column anyway. For an
    empty string, just get outta here. If something wasn't selected, then
    we want to erase the entire window.
  */
  if (b->nTotalBytes == 0 && !bSelected)
    goto bye;


  /*
    Go through all of the lines in the dirty area
  */
  for (  ;  row < height && iLine <= iLast;  row++, iLine++)
  {
    UINT  iLen;
    LPSTR szNextLine = lmemchr(szLine, CH_NEWLINE, b->nTotalBytes - iPos);
    BOOL  bIsLineSelected;
    INT   yTop = row * yTotalHeight + yOffset;

    /*
      Determine the number of characters to write.
      Note - we should really have a function like
        ltabbedstrlen(szLine, startingCol);
    */
    if (szNextLine)
      *szNextLine = '\0';        /* make a string out of this line */
    iLen = lstrlen(szLine);

    /*
      See if the line is part of the selected area.
    */
    bIsLineSelected = (BOOL) 
          (bSelected && (iPos+iLen >= startMark && iPos <= endMark));

    /*
      If the line is totally scrolled off to the left of the screen,
      then don't write anything. However, in a selected line, 
      if the line is totally off the screen or the line is blank,
      write at least one highlighted character to let the user know
      that the line is part of a selection.
    */
    if (iLen <= iLocalHscroll && !bIsLineSelected)
      goto advance_to_next_line;

    /* 
      Write the line, regardless whether it is a part of a selected area
    */
    TabbedTextOut(hDC, xOffset, yTop, szLine + iLocalHscroll, iLen,
                  1, &b->iTabSize, iLocalHscroll);


    /*
      See if this line is part of a highlighted area.
    */
    if (bIsLineSelected)
    {
      LPSTR pEndMarked;
      BYTE  chSave;

      /*
        Get ready to invert.
      */
      SetTextColor(hDC, AttrToRGB(GET_FOREGROUND(attrRev)));
      SetBkColor(hDC, AttrToRGB(GET_BACKGROUND(attrRev)));
      HIGHLITE_ON();

      /*
        If the line is blank or scrolled off, just show a highlight
        at the beginning of the line.
      */
      if (iLen == 0 || iLen <= iLocalHscroll)
      {
#if !13095
        TextOut(hDC, xOffset, yTop, " ", 1);
#endif
      }
      else
      {
        /* 
          See if the line is the starting marked line
        */
        if (startMark > iPos)
        {
          LPSTR lpStart = szLine + (startMark - iPos);

          /*
            For proportionally spaced text, we need to figure out the
            x coordinate at which the highlight starts.
          */
          UINT  xStart;
          BYTE  ch = *lpStart;
          *lpStart = '\0';
#ifdef TELEVOICE
          xStart = LOWORD(GetTabbedTextExtent(hDC, szLine,
                                startMark-iPos, 1, &b->iTabSize));
          xStart -= iLocalHscroll;
#else
          xStart = LOWORD(GetTabbedTextExtent(hDC, szLine+iLocalHscroll,
                                startMark-iPos, 1, &b->iTabSize));
#endif
          *lpStart = ch;


          /*
            If the end mark is on this line, make this the end of the 
            marked string for display.
          */
          if (endMark < iPos + iLen)
          {
            pEndMarked = szLine + endMark - iPos + 1;
            chSave = *pEndMarked;
            *pEndMarked = '\0';
          }
          else
            pEndMarked = NULL;

          TabbedTextOut(hDC,
                        xStart + xOffset,
                        yTop,
                        lpStart, iLen,
                        1, &b->iTabSize, iLocalHscroll);

          if (pEndMarked)
            *pEndMarked = chSave;
        } /* end if (startMark > iPos) */

        /*
          See if the line is the ending marked line
        */
        else if (endMark < iPos + iLen)
        {
#ifdef TELEVOICE
          if (endMark-iLocalHscroll+1 > iPos) 
#endif
          {
          pEndMarked = szLine + endMark - iPos + 1;
          chSave = *pEndMarked;
          *pEndMarked = '\0';    /* close off the string */
          TabbedTextOut(hDC, xOffset, yTop, szLine + iLocalHscroll, 
                        lstrlen(szLine+iLocalHscroll),
                        1, &b->iTabSize, iLocalHscroll);
          *pEndMarked = chSave;
          }
        }

        /* 
          The line is a selected line in the middle of the highlited area.
        */
        else
        {
          TabbedTextOut(hDC, xOffset, yTop, szLine + iLocalHscroll, iLen,
                        1, &b->iTabSize, iLocalHscroll);
        }
      }

      /*
        Restore the original text color
      */
      SetTextColor(hDC, AttrToRGB(GET_FOREGROUND(attrNorm)));
      SetBkColor(hDC, AttrToRGB(GET_BACKGROUND(attrNorm)));
      HIGHLITE_OFF();
    } /* end if line is part of a selection */


    /*
      If we converted a \n to \0, restore it
    */
advance_to_next_line:
    if (szNextLine) 
    {
      *szNextLine = CH_NEWLINE;     /* Restore the old character */
      szLine = szNextLine + 1;      /* Set up for next line */
      if (*szLine == CH_WORDWRAP)   /* Go past the wordwrap character */
        szLine++;
      iPos = szLine - b->szText;    /* Calculate the new offset */
    }

  } /* end for (row) */


bye:
  /*
    Redraw the cursor
  */
  if ((b->fFlags & (STATE_HASCURSOR | STATE_NO_SYNCCURSOR)) == STATE_HASCURSOR)
    PostMessage(hEdit, WM_SYNCCURSOR, 0, 0L);

  /*
    Release the DC if we obtained one here.
  */
  if (hFont)
    SelectObject(hDC, hOldFont);
}

