/*===========================================================================*/
/*                                                                           */
/* File    : WPRINTDC.C                                                      */
/*                                                                           */
/* Purpose : Printer interface through device contexts.                      */
/*                                                                           */
/* History : Grateful acknowledgement to Thomas Wagner of Ferrari Electronics*/
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"
#include "winprint.h"

PRDEVICE PrDCs[MAX_PRDEVS];

/*
  The declaration of PrDrivers should be made in an external data file.
  See WPRNTDRV.C for a sample declaration. Ie :

      PRDRIVER PrDrivers[] =
      {
        { "HPPCL",   ena_hppcl },
      };
*/


/*
  Macro to determine if the DC is a valid DC for a display rather than a prt
*/
#define IS_DISPLAY_DC(hDC)  ((hDC) > 0 && _GetDC(hDC)->hPrDevice > 0)


/*
  Compatibility Notes & Wishes
    1.  Instead of having functions pointers in the device structure, we
  should dispatch everything through a central Control() call.
*/


/****************************************************************************/
/*                                                                          */
/* Function : CreateDC()                                                    */
/*                                                                          */
/* Purpose  : Create a device context for the specified device.             */
/*                                                                          */
/* Returns  : A handle to a device context if successful, 0 if not.         */
/*                                                                          */
/****************************************************************************/
HDC FAR PASCAL _CreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,bIsIC)
  LPSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  LPSTR     lpInitData;     /* usually NULL                */
  BOOL      bIsIC;
{
  int       i;
  HDC       hDC;
  LPHDC     lphDC;
  PPRDRIVER pr;
  PRDEVICE *prdesc;

  /*
    Special processing for DISPLAY
  */
  if (!lstricmp(lpDriverName, "DISPLAY"))
  {
    return GetDC(_HwndDesktop);
  }

  /*
    Do a lookup in the driver table to see if we have a driver for
    the printer. If not, return 0.

    COMPATIBILITY NOTE -
      We should really do run-time loading of the printer driver using
    a LoadLibrary() call.
  */
  for (i = 0, pr = PrDrivers;  pr->lpDriverName != NULL;  i++, pr++)
  {
    if (!stricmp(pr->lpDriverName, lpDriverName))
      break;
  }
  if (pr->lpDriverName == NULL)
    return (HDC) 0;

  if ((hDC = GetDC(HWND_NONDISPLAY)) == 0)
    return 0;

  lphDC = _GetDC(hDC);
  lphDC->hPrDevice = (i+1);

  prdesc = hPrDCToPrDevice(hDC);
  prdesc->fState        = (bIsIC) ? PRSTATE_IS_IC : 0;
  prdesc->lpfnAbortProc = NULL;
  prdesc->lpfnOutFunc   = PrOutDummy;
  lstrcpy(prdesc->portname, lpOutput);

  /*
    Call the printer driver, passing it a description of the printer,
    the full name of the printer, and the output port to use.
    Note - this is the only place where this module communicates with
    the driver code.
  */
  if (!pr->lpfnEnableProc(prdesc, lpDeviceName, lpOutput))
    return (HDC) 0;

  return hDC;
}

HDC FAR PASCAL CreateDC(lpDriverName, lpDeviceName, lpOutput, lpInitData)
  LPSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  LPSTR     lpInitData;     /* usually NULL                */
{
  return _CreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,FALSE);
}

HDC FAR PASCAL CreateIC(lpDriverName, lpDeviceName, lpOutput, lpInitData)
  LPSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  LPSTR     lpInitData;     /* usually NULL                */
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
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc && !(prdesc->fState & PRSTATE_IS_IC))
    return prdesc->lpfnDisableProc(prdesc);
  else if (IS_DISPLAY_DC(hDC))
  {
    ReleaseDC(hDC, _GetDC(hDC)->hWnd);
    return TRUE;
  }
  else
    return FALSE;
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
int FAR PASCAL GetDeviceCaps(hDC, nIndex)
  HDC  hDC;
  int  nIndex;
{
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc)
    return prdesc->lpfnDevCapsProc(prdesc, nIndex);
  else if (IS_DISPLAY_DC(hDC))
  {
    return _GetDC(hDC)->lpfnDevCapsProc(hDC, nIndex);
  }
  else
    return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : Escape()                                                      */
/*                                                                          */
/* Purpose  : Talk to the device driver.                                    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
int FAR PASCAL Escape(hDC, nEscape, nCount, lpInData, lpOutData)
  HDC   hDC;
  int   nEscape;
  int   nCount;
  LPSTR lpInData;
  LPSTR lpOutData;
{
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc == NULL)
    return SP_ERROR;

  if (nEscape == SETABORTPROC)
  {
    prdesc->lpfnAbortProc = (abtproc) lpInData;
    return 1;
  }

  /*
    If we have an information context, don't pass through requests which
    actually cause the printer to do something.
  */
  if ((prdesc->fState & PRSTATE_IS_IC))
  {
    switch (nEscape)     
    {
      case NEWFRAME :
      case STARTDOC :
      case ABORTDOC :
      case ENDDOC   :
        return 0;
    }
  }

  return prdesc->lpfnControlProc(prdesc,nEscape,nCount,lpInData,lpOutData);
}



/****************************************************************************/
/*                                                                          */
/* Function : DeviceMode()                                                  */
/*                                                                          */
/* Purpose  : Puts up a dialog mode which prompts the user with mode        */
/*            information for the specified printer.                        */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
int FAR PASCAL DeviceMode(hDC, hwnd, hInstance, lpDevName, lpOutput)
  HDC    hDC;       /* This arg is not in Windows */
  HWND   hwnd;
  HANDLE hInstance;
  LPSTR  lpDevName;
  LPSTR  lpOutput;
{
  /*
    Windows Compatibility Note :
      The hDC argument is not in Windows. This function should create
    a temporary information context and then delete it. ie -
      hIC = CreateIC();
      ....
      DeleteDC(hIC);
  */
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc)
    return prdesc->lpfnDevModeProc(prdesc,hwnd,hInstance,lpDevName,lpOutput);
  else
    return 0;
}


int FAR PASCAL PrTextOut(hDC, x, y, lpInData, nLen)
  HDC   hDC;
  int   x, y;
  LPSTR lpInData;
  int   nLen;
{
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc && !(prdesc->fState & PRSTATE_IS_IC))
    return prdesc->lpfnTextOutProc(prdesc, x, y, lpInData, nLen);
  else
    return 0;
}


int FAR PASCAL PrBitBlt(hDC, left, top, width, lines, lpInData)
  HDC hDC;
  int left;
  int top;
  int width;
  int lines;
  LPSTR lpInData;
{
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc && !(prdesc->fState & PRSTATE_IS_IC))
    return prdesc->lpfnBitbltProc(prdesc, left, top, width, lines, lpInData);
  else
    return 0;
}


int FAR PASCAL PrGetTextPars(hDC, piWidth, lppage)
  HDC hDC;
  int *piWidth;
  int *lppage;
{
  PRDEVICE *prdesc = hPrDCToPrDevice(hDC);
  if (prdesc)
    return prdesc->lpfnTextParsProc(prdesc, piWidth, lppage);
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
/****************************************************************************/
PRDEVICE FAR *PASCAL hPrDCToPrDevice(hDC)
  HDC hDC;
{
  LPHDC lphDC = _GetDC(hDC);

  if (lphDC)
    return &PrDCs[lphDC->hPrDevice - 1];
  else
    return (PRDEVICE *) NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : PrOutDummy()                                                  */
/*                                                                          */
/* Purpose  : This is a dummy routine which the printer device's            */
/*            lpfnOutFunc pointer points to when the device is not open.    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
int FAR PASCAL PrOutDummy(prdesc, str, len)
  PRDEVICE *prdesc;
  LPSTR    str;
  int      len;
{
  return 0;
}

