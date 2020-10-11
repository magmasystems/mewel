/*===========================================================================*/
/*                                                                           */
/* File    : WCREATDC.C                                                      */
/*                                                                           */
/* Purpose : Implements some GDI functions which are usually used for        */
/*           printers.                                                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define FULLGDI
#define MEWEL_PRINTER

#include "wprivate.h"
#include "window.h"

static HDC FAR PASCAL _CreateDC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *, BOOL);

/****************************************************************************/
/*                                                                          */
/* Function : CreateDC()                                                    */
/*                                                                          */
/* Purpose  : Create a device context for the specified device.             */
/*                                                                          */
/* Returns  : A handle to a device context if successful, 0 if not.         */
/*                                                                          */
/****************************************************************************/
static 
HDC FAR PASCAL _CreateDC(
  LPCSTR     lpDriverName,   /* HPPCL, DISPLAY              */
  LPCSTR     lpDeviceName,   /* PCL/HP Laserjet             */
  LPCSTR     lpOutput,       /* COM1:, LPT1:, FILE:, etc... */
  CONST VOID FAR *lpInitData,/* Usually NULL */
  BOOL       bIsIC)
{
  HDC   hDC;

  /*
    Special processing for DISPLAY
  */
  if (!lstricmp((LPSTR) lpDriverName, "DISPLAY"))
    return GetDC(_HwndDesktop);

  /*
    Fail if no printer support installed
  */
  if (!SysGDIInfo.lpfnCreatPrDCHook)
    return (HDC) 0;

  /*
    Use the printer-specific creation routine
  */
  hDC = (*SysGDIInfo.lpfnCreatPrDCHook) ((LPSTR) lpDriverName, 
                                         (LPSTR) lpDeviceName, 
                                         (LPSTR) lpOutput, 
                                         (LPSTR) lpInitData, 
                                         bIsIC);
  return hDC;
}


HDC FAR PASCAL CreateDC(lpDriverName, lpDeviceName, lpOutput, lpInitData)
  LPCSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPCSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPCSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  CONST VOID FAR *lpInitData;/* usually NULL                */
{
  return _CreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,FALSE);
}

HDC FAR PASCAL CreateIC(lpDriverName, lpDeviceName, lpOutput, lpInitData)
  LPCSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPCSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPCSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  CONST VOID FAR *lpInitData;/* usually NULL                */
{
  return _CreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,TRUE);
}


/****************************************************************************/
/*                                                                          */
/* Function : DeleteDC(HDC hDC)                                             */
/*                                                                          */
/* Purpose  : Deletes a device context which was allocated by CreateDC().   */
/*                                                                          */
/* Returns  : TRUE if deleted, FALSE if not.                                */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL DeleteDC(hDC)
  HDC hDC;
{
  LPPRDEVICE prdesc;
  LPHDC      lphDC;

  if (hDC == 0)
    return FALSE;

  /*
    If this is a printer DC, call the printer-specific DC deletion routine.
  */
  prdesc = hPrDCToPrDevice(hDC);
  if (prdesc && SysGDIInfo.lpfnDelPrDCHook)
    return (*SysGDIInfo.lpfnDelPrDCHook) (hDC);


  lphDC = _GetDC(hDC);

  /*
    Call any DeleteDC hook for printer drivers
  */
  if (lphDC->lpfnDeleteDCHook)
    if ((*lphDC->lpfnDeleteDCHook)(hDC) == FALSE)
      return FALSE;

  ReleaseDC(lphDC->hWnd, hDC);
  return TRUE;
}

