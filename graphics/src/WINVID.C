/*===========================================================================*/
/*                                                                           */
/* File    : WINVID.C                                                        */
/*                                                                           */
/* Purpose : Low-level video routines for the IBM PC running MS-DOS.         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/*#define NOGDI*/
#define NOKERNEL
#define INCLUDE_MOUSE

#include "wprivate.h"
#include "window.h"

#if defined(__DPMI32__)
#include "winevent.h"
#endif

#if defined(MSC) || defined(__WATCOMC__) || defined(PLTNT)
#include <conio.h>  /* for inp() */
#if defined(PLTNT) && !defined(__WATCOMC__) && !defined(__BORLANDC__)
#define inp   _inp
#define outp  _outp
#endif
#endif
#if defined(__GNUC__)
#include <pc.h>
#define inp   inportb
#define outp  outportb
#endif

#if defined(__TURBOC__)
#pragma warn -par
#endif

/*
  Special selectors used by the Pharlap DOS extender
*/
#if defined(__DPMI16__) || defined(__DPMI32__)
typedef USHORT SEL;
#endif

#if defined(DOS286X) || defined(ERGO) || defined(__DPMI16__) || defined(__DPMI32__)
extern SEL selBiosSeg;
extern SEL selZeroSeg;
extern SEL selA000Seg;
extern SEL selCharGenerator;
#if defined(__DPMI16__) || defined(__DPMI32__)
static SEL PASCAL ParaToSel(WORD sel);
#endif
#endif


#ifdef WAGNER_GRAPHICS
VOID FAR PASCAL VidGetGraphMode(void);
#endif

static WORD PASCAL DesqviewGetVideoBuffer(unsigned);
static VOID PASCAL VidSetGraphicsCursor(void);
static VOID PASCAL VidDoCursorBits(BYTE achPixels[MAXCURSORHEIGHT][MAXCURSORWIDTH], int xPixel, int yPixel, BOOL bSave);
static VOID PASCAL VidQueryDimensions(void);
extern INT FAR PASCAL _VidDevCaps(HDC, INT);

#define MAX_FONT_WIDTH  8


/****************************************************************************/
/*                                                                          */
/* Function : Static table describing resolution and colors for the various */
/*            video modes.                                                  */
/*                                                                          */
/****************************************************************************/

typedef struct tagVidResInfo
{
  WORD  iMode;
  int   iHorzRes;  /* negative number means get info from VideoInfo.width  */
  int   iVertRes;  /* negative number means get info from VideoInfo.length */
  WORD  nColors;
  WORD  nBitsPerPixel;
} VIDRESINFO, *PVIDRESINFO;

static VIDRESINFO VidResInfo[] =
{
   0x00,  -40, -25,  16, 4,    /* mode 0 */
   0x01,  -40, -25,  16, 4,    /* mode 1 */
   0x02,  -80, -25,  16, 4,    /* mode 2 */
   0x03,  -80, -25,  16, 4,    /* mode 3 */
   0x04,  320, 200,   4, 2,    /* mode 4 */
   0x05,  320, 200,   4, 2,    /* mode 5 */
   0x06,  640, 200,   2, 1,    /* mode 6 */
   0x07,  -80, -25,   2, 1,    /* mode 7 */
   0x08,  160, 200,  16, 4,    /* mode 8 */
   0x09,  320, 200,  16, 4,    /* mode 9 */
   0x0A,  640, 200,   4, 2,    /* mode A */
   0x0D,  320, 200,  16, 4,    /* mode D */
   0x0E,  640, 200,  16, 4,    /* mode E */
   0x0F,  640, 350,   2, 1,    /* mode F */
   0x10,  640, 350,  16, 4,    /* mode 10*/
   0x11,  640, 480,   2, 1,    /* mode 11*/
   0x12,  640, 480,  16, 4,    /* mode 12*/
   0x13,  320, 200, 256, 16,   /* mode 13*/
   0x61,  640, 400, 256, 16,   /* mode 61 - ATI VGA */
   0x62,  640, 480, 256, 16,   /* mode 62 - ATI VGA */
};


/*
  Important video declarations
*/
BOOL  bForceMono     = FALSE;
BYTE  HilitePrefix   = HILITE_PREFIX;
COLOR HilitePrefixAttr = 0;     /* Color attr of hot-key */
WORD  Scr_Base       = 0xB800;  /* Segment of video memory */
WORD  ScrBaseOrig    = 0xB800;  /* Same as above before Desqview */
unsigned MouseHandle = 0;       /* Handle of mouse returned by MouseInit */

struct videoinfo VideoInfo =
{
   25,
   80,
   0xB800,
   0,
   0x0000
};

CURSORINFO CursorInfo =
{
  0, 0,  /* row, col */
  0,     /* startscan */
#ifdef MEWEL_GUI
  1,     /* endscan */
#else
  0,     /* endscan */
#endif
  0,     /* nWidth */
  0      /* fState */
};


/****************************************************************************/
/*                                                                          */
/* Function : VidSetVideoMode, _VidInitNewVideoMode, ToggleEGA              */
/*                                                                          */
/* Purpose  : Routines which manipulate the current video mode.             */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

void FAR PASCAL VidSetVideoMode(int iMode)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
  r.AX = iMode;
  RealIntr(0x10, &r);
#elif defined(__DPMI16__)
  _AH = 0x00;
  _AL = (char) iMode;
  geninterrupt(0x10);
#else
  union REGS r;
  r.h.ah = 0x00;
  r.h.al = (char) iMode;
  INT86(0x10, &r, &r);
#endif
  _VidInitNewVideoMode();
}


void FAR PASCAL _VidInitNewVideoMode(void)
{
#ifdef WAGNER_GRAPHICS
  static BOOL sav_use_virt = TRUE;
#endif

  MouseTerminate();
#ifdef DOS286X
  DosFreeSeg(Scr_Base);
  DosFreeSeg(selZeroSeg);
  DosFreeSeg(selA000Seg);
  DosFreeSeg(selCharGenerator);
#endif

#ifdef WAGNER_GRAPHICS
  if (bGraphicsSystemInitialized)
  {
    VidGetGraphMode();
    sav_use_virt = bUseVirtualScreen;
    bUseVirtualScreen = FALSE;
    VirtualScreenInit();
    WinInitVisMap();
    if (InternalSysParams.WindowList)
      WinUpdateVisMap();
  }
  else
  {
    VidGetMode();
    bUseVirtualScreen = sav_use_virt;
  }
#else
  VidGetMode();
#endif

  VidSetBlinking(TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) ? FALSE : TRUE);
  VidHideCursor();
  SendMessage(0xFFFF, WM_DEVMODECHANGE, 0, (DWORD) (LPSTR) "SCREEN");
  SendMessage(_HwndDesktop, WM_DEVMODECHANGE, 0, (DWORD) (LPSTR) "SCREEN");
  VidClearScreen(WinQuerySysColor(NULLHWND, SYSCLR_BACKGROUND));
#ifndef WAGNER_SPECIAL
  WinDrawAllWindows();
#endif
  MouseInitialize();
}


int FAR PASCAL ToggleEGA(void)
{
#if !defined(MEWEL_GUI)

#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

  FARADDR lpCursorEmu;
  BOOL    bUseVGAFonts = InternalSysParams.bVGAFontsDone;

  if (!IsEGA())
    return FALSE;

  VidRestoreVGAFonts();
  MouseHide();

  lpCursorEmu = GetBIOSAddress(0x87);
  if (VideoInfo.length > 25)
  {
#if defined(__DPMI32__) && !defined(PLTNT)
   r.AX = 2;
   RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AX = 2;
    geninterrupt(0x10);
#else
    r.x.ax = 0x0002;
    INT86(0x10, &r, &r);
#endif

    *lpCursorEmu &= ~1;

#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = VideoInfo.starting_mode;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AX = VideoInfo.starting_mode;
    geninterrupt(0x10);
#else
    r.x.ax = VideoInfo.starting_mode;
    INT86(0x10, &r, &r);
#endif
    VidSetCursorMode(0, 0);
  }
  else
  {
#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = 0x1112;
    r.BX = 0;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AX = 0x1112;
    _BL = 0;
    geninterrupt(0x10);
#else
    r.x.ax = 0x1112;
    r.h.bl = 0x00;
    INT86(0x10, &r, &r);
#endif

    *lpCursorEmu |= 1;

#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = 0x0100;
    r.BX = 0;
    r.CX = 0x0600;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AH = 1;
    _BH = 0;
    _CX = 0x0600;
    geninterrupt(0x10);
#else
    r.h.ah = 1;
    r.h.bh = 0;
    r.x.cx = 0x0600;
    INT86(0x10, &r, &r);
#endif
  }

  _VidInitNewVideoMode();
  if (bUseVGAFonts)
    VidInitVGAFonts();
#endif
  return 1;
}


BOOL FAR PASCAL VidSetBlinking(BOOL fBlink)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

#if defined(WAGNER_GRAPHICS)
  return FALSE;
#else

  if ((VideoInfo.flags & 0x07) != MDA)
    if (!IsEGA())
      return FALSE;

  /*
    Force no blinking in graphics mode, or else the mouse cursor will
    blink.
  */
  if (VID_IN_GRAPHICS_MODE() && !TEST_PROGRAM_STATE(STATE_EXITING))
    fBlink = FALSE;

#if defined(__DPMI32__) && !defined(PLTNT)
  r.AX   = 0x1003;
#else
  r.x.ax = 0x1003;
#endif

  if (fBlink)
  {
    CLR_PROGRAM_STATE(STATE_EXTENDED_ATTRS);
#if defined(__DPMI32__) && !defined(PLTNT)
    r.BX = 1;
#else
    r.h.bl = 1;
#endif
  }
  else
  {
    SET_PROGRAM_STATE(STATE_EXTENDED_ATTRS);
#if defined(__DPMI32__) && !defined(PLTNT)
    r.BX = 0;
#else
    r.h.bl = 0;
#endif
  }

  if ((VideoInfo.flags & 0x07) != MDA)
  {
#if defined(__DPMI32__) && !defined(PLTNT)
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AX = r.x.ax;
    _BL = r.h.bl;
    geninterrupt(0x10);
#else
    INT86(0x10, &r, &r);
#endif
  }
  else if (!VID_IN_GRAPHICS_MODE())
  {
#if !defined(__DPMI32__)
    outp(0x3b8, 0x19 + (r.h.bl << 5));
#endif
  }

  return TRUE;
#endif
}


int FAR PASCAL _VidDevCaps(hDC, nIndex)
  HDC hDC;
  int nIndex;
{
  (void) hDC;

  switch (nIndex)
  {
#ifdef MEWEL_GUI
    case HORZRES   :
      return SysGDIInfo.cxScreen;
    case VERTRES   :
      return SysGDIInfo.cyScreen;
    case NUMCOLORS :
      return SysGDIInfo.nColors;
    case BITSPIXEL :
      return SysGDIInfo.nBitsPerPixel;
#else
    case HORZRES   :
    case VERTRES   :
    case NUMCOLORS :
    case BITSPIXEL :
    {
      int  i;
      PVIDRESINFO pi;

      pi = VidResInfo;

      for (i = 0;  i < sizeof(VidResInfo) / sizeof(VidResInfo[0]);  i++, pi++)
      {
        if (pi->iMode == VideoInfo.starting_mode)
        {
          switch (nIndex)
          {
            case HORZRES   :
              return (pi->iHorzRes < 0) ? VideoInfo.width  : pi->iHorzRes;
            case VERTRES   :
              return (pi->iVertRes < 0) ? VideoInfo.length : pi->iVertRes;
            case NUMCOLORS :
              return VID_IN_MONO_MODE() ? 2 : pi->nColors;
            case BITSPIXEL :
              return VID_IN_MONO_MODE() ? 1 : pi->nBitsPerPixel;
          }
        }
      }
      return 0;
    }
#endif

    case HORZSIZE   :
      return SysGDIInfo.cxScreenMM;

    case VERTSIZE   :
      return SysGDIInfo.cyScreenMM;

    case LOGPIXELSX :
      return SysGDIInfo.cxLogPixel;

    case LOGPIXELSY :
      return SysGDIInfo.cyLogPixel;

#ifdef MEWEL_GUI
    case ASPECTX    :
    case ASPECTY    :
      return 36;
    case ASPECTXY   :
      return 51;
#else
    case ASPECTX    :
    case ASPECTY    :
    case ASPECTXY   :
      return 1;
#endif

    case TECHNOLOGY :
      return 1;  /* DT_RASDISPLAY */

    case NUMFONTS   :
      return 1;

    case PLANES     :
      return 1;

    case RASTERCAPS :
#ifdef MEWEL_GUI
    {
      int fFlags = RC_BITBLT;
      if (IsVGA() && SysGDIInfo.nColors >= 256)
        fFlags |= RC_PALETTE;
      return fFlags;
    }
#else
      return 0;
#endif

    case NUMRESERVED :
      return 20;
    case SIZEPALETTE :
      return _VidDevCaps(hDC, NUMCOLORS);

    default :
      return 0;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : VidClearScreen(attr)                                          */
/*                                                                          */
/* Purpose  : Clears the entire screen to a specified attribute.            */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VidClearScreen(attr)
  COLOR attr;
{
#ifdef MEWEL_GUI
  (void) attr;

#else

#ifdef WAGNER_GRAPHICS
  if (VID_IN_GRAPHICS_MODE())
  {
    gclearscreen();
    return;
  }
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
  r.AX   = 0x0600;      /* scroll up, 0 means clear whole screen */
#else
  r.x.ax = 0x0600;      /* scroll up, 0 means clear whole screen */
#endif

  /*
    Set scroll mode fill character
  */
  if (VID_IN_GRAPHICS_MODE())
#if defined(__DPMI32__) && !defined(PLTNT)
    r.BX   = 0;	        /* blanks are 00h in graphics mode */
#else
    r.h.bh = 0;	        /* blanks are 00h in graphics mode */
#endif
  else
#if defined(__DPMI32__) && !defined(PLTNT)
    r.BX   = ((BYTE) attr) << 8;
#else
    r.h.bh = (BYTE) attr;
#endif

  /*
    Specify the rectangle to scroll. In graphics mode on certain systems,
    we cannot use the pixel-based VideoInfo.length and width, or else that
    system can hang. So just use [24,79]
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  r.CX   = 0x0000;      /* upper left = [0,0] */
#else
  r.x.cx = 0x0000;      /* upper left = [0,0] */
#endif

  if (VID_IN_GRAPHICS_MODE())
#if defined(__DPMI32__) && !defined(PLTNT)
    r.DX   = 0x184F;
#else
    r.x.dx = 0x184F;
#endif
  else
#if defined(__DPMI32__) && !defined(PLTNT)
    r.DX   = ((VideoInfo.length-1) << 8) | (VideoInfo.width - 1);
#else
    r.x.dx = ((VideoInfo.length-1) << 8) | (VideoInfo.width - 1);
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
  RealIntr(0x10, &r);
#elif defined(__DPMI16__)
  _AX = r.x.ax;
  _BX = r.x.bx;
  _CX = r.x.cx;
  _DX = r.x.dx;
  geninterrupt(0x10);
#else
  INT86(0x10, &r, &r);
#endif
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : Vid(Fill)Frame                                                */
/*                                                                          */
/* Purpose  : Draws a frame around a rectangular area                       */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/

static WORD iFramePenStyle = PS_SOLID;

static VOID PASCAL VidPutc(int,int,int,COLOR);


/*
  VidFrame()
    Puts a frame around a rectangular area without regard to clipping.
  The only place where this is called from is WinRubberband().
*/
#ifndef MEWEL_GUI
int FAR PASCAL VidFrame(row1, col1, row2, col2, attr, type)
  int   row1, col1, row2, col2;
  COLOR attr;
  int   type;
{
  register int i;
  register int height = row2 - row1 - 2;  /* not including top/bot */
  register int width  = col2 - col1 - 2;  /* not including left/right */
  
  BYTE     szLineBuf[MAXSCREENWIDTH+1];

  if (height < -1 || width < -1)
    return FALSE;

  if (type >= 6)
    type = 0;

  if (width > 0)
  {
    memset(szLineBuf, SysPenDrawingChars[type][0], width);
    szLineBuf[width] = '\0';
  }
  else
    szLineBuf[0] = '\0';

  /*
    Draw the top line of the frame
  */
  VidPutc(row1,  col1,         SysPenDrawingChars[type][2], attr);
  if (width > 0)
    VidBlastToScreen((HDC) 0, row1, col1+1, 
                     min(col1+width, (int)VideoInfo.width-1), 
                     attr, (LPSTR) szLineBuf);
  VidPutc(row1,  col1+width+1, SysPenDrawingChars[type][4], attr);

  /*
    Draw the sides of the frame
  */
  for (i = height;  i-- > 0;  )
  {
    VidPutc(++row1, col1,         SysPenDrawingChars[type][1], attr);
    VidPutc(row1,   col1+width+1, SysPenDrawingChars[type][1], attr);
  }
  
  /*
    Draw the bottom line of the frame
  */
  if (row2 > row1+1)
  {
    VidPutc(++row1,col1,          SysPenDrawingChars[type][3], attr);
    if (width > 0)
      VidBlastToScreen((HDC) 0, row1, col1+1, 
                       min(col1+width, (int)VideoInfo.width-1), 
                       attr, (LPSTR) szLineBuf);
    VidPutc(row1,  col1+width+1,  SysPenDrawingChars[type][5], attr);
  }
  return TRUE;
}
#endif

/*===========================================================================*/
/*                                                                           */
/* Function: VidPutc()                                                       */
/*                                                                           */
/* Purpose : Blasts a single char/attr to the screen at <row,col>            */
/*                                                                           */
/* The only place where this is called from is VidFrame(), which is called   */
/*  only by WinRubberband().                                                 */
/*                                                                           */
/*===========================================================================*/
#ifndef MEWEL_GUI
static VOID PASCAL VidPutc(row, col, c, attr)
  int row, col;
  int c;
  COLOR attr;
{
  char s[2];
  s[0] = (BYTE) c;  s[1] = '\0';
  VidBlastToScreen((HDC) 0, row, col, min(col, (int) VideoInfo.width-1), 
                   attr, s);
}
#endif

VOID FAR PASCAL WinSetRubberbandingFrame(INT iStyle)
{
  iFramePenStyle = iStyle;
}


/****************************************************************************/
/*                                                                          */
/* Function : VidScroll(x1, y1, x2, y2, n)                                  */
/*  n > 0    roll screen up                                                 */
/*  n < 0    roll screen down                                               */
/*                                                                          */
/* Purpose  : Scrolls the screen using the BIOS functions                   */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VidScroll(int x1, int y1, int x2, int y2, int n, COLOR attr)
{
#if !defined(__DPMI32__)
  union REGS r;

#ifdef WAGNER_GRAPHICS  
  if (VID_IN_GRAPHICS_MODE())
  {
    gscrollrect(x1 * FONT_WIDTH, y1 * VideoInfo.yFontHeight, 
                x2 * FONT_WIDTH, y2 * VideoInfo.yFontHeight, 
                n * VideoInfo.yFontHeight, NULL);
    return;
  }
#endif

  if (n < 0)
  {
    r.h.ah = 0x07;
    r.h.al = (BYTE) -n; /* number of lines */
  }
  else
  {
    r.h.ah = 0x06;
    r.h.al = (BYTE) n;  /* number of lines */
  }

  /* tell BIOS to scroll */
  /*
    Set scroll mode fill character
  */
  if (VID_IN_GRAPHICS_MODE())
    r.h.bh = 0;			/* blanks are 00h in graphics mode */
  else
    r.h.bh = (BYTE) attr;
  r.h.ch = (BYTE) y1;
  r.h.cl = (BYTE) x1;
  r.h.dh = (BYTE) y2;
  r.h.dl = (BYTE) x2;
#if defined(__DPMI16__)
  _AX = r.x.ax;
  _BX = r.x.bx;
  _CX = r.x.cx;
  _DX = r.x.dx;
  geninterrupt(0x10);
#else
  INT86(0x10, &r, &r);
#endif
#endif
}



/*===========================================================================*/
/*                                                                           */
/* File    : VIDMON.C                                                        */
/*                                                                           */
/* Purpose : Routines for controlling multiple monitors                      */
/*                                                                           */
/*===========================================================================*/
#ifndef MEWEL_GUI

static int  CurrMonitor = 0;
void (*pfMonitorSwitch)(int) = NULL;       /* routine to perform monitor switch */
/* Monitor switching flags */
#define SAVE    0
#define RESTORE 1


INT FAR PASCAL MonitorSetRoutine(f)
  void (*f)(int);
{
  pfMonitorSwitch = f;
  return 0;
}

INT FAR PASCAL MonitorSwitch(iMon, mode)
  int  iMon,
       mode;
{
  static int new_mon;

  if (pfMonitorSwitch)
  {
    if (mode == RESTORE)
    {
      if (CurrMonitor != new_mon)
        (*pfMonitorSwitch)(CurrMonitor);
    }
    else 
    {
      if ((new_mon = iMon) != CurrMonitor)
        (*pfMonitorSwitch)(iMon);
    }
  }
  return 1;
}

#endif /* MEWEL_GUI */



/*===========================================================================*/
/*                                                                           */
/* File    : VIDPT2AD.C                                                      */
/*                                                                           */
/* Purpose : Takes a <row,col> point and returns the video address of it.    */
/*                                                                           */
/*===========================================================================*/
LPSTR FAR PASCAL PointToScrAddress(row, col)
  int row, col;
{
  LPSTR s;
#if defined(DOS386) || defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
  s = (LPSTR) (Scr_Base + ((row * VideoInfo.width + col) << 1));
#else
  s =  MK_FP(Scr_Base, ((row * VideoInfo.width + col) << 1));
#endif
  return s;
}


static WORD PASCAL DesqviewGetVideoBuffer(videoseg)
  unsigned videoseg;
{
#if defined(__DPMI32__)
  return videoseg;
#else
  union REGS r;
  struct SREGS seg;

#if defined(DOS286X)
  REGS16    r16;
  USHORT    sel;
#endif

  /*
    See if Desqview is installed. 
  */
  r.h.ch = 'D';
  r.h.cl = 'E';
  r.h.dh = 'S';
  r.h.dl = 'Q';
  r.x.ax = 0x2B01;
  intdos(&r, &r);
  if (r.h.al == 0xFF)
    return videoseg;   /* not installed - return the original video buf */
    
  /*
    Fetch the segment of the Desqview video buffer
  */

#if defined(DOS286X)
  //Under Phalap int10 function 0xFE is not supported.
  (void) seg;
  memset(&r16, 0, sizeof(REGS16));
  r16.es = videoseg;
  r16.ax = 0xFE00;
  DosRealIntr(0x10, &r16, 0L, 0);
  /*
    Map the segment to a selector of 64k bytes long
  */
  DosMapRealSeg(r16.es, 65536, &sel);
  return(sel);
#else
  seg.es = videoseg;
  r.h.ah = 0xFE;
  INT86X(0x10, &r, &r, &seg);
  return (seg.es);
#endif
#endif
}


/*===========================================================================*/
/*                                                                           */
/* Function: VidGetMode(void)                                                */
/*                                                                           */
/* Purpose : Initializes the video hardware. Detects which video mode we are */
/*           in, and fills the VideoInfo structure with crucial info.        */
/*                                                                           */
/* Returns : The current video mode.                                         */
/*                                                                           */
/*===========================================================================*/

/*
  We need byte-alignment on the BIOSVIDEOINFO structure because of the
  BYTE-length fields.
*/
#if defined(__ZTC__)
#pragma ZTC align 1
#elif defined(__HIGHC__)
#pragma Align_members(1)
#else
#pragma pack(1)
#endif

typedef struct video
{
  BYTE  crt_mode;        /* 0 */
  short crt_cols;        /* 1 */
  short crt_len;         /* 3 */
  short crt_start;       /* 5 */
  short cursor_pos[8];   /* 7 */
  short cursor_mode;     /* 23 */
  BYTE  active_page;     /* 25 */
  short addr_6845;       /* 26 */
  BYTE  crt_mode_set;    /* 28 */
  BYTE  crt_pallette;    /* 29 */
} BIOSVIDEOINFO,  TRUEFAR *LPBIOSVIDEOINFO;


INT FAR PASCAL VidGetMode(void)
{
#if defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FARPTR v;                       /* need 'far' for DOS386 */
#else
  LPBIOSVIDEOINFO v;              /* need 'far' for DOS386 */
#endif

#if (defined(__DPMI16__) || defined(__DPMI32__)) && defined(MEWEL_TEXT) && !defined(PLTNT)
  selZeroSeg = ParaToSel(0x0000);
  selA000Seg = ParaToSel(0xA000);
  selBiosSeg = ParaToSel(0x0040);

#if !defined(__DPMI32__)
  {
  DWORD FAR **lp = MK_FP(selZeroSeg, (0x43 * 4));
  DWORD FAR *lpGen = *lp;
  selCharGenerator = ParaToSel(FP_SEG(lpGen));
  }
#endif

#elif defined(DOS286X)
  DosMapRealSeg(0x0000, (long) 256*4, &selZeroSeg);
  DosMapRealSeg(0xA000, (long) 64L*1024L, &selA000Seg);
  {
  DWORD FAR * FAR *lp = MK_FP(selZeroSeg, (0x43 * 4));
  DWORD FAR *lpGen = *lp;
  DosMapRealSeg(FP_SEG(lpGen), (long) 64L*1024L, &selCharGenerator);
  }
  DosGetBIOSSeg(&selBiosSeg);

#elif defined(ERGO)
  ParaToSel((WORD) 0x0000, &selZeroSeg);
  ParaToSel((WORD) 0xA000, &selA000Seg);
  {
  DWORD FAR **lp = MK_FP(selZeroSeg, (0x43 * 4));
  DWORD FAR *lpGen = *lp;
  ParaToSel(FP_SEG(lpGen), &selCharGenerator);
  }
  ParaToSel((WORD) 0x40, &selBiosSeg);
#endif


  /*
    Get a pointer to the BIOS video info structure
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  {
  LPVOID pReal = (LPVOID) 0x00400049L;
  GetMappedDPMIPtr(&((LPVOID) v), (LPVOID) pReal, (int) 256);
  }
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FP_SET(v, 0x449, 0x34);
#else
  v = (LPBIOSVIDEOINFO) GetBIOSAddress(0x49);
#endif

  VidQueryDimensions();


  /*
    For text mode, we must get the segment of video memory. This
    segment can be different under Desqview, so we keep two variables :
      Scr_Base     : can be maped by Desqview
      ScrBaseOrig  : Always B800 or B000
  */
#if defined(MEWEL_TEXT)

#if defined(__DPMI32__) && !defined(PLTNT)
  ScrBaseOrig = Scr_Base = (PeekFarWord(&v->addr_6845)==0x3D4) ? 0xB800 : 0xB000;
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FP_SET(v, 0x449 + 26, 0x34);
  ScrBaseOrig = Scr_Base = (PeekFarWord(v) == 0x3D4) ? 0xB800 : 0xB000;
#elif defined(__GNUC__)
  ScrBaseOrig = Scr_Base = 0xB800;
#else
  ScrBaseOrig = Scr_Base = (v->addr_6845 == 0x3D4) ? 0xB800 : 0xB000;
#endif

  Scr_Base = DesqviewGetVideoBuffer(Scr_Base);


#if defined(__DPMI32__) && !defined(PLTNT)
  {
  LPVOID pReal = (LPVOID) (Scr_Base << 16);
  LPVOID lpScr;
  GetMappedDPMIPtr(&((LPVOID) lpScr), (LPVOID) pReal, (ULONG) VideoInfo.width * VideoInfo.length * 2);
  Scr_Base = (WORD) (ULONG) lpScr;
  }
#elif defined(__DPMI16__)
  Scr_Base = ParaToSel(Scr_Base);
#elif defined(DOS286X)
  {
  unsigned sel;
  DosMapRealSeg(Scr_Base, (ULONG) VideoInfo.width * VideoInfo.length * 2,
                (PSEL) &sel);
  Scr_Base = sel;
  }
#elif defined(ERGO)
  {
  WORD sel;
  ParaToSel(Scr_Base, &sel);
  Scr_Base = sel;
  }
#elif defined(DOS386)
  /*
    For Zortech's DOSX, we need to use 0xB8000 as the screen address
  */
  Scr_Base <<= 4;
  ScrBaseOrig <<= 4;
#elif defined(PL386)
  ScrBaseOrig = Scr_Base = 0x1C;
#endif

  /*
    Get the segment of the video buffer, and add the starting offset
    from the BIOS info.
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  VideoInfo.segment = Scr_Base + (PeekFarByte(&v->crt_start) >> 4);
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FP_SET(v, 0x449 + 0, 0x34);
  VideoInfo.segment = Scr_Base + (PeekFarByte(v) >> 4);
#else
  VideoInfo.segment = Scr_Base + (v->crt_start >> 4);
#endif

#endif /* MEWEL_TEXT */


  /*
    Get the starting video mode so that we can restore it when we
    exit the program.
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  VideoInfo.starting_mode = PeekFarByte(&v->crt_mode);
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FP_SET(v, 0x449 + 0, 0x34);
  VideoInfo.starting_mode = PeekFarByte(v);
#else
  VideoInfo.starting_mode = v->crt_mode;
#endif


  if (SysGDIInfo.StartingVideoMode < 0)
    SysGDIInfo.StartingVideoMode = VideoInfo.starting_mode;
  VideoInfo.flags = VidQueryAdapterType();

  /*
    If we are dealing with a graphics mode we don't know about,
    then just use the BIOS...

     WARNING:  This test will REPLACE the .flags for video mode 6
               which means the value in .flags is lost
  */
  if (VideoInfo.flags != CGA)
    if (VideoInfo.starting_mode == 6)
    {
      VideoInfo.flags = USEVIDEOBIOS;
    }

#if defined(__DPMI32__) && defined(MEWEL_TEXT)
  VideoInfo.flags |= USEVIDEOBIOS;
#endif

  VideoInfo.yFontHeight = VidQueryFontHeight(VideoInfo.flags & 0x07);
  VideoInfo.xFontWidth  = 8;

#ifdef MEWEL_GUI
  /*
    For the GUI version, the display is measured in pixels.
  */
  VideoInfo.width  = _VidDevCaps(0, HORZRES);
  VideoInfo.length = _VidDevCaps(0, VERTRES);
#endif

  /*
    Set up MEWEL's system metrics
  */
  MEWELSysMetrics[SM_CXSCREEN]     = VideoInfo.width;
  MEWELSysMetrics[SM_CYSCREEN]     = VideoInfo.length;
  MEWELSysMetrics[SM_CXFULLSCREEN] = VideoInfo.width;
  MEWELSysMetrics[SM_CYFULLSCREEN] = VideoInfo.length - 
                                            IGetSystemMetrics(SM_CYCAPTION);
  SetRect(&SysGDIInfo.rectScreen, 0, 0, VideoInfo.width, VideoInfo.length);

  /*
    Set some vidcaps info
  */
  SysGDIInfo.cxScreenMM = 240;
  SysGDIInfo.cyScreenMM = 180;
#if defined(MEWEL_TEXT)
  SysGDIInfo.cxLogPixel = 12;
  SysGDIInfo.cyLogPixel = 6; 
#else
  SysGDIInfo.cxLogPixel = 96;
  SysGDIInfo.cyLogPixel = 96;
#endif


  /*
    CUSTOMIZATION NOTE :
    This is the place where we detect if the video adapter is in graphics
    mode. Right now, we do a simple test to see if the mode is greater 
    than 3 and not 7 (mono mode). For special video modes which your
    video card supports, you may want to insert more checks in here.
  */
  if (VideoInfo.starting_mode >= 4 && VideoInfo.starting_mode != 7)
  {
    /*
      Much thanks to Modular Software for this code...
    */
    if (IsVGA() && VideoInfo.starting_mode > 0x13)
    {
      /*
        If we are using a VGA, and the mode is outside of the "standard"
        range, then we check the Graphics Mode Enable bit of the
        Miscellaneous Register.  This should be more reliable than just
        ignoring the possibility of enhanced text modes.
      */
#if !defined(MEWEL_GUI) && !defined(__DPMI32__)
      BYTE index, value;

      index = (BYTE) inp(0x3CE);      /* save previous "index" */
      outp(0x3CE, 6);                 /* index to Miscellaneous Register */
      value = (BYTE) inp(0x3CF);      /* get the Miscellaneous Register value */
      outp(0x3CE, index);             /* restore the "index" */
      if (value & 1)                  /* bit 0 is the Graphics Mode Enable bit */
#endif
        VideoInfo.flags |= IN_GRAPHICS_MODE;
    }
    else 
      VideoInfo.flags |= IN_GRAPHICS_MODE;
  }

  if (VideoInfo.flags & 0x01 || bForceMono || VideoInfo.starting_mode == 2)
    VideoInfo.flags |= IN_MONO_MODE;

  /*
    Set up the virtual screen and the visibily map for text mode
  */
#if defined(MEWEL_TEXT)
  VirtualScreenInit();
  WinInitVisMap();
  /*
    If we are coming here from WinExec, and hence there are windows
    on the screen, then refresh the vismap.
  */
  if (InternalSysParams.WindowList)
    WinUpdateVisMap();
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
  FreeMappedDPMIPtr(v);
#endif

  return VideoInfo.starting_mode;
}


static VOID PASCAL VidQueryDimensions(void)
{
#if defined(PLTNT)
  VideoInfo.length = 25;

#else

#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

  /*
    Get the width by using int 10H, service 0FH
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  r.AX = 0x0F00;
  RealIntr(0x10, &r);
  VideoInfo.width = (r.AX >> 8) & 0x00FF;
#elif defined(__DPMI16__)
  _AH = 0x0F;
  geninterrupt(0x10);
  VideoInfo.width = _AH;
#else
  r.h.ah = 0x0F;
  INT86(0x10, &r, &r);
  VideoInfo.width = r.h.ah;
#endif

  /*
    Get number of rows
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  r.AX = 0x1130;
  r.DX = 0x18;   /* preload with 24 if we don't have an EGA card */
  r.BX = 0x00;
  RealIntr(0x10, &r);
  VideoInfo.length = (r.DX & 0x00FF) + 1;
#elif defined(__DPMI16__)
  _AX = 0x1130;
  _DL = 0x18;   /* preload with 24 if we don't have an EGA card */
  _BX = 0x00;
  geninterrupt(0x10);
  VideoInfo.length = _DL + 1;
#else
  r.h.ah = 0x11;
  r.h.al = 0x30;
  r.h.dl = 0x18;   /* preload with 24 if we don't have an EGA card */
  r.x.bx = 0x00;
  INT86(0x10, &r, &r);
  VideoInfo.length = r.h.dl + 1;
#endif

#endif
}


/*
  This short function does a simple fetch of the BIOS area video mode
  without the frills which VidGetMode() has.
*/
INT FAR PASCAL VidGetCurrVideoMode(void)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  LPBIOSVIDEOINFO v;
  LPVOID pReal;
  int iMode = -1;

  pReal = (LPVOID) 0x00400049L;
  if (GetMappedDPMIPtr(&((LPVOID) v), (LPVOID) pReal, (int) 256))
  {
    iMode = PeekFarByte(&v->crt_mode);
    FreeMappedDPMIPtr(v);
  }
  return iMode;
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
  FARPTR v;
  FP_SET(v, 0x449 + 0, 0x34);
  return (INT) PeekFarByte(v);
#else
  LPBIOSVIDEOINFO v = (LPBIOSVIDEOINFO) GetBIOSAddress(0x49);
  return v->crt_mode;
#endif
}


#ifdef WAGNER_GRAPHICS
VOID FAR PASCAL VidGetGraphMode(void)
{
  VideoInfo.width  = vidmode.hor_pels / vidmode.char_width;
  VideoInfo.length = vidmode.ver_pels / vidmode.char_height;
  ScrBaseOrig = Scr_Base = vidmode.vidseg;
  VideoInfo.segment = Scr_Base;
  VideoInfo.starting_mode = 6;
  VideoInfo.yFontHeight = vidmode.char_height;
  VideoInfo.flags = VGAMONO | IN_GRAPHICS_MODE; /* ?? */

  /*
    Set up MEWEL's system metrics
  */
  MEWELSysMetrics[SM_CXSCREEN]     = VideoInfo.width;
  MEWELSysMetrics[SM_CYSCREEN]     = VideoInfo.length;
  MEWELSysMetrics[SM_CXFULLSCREEN] = VideoInfo.width;
  MEWELSysMetrics[SM_CYFULLSCREEN] = VideoInfo.length - 
                                            IGetSystemMetrics(SM_CYCAPTION);
  SetRect(&SysGDIInfo.rectScreen, 0, 0, VideoInfo.width, VideoInfo.length);
}
#endif


int FAR PASCAL VidQueryAdapterType(void)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

  /*
    See if we have a VGA
  */
#if defined(__DPMI32__) && !defined(PLTNT)
  r.AX = 0x1A00;
  RealIntr(0x10, &r);
#elif defined(__DPMI16__)
  _AX = 0x1A00;
  geninterrupt(0x10);
  r.x.ax = _AX;
  r.x.bx = _BX;
#else
  r.x.ax = 0x1A00;
  INT86(0x10, &r, &r);
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
  if ((r.AX & 0x00FF) == 0x1A)
#else
  if (r.h.al == 0x1A)
#endif
  {
    /*
      We have a VGA.
    */
#if defined(__DPMI32__) && !defined(PLTNT)
    switch (r.BX & 0x00FF)
#else
    switch (r.h.bl)
#endif
    {
      case 0 :  return -1;
      case 1 :  return MDA;
      case 2 :  return CGA;
      case 4 :  return EGACOLOR;
      case 5 :  return EGAMONO;
      case 7 :  
        /*
          Include check for IBM 8514A
        */
#if defined(__DPMI32__) && !defined(PLTNT)
        r.AX = 0x1200;
        r.BX = 0x10;
        RealIntr(0x10, &r);
        return ((r.BX & 0x0000FF00L) == 0) ? VGACOLOR : VGAMONO;
#elif defined(__DPMI16__)
        _AH = 0x12;
        _BL = 0x10;
        geninterrupt(0x10);
        return (_BH == 0) ? VGACOLOR : VGAMONO;
#else
        r.h.ah = 0x12;
        r.h.bl = 0x10;
        INT86(0x10, &r, &r);
        return (r.h.bh == 0) ? VGACOLOR : VGAMONO;
#endif
      case 8 :  return (ScrBaseOrig == 0xB000) ? VGAMONO : VGACOLOR;
      case 10:
      case 12:  return MCGACOLOR;
      case 11:  return MCGAMONO;
      default:  return CGA;
    }
  }
  else
  {
    /*
      See if we have an EGA
    */
#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = 0x1200;
    r.BX = 0x10;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AH = 0x12;
    _BL = 0x10;
    geninterrupt(0x10);
    r.x.bx = _BX;
#else
    r.h.ah = 0x12;
    r.x.bx = 0x0010;
    INT86(0x10, &r, &r);
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
    if ((r.BX & 0x0000FFFFL) != 0x0010)
#else
    if (r.x.bx != 0x0010)
#endif
    {
      /*
        We have an EGA. See if it's color or mono.
      */
#if defined(__DPMI32__) && !defined(PLTNT)
      r.AX = 0x1200;
      r.BX = 0x10;
      RealIntr(0x10, &r);
      return ((r.BX & 0x0000FF00L) == 0) ? EGACOLOR : EGAMONO;
#elif defined(__DPMI16__)
      _AH = 0x12;
      _BL = 0x10;
      geninterrupt(0x10);
      return (_BH == 0) ? EGACOLOR : EGAMONO;
#else
      r.h.ah = 0x12;
      r.h.bl = 0x10;
      INT86(0x10, &r, &r);
      return (r.h.bh == 0) ? EGACOLOR : EGAMONO;
#endif
    }
    else
    {
      /*
        Not a VGA, MCGA, nor an EGA. Must be a CGA or MDA.
      */
#if defined(__DPMI32__) && !defined(PLTNT)
      RealIntr(0x11, &r);
#elif defined(__DPMI16__)
      geninterrupt(0x11);
      r.h.al = _AL;
#else
      INT86(0x11, &r, &r);          /* Get equipment flags  */
#endif

#if defined(__DPMI32__) && !defined(PLTNT)
      switch ((r.AX   & 0x30) >> 4) /* Isolate monitor type */
#else
      switch ((r.h.al & 0x30) >> 4) /* Isolate monitor type */
#endif
      {
        case 1 :
        case 2 :  return CGA;
        case 3 :  return MDA;
        default:  return -1;
      }
    }
  }
}

int FAR PASCAL IsEGA(void)
{
  return (IsVGA() ||
          (VideoInfo.flags & 0x07) == EGACOLOR || 
          (VideoInfo.flags & 0x07) == EGAMONO);
}
int FAR PASCAL IsVGA(void)
{
  return ((VideoInfo.flags & 0x07) == VGACOLOR || 
          (VideoInfo.flags & 0x07) == VGAMONO);
}

#if (defined(__DPMI16__) || defined(__DPMI32__)) && !defined(PLTNT)
static SEL PASCAL ParaToSel(WORD sel)
{
  _AX = 2;
  _BX = sel;
  geninterrupt(0x31);
  return _AX;
}
#endif


/****************************************************************************/
/*                                                                          */
/* Function : VidQueryFontHeight()                                          */
/*                                                                          */
/* Purpose  : Maps the adapter type to the current pixel height of the font.*/
/*                                                                          */
/* Returns  : The height of the system font in pixels.                      */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL VidQueryFontHeight(iAdapterType)
  INT iAdapterType;
{
  switch (iAdapterType)
  {
    case CGA      :
      return 8;

    case MDA      :
      return 14;

    case MCGAMONO :
    case MCGACOLOR:
      return 16;

    case EGAMONO  :
    case EGACOLOR :
    case VGAMONO  :
    case VGACOLOR :
    {
      /*
        Look at the BIOS data area in location 0x40:0x85.
      */
#if defined(__DPMI32__) && !defined(PLTNT)
      LPVOID lpVidPixheight;
      LPVOID pReal;
      int    iMode = 16;
      pReal = (LPVOID) 0x00400085L;
      if (GetMappedDPMIPtr(&((LPVOID) lpVidPixheight), (LPVOID) pReal, 16))
      {
        iMode = PeekFarWord(lpVidPixheight);
        FreeMappedDPMIPtr(lpVidPixheight);
      }
      return iMode;
#elif defined(PLTNT) && (defined(MSC) || defined(__BORLANDC__))
      FARPTR lpVidPixheight;
      FP_SET(lpVidPixheight, 0x485, 0x34);
      return PeekFarWord(lpVidPixheight);
#elif defined(DOS386) || defined(__HIGHC__) || (defined(PL386) && defined(__WATCOMC__))
      short TRUEFAR *lpVidPixheight = (short TRUEFAR *) GetBIOSAddress(0x85);
      return *lpVidPixheight;
#else
      WORD  FAR *lpVidPixheight = (WORD  FAR *) GetBIOSAddress(0x85);
      return *lpVidPixheight;
#endif
    }
  }
  return CGA;
}


/****************************************************************************/
/*                                                                          */
/* Function : VidTerminate()                                                */
/*                                                                          */
/* Purpose  : Called by WinTerminate() as the very last thing before        */
/*            exiting. Restores the original video mode, hides the mouse,   */
/*            shows the hardware cursor, and clears the screen.             */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VidTerminate(void)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif

#ifdef MEWEL_GUI
  WinCloseGraphics();
  VideoInfo.flags &= ~IN_GRAPHICS_MODE;
#endif

  MouseHide();
  if (VidGetCurrVideoMode() != SysGDIInfo.StartingVideoMode)
  {
#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = SysGDIInfo.StartingVideoMode;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AH = 0x00;
    _AL = (char) SysGDIInfo.StartingVideoMode;
    geninterrupt(0x10);
#else
    r.h.ah = 0x00;
    r.h.al = (char) SysGDIInfo.StartingVideoMode;
    INT86(0x10, &r, &r);
#endif
    VidQueryDimensions(); /* so VidClearScreen has good coords to work with */
  }
#ifndef MEWEL_GUI
  else if (IsEGA() && (SysGDIInfo.StartingVideoLength != (int) VideoInfo.length))
    ToggleEGA();
#endif
  VidSetBlinking(TRUE);
  MouseHide();

  /*
    Restore the cursor and clear the screen.
  */
  VidShowCursor();
  VidSetPos(0, 0);
  VidClearScreen(MAKE_ATTR(WHITE, BLACK));
}



/*===========================================================================*/
/*                                                                           */
/* File    : VIDCURS.C                                                       */
/*                                                                           */
/* Purpose : Routines to set the size of the hardware cursor                 */
/*                                                                           */
/*===========================================================================*/
/* VidSetPos(row, col) - positions the cursor at row,col */
VOID FAR PASCAL VidSetPos(int row, int col)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif
  
  if (VID_IN_GRAPHICS_MODE() && IsEGA())
  {
    /*
      If the cursor is visible and we are moving to a new position,
      then restore the bits under the current cursor position.
    */
    if ((CursorInfo.fState & CURSOR_VISIBLE) && !bVirtScreenEnabled)
      VidDoCursorBits(CursorInfo.achSavedBits, 
#ifdef MEWEL_GUI
                      CursorInfo.col,
                      CursorInfo.row,
#else
                      CursorInfo.col * VideoInfo.xFontWidth,
                      CursorInfo.row * VideoInfo.yFontHeight,
#endif
                      FALSE);
    CursorInfo.fState |= CURSOR_VISIBLE;
    CursorInfo.row = row;
    CursorInfo.col = col;
    VidSetGraphicsCursor();
  }
  else
  {
    CursorInfo.row = row;
    CursorInfo.col = col;
#if defined(__DPMI32__) && !defined(PLTNT)
    r.DX = (row << 8) | col;
    r.AX = 0x0200;
    r.BX = 0;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _DX = (row << 8) | col;
    _AH = 0x02;
    _BH = 0;
    geninterrupt(0x10);
#else
    r.h.ah = 2;
    r.h.bh = 0;
    r.x.dx = (row << 8) | col;
    INT86(0x10, &r, &r);
#endif
  }
}

/* VidGetPos - returns the cursor coordinates */
VOID FAR PASCAL VidGetPos(int *row, int *col)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif
  
/* WAGNER: moved INT 10 into ELSE part - doesn't make sense in graphics */

  if (VID_IN_GRAPHICS_MODE())
  {
    *row = CursorInfo.row;
    *col = CursorInfo.col;
  }
  else
  {
#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = 0x0300;
    r.BX = 0;
    RealIntr(0x10, &r);
    CursorInfo.row = *row = ((r.DX >> 8) & 0x00FF);
    CursorInfo.col = *col = (r.DX & 0x00FF);
#elif defined(__DPMI16__)
    _AH = 0x03;
    _BH = 0;
    geninterrupt(0x10);
    CursorInfo.row = *row = _DH;
    CursorInfo.col = *col = _DL;
#else
    r.h.ah = 3;
    r.h.bh = 0;
    INT86(0x10, &r, &r);
    CursorInfo.row = *row = r.h.dh;
    CursorInfo.col = *col = r.h.dl;
#endif
  }
}

INT FAR PASCAL VidHideCursor(void)
{
  return VidSetCursorScanLines(32, 0, 0);
}

INT FAR PASCAL VidShowCursor(void)
{
  return VidSetCursorMode(0, 0);
}

/* VidSetCursorMode() - change the size of the cursor */
INT FAR PASCAL VidSetCursorMode(int isblock, int nWidth)
{
#if defined(PL386)
  BOOL bColor = (BOOL) (ScrBaseOrig == 0x1C);
#elif defined(DOS386)
  BOOL bColor = (BOOL) (ScrBaseOrig == 0xB8000);
#else
  BOOL bColor = (BOOL) (ScrBaseOrig == 0xB800);
#endif
  int  start = 0;
  int  end   = bColor ? 7 : 13;    /* block cursor */

  static int iLastMode;      /* holds the previous mode */


  /*
    If the user passes in -1, this means that the previous mode should
    be restored.
  */
  if (isblock == -1)
    isblock = iLastMode;

  if (!isblock)
    start = bColor ? 6 : 12;    /* underline */
  iLastMode = isblock;
  return VidSetCursorScanLines(start, end, nWidth);
}

INT FAR PASCAL VidSetCursorScanLines(int startscan, int endscan, int nWidth)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif
  
#if defined(__DPMI32__) && !defined(PLTNT)
  CursorInfo.startscan =          (BYTE) startscan;
  CursorInfo.endscan   =          (BYTE) endscan;
  CursorInfo.nWidth    = nWidth;
  r.CX = (startscan << 8) | endscan;
#else
  CursorInfo.startscan = r.h.ch = (BYTE) startscan;
  CursorInfo.endscan   = r.h.cl = (BYTE) endscan;
  CursorInfo.nWidth    = nWidth;
#endif

  if (VID_IN_GRAPHICS_MODE())
  {
    if ((CursorInfo.fState & CURSOR_VISIBLE) && !bVirtScreenEnabled)
      VidDoCursorBits(CursorInfo.achSavedBits, 
#ifdef MEWEL_GUI
                      CursorInfo.col,
                      CursorInfo.row,
#else
                      CursorInfo.col * VideoInfo.xFontWidth,
                      CursorInfo.row * VideoInfo.yFontHeight,
#endif
                      FALSE);
    VidSetGraphicsCursor();
    return TRUE;
  }
  else
  {
#if defined(__DPMI32__) && !defined(PLTNT)
    r.AX = 0x0100;
    RealIntr(0x10, &r);
#elif defined(__DPMI16__)
    _AX = 0x0100;
    _CX = r.x.cx;
    geninterrupt(0x10);
#else
    r.x.ax = 0x0100;
    INT86(0x10, &r, &r);
#endif
    if (startscan > endscan)
      CursorInfo.fState &= ~CURSOR_VISIBLE;
    else
      CursorInfo.fState |= CURSOR_VISIBLE;
    return TRUE;
  }
}

/****************************************************************************/
/*                                                                          */
/*     Routines to deal with drawing an edit cursor n graphics mode         */
/*                                                                          */
/****************************************************************************/
static VOID PASCAL VidSetGraphicsCursor(void)
{
  BYTE achXOR[MAXCURSORHEIGHT][MAXCURSORWIDTH];
  int  i, j;
  int  xPixel, yPixel;

#ifdef MEWEL_GUI
  yPixel = CursorInfo.row;
  xPixel = CursorInfo.col;
#else
  yPixel = CursorInfo.row * VideoInfo.yFontHeight;
  xPixel = CursorInfo.col * VideoInfo.xFontWidth;
#endif

  if (CursorInfo.nWidth <= 0)
    CursorInfo.nWidth = SysGDIInfo.tmAveCharWidth;
  if (CursorInfo.endscan <= 0)
    CursorInfo.endscan = 1;

  if (CursorInfo.startscan > CursorInfo.endscan)
  {
    /*
      The cursor should be hidden. We must restore the previous contents
      under the visible cursor position and mark the cursor as invisible.
    */
    if (CursorInfo.fState & CURSOR_VISIBLE)
    {
      VidDoCursorBits(CursorInfo.achSavedBits, xPixel, yPixel, FALSE);
      CursorInfo.fState &= ~CURSOR_VISIBLE;
    }
  }
  else if ((CursorInfo.fState & CURSOR_VISIBLE) && !bVirtScreenEnabled)
  {
    /*
      The cursor should be made visible. We need to save the contents under
      the cursor and mark the cursor as visible.
    */
    VidDoCursorBits(CursorInfo.achSavedBits, xPixel, yPixel, TRUE);

    /*
      XOR the saved bits, according to the size of the cursor.
    */
    memcpy(achXOR, CursorInfo.achSavedBits, sizeof(achXOR));
#if 1
    for (i = 0;  i < CursorInfo.endscan;  i++)
#else
    for (i = CursorInfo.startscan;  i <= CursorInfo.endscan;  i++)
#endif
      for (j = CursorInfo.nWidth-1;  j >= 0;  j--)
        achXOR[i][j] ^= 0x0F;

    /*
      Put the XORed bits back onto the display
    */
    VidDoCursorBits(achXOR, xPixel, yPixel, FALSE);
  }
}


static VOID PASCAL VidDoCursorBits(achPixels, xPixel, yPixel, bSave)
  BYTE achPixels[MAXCURSORHEIGHT][MAXCURSORWIDTH];
  int  xPixel, yPixel;  /* absolute pixel coords of origin */
  BOOL bSave;
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif
  int        x, y;

  static     int nWidth = 0, nHeight = 0;


  MOUSE_HideCursor();

#ifndef MEWEL_GUI
  /*
    Save the area under the character, so increment 'yPixel' so that 
    we are below the character.
  */
  yPixel += VideoInfo.yFontHeight;
#endif

  if (CursorInfo.nWidth <= 0)
    CursorInfo.nWidth = SysGDIInfo.tmAveCharWidth;
  if (CursorInfo.endscan <= 0)
    CursorInfo.endscan = 1;

  if (bSave)
  {
    nHeight = CursorInfo.endscan;
    nWidth  = CursorInfo.nWidth;
  }

  if (nHeight >= MAXCURSORHEIGHT)
    nHeight = MAXCURSORHEIGHT;
  if (nWidth  >= MAXCURSORWIDTH)
    nWidth  = MAXCURSORWIDTH;


  for (y = 0;  y < nHeight;  y++)
  {
    for (x = 0;  x < nWidth;  x++)
    {
#if defined(__DPMI32__) && !defined(PLTNT)
      if (bSave)
        r.AX = 0x0D00;
      else
        r.AX = 0x0C00 | achPixels[y][x];
      r.BX = 0;
      r.CX = xPixel + x;
      r.DX = yPixel + y;
      RealIntr(0x10, &r);
      if (bSave)
        achPixels[y][x] = (BYTE) (r.AX & 0x00FF);
#else
      if (bSave)
      {
        r.h.ah = 0x0D;
      }
      else
      {
        r.h.ah = 0x0C;
        r.h.al = achPixels[y][x];
      }
      r.h.bh = 0;
      r.x.cx = xPixel + x;
      r.x.dx = yPixel + y;
#if defined(__DPMI16__)
      _AX = r.x.ax;
      _BX = r.x.bx;
      _CX = r.x.cx;
      _DX = r.x.dx;
      geninterrupt(0x10);
      r.h.al = _AL;
#else
      INT86(0x10, &r, &r);
#endif
      if (bSave)
        achPixels[y][x] = r.h.al;
#endif
    }
  }

  MOUSE_ShowCursor();
}



#if !defined(MEWEL_GUI)
/****************************************************************************/
/*                                                                          */
/*                          VIO Routines                                    */
/*                                                                          */
/****************************************************************************/

typedef VOID FAR PASCAL VGAWRTPROC(WORD, int, int, COLOR);
static VGAWRTPROC *PASCAL VioGetDriver(void);

extern VOID FAR PASCAL ATIWrtGraphicsChar256(WORD, int, int, COLOR);
extern VOID FAR PASCAL VGAWrtGraphicsChar(WORD, int, int, WORD);
extern VOID FAR PASCAL WrtGraphicsChar6(WORD, int, int, WORD);
extern VOID FAR PASCAL VGAMode13WrtGraphicsChar(WORD, int, int, COLOR);
extern VOID FAR PASCAL BGIWrtGraphicsChar(WORD, int, int, COLOR);


#if defined(DOS) && !defined(DOS386) && !defined(WC386) && !defined(PL386) && !defined(__GNUC__) && !defined(__DPMI32__)
#define GETVIRTSCREENBASE()  (MK_FP(VirtualScreen.segVirtScreen, 0))
#else
#define GETVIRTSCREENBASE()  ((LPCELL) VirtualScreen.pVirtScreen)
#endif



int FAR PASCAL 
VioReadCellStr(LPCELL buf, int *piWidth, int row, int col, int iReserved)
{
  int    n;
  LPCELL lpVirtScr;

  (void) iReserved;

  /*
    Get a pointer into the virtual screen buffer. This should have a
    copy of the current display. We will read from there.
  */
  lpVirtScr = GETVIRTSCREENBASE();
  lpVirtScr += row*VideoInfo.width + col;

  for (n = (*piWidth) >> 1;  n > 0;  n--)
  {
    *buf++ = *lpVirtScr++;
    if (++col >= (int) VideoInfo.width)
    {
      col = 0;
      if (++row >= (int) VideoInfo.length)
        break;
    }
  }
  return (*piWidth -= n*SCREENCELLSIZE);
}


/****************************************************************************/
/* These routines take care of screen writing when MEWEL is in graphics     */
/* mode or when the BIOS is being used in text mode.                        */
/*                                                                          */
/* The entry point in VioWrtCellStr(), and is called by MEWEL in two places */
/* from VirtualScreenFlush and from VidBlastToScreen.                       */
/*                                                                          */
/* The call tree looks like this :               ------- _VGAVioWrtCellStr  */
/*                                               |text or graphics BIOS     */
/*               EGA/VGA                         |fast graphics             */
/* VioWrtCellStr -------------- VGAVioWrtCellStr |------ VGAWrtGraphicsChar */
/*               |                                /      VGAMode13WrtGraph  */
/*               |                               /                          */
/*               |                              /                           */
/*               -------------- VioWrtNCell----|                            */
/* CGA or E/VGA text writing to bottom corner  |                            */
/*                                             ------- BIOS int 10, svc 9   */
/*                                                                          */
/****************************************************************************/

#if defined(__NTCONSOLE__)
VOID FAR PASCAL
VioWrtCellStr(LPCELL buf, int iWidth, int row, int col, int iReserved)
{
  static CHAR_INFO *pCharInfoBuf = NULL;
  static int  iMaxWidth = 0;

  CHAR_INFO *lpCI;
  COORD     coordSize;
  COORD     coordBuf;
  SMALL_RECT r;


  (void) iReserved;

  /*
    Get a handle to the NT console.... this is done only one time.
  */
  if (NTConsoleInfo.hStdOutput == NULL)
    NTConsoleInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

  /*
    iWidth is the width of the buf[] array, which is the number of
    chars to write times the size of a character cell.
  */
  iWidth /= sizeof(CELL);

  /*
    Get the size of the source buffer and the starting coordinates
    of the source buffer.
  */
  coordSize.X = iWidth;
  coordSize.Y = 1;
  coordBuf.X  = coordBuf.Y  = 0;

  /*
    Determine the output rectangle
  */
  r.Left   = col;
  r.Top    = row;
  r.Right  = col+iWidth;
  r.Bottom = row+1;

  /*
    Allocate (or reallocate) the CHAR_INFO buffer
  */
  if (iWidth > iMaxWidth)
  {
    if (pCharInfoBuf)
      MyFree(pCharInfoBuf);
    pCharInfoBuf = (CHAR_INFO *) emalloc_far((iWidth+1) * sizeof(CHAR_INFO));
    iMaxWidth = iWidth;
  }

  /*
    Transfer the char/attr pairs into a CHAR_INFO buffer
  */
  for (lpCI = pCharInfoBuf;  iWidth-- > 0;  lpCI++)
  {
    CELL cell = *buf++;
    lpCI->Attributes = (cell >> 8) & 0xFF;
    lpCI->Char.AsciiChar = (cell & 0xFF);
  }

  /*
    Write the text and the attributes
  */
  WriteConsoleOutputA(NTConsoleInfo.hStdOutput, pCharInfoBuf,
                      coordSize, coordBuf, &r);
}

#else

VOID FAR PASCAL
VioWrtCellStr(LPCELL buf, int iWidth, int row, int col, int iReserved)
{
  int   origRow = row,
        origCol = col;
  int   nReps;
  CELL  cell;

  (void) iReserved;

#ifdef WAGNER_GRAPHICS
  if (VID_IN_GRAPHICS_MODE())
  {
    LPCELL lpVirtScr;
    BOOL bHidMouse = MouseHideIfInRange(row - 1, row + 1);
    BOOL bCaretPushed;

    bCaretPushed = CaretPush(row - 1, row + 1);
    gwritecellstring(buf, col*FONT_WIDTH,row*VideoInfo.yFontHeight, iWidth>>1);
    if (bHidMouse)
      MouseShow();
    if (bCaretPushed)
      CaretPop();
      

    /*
      Write the string back into the virtual screen.
    */
    lpVirtScr = GETVIRTSCREENBASE();
    lpVirtScr += row*VideoInfo.width + col;
    while (iWidth)
    {
      if (*buf != 0xFFFF)
        *lpVirtScr = *buf;
      lpVirtScr++;
      buf++;
      iWidth -= 2;
    }
    return;
  }
#else   
  /*
    A VGA in graphics mode? Use the fast graphics writing routines.

    We use the fast VGA writing routine if we detect a VGA and
    if we are not writing in the bottom right corner. The VGA fast
    writing routine will scroll the screen vertically if we attempt
    to write a character in the lower right corner, so we bypass
    it in this special case. (IBM bug?)
  */
  if (VideoInfo.starting_mode != 6)
    if (IsEGA())
      if (VID_IN_GRAPHICS_MODE() ||
          row < (int) VideoInfo.length-1 || 
          col + (iWidth >> 1) < (int) VideoInfo.width)
      {
        VGAVioWrtCellStr(buf, iWidth, row, col, iReserved);
        return;
      }
#endif

  /*
    VioWrtNCell is called if we are hooked up to a CGA, or if we are
    in text mode on a E/VGA and we are writing into the bottom right corner.
  */
  for (iWidth >>= 1;  iWidth > 0 && *buf;  )
  {
    cell = *buf++;
    for (nReps = 1, iWidth--;  iWidth > 0 && *buf && *buf == cell;  iWidth--)
    {
      buf++;
      nReps++;
    }
    VioWrtNCell(cell, nReps*2, row, col, 0);

    if ((col += nReps) >= (int) VideoInfo.width)
    {
      col = nReps % VideoInfo.width;  
      if ((row += nReps / VideoInfo.width) >= (int) VideoInfo.length)
        break;
    }
  }

  VidSetPos(origRow, origCol);
}
#endif


VOID FAR PASCAL 
VioWrtNCell(CELL cell, int iWidth, int row, int col, int iReserved)
{
#if defined(__DPMI32__) && !defined(PLTNT)
  TREGISTERS r;
#else
  union REGS r;
#endif
  BOOL       bHidMouse, bCaretPushed;
  LPCELL     lpVirtScr;

  (void) iReserved;

  if ((cell & 0x00FF) == 0x00FF)
    return;

  /*
    See if the mouse is visible in the area we are writing to. If so,
    then hide it while we write to the screen.
  */
  bHidMouse = MouseHideIfInRange(row - 1, row + 1);
  bCaretPushed = CaretPush(row - 1, row + 1);

  if (VID_IN_GRAPHICS_MODE() && IsEGA())
  {
    int  n;
    int  x    = col * FONT_WIDTH;
    int  y    = row * VideoInfo.yFontHeight;
    WORD ch   = (cell & 0xFF);
    WORD attr = (cell >> 8) & 0x00FF;
    VGAWRTPROC *lpfnVGAWrtGraphicsChar;

#ifdef WAGNER_GRAPHICS
    {
    vidcontext *savdc = setcontext(NULL);
    for (n = iWidth >> 1;  n-- > 0;  x += FONT_WIDTH)
      gwritechar(ch, x, y, attr);
    setcontext(savdc);
    }
#else
    if (VID_USE_GRAPHICS_BIOS())
      cell = (cell & 0x00FF) | 0x0700;
    else
      lpfnVGAWrtGraphicsChar = VioGetDriver();
    for (n = iWidth >> 1;  n-- > 0;  x += FONT_WIDTH)
    {
      if (VID_USE_GRAPHICS_BIOS())
        _VGAVioWrtCellStr((LPCELL) &cell, 1, y, x, 0);
      else
        (*lpfnVGAWrtGraphicsChar)(ch, x, y, attr);
    }
#endif
  }
  else
  {
    /*
      Use the boring old BIOS to write
    */

#if defined(__DPMI32__) && !defined(PLTNT)
#if defined(USE_NT_CONSOLE)
    CHAR_INFO ci;
    COORD     coordSize;
    COORD     coordBuf;
    SMALL_RECT r;
    int        iw;

    coordSize.X = coordSize.Y = 1;
    coordBuf.X  = coordBuf.Y  = 0;

    r.Left   = col;
    r.Top    = row;
    r.Right  = col+1;
    r.Bottom = row+1;

    ci.Char.AsciiChar = (cell & 0xFF);
    ci.Attributes     = ((cell >> 8) & 0xFF);

    if (NTConsoleInfo.hStdOutput == NULL)
      NTConsoleInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    for (iw = iWidth >> 1;  iw-- > 0;  r.Left++, r.Right++)
      WriteConsoleOutputA(NTConsoleInfo.hStdOutput, &ci, coordSize, coordBuf, &r);
#else
    VidSetPos(row, col);
    r.AX = (0x09 << 8) | ((BYTE) (cell & 0xFF));     /* character  */
    r.BX = (cell >> 8) & 0x00FF;    /* attr       */
    r.CX = iWidth >> 1;             /* char count */
    RealIntr(0x10, &r);
#endif

#elif defined(__DPMI16__)
    VidSetPos(row, col);
    _AH = 0x09;
    _AL = (BYTE) (cell & 0xFF);     /* character  */
    _BX = (cell >> 8) & 0x00FF;     /* attr       */
    _CX = iWidth >> 1;              /* char count */
    geninterrupt(0x10);
#else
    VidSetPos(row, col);
    r.h.ah = 0x09;
    r.h.al = (BYTE) (cell & 0xFF);  /* character  */
    r.x.bx = (cell >> 8) & 0x00FF;  /* attr       */
    r.x.cx = iWidth >> 1;           /* char count */
    INT86(0x10, &r, &r);
#endif
  }

  /*
    Restore the mouse if we hid it.
  */
  if (bHidMouse)
    MouseShow();
  if (bCaretPushed)
    CaretPop();

  /*
    Make the changes to the virtual screen too!
  */
  lpVirtScr = GETVIRTSCREENBASE();
  lpVirtScr += row*VideoInfo.width + col;
  /*
    lwmemset inverts the cell, so let's trick it by inverting it ourselves.
    This way, we should get the proper ordering of the pair in the 
    virtual screen buffer. The virtual screen buffer expects the character
    first and then the attribute.
  */
#if !defined(__DPMI32__)
  cell = ((cell >> 8) & 0x00FF) | ((cell & 0x00FF) << 8);
#endif
  lwmemset(lpVirtScr, cell, iWidth >> 1);
}


VOID FAR PASCAL 
VGAVioWrtCellStr(LPCELL buf,int iWidth,int row,int col,int iReserved)
{
  BOOL   bHidMouse, bCaretPushed;
  LPCELL lpVirtScr;

  /*
    Hide the mouse if it could interfere with our screen writing
  */
  bHidMouse = MouseHideIfInRange(row - 1, row + 1);
  bCaretPushed = CaretPush(row-1, row+1);

  if (VID_IN_GRAPHICS_MODE())
  {
    LPSTR s;
    int   n;
    int   x = col * FONT_WIDTH;
    int   y = row * VideoInfo.yFontHeight;

    if (!VID_USE_GRAPHICS_BIOS())
    {
      VGAWRTPROC *lpfnVGAWrtGraphicsChar = VioGetDriver();
      for (s = (LPSTR) buf, n=iWidth >> 1;  n-- > 0 && *s;  
           s += 2, x += FONT_WIDTH)
        if (*s != 0xFF)
          (*lpfnVGAWrtGraphicsChar)((CELL) *s, x, y, (CELL) *(s+1));
    }
    else
    {
      /*
        If we're in graphics mode and we are using the BIOS, then make sure
        that the attribute for every cell is 0x07 since the BIOS functions
        can't deal with different attributes.
      */
      for (s = (LPSTR) buf+1, n=iWidth >> 1;  n-- > 0;  s += 2)
        *s = 0x07;
      _VGAVioWrtCellStr(buf, iWidth >> 1, row, col, iReserved);
    }
  }
  else
  {
    /*
      We are in text mode on an EGA/VGA
    */
    _VGAVioWrtCellStr(buf, iWidth >> 1, row, col, iReserved);
  }

  /*
    Show the mouse again if we hid it
  */
  if (bHidMouse)
    MouseShow();
  if (bCaretPushed)
    CaretPop();

  /*
    Copy the line back into the virtual buffer...
    There should be no embedded 0xFF's in the buffer, so we can use
    a fast lmemcpy().
  */
  lpVirtScr = GETVIRTSCREENBASE();
  lpVirtScr += row*VideoInfo.width + col;

#if 1
  /*
    3/24/92 (maa)
      There may be embedded 0xFF's if we are using WAGNER_GRAPHICS
      in wstrdisp.c
  */
  while (iWidth)
  {
    if (*buf != 0xFFFF)
      *lpVirtScr = *buf;
    lpVirtScr++;
    buf++;
    iWidth -= 2;
  }
#else
  lmemcpy((LPSTR) lpVirtScr, (LPSTR) buf, iWidth);
#endif
}


static VGAWRTPROC *PASCAL VioGetDriver(void)
{
  /*
    If we are using the graphics wrapper, use the output routines
    found in the graphics engine.
  */
#if defined(BGI) || defined(GX)
  return BGIWrtGraphicsChar;
#endif

  switch (VideoInfo.starting_mode)
  {
    case 0x06 :
      return WrtGraphicsChar6;
    case 0x13 :
      return VGAMode13WrtGraphicsChar;
    case 0x61 :
    case 0x62 :
      return ATIWrtGraphicsChar256;
    default   :
      return VGAWrtGraphicsChar;
  }
}


#if 0
VOID cdecl FAR VGAWrtGraphicsChar(WORD ch, int x, int y, WORD color)
{
#define seq_out(idx,val)    (outp(0x3C4,idx), outp(0x3C5,val))
#define graph_out(idx,val)  (outp(0x3CE,idx), outp(0x3CF,val))

  int   fg, bg;
  int   xMask, yMask;
  LPSTR lpBitmap;

  fg = color & 0x0F;
  bg = (color >> 4) & 0x0F;

  lpBitmap = * (LPSTR *) (0x43 * 4);
  lpBitmap += ch * VideoInfo.yFontHeight;

  for (yMask = 0;  yMask < VideoInfo.yFontHeight;  yMask++)
  {
    for (xMask = 0;  xMask < 8;  xMask++)
    {
      LPSTR lpScreen = MK_FP(0xA000, ((y+yMask) * 80 + ((x+xMask) >> 3)));
      int clrPixel = (lpBitmap[yMask] & (0x80 >> xMask)) ? fg : bg;
      int mask = 0x80 >> ((x+xMask) % 8);
      int dummy;

      graph_out(8, mask);
      seq_out(2, 0x0F);
      dummy = *lpScreen;
      *lpScreen = 0;
      seq_out(2, clrPixel);
      *lpScreen = 0xFF;
    }
  }

  seq_out(2, 0x0F);
  graph_out(3, 0);
  graph_out(8, 0xFF);
}
#endif


VOID FAR PASCAL VGAMode13WrtGraphicsChar(WORD ch, int x, int y, COLOR color)
{
#if !defined(MEWEL_32BITS)
  int fg, bg;
  int xMask, yMask;
  int pixRows;
  unsigned long ulScreen;
  LPSTR    lpchBitmap;

  pixRows    = * (int far *) MK_FP(0x40, 0x85);
  lpchBitmap = (LPSTR) MK_FP(0x00, (0x43*4));
  lpchBitmap = * (unsigned char far * far *) lpchBitmap;
  lpchBitmap += pixRows * ch;

  fg = color & 0x0F;
  bg = (color >> 4) & 0x0F;

  ulScreen = 0xA0000000L + y*(long)pixRows*320L + (x*8);

  for (yMask = 0;  yMask < pixRows;  yMask++)
  {
    for (xMask = 0;  xMask < 8;  xMask++)
      * (unsigned char far *) (ulScreen+xMask) = 
          (unsigned char) ((lpchBitmap[yMask] & (0x80 >> xMask)) ? fg : bg);
    ulScreen += 320;
  }
#endif
}


/*===========================================================================
| Very very system dependent stuff.... Output routines for ATI VGA
| mode 61H (640x400 256-colors) and 62H (640x480 256-colors).
| This stuff should really be put into separate video drivers.
===========================================================================*/
extern VOID FAR PASCAL ATIPageSelect(WORD);

VOID FAR PASCAL ATIWrtGraphicsChar256(WORD ch, int x, int y, COLOR color)
{
#if !defined(MEWEL_32BITS)
  int  fg, bg;
  int  xMask, yMask;
  WORD pixRows;
  BYTE iPage;
  DWORD ulScreen;
  LPSTR lpchBitmap;
  BOOL  bCheckBoundary;

  static BYTE bCurrPage = 0xFF;

  pixRows    = * (LPWORD) MK_FP(0x40, 0x85);
  lpchBitmap = (LPSTR) MK_FP(0x00, (0x43*4));
  lpchBitmap = * (LPSTR FAR *) lpchBitmap;
  lpchBitmap += pixRows * ch;

  fg = color & 0x0F;
  bg = (color >> 4) & 0x0F;

  y /= VideoInfo.yFontHeight;
  y = y * 16;

  /*
    Pixels to row/col
  */
  if      (y < 102 || y == 102 && x < 256)  iPage = 0;
  else if (y < 204 || y == 204 && x < 512)  iPage = 1;
  else if (y < 307 || y == 307 && x < 128)  iPage = 2;
  else
  {
    if      (VideoInfo.starting_mode == 0x61)  iPage = 3;
    else if (VideoInfo.starting_mode == 0x62)
    {
      if (y < 409 || y == 409 && x < 384)      iPage = 3;
      else                                     iPage = 4;
    }
  }
  if (bCurrPage != iPage)
  {
    ATIPageSelect(iPage);
    bCurrPage = iPage;
  }
  ulScreen  =  0xA0000000L + (y*640L + x) - (iPage * 65536L);

  bCheckBoundary =
    (y >= 102-16 && y+16 <= 102+16) ||
    (y >= 204-16 && y+16 <= 204+16) ||
    (y >= 307-16 && y+16 <= 307+16) ||
    (VideoInfo.starting_mode == 0x62 && y >= 409-16 && y+16 <= 409+16);

  for (yMask = 0;  yMask < 16;  yMask++)
  {
    for (xMask = 0;  xMask < 8;  xMask++)
    {
      if (bCheckBoundary)
        if      (y+yMask  < 102 || y+yMask  == 102 && x+xMask < 256)  iPage = 0;
        else if (y+yMask  < 204 || y+yMask  == 204 && x+xMask < 512)  iPage = 1;
        else if (y+yMask  < 307 || y+yMask  == 307 && x+xMask < 128)  iPage = 2;
        else
        {
          if      (VideoInfo.starting_mode == 0x61)  iPage = 3;
          else if (VideoInfo.starting_mode == 0x62)
          {
            if (y+yMask  < 409 || y+yMask  == 409 && x+xMask < 384)  iPage = 3;
            else                                                     iPage = 4;
          }
        }
      if (bCurrPage != iPage)
      {
        ATIPageSelect(iPage);
        bCurrPage = iPage;
        ulScreen = 0xA0000000L + ((y+yMask)*640L + (x+xMask)) - (iPage*65536L);
      }

      * (unsigned char far *) (ulScreen+xMask) = 
          (unsigned char) ((lpchBitmap[yMask] & (0x80 >> xMask)) ? fg : bg);

    }
    ulScreen += 640;
  }
#endif
}


#if defined(BGI)
void far cdecl outtextxy(int __x, int __y, const char far *__textstring);
void far cdecl setcolor(int __color);

VOID FAR PASCAL BGIWrtGraphicsChar(WORD ch, int x, int y, COLOR color)
{
  int  fg, bg;
  char s[2];
  static int oldFg = -1;

  /*
    Figure out the foreground color
  */
  fg = color & 0x0F;
  (void) bg;

  /*
    Set up a one-character string
  */
  s[0] = ch;  s[1] = '\0';

  /*
    Set the pixel color. To save time, do this only if we are
    using a different foreground color than last time.
  */
  if (oldFg != fg)
  {
    setcolor(fg);
    oldFg = fg;
  }

  /*
    Output the character using the BGI
  */
  outtextxy(x, y, s);
}
#endif /* BGI */


#if defined(GX)
extern int far pascal grMoveTo(int,int);
extern int far pascal grOutText(char far *);
extern int far pascal grSetColor(int);

VOID FAR PASCAL BGIWrtGraphicsChar(WORD ch, int x, int y, COLOR color)
{
  int  fg, bg;
  char s[2];
  static int oldFg = -1;

  /*
    Figure out the foreground color
  */
  fg = color & 0x0F;
  (void) bg;

  /*
    Set up a one-character string
  */
  s[0] = ch;  s[1] = '\0';

  /*
    Set the pixel color. To save time, do this only if we are
    using a different foreground color than last time.
  */
  if (oldFg != fg)
  {
    grSetColor(fg);
    oldFg = fg;
  }

  /*
    Output the character using the BGI
  */
  grMoveTo(x, y);
  grOutText(s);
}
#endif /* GX */


#if defined(MEWEL_32BITS)
VOID PASCAL _VGAVioWrtCellStr(LPCELL lpBuf, WORD iWidth,
                              WORD row, WORD col, WORD iReserved)
{
  return;
}
VOID FAR PASCAL VGAWrtGraphicsChar(WORD cell, int x, int y, WORD attr)
{
  return;
}
VOID FAR PASCAL WrtGraphicsChar6(WORD cell, int x, int y, WORD attr)
{
  return;
}
#endif

#endif /* MEWEL_GUI */

