/*===========================================================================*/
/*                                                                           */
/* File    : WMETAFIL.C                                                      */
/*                                                                           */
/* Purpose : Stubs for the Metafile functions                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if defined(__TURBOC__)
#pragma warn -par
#endif

HMETAFILE WINAPI CloseMetaFile(HDC hDC)
{
  return 0;
}

HMETAFILE WINAPI CopyMetaFile(HMETAFILE hMF, LPCSTR lpFile)
{
  return 0;
}

HDC       WINAPI CreateMetaFile(LPCSTR lpFileName)
{
  return 0;
}

BOOL      WINAPI DeleteMetaFile(HMETAFILE hMF)
{
  return 0;
}

HMETAFILE WINAPI GetMetaFile(LPCSTR lpFile)
{
  return 0;
}

HGLOBAL   WINAPI GetMetaFileBits(HMETAFILE hMF)
{
  return 0;
}

BOOL      WINAPI PlayMetaFile(HDC hDC, HMETAFILE hMF)
{
  return 0;
}

HMETAFILE WINAPI SetMetaFileBits(HGLOBAL h)
{
  return 0;
}

HMETAFILE WINAPI SetMetaFileBitsBetter(HGLOBAL h)
{
  return 0;
}


BOOL WINAPI EnumMetaFile(HDC hDC, HMETAFILE hMF, MFENUMPROC lpfn, LPARAM lParam)
/* GDI.175 */
{
  (void) hDC;  (void) hMF;  (void) lpfn;  (void) lParam;
  return 0;
}

void WINAPI PlayMetaFileRecord(HDC hDC, HANDLETABLE FAR *lpHT, 
                               METARECORD FAR *lpMR, UINT nHandles)
/* GDI.176 */
{
  (void) hDC;  (void) lpHT;  (void) lpMR;  (void) nHandles;
}


