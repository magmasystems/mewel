/*===========================================================================*/
/*                                                                           */
/* File    : WGRAPHIC.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/*  September 1993: GX Graphics support modified by                          */
/*      Keith Nash                                                           */
/*      Nash Computing Services                                              */
/*      (501)758-3104                                                        */
/*      CSID 71021,2130                                                      */
/*                                                                           */
/* (C) Copyright 1991-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_REGIONS
#define INCLUDE_MOUSE  

#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

#if defined(XWINDOWS) || defined(__GNUC__)
extern int CDECL getch(void);
#endif

/*
  Structure which contains info about a saved screen
*/
typedef struct
{
  UINT  tag;
#define IMAGE_SIGNATURE  0x1234
  UINT  width;
  UINT  rows;
} IMAGE, FAR *LPIMAGE;



#ifdef __cplusplus
extern "C" {
#endif
static INT   PASCAL _GraphicsGetDC(HDC hDC);
static INT   PASCAL _GraphicsReleaseDC(HDC hDC);
extern VOID FAR PASCAL WinCloseGUIFontStuff(void);

BOOL  FAR PASCAL _GTextOut(HDC hDC, INT x, INT y, PSZ pText, int cchText);
BOOL  FAR PASCAL _GGetTextMetrics(HDC hDC, LPTEXTMETRIC lpMetrics);

BOOL  PASCAL _GBltRect(LPRECT lprcDest, LPRECT lprcSrc);

HANDLE PASCAL _GraphicsSaveRect(LPRECT lpRect);
VOID   PASCAL _GraphicsRestoreRect(LPRECT lpRect, HANDLE hBuf);

#if defined(BGI)
BOOL PASCAL BGIInitDriver(int);
int  huge BGIAutoDetect(void);
VOID PASCAL BGISaveScreenDuringSwitch(BOOL);
#endif

#if defined(GX)
static int FAR PASCAL GXFarFree(void far *s);
#if defined(USE_NCS_EXT)
static int FAR PASCAL GXSetVESAMode(void);
#endif
#endif
#ifdef __cplusplus
}
#endif

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  GDI-dependent Routines                                                */
/*                                                                        */
/*------------------------------------------------------------------------*/

extern
FARPROC lpfnGetDCHook,
        lpfnReleaseDCHook,

        lpfnTextOutHook,
        lpfnGetTextMetricsHook,

        lpfnFillRectHook,
        lpfnFrameRectHook,
        lpfnInvertRectHook,
        lpfnRectangleHook,

        lpfnSaveRectHook,
        lpfnRestoreRectHook,
        lpfnBltRectHook,
        lpfnWinFillRectHook,
        lpfnWinInvertRectHook,

        lpfnSetFontHook,
        lpfnDeleteBitmapHook,
        lpfnDeleteRegionHook;


#if defined(META)
METAINFO _MetaInfo =
{
  NULL, NULL,
  NULL,
  0,
  0
};

INT   GrafixCard, GrafixInput;

extern VOID FAR PASCAL MetaInitDefaultFont(void);
#endif


/*
  This is the Windows ROP-to-engine translation table
*/
#if defined(META)
int ROPtoEngineCode[] =
{
  zCLEARz,        /* R2_BLACK            1    0   */
  zNANDNz,        /* R2_NOTMERGEPEN      2   DPon */
  zNANDz,         /* R2_MASKNOTPEN       3   DPna */
  zNREPz,         /* R2_NOTCOPYPEN       4   Pn   */
  zANDNz,         /* R2_MASKPENNOT       5   PDna */
  zINVERTz,       /* R2_NOT              6   Dn   */
  zXORz,          /* R2_XORPEN           7   DPx  */
  zNORNz,         /* R2_NOTMASKPEN       8   DPan */
  zANDz,          /* R2_MASKPEN          9   DPa  */
  zNXORz,         /* R2_NOTXORPEN       10   DPxn */
  zNOPz,          /* R2_NOP             11   D    */
  zNORz,          /* R2_MERGENOTPEN     12   DPno */
  zREPz,          /* R2_COPYPEN	        13   P    */
  zORNz,          /* R2_MERGEPENNOT     14   PDno */
  zORz,           /* R2_MERGEPEN        15   DPo  */
  zSETz,          /* R2_WHITE           16   1    */
};
#endif



/****************************************************************************/
/*                                                                          */
/* Function : WinOpenGraphics()                                             */
/*                                                                          */
/* Purpose  : Calls the underlying graphics engine to put the system into   */
/*            graphics mode. If successful, a bunch of GDI "hooks" are      */
/*            set up which redirects MEWEL GDI calls into this module.      */
/*                                                                          */
/* Returns  : TRUE if the graphics engine was initialized, FALSE if not.    */
/*                                                                          */
/****************************************************************************/
int PASCAL WinOpenGraphics(int iMode)
{
  int rc;
  TEXTMETRIC tm;


  if (SysGDIInfo.bGraphicsSystemInitialized)
    return TRUE;

  /*
    Tell MEWEL that we want to use complex visibility regions
  */
  SET_PROGRAM_STATE(STATE_USE_CLIPREGIONS);

  SysGDIInfo.iSavedVideoMode = VideoInfo.starting_mode;

#if defined(XWINDOWS)
  XWinOpenGraphics();

#elif defined(META)
  {
  char szPath[MAXPATH];
  grafMap *gm;

  (void) rc;

  /*
    Get the path for the MetaWindow drivers from the display.drv variable.
  */
  if (GetProfileString("boot", "display.drv", "", szPath, sizeof(szPath)) > 0)
    mwMetaPath(szPath);

  /*
    See if an initial display mode was specified in the MEWEL.INI file.
    If it wasn't, then use MetQuery() to dectect the graphics card.
  */
  iMode = GetProfileInt("boot", "display.mode", MEWELInitialGraphicsMode);
  if (iMode < 0)
    MetQuery(0, (char **) NULL);
  else
    GrafixCard = iMode;

  /*
    Put the card into graphics mode
  */
  if ((rc = mwInitGraphics(GrafixCard)) != 0)
  {
    printf(
"Fatal Error: MetaWindows init routines failed. Cannot go into graphics mode.\n");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }

  /*
    Set up the default port.
  */
  mwSetDisplay(GrafPg0);
  mwGetPort(&_MetaInfo.pOrigPort);
  mwInitPort(&_MetaInfo.MyPort);
  mwSetPort(&_MetaInfo.MyPort);

  gm = _MetaInfo.pOrigPort->portMap;
  SysGDIInfo.iVideoMode = (int) gm->devMode;
  SysGDIInfo.cxScreen   = gm->pixWidth;
  SysGDIInfo.cyScreen   = gm->pixHeight;
  switch (gm->pixPlanes)
  {
    case 1 :
      if (gm->pixBits == 1)
      {
        SysGDIInfo.nBitsPerPixel = 1;
        SysGDIInfo.nColors = 2;
      }
      else
      {
        SysGDIInfo.nBitsPerPixel = 8;
        SysGDIInfo.nColors = 256;
      }
      break;
    case 4 :
      SysGDIInfo.nBitsPerPixel = 4;
      SysGDIInfo.nColors = 16;
      break;
  }

  /*
    Define a better PS_DOT style.
  */
  {
  static dashRcd dsh;
  static BYTE bOnOff[2] = { 2, 2 };
  dsh.dashSize = 2;
  dsh.dashList = bOnOff;
  mwDefineDash(4, &dsh);
  }
  }

#elif defined(GX)
  /*
    Get the user-defined initial display mode. If it is not defined, then
    use a default display mode based on the video card in use.
  */
  iMode = GetProfileInt("boot", "display.mode", MEWELInitialGraphicsMode);
  if (iMode < 0)
  {
    switch (VideoInfo.flags & 0x07)
    {
      case CGA       :
        iMode = gxCGA_4;
        break;
      case MDA       :
        iMode = gxCGA_6;
        break;
      case EGACOLOR  :
        iMode = gxEGA_10;
        break;
      case EGAMONO   :
        iMode = gxEGA_F;
        break;
      case VGACOLOR  :
        /*
          If we get here, the user hasn't specified a video mode in
          his/her INI file, so check the video adapter for VESA support
          and, if it has it, use that.  Otherwise, drop back to standard VGA
        */
  #if defined(USE_NCS_EXT)
        iMode = GXSetVESAMode();
        if (iMode < 0)
          iMode = gxVGA_12;
  #else
        iMode = gxVGA_12;
  #endif
        break;
      case VGAMONO   :
        iMode = gxVGA_11;
        break;
      default        :
        iMode = gxEGA_10;
        break;
    }
  }


#if defined(GX3)

  /*
    See if the app wants to allocate a custom Genus GX buffer size
      [boot]
      GXBufferSize=16384
    GX gives you a defualt 4K buffer
  */
  rc = GetProfileInt("boot", "GXBufferSize", 0);
  if (rc > 0)
  {
    LPSTR lpGXBuffer = emalloc_far(rc);
    if (lpGXBuffer)
      gxSetBuffer(lpGXBuffer, rc);
  }

  /*
    GX 3.0 requires a call to gxInit for real and protected modes.
  */
  if ((rc = gxInit()) != gxSUCCESS)
    goto err;

#else
#if defined(DOS286X)
  /*
    Initialize Genus protected mode support
  */
  if (gxInitPL286() != gxSUCCESS)
  {
    printf("Fatal Error: GX init routines failed. Cannot go into protected mode.");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }
#elif defined(DOS16M)
  if (gxInitRS286() != gxSUCCESS)
  {
    printf("Fatal Error: GX init routines failed. Cannot go into protected mode.");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }
#endif
#endif /* GX 3.x */


  if ((rc = gxSetDisplay(iMode))   != gxSUCCESS ||
      (rc = gxSetMode(gxGRAPHICS)) != gxSUCCESS)
  {
    (void) rc;
err:
    printf("Fatal Error: GX init routines failed. Cannot go into graphics mode.");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }

  {
  GXDINFO vc;
  gxGetDisplayInfo(iMode, &vc);
  SysGDIInfo.iVideoMode = vc.mode;
  SysGDIInfo.cxScreen   = vc.hres;
  SysGDIInfo.cyScreen   = vc.vres;
  switch (vc.planes)
  {
    case 1 :
      if (vc.bitpx == 1)
      {
        SysGDIInfo.nColors = 2;
        SysGDIInfo.nBitsPerPixel = 1;
      }
      else
      {
        SysGDIInfo.nColors = 256;
        SysGDIInfo.nBitsPerPixel = 8;
      }
      break;
    case 4 :
      SysGDIInfo.nColors = 16;
      SysGDIInfo.nBitsPerPixel = 4;
      break;
  }
  }

#ifdef USE_GENUS_DMM
  gxInstallDMM("", gxCMM);
#endif

  /*
    Install a custom memory management handler for GX. We do not
    have to do this with GX 3.x, because we link in with
    gx_bcl.lib and gr_bcl.lib.
  */
#if defined(__TURBOC__) && !defined(GX3)
#if defined(USE_GX_TEXT)
  gxSetUserMalloc(emalloc_far, GXFarFree, MyFarCoreLeft);
#endif
#endif

#if !defined(EXTENDED_DOS)
  SET_PROGRAM_STATE(STATE_NO_SAVEBITS | STATE_NO_SAVESCREENUNDERCOMBO);
#endif


#elif defined(GURU)
  SetVideoMode(VGA, 1, 0);
  SysGDIInfo.iVideoMode = 0x13;
  SysGDIInfo.cxScreen   = 640;
  SysGDIInfo.cyScreen   = 480;
  SysGDIInfo.nColors    = 16;
  SysGDIInfo.nBitsPerPixel = 4;


#elif defined(MSC)
  {
  char szPath[MAXPATH];
  struct videoconfig vc;

  /*
    Get the starting vide mode and go into graphics mode.
  */
  iMode = GetProfileInt("boot", "display.mode", MEWELInitialGraphicsMode);
  if (iMode < 0)
    iMode = _MAXRESMODE;
  if (_setvideomode(iMode) == 0)
  {
    printf("Fatal Error: _setvideomode() failed. Cannot go into graphics mode.");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }

  /*
    Register all of the fonts in the start-up directory
  */
  GetProfileString("boot", "display.drv", ".", szPath, sizeof(szPath));
  strcat(szPath, "\\*.fon");
  _registerfonts(szPath);

  /*
    Should we use the best-fit for the fonts?
  */
  if (GetProfileInt("boot", "NoBestMatch", 0))
    SysGDIInfo.fFlags |= GDISTATE_MSC_NOBESTMATCH;


  /*
    Get the BIOS graphics values
  */
  _getvideoconfig(&vc);
  SysGDIInfo.iVideoMode = vc.mode;
  SysGDIInfo.cxScreen   = vc.numxpixels;
  SysGDIInfo.cyScreen   = vc.numypixels;
  SysGDIInfo.nColors    = vc.numcolors;
  SysGDIInfo.nBitsPerPixel = vc.bitsperpixel;
  }


#elif defined(BGI)
  if (BGIInitDriver(iMode) == FALSE)
    return FALSE;

  (void) rc;
  SysGDIInfo.iVideoMode = getgraphmode();
  SysGDIInfo.cxScreen   = getmaxx() + 1;
  SysGDIInfo.cyScreen   = getmaxy() + 1;
  SysGDIInfo.nColors    = getmaxcolor() + 1;
  switch (SysGDIInfo.nColors)
  {
    case 2:
      SysGDIInfo.nBitsPerPixel = 1;
      break;
    case 4:
      SysGDIInfo.nBitsPerPixel = 2;
      break;
    case 16:
      SysGDIInfo.nBitsPerPixel = 4;
      break;
    case 256:
      SysGDIInfo.nBitsPerPixel = 8;
      break;
  }
#endif

#if !defined(XWINDOWS)
  _VidInitNewVideoMode();

  if (IsVGA())
    VideoInfo.yFontHeight = 16;
  _GGetTextMetrics((HDC) 0, &tm);
  VideoInfo.yFontHeight = tm.tmHeight + tm.tmExternalLeading;
#endif


  /*
    Set all of the GDI hooks.
  */
#if defined(XWINDOWS)
  lpfnGetDCHook          = (FARPROC) GUIGetDCHook;
  lpfnReleaseDCHook      = (FARPROC) GUIReleaseDCHook;
#else
  lpfnGetDCHook          = (FARPROC) _GraphicsGetDC;
  lpfnReleaseDCHook      = (FARPROC) _GraphicsReleaseDC;
#endif

  lpfnSetFontHook        = (FARPROC) RealizeFont;
  lpfnTextOutHook        = (FARPROC) _GTextOut;
  lpfnGetTextMetricsHook = (FARPROC) _GGetTextMetrics;

  lpfnDeleteBitmapHook   = (FARPROC) EngineDeleteBitmap;

#if !defined(XWINDOWS)
  lpfnSaveRectHook       = (FARPROC) _GraphicsSaveRect;
  lpfnRestoreRectHook    = (FARPROC) _GraphicsRestoreRect;
  lpfnBltRectHook        = (FARPROC) _GBltRect;
#endif

#if defined(XWINDOWS)
  lpfnDeleteRegionHook   = (FARPROC) EngineDeleteRegion;
#endif


  /*
    Turn off the blinking so we can get the full 16 background colors
  */
#if !defined(XWINDOWS)
  if ((SysGDIInfo.bWasBlinkingEnabled = 
                TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) != 0) == TRUE)
    VidSetBlinking(FALSE);
#endif

  /*
    The graphics engine was initialized!
  */
  SysGDIInfo.bGraphicsSystemInitialized = TRUE;
  return TRUE;
}



#if defined(BGI)
BOOL PASCAL BGIInitDriver(iMode)
  int  iMode;
{
  char szPath[MAXPATH];
  char szDriver[16];
  PSTR pszDriver;
  int  gDriver;
  int  gMode;
  int  rc;

  /*
    Get the name of the BGI path
  */
  GetProfileString("boot", "display.drv", "", szPath, sizeof(szPath));

  /*
    See if the entry has the actual name of the BGI driver
  */
#if !defined(USE_BCC2GRX)
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
    */
    installuserdriver(pszDriver, BGIAutoDetect);
    if ((rc = graphresult()) != grOk)
      goto bgi_error;

    /*
      If the display.drv entry was something like 
        display.drv=vga256
      then set the driver path to "".
    */
    if (pszDriver == szPath)
      szPath[0] = '\0';
  }
  else
    iMode = GetProfileInt("boot", "display.mode", 0);
#endif

  gDriver = GetProfileInt("boot", "display.gdriver", DETECT);
  gMode = (iMode == -1) ? 0 : iMode;
  initgraph(&gDriver, &gMode, szPath);
  if ((rc = graphresult()) != grOk)
  {
bgi_error:
    printf("Fatal Error: initgraph() failed.\nMake sure you have EGAVGA.BGI in your current directory");
    getch();
    VidClearScreen(0x07);
    return FALSE;
  }

  (void) rc;

  /*
    Inmark put these two palette settings in.
  */
  setrgbpalette(7, 50,50,50);
  setrgbpalette(56,32,32,32);

  return TRUE;
}


int huge BGIAutoDetect(void)
{
  return GetProfileInt("boot", "display.mode", MEWELInitialGraphicsMode);
}

/****************************************************************************/
/*                                                                          */
/* Function : BGISaveScreenDuringSwitch()                                   */
/*                                                                          */
/* Purpose  : Tells BGI not to restore the screen to text mode when the     */
/*            closegraph() function is called. This allows us to load in    */
/*            another BGI driver (like a printer driver) and still retain   */
/*            the graphics screen when the closegraph() call is issued for  */
/*            the video driver.                                             */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID PASCAL BGISaveScreenDuringSwitch(iSaving)
  BOOL iSaving;
{
#if defined(EXTENDED_DOS)
  (void) iSaving;
#else
  extern BYTE _BGI_auto;
  static BYTE chSaveAuto = 0;

  if (iSaving)
  {
    chSaveAuto = _BGI_auto;
    _BGI_auto  = 0xA5;
  }
  else
  {
    _BGI_auto = chSaveAuto;
  }
#endif
}
#endif /* BGI */


#if defined(GX)
static int FAR PASCAL GXFarFree(void far *s)
{
  MyFree_far(s);
  return 0;
}

/****************************************************************************/
/*                                                                          */
/* Function : GXSetVESAMode(void)                                           */
/*                                                                          */
/* Purpose  : Invoke GX VESA support if the video board has VESA support    */
/*            built in.                                                     */
/*                                                                          */
/* Returns  : Highest resolution of board or -1 if VESA unavailable.        */
/*                                                                          */
/* Keith Nash, Nash Computing Services                                      */
/*                                                                          */
/****************************************************************************/
#if defined(USE_NCS_EXT)
static int FAR PASCAL GXSetVESAMode(void)
{
  GXVINFO    vi[2];
  GXCINFO    ci;
  GXVESAINFO vesa;

  if (gxVESAInstalled(&vesa) == gxSUCCESS)
  {
    /* Found VESA support! */
    if (gxSetChipset(chipVESA) == gxSUCCESS)
      if (gxVideoInfo(vi) == gxSUCCESS)
        if (vi[0].adapter == viVGA)
          if (gxChipsetInfo(&ci) == gxSUCCESS)
            return ci.maxdisp;
  }

  return -1;
}
#endif
#endif


/****************************************************************************/
/*                                                                          */
/* Function : WinCloseGraphics(void)                                        */
/*                                                                          */
/* Purpose  : Terminates the underlying graphics engine and removes the     */
/*            series of hooks.                                              */
/*                                                                          */
/* Returns  : TRUE.                                                         */
/*                                                                          */
/****************************************************************************/
int PASCAL WinCloseGraphics(void)
{
  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return TRUE;

#if defined(META)
  if (SysGDIInfo.hFontDefault && !TEST_PROGRAM_STATE(STATE_SPAWNING))
  {
    GlobalFree(SysGDIInfo.hFontDefault);
    SysGDIInfo.hFontDefault = NULL;
  }
//mwSetDisplay(TextPg0);
  mwStopGraphics();

#elif defined(GX)
#ifdef USE_GENUS_DMM
  if (gxDMInstalled() == gxSUCCESS)
    gxRemoveDMM(gxCMM);
#endif
  gxSetMode(gxTEXT);

#elif defined(GURU)
  TextMode(-1);


#elif defined(MSC)
  _setvideomode(SysGDIInfo.iSavedVideoMode);
  _unregisterfonts();

#elif defined(BGI)
  closegraph();

#endif

  lpfnGetDCHook           = (FARPROC) 0;
  lpfnReleaseDCHook       = (FARPROC) 0;

  lpfnTextOutHook         = (FARPROC) 0;

#if !defined(XWINDOWS)
  lpfnGetTextMetricsHook  = (FARPROC) 0;

  lpfnSaveRectHook        = (FARPROC) 0;
  lpfnRestoreRectHook     = (FARPROC) 0;
  lpfnBltRectHook         = (FARPROC) 0;

  lpfnSetFontHook         = (FARPROC) 0;
  lpfnDeleteBitmapHook    = (FARPROC) 0;
#endif


#if !defined(XWINDOWS)
  WinCloseGUIFontStuff();
  SysGDIInfo.bGraphicsSystemInitialized = FALSE;
  if (SysGDIInfo.bWasBlinkingEnabled)
    VidSetBlinking(TRUE);
#ifndef MEWEL_GUI
  VidSetVideoMode(SysGDIInfo.iSavedVideoMode);
#endif
#endif

  return TRUE;
}


LPHDC FAR PASCAL GDISetup(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  /*
    Make sure that the graphics system is initialized
  */
  if (!SysGDIInfo.bGraphicsSystemInitialized ||
  /*
    Make sure that the DC is good
  */
      (lphDC = _GetDC(hDC)) == NULL          ||
  /*
    Set the clipping region and make sure that the clip region is not NULL
  */
      !_GraphicsSetViewport(hDC))
    return NULL;

  return lphDC;
}


/****************************************************************************/
/*                                                                          */
/* Function : _GraphicsGetDC/GraphicsReleaseDC()                            */
/*                                                                          */
/* Purpose  : Sets up the viewport and clipping regions for a DC            */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
#define MAXVIEWPORTS    16

#if defined(META) || defined(GX) || defined(GURU)
static RECT   viewOriginal;
#elif defined(BGI)
static struct viewporttype viewOriginal[MAXVIEWPORTS];
#endif
static int    nDCStackSP   = 0;
static BOOL   bCaretPushed = FALSE;


static INT PASCAL _GraphicsGetDC(HDC hDC)
{
  LPHDC lphDC;

  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return 0;
  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (nDCStackSP == 0)
  {
    bCaretPushed = CaretPush(lphDC->rClipping.top, lphDC->rClipping.bottom);
  }

#if defined(META)
  if (nDCStackSP == 0)
  {
    grafPort *gp;
    mwGetPort(&gp);
    _MetaInfo.thePort = gp;
    viewOriginal.left   = gp->portRect.Xmin;
    viewOriginal.top    = gp->portRect.Ymin;
    viewOriginal.right  = gp->portRect.Xmax;
    viewOriginal.bottom = gp->portRect.Ymax;
  }

  if (lphDC->lpDCExtra == NULL)
    lphDC->lpDCExtra = EMALLOC_FAR(sizeof(METADCINFO));


#elif defined(GX)
  if (nDCStackSP == 0)
    grGetViewPort(&viewOriginal.left, &viewOriginal.top, 
                  &viewOriginal.right, &viewOriginal.bottom);

  if (lphDC->lpDCExtra == NULL)
    lphDC->lpDCExtra = EMALLOC_FAR(sizeof(GXDCINFO));

#elif defined(GURU)

#elif defined(MSC)

#elif defined(BGI)
  if (nDCStackSP >= MAXVIEWPORTS || nDCStackSP < 0)
  {
    VidClearScreen(0x07);
    printf("GetDC() - nDCStackSP (%d) exceeeds MAXVIEWPORTS\n", nDCStackSP);
    getch();
    exit(1);
  }

  getviewsettings(&viewOriginal[nDCStackSP]);

#if defined(GNUGRX)
  if (lphDC->lpDCExtra == NULL)
    lphDC->lpDCExtra = EMALLOC_FAR(sizeof(GRXDCINFO));
#endif

#endif

  /*
    Initialize the DC structure
  */
  if (!IS_RESETING_OWNDC(lphDC))
  {
    lphDC->clrText       = RGB(0x00, 0x00, 0x00);
    lphDC->clrBackground = GetSysColor(COLOR_WINDOW);
    lphDC->attr          = MAKE_ATTR(BLACK, INTENSE(WHITE));
  }

#if defined(META)
  if (lphDC->iLockCnt <= 1)
  {
    ((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort = (grafPort FAR *) EMALLOC_FAR(sizeof(grafPort));
    mwInitPort(((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort);
    MetaSetDefaultFont();
  }
#endif

#if defined(GX)
#if defined(USE_GX_TEXT)
  if (lphDC->iLockCnt <= 1)
  {
    GXSetDefaultFont();
  }
#endif
#endif

  /*
    For the GUI, calculate the window's visible region
  */
#ifdef USE_REGIONS
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS) && lphDC->hWnd != _HwndDesktop)
    lphDC->hRgnVis = WinCalcVisRgn(lphDC->hWnd, hDC);
#endif


  /*
    Map the current pen, brush, and write-mode into device-specific objects
  */
  RealizePen(hDC);
  RealizeBrush(hDC);
  RealizeROP2(hDC);
  RealizeFont(hDC);

  if (!IS_RESETING_OWNDC(lphDC))
    nDCStackSP++;
  return TRUE;
}


static INT PASCAL _GraphicsReleaseDC(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return 0;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  if (!IS_RESETING_OWNDC(lphDC) && --nDCStackSP == 0)
  {
#if defined(META)
    mwSetPort(&_MetaInfo.MyPort);

#elif defined(GX)
    grSetClipping(grNOCLIP);
    GXVirtualScreen(FALSE);
    grSetViewPort(viewOriginal.left,  viewOriginal.top,
                  viewOriginal.right, viewOriginal.bottom);
    ((LPGXDCINFO)lphDC->lpDCExtra)->gxHeader = NULL;

#elif defined(MSC)
     _setviewport(0,
                  0,
                  VideoInfo.width - 1,
                  VideoInfo.length - 1);

#elif defined(BGI)
    setviewport(viewOriginal[nDCStackSP].left,  viewOriginal[nDCStackSP].top,
                viewOriginal[nDCStackSP].right, viewOriginal[nDCStackSP].bottom, 
                viewOriginal[nDCStackSP].clip);

#endif


    if (bCaretPushed)
    {
      CaretPop();
      bCaretPushed = FALSE;
    }
  }
  else
  {
#if defined(META)
    /*
      In case we are holding on to a DC, set a valid MetaWindows port
      anyway.
    */
    mwSetPort(&_MetaInfo.MyPort);

#elif defined(BGI) && !defined(GX)
    setviewport(viewOriginal[nDCStackSP].left,  viewOriginal[nDCStackSP].top,
                viewOriginal[nDCStackSP].right, viewOriginal[nDCStackSP].bottom, 
                viewOriginal[nDCStackSP].clip);

#endif
  }

  /*
    Get rid of any memory in the lpDCExtra field
  */
#if defined(META)
  if (lphDC->lpDCExtra != NULL && !IS_RESETING_OWNDC(lphDC))
  {
    LPMETADCINFO lpM = (LPMETADCINFO) lphDC->lpDCExtra;
    if (lpM->pClipRegion)
    {
      mwGrafFree(lpM->pClipRegion);
      mwClipRegion(NULL);
    }

    /*
      Don't free the bitmap here, since DeleteObject will do it
        MYFREE_FAR(((LPMETADCINFO) lphDC->lpDCExtra)->pGrafMap);
    */
    if (lphDC->iLockCnt == 0)
    {
      MYFREE_FAR(((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort);
      ((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort = NULL;
    }
  }
#endif

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _GraphicsSetViewport(HDC hDC)                                 */
/*                                                                          */
/* Purpose  : Given a display context, sets the "viewport" to the           */
/*            upper left corner of the corresponding clipping area.         */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL _GraphicsSetViewport(HDC hDC)
{
  RECT  rClipping, rBounding;
  LPHDC lphDC;
#if defined(GX)
  GXHEADER *pgxHeader;
#endif

  if (!SysGDIInfo.bGraphicsSystemInitialized)
    return FALSE;
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    If this DC is associated with a non-screen device (printer, plotter),
    then set the DC's clipping rectangle and the viewport to the extent
    of the device.
  */
  if (lphDC->hWnd == HWND_NONDISPLAY)
  {
    SetRect(&lphDC->rClipping,
              0, 0, GetDeviceCaps(hDC,HORZRES), GetDeviceCaps(hDC,VERTRES));
    rClipping = lphDC->rClipping;
    goto next;
  }


#if 1694
  if (IS_DCATOMIC(lphDC) && !IS_OWN_OR_CLASSDC(lphDC))
    return TRUE;
#endif

  /*
    Bound the viewport by the window's clipping rectangle and by the
    physical screen. If we have a printer DC, then do not clip to screen.
    Rather, clip to the device dimensions.
  */
  if (IS_PRTDC(lphDC))
  {
    SetRect(&rClipping, 0, 0, lphDC->devInfo.cxPixels, lphDC->devInfo.cyPixels);
  }
  else
  {
#if defined(XWINDOWS)
    rClipping = lphDC->rClipping;
#else
    WinGenAncestorClippingRect(lphDC->hWnd, hDC, &rClipping);
#endif
  }
  if (IsRectEmpty(&rClipping))
    return FALSE;

#ifdef USE_REGIONS
  /*
    See if the visibilty region is empty. If it is, do not draw anything.
    The exception is for a DC for the desktop window. The rubberbanding
    routine (and many WinApps) need to get a DC for the desktop in order
    to implement rubberbanding.

    Also, do not pay attention to the visible regions if we have a memory DC.
  */
  if (TEST_PROGRAM_STATE(STATE_USE_CLIPREGIONS))
  {
    if (lphDC->hWnd != _HwndDesktop && !IS_MEMDC(lphDC))
      if (IsVisRegionEmpty(hDC, &rBounding) ||
          !IntersectRect(&rBounding, &rClipping, &rBounding))
        return FALSE;
  }
#endif


next:
#if !defined(META) && !defined(XWINDOWS)
  /*
    BGI, MSC and GX don't include the bottom corner
  */
  rClipping.right--;
  rClipping.bottom--;
#endif


#if defined(XWINDOWS)
  if (lphDC->gc && lphDC->hRgnClip == NULL)
  {
    XRectangle xRect;
    WINDOW     *w = WID_TO_WIN(lphDC->hWnd);

    /*
      In a top-level window, since the GC is based on the drawing area,
      forget about the non-client area
    */
    xRect.x = xRect.y = 0;
    xRect.width  = rClipping.right - rClipping.left;
    xRect.height = rClipping.bottom - rClipping.top;
    XSetClipRectangles(XSysParams.display,  /* display          */
                       lphDC->gc,           /* graphics context */
                       rClipping.left,      /* x clip origin    */
                       rClipping.top,       /* y clip origin    */
                       &xRect,              /* Array of rects   */
                       1,                   /* number of XRects */
                       Unsorted);           /* way the rects are laid out */
  }


#elif defined(META)
  mwSetPort(((LPMETADCINFO) lphDC->lpDCExtra)->pGrafPort);
  if (!IS_BMPSELECTED(lphDC))
  {
    mwMovePortTo(rClipping.left, rClipping.top);
#if 22894
    mwPortSize(RECT_WIDTH(rClipping)+1, RECT_HEIGHT(rClipping)+1);
#else
    mwPortSize(RECT_WIDTH(rClipping), RECT_HEIGHT(rClipping));
#endif
  }

#elif defined(GX)
  /*
    If we have a virtual screen attached to this DC, map all screen
    drawing to it.
  */
  pgxHeader = ((LPGXDCINFO) lphDC->lpDCExtra)->gxHeader;
  if (pgxHeader)
  {
    grSetActiveVirtual(pgxHeader);
#if defined(USE_GX_TEXT)
    txSetActiveVirtual(pgxHeader);
#endif
    GXVirtualScreen(TRUE);
  }
  else
  {
    GXVirtualScreen(FALSE);
  }
  grSetViewPort(rClipping.left,
                rClipping.top,
                rClipping.right,
                rClipping.bottom);
  grSetClipRegion(rClipping.left,
                  rClipping.top,
                  rClipping.right,
                  rClipping.bottom);
  grSetClipping(grCLIP);


#elif defined(MSC) || defined(BGI) || defined(GURU)
  _setviewport(rClipping.left,
               rClipping.top,
               rClipping.right,
               rClipping.bottom);
#endif


#if !defined(META) && !defined(XWINDOWS)
  /*
    The rectLastClipping variable should be like Windows rectangles. This
    rectangle will be used in the various GDI functions to determine the
    DC's clipping recgion.
  */
  rClipping.right++;
  rClipping.bottom++;
#endif
  SysGDIInfo.rectLastClipping = rClipping;

  return TRUE;
}


#if defined(META)
VOID FAR PASCAL _setviewport(int x1, int y1, int x2, int y2)
{
  grafPort *p;

  mwMovePortTo(x1, y1);
  mwPortSize(x2-x1+1, y2-y1+1);
  mwGetPort(&p);
  mwClipRect(&p->portRect);
}
#endif


VOID FAR PASCAL WindowRectToPixels(HWND hWnd, LPRECT lpRect)
{
  GetWindowRect(hWnd, lpRect);
  OffsetRect(lpRect, -lpRect->left, -lpRect->top);
}


#if defined(GX)
VOID FAR PASCAL GXVirtualScreen(bEnable)
  BOOL bEnable;
{
  if (bEnable)
  {
    grSetVirtual(TRUE);
#if defined(USE_GX_TEXT)
    txSetVirtual(TRUE);
#endif
    SysGDIInfo.fFlags |= GDISTATE_GX_VIRTUALON;
  }
  else
  {
    if (SysGDIInfo.fFlags & GDISTATE_GX_VIRTUALON)
    {
      grSetVirtual(FALSE);
#if defined(USE_GX_TEXT)
      txSetVirtual(FALSE);
#endif
      SysGDIInfo.fFlags &= ~GDISTATE_GX_VIRTUALON;
    }
  }
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : RealizeBrush()                                                */
/*                                                                          */
/* Purpose  : Realizes the brush which is selected in the device context    */
/*                                                                          */
/* Returns  : TRUE if realized and if not a NULL brush.                     */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL RealizeBrush(HDC hDC)
{
#if defined(XWINDOWS)
  return XMEWELRealizeBrush(hDC);

#else
  /* HORZ      VERT       FDIAG           BDIAG         CROSS       DIAGCROSS   */
  static UINT awHatch[6] =
  #if defined(META)
  {  19,       21,        8,              13,           22,         24          };
  #elif defined(GX)
  { grFLINE,   grFINTER,  grFBKSLASH,     grFSLASH,     grFHATCH,   grFCLOSEDOT };
  #elif defined(BGI)
  { LINE_FILL, USER_FILL, LTBKSLASH_FILL, LTSLASH_FILL, HATCH_FILL, XHATCH_FILL };
  #elif defined(MSC)
  { 0, 0, 0, 0, 0, 0 };
  #endif
  
  #if defined(MSC) || defined(GURU)
  static PSTR achHatch[6] =
  { "\xFF\x00\x00\x00\xFF\x00\x00\x00", "\x11\x11\x11\x11\x11\x11\x11\x11",
    "\x80\x40\x20\x10\x08\x04\x02\x01", "\x01\x02\x04\x08\x10\x20\x40\x80",
    "\x08\x08\x08\xFF\x08\x08\x08\x08", "\x81\x42\x24\x18\x18\x24\x42\x81"      };
  #endif
  

  LPOBJECT lpObj;
  LPHDC    lphDC;
  int      iStyle;
  COLOR    attr;



  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;
  if ((lpObj = _ObjectDeref(lphDC->hBrush)) == NULL)
    return FALSE;


  /*
    If the style is BS_NULL, return FALSE.
  */
  if ((iStyle = lpObj->uObject.uLogBrush.lbStyle) == BS_NULL ||
       iStyle == BS_INDEXED
#if !defined(META)
       || iStyle == BS_DIBPATTERN
#endif
     )
    return FALSE;

  /*
    Get the brush color.
  */
  attr = RGBtoAttr(hDC, lpObj->uObject.uLogBrush.lbColor);

#if defined(META)
  {
  int iPattern;

  switch (iStyle)
  {
    case BS_SOLID            :
      iPattern = 0;  /* use the background color */
      break;

    case BS_HATCHED          :
      iPattern = awHatch[lpObj->uObject.uLogBrush.lbHatch];
      break;

    case BS_PATTERN          :
    case BS_DIBPATTERN       :
      if (lpObj->lpExtra)
      {
        mwDefinePattern(31, (patRcd *) lpObj->lpExtra);
        mwBackPattern(31);
      }
      return TRUE;
  }

  if (lpObj->uObject.uLogBrush.lbStyle == BS_HATCHED)
  {
    mwPenColor((attr == INTENSE(WHITE)) ? -1 : attr);
    attr = RGBtoAttr(hDC, lphDC->clrBackground);
  }
  mwBackColor((attr == INTENSE(WHITE)) ? -1 : attr);
  if (iPattern != -1)
    mwBackPattern(iPattern);
  }


#elif defined(GX)
  {
  int iPattern;

  switch (iStyle)
  {
    case BS_SOLID            :
      iPattern = grFSOLID;
      break;
    case BS_HATCHED          :
      iPattern = awHatch[lpObj->uObject.uLogBrush.lbHatch];
      /*
        Use the background color for the hatch background
      */
      grSetBkColor(RGBtoAttr(hDC, lphDC->clrBackground));
      break;
    case BS_PATTERN          :
      return FALSE;
  }

  grSetColor(attr);
#ifdef NOTYET
  if (iPattern == grFUSER)
    grSetFillPattern(grFUSER, pMask);
  else
#endif
    grSetFillStyle(iPattern, attr, grOPAQUE);
  }


#elif defined(MSC) || defined(GURU)
  _setcolor(attr);
  switch (iStyle)
  {
    case BS_SOLID            :
      _setfillmask("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
      return TRUE;  /* brush was set */
    case BS_HATCHED          :
      _setfillmask(achHatch[lpObj->uObject.uLogBrush.lbHatch]);
      return TRUE;  /* brush was set */
    case BS_PATTERN          :
      return FALSE;
  }


#elif defined(BGI)
  {
  int  iPattern = USER_FILL;
  PSTR pMask = NULL;

  switch (iStyle)
  {
    case BS_SOLID            :
      iPattern = SOLID_FILL;
      break;

    case BS_HATCHED          :
      iPattern = awHatch[lpObj->uObject.uLogBrush.lbHatch];
      if (lpObj->uObject.uLogBrush.lbHatch == HS_VERTICAL)
        pMask = "\x11\x11\x11\x11\x11\x11\x11\x11";
      break;

    case BS_PATTERN          :
    {
      HBITMAP hbm;
      BITMAP  bmp;

      iPattern = USER_FILL;
      hbm = lpObj->uObject.uLogBrush.lbHatch;
      GetObject(hbm, sizeof(BITMAP), (VOID FAR *) &bmp);
      pMask = bmp.bmBits + (2*sizeof(WORD));
      break;
    }
  }

  setcolor(attr);
  if (iPattern == USER_FILL)
  {
    setfillpattern(pMask, min(attr, getmaxcolor()));
  }
  else
  {
    setfillstyle(iPattern, attr);
#if 0
    /*
      For a hatched brush, we must set the background color which goes
      between the hatches.
    */
    if (lpObj->uObject.uLogBrush.lbStyle == BS_HATCHED)
      setbkcolor(GET_BACKGROUND(lphDC->attr));
#endif
  }
  }

#endif

  return TRUE;

#endif /* XWINDOWS */
}



/****************************************************************************/
/*                                                                          */
/* Function : RealizePen()                                                  */
/*                                                                          */
/* Purpose  : Realizes the pen                                              */
/*                                                                          */
/* Returns  : TRUE if the pen was realized and is not a NULL pen            */
/*                                                                          */
/****************************************************************************/
BOOL PASCAL RealizePen(HDC hDC)
{
#if defined(XWINDOWS)
  return XMEWELRealizePen(hDC);

#else
  LPOBJECT lpObj;
  LPHDC    lphDC;

  static int aiPenMask[5] =
#if defined(META)
  { 0, 3, 4, 7, 6 };
#elif defined(GX)
  { grLSOLID, grLMEDDASH, grLWIDEDOT, grLDASHDOT, 0xFF66 };
#elif defined(MSC)
  { 0xFFFF, 0xF8F8, 0xAAAA, 0xFF18, 0xFF66 };
#elif defined(BGI)
  { SOLID_LINE, DASHED_LINE, DOTTED_LINE, USERBIT_LINE, USERBIT_LINE };
  static UINT awPattern[5] =
  { 0, 0, 0, 0xFF18, 0xFF66 };
#endif


  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;


  if ((lpObj = _ObjectDeref(lphDC->hPen)) != NULL)
  {
    int   mask;
    int   iStyle;
    int   iWidth;
    COLOR attr;

    /*
      Don't do anything for NULL pens
    */
    if ((iStyle = lpObj->uObject.uLogPen.lopnStyle) == PS_NULL)
      return FALSE;

    /*
      Set the pen color
    */
    attr = RGBtoAttr(hDC, lpObj->uObject.uLogPen.lopnColor);
    _setcolor(attr);

    /*
      Get the engine-specific pen style
    */
    if (iStyle >= 5 || iStyle < 0)
      iStyle = 0;
    mask = aiPenMask[iStyle];

    /*
      Get the width.
    */
    iWidth = lpObj->uObject.uLogPen.lopnWidth.x;
    if (iWidth == 0)
      iWidth = 1;

#if defined(META)
    mwPenDash(mask);
    mwPenSize(iWidth, iWidth);
#elif defined(GX)
    grSetLineStyle(mask, iWidth);
#elif defined(MSC)
    (void) iWidth;
    _setlinestyle(mask);
#elif defined(BGI)
    setlinestyle(mask,awPattern[iStyle],(iWidth<=1) ? NORM_WIDTH : THICK_WIDTH);
#endif
  }

  return TRUE;
#endif /* XWINDOWS */
}


/****************************************************************************/
/*                                                                          */
/* Function : RealizeROP2()                                                 */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID PASCAL RealizeROP2(HDC hDC)
{
  int   rop;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return;

  rop = lphDC->wDrawingMode;
  if (rop < R2_BLACK || rop > R2_WHITE)
    rop = R2_COPYPEN;

#if defined(XWINDOWS)
  XSetFunction(XSysParams.display, lphDC->gc, ROPtoEngineCode[rop-1]);

#elif defined(META)
  mwRasterOp(ROPtoEngineCode[rop-1]);

#elif defined(GX)
  switch (rop)
  {
    case R2_COPYPEN :
      rop = gxSET;
      break;
    case R2_NOT :
      rop = 0;
      break;
    case R2_MASKPEN  :
      rop = gxAND;
      break;
    case R2_MERGEPEN  :
      rop = gxOR;
      break;
    case R2_NOTXORPEN :
    case R2_XORPEN :
      rop = gxXOR;
      break;
    default   :
      rop = gxSET;
      break;
  }

  grSetOp(rop);

#elif defined(GURU)
  (void) rop;

#elif defined(MSC) || defined(BGI)
  switch (rop)
  {
    case R2_COPYPEN   :
      rop = _GPSET;
      break;
    case R2_NOT       :
#if 20293 && defined(MSC)
      rop = _GXOR;
#else
      rop = _GPRESET;
#endif
      break;
    case R2_MASKPEN   :
      rop = _GAND;
      break;
    case R2_MERGEPEN  :
      rop = _GOR;
      break;
    case R2_XORPEN    :
    case R2_NOTXORPEN :
      rop = _GXOR;
      break;
    case R2_BLACK     :
      rop = _GPRESET;
      break;
    default           :
      rop = _GPSET;
      break;
  }

  _setwritemode(rop);

#endif
}



/****************************************************************************/
/*                                                                          */
/*        ROUTINES FOR SAVING AND RESTORING A GRAPHICS SCREEN               */
/*                                                                          */
/****************************************************************************/

#define MAXSAVEDIMAGES  8


/*
  Structure which contains info about a saved screen
*/
typedef struct tagImage
{
#if defined(GX)
  GXHEADER gxHeader;
#else
  HANDLE   hpBuf[4];
#endif
  RECT     rectImage;
  DWORD    ulSize;
  int      yIncrement;
} SAVEDIMAGE;

static SAVEDIMAGE _SavedImage[MAXSAVEDIMAGES];
static int _SavedImageSP = -1;



BOOL  PASCAL _GBltRect(LPRECT lprcDest, LPRECT lprcSrc)
{
#if defined(META)
  int  cx, cy;

  cx = lprcDest->left - lprcSrc->left;
  cy = lprcDest->top  - lprcSrc->top;
  mwScrollRect((rect *) lprcSrc, cx, cy);

#else
  HANDLE hImg;
  if ((hImg = _GraphicsSaveRect(lprcSrc)) == NULL)
    return FALSE;
  _GraphicsRestoreRect(lprcDest, hImg);
#endif
  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _GraphicsSaveRect()                                           */
/*                                                                          */
/* Purpose  : Saves an underlying screen image in graphics mode. This       */
/*            is called by the hook in WinSaveRect().                       */
/*            Also called by InvertRect and BltRect.                        */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
HANDLE PASCAL _GraphicsSaveRect(lpRect)
  LPRECT lpRect;
{
  int     i, nBands;
#if defined(META)
  DWORD   size;
#else
  UINT    size;
#endif
  UINT    y;
  HANDLE  hRetVal;
  LPSTR   lpRetVal;
  LPIMAGE lpImg;
  HANDLE  hMem;
  LPSTR   lpMem;
  BOOL    bCaretPushed;
  RECT    rSave;

  static IMAGE wagnerBuf = { IMAGE_SIGNATURE, 0, 0 };


  (void) i;
  (void) y;
  (void) nBands;
  (void) lpImg;
  (void) lpMem;
  (void) hMem;
  (void) size;


  if ((hRetVal = GlobalAlloc(GMEM_ZEROINIT, (DWORD) sizeof(IMAGE))) == NULL)
    return NULL;
  lpRetVal = GlobalLock(hRetVal);
  memcpy(lpRetVal, &wagnerBuf, sizeof(IMAGE));

  if (_SavedImageSP >= MAXSAVEDIMAGES)
  {
    GlobalUnlock(hRetVal);
    GlobalFree(hRetVal);
    return NULL;
  }
  _SavedImageSP++;


  /*
    Generate the coordinates of the rectangle to save. Portable
    over GUI and non-GUI.
  */
  SetRect(&rSave, 
          lpRect->left,
          lpRect->top,
          lpRect->right - 1, 
          lpRect->bottom - 1);

#if defined(META)
  /*
    MetaWindows is like Windows
  */
  rSave.right++;
  rSave.bottom++;
#endif

  _SavedImage[_SavedImageSP].rectImage = rSave;


  /*
    Hide the mouse and the caret
  */
  MouseHide();
  bCaretPushed = CaretPush(rSave.top, rSave.bottom);

#if defined(META)
  mwSetPort(&_MetaInfo.MyPort);
  mwRasterOp(zREPz);

  /*
    Since the BGI get/putimage() routines only deal with 64K max, we need
    to use a banding technique to store the screen.
  */

  for (i = 0;  i < 4;  i++)
    _SavedImage[_SavedImageSP].hpBuf[i] = NULL;

  _SavedImage[_SavedImageSP].ulSize = mwImageSize((rect *) &rSave);

  /*
    Image too big? Save the whole screen.
  */
  if ((_SavedImage[_SavedImageSP].ulSize >= 0xFFC0))
  {
    UINT maxx, maxy;
    RECT rTmp;

    maxy = rSave.bottom - rSave.top;
    maxx = rSave.right  - rSave.left;

    _SavedImage[_SavedImageSP].yIncrement = (maxy + 1) / 4;
    SetRect(&rTmp, 0, 0, maxx, (maxy+4) / 4);
    size = mwImageSize((rect *) &rTmp);
    _SavedImage[_SavedImageSP].ulSize = size * 4L;
    nBands = 4;
  }
  else
  {
    RECT rTmp;

    _SavedImage[_SavedImageSP].yIncrement = (int)
 ((rSave.bottom-rSave.top+1) / (_SavedImage[_SavedImageSP].ulSize/64000L + 1));

    SetRect(&rTmp, rSave.left, rSave.top, rSave.right, 
                      rSave.top + _SavedImage[_SavedImageSP].yIncrement - 1);
    size = mwImageSize((rect *) &rTmp);
    _SavedImage[_SavedImageSP].ulSize = size;
    nBands = (int) (_SavedImage[_SavedImageSP].ulSize / 64000L) + 1;
  }




  y = rSave.top;

  for (i = 0;  i < nBands;  i++)
  {
    if ((hMem = _SavedImage[_SavedImageSP].hpBuf[i] = 
                      GlobalAlloc(GMEM_MOVEABLE, (DWORD) size)) != NULL)
    {
      RECT rTmp;

      if ((lpMem = GlobalLock(hMem)) == NULL)
        goto badalloc;
      SetRect(&rTmp, rSave.left,  y, 
                     rSave.right, y + _SavedImage[_SavedImageSP].yIncrement-1);
      mwReadImage((rect *) &rTmp, lpMem);
      GlobalUnlock(hMem);
      y += _SavedImage[_SavedImageSP].yIncrement;
    }
    else
      goto badalloc;
  }


#elif defined(GX)

#if 1

#ifdef USE_GENUS_DMM
  if (gxCreateVirtual(gxDMM,
#else
  if (gxCreateVirtual(gxCMM,
#endif
       &_SavedImage[_SavedImageSP].gxHeader, gxGetDisplay(),
       rSave.right - rSave.left + 1,
       rSave.bottom - rSave.top + 1) == gxSUCCESS)
  {
    /*
      If we are inverting a rectangle (a small area), then use GetImage.
    */
    if (SysGDIInfo.fFlags & GDISTATE_INVERTINGRECT)
      gxGetImage(&_SavedImage[_SavedImageSP].gxHeader,
                 rSave.left, rSave.top, rSave.right, rSave.bottom, 0);
    else
      gxDisplayVirtual(rSave.left, rSave.top, rSave.right, rSave.bottom,
                       0, &_SavedImage[_SavedImageSP].gxHeader, 0, 0);
  }
  else
  {
    _SavedImage[_SavedImageSP].gxHeader.id = 0;
    goto badalloc;
  }
#endif


#elif defined(MSC) || defined(BGI) || defined(GURU)
  /*
    Since the BGI get/putimage() routines only deal with 64K max, we need
    to use a banding technique to store the screen.
  */

  for (i = 0;  i < 4;  i++)
    _SavedImage[_SavedImageSP].hpBuf[i] = NULL;

  _SavedImage[_SavedImageSP].ulSize =
             _imagesize(rSave.left, rSave.top, rSave.right, rSave.bottom);


  /*
    Image too big? Save the whole screen.
  */
  if ((_SavedImage[_SavedImageSP].ulSize == 0xFFFF) ||
      (_SavedImage[_SavedImageSP].ulSize == NULL))
  {
    UINT maxx, maxy;

    maxy = rSave.bottom - rSave.top;
    maxx = rSave.right  - rSave.left;

    _SavedImage[_SavedImageSP].yIncrement = (maxy + 1) / 4;
    size = _imagesize(0, 0, maxx, (maxy+4) / 4);
    _SavedImage[_SavedImageSP].ulSize = size * 4;
    nBands = 4;
  }
  else
  {
    _SavedImage[_SavedImageSP].yIncrement =
     (rSave.bottom-rSave.top+1) / (_SavedImage[_SavedImageSP].ulSize/64000L + 1);
    size = _imagesize(rSave.left, rSave.top, rSave.right, 
                      rSave.top + _SavedImage[_SavedImageSP].yIncrement - 1);
    nBands = (int) (_SavedImage[_SavedImageSP].ulSize / 64000L) + 1;
  }



  y = rSave.top;

  for (i = 0;  i < nBands;  i++)
  {
    if ((hMem = _SavedImage[_SavedImageSP].hpBuf[i] = 
                      GlobalAlloc(GMEM_MOVEABLE, (DWORD) size)) != NULL)
    {
      if ((lpMem = GlobalLock(hMem)) == NULL)
        goto badalloc;
      _getimage(rSave.left,  y, 
                rSave.right, y + _SavedImage[_SavedImageSP].yIncrement - 1, 
                (void FAR *) lpMem);
      GlobalUnlock(hMem);
      y += _SavedImage[_SavedImageSP].yIncrement;
    }
    else
      goto badalloc;
  }

#endif

  /*
    Restore the mouse and caret
  */
  MouseShow();
  if (bCaretPushed)
    CaretPop();

  GlobalUnlock(hRetVal);
  return hRetVal;


badalloc:
  MouseShow();
  if (bCaretPushed)
    CaretPop();
#if !defined(GX)
  while (--i >= 0)
    GlobalFree(_SavedImage[_SavedImageSP].hpBuf[i]);
#endif
  _SavedImageSP--;
  GlobalUnlock(hRetVal);
  GlobalFree(hRetVal);
  return NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : _GraphicsRestoreRect()                                        */
/*                                                                          */
/* Purpose  : Restores a previously saved graphics screen image. Called by  */
/*            the hook in WinRestoreRect().                                 */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID PASCAL _GraphicsRestoreRect(lpRect, hImg)
  LPRECT lpRect;
  HANDLE hImg;    /* Thomas Wagner's code uses this */
{
  int     i, x, y;
  LPIMAGE lpImg;
  LPSTR   lpBuf;
  LPSTR   lpMem;
  BOOL    bCaretPushed;
  RECT    rSave;

  (void) hImg;
  (void) lpImg;
  (void) lpMem;
  (void) lpBuf;
  (void) i;
  (void) x;
  (void) y;


  if (_SavedImageSP < 0)
    return;

  MouseHide();
  bCaretPushed = CaretPush(lpRect->top, lpRect->bottom);

  /*
    Generate the coordinates of the rectangle to save. Portable
    over GUI and non-GUI.
  */
  SetRect(&rSave, 
          lpRect->left,
          lpRect->top,
          lpRect->right - 1, 
          lpRect->bottom - 1);


#if defined(GX)
  if (_SavedImage[_SavedImageSP].gxHeader.id)
  {
    if (SysGDIInfo.fFlags & GDISTATE_INVERTINGRECT)
    {
#if 52193
      /*
        5/21/93 (maa)
          If we XOR an area of the screen which has the BIOS color 7,
        then we end up with a black area and we cannot restore it
        by XORing again. So, before we XOR back the area, fill it
        with bright white (0xF) so that the XORing will really work.
            1111
          ^ 0111
            ----
            1000
      */
      int x1, y1, x2, y2;
      int bkColor;

      /*
        Save the old settings
      */
      grGetViewPort(&x1, &y1, &x2, &y2);
      bkColor = (int) grGetBkColor();

      /*
        Fill the viewport with bright white
      */
      grSetBkColor(0x0F);
      grSetViewPort(rSave.left, rSave.top, rSave.right, rSave.bottom);
      grClearViewPort();

      /*
        Restore the old settings
      */
      grSetViewPort(x1, y1, x2, y2);
      grSetBkColor(bkColor);
#endif

      gxPutImage(&_SavedImage[_SavedImageSP].gxHeader,
                 gxXOR,
                 rSave.left, rSave.top, 0);
    }
    else
      gxVirtualDisplay(&_SavedImage[_SavedImageSP].gxHeader, 0,0,
                       rSave.left, rSave.top, rSave.right, rSave.bottom, 0);

    gxDestroyVirtual(&_SavedImage[_SavedImageSP].gxHeader);
  }


#elif defined(MSC) || defined(BGI) || defined(META) || defined(GURU)
#if defined(META)
  mwSetPort(&_MetaInfo.MyPort);
  mwRasterOp(zREPz);
#endif

  if (_SavedImage[_SavedImageSP].hpBuf[0] != NULL)
  {
    if (_SavedImage[_SavedImageSP].ulSize == 0xFFFF)
      y = x = 0;
    else
    {
      x = rSave.left;
      y = rSave.top;
    }

    for (i = 0;  i < 4;  i++)
      if (_SavedImage[_SavedImageSP].hpBuf[i] != NULL)
      {
        if ((lpMem = GlobalLock(_SavedImage[_SavedImageSP].hpBuf[i])) != NULL)
        {
#if defined(META)
          mwWriteImage((rect *) &_SavedImage[_SavedImageSP].rectImage, lpMem);
#else
          _putimage(x, y, lpMem, 
            (SysGDIInfo.fFlags & GDISTATE_INVERTINGRECT) ? _GPRESET : _GPSET);
#endif
          GlobalUnlock(_SavedImage[_SavedImageSP].hpBuf[i]);
        }
        GlobalFree(_SavedImage[_SavedImageSP].hpBuf[i]);
        _SavedImage[_SavedImageSP].hpBuf[i] = NULL;
        y += _SavedImage[_SavedImageSP].yIncrement;
      }
  }

#endif

  _SavedImageSP--;
  MouseShow();
  if (bCaretPushed)
    CaretPop();
}


/*
  Special GrLPtoSP routine for graphics mode
*/
BOOL FAR PASCAL GrLPtoSP(hDC, lpPt, nCount)
  HDC     hDC;
  LPPOINT lpPt;
  int     nCount;
{
  LPHDC lphDC;
  RECT  rClipping;
  MWCOORD xScreenOrg, yScreenOrg;
  MWCOORD x, y;


  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  /*
    We subtract the clipping rectangle from the calculations because
    all of the low-level graphics functions are relative to the
    viewport. The viewport is set by the DC's clipping rect.
  */
  rClipping = lphDC->rClipping;

  /*
    After we compute the logical-to-device coordinates for each point,
    we must add a value to get to the screen coordinates.

    The formulas come from Petzold pages 521-522
  */
#if defined(MOTIF)
  /* no viewports in MOTIF, xo don't make cords viewport-relative */
  xScreenOrg = yScreenOrg = 0;
#else
  xScreenOrg = (MWCOORD) (lphDC->ptOrg.x - rClipping.left);
  yScreenOrg = (MWCOORD) (lphDC->ptOrg.y - rClipping.top);
#endif

  while (nCount-- > 0)
  {
    if (lphDC->wMappingMode == MM_TEXT)
    {
      x = lpPt->x - lphDC->ptWindowOrg.x + lphDC->ptViewOrg.x;
      y = lpPt->y - lphDC->ptWindowOrg.y + lphDC->ptViewOrg.y;
    }
    else
    {
      x = (MWCOORD) ( ((long)lpPt->x - (long)lphDC->ptWindowOrg.x) * 
                     (long)lphDC->extView.cx / (long)lphDC->extWindow.cx +
                     (long)lphDC->ptViewOrg.x);
      y = (MWCOORD) ( ((long)lpPt->y - (long)lphDC->ptWindowOrg.y) * 
                     (long)lphDC->extView.cy / (long)lphDC->extWindow.cy +
                     (long)lphDC->ptViewOrg.y);
    }

    /*
      Device-to-screen
    */
    lpPt->x = x + xScreenOrg;
    lpPt->y = y + yScreenOrg;

    /*
     Advance to the next point
    */
    lpPt++;
  }

  return TRUE;
}



/****************************************************************************/
/*                                                                          */
/* Function : BitmapBitsToLineWidth()                                       */
/*                                                                          */
/* Purpose  : Utility routine needed by all of the various bitmap functions.*/
/*            Given the number of bits per pixel and pixel width of the     */
/*            bitmap, returns the number of bytes for a scanline and the    */
/*            number of colors.                                             */
/*                                                                          */
/* Returns  : The number of bytes in a scanline, rounded to the nearest     */
/*            LONG.                                                         */
/*            Returns -1 if the number of bits-per-pixel is not supported.  */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL BitmapBitsToLineWidth(wBitsPerPixel, wWidth, piColor)
  UINT wBitsPerPixel;
  UINT wWidth;
  INT  *piColor;
{
  INT  nColors, nBytesPerLine;

  nColors = 1 << wBitsPerPixel;

  switch (wBitsPerPixel)
  {
    case 1 : /* 1 byte = 8 pixels */
      nBytesPerLine = (int) ((wWidth+7) >> 3);
      break;
    case 4 : /* 1 byte = 2 pixels */
      nBytesPerLine = (int) ((wWidth+1) >> 1);
      break;
    case 8 : /* 1 byte = 1 pixels */
      nBytesPerLine = (int) (wWidth);
      break;
    case 16 :
      nBytesPerLine = (int) (wWidth * 2);
      break;
    case 24 :
      nBytesPerLine = (int) (wWidth * 3);
      /*
        If we have a 24-bit color bitmap, then there is no palette.
      */
      nColors = 0;
      break;
    default : /* not supported */
      return -1;
  }

  /*
    Round to nearest sizeof(LONG)
  */
#if defined(META) || defined(METABGI)
  if (wBitsPerPixel == 4 || wBitsPerPixel == 8)
    nBytesPerLine = (nBytesPerLine + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1);
#elif defined(GX)
  if (wBitsPerPixel == 4)
    nBytesPerLine = (int) gxVirtualSize(gxGetDisplay(), (int) wWidth, 1);
#else
  if (wBitsPerPixel != 1)
    nBytesPerLine = (nBytesPerLine + (sizeof(LONG)-1)) & ~(sizeof(LONG)-1);
#endif

#if defined(METABGI)
  nBytesPerLine = imagesize(0, 0, wWidth-1, 0);
  nBytesPerLine -= sizeof(IMGHEADER);
#endif

  if (piColor)
    *piColor = nColors;
  return nBytesPerLine;
}


#if defined(META)
#define GRQtrimMenu 500
#include "metquery.c"
#endif


static BOOL bMouseCondHidden = FALSE;

VOID FAR PASCAL MOUSE_ConditionalOffDC(LPHDC lphDC, 
                                       int x1, int y1, int x2, int y2)
{
#if !defined(XWINDOWS)
  int  tmp;
  int  mx, my, mb;
  RECT rMouse, r;


  /*
    We do not need to hide the mouse if we have a memory DC.
  */
  if (lphDC && IS_MEMDC(lphDC))
    return;

  /*
    Sort the logical points
  */
  if (x1 > x2)
  {
    tmp = x1;
    x1  = x2;
    x2  = tmp;
  }
  if (y1 > y2)
  {
    tmp = y1;
    y1  = y2;
    y2  = tmp;
  }

  /*
    Get the absolute screen coordinates of the area we are drawing
    If lphDC is NULL, then the coordinates which were passed were
    absolute screen coordinates.
  */
  if (lphDC)
  {
    x1 += lphDC->rClipping.left;
    x2 += lphDC->rClipping.left;
    y1 += lphDC->rClipping.top;
    y2 += lphDC->rClipping.top;
  }

#if 0
  MOUSE_ConditionalOff(x1, y1, x2, y2);

#else

  /*
    All of this is done because there seems to be a bug in ConditionOff
    where the cursor does not get redisplayed when the MOUSE_SowCursor
    is called. So, kludge this by getting the mouse position, seeing
    if the mouse can possibly intersect the drawing area, and hiding
    the mouse.
  */
  MOUSE_GetStatus(&mb, (MWCOORD *) &mx, (MWCOORD *) &my);

  /*
    Frame the mouse in a rectangle 16 pixels around the hotspot. This
    will take care of cases where the hotspot is in the middle of
    the cursor.
  */
  SetRect(&rMouse, mx-32, my-32, mx+33, my+33);
  SetRect(&r, x1, y1, x2+1, y2+1);  /* add 1 so IntersectRect works on lines */

  /*
    If the mouse intersects the drawing rectangle, hide it.
  */
  if ((bMouseCondHidden = IntersectRect(&r, &r, &rMouse)) == TRUE)
  {
#if 0
    MouseHide();
#else
    MOUSE_HideCursor();
#endif
  }
#endif
#endif
}

VOID FAR PASCAL MOUSE_ShowCursorDC()
{
#if !defined(XWINDOWS)
  if (bMouseCondHidden)
  {
    bMouseCondHidden = FALSE;
#if 0
    MouseShow();
#else
    MOUSE_ShowCursor();
#endif
  }
#endif
}


VOID PASCAL MEWELSortPoints(px1, py1, px2, py2)
  MWCOORD *px1, *py1, *px2, *py2;
{
  MWCOORD t;

  if (*px1 > *px2)
  {
    t = *px1;  *px1 = *px2;  *px2 = t;
  }
  if (*py1 > *py2)
  {
    t = *py1;  *py1 = *py2;  *py2 = t;
  }
}


#ifdef GURU
int GetPixelX(int x, int y)
{
  return 0;
}
#endif

