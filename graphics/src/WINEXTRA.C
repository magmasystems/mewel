/*===========================================================================*/
/*                                                                           */
/* File    : WINEXTRA.C                                                      */
/*                                                                           */
/* Purpose : Routines to handle the querying/setting of window-extra bytes.  */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include <stdlib.h>  /* for offsetof() */

typedef struct extra
{
  UINT nBytes;      /* the first 2 bytes of the extra data is the length */
  char szData[1];   /* the actual data */
} EXTRA;

#if !defined(offsetof)
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

static PSTR PASCAL GetWindowExtraAddr(HWND, int);


BOOL FAR PASCAL SetWindowExtra(hWnd, nBytes)
  HWND hWnd;
  int  nBytes;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  PSTR   pOldExtra;

  if (w == NULL)
    return FALSE;
  pOldExtra = w->pWinExtra;

  if ((w->pWinExtra = emalloc(nBytes + offsetof(EXTRA, szData))) == NULL)
  {
    w->pWinExtra = pOldExtra;
    return FALSE;
  }

  ((EXTRA *) w->pWinExtra)->nBytes = nBytes;

  if (pOldExtra)
    MyFree(pOldExtra);
  return TRUE;
}


WORD FAR PASCAL GetWindowWord(hWnd, iWord)
  HWND hWnd;
  int  iWord;
{
  USHORT  *pExtra;
  WINDOW  *w = WID_TO_WIN(hWnd);

  if (w == NULL)
    return FALSE;

  switch (iWord)
  {
    case GWW_HINSTANCE:       /* Instance handle of the window */
      return w->hInstance;

    case GWW_HWNDPARENT:      /* Handle of the parent window, if any */
      return w->parent->win_id;

    case GWW_ID:              /* Control ID of the child window */
      return w->idCtrl;

    default:
      break;
  }

  pExtra = (USHORT *) GetWindowExtraAddr(hWnd, iWord);
  return (pExtra) ? *pExtra : 0;
}


LONG FAR PASCAL GetWindowLong(hWnd, iLong)
  HWND hWnd;
  int  iLong;
{
  LONG    *pExtra;
  WINDOW  *w = WID_TO_WIN(hWnd);

  if (w == NULL)
    return FALSE;

  switch (iLong)
  {
    case GWL_EXSTYLE:           /* Extended window style */
      return(w->dwExtStyle);

    case GWL_STYLE:             /* Window style */
      return(w->flags);

    case GWL_WNDPROC:           /* Long pointer to the window function */
      return((LONG) w->winproc);

    case DWL_MSGRESULT:
    case DWL_DLGPROC  :
    case DWL_USER     :
      if (IS_DIALOG(w))
        return DlgGetSetWindowLong(hWnd, iLong, 0, FALSE);
      /* fall through ... */

    default:
      break;
  }

  pExtra = (LONG *) GetWindowExtraAddr(hWnd, iLong);
  return (pExtra) ? *pExtra : 0L;
}


WORD FAR PASCAL SetWindowWord(HWND hWnd, int iWord, WORD word)
{
  USHORT  *pExtra;
  WINDOW  *w = WID_TO_WIN(hWnd);
  WORD    wOldWord;

  if (w == NULL)
    return FALSE;

  switch (iWord)
  {
    case GWW_HINSTANCE:       /* Instance handle of the window */
      wOldWord = w->hInstance;
      w->hInstance = word;
      return wOldWord;

    case GWW_HWNDPARENT:      /* Handle of the parent window, if any */
      wOldWord          = w->parent->win_id;
      w->parent->win_id = word;
      return wOldWord;

    case GWW_ID:              /* Control ID of the child window */
      wOldWord = w->idCtrl;
      w->idCtrl = word;
      return wOldWord;

    default:
      pExtra = (USHORT *) GetWindowExtraAddr(hWnd, iWord);
      if (pExtra)
      {
        wOldWord = (WORD) *pExtra;
        *pExtra = (USHORT) word;
        return wOldWord;
      }
      else
        return (WORD) 0;
  }
}


LONG FAR PASCAL SetWindowLong(hWnd, iLong, llong)
  HWND hWnd;
  int  iLong;
  LONG llong;
{
  LONG    *pExtra;
  WINDOW  *w = WID_TO_WIN(hWnd);
  LONG    lOldLong;

  if (w == NULL)
    return FALSE;

  switch (iLong)
  {
    case GWL_EXSTYLE:               /* Extended window style */
      lOldLong    = w->dwExtStyle;
      w->dwExtStyle  = llong;
      return lOldLong;

    case GWL_STYLE:                 /* Window style */
      lOldLong    = w->flags;
      w->flags    = llong;
      return lOldLong;

    case GWL_WNDPROC:               /* Long ptr to the window function */
      lOldLong    = (LONG) w->winproc;
      w->winproc  = (WINPROC *) llong;
      return lOldLong;

    case DWL_MSGRESULT:
    case DWL_DLGPROC  :
    case DWL_USER     :
      if (IS_DIALOG(w))
        return DlgGetSetWindowLong(hWnd, iLong, llong, TRUE);
      /* fall through ... */

    default:
      break;
  }

  pExtra = (LONG *) GetWindowExtraAddr(hWnd, iLong);
  if (pExtra)
  {
    lOldLong = *pExtra;
    *pExtra = llong;
    return lOldLong;
  }
  else
    return 0L;
}


static PSTR PASCAL GetWindowExtraAddr(hWnd, index)
  HWND hWnd;
  int  index;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  PSTR pExtra = (w && w->pWinExtra) ? ((EXTRA *) w->pWinExtra)->szData : NULL;

  if (pExtra)
  {
    EXTRA *ex = (EXTRA *) (pExtra - offsetof(EXTRA, szData)); 
    if (index >= (int) (ex->nBytes))
      return NULL;
    else
      return ex->szData + index;
  }
  else
    return NULL;
}

