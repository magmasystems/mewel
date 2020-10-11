/*===========================================================================*/
/*                                                                           */
/* File    : WENUMOBJ.C                                                      */
/*                                                                           */
/* Purpose : Implements the EnumObjects() function                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define STRICT

#include "wprivate.h"
#include "window.h"
#include "wobject.h"

int WINAPI EnumObjects(hDC, fObjectType, lpfn, lParam)
  HDC hDC;
  int fObjectType;
  GOBJENUMPROC lpfn;
  LPARAM lParam;
{
  LPOBJECT lpObj;
  int rc;
  int i;

  (void) hDC;

  for (i = 0;  i < (int) _ObjectTblSize;  i++)
  {
    if ((lpObj = _ObjectTable[i]) == NULL)
      continue;

    if (lpObj->iObjectType == fObjectType)  /* either OBJ_BRUSH or OBJ_PEN */
    {
      rc = (int) (* (GOBJENUMPROC) lpfn) (&lpObj->uObject.uLogBrush, lParam);
      if (rc == 0)
        return 0;
    }
  }

  return rc;
}


