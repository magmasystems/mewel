/*===========================================================================*/
/*                                                                           */
/* File    : WINENUM.C                                                       */
/*                                                                           */
/* Purpose : EnumWindows and EnumChildWindows  routines                      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

typedef BOOL (FAR PASCAL *ENUMPROC)(HWND, LONG);


/****************************************************************************/
/*                                                                          */
/* Function : EnumTaskWindows(HTASK, FARPROC, LONG)                         */
/*                                                                          */
/* Purpose  : Same thing as EnumWindows().                                  */
/*                                                                          */
/* Returns  : TRUE if all windows were enumerated, FALSE if not.            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL EnumTaskWindows(hTask, func, lParam)
  HANDLE  hTask;
  FARPROC func;
  LONG    lParam;
{
  (void) hTask;
  return EnumWindows(func, lParam);
}

/****************************************************************************/
/*                                                                          */
/* Function : EnumWindows(FARPROC, LONG)                                    */
/*                                                                          */
/* Purpose  : Enumerates all top-level windows. Calls a user-supplied       */
/*            enumeration function for each window.                         */
/*                                                                          */
/* Returns  : TRUE if all windows were enumerated, FALSE if not.            */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL EnumWindows(func, lParam)
  FARPROC func;
  LONG  lParam;
{
  WINDOW  *w, *wSibling;
  ENUMPROC ep = (ENUMPROC) func;
  
  for (w = InternalSysParams.wDesktop->children;  w;  w = wSibling)
  {
    wSibling = w->sibling; /* in case the enum function destroys w (like MDI) */
    if (!ep(w->win_id, lParam))
      break;
  }
  return (BOOL) (w == NULL);
}


/****************************************************************************/
/*                                                                          */
/* Function : EnumChildWindows(hParent, enumFunc, lParam)                   */
/*                                                                          */
/* Purpose  : Enumerates all child windows of a window.                     */
/*                                                                          */
/* Returns  : TRUE if all child windows were enumerated, FALSE if not.      */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL EnumChildWindows(hParent, func, lParam)
  HWND  hParent;
  FARPROC func;
  LONG  lParam;
{
  WINDOW  *w;
  WINDOW  *wSibling;
  BOOL     rc;
  ENUMPROC ep = (ENUMPROC) func;
  
  if ((w = WID_TO_WIN(hParent)) == (WINDOW *) NULL)
    return TRUE;

  /*
    Go through the child list
  */
  for (w = w->children;  w;  w = wSibling)
  {
    /*
      Save the sibling ptr in case the enum function destroys w (like MDI)
    */
    wSibling = w->sibling;

    /*
      Recurse on the grandchildren
    */
    if (w->children)
      rc = EnumChildWindows(w->win_id, func, lParam);
    else
      rc = TRUE;

    /*
      Call the enumeration function
    */
    if (!rc || !ep(w->win_id, lParam))
      break;
  }

  return (BOOL) (w == NULL);
}
