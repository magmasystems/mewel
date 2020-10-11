/*===========================================================================*/
/*                                                                           */
/* File    : WCLIPBRD.C                                                      */
/*                                                                           */
/* Purpose : MS Windows-like clipboard functions for MEWEL.                  */
/*                                                                           */
/* History : 7/90 (maa) Created for use in MULTIPAD port                     */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


#ifndef CF_TEXT
#define CF_TEXT     1
#endif

CLIPBOARD _Clipboard =
{
  (GLOBALHANDLE) 0, 0, NULLHWND, NULLHWND, NULLHWND, 0x0000, 1, 
     { CF_TEXT, (LPSTR) "CF_TEXT", 0 }
};


/*
extern BOOL FAR PASCAL CloseClipboard();
extern BOOL FAR PASCAL EmptyClipboard();
extern UINT FAR PASCAL EnumClipboardFormats(UINT);
extern MEMHANDLE FAR PASCAL GetClipboardData(UINT);
extern INT FAR PASCAL GetClipboardFormatName(UINT, PSTR, INT);
extern HWND FAR PASCAL GetClipboardOwner();
extern HWND FAR PASCAL GetClipboardViewer();
extern HWND WINAPI GetOpenClipboardWindow(void);
extern BOOL FAR PASCAL IsClipboardFormatAvailable(UINT);
extern BOOL FAR PASCAL OpenClipboard(HWND);
extern MEMHANDLE FAR PASCAL SetClipboardData(UINT, MEMHANDLE);
extern HWND FAR PASCAL SetClipboardViewer(HWND);
*/

extern BOOL FAR PASCAL IsClipboardFormatRegistered(UINT);



BOOL FAR PASCAL ChangeClipboardChain(hWnd, hWndNext)
  HWND hWnd, hWndNext;
{
  if (_Clipboard.hWndViewer != hWnd)
    return FALSE;

  SendMessage(hWnd, WM_CHANGECBCHAIN, hWnd, (LONG) hWndNext);
  _Clipboard.hWndViewer = hWndNext;

  return TRUE;
}

BOOL FAR PASCAL CloseClipboard(void)
{
  _Clipboard.fFlags &= ~CLIP_OPEN;
  _Clipboard.hWndOpen = NULLHWND;
  return TRUE;
}

INT FAR PASCAL CountClipboardFormats(void)
{
  return _Clipboard.nFormats;
}

BOOL FAR PASCAL EmptyClipboard(void)
{
  if (_Clipboard.hData && (_Clipboard.fFlags & CLIP_OPEN))
  {
    /*
      Inform the old owner that it's memory is being freed
    */
    if (_Clipboard.hWndOwner)
      SendMessage(_Clipboard.hWndOwner, WM_DESTROYCLIPBOARD, 0, 0L);
    GlobalFree(_Clipboard.hData);
    _Clipboard.hWndOwner = _Clipboard.hWndOpen;  /* assign ownership */
    _Clipboard.hData = (GLOBALHANDLE) 0;
  }
  return TRUE;
}

UINT FAR PASCAL EnumClipboardFormats(wFormat)
  UINT wFormat;
{
  INT  idxFmt;

  if (wFormat == 0)
    return _Clipboard.fmtInfo[0].idFormat;

  idxFmt = IsClipboardFormatRegistered(wFormat);
  if (idxFmt && idxFmt < (INT) (_Clipboard.nFormats - 1))
    return _Clipboard.fmtInfo[idxFmt+1].idFormat;
  else
    return 0;
}

GLOBALHANDLE FAR PASCAL GetClipboardData(wFormat)
  UINT wFormat;
{
  if (!IsClipboardFormatRegistered(wFormat))
    return (GLOBALHANDLE) 0;
  if (_Clipboard.hData == NULL)
    SendMessage(_Clipboard.hWndOwner, WM_RENDERFORMAT, wFormat, 0L);
  return _Clipboard.hData;
}

INT FAR PASCAL GetClipboardFormatName(wFormat, lpFormatName, nMax)
  UINT  wFormat;
  LPSTR lpFormatName;
  INT   nMax;
{
  if (wFormat == CF_OWNERDISPLAY)
    return (int) SendMessage(_Clipboard.hWndOwner, WM_ASKCBFORMATNAME,
                             nMax, (LONG) lpFormatName);

  if (wFormat >= 1 && wFormat <= _Clipboard.nFormats &&
                      _Clipboard.fmtInfo[wFormat-1].pszFormat)
  {
    return lstrlen(lstrncpy(lpFormatName,      
                  _Clipboard.fmtInfo[wFormat-1].pszFormat, nMax));
  }
  else
    return 0;
}

HWND FAR PASCAL GetClipboardOwner(void)
{
  return _Clipboard.hWndOwner;
}

HWND FAR PASCAL GetClipboardViewer(void)
{
  return _Clipboard.hWndViewer;
}

INT FAR PASCAL GetPriorityClipboardFormat(lpPriorityList, nCount)
  LPUINT lpPriorityList;
  INT    nCount;
{
  INT    i;

  /*
    If there's no data in the clipboard, return NULL
  */
  if (!_Clipboard.hData)
    return 0;

  for (i = 0;  i < nCount;  i++, lpPriorityList++)
  {
    if (IsClipboardFormatAvailable(*lpPriorityList))
      return *lpPriorityList;
  }

  /*
    No match? Return -1.
  */
  return -1;
}

BOOL FAR PASCAL IsClipboardFormatAvailable(wFormat)
  UINT wFormat;
{
  return (BOOL) (_Clipboard.hData && _Clipboard.fmtData == wFormat);
}

BOOL FAR PASCAL IsClipboardFormatRegistered(wFormat)
  UINT wFormat;
{
  INT  i;

  for (i = 0;  i < (INT) _Clipboard.nFormats;  i++)
    if (_Clipboard.fmtInfo[i].idFormat == wFormat)
      return (BOOL) (i+1);
  return (BOOL) 0;
}

BOOL FAR PASCAL OpenClipboard(hWnd)
  HWND hWnd;
{
  if (_Clipboard.fFlags & CLIP_OPEN)
    return FALSE;

  _Clipboard.hWndOpen = hWnd;
  _Clipboard.fFlags |= CLIP_OPEN;
  return TRUE;
}

UINT FAR PASCAL RegisterClipboardFormat(lpszFmt)
  LPCSTR lpszFmt;
{
  if (_Clipboard.nFormats < MAXFORMATS)
  {
    int iFmt;

    /*
      See if the format name exists already. if it does, then just
      bump up its reference count.
    */
    for (iFmt = 0;  iFmt < (int) _Clipboard.nFormats;  iFmt++)
      if (!lstricmp(_Clipboard.fmtInfo[iFmt].pszFormat, (LPSTR) lpszFmt))
      {
        _Clipboard.fmtInfo[iFmt].iRefCount++;
        return CF_TEXT + iFmt;
      }

    iFmt = _Clipboard.nFormats++;
    _Clipboard.fmtInfo[iFmt].pszFormat = lstrsave((LPSTR) lpszFmt);
    _Clipboard.fmtInfo[iFmt].idFormat  = CF_TEXT + iFmt;
    _Clipboard.fmtInfo[iFmt].iRefCount = 0;
    return CF_TEXT + iFmt;
  }
  else
    return 0;
}

GLOBALHANDLE FAR PASCAL SetClipboardData(wFormat, hMem)
  UINT      wFormat;
  GLOBALHANDLE hMem;
{
  if (!IsClipboardFormatRegistered(wFormat))
    return (GLOBALHANDLE) 0;

  _Clipboard.fmtData = wFormat;
  _Clipboard.hData = hMem;
  SendMessage(_Clipboard.hWndOwner, WM_DRAWCLIPBOARD, 0, 0L);
  return hMem;
}

HWND FAR PASCAL SetClipboardViewer(hWnd)
  HWND hWnd;
{
  HWND hOldViewer = _Clipboard.hWndViewer;
  _Clipboard.hWndViewer = hWnd;
  return hOldViewer;
}

HWND WINAPI GetOpenClipboardWindow(void)
{
  return _Clipboard.hWndOpen;
}

