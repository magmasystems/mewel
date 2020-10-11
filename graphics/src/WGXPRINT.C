#define MEWEL_PRINTER
#define STRICT

/*===========================================================================*/
/*                                                                           */
/* File    : WGXPRINT.C                                                      */
/*                                                                           */
/* Purpose : Interface to the Genus GX Printer toolkit                       */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#include <prlib.h>    /* Genus Printer header */

#ifdef __cplusplus
extern "C" {
#endif
static BOOL FAR PASCAL WinOpenGraphicsPrinter(LPCSTR, LPCSTR);
static INT      PASCAL GXStringToPort(PSTR);
static HDC  FAR PASCAL PrCreateDC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *, BOOL);
static INT  FAR PASCAL PrGetDeviceCaps(HDC, int);
static INT  FAR PASCAL PrEscape(HDC, INT, INT, LPCSTR, VOID FAR *);
static BOOL FAR PASCAL PrDeleteDC(HDC);
static short FAR PASCAL PrProgress(short, short);
static VOID PASCAL InitPrinterPage(HDC hDC);
#ifdef __cplusplus
}
#endif

static struct
{
  PRHEADER  gxPRD;
  HDC       hDC;
  BOOL      bPrinterAborting;
  HBITMAP   hOldBitmap;
  HBITMAP   hPrinterBitmap;
  ABORTPROC lpfnAbortProc;
} PrinterInfo =
{
  { 0 },
  (HDC) 0,
  FALSE,
  (HBITMAP) 0,
  (HBITMAP) 0,
  NULL,
};


/*
  InstallMEWELPrinterSupport()
    This is the only public function here. An app must call this once.
  This must be called before a DC is created for a printer device.
*/
void FAR PASCAL InstallPrinterSupport(void)
{
  SysGDIInfo.lpfnCreatPrDCHook = PrCreateDC;
}



static BOOL FAR PASCAL WinOpenGraphicsPrinter(LPCSTR lpDriver, LPCSTR lpPort)
{
  char szPath[MAXPATH];
  char szPort[64];

  /*
    Get the name of the PRD path
      [windows]
      device=HP LaserJet Series II,HPPCL,LPT1:

      [devices]
      HP LaserJet Series II=HPPCL,FILE:,LPT1:
  */

  if (!lpDriver || !lpDriver[0])
  {
    GetProfileString("printer", "printer.drv",  "", szPath, sizeof(szPath));
    if (!szPath[0])
      return FALSE;
  }
  else
    lstrcpy(szPath, lpDriver);

  if (!lpPort || !lpPort[0])
    GetProfileString("printer", "printer.port", "LPT1:", szPort, sizeof(szPort));
  else
    lstrcpy(szPort, lpPort);

  /*
    See if the entry has the actual name of the BGI driver
  */
  if (strchr(szPath, '.') == NULL)
    strcat(szPath, ".prd");


  /*
    Open the printer driver
  */
  if (prFileDriver(szPath, &PrinterInfo.gxPRD) != prSUCCESS ||
      prSetDriver(&PrinterInfo.gxPRD) != prSUCCESS)
  {
    MessageBox(GetDesktopWindow(), 
               "Cannot open the printer driver specified in printer.drv",
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  /*
    Set the printer port
  */
  prSetPort(GXStringToPort(szPort));

  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : PrCreateDC()                                                  */
/*                                                                          */
/* Purpose  : Create a device context for the specified device.             */
/*                                                                          */
/* Returns  : A handle to a device context if successful, 0 if not.         */
/*                                                                          */
/****************************************************************************/
static HDC FAR PASCAL PrCreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,bIsIC)
  LPCSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPCSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPCSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  CONST VOID FAR *lpInitData;/* Usually NULL */
  BOOL       bIsIC;
{
  HDC     hDC, hDCTmp;
  LPHDC   lphDC;
  char    newPat[8];
  char    szBuf[256]; 


  (void) lpDeviceName;
  (void) lpOutput;
  (void) lpInitData;
  (void) bIsIC;

  /*
    Use the printer-specific creation routine
  */
  if (WinOpenGraphicsPrinter(lpDriverName, lpOutput))
  {
    hDCTmp = GetDC(_HwndDesktop);
    hDC    = CreateCompatibleDC(hDCTmp);
    lphDC  = _GetDC(hDC);
    lphDC->fFlags |= DC_ISPRTDC | DC_ISMEMDC;

    /*
      Set up the device driver functions
    */
    lphDC->lpfnEscapeProc   = PrEscape;
    lphDC->lpfnDevCapsProc  = PrGetDeviceCaps;
    lphDC->lpfnDeleteDCHook = PrDeleteDC;

    /*
      Get the page size in pixels
    */
    prSetUnits(prPIXELS);
    prGetPageSize(&lphDC->devInfo.cxPixels, &lphDC->devInfo.cyPixels);

    /*
      Get inches * 100
    */
    prSetUnits(prINCHES);
    prGetPageSize(&lphDC->devInfo.cxInches, &lphDC->devInfo.cyInches);

    /*
      Get the number of pixels per inch. Since the inches is expressed
      in inches*100, then to compensate, we must multiply the number
      of pixels by 100 too.
    */
    lphDC->devInfo.cxLogPixels = (int) (((LONG) lphDC->devInfo.cxPixels * 100L) /
                                  (LONG) lphDC->devInfo.cxInches);
    lphDC->devInfo.cyLogPixels = (int) (((LONG) lphDC->devInfo.cyPixels * 100L) /
                                  (LONG) lphDC->devInfo.cyInches);

    /*
      Get centimeters * 100. If the page is 8", then it is about 20 cm.
      prGetPageSize would return 2000. If we divide by 10, then we
      get the number of millimeters.
    */
    prSetUnits(prCENTIMETERS);
    prGetPageSize(&lphDC->devInfo.cxMM, &lphDC->devInfo.cyMM);
    lphDC->devInfo.cxMM /= 10;
    lphDC->devInfo.cyMM /= 10;

    /*
      Restore the original Pixel unit
    */
    prSetUnits(prPIXELS);

    /*
      Set the pattern for 63 to be all 1's
    */
    memset(newPat, 0xFF, sizeof(newPat));
    prSetPattern(63, newPat);


    szBuf[0] = ' ';
    GetProfileString("printer", "pattern", "", szBuf+1, sizeof(szBuf)-1);
    if (szBuf[1])
    {
      int n, ch;
      LPSTR s;
      for (s = szBuf;  s && (s = next_int_token(s, &n)) != NULL;  )
      {
        s = next_int_token(s, &ch);
        memset(newPat, ch, sizeof(newPat));
        prSetPattern(n, newPat);
      }
    }

    prSetPaletteRGB(0,   0x00, 0x00, 0x00);
    prSetPaletteRGB(15,  0x3F, 0x3F, 0x3F);
    prSetPaletteRGB(255, 0x3F, 0x3F, 0x3F);

    szBuf[0] = ' ';
    GetProfileString("printer", "palette", "", szBuf+1, sizeof(szBuf)-1);
    if (szBuf[1])
    {
      int n, r, g, b;
      LPSTR s;
      for (s = szBuf;  s && (s = next_int_token(s, &n)) != NULL;  )
      {
        s = next_int_token(s, &r);
        s = next_int_token(s, &g);
        s = next_int_token(s, &b);
        prSetPaletteRGB(n, r, g, b);
      }
    }

    if (GetProfileInt("printer", "intensity", 1) == 1)
      prSetIntensity(prFULL);
    else
      prSetIntensity(prHALF);
    if (GetProfileInt("printer", "black", 1) == 1)
      prSetBlack(prBLACK);
    else
      prSetBlack(prWHITE);


    /*
      Create a bitmap the same size as the screen.
    */
    PrinterInfo.hPrinterBitmap = CreateBitmap(
                           lphDC->devInfo.cxPixels, lphDC->devInfo.cyPixels, 
                           1, /* nPlanes - always 1 */
                           4, /* nBitsPerPixel */
                           NULL);
    PrinterInfo.hOldBitmap = SelectObject(hDC, PrinterInfo.hPrinterBitmap);
    InitPrinterPage(hDC);

    ReleaseDC(_HwndDesktop, hDCTmp);
  }
  else
    hDC = 0;

  return hDC;
}


static VOID PASCAL InitPrinterPage(HDC hDC)
{
  RECT   r;
  BITMAP bmp;
  HBRUSH hBr;

  GetObject(PrinterInfo.hPrinterBitmap, sizeof(bmp), (LPSTR) &bmp);
  SetRect(&r, 0, 0, bmp.bmWidth, bmp.bmHeight);
  hBr = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
  FillRect(hDC, &r, hBr);
  DeleteObject(hBr);
}


/****************************************************************************/
/*                                                                          */
/* Function : PrDeleteDC(HDC hDC)                                           */
/*                                                                          */
/* Purpose  : Deletes a device context which was allocated by CreateDC().   */
/*                                                                          */
/* Returns  : TRUE if deleted, FALSE if not.                                */
/*                                                                          */
/****************************************************************************/
static BOOL FAR PASCAL PrDeleteDC(HDC hDC)
{
  LPHDC lphDC;
  HBITMAP hBitmap;

  if (hDC == 0)
    return FALSE;

  lphDC = _GetDC(hDC);

  if (IS_PRTDC(lphDC))
  {
    /*
      GX/Printer commands to reset the printer
    */
    prSendCommand(prRESET);

    /*
      Delete the printer bitmap
    */
    if (PrinterInfo.hOldBitmap)
    {
      hBitmap = SelectObject(hDC, PrinterInfo.hOldBitmap);
      DeleteObject(hBitmap);
      PrinterInfo.hOldBitmap = NULL;
    }
  }

  return TRUE;
}

/****************************************************************************/
/*                                                                          */
/* Function : PrEscape()                                                    */
/*                                                                          */
/* Purpose  : Talk to the device driver.                                    */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL PrEscape(hDC, nCode, nLen, lpDataIn, lpDataOut)
  HDC   hDC;
  INT   nCode;
  INT   nLen;
  LPCSTR lpDataIn;
  VOID FAR *lpDataOut;
{
  GXHEADER *pGXHeader;
  LPHDC    lphDC = _GetDC(hDC);

  (void) nLen;
  (void) lpDataIn;
  (void) lpDataOut;

  if (lphDC == NULL)
    return 0;
  if ((pGXHeader = ((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader) == NULL)
    return 0;

  switch (nCode)
  {
    case STARTDOC :
      prSetProgressFunction(PrProgress);
      break;

    case ENDDOC   :
      prSetProgressFunction(NULL);
      break;

    case ABORTDOC :
      PrinterInfo.bPrinterAborting = TRUE;
      break;

    case NEXTBAND :
      break;

    case SETABORTPROC :
      PrinterInfo.lpfnAbortProc = (ABORTPROC) lpDataIn;
      break;

    case NEWFRAME :
#if 0
    {
      HWND hWnd = WID_TO_WIN(2)->children->win_id;
      HDC hDCScreen = GetDC(hWnd);
      BITMAP bm;
      GetObject(PrinterInfo.hPrinterBitmap, sizeof(bm), (LPSTR) &bm);
      MEWELDrawDIBitmap(hDCScreen, 0, 0, bm.bmWidth, bm.bmHeight,
                        hDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
      ReleaseDC(hWnd, hDCScreen);
    }
#endif

#if 1
      if (prPrinterReady() == prSUCCESS)
      {
        prSetUnits(prINCHES);
        prSetSize(lphDC->devInfo.cxInches, lphDC->devInfo.cyInches);
        prSetPageSize(lphDC->devInfo.cxInches, lphDC->devInfo.cyInches);
        prVirtualPrint(pGXHeader, pGXHeader->x1, pGXHeader->y1,
                                  pGXHeader->x2, pGXHeader->y2);
        prSendCommand(prEJECT);
      }
#endif

      InitPrinterPage(hDC);
      break;
  }
  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : PrGetDeviceCaps(HDC hDC, int nIndex)                          */
/*                                                                          */
/* Purpose  : Retrieves device-specific information about a given device.   */
/*                                                                          */
/* Returns  : The value of the desired item, 0 if not successful.           */
/*                                                                          */
/****************************************************************************/
static INT FAR PASCAL PrGetDeviceCaps(HDC hDC, INT nCode)
{
  LPHDC lphDC = _GetDC(hDC);
  if (lphDC == NULL)
    return 0;

  switch (nCode)
  {
    case VERTRES  :
      return lphDC->devInfo.cyPixels;
    case HORZRES  :
      return lphDC->devInfo.cxPixels;
    case VERTSIZE :
      return lphDC->devInfo.cyMM;
    case HORZSIZE :
      return lphDC->devInfo.cxMM;
    case LOGPIXELSX :
      return lphDC->devInfo.cxLogPixels;
    case LOGPIXELSY :
      return lphDC->devInfo.cyLogPixels;
  }

  return 0;
}


static short FAR PASCAL PrProgress(short iCurrPass, short nPasses)
{
  (void) iCurrPass;
  (void) nPasses;

  /*
    Call the AbortProc (if there is one). If the AbortProc returns
    FALSE< then the print job is aborted.
  */
  if (PrinterInfo.lpfnAbortProc)
    if ((*PrinterInfo.lpfnAbortProc)(PrinterInfo.hDC, 0) == FALSE)
      PrinterInfo.bPrinterAborting = TRUE;

  /*
    If we are aborting, then return a value other than prSUCCESS
  */
  if (PrinterInfo.bPrinterAborting)
  {
    PrinterInfo.bPrinterAborting = FALSE;
    return 999;
  }

  return prSUCCESS;
}


static INT PASCAL GXStringToPort(PSTR pszPort)
{
  if (!lstrnicmp(pszPort, "lpt", 3))
    return prLPT1 + (pszPort[3] - '1');
  if (!lstrnicmp(pszPort, "com", 3))
    return prCOM1 + (pszPort[3] - '1');
  return prLPT1;
}

