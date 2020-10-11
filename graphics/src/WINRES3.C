/*===========================================================================*/
/*                                                                           */
/* File    : WINRES3.C                                                       */
/*                                                                           */
/* Purpose : Low-level resource handlign functions for MEWEL                 */
/*           These are convenience functions when using Borland's DPMI       */
/*           resource-handling functions.                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES

#include "wprivate.h"
#include "window.h"


/****************************************************************************/
/*                                                                          */
/* Function : LoadResourcePtr(hData)                                        */
/*                                                                          */
/* Purpose  : Internal function which is a shortcut for the Find/Load/Lock  */
/*            triplet which the various LoadXXX() functions use.            */
/*                                                                          */
/* Returns  : A pointer to the resource data if successful.                 */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL LoadResourcePtr(hInst, lpszID, lpszType, phRes)
  HINSTANCE hInst;
  LPCSTR    lpszID;
  LPCSTR    lpszType;
  HANDLE    *phRes;
{
  LPSTR  lpMem;
  HANDLE hRes;

  if (phRes)
    *phRes = NULL;

#if defined(__DPMI16__) && 0
  if (IS_MEWEL_INSTANCE(hInst))
  {
    lpMem = GetResource(hInst, (LPSTR) lpszID, lpszType);
    /*
      The LockResource(RT_BITMAP) should return a pointer to the
      BITMAPINFO structure which follows the BITMAPFILEHEADER structure.
      So, advance the read pointer past the BITMAPFILEHEADER structure.
    */
    if (lpszType == RT_BITMAP)
    {
      lpMem += sizeof(BITMAPFILEHEADER);
#if defined(UNIX_STRUCTURE_PACKING)
      lpMem -= 2;
#endif
    }
  }
  else
#endif
  {
    if ((hRes = FindResource(hInst, lpszID, lpszType)) == 0)
      return NULL;
    if ((hRes = LoadResource(hInst, hRes)) == 0)
      return NULL;
    if ((lpMem = LockResource(hRes)) == NULL)
    {
      FreeResource(hRes);
      return NULL;
    }
    if (phRes)        /* used for icons & bitmaps */
      *phRes = hRes;
  }

  return lpMem;
}


/****************************************************************************/
/*                                                                          */
/* Function : UnloadResourcePtr(hData)                                      */
/*                                                                          */
/* Purpose  : Internal function which is a shortcut for the Unlock/Free     */
/*            duot which the various LoadXXX() functions use.               */
/*                                                                          */
/* Returns  : A pointer to the resource data if successful.                 */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL UnloadResourcePtr(hInst, lpRes, hRes)
  HINSTANCE hInst;
  LPSTR  lpRes;
  HANDLE hRes;
{
  if (lpRes)
  {
#if defined(__DPMI16__) && 0
    if (IS_MEWEL_INSTANCE(hInst))
    {
      MyFree_far(lpRes);
      return;
    }
#endif

#if defined(__DPMI16__) && 0
    hRes = LOWORD(GlobalHandle(FP_SEG(lpRes)));
#endif
    UnlockResource(hRes);
    FreeResource(hRes);
  }
}

