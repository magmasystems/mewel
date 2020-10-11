/*===========================================================================*/
/*                                                                           */
/* File    :  WGETCLAS.C                                                     */
/*                                                                           */
/* Purpose :  MS Windows compatible Get/SetClassWord/Long() functions        */
/*                                                                           */
/* History :  Created 8/14/90 (maa)                                          */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


WORD FAR PASCAL GetClassWord(hWnd, nIndex)
  HWND hWnd;
  INT  nIndex;
{
  LPEXTWNDCLASS pCl = ClassIDToClassStruct(WinGetClass(hWnd));

  if (!pCl)
    return 0;

  switch (nIndex)
  {
    case GCW_HBRBACKGROUND  :
      return pCl->hbrBackground;
    case GCW_HCURSOR	    :
      return pCl->hCursor;
    case GCW_HICON	    :
      return pCl->hIcon;
    case GCW_HMODULE	    :
      return pCl->hInstance;
    case GCW_CBWNDEXTRA	    :
      return pCl->cbWndExtra;
    case GCW_CBCLSEXTRA	    :
      return pCl->cbClsExtra;
    case GCW_STYLE	    :
      return (WORD) pCl->style;
    default                 :
      if (pCl->lpszClassExtra && nIndex <= (int) (pCl->cbClsExtra - sizeof(WORD)))
        return * (WORD *) (pCl->lpszClassExtra + nIndex);
      else
        return 0;
  }
}


WORD FAR PASCAL SetClassWord(HWND hWnd, INT nIndex, WORD iNewWord)
{
  LPEXTWNDCLASS pCl = ClassIDToClassStruct(WinGetClass(hWnd));
  WORD       wOld = 0;
  WORD       *pW;

  if (!pCl)
    return 0;

  switch (nIndex)
  {
    case GCW_HBRBACKGROUND  :
      wOld = pCl->hbrBackground;
      pCl->hbrBackground = iNewWord;
      break;
    case GCW_HCURSOR	    :
      wOld = pCl->hCursor;
      pCl->hCursor = iNewWord;
      break;
    case GCW_HICON	    :
      wOld = pCl->hIcon;
      pCl->hIcon = iNewWord;
      break;
    case GCW_HMODULE	    :
      wOld = pCl->hInstance;
      pCl->hInstance = iNewWord;
      break;
    case GCW_CBWNDEXTRA	    :
      wOld = pCl->cbWndExtra;
      pCl->cbWndExtra = iNewWord;
      break;
    case GCW_CBCLSEXTRA	    :
      wOld = pCl->cbClsExtra;
      pCl->cbClsExtra = iNewWord;
      break;
    case GCW_STYLE	    :
      wOld = (WORD) pCl->style;
      pCl->style = iNewWord;
      break;
    default :
      if (!pCl->lpszClassExtra || nIndex > (int) (pCl->cbClsExtra - sizeof(WORD)))
        return 0;
      wOld = * (WORD *) (pCl->lpszClassExtra + nIndex);
      pW = (WORD *) (pCl->lpszClassExtra + nIndex);
      *pW = iNewWord;
      break;
  }
  return wOld;
}



LONG FAR PASCAL GetClassLong(hWnd, nIndex)
  HWND hWnd;
  INT  nIndex;
{
  LPEXTWNDCLASS pCl = ClassIDToClassStruct(WinGetClass(hWnd));

  if (!pCl)
    return 0;

  switch (nIndex)
  {
    case GCL_WNDPROC        :
      return (LONG) pCl->lpfnWndProc;
    case GCL_DEFPROC        :
      return (LONG) pCl->lpfnDefWndProc;
    case GCL_MENUNAME       :
      return (LONG) pCl->lpszMenuName;
    default                 :
      if (pCl->lpszClassExtra && nIndex <= (int) (pCl->cbClsExtra - sizeof(LONG)))
        return * (LONG *) (pCl->lpszClassExtra + nIndex);
      else
        return 0;
  }
}


LONG FAR PASCAL SetClassLong(hWnd, nIndex, iNewLong)
  HWND hWnd;
  INT  nIndex;
  LONG iNewLong;
{
  LPEXTWNDCLASS pCl = ClassIDToClassStruct(WinGetClass(hWnd));
  LONG       lOld = 0L;
  LONG       *pL;

  if (!pCl)
    return 0;

  switch (nIndex)
  {
    case GCL_WNDPROC        :
      lOld = (LONG) pCl->lpfnWndProc;
      pCl->lpfnWndProc = (WINPROC *) iNewLong;
      break;
    case GCL_DEFPROC        :
      lOld = (LONG) pCl->lpfnDefWndProc;
      pCl->lpfnDefWndProc = (WINPROC *) iNewLong;
      break;
    case GCL_MENUNAME       :
      lOld = (LONG) pCl->lpszMenuName;
      pCl->lpszMenuName = (LPSTR) iNewLong;
      break;
    default                 :
      if (!pCl->lpszClassExtra || nIndex > (int) (pCl->cbClsExtra - sizeof(LONG)))
        return 0;
      lOld = * (LONG *) (pCl->lpszClassExtra + nIndex);
      pL = (LONG *) (pCl->lpszClassExtra + nIndex);
      *pL = iNewLong;
      break;
  }
  return lOld;
}

