/*===========================================================================*/
/*                                                                           */
/* File    : WINEDIT.H                                                       */
/*                                                                           */
/* Purpose : Implements the edit class.                                      */
/*                                                                           */
/* History : July, 1991 - modified by Rachel McKenzie.                       */
/*           Added OVERTYPE_DEFAULT switch which causes the edit control to  */
/*            default to overtype instead of insert mode.                    */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"

#ifndef NOCLIPBOARD
#define CLIPBOARD
#endif

#define WM_SETTEXT_STATIC   (WM_USER+201)

#define WAGNER

/*
  EPOS is a typedef used for editing positions. If we wanted to break
  the 64K editing limit under DOS, then part of it would be to change EPOS
  to DWORD.
*/
typedef UINT  EPOS;

typedef struct pt
{
  int  row;
  int  column;
} PT;

typedef struct marker
{
  PT   ptStartMark;
  PT   ptEndMark;
} MARKER;

typedef struct buffer
{
  int  idCtrl;                  /* the control identifier */
#ifdef MEM_MACROS
  GLOBALHANDLE hText;
#else
  HANDLE hText;
#endif
  LPSTR szText;

  int  currlinenum;
  int  lastlnum;
  int  topline;

  EPOS nTotalBytes;             /* Total # of chars in the buffer   */
  EPOS iCurrPosition;           /* 0-based current position within buffer */
  EPOS nTotalAllocated;         /* Memory allocated for this buffer */
  EPOS nMaxLimit;               /* set by EM_SETLIMIT - 0 if not set */

  EPOS iposStartMark,
       iposEndMark;

  EPOS iHscroll;                /* horizontal scroll factor */
  PT   cursor;

  HWND hHorizSB;
  HWND hVertSB;

  DWORD style;

  UINT fFlags;
#define STATE_INSERT              0x01
#define STATE_DIRTY               0x02
#define STATE_SOMETHING_SELECTED  0x04
#define STATE_WORDWRAPPING        0x08
#define STATE_HASFOCUS            0x10
#define STATE_HASCURSOR           0x20
#define STATE_LIMITSET            0x40
#define STATE_EM_FMTLINES         0x80   /*tw*/
#define STATE_SYNCCURSOR_POSTED   0x100
#define STATE_SCROLLING           0x200  /* just got WM_H/VSCROLL  */
#define STATE_NO_SYNCCURSOR       0x400  /* don't post WM_SYNCCURSOR */

  UINT fRedrawFlags;
#define NO_REDRAW       0x00
#define REDRAW_WINDOW   0x01
#define REDRAW_EOW      0x02
#define REDRAW_LINE     0x04
#define REDRAW_STATIC   0x08    /* rsm */
#define REDRAW_BARS     0x10    

  MARKER marker;
  BYTE   chPassword;
  INT    iTabSize;
  DWORD  dwModify;

  INT    xFontWidth, yFontHeight, yFontLeading;
  INT    nLinesInWindow;
} BUFFER;


#ifdef WAGNER
#define SCROLL_AMOUNT   1
VOID FAR PASCAL _EditReformatAll(HWND hEdit);
#else
#define SCROLL_AMOUNT   0
#endif
#define CH_NEWLINE     '\n'
#define CH_WORDWRAP    '\r'

#ifdef CH_WORDWRAP
#define EDIT_REFORMAT   1
extern UINT FAR PASCAL _EditReformat(HWND);
#endif

extern VOID FAR PASCAL _EditPositionCursor(HWND,struct buffer *);
extern VOID FAR PASCAL EditShow(HWND, HDC, LPRECT);
extern VOID FAR PASCAL EditSetCursor(HWND);
extern int FAR PASCAL EditRight(HWND);
extern int FAR PASCAL EditLeft(HWND);
extern int FAR PASCAL EditUp(HWND);
extern int FAR PASCAL EditDown(HWND);
extern int FAR PASCAL EditNextWord(HWND);
extern int FAR PASCAL EditPrevWord(HWND);
extern int FAR PASCAL EditPageDown(HWND);
extern int FAR PASCAL EditPageUp(HWND);
extern int FAR PASCAL EditBeginningOfLine(HWND);
extern int FAR PASCAL EditEndOfLine(HWND);
extern int FAR PASCAL EditBeginningOfBuffer(HWND);
extern int FAR PASCAL EditEndOfBuffer(HWND);
extern int FAR PASCAL EditGoto(HWND,int,int);
extern int FAR PASCAL EditScroll(HWND,int,int);
extern int FAR PASCAL EditSelect(HWND,int);
extern int FAR PASCAL EditClearSelect(HWND);
extern int FAR PASCAL EditCopySelection(HWND,BOOL,BOOL);
extern int FAR PASCAL EditPaste(HWND);
extern int FAR PASCAL EditToggleInsert(HWND);
extern int FAR PASCAL EditDeleteChar(HWND);
extern int FAR PASCAL EditBackspace(HWND);
extern int FAR PASCAL EditInsertChar(HWND,LPSTR);
extern int FAR PASCAL EditIsBOF(HWND);
extern int FAR PASCAL EditIsEOF(HWND);
extern int FAR PASCAL EditIsBOL(HWND);
extern INT   FAR PASCAL EditIsEOL(HWND);
extern LPSTR FAR PASCAL _EditFindEndOfPrevLine(LPSTR,EPOS*);
extern LPSTR FAR PASCAL _EditFindStartOfNextLine(LPSTR,EPOS*);
extern LPSTR FAR PASCAL _EditGetLine(HWND,int,EPOS*);
extern EPOS  FAR PASCAL _EditIndexToLine(HWND,EPOS,EPOS*);
extern BUFFER* FAR PASCAL EditHwndToBuffer(HWND);
extern INT   FAR PASCAL _EditNotifyParent(HWND,UINT);
extern VOID  FAR PASCAL _EditGetDimensions(HWND,int *,int *);
extern INT   FAR PASCAL EditBufferCreate(HWND,INT,DWORD);
extern INT   FAR PASCAL EditInsertBlock(HWND ,LPSTR);
extern INT   FAR PASCAL _EditIsDirectionKey(UINT);
extern BOOL  FAR PASCAL _EditReallocate(BUFFER *, UINT);
extern VOID      PASCAL InvalidateLine(HWND, INT, INT);
extern DWORD FAR PASCAL _GetTextExtent(LPCSTR);
extern VOID  FAR PASCAL EditMouseToChar(HWND,BUFFER*,MWCOORD,MWCOORD,MWCOORD*,MWCOORD*);
extern VOID      PASCAL EditSyncCursor(HWND, BUFFER *);
extern VOID  FAR PASCAL EditSetFont(HWND);

/*
  In MEWEL we use GlobalAlloc for the editor buffer, unless we want
  extreme Windows compatibility, where in that case, we use LocalAlloc.
*/
#if defined(__DPMI16__) && defined(__DLL__)
#define EDITORAlloc(f, n)          LocalAlloc(f, (UINT) (n))
#define EDITORReAlloc(h, n, f)     LocalReAlloc(h, (UINT) (n), f)
#define EDITORLock(h)              LocalLock(h)
#define EDITORUnlock(h)            LocalUnlock(h)
#define EDITORFree(h)              LocalFree(h)
#define EDITORFlags(h)             LocalFlags(h)
#define EDITORSize(h)              LocalSize(h)
#else
#define EDITORAlloc(f, n)          GlobalAlloc(f, (DWORD) (n))
#define EDITORReAlloc(h, n, f)     GlobalReAlloc(h, (DWORD) (n), f)
#define EDITORLock(h)              GlobalLock(h)
#define EDITORUnlock(h)            GlobalUnlock(h)
#define EDITORFree(h)              GlobalFree(h)
#define EDITORFlags(h)             GlobalFlags(h)
#define EDITORSize(h)              GlobalSize(h)
#endif

