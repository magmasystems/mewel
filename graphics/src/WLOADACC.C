/*===========================================================================*/
/*                                                                           */
/* File    : WLOADACC.C                                                      */
/*                                                                           */
/* Purpose : Implements the LoadAccelerator() function                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define MEWEL_RESOURCES
#include "wprivate.h"
#include "window.h"

HACCEL FAR PASCAL LoadAccelerators(hModule, idAccel)
  HINSTANCE hModule;
  LPCSTR idAccel;
{
  LPSTR   pData;
  LPSTR   pDataOrig;
  LPACCEL lpAcc;
  int     nAccels;
  HACCEL  hAccel;

  pData = LoadResourcePtr(hModule, (LPSTR) idAccel, RT_ACCELERATOR, NULL);
  if ((pDataOrig = pData) == NULL)
    return NULL;

  /*
    Count the number of accelerators. The last accelerator entry
    has 0x80 or'ed into the fFlags member.
  */
  lpAcc = (LPACCEL) pData;
  for (nAccels = 1;  !(lpAcc->fFlags & 0x80);  nAccels++)
    lpAcc++;

  /*
    Create an empty accelerator table and fill it with the data
  */
  hAccel = GlobalAlloc(GMEM_ZEROINIT, (DWORD) (sizeof(ACCEL)*nAccels));
  if (hAccel)
  {
    LPSTR lpAccel;
    lpAccel = GlobalLock(hAccel);
    lmemcpy(lpAccel, pData, nAccels * sizeof(ACCEL));
    GlobalUnlock(hAccel);
  }

  /*
    Free the loaded resource
  */
  UnloadResourcePtr(hModule, pDataOrig, NULL);
  return hAccel;
}

