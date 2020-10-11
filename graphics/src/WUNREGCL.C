/*===========================================================================*/
/*                                                                           */
/* File    : WUNREGCL.C                                                      */
/*                                                                           */
/* Purpose : Implements the UnregisterClass() function.                      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  External decls found in WINCLASS.C
*/
extern EXTWNDCLASS *WndClassList;

static VOID PASCAL FreeClass(int);


BOOL FAR PASCAL UnregisterClass(lpClassName, hInstance)
  LPCSTR lpClassName;
  HANDLE hInstance;
{
  int  idClass;

  (void) hInstance;

  /*
    Don't remove any pre-defined classes
  */
  if ((idClass = ClassNameToClassID(lpClassName)) >= USER_CLASS)
  {
    WINDOW *w = InternalSysParams.WindowList;

    /*
      Destroy all windows belonging to that class
    */
    while (w)
    {
      if (w->idClass == idClass)
      {
        DestroyWindow(w->win_id);
        w = InternalSysParams.WindowList;    /* start all over again */
      }
      else
        w = w->next;
    }

    /*
      Free the class memory
    */
    FreeClass(idClass);

    return TRUE;
  }
  else
    return FALSE;
}


static VOID PASCAL FreeClass(iClass)
  int iClass;
{
  LPEXTWNDCLASS pClass;
  LPCSTR s;

  if ((pClass = ClassIDToClassStruct(iClass)) == NULL)
    return;

  /*
    Free any class-extra area
  */
  MyFree((LPSTR) pClass->lpszClassExtra);

  /*
    Free the various allocated strings.... the menu, base class and class name.
  */
  if (iClass > MENU_CLASS)
  {
    if (!ISNUMERICRESOURCE(pClass->lpszClassName))
      MYFREE_FAR((LPSTR) pClass->lpszClassName);
    MYFREE_FAR((LPSTR) pClass->lpszBaseClass);
  }
  if ((s = pClass->lpszMenuName) != NULL && !ISNUMERICRESOURCE(s))
    MYFREE_FAR((LPSTR) s);

  /*
    Get rid of the class from the ClassIndex
  */
  if (iClass < MAXCLASSES)
    ClassIndex[iClass] = NULL;

  /*
    Unlink the class from the class list
  */
  if (pClass == WndClassList)
  {
    WndClassList = pClass->next;
  }
  else
  {
    LPEXTWNDCLASS pCl;
    for (pCl = WndClassList;  pCl;  pCl = pCl->next)
    {
      if (pCl->next == pClass)
      {
        pCl->next = pClass->next;
        break;
      }
    }
  }

  /*
    Free the EXTWNDCLASS structure itself
  */
  MyFree(pClass);
}

