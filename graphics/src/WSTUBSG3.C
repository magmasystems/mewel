/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSG3.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           These are for GDI bounding functions.                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


UINT WINAPI GetBoundsRect(HDC hDC, RECT FAR* lprcBounds, UINT flags)
/* GDI.194 */
{
  (void) hDC;  (void) lprcBounds;  (void) flags;
  return 0;
}

UINT WINAPI SetBoundsRect(HDC hDC, CONST RECT FAR* lprcBounds, UINT flags)
/* GDI.193 */
{
  (void) hDC;  (void) lprcBounds;  (void) flags;
  return 0;
}

