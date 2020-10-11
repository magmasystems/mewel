/*===========================================================================*/
/*                                                                           */
/* File    : WINPROP.C                                                       */
/*                                                                           */
/* Purpose : Property-list functions for MEWEL                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

typedef struct tagProp
{
  struct tagProp FAR *next;
  HANDLE wPropVal;
  BOOL  bIntResource;  /* true if prop name is a numeric resource */
  union
  {
    int   iProp;
    char  szProp[1];
  } u;
} PROP, *PPROP, FAR *LPPROP;

static LPPROP FAR PASCAL _FindProp(HWND, LPCSTR);
static BOOL FAR PASCAL _AddProp(HWND, LPCSTR, HANDLE);


static LPPROP FAR PASCAL _FindProp(hWnd, lpszProp)
  HWND  hWnd;
  LPCSTR lpszProp;
{
  LPPROP pProp;
  WINDOW *w;
  BOOL   bIsNumber;

  if ((w = WID_TO_WIN(hWnd)) == NULL)
    return (LPPROP) NULL;

  bIsNumber = ISNUMERICRESOURCE(lpszProp);

  for (pProp = (LPPROP) w->plistProp;  pProp;  pProp = pProp->next)
    if ((bIsNumber && pProp->u.iProp == (INT) lpszProp) ||
        (!bIsNumber && !pProp->bIntResource && !lstrcmp(pProp->u.szProp, (LPSTR) lpszProp)))
      return pProp;

  return (LPPROP) NULL;
}

static BOOL FAR PASCAL _AddProp(hWnd, lpszProp, wPropVal)
  HWND  hWnd;
  LPCSTR lpszProp;
  HANDLE wPropVal;
{
  LPPROP pProp;
  WINDOW *w;
  int    iLen;
  
  
  /*
    Allocate the memory for the prop structure
  */
  iLen = ISNUMERICRESOURCE(lpszProp) ? 0 : lstrlen((LPSTR) lpszProp)+1;
  pProp = (LPPROP) EMALLOC_FAR_NOQUIT((sizeof(PROP) + iLen));
  if (pProp == NULL)
    return FALSE;

  /*
    Copy the property value and property name (or number)
  */
  pProp->wPropVal = wPropVal;
  if ((pProp->bIntResource = ISNUMERICRESOURCE(lpszProp)) == TRUE)
    pProp->u.iProp = (INT) lpszProp;
  else
    lstrcpy(pProp->u.szProp, (LPSTR) lpszProp);

  /*
    Hook the prop onto the front of the window's list of props
  */
  w = WID_TO_WIN(hWnd);
  pProp->next = (LPPROP) w->plistProp;
  w->plistProp = (LPVOID) pProp;
  return TRUE;
}

HANDLE FAR PASCAL GetProp(hWnd, lpszProp)
  HWND  hWnd;
  LPCSTR lpszProp;
{
  LPPROP pProp;

  if ((pProp = _FindProp(hWnd, lpszProp)) != NULL)
    return pProp->wPropVal;
  else
    return 0;
}

BOOL FAR PASCAL SetProp(hWnd, lpszProp, wPropVal)
  HWND    hWnd;
  LPCSTR  lpszProp;
  HANDLE  wPropVal;
{
  LPPROP  pProp;

  if ((pProp = _FindProp(hWnd, lpszProp)) != NULL)
    return pProp->wPropVal = wPropVal;
  else
  {
    if (_AddProp(hWnd, lpszProp, wPropVal))
      return wPropVal;
    else
      return 0;
  }
}

HANDLE FAR PASCAL RemoveProp(hWnd, lpszProp)
  HWND  hWnd;
  LPCSTR lpszProp;
{
  LPPROP pProp;
  WINDOW *w;

  if ((pProp = _FindProp(hWnd, lpszProp)) != NULL)
  {
    w = WID_TO_WIN(hWnd);
    if (w->plistProp == pProp)
    {
      w->plistProp = (LPVOID) pProp->next;
    }
    else
    {
      LPPROP pPrev;
      for (pPrev = (LPPROP) w->plistProp;  pPrev->next != pProp;
           pPrev = pPrev->next)
        ;
      pPrev->next = pProp->next;
    }
    MYFREE_FAR(pProp);
    return TRUE;
  }
  else
    return NULLHWND;
}


typedef BOOL (CALLBACK* PropEnumProc)(HWND, LPCSTR, HANDLE);

INT FAR PASCAL EnumProps(hWnd, lpfn)
  HWND    hWnd;
  PROPENUMPROC lpfn;
{
  INT    rc = 0;
  LPPROP pProp;

  for (pProp = (LPPROP)WID_TO_WIN(hWnd)->plistProp;  pProp;  pProp=pProp->next)
  {
    LPCSTR lp = pProp->bIntResource ? ((LPCSTR)pProp->u.iProp) : (LPCSTR) pProp->u.szProp;
    if ((rc = (* (PropEnumProc) lpfn)(hWnd, lp, pProp->wPropVal)) == 0)
      break;
  }

  return rc;
}

