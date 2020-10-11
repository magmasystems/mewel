/*===========================================================================*/
/*                                                                           */
/* File    : WFINDWIN.C                                                      */
/*                                                                           */
/* Purpose : Impelemts the FindWindow() function                             */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

HWND FAR PASCAL FindWindow(lpClassName, lpWindowName)
  LPCSTR lpClassName;
  LPCSTR lpWindowName;
{
  WINDOW *w;

  for (w = InternalSysParams.WindowList;  w;  w = w->next)
  {
    /*
      If the class name is NULL, then only match the window name.
    */
    BOOL bMatchedClass = (BOOL)
      (lpClassName == NULL || 
       lstricmp((LPSTR) lpClassName, (LPSTR) WinGetClassName(w->win_id)) == 0);

    if (bMatchedClass)
    {
      if (lpWindowName == NULL ||  
          (w->title && !lstricmp((LPSTR) lpWindowName, w->title)))
        return w->win_id;
    }
  }

  return NULLHWND;
}

