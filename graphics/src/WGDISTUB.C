/*===========================================================================*/
/*                                                                           */
/* File    : WGDISTUB.C                                                      */
/*                                                                           */
/* Purpose : Implements some GDI functions which are usually used for        */
/*           printers. These include CreateCompatibleDC, CreateDC, CreateIC, */
/*           DeleteDC, Escape, and GetDeviceCaps.                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define FULLGDI
#define MEWEL_PRINTER

#include "wprivate.h"
#include "window.h"


/****************************************************************************/
/*                                                                          */
/* Function : Escape()                                                      */
/*                                                                          */
/* Purpose  : Talk to the device driver.                                    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL Escape(hDC, nCode, nLen, lpDataIn, lpDataOut)
  HDC   hDC;
  INT   nCode;
  INT   nLen;
  LPCSTR lpDataIn;
  VOID FAR *lpDataOut;
{
  LPPRDEVICE prdesc = hPrDCToPrDevice(hDC);

  if (prdesc == NULL)
  {
    LPHDC lphDC;
    if ((lphDC = _GetDC(hDC)) != NULL && lphDC->lpfnEscapeProc)
      return (*lphDC->lpfnEscapeProc)(hDC, nCode, nLen, 
                                      (LPSTR) lpDataIn, (LPSTR) lpDataOut);
    else
      return SP_ERROR;
  }

  if (nCode == SETABORTPROC)
  {
    prdesc->lpfnAbortProc = (prnabtproc) lpDataIn;
    return 1;
  }

  /*
    If we have an information context, don't pass through requests which
    actually cause the printer to do something.
  */
  if ((prdesc->fState & PRSTATE_IS_IC))
  {
    switch (nCode)
    {
      case NEWFRAME :
      case STARTDOC :
      case ABORTDOC :
      case ENDDOC   :
        return 0;
    }
  }

  return prdesc->lpfnControlProc(prdesc, nCode, nLen,
                                 (LPSTR) lpDataIn, (LPSTR) lpDataOut);
}

/****************************************************************************/
/*                                                                          */
/* Function : GetDeviceCaps(HDC hDC, int nIndex)                            */
/*                                                                          */
/* Purpose  : Retrieves device-specific information about a given device.   */
/*                                                                          */
/* Returns  : The value of the desired item, 0 if not successful.           */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL GetDeviceCaps(hDC, nCode)
  HDC hDC;
  INT nCode;
{
  LPHDC lphDC;
  LPPRDEVICE prdesc = hPrDCToPrDevice(hDC);

  if (prdesc)
    return prdesc->lpfnDevCapsProc(prdesc, nCode);

  if ((lphDC = _GetDC(hDC)) != NULL && lphDC->lpfnDevCapsProc)
    return (*lphDC->lpfnDevCapsProc)(hDC, nCode);
  else
    return 0;
}

/****************************************************************************/
/*                                                                          */
/* Function : hPrDCToPrDevice()                                             */
/*                                                                          */
/* Purpose  : Given a handle to a device context, returns a pointer to the  */
/*            corresponding printer device structure.                       */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/* Note     : This stuff is only used in Thomas Wagner's printer stuff.     */
/*                                                                          */
/****************************************************************************/
LPPRDEVICE FAR PASCAL hPrDCToPrDevice(hDC)
  HDC hDC;
{
  if (SysGDIInfo.lpfnPrDCXlatHook)
    return (*SysGDIInfo.lpfnPrDCXlatHook)(hDC);
  else
    return (PRDEVICE *) NULL;
}

