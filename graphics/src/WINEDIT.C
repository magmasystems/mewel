/*===========================================================================*/
/*                                                                           */
/* File    : WINEDIT.C                                                       */
/*                                                                           */
/* Purpose : Implements the edit class.                                      */
/*                                                                           */
/* History : July, 1991 - modified by Rachel McKenzie.                       */
/*           Added OVERTYPE_DEFAULT switch which causes the edit control to  */
/*            default to overtype instead of insert mode.                    */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOKERNEL
#define INCLUDE_CURSES

#include "winedit.h"
#if defined(MEWEL_GUI)
#include "wgraphic.h"
#define USE_IBEAM_CARET
#endif


HANDLE hEditPasteBuffer = 0;
LPSTR  EditPasteBuffer = NULL;  /* holds scrap text */
UINT   EditBufferSize = 2048;


/*
  Declarations of various internal functions
*/
#ifdef __cplusplus
extern "C" {
#endif
static EPOS PASCAL WordwrapCheck(BUFFER *, EPOS, BOOL);
static INT  PASCAL _EditTryMovingToColumn(LPSTR, int, BOOL *);
#ifdef __cplusplus
}
#endif


/***************************************************************************\
| EditCreate()                                                              |
|   Creates an edit control.                                                |
\***************************************************************************/
HWND FAR PASCAL EditCreate(hParent, row1,col1,row2,col2, title, attr, flags, id)
  HWND  hParent;
  int   row1,col1, row2,col2;
  LPSTR title;
  COLOR attr;
  DWORD flags;
  INT   id;
{
  HWND  hEdit, hHSB, hVSB;

  /*
    Create an edit window
  */
  if ((hEdit = WinCreate(hParent, row1,col1,row2,col2, title, attr,
                         flags | WS_CLIPSIBLINGS,
                         EDIT_CLASS, id)) == NULLHWND)
    return NULLHWND;

  /*
    Create the edit buffer
  */
  if (EditBufferCreate(hEdit, id, flags))
  {
    BUFFER *b = EditHwndToBuffer(hEdit);

    /*
      We will search for the horizontal and vertical scrollbars, and
      set each to an appropriate scroll range.
    */
    WinGetScrollbars(hEdit, &hHSB, &hVSB);
    if (hVSB)
      SetScrollRange(b->hVertSB  = hVSB, SB_CTL, 1, 2,  FALSE);
    if (hHSB)
      SetScrollRange(b->hHorizSB = hHSB, SB_CTL, 0, 80, FALSE);

#if defined(MEWEL_TEXT)
    /*
      Set the fill char for single-line edit fields
    */
    if (!(b->style & ES_MULTILINE))
      WID_TO_WIN(hEdit)->fillchar = WinGetSysChar(SYSCHAR_EDIT_BACKGROUND);
#endif

    return hEdit;
  }
  else
    return NULLHWND;
}


/****************************************************************************/
/*                                                                          */
/* Function : EditSetCursor()                                               */
/*                                                                          */
/* Purpose  : Make sure that the current editing position is valid.         */
/*            Updates some important edit buffer variables, such as         */
/*            the currlinenum, topline, iHscroll, and cursor.row/col.       */
/*                                                                          */
/*            In this routine, we :                                         */
/*              Calculate the currlinenum                                   */
/*              See if the currentline is off the screen (adjust topline)   */
/*              Calculate the cursor column and row                         */
/*              See if the cursor column is off the screen (adjust iHscroll)*/
/*              Adjust the scrollbars                                       */
/*              Sync the caret                                              */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/* Called by: EditBackspace()                                               */
/*            Various places in EditWinProc                                 */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EditSetCursor(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  int    width, height;
  EPOS   iOrigHscroll;
  EPOS   iPosBOL;
  BOOL   bCanScroll;
  BOOL   bInvalidate = FALSE;

  /*
    Get a pointer to the buffer structure
  */
  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return;

  /*
    Save the initial horz scroll amount so we can prevent redundant
    calls to SetScrollPos(hHSB)
  */
  iOrigHscroll = b->iHscroll;

  b->iCurrPosition = WordwrapCheck(b, b->iCurrPosition, FALSE);

  /*
    Get the width and height of the edit control.
    Derive the current line number from the current buffer position.
  */
  _EditGetDimensions(hEdit, &height, &width);

  /*
    Check to see if we are at or past the bottom-right corner of a 
    non-scrollable edit control. If so, then force the current position 
    back to the bottom right.
  */
  bCanScroll = (BOOL)
                  ((b->style & (ES_AUTOVSCROLL | ES_AUTOHSCROLL)) != 0L);
  if (!bCanScroll && b->cursor.column >= width-0 && b->cursor.row >= height-1)
    b->iCurrPosition = b->nTotalBytes-1;


  /*
    Given the current index into the buffer (iCurrPosition), return
    the line number and the offset from the beginning of the line.
  */
  b->currlinenum = _EditIndexToLine(hEdit, b->iCurrPosition, &iPosBOL);

  /*
    Insure that the currentline is in the window. If it's not, then
    make it the top line of the window.
  */
  if (b->currlinenum < b->topline || b->currlinenum >= b->topline + height)
  {
    /*
      The two if's test if we can merely scroll up or down by 1 line.
    */
    if (b->topline + height == b->currlinenum)
      b->topline++;
    else if (b->topline - 1 == b->currlinenum)
      b->topline--;
    else
      b->topline = b->currlinenum;
    bInvalidate++;
  }

  /*
    Set the cursor column
  */
#ifdef MEWEL_TEXT
  b->cursor.column = 
     LOWORD(GetTabbedTextExtent((HDC) 0, b->szText+iPosBOL, 
                                b->iCurrPosition-iPosBOL, 1, &b->iTabSize));
  if (!(b->style & ES_MULTILINE) && b->nMaxLimit > 0)
    b->cursor.column = min(b->cursor.column, (int) b->nMaxLimit-1);

#else
  /*
    This block handles positioning the cursor in the presence
    of tabs in the buffer
  */
  {
  LPSTR pStart = b->szText + iPosBOL,
        pEnd   = b->szText + b->iCurrPosition;
  int   col = 0;

  while (pStart != pEnd)  /* For every tab between the start & the current */  
  {                       /* position, move the col to the next tab stop.  */
    if (*pStart++ == '\t')
      col += (b->iTabSize - (col % b->iTabSize));
    else
      col++;
  }
  b->cursor.column = col;
  }
#endif

  /*
    Set the row position relative to the client area.
  */
  b->cursor.row = b->currlinenum - b->topline;    /* 0-based row position */


  /*
    Insure that the current column is in the window.
  */
  if (b->fFlags & STATE_SCROLLING)
  {
    /*
      If we are in the middle of scrolling, we want to hide the
      caret if it is not in the visible window.
    */
    b->fFlags &= ~STATE_SCROLLING;
  }
  else if (max(0, b->cursor.column - SCROLL_AMOUNT) < (int) b->iHscroll)
  {
    b->iHscroll = max(0, b->cursor.column - SCROLL_AMOUNT);
    bInvalidate++;
  }
  else if ((int) b->cursor.column >= (int) (b->iHscroll+width))
  {
    /* Move the column into visible range */
    while ((int) b->cursor.column >= (int) (b->iHscroll+width))
      b->iHscroll++;
    b->iHscroll += SCROLL_AMOUNT;
    bInvalidate++;
  }

  /*
    See if we need to repaint the edit control.
  */
  if (bInvalidate)
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);

  /*
    Adjust the scrollbar positions
  */
  if (b->hVertSB)
  {
    HWND hVSB = b->hVertSB;
    INT  minPos, maxPos, currPos;

    GetScrollRange(hVSB, SB_CTL, &minPos, &maxPos);
    currPos = GetScrollPos(hVSB, SB_CTL);

    /*
      Minimize redrawing by setting the new scroll position only
      if the current line or last line has changed since the last time.
    */
    minPos = max(b->lastlnum, 2);  /* use minPos as a scratch var */
    if (minPos != maxPos || b->currlinenum != currPos)
    {
      SetScrollRange(hVSB, SB_CTL, 1, minPos, FALSE);
      SetScrollPos(hVSB, SB_CTL, b->currlinenum, TRUE);
    }
  }

  if (b->hHorizSB && b->iHscroll != iOrigHscroll)
  {
    SetScrollPos(b->hHorizSB, SB_CTL, b->iHscroll, TRUE);
  }

  /*
    Sync the caret
  */
  EditSyncCursor(hEdit, b);
}


/****************************************************************************/
/*                                                                          */
/* Function : EditSyncCursor()                                              */
/*                                                                          */
/* Purpose  : Posts a WM_SYNCCURSOR message to update the caret.            */
/*                                                                          */
/* Called by: EditWinProc, in response to WM_SETFOCUS message.              */
/*            EditSetCursor, at the end.                                    */
/*                                                                          */
/****************************************************************************/
VOID PASCAL EditSyncCursor(hEdit, b)
  HWND   hEdit;
  BUFFER *b;
{
  /*
    If the WM_SYNCCURSOR message has already been posted for this edit
    control, then there is no need to post another one.

    Also, do not post it if the edit control will be getting updated anyway.
    This will cause redundant cursor processing.
  */
  if (!(b->fFlags & (STATE_NO_SYNCCURSOR | STATE_SYNCCURSOR_POSTED)) && 
      !GetUpdateRect(hEdit, (LPRECT) NULL, FALSE))
  {
    PostMessage(hEdit, WM_SYNCCURSOR, 0, 0L);
    b->fFlags |= STATE_SYNCCURSOR_POSTED;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : _EditPositionCursor()                                         */
/*                                                                          */
/* Purpose  : Responsible for setting the cursor shape and position within  */
/*            an edit control.                                              */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/* Called by: EditWinProc, in response to the WM_SYNCCURSOR message.        */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _EditPositionCursor(hEdit, b)
  HWND   hEdit;
  BUFFER *b;
{
  if (InternalSysParams.hWndFocus == hEdit)
  {
    INT x, y;
    INT cxChar;  /* the width of the caret - same as width of curr character */

#if defined(MEWEL_GUI)
    extern INT PASCAL EditGetCurrPosExtent(HWND, BUFFER *, INT *);
    x = EditGetCurrPosExtent(hEdit, b, &cxChar);
    x += b->xFontWidth / 2;  /* xOffset to start at */
#else
    cxChar = 1;
    x = b->cursor.column - b->iHscroll;
#endif

    y = b->cursor.row * (b->yFontHeight + b->yFontLeading);

    if (WinIsPointVisible(hEdit, y, x))
    {
#if defined(MEWEL_GUI)
      WINDOW *w = WID_TO_WIN(hEdit);
      int    yOffset;

#if defined(USE_IBEAM_CARET)
      CreateCaret(hEdit, NULL, 1, b->yFontHeight);
#else
      CreateCaret(hEdit, NULL, cxChar, (b->fFlags & STATE_INSERT) ? 2 : 1);
#endif

      /*
        Since SetCaretPos() does nothing more that to put the caret
        at the exact coordinates <x,y>, we need to play with the
        caret positioning in graphics mode to make sure that it
        falls below the character
      */
      if (w->flags & ES_MULTILINE)
        yOffset = 0;    /* Windows top-justifies the text */
      else
      {
        if ((yOffset = (RECT_HEIGHT(w->rClient) - b->yFontHeight) / 2) < 0)
          yOffset = 0;
      }
#if defined(USE_IBEAM_CARET)
      y += yOffset;
#else
      y += (b->yFontHeight + yOffset + 1);
#endif

#else
      CreateCaret(hEdit, NULL, cxChar, (b->fFlags & STATE_INSERT) ? 8 : 1);
#endif

      ShowCaret(hEdit);
      SetCaretPos(x, y);
      b->fFlags |= STATE_HASCURSOR;
    }
    else
    {
      /*
        The caret is not in a visible portion of the window, probably
        because we are doing some scrolling.
      */
      HideCaret(hEdit);
    }
  }
  else
  {
    if (b->fFlags & STATE_HASCURSOR)
    {
      HideCaret(hEdit);
      b->fFlags &= ~STATE_HASCURSOR;
    }
  }
}


/***************************************************************************/
/*                                                                         */
/*        CURSOR MOVEMENT ROUTINES                                         */
/*                                                                         */
/***************************************************************************/

int FAR PASCAL EditRight(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  EPOS   iPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;
  iPos = b->iCurrPosition;
  if (b->nMaxLimit > 0 && iPos >= b->nMaxLimit-1)
    return FALSE;
  if (iPos < b->nTotalBytes)
    iPos++;
#ifdef CH_WORDWRAP
  if (b->szText[iPos] == CH_WORDWRAP)
    iPos++;
#endif
  b->iCurrPosition = iPos;
  return TRUE;
}


int FAR PASCAL EditLeft(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  EPOS   iPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;
  if ((iPos = b->iCurrPosition) > 0)
    iPos--;
  b->iCurrPosition = WordwrapCheck(b, iPos, FALSE);
  return TRUE;
}


int FAR PASCAL EditUp(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  szEOL;
  BOOL   bWrap;
  EPOS   iPos;

  /*
    Get a pointer to the BUFFER
  */
  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  /*
    Can't go up in a single-line control
  */
  if (!(b->style & ES_MULTILINE))
    return FALSE;

  iPos = b->iCurrPosition;

  if (b->currlinenum > 1 &&
      (szEOL = (LPSTR) _EditFindEndOfPrevLine(b->szText, &iPos)) != NULL)
  {
    if ((szEOL = (LPSTR) _EditFindEndOfPrevLine(b->szText, &iPos)) == NULL)
      szEOL = b->szText - 1;
    b->cursor.column = _EditTryMovingToColumn(szEOL+1, b->cursor.column, &bWrap);
    iPos += b->cursor.column+1;
    b->iCurrPosition = WordwrapCheck(b, iPos, bWrap);
    b->currlinenum--;
    return TRUE;
  }
  else
    return FALSE;
}


int FAR PASCAL EditDown(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  szBOL;
  EPOS   iPos;
  BOOL   bWrap;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  if (!(b->style & ES_MULTILINE))
    return FALSE;

  iPos = b->iCurrPosition;

  if (b->currlinenum < b->lastlnum &&
      (szBOL = (LPSTR) _EditFindStartOfNextLine(b->szText, &iPos)) != NULL)
  {
    b->currlinenum++;
    b->cursor.column = _EditTryMovingToColumn(szBOL, b->cursor.column, &bWrap);
    iPos += b->cursor.column;
    b->iCurrPosition = WordwrapCheck(b, iPos, bWrap);
    return TRUE;
  }
  return FALSE;
}


int FAR PASCAL EditNextWord(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  s;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  /*
    Span the non-blank characters and get to the next blank
  */
  for (s = b->szText + b->iCurrPosition;  !isspace(*s);  s++)
    ;
  if (!*s)
    return FALSE;
  /*
    Span the blanks and get to the next non-blank char
  */
  while (isspace(*s))
    s++;
  if (!*s)
    return FALSE;

  b->iCurrPosition = s - b->szText;
  return TRUE;
}

int FAR PASCAL EditPrevWord(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  s, szBuffer;

  if ((b = EditHwndToBuffer(hEdit)) == NULL || b->iCurrPosition == 0)
    return FALSE;

  szBuffer = b->szText;
  s = szBuffer + b->iCurrPosition;

  /*
    Don't get stuck at the beginning of a word
  */
  if (s > szBuffer && isspace(*(s-1)))
    s--;
  while (s > szBuffer && isspace(*s))
    s--;
  while (s > szBuffer && !isspace(*s))
    s--;
  if (s > szBuffer)
    s++;

  b->iCurrPosition = s - szBuffer;
  return TRUE;
}

int FAR PASCAL EditPageDown(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  int    height;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  _EditGetDimensions(hEdit, &height, NULL);

  if (b->topline + height <= b->lastlnum)
  {
    b->topline += height;
    if ((b->currlinenum += height) > b->lastlnum)
      b->currlinenum = b->lastlnum;
    _EditGetLine(hEdit, b->currlinenum, &b->iCurrPosition);
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  }
  else
    EditEndOfBuffer(hEdit);

  return TRUE;
}

int FAR PASCAL EditPageUp(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  int    height;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  if (b->currlinenum <= 1)
    return TRUE;

  _EditGetDimensions(hEdit, &height, NULL);

  if ((b->topline -= height) <= 0)
    b->currlinenum = b->topline = 1;
  else
    b->currlinenum -= height;

  _EditGetLine(hEdit, b->currlinenum, &b->iCurrPosition);

  InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  return TRUE;
}


int FAR PASCAL EditBeginningOfLine(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  EPOS   iPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  iPos = b->iCurrPosition;
  if (_EditFindEndOfPrevLine(b->szText, &iPos) != NULL)
    iPos++;
  else
    iPos = 0;
#ifdef CH_WORDWRAP
  if (b->szText[iPos] == CH_WORDWRAP)
    iPos++;
#endif

  b->iCurrPosition = iPos;
  return TRUE;
}


int FAR PASCAL EditEndOfLine(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  szBOL;
  EPOS   iPos;
  EPOS   iTmp;

  if (EditIsEOL(hEdit) || (b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  iPos = b->iCurrPosition;

  /*
    Try searching for the start of the next line. If we found a newline,
    then back up one position.
  */
  if ((szBOL = _EditFindStartOfNextLine(b->szText, &iPos)) != NULL)
    iPos--;
  else
  {
    /*
      There is no next line. We are either on the last line of a multiline
      buffer or we have a single line buffer.
    */
    iTmp = iPos = b->nTotalBytes;
    if ((szBOL = _EditFindEndOfPrevLine(b->szText, &iTmp)) == NULL)
    {
      b->cursor.column = lstrlen(b->szText); /* we have a 1-line buffer */
      if (b->nMaxLimit > 0)
      {
        b->cursor.column = min((int) b->cursor.column, (int) b->nMaxLimit-1);
        if (iPos >= b->nMaxLimit)
          iPos = b->nMaxLimit-1;
      }
    }
    else
      b->cursor.column = lstrlen(szBOL+1);    /* we have a multi-line buf */
  }

  b->iCurrPosition = iPos;
  return TRUE;
}


int FAR PASCAL EditBeginningOfBuffer(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  b->currlinenum   = 1;
  b->cursor.column = b->cursor.row = b->iHscroll = 0;
  b->iCurrPosition = 0;
  if (b->topline > 1)
  {
    b->topline = 1;
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  }
  return TRUE;
}


int FAR PASCAL EditEndOfBuffer(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if (EditIsEOF(hEdit))
    return TRUE;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  b->iCurrPosition = b->nTotalBytes;
  InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  return TRUE;
}


int FAR PASCAL EditGoto(hEdit, iLine, col)
  HWND hEdit;
  int  iLine, col;
{
  BUFFER *b;
  EPOS   iPos;
  LPSTR  szLine;
  BOOL   bWrap;
  int    width, height;
  BOOL   bCanScroll;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  _EditGetDimensions(hEdit, &height, &width);
  iLine = min(iLine, b->lastlnum);
  b->currlinenum   = iLine;

  szLine = _EditGetLine(hEdit, iLine, &iPos);
  col = _EditTryMovingToColumn(szLine, col, &bWrap);

  bCanScroll = ((b->style & (ES_AUTOVSCROLL | ES_AUTOHSCROLL)) != FALSE);
  /*
    Make sure that we are not moving past the lower-right corner of a
    non-scrolling edit field.
  */
  if (!bCanScroll && col >= width && iLine >= height-1)
    col = width - 1;

  b->iCurrPosition = iPos + col;
#ifdef CH_WORDWRAP
  if (bWrap)  /* if we passed a wordwrap marker, then bump up the position */
    b->iCurrPosition++;  
#endif

  return TRUE;
}


int FAR PASCAL EditScroll(hEdit, nLines, nCols)
  HWND hEdit;
  int  nLines;
  int  nCols;
{
  BUFFER *b;
  EPOS   iPos;
  int    height;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  if (b->style & ES_AUTOVSCROLL)
  {
    b->topline += nLines;
    b->topline = min(b->lastlnum, b->topline);
    b->topline = max(1, b->topline);
  }

  if (b->style & ES_AUTOHSCROLL)
  {
    b->iHscroll += nCols;
    b->iHscroll = max(0, (int) b->iHscroll);
  }

  _EditGetDimensions(hEdit, &height, NULL);
  if (b->currlinenum < b->topline)
    b->currlinenum = b->topline;
  else if (b->currlinenum > b->topline + height - 1)
    b->currlinenum = b->topline + height - 1;

  (void) _EditGetLine(hEdit, b->currlinenum, &iPos);
  b->iCurrPosition = iPos + b->cursor.column;
  b->iCurrPosition = WordwrapCheck(b, b->iCurrPosition, FALSE);

  InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  return TRUE;
}


/***************************************************************************/
/*                                                                         */
/*        TEXT SELECTION ROUTINES                                          */
/*                                                                         */
/***************************************************************************/

int FAR PASCAL EditSelect(hEdit, key)
  HWND hEdit;
  int  key;
{
  BUFFER *b;
  int    rc;
  int    oldCurrline;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  if (!(b->fFlags & STATE_SOMETHING_SELECTED))
    b->iposStartMark = b->iCurrPosition;
  oldCurrline = b->currlinenum;

  switch (key)
  {
#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_LEFT      :
#else
    case VK_SH_LEFT   :
#endif
      rc = EditLeft(hEdit);
      break;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_RIGHT     :
#else
    case VK_SH_RIGHT  :
#endif
      rc = EditRight(hEdit);
      break;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_UP        :
#else
    case VK_SH_UP     :
#endif
      rc = EditUp(hEdit);
      break;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_DOWN      :
#else
    case VK_SH_DOWN   :
#endif
      rc = EditDown(hEdit);
      break;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_HOME      :
#else
    case VK_SH_HOME   :
#endif
      rc = EditBeginningOfLine(hEdit);
      break;

#if defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_END       :
#else
    case VK_SH_END    :
#endif
      rc = EditEndOfLine(hEdit);
      break;

    case 0         :  /* mouse move */
      rc = TRUE;
      break;
  }

  if (rc == TRUE)
  {
    b->iposEndMark = b->iCurrPosition;

#ifdef ES_NON_INCLUSIVE   /* ec */
   if ((b->style & ES_NON_INCLUSIVE) && b->iposStartMark != b->iposEndMark)
   {
     if (b->iposStartMark > b->iposEndMark)       /* these lines make the   */
       b->iposEndMark++;                          /* marking function into  */
     else                                         /* a 'non-inclusive' mark */
       b->iposEndMark--;             /* not including the current character */
   }
#endif

    b->fFlags |= STATE_SOMETHING_SELECTED;

    if (key)
    {
      if (oldCurrline == b->currlinenum)
        InvalidateLine(hEdit, REDRAW_LINE, 0);
      else
      {
#if 60894
        int iStartRow, iEndRow;
        int nLines, nLinesInWindow;
        int iSaveRow;
        RECT r;

        GetClientRect(hEdit, &r);
        nLinesInWindow = r.bottom / (b->yFontHeight + b->yFontLeading);

        EditSetCursor(hEdit);  /* need to set b->cursor correctly */

        iSaveRow = b->cursor.row;
        nLines = b->currlinenum - oldCurrline;
        if (nLines < 0)
        {
          nLines    = -nLines;
          iStartRow = b->cursor.row;
          iEndRow   = iStartRow + nLines;
        }
        else
        {
          iEndRow   = b->cursor.row;
          iStartRow = iEndRow - nLines;
        }

        /*
          Do some range checking on the first and last rows
        */
        if (iStartRow < 0)
          iStartRow = 0;
        if (iEndRow >= nLinesInWindow)
          iEndRow = nLinesInWindow;
        if (b->topline + nLinesInWindow >= b->lastlnum &&
            iEndRow > b->lastlnum - b->topline)
          iEndRow = b->lastlnum - b->topline;

        b->cursor.row = iStartRow;
        while (iStartRow++ <= iEndRow)
        {
          InvalidateLine(hEdit, REDRAW_LINE, 0);
          b->cursor.row++;
        }

        b->cursor.row = iSaveRow;
#else
        InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
#endif
      }
    }
  }

  return rc;
}

int FAR PASCAL EditClearSelect(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  BOOL   bSelected;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  ReleaseCapture();             /* give the mouse back to other windows */

  bSelected = (BOOL) ((b->fFlags & STATE_SOMETHING_SELECTED) != 0);
  b->fFlags &= ~STATE_SOMETHING_SELECTED;
  if (bSelected)
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
  return TRUE;
}

int FAR PASCAL EditCopySelection(HWND hEdit, BOOL bDelete, BOOL bCopy)
{
  BUFFER *b;
  LPSTR  szSelection;
  UINT   len;
  EPOS   startMark, endMark;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  if (!(b->fFlags & STATE_SOMETHING_SELECTED))
    return FALSE;

  if ((startMark = b->iposStartMark) > (endMark = b->iposEndMark))
    startMark ^= endMark, endMark ^= startMark, startMark ^= endMark;

  if (endMark >= b->nTotalBytes)
    endMark = b->nTotalBytes - 1;

  len = endMark - startMark + 1;
  szSelection = b->szText + startMark;

  /*
    If the selection was over the start of a wordwrap-pair, then make
    sure that it encompasses the entire pair.
  */
  if (szSelection[len-1] == CH_NEWLINE && szSelection[len] == CH_WORDWRAP)
  {
    endMark++;
    len++;
  }

  if (bCopy)                     /* decide whether or not to copy */
  {
#ifdef CLIPBOARD
    BYTE chSave;
    HANDLE hSel;

    chSave = szSelection[len];  szSelection[len] = '\0';
    if (OpenClipboard(hEdit))
    {
      EmptyClipboard();
      hSel = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) len+1);
      lstrcpy(GlobalLock(hSel), szSelection);
      GlobalUnlock(hSel);
      SetClipboardData(CF_TEXT, hSel);
      CloseClipboard();
    }
    szSelection[len] = chSave;

#else
    if (EditPasteBuffer)
      MyFree_far(EditPasteBuffer);
    if ((EditPasteBuffer = emalloc_far((DWORD) len+1)) == NULL)
      return FALSE;

    lmemcpy(EditPasteBuffer, szSelection = b->szText + startMark, len);
    EditPasteBuffer[len] = '\0';
#endif
  }

  if (bDelete)
  {
    LPSTR szLine;
    int  iLine;
    EPOS iPos;

    /* Shift the rest of the stuff over */
    lmemcpy(szSelection, szSelection+len, b->nTotalBytes - endMark - 1);
    lmemset(b->szText + (b->nTotalBytes - len), '\0', len);
    b->nTotalBytes -= len;
    b->dwModify++;
    InvalidateRect(hEdit, (LPRECT) NULL, FALSE);

    b->iCurrPosition = min(startMark, b->nTotalBytes);

    /*
      We want to recalc the cursor position and the number of lines
    */
    szLine = b->szText;
    for (iPos = 0, iLine = 1;  szLine;  )
    {
      if (iPos <= b->iCurrPosition)
      {
        b->currlinenum = iLine;
        b->cursor.column = b->iCurrPosition - iPos;
      }

      /*
        SCO UNIX barfs if you do a memchr() where the count is 0
      */
      if (b->nTotalBytes == 0)
      {
        break;
      }

      if ((szLine = lmemchr(szLine, CH_NEWLINE, b->nTotalBytes - iPos)) != NULL)
      {
        szLine++, iLine++;
#ifdef CH_WORDWRAP
        if (*szLine == CH_WORDWRAP)   /* Go past the wordwrap character */
          szLine++;
#endif
        iPos = szLine - b->szText;
      }
    }
    b->lastlnum = iLine;

#ifdef EDIT_REFORMAT
    _EditReformat(hEdit);
#endif
  }

  EditClearSelect(hEdit);
  return TRUE;
}


int FAR PASCAL EditPaste(hEdit)
  HWND hEdit;
{
  UINT   fOldFlags;
  BUFFER *b;
  int    rc;

#ifdef CLIPBOARD
  if (!OpenClipboard(hEdit) ||
      (hEditPasteBuffer = GetClipboardData(CF_TEXT)) == NULL)
  {
    CloseClipboard();
#else
  if (EditPasteBuffer == NULL)
  {
#endif
    return FALSE;
  }

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  fOldFlags = b->fFlags;
  b->fFlags |= STATE_INSERT;

  EditPasteBuffer = GlobalLock(hEditPasteBuffer);

  /*
    Insert the text from the clipboard
  */
  rc = EditInsertBlock(hEdit, (LPSTR) EditPasteBuffer);

  if (!(fOldFlags & STATE_INSERT))  /* only remove insert mode if in */
    b->fFlags &= ~STATE_INSERT;     /* overstrike when we started */

  /*
    Force the edit control to be redrawn
  */
  InvalidateRect(hEdit, (LPRECT) NULL, FALSE);

#ifdef CLIPBOARD
  CloseClipboard();
  GlobalUnlock(hEditPasteBuffer);
#endif

  return rc;
}



/***************************************************************************/
/*                                                                         */
/*        INSERTION/DELETION ROUTINES                                      */
/*                                                                         */
/***************************************************************************/

int FAR PASCAL EditToggleInsert(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;
  b->fFlags ^= STATE_INSERT;
  return TRUE;
}


int FAR PASCAL EditDeleteChar(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  LPSTR  s;
  EPOS   nChars = 1;    /* # of chars to delete */

  if ((b = EditHwndToBuffer(hEdit)) == NULL || b->nTotalBytes == 0)
    return FALSE;

  /*
    Get the address of the current cursor position
  */
  s = b->szText + b->iCurrPosition;

  /*
    Don't delete past the end of file
  */
  if (*s == '\0')
    return FALSE;

  if (*s == CH_NEWLINE)
  {
    b->lastlnum--;
    if (b->currlinenum <= b->topline)
      b->topline = b->currlinenum;
    InvalidateLine(hEdit, REDRAW_EOW, 0);

#ifdef CH_WORDWRAP
    if (s[1] == CH_WORDWRAP)
      nChars++;
#endif
  }

  if (b->iCurrPosition < b->nTotalBytes)
    lmemcpy(s, s+nChars, b->nTotalBytes - b->iCurrPosition - nChars);
  b->nTotalBytes -= nChars;
  b->szText[b->nTotalBytes] = '\0';

#ifdef EDIT_REFORMAT
  _EditReformat(hEdit);
#endif

  b->dwModify++;
  InvalidateLine(hEdit, REDRAW_LINE, -1);
  return TRUE;
}


int FAR PASCAL EditBackspace(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if ((b=EditHwndToBuffer(hEdit))==NULL || !b->nTotalBytes || EditIsBOF(hEdit))
    return FALSE;

  if (b->fFlags & STATE_SOMETHING_SELECTED)
  {
    return EditCopySelection(hEdit, TRUE, FALSE);
  }
  else
  {
    /* If we are not at the start of the line, move left & delete */
    EditLeft(hEdit);
    /*
      If we are about to delete a newline, we need to update variables like
      the cursor-row and currentline.
    */
    if (EditIsEOL(hEdit))
      EditSetCursor(hEdit);
    return EditDeleteChar(hEdit);
  }
}


int FAR PASCAL EditInsertChar(hEdit, szText)
  HWND hEdit;
  LPSTR szText;
{
  BUFFER *b;
  LPSTR  s;
  BYTE   c;
  int    width, height;
  EPOS   nMaxLimit;
  BOOL   bCanScroll;
  BOOL   bAtTheBottomRight;
  BOOL   bInserting, bMustInsert;


  if (szText == NULL || (b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  /*
    If an area is selected and we insert a char, erase the selected
    area first....
  */
  if (b->fFlags & STATE_SOMETHING_SELECTED)
  {
    if (b->nTotalBytes > 0)
      EditCopySelection(hEdit,TRUE,FALSE); /* delete do not copy to scrap */
    else
      b->fFlags &= ~STATE_SOMETHING_SELECTED;
  }

  /*
    Get the limit, height, and width of the edit field.
  */
  nMaxLimit = b->nMaxLimit;
  _EditGetDimensions(hEdit, &height, &width);

  /*
    See if the edit field can scroll.
  */
  bCanScroll = (BOOL)
                  ((b->style & (ES_AUTOVSCROLL | ES_AUTOHSCROLL)) != 0L);
                        
  /*
    See if we are in insert mode. This variable can be forced to FALSE
    if we are inserting a char at the last position of a non-scrolling
    field.
  */
  bInserting = (BOOL) ((b->fFlags & STATE_INSERT) != 0);


  /*
    Go through all of the characters in the string to insert
  */
  while ((c = *szText++) != '\0')
  {
    /*
      Make sure that we are not inserting a char past the lower-right
      corner of a non-scrolling edit field. If we are, then *replace*
      the last character with the character currently typed.
    */
    bAtTheBottomRight = (BOOL)
     (!bCanScroll && b->cursor.column >= width-1 && b->cursor.row >= height-1);

    if (bAtTheBottomRight)   /* force into "overstrike" mode */
      bInserting = FALSE;


    if (c == '\r' || c == CH_NEWLINE)
    {
      /*
        Are we trying to insert a newline at the end of a non-vertical
        scrolling multiline edit control? If so, return.
      */
      if ((b->style & ES_AUTOVSCROLL) == FALSE && b->cursor.row >= height - 1)
        return FALSE;
      c = CH_NEWLINE;
    }
#ifdef CH_WORDWRAP
    else if (c == 0xFF)
      c = CH_WORDWRAP;
#endif

    /*
      Get a pointer to the insert position within the edit buffer
    */
    s = b->szText + b->iCurrPosition;

    /*
      If we are in insert mode, or if we are inserting a character
      before a hard or soft return, or if we are inserting a hard
      or soft return, then shift the characters over by one.
    */
    bMustInsert = (bInserting || *s == CH_NEWLINE || c == CH_NEWLINE);
#ifdef CH_WORDWRAP
    bMustInsert |= (*s == CH_WORDWRAP || c == CH_WORDWRAP);
#endif

    /*
      If we are inserting or we are overstriking at the end of the buffer,
      then check for buffer overflow or for exceeding the preset limit.
    */
    if (bMustInsert || *s == '\0')
    {
      if (b->nTotalBytes >= b->nTotalAllocated-1 || 
          (nMaxLimit > 0 && b->nTotalBytes >= nMaxLimit))
      {
        if (nMaxLimit > 0 && b->nTotalBytes >= nMaxLimit)
          return FALSE;
        if (!_EditReallocate(b, b->nTotalBytes*2))
          return FALSE;
        nMaxLimit = b->nMaxLimit;
        s = b->szText + b->iCurrPosition;
      }
    }


    if (bMustInsert)
    {
      /*
        Shift the chars at the current position rightwards by one,
        and place 'c' at the current position.
      */
      LPSTR s2 = b->szText + b->nTotalBytes;
      lmemshr(s+1, s, s2-s+1);
      *s = c;
      b->nTotalBytes++;
    }

    else  /* We are overstriking, not inserting */
    {
      /*
        If at the end of the buffer, bump up the char count.
      */
      if (*s == '\0') 
      {
        b->nTotalBytes++;
        s[1] = '\0';
      }
      *s = c;
    } /* end overstriking */

    /*
      If we inserted a newline, bump up the line counter.
    */
    if (c == CH_NEWLINE)
    {
      b->lastlnum++;
      EditRight(hEdit);

      /*
        If we are not at the end of the edit buffer, reformat the text.
      */
      if (s[1] != '\0')
      {
        if (!bAtTheBottomRight)
          _EditReformat(hEdit);
        InvalidateRect(hEdit, (LPRECT) NULL, FALSE);
      }
    }
    else
    {
      /*
        Bump up the current position in the buffer if we are not beyond
        the maximum allocated or if we have not exceeded the text limit.
      */
      if (b->iCurrPosition < b->nTotalAllocated-1 && 
          (nMaxLimit <= 0 || b->iCurrPosition < nMaxLimit-1))
      {
        b->iCurrPosition++;
        /*
          Special case - ignore the position bumping if we are at the
          bottom right corner of a non-scrollable edit field.
        */
        if (bAtTheBottomRight)
          b->iCurrPosition--;
      }

      /*
        See if we have typed past the right boundary
      */
#ifdef CH_WORDWRAP
      if (!(b->style & ES_AUTOHSCROLL) && c != CH_WORDWRAP)
#else
      if (!(b->style & ES_AUTOHSCROLL))
#endif
      {
        if (height > 1)
        {
          /*
            Reformat the edit buffer... but only if we are not at the
            bottom-right corner of a non-scrollable edit field.
          */
          if (!bAtTheBottomRight)
            _EditReformat(hEdit);
          InvalidateLine(hEdit, REDRAW_LINE, -1);
          continue;
        }
        b->cursor.column++;
      }
      else
        b->cursor.column++;

set_redraw:
      InvalidateLine(hEdit, REDRAW_LINE, -1);
    }
  } /* end while */


  b->dwModify++;
  return TRUE;
}


/***************************************************************************/
/*                                                                         */
/*        UTILITY ROUTINES                                                 */
/*                                                                         */
/***************************************************************************/

int FAR PASCAL EditIsBOF(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  return b->iCurrPosition == 0;
}

int FAR PASCAL EditIsEOF(hEdit)
  HWND hEdit;
{
  BUFFER *b;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  return b->iCurrPosition>=b->nTotalBytes || 
         (b->nMaxLimit > 0 && b->iCurrPosition>=b->nMaxLimit-1);
}

int FAR PASCAL EditIsBOL(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  EPOS   iPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  iPos = b->iCurrPosition;

  return (b->szText[iPos-1] == CH_NEWLINE)  ||
#ifdef CH_WORDWRAP
         (b->szText[iPos-1] == CH_WORDWRAP) ||
#endif
         (iPos == 0);
}

int FAR PASCAL EditIsEOL(hEdit)
  HWND hEdit;
{
  BUFFER *b;
  EPOS   iPos;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  iPos = b->iCurrPosition;

  return (b->szText[iPos] == CH_NEWLINE)  ||
#ifdef CH_WORDWRAP
         (b->szText[iPos] == CH_WORDWRAP) ||
#endif
         (iPos > b->nTotalBytes);
}



/****************************************************************************/
/* _EditFindStartOfPrevLine()                                               */
/*    Given a pointer to the start of the edit buffer, and an offset into   */
/* the buffer, searches for the start of the prev line. Returns both a      */
/* pointer to that line and the index of that line.                         */
/****************************************************************************/
LPSTR FAR PASCAL _EditFindEndOfPrevLine(szBuf, iRetPos)
  LPSTR szBuf;
  EPOS *iRetPos;
{
  register long iPos;
  LPSTR s;

  iPos = *iRetPos;
  for (s = szBuf + (--iPos);  iPos >= 0 && *s != CH_NEWLINE;  s--, iPos--)
    ;
  *iRetPos = (EPOS) iPos;
  return (iPos >= 0) ? s : NULL;
}

/****************************************************************************/
/* _EditFindStartOfNextLine()                                               */
/*    Given a pointer to the start of the edit buffer, and an offset into   */
/* the buffer, searches for the start of the next line. Returns both a      */
/* pointer to that line and the index of that line.                         */
/****************************************************************************/
LPSTR FAR PASCAL _EditFindStartOfNextLine(szBuf, iRetPos)
  LPSTR szBuf;
  EPOS *iRetPos;
{
  register EPOS iPos;
  LPSTR s;

  iPos = *iRetPos;

  /*
    Point to the proper position in the edit buffer.
  */
  if (*(s = szBuf + iPos) == CH_NEWLINE)
    goto bye;
#ifdef CH_WORDWRAP
  if (*s == CH_WORDWRAP)
    goto bye2;
#endif

  /*
    Search forward for a newline or the EOF
  */
  while (*s && *s != CH_NEWLINE)
    s++, iPos++;
  *iRetPos = iPos;

  if (*s == '\0')
  {
    if (s == szBuf)
      return NULL;
#ifdef CH_WORDWRAP
    return (s[-1] == CH_NEWLINE || s[-1] == CH_WORDWRAP) ? s : NULL;
#else
    return s[-1] == CH_NEWLINE ? s : NULL;
#endif
  }

bye:
  (*iRetPos)++;

#ifdef CH_WORDWRAP
bye2:
  if (s[1] == CH_WORDWRAP)
    s++, (*iRetPos)++;
#endif
  return s+1;
}


/****************************************************************************/
/*                                                                          */
/* _EditGetLine()                                                           */
/*                                                                          */
/*    Returns a pointer to the start of line 'iLine'. Also returns the      */
/* index of the start of that line.                                         */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL _EditGetLine(hEdit, iLine, iposLine)
  HWND hEdit;
  int  iLine;
  EPOS *iposLine;
{
  BUFFER *b;
  LPSTR  szLine;
  int    i;
  EPOS   nBytes;
  EPOS   iPosBOL = 0;

  if ((b = EditHwndToBuffer(hEdit)) == NULL  ||  iLine > b->lastlnum)
    return NULL;

  /*
    Search for the desired line by moving past newline characters.
  */
  nBytes = b->nTotalBytes;
  for (szLine = b->szText, i = 1;  i < iLine && szLine;  i++)
    if ((szLine = lmemchr(szLine, CH_NEWLINE, nBytes)) != NULL)
    {
      szLine++;                           /* advance to the next line     */
#ifdef CH_WORDWRAP
      if (*szLine == CH_WORDWRAP)   /* Go past the wordwrap character */
        szLine++;
#endif
      iPosBOL = szLine - b->szText;       /* get the index of the line    */
      nBytes = b->nTotalBytes - iPosBOL;  /* get the # of bytes remaining */
    }

  /*
    Return the index of the start of the the line, and a pointer to the line.
  */
  if (szLine && iposLine)
    *iposLine = iPosBOL;
  return szLine;
}


/*****************************************************************************/
/*                                                                           */
/*  Function : _EditIndexToLine()                                            */
/*                                                                           */
/*    Given an index into the text, return the line number where the char    */
/*  at that index occurs. Also, return the index where that line begins.     */
/*                                                                           */
/*****************************************************************************/
EPOS FAR PASCAL _EditIndexToLine(hEdit, iPos, iPosBOL)
  HWND hEdit;
  EPOS iPos;
  EPOS *iPosBOL;
{
  BUFFER *b;
  LPSTR  szLine;
  int    iLine;
  EPOS   iLinePos;
  EPOS   searchlen;

  if ((b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  /*
    If iPos is -1, then the user wants the line number of the first
    character of the selected text;
  */
  if (iPos == 0xFFFF)
  {
    if (!(b->fFlags & STATE_SOMETHING_SELECTED))
      return 0;
    iPos = min(b->iposStartMark, b->iposEndMark);
  }

  szLine = b->szText;
  iLinePos = 0;
  if (iPosBOL)
    *iPosBOL = 0;
  iLine = 1;
  searchlen = b->nTotalBytes;

  while (szLine && iLinePos < iPos)
  {
    if ((szLine = lmemchr(szLine, CH_NEWLINE, searchlen)) != NULL)
    {
      szLine++;
#ifdef CH_WORDWRAP
      if (*szLine == CH_WORDWRAP)   /* Go past the wordwrap character */
        szLine++;
#endif
      if ((iLinePos = szLine - b->szText) > iPos)
        break;
      if (iPosBOL)
        *iPosBOL = iLinePos;
      iLine++;
      searchlen = b->nTotalBytes - iLinePos;
    }
  }

  return iLine;
}


/****************************************************************************/
/*                                                                          */
/* Function : _EditTryMovingToColumn()                                      */
/*                                                                          */
/* Purpose  :  Given a string and a desired column to move to, tries to     */
/*             move there. Will fail if we find an end-of-line before the   */
/*             column.                                                      */
/*                                                                          */
/****************************************************************************/
static int PASCAL _EditTryMovingToColumn(szLine, iTargetCol, bWordWrap)
  LPSTR szLine;
  int  iTargetCol;
  BOOL *bWordWrap;
{
  register int col;
  int  c;

#ifdef CH_WORDWRAP
  *bWordWrap = FALSE;
#endif

  /*
    Go through each character until we reach the target column or find the
    end of the line.
  */
  for (col = 0;  col < iTargetCol && (c = *szLine++) != CH_NEWLINE && c;  col++)
#ifdef CH_WORDWRAP
    if (c == CH_WORDWRAP)
    {
      col--;
      *bWordWrap = TRUE;
    }
#else
    ;
#endif

#ifdef CH_WORDWRAP
  if (iTargetCol == 0 && *szLine == CH_WORDWRAP)
    *bWordWrap = TRUE;
#endif

  return col;
}

/****************************************************************************/
/* EditHwndToBuffer()                                                       */
/*   Returns a pointer to an edit control's buffer structure.               */
/*                                                                          */
/****************************************************************************/
BUFFER *FAR PASCAL EditHwndToBuffer(hEdit)
  HWND hEdit;
{
  WINDOW *w = WID_TO_WIN(hEdit);
  return (!w) ? NULL : (BUFFER *) w->pPrivate;
}

/****************************************************************************/
/* _EditNotifyParent()                                                      */
/*    Notifies the parent of an edit control that something interesting has */
/* occured. Sends a WM_COMMAND message with 'code' in HIWORD(lParam).       */
/****************************************************************************/
int FAR PASCAL _EditNotifyParent(hEdit, code)
  HWND hEdit;
  UINT code;
{
  BUFFER *b;
  HWND   hParent;

  b = EditHwndToBuffer(hEdit);
  if ((hParent = GetParent(hEdit)) != NULLHWND)
    return (int) SendMessage(hParent,WM_COMMAND,b->idCtrl,MAKELONG(hEdit,code));
  return FALSE;
}

/****************************************************************************/
/* _EditGetDimensions()                                                     */
/*    Returns the height and width of an edit control                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL _EditGetDimensions(hEdit, pHeight, pWidth)
  HWND hEdit;
  int *pHeight;
  int *pWidth;
{
  RECT r;
  BUFFER *b;

  b = EditHwndToBuffer(hEdit);
  GetClientRect(hEdit, &r);

  if (pHeight)
  {
    *pHeight = RECT_HEIGHT(r) / (b->yFontHeight + b->yFontLeading);
    if (*pHeight <= 0)
      *pHeight = 1;
  }
  if (pWidth)
  {
#ifdef MEWEL_GUI
    *pWidth  = (RECT_WIDTH(r) - (b->xFontWidth/2)) / b->xFontWidth;
#else
    *pWidth  = RECT_WIDTH(r) / b->xFontWidth;
#endif
    if (*pWidth <= 0)
      *pWidth = 1;
  }
}


INT FAR PASCAL EditBufferCreate(hEdit, id, style)
  HWND hEdit;
  INT  id;
  DWORD style;
{
  BUFFER *b;
  WINDOW *w;
  UINT   nAllocSize;
  BOOL   bIsSingleLine;

  if ((w = WID_TO_WIN(hEdit)) == NULL)
    return FALSE;


  if ((b = (BUFFER *) emalloc(sizeof(BUFFER))) != NULL)
  {
    /*
      The window's pPrivate member points to the buffer structure
    */
    w->pPrivate = (PSTR) b;

    EditSetFont(hEdit);

#if 0
#ifdef MEWEL_GUI
    bIsSingleLine = (BOOL) (w->rClient.top + b->yFontHeight >= w->rClient.bottom);
#else
    bIsSingleLine = (BOOL) (RECT_HEIGHT(w->rClient) == 1);
#endif
#else
    bIsSingleLine = (BOOL) ((w->flags & ES_MULTILINE) == 0);
#endif

    if (!bIsSingleLine)
      nAllocSize = EditBufferSize;
    else if (style & ES_AUTOHSCROLL)
      nAllocSize = 256;
    else
    {
#ifdef MEWEL_GUI
      nAllocSize = (WIN_CLIENT_WIDTH(w) + 7) / b->xFontWidth + 1;
#else
      nAllocSize = WIN_CLIENT_WIDTH(w) + 1;
#endif
#if defined(__TURBOC__)
      nAllocSize += 4;  /* get around RTL bug in strchr in prot mode */
#endif
    }


    /*
      Allocate global memory to hold the edit buffer. This is different
      from Windows, which allocates memory from the local heap, but
      allocating from the global heap allows us to use XMS, EMS, etc.
    */
    if ((b->hText = EDITORAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) nAllocSize)) == 0)
    {
      MyFree(b);
      return FALSE;
    }

    b->currlinenum = b->lastlnum = b->topline = 1;
    b->nTotalAllocated = nAllocSize;

    /*
      Set the initial limit on the number of characters which can be
      entered. We only do this for a non-scrolling single-line edit field.
    */
    b->nMaxLimit = (bIsSingleLine && !(style&ES_AUTOHSCROLL)) ? nAllocSize-1
                                                              : 0;
    b->idCtrl = id;
    b->style  = style;
    b->iTabSize = 8;
    b->fFlags = STATE_INSERT;
#ifndef OVERTYPE_DEFAULT
    b->fFlags = STATE_INSERT;   /* rsm - 7/91 */
#else
    b->fFlags = 0;
#endif
    b->chPassword = '*';

    if (!bIsSingleLine)
      b->style |= ES_MULTILINE;
    else
      b->style &= ~(ES_AUTOVSCROLL);

    return TRUE;
  }
  else
    return FALSE;
}



/****************************************************************************/
/*                                                                          */
/* Function : EditSetFont(HWND hEdit)                                       */
/*                                                                          */
/* Purpose  : Internal func to sync the edit's font info                    */
/*                                                                          */
/* Returns  : Nothing                                                       */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EditSetFont(hEdit)
  HWND hEdit;
{
#ifdef MEWEL_GUI
  HDC   hDC;
  TEXTMETRIC tm;
  HFONT hFont = NULL, 
        hOldFont;
#endif
  BUFFER *b;
  RECT   rClient;

  b = EditHwndToBuffer(hEdit);

#ifdef MEWEL_GUI
  hDC = GetDC(hEdit);
  if (b->xFontWidth != 0)  /* don't send WM_GETFONT on initialization */
    if ((hFont = (HFONT) SendMessage(hEdit, WM_GETFONT, 0, 0L)) != NULL)
      hOldFont = SelectObject(hDC, hFont);
  GetTextMetrics(hDC, &tm);
  if (hFont)
    SelectObject(hDC, hOldFont);
  ReleaseDC(hEdit, hDC);

  b->xFontWidth   = tm.tmAveCharWidth;
  b->yFontHeight  = tm.tmHeight;
  b->yFontLeading = tm.tmExternalLeading;
#else
  b->xFontWidth   = 1;
  b->yFontHeight  = 1;
  b->yFontLeading = 0;
#endif

  GetClientRect(hEdit, &rClient);
  b->nLinesInWindow = rClient.bottom / (b->yFontHeight + b->yFontLeading);
  if (b->nLinesInWindow <= 0)
    b->nLinesInWindow = 1;
}



/****************************************************************************/
/*                                                                          */
/* Function : _EditReallocate(BUFFER *, UINT)                               */
/*                                                                          */
/* Purpose  : Internal func to reallocate the edit control's buffer.        */
/*                                                                          */
/* Returns  : TRUE if reallocated, FALSE if not.                            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL _EditReallocate(pBuffer, wNewLen)
  BUFFER *pBuffer;
  UINT   wNewLen;
{
  UINT   nTotal;
  UINT   nLocks;
  HANDLE hNew;

  nTotal = max(wNewLen, pBuffer->nTotalAllocated * 2); /* double the size */
 
  /*
    Make sure that the edit buffer is fully unlocked and all current contents
    written back to virtual memory.
  */
  nLocks = (EDITORFlags(pBuffer->hText) & 0x00FF);
  while (nLocks-- > 0)
    EDITORUnlock(pBuffer->hText);

  if ((hNew = EDITORReAlloc(pBuffer->hText, (DWORD)nTotal, GMEM_MOVEABLE)) != 0)
  {
    pBuffer->hText  = hNew;
    /*
      Make a copy so that something resides in virtual memory.
    */
    pBuffer->szText = EDITORLock(hNew);
    EDITORUnlock(hNew);
    /*
      We must re-lock the block since the routines which call _EditReallocate
      expect a valid pointer in b->szText
    */
    pBuffer->szText = EDITORLock(hNew);
    pBuffer->nTotalAllocated = nTotal;
    return TRUE;
  }
  else
    return FALSE;
}


/* bytedel - slide the text leftwards from pos by n characters */
VOID FAR PASCAL bytedel(line, n, pos)
  LPSTR line;
  EPOS  n, pos;
{
  LPSTR s, t;

  s = line+pos;
  t = s+n;
  while ((*s++ = *t++) != '\0') ;
}

VOID FAR PASCAL byteinsert(pos, count)
  LPSTR pos;
  EPOS count;
{
  LPSTR begin, end;

  begin = pos + lstrlen(pos);
  end = begin + count;
  while (begin >= pos)
    (*end--) = (*begin--);
}


int FAR PASCAL EditInsertBlock(hEdit, szText)
  HWND hEdit;
  LPSTR szText;
{
  BUFFER *b;
  LPSTR  s, s2;
  EPOS   iLen;
  EPOS   nMaxLimit, nLine, nPos;

  if (szText == NULL || (b = EditHwndToBuffer(hEdit)) == NULL)
    return FALSE;

  /*
    If an area is selected and we insert a block, erase the selected
    area first....
  */
  if (b->fFlags & STATE_SOMETHING_SELECTED)
    EditCopySelection(hEdit,TRUE,FALSE); /* delete do not copy to scrap */

  iLen = lstrlen(szText);
  nMaxLimit = b->nMaxLimit;

  if ((b->nTotalBytes + iLen) >= b->nTotalAllocated-1 ||
      (nMaxLimit > 0 && (b->nTotalBytes + iLen) >= nMaxLimit/*-1*/))
  {
    if (!_EditReallocate(b, b->nTotalBytes+iLen+1))
      return FALSE;
  }

  s = b->szText + b->iCurrPosition;

  /*
    Move the ending stuff down
  */
  s2 = b->szText + b->nTotalBytes;
  lmemshr(s+iLen, s, s2-s+1);
  /*
    Now stuff the new stuff in
  */
  lmemcpy(s, szText, iLen);
  b->nTotalBytes += iLen;

  /*
    Windows leaves the caret at the letter which comes right after
    the end of the pasted text.
  */
  b->iCurrPosition += iLen;

#ifdef EDIT_REFORMAT
  _EditReformat(hEdit);
#endif

  /*
    We want to recalc the number of lines
  */
  s = b->szText;
  for (nPos = 0, nLine = 1;  s;  )
  {
    if ((s = lmemchr(s, CH_NEWLINE, b->nTotalBytes - nPos)) != NULL)
    {
      s++, nLine++;
#ifdef CH_WORDWRAP
      if (*s == CH_WORDWRAP)   /* Go past the wordwrap character */
        s++;
#endif
      nPos = s - b->szText;
    }
  }

  b->lastlnum = nLine;
  b->dwModify++;
  return TRUE;
}


int FAR PASCAL _EditIsDirectionKey(iKey)
  UINT iKey;
{
  switch (iKey)
  {
    case VK_UP        : 
    case VK_DOWN      :
    case VK_LEFT      :
    case VK_RIGHT     :
    case VK_NEXT      :
    case VK_PRIOR     :
    case VK_HOME      :
    case VK_END       :

#if !defined(USE_WINDOWS_COMPAT_KEYS)
    case VK_CTRL_LEFT :
    case VK_CTRL_RIGHT:
    case VK_CTRL_HOME :
    case VK_CTRL_END  :
#endif

      return TRUE;

    default :
      return FALSE;
  }
}


VOID PASCAL InvalidateLine(hEdit, wArea, iStartCol)
  HWND hEdit;
  INT  wArea;      /* REDRAW_LINE or REDRAW_EOW */
  INT  iStartCol;  /* either 0 or -1 */
{
  BUFFER *b = EditHwndToBuffer(hEdit);
  RECT   r;

  (void) iStartCol;

  GetClientRect(hEdit, (LPRECT) &r);
  r.top    = b->cursor.row * (b->yFontHeight + b->yFontLeading);
  if (wArea == REDRAW_LINE)
  {
#ifdef MEWEL_GUI
    /*
      For a multiline edit control, only invalidate the area taken up
      by the font. For a single line control, invalidate the entire
      height of the edit control, since EditShow() places the text
      center-justified within the entire control.
    */
    if (WinGetFlags(hEdit) & ES_MULTILINE)
      r.bottom = r.top + (b->yFontHeight + b->yFontLeading) - 1;

    /*
      If we passed -1 as the starting invalid column, then invalidate
      everything from the current column (minus 1) and to the right.
    */
#if defined(GX) || 1
    if (iStartCol == -1)
    {
      RECT rUpdate;

      /*
        If the edit window does not have an update area, then we
        can do this special processing.
      */
      if (!GetUpdateRect(hEdit, &rUpdate, FALSE))
      {
        if ((iStartCol = b->cursor.column - 1) > 0)
        {
          LPSTR szLine = _EditGetLine(hEdit, b->currlinenum, NULL);
          LPSTR szEnd;
          BYTE  chSave;
          int   iWidth;

          /*
            Temporarily put an end-of-line marker at the current position
            in order for GetTextExtent to work.
          */
          szEnd  = szLine + iStartCol;
          chSave = *szEnd;
          *szEnd = '\0';

          /*
            The left side of the update rectangle starts at the current
            character. Use GetTextExtent to account for proportional
            characters. We also add an offset of (b->xFontWidth/2) because
            this is the left margin that EditShow uses.
          */
          iWidth = LOWORD(_GetTextExtent(szLine + b->iHscroll));
#if defined(BGI)
          /*
            BGI stroked font kludge.
            If we have a string which contains small characters like
            't', 'l', 'f', and 'i', then the string will not be
            displayed correctly. As a kludge, assume that all characters
            in the string are small, and refresh more of the string.
          */
          if (iWidth < iStartCol * b->xFontWidth)
            iWidth = iStartCol * textwidth("l");
#endif
          r.left = iWidth + (b->xFontWidth/2);

          /*
            Get rid of the temp end-of-line marker
          */
          *szEnd = chSave;
        }
      }
    }
#endif

#else /* TEXT MODE */
    r.bottom = r.top+1;
#endif
  }
  InvalidateRect(hEdit, (LPRECT) &r, FALSE);
}



/****************************************************************************/
/*                                                                          */
/* Function : EditMouseToChar()                                             */
/*                                                                          */
/* Purpose  : Given client-relative mouse pixel coordinates, returns the    */
/*            character-based row and column where the mouse movement       */
/*            occurred.                                                     */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL EditMouseToChar(HWND hEdit, BUFFER *b, 
                                MWCOORD xMouse, MWCOORD yMouse, 
                                MWCOORD *piXRet, MWCOORD *piYRet)
{
#if defined(XWINDOWS)
  int iWidth;
  HDC hDC;
  hDC = GetDC(hEdit);
#endif

  /*
    For a single-line edit field, force the mouse to row 0.
  */
  if (!(WinGetFlags(hEdit) & ES_MULTILINE))
    yMouse = 0;
  else
    yMouse /= (b->yFontHeight + b->yFontLeading);

  if (piYRet)
    *piYRet = yMouse;


  if (piXRet)
  {
    /*
      Compensate for the 1/2-charwidth margin
    */
    xMouse -= (b->xFontWidth / 2);
    if (xMouse < 0)
    {
      xMouse = 0;
    }
    else
    {
      LPSTR szLine, szVisible;
      EPOS  iPos, x;
      INT   c;
      char  s[2];

      /*
        Get a pointer to the left window column of the current line.
      */
      if ((szLine = (LPSTR)_EditGetLine(hEdit,b->topline+yMouse,NULL)) == NULL)
        return;
      szVisible = szLine + b->iHscroll;
      iPos = x = 0;
      s[1] = '\0';

      /*
        Starting at the beginning of the line, go through the line until we
        reach a letter which is displayed just past the mouse cursor.
      */
      while ((c = *szVisible++) != '\0' && c != CH_NEWLINE && c != CH_WORDWRAP)
      {
        if ((INT) x > xMouse)
          break;

        if (c == '\t')
        {
          /*
            If we have a tab, then advance 'x' to the next tabstop. 
            Incompatibility note :
              We should really use the Windows-compatible array of
              tabs, and thus, use GetTabbedTextExtent().
          */
          x += (b->iTabSize - (x % b->iTabSize));
        }
        else
        {
#if defined(MEWEL_GUI)
#if defined(XWINDOWS)
          GetCharWidth(hDC, c, c, &iWidth);
          x += iWidth;
#else
          s[0] = (BYTE) c;
          x += textwidth(s);
#endif
#else
          x++;
#endif
        }
        iPos++;
      }

      /*
        If we have not reached the end of the line, and we went
        past the mouse cursor position, then move back a letter.
        If we reached the end of the line, it means that we clicked
        the mouse past the last character of the line, and we should
        move the cursor to the end of the line.
      */
      xMouse = iPos;
      if (c != '\0')
        xMouse--;
    }

    *piXRet = xMouse;
  }

#if defined(XWINDOWS)
  ReleaseDC(hEdit, hDC);
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : EditGetCurrPosExtent()                                        */
/*                                                                          */
/* Purpose  : Determines the x coordinate (in pixels) where the caret       */
/*            starts. Also determines the width of the caret.               */
/*                                                                          */
/* Returns  : The x coordinates (in pixels) where the current character is. */
/*            This will also be the position where the caret starts.        */
/*                                                                          */
/* Called by: _EditPositionCursor()                                         */
/*                                                                          */
/****************************************************************************/
#if defined(MEWEL_GUI)
INT PASCAL EditGetCurrPosExtent(hEdit, b, pcxChar)
  HWND   hEdit;
  BUFFER *b;
  INT    *pcxChar;
{
  LPSTR szLine, szVisible, pCurrChar;
  EPOS  wExtent;
  BYTE  chSave;
  HDC   hDC;
  HFONT hFont, hOldFont;


  /*
    Get a device context and the font information.
  */
  hDC = GetDC(hEdit);
  if ((hFont = (HFONT) SendMessage(hEdit, WM_GETFONT, 0, 0L)) != NULL)
    hOldFont = SelectObject(hDC, hFont);

  /*
    Lock the buffer and get a pointer to the current line.
  */
  b->szText = (LPSTR) EDITORLock(b->hText);
  szLine = (LPSTR) _EditGetLine(hEdit, b->currlinenum, &wExtent);

  /*
    Get the part of the line which is visible starting at the left margin
    of the edit control. Also, get a pointer to the current character.
  */
  szVisible = szLine + b->iHscroll;
  pCurrChar = szLine + b->cursor.column;

  /*
    Get the length of the visible part of the string, up until the
    current position. This is the x position of the caret.
  */
  chSave = *pCurrChar;
  *pCurrChar = '\0';
  wExtent = LOWORD(GetTabbedTextExtent(hDC, (LPCSTR) szVisible, 
                        b->cursor.column - b->iHscroll, 1, &b->iTabSize));
  *pCurrChar = chSave;

  /*
    Get the width of the current character. This is how wide we should
    make the caret.
  */
  if (pcxChar)
  {
    if (chSave == '\0')
      *pcxChar = b->xFontWidth;
    else
    {
      char s[2];
      s[0] = (BYTE) chSave;
      s[1] = '\0';
      *pcxChar = (INT) LOWORD(_GetTextExtent(s));
    }
  }

  /*
    Clean up. Unlock the text buffer, select the old font, and free the DC.
  */
  EDITORUnlock(b->hText);
  if (hFont)
    SelectObject(hDC, hOldFont);
  ReleaseDC(hEdit, hDC);

  return (INT) wExtent;
}
#endif


static EPOS PASCAL WordwrapCheck(BUFFER *b, EPOS iPos, BOOL bWrap)
{
  if (bWrap)
    iPos++;

  /*
    If we are at the right side of a \n\r pair, put the cursor over
    the \n
  */
  if (b->szText[iPos] == CH_WORDWRAP)
    iPos--;

  return iPos;
}

