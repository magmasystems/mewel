/*===========================================================================*/
/*                                                                           */
/* File    : WINCOMM.C                                                       */
/*                                                                           */
/* Purpose : Stubs for the comm functions                                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


int  WINAPI OpenComm(LPCSTR lpName, UINT rxSize, UINT txSize)
{
  (void) lpName;  (void) rxSize;  (void) txSize;
  return 0;
}

int  WINAPI CloseComm(int commID)
{
  (void) commID;
  return 0;
}

int  WINAPI SetCommState(CONST DCB FAR *lpDCB)
{
  (void) lpDCB;
  return 0;
}

int  WINAPI GetCommState(int chann, DCB FAR *lpDCB)
{
  (void) chann;  (void) lpDCB;
  return 0;
}

int  WINAPI UngetCommChar(int commID, char ch)
{
  (void) commID;  (void) ch;
  return 0;
}

int  WINAPI TransmitCommChar(int commID, char ch)
{
  (void) commID;  (void) ch;
  return 0;
}

int  WINAPI GetCommError(int commID, COMSTAT FAR *lpStat)
{
  (void) commID;  (void) lpStat;
  return 0;
}

LONG WINAPI EscapeCommFunction(int commID, int nFunc)
{
  (void) commID;  (void) nFunc;
  return 0L;
}

int  WINAPI FlushComm(int commID, int whichBuff)
{
  (void) commID;  (void) whichBuff;
  return 0;
}

int  WINAPI BuildCommDCB(LPCSTR lpDef, DCB FAR *lpDCB)
{
  (void) lpDef;  (void) lpDCB;
  return 0;
}

int  WINAPI ReadComm(int commID, VOID FAR *lpBuff, int maxLen)
{
  (void) commID;  (void) lpBuff;  (void) maxLen;
  return 0;
}

int  WINAPI WriteComm(int commID, CONST VOID FAR *lpBuff, int maxLen)
{
  (void) commID;  (void) lpBuff;  (void) maxLen;
  return 0;
}

int  WINAPI SetCommBreak(int commID)
{
  (void) commID;
  return 0;
}

int  WINAPI ClearCommBreak(int commID)
{
  (void) commID;
  return 0;
}

BOOL WINAPI EnableCommNotification(int idCommDev, HWND hWnd, 
                                   int cbWriteNotify, int cbOutQueue)
{
  (void) idCommDev;
  (void) hWnd;
  (void) cbWriteNotify;
  (void) cbOutQueue;
  return 0;
}

UINT FAR* WINAPI SetCommEventMask(int commID, UINT eventMask)
{
  (void) commID;  (void) eventMask;
  return 0;
}

UINT WINAPI GetCommEventMask(int commID, int eventMask)
{
  (void) commID;  (void) eventMask;
  return 0;
}

