/*===========================================================================*/
/*                                                                           */
/* File    : WSTUBSU2.C                                                      */
/*                                                                           */
/* Purpose : Stubs for MS Windows-compatible functions                       */
/*           This deals with the Driver functions in USER.EXE                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "windows.h"


LRESULT WINAPI CloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2)
/* USER.253 */
{
  (void) hDriver;  (void) lParam1;  (void) lParam2;
  return 0;
}

LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR driverID,
                             UINT message, LPARAM lParam1, LPARAM lParam2)
/* USER.255 */
{
  (void) dwDriverIdentifier;  (void) driverID;
  (void) message;  (void) lParam1;  (void) lParam2;
  return 0;
}

BOOL WINAPI GetDriverInfo(HDRVR hDriver, DRIVERINFOSTRUCT FAR *lpDIS)
/* USER.256 */
{
  (void) hDriver;  (void) lpDIS;
  return 0;
}

HINSTANCE WINAPI GetDriverModuleHandle(HDRVR hDriver)
/* USER.254 */
{
  (void) hDriver;
  return 0;
}

HDRVR WINAPI GetNextDriver(HDRVR hDriver, DWORD fdwFlag)
/* USER.257 */
{
  (void) hDriver;  (void) fdwFlag;
  return 0;
}

HDRVR WINAPI OpenDriver(LPCSTR szDriverName,LPCSTR szSectionName,LPARAM lParam2)
/* USER.252 */
{
  (void) szDriverName;  (void) szSectionName;  (void) lParam2;
  return 0;
}

LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT message, 
                                 LPARAM lParam1, LPARAM lParam2)
/* USER.251 */
{
  (void) hDriver;  (void) message;  (void) lParam1;  (void) lParam2;
  return 0;
}


