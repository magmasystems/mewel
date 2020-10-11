/*===========================================================================*/
/*                                                                           */
/* File    : WINATOM2.C                                                      */
/*                                                                           */
/* Purpose : Atom Management functions for MEWEL                             */
/*           Stubs for the GlobalAtom functions                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

ATOM WINAPI GlobalAddAtom(lpszName)
  LPCSTR lpszName;
{
  return AddAtom(lpszName);
}

ATOM WINAPI GlobalDeleteAtom(a)
  ATOM a;
{
  return DeleteAtom(a);
}

ATOM WINAPI GlobalFindAtom(lpszName)
  LPCSTR lpszName;
{
  return FindAtom(lpszName);
}

UINT WINAPI GlobalGetAtomName(a, lpszBuffer, cbBuffer)
  ATOM  a;
  LPSTR lpszBuffer;
  INT   cbBuffer;
{
  return GetAtomName(a, lpszBuffer, cbBuffer);
}

