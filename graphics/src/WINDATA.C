/*===========================================================================*/
/*                                                                           */
/* File    : WINDATA.C                                                       */
/*                                                                           */
/* Purpose : Holds some of the global data used by MEWEL                     */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/*
  Structure which records the internal system values.
*/
WINSYSPARAMS InternalSysParams =
{
  NULLHWND,       /* hWndActive   */
  NULLHWND,       /* hWndSysModal */
  NULLHWND,       /* hWndCaret    */
  NULLHWND,       /* hWndCapture  */
  NULLHWND,       /* hWndFocus    */
  FALSE,          /* bVGAFontsDone */
  "MEWEL Window System v4.00 (C) Copyright 1989-1993  Magma Systems",/*pszMEWELcopyright */
  0,              /* hCursor    */
  FALSE,          /* bInMsgBox  */
  FALSE,          /* fOldBrkFlag*/

  0,              /* wWindowsCompatibilityState */
  0L,             /* fProgramState */

  NULL,           /* WindowList */
  NULL,           /* wDesktop   */

  0,              /* nTotalHooks */
  { 0,0,0,0,0,0,0,0,0,0, }, /* lpfnHookProc */
};


INT FAR CDECL MEWELSysMetrics[SM_CMETRICS] =
{
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  /* SM_CXSCREEN          */  80, /* set during VidGetMode() */
  /* SM_CYSCREEN          */  25, /* set during VidGetMode() */
  /* SM_CXVSCROLL         */  17, /* width of arrow bitmap on vert sb */
  /* SM_CYHSCROLL         */  17, /* height of arrow bitmap on horz sb */
  /* SM_CYCAPTION         */  20, /* height of caption bar */
  /* SM_CXBORDER          */  1,  /* width of frame */
  /* SM_CYBORDER          */  1,  /* height of frame */
  /* SM_CXDLGFRAME        */  4,  /* width of frame for WS_DLGFRAME */
  /* SM_CYDLGFRAME        */  4,  /* height of frame for WS_DLGFRAME */
  /* SM_CYVTHUMB          */  17, /* height of thumb on vert sb */
  /* SM_CXHTHUMB          */  17, /* width of thumb on horz sb */
  /* SM_CXICON            */  32, /* width of icon */
  /* SM_CYICON            */  32, /* height of icon */
  /* SM_CXCURSOR          */  16, /* width of cursor */
  /* SM_CYCURSOR          */  16, /* height of cursor */
  /* SM_CYMENU            */  18, /* height of menubar */
  /* SM_CXFULLSCREEN      */  80, /* width of client area of zoomed window */
  /* SM_CYFULLSCREEN      */  24, /* height of client area of zoomed window */
  /* SM_CYKANJIWINDOW     */  0,  /* height of Kanji window */
  /* SM_MOUSEPRESENT      */  0,  /* set during MouseInit() */
  /* SM_CYVSCROLL         */  17, /* height of arrow bitmap of vert sb */
  /* SM_CXHSCROLL         */  17, /* width of arrow bitmap of horz sb */
#if defined(DEBUG)
  /* SM_DEBUG             */  1,  /* are we in the debugging version? */
#else
  /* SM_DEBUG             */  0,  /* are we in the debugging version? */
#endif
  /* SM_SWAPBUTTON        */  0,  /* are the mouse buttons swapped? */
  /* SM_RESERVED1         */  0,  /* unknown 1 */
  /* SM_RESERVED2         */  0,  /* unknown 2 */
  /* SM_RESERVED3         */  0,  /* unknown 3 */
  /* SM_RESERVED4         */  0,  /* unknown 4 */
  /* SM_CXMIN             */  102,/* min width of window */
  /* SM_CYMIN             */  26, /* min height of window */
  /* SM_CXSIZE            */  18, /* width of bitmap on caption */
  /* SM_CYSIZE            */  18, /* height of bitmap on caption */
  /* SM_CXFRAME           */  4,  /* width of thickframe */
  /* SM_CYFRAME           */  4,  /* height of thickframe */
  /* SM_CXMINTRACK        */  102,/* min tracking width of window */
  /* SM_CYMINTRACK        */  26, /* min tracking height of window */

  /* Win 3.1 specific variables */
  /* SM_CXDOUBLECLK       */  1,  /* width of bounding rect for dbl-click */
  /* SM_CYDOUBLECLK       */  1,  /* height of bounding rect for dbl-click */
  /* SM_CXICONSPACING     */  72, /* rect for calculating tiled icon spacing */
  /* SM_CYICONSPACING     */  72, /* rect for calculating tiled icon spacing */
  /* SM_MENUDROPALIGNMENT */  0,  /* 0 if popups are left-aligned under items */
  /* SM_PENWINDOWS        */  0,  /* Is PenWindows enabled? */
  /* SM_DBCSENABLED       */  0,  /* Do we have double-byte chars? */

  /* MEWEL-specific variables */
  /* SM_CXCHECKBOX        */  16, /* width of checkbox bitmap */
  /* SM_CYCHECKBOX        */  16, /* height of checkbox bitmap */

#else
  /* SM_CXSCREEN          */  80, /* set during VidGetMode() */
  /* SM_CYSCREEN          */  25, /* set during VidGetMode() */
  /* SM_CXVSCROLL         */  1,  /* width of arrow bitmap on vert sb */
  /* SM_CYHSCROLL         */  1,  /* height of arrow bitmap on horz sb */
  /* SM_CYCAPTION         */  1,  /* height of caption bar */
  /* SM_CXBORDER          */  1,  /* width of frame */
  /* SM_CYBORDER          */  1,  /* height of frame */
  /* SM_CXDLGFRAME        */  1,  /* width of frame for WS_DLGFRAME */
  /* SM_CYDLGFRAME        */  1,  /* height of frame for WS_DLGFRAME */
  /* SM_CYVTHUMB          */  1,  /* height of thumb on vert sb */
  /* SM_CXHTHUMB          */  1,  /* width of thumb on horz sb */
  /* SM_CXICON            */  8,  /* width of icon */
  /* SM_CYICON            */  4,  /* height of icon */
  /* SM_CXCURSOR          */  1,  /* width of cursor */
  /* SM_CYCURSOR          */  1,  /* height of cursor */
  /* SM_CYMENU            */  1,  /* height of menubar */
  /* SM_CXFULLSCREEN      */  80, /* width of client area of zoomed window */
  /* SM_CYFULLSCREEN      */  24, /* height of client area of zoomed window */
  /* SM_CYKANJIWINDOW     */  0,  /* height of Kanji window */
  /* SM_MOUSEPRESENT      */  0,  /* set during MouseInit() */
  /* SM_CYVSCROLL         */  1,  /* height of arrow bitmap of vert sb */
  /* SM_CXHSCROLL         */  1,  /* width of arrow bitmap of horz sb */
  /* SM_DEBUG             */  0,  /* are we in the debugging version? */
  /* SM_SWAPBUTTON        */  0,  /* are the mouse buttons swapped? */
  /* SM_RESERVED1         */  0,  /* unknown 1 */
  /* SM_RESERVED2         */  0,  /* unknown 2 */
  /* SM_RESERVED3         */  0,  /* unknown 3 */
  /* SM_RESERVED4         */  0,  /* unknown 4 */
  /* SM_CXMIN             */  2,  /* min width of window */
  /* SM_CYMIN             */  2,  /* min height of window */
  /* SM_CXSIZE            */  1,  /* width of bitmap on caption */
  /* SM_CYSIZE            */  1,  /* height of bitmap on caption */
  /* SM_CXFRAME           */  1,  /* width of thickframe */
  /* SM_CYFRAME           */  1,  /* height of thickframe */
  /* SM_CXMINTRACK        */  0,  /* min tracking width of window */
  /* SM_CYMINTRACK        */  0,  /* min tracking height of window */

  /* Win 3.1 specific variables */
  /* SM_CXDOUBLECLK       */  1,  /* width of bounding rect for dbl-click */
  /* SM_CYDOUBLECLK       */  1,  /* height of bounding rect for dbl-click */
  /* SM_CXICONSPACING     */ 16,  /* rect for calculating tiled icon spacing */
  /* SM_CYICONSPACING     */  4,  /* rect for calculating tiled icon spacing */
  /* SM_MENUDROPALIGNMENT */  0,  /* 0 if popups are left-aligned under items */
  /* SM_PENWINDOWS        */  0,  /* Is PenWindows enabled? */
  /* SM_DBCSENABLED       */  0,  /* Do we have double-byte chars? */

  /* MEWEL-specific variables */
  /* SM_CXCHECKBOX        */  3,  /* width of checkbox bitmap */
  /* SM_CYCHECKBOX        */  1,  /* height of checkbox bitmap */
#endif
};

/*
  This is the structure which contains a lot of info which the GDI
  layer needs. It's initially set up for text mode, and will be
  altered in the GUI layer by WinOpenGraphics and some of the font routines.
*/
GDIINFO SysGDIInfo =
{
  FALSE,  /* bGraphicsSystemInitialized */
  FALSE,  /* bWasBlinkingEnabled        */

  3,      /* iVideoMode */
  80,     /* cxScreen   */
  25,     /* cyScreen   */
  16,     /* nColors    */
  4,      /* nBitsPerPixel */

#if defined(MEWEL_GUI)
  8,      /* tmHeight          */
  8,      /* tmExternalLeading */
 16,      /* tmHeightAndSpace  */
  8,      /* tmAveCharWidth    */
#else
  1,      /* tmHeight          */
  0,      /* tmExternalLeading */
  1,      /* tmHeightAndSpace  */
  1,      /* tmAveCharWidth    */
#endif

  3,      /* iSavedVideoMode   */
  -1,     /* StartingVideoMode */
  0,      /* StartingVideoLength */

  0,      /* hFontDefault      */
  NULL,   /* hDefMonoBitmap    */
  { 0, 0, 0, 0   },  /* rLastClipping */
  { 0, 0, 80, 25 },  /* rectScreen    */

  NULL, NULL, NULL,  /* printer hooks */
  (HPALETTE) 0,      /* hSysPalette */
  -1,                /* CurrFontID  */

  0L,                /* fFlags */

  NULL,              /* lpFontWidthCache */
  (HFONT) -1,        /* hFontCached      */
};


/*
  Strings used in MEWEL
*/
LPSTR FARDATA SysStrings[] =
{
  "~Restore",              /* SYSSTR_RESTORE  0 */
  "~Move",                 /* SYSSTR_MOVE     1 */
  "~Size",                 /* SYSSTR_SIZE     2 */
  "Mi~nimize",             /* SYSSTR_MINIMIZE 3 */
  "Ma~ximize",             /* SYSSTR_MAXIMIZE 4 */
  "~Close   ALT+F4",       /* SYSSTR_CLOSE    5 */
  "~Close   CTRL+F4",      /* SYSSTR_CLOSEMDI 6 */

  "  ~OK  ",               /* SYSSTR_OK       7 */
  "~CANCEL",               /* SYSSTR_CANCEL   8 */
  "~ABORT",                /* SYSSTR_ABORT    9 */
  "~RETRY",                /* SYSSTR_RETRY   10 */
  "~IGNORE",               /* SYSSTR_IGNORE  11 */
  "  ~YES ",               /* SYSSTR_YES     12 */
  "  ~NO  ",               /* SYSSTR_NO      13 */
  " ~HELP ",               /* SYSSTR_HELP    14 */

  "\007\007Out of Memory!!!\n",  /* SYSSTR_OUTOFMEMORY  15 */
  "Error",                 /* SYSSTR_ERROR   16 */
};


#ifdef MEWEL_TEXT
/*
  Cursor definitions for the text mode version of MEWEL. There
  are non-ASCII characters here, so watch when porting to other
  systems.

  Note: JPI TopSpeed C freaks at these characters
*/
BYTE FARDATA achSysCursor[] =
{
  NORMAL_CURSOR,         /* ' '          */
  29, /* '', */         /* HTLEFT       */
  29, /* '', */         /* HTRIGHT      */
  18, /* '', */         /* HTTOP        */
  4,  /* '', */         /* HTTOPLEFT    */
  4,  /* '', */         /* HTTOPRIGHT   */
  18, /* '', */         /* HTBOTTOM     */
  4,  /* '', */         /* HTBOTTOMLEFT */
  4,  /* '', */         /* HTBOTTOMRIGHT*/
};
#endif



/*
  This can be set to FALSE if you do not want the MEWEL app to restore
  the starting directory upon program termination.
*/
BOOL   bRestoreDirectory = TRUE;


/*
  We need a flag to signal the clipping routines whether we should allow
  writing in w->rect as opposed to w->client.
*/
BOOL   bDrawingBorder = FALSE;

/*
  Visibility map for text mode. This map has one UINT for each cell
  on the screen. Each element contains the window handle of the
  window which is visible in that cell.
*/
UINT FAR *WinVisMap = NULL;

#if !defined(MEWEL_TEXT)
BOOL bVirtScreenEnabled = FALSE;
#endif

/*
  Important GUI specific variables
*/
#if defined(MEWEL_GUI) || defined(MOTIF)
/*
  The initial graphics mode. The app can set this before WinInit() in
  order to control the video mode which the GUI starts up in. The
  value -1 means to default to the maximum resolution.
*/
INT    MEWELInitialGraphicsMode = -1;
#endif

/*
  Handle to the open MEWEL rsource file
*/
INT    MewelCurrOpenResourceFile = -1;

/*
  Important selectors for the extended-DOS version
*/
#if defined(EXTENDED_DOS)
WORD   selBiosSeg = 0;
WORD   selZeroSeg = 0;
WORD   selA000Seg = 0;
WORD   selCharGenerator = 0;
#endif

/*
  Handle to the desktop window
*/
HWND _HwndDesktop = NULLHWND;


VOID FAR PASCAL SET_PROGRAM_STATE(DWORD f)
{
  InternalSysParams.fProgramState |= f;
}
VOID FAR PASCAL CLR_PROGRAM_STATE(DWORD f)
{
  InternalSysParams.fProgramState &= ~f;
}
BOOL FAR PASCAL TEST_PROGRAM_STATE(DWORD f)
{
  return (BOOL) ((InternalSysParams.fProgramState & f) != 0);
}

VOID FAR PASCAL SetWindowsCompatibility(UINT f)
{
  InternalSysParams.wWindowsCompatibilityState |= f;
}
UINT FAR PASCAL TestWindowsCompatibility(UINT f)
{
  return (InternalSysParams.wWindowsCompatibilityState & f);
}

int WINAPI GetSystemMetrics(int n)
{
  return (n >= 0 && n < SM_CMETRICS) ? MEWELSysMetrics[n] : 0;
}

#if !defined(__DPMI16__) && !defined(__DPMI32__)
DWORD WINAPI GetVersion(void)
{
  return MEWEL_VERSION;
}

DWORD WINAPI GetWinFlags(void)
{
  return 0L;
}
#endif

