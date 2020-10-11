/*===========================================================================*/
/*                                                                           */
/* File    : WSAVEDC.C                                                       */
/*                                                                           */
/* Purpose : Implements the SaveDC() and RestoreDC() functions               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static VOID PASCAL ListFreeToEnd(LIST **, LIST *);

/*
  Hook to GUIs and graphics engines
*/
typedef INT (FAR PASCAL *GETDCPROC)(HDC);
extern GETDCPROC lpfnGetDCHook;


BOOL FAR PASCAL RestoreDC(hDC, nSavedDC)
  HDC  hDC;
  INT  nSavedDC;  /* -1 to restore the last one */
{
  LPHDC lphDC;
  LPHDC lpSavedDC;
  INT   nDCs;
  LIST  *pList;
  LIST  *listSave;


  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  if ((nDCs = ListGetLength(lphDC->listSavedDCs)) == 0)
    return FALSE;

  /*
    Do some checking on the index of the DC to restore.
  */
  if (nSavedDC == -1)
    nSavedDC = nDCs;
  else if (nSavedDC <= 0 || nSavedDC > nDCs)
    return FALSE;

  /*
    Retrieve the specified DC. ListGetNth() expects a 0-based index.
  */
  if ((pList = ListGetNth(lphDC->listSavedDCs, nSavedDC-1)) == NULL ||
      (lpSavedDC = (LPHDC) (pList->data)) == NULL)
    return FALSE;

  /*
    Copy the old DC into the current DC.
    We must be careful to preserve the current DC's link-list pointer
    and place that into the new DC.
  */
  listSave = lphDC->listSavedDCs;
  lmemcpy((LPSTR) lphDC, (LPSTR) lpSavedDC, sizeof(DC));
  lphDC->listSavedDCs = listSave;

  /*
    Now delete everything on the stack from this DC to the end
  */
  ListFreeToEnd(&lphDC->listSavedDCs, pList);

  /*
    Call the GUI-specific hook
  */
#if !20495
  if (lphDC->hWnd != HWND_NONDISPLAY && lpfnGetDCHook)
    (*lpfnGetDCHook)(hDC);
#endif

  return TRUE;
}


INT FAR PASCAL SaveDC(hDC)
  HDC  hDC;
{
  LPHDC  lphDC;
  LPHDC  lpSavedDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Allocate space for a DC structure. Don't use GlobalAlloc if we
    want to take advantage of the linked list routines.
  */
  if ((lpSavedDC = (LPHDC) EMALLOC_FAR(sizeof(DC))) == NULL)
    return FALSE;

  /*
    Copy the current DC
  */
  lmemcpy((LPSTR) lpSavedDC, (LPSTR) lphDC, sizeof(DC));

  /*
    Add this DC onto the end of the saved DC list
  */
  ListAdd(&lphDC->listSavedDCs, ListCreate((LPSTR) lpSavedDC));

  return ListGetLength(lphDC->listSavedDCs);
}


static VOID PASCAL ListFreeToEnd(headptr, listStart)
  LIST **headptr;
  LIST *listStart;
{
  LIST *p, *savep;

  for (p = listStart;  p;  p = savep)
  {
    savep = p->next;
    MYFREE_FAR(p->data);
    MyFree(p);
  }
  if (listStart == *headptr)
    *headptr = (LIST *) NULL;
}

