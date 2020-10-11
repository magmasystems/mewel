/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU2.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with the network functions in USER.EXE               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


UINT WINAPI WNetAddConnection(LPSTR lpszNetPath, LPSTR lpszPassword,
                              LPSTR lpszLocalName)
/* USER.517 */
{
  (void) lpszNetPath;  (void) lpszPassword;  (void) lpszLocalName;
  return WN_NOT_SUPPORTED;
}

UINT WINAPI WNetCancelConnection(LPSTR lpszName, BOOL fForce)
/* USER.518 */
{
  (void) lpszName;  (void) fForce;
  return WN_NOT_SUPPORTED;
}

UINT WINAPI WNetGetConnection(LPSTR lpszLocalName, LPSTR lpszRemoteName, 
                              UINT FAR *cb)
/* USER.512 */
{
  (void) lpszLocalName;  (void) lpszRemoteName;  (void) cb;
  return WN_NOT_SUPPORTED;
}


