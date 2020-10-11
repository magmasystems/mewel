//#define RYLE
#define FLEMING
#define MEWEL_PRINTER

/*===========================================================================*/
/*                                                                           */
/* File    : WGRAPHPRT.C                                                     */
/*                                                                           */
/* Purpose : Interface to third-party BGI printer libs (Ryle, Fleming, etc). */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#ifdef __cplusplus
extern "C" {
#endif
BOOL FAR PASCAL WinOpenGraphicsPrinter(void);
BOOL FAR PASCAL WinCloseGraphicsPrinter(void);
static HDC  FAR PASCAL PrCreateDC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *, BOOL);
static INT  FAR PASCAL PrGetDeviceCaps(HDC, int);
static INT  FAR PASCAL PrEscape(HDC, INT, INT, LPCSTR, VOID FAR *);
static BOOL FAR PASCAL PrDeleteDC(hDC);

extern BOOL PASCAL BGIInitDriver(int);
extern VOID PASCAL BGISaveScreenDuringSwitch(BOOL);
extern VOID FAR PASCAL WinCloseGUIFontStuff(void);
extern VOID PASCAL ReadINIFontInfo(HDC);
#ifdef __cplusplus
}
#endif


#if defined(RYLE)
#include "bgiprt.h"
#define DEFAULT_PR_MODE    (PRT_QUIET | PRT_LANDSCAPE | HPLJ_LETTER_300)
#define DrvMask    0
#define SAVE_SCREEN_ON()   BGISaveScreenDuringSwitch(TRUE)
#define SAVE_SCREEN_OFF()  BGISaveScreenDuringSwitch(FALSE)
#define CLOSE_GRAPH()      closegraph()
#define 

#elif defined(FLEMING)
#include "graphadd.h"
#define DEFAULT_PR_MODE    (PortLPT1 + HalfLo)
#define SAVE_SCREEN_ON()   BGISaveScreenDuringSwitch(TRUE)
#define SAVE_SCREEN_OFF()  BGISaveScreenDuringSwitch(FALSE)
#define CLOSE_GRAPH()      bclosegraph()
#endif


INT BGIPrtMode   = DEFAULT_PR_MODE;
INT BGIPrtDriver = 0;

/*
  InstallMEWELPrinterSupport()
    This is the only public function here. An app must call this once.
  This must be called before a DC is created for a printer device.
*/
void FAR PASCAL InstallPrinterSupport(void)
{
  SysGDIInfo.lpfnCreatPrDCHook = PrCreateDC;
}


BOOL FAR PASCAL WinOpenGraphicsPrinter(void)
{
  char szPath[MAXPATH];
  char szDriver[16];
  PSTR pszDriver;
  int  rc;


  /*
    Get the name of the BGI path
  */
  GetProfileString("printer", "printer.drv", "", szPath, sizeof(szPath));

  /*
    See if the entry has the actual name of the BGI driver
  */
  if ((pszDriver = strchr(szPath, '.')) != NULL)
  {
    *pszDriver-- = '\0';   /* get rid of the .BGI extension */
    /*
      Split the driver name from the rest of the path.
    */
    if ((pszDriver = strrchr(szPath, '\\')) != NULL)
      *pszDriver++ = '\0';
    else
      pszDriver = szPath;

    /*
      Install the user-defined driver
      Install the printer driver only once.
    */
    if (!BGIPrtDriver)
      BGIPrtDriver = installuserdriver(pszDriver, NULL) + DrvMask;

    /*
      If the display.drv entry was something like 
        printer.drv=hplj
      then set the driver path to "".
    */
    if (pszDriver == szPath)
      szPath[0] = '\0';
  }


  /*
    Close the screen driver
  */
  SAVE_SCREEN_ON();
  closegraph();
  WinCloseGUIFontStuff();

  /*
    Open the printer driver
  */
  BGIPrtMode = GetProfileInt("printer", "printer.mode", DEFAULT_PR_MODE);

#if defined(RYLE)
  initgraph(&BGIPrtDriver, &BGIPrtMode, szPath);
#elif defined(FLEMING)
  {
  /*
    Figure out the memory left in the app so we can provide memory for
    the driver.
  */
  DrvINFO drvInfo;

  DWORD dwMem = coreleft() - 24000;
  if (dwMem > 65000)
    dwMem = 65000;

  /*
    Set up the driver info structure
    Options :
      gdNoCls   don't clear the screen
      gdUseEMS  use EMS memory
      gdStdPattern/gdFinePattern  fine or standard fill patterns
  */
  drvInfo.DrvOptions = gdNoCls;
  drvInfo.EMSpages   = 64;
  drvInfo.DrvOutfile = 0;
  drvInfo.DrvWrkpath = 0;
  drvInfo.StatFunc   = 0;
  drvInfo.Timeout    = 0;
  drvInfo.Width      = 0;
  drvInfo.Height     = 0;
  drvInfo.TopMargin  = 0;
  drvInfo.LeftMargin = 0;
  drvInfo.ScnDriver  = 0;
  drvInfo.ScnBiosFlag= 0;

  binitgraph(&BGIPrtDriver, &BGIPrtMode, szPath, dwMem, &drvInfo);
  }
#endif


#if defined(FLEMING)
  if ((rc = graphstatus()) != grOk)
#else
  if ((rc = graphresult()) != grOk)
#endif
  {
    RECT r;

    BGIInitDriver(MEWELInitialGraphicsMode);
    SAVE_SCREEN_OFF();
    GetClientRect(_HwndDesktop, &r);
    WinGenInvalidRects(_HwndDesktop, &r);
    RefreshInvalidWindows(_HwndDesktop);
    return FALSE;
  }
  (void) rc;

  return TRUE;
}



BOOL FAR PASCAL WinCloseGraphicsPrinter(void)
{
  /*
    Close the printer driver
  */
  CLOSE_GRAPH();
  WinCloseGUIFontStuff();

  /*
    Re-init the screen driver
  */
  BGIInitDriver(MEWELInitialGraphicsMode);
  SAVE_SCREEN_OFF();

  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : CreateDC()                                                    */
/*                                                                          */
/* Purpose  : Create a device context for the specified device.             */
/*                                                                          */
/* Returns  : A handle to a device context if successful, 0 if not.         */
/*                                                                          */
/****************************************************************************/
static HDC FAR PASCAL _CreateDC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *, BOOL);

static HDC FAR PASCAL PrCreateDC(lpDriverName,lpDeviceName,lpOutput,lpInitData,bIsIC)
  LPCSTR     lpDriverName;   /* HPPCL, DISPLAY              */
  LPCSTR     lpDeviceName;   /* PCL/HP Laserjet             */
  LPCSTR     lpOutput;       /* COM1:, LPT1:, FILE:, etc... */
  CONST VOID FAR *lpInitData;/* Usually NULL */
  BOOL       bIsIC;
{
  HDC   hDC;
  LPHDC lphDC;

  if (WinOpenGraphicsPrinter())
  {
    hDC = GetDC(HWND_NONDISPLAY);
    lphDC = _GetDC(hDC);
    lphDC->fFlags |= DC_ISPRTDC;

    /*
      Set up the device driver functions
    */
    lphDC->lpfnEscapeProc   = PrEscape;
    lphDC->lpfnDevCapsProc  = PrGetDeviceCaps;
    lphDC->lpfnDeleteDCHook = PrDeleteDC;

    ReadINIFontInfo(hDC);
  }
  else
    hDC = 0;

  return hDC;
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
static BOOL FAR PASCAL PrDeleteDC(HDC hDC)
{
  RECT  r;
  LPHDC lphDC;

  if (hDC == 0)
    return FALSE;

  lphDC = _GetDC(hDC);

  if (IS_PRTDC(lphDC))
    WinCloseGraphicsPrinter();

  GetClientRect(_HwndDesktop, &r);
  WinGenInvalidRects(_HwndDesktop, &r);
  RefreshInvalidWindows(_HwndDesktop);

  return TRUE;
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
static INT FAR PASCAL PrEscape(hDC, nCode, nLen, lpDataIn, lpDataOut)
  HDC   hDC;
  INT   nCode;
  INT   nLen;
  LPCSTR lpDataIn;
  VOID FAR *lpDataOut;
{
  if (nCode == NEWFRAME)
    cleardevice();
  return 0;
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
static INT FAR PASCAL PrGetDeviceCaps(HDC hDC, int nCode)
{
  switch (nCode)
  {
    case VERTRES :
      return getmaxy();
    case HORZRES :
      return getmaxx();
  }

  return 0;
}

