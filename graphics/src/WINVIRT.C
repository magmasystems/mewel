/*===========================================================================*/
/*                                                                           */
/* File    : WINVIRT.C                                                       */
/*                                                                           */
/* Purpose : Handles the virtual screen buffer                               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
/* #define PCTEST 1 */
#define INCLUDE_OS2
#define INCLUDE_CURSES

#include "wprivate.h"
#include "window.h"


VIRTUALSCREEN VirtualScreen;
BOOL bVirtScreenEnabled = FALSE;
BOOL bUseVirtualScreen = TRUE;
static BOOL bInitVS = FALSE;
static BOOL bDidInitialFill = FALSE;
#ifdef OS2
extern PCH pchPhysScreen;   /* used in WINVID2.C to point to physical screen */
#endif

#ifdef MINIMIZE_CURSES_REFRESH
extern int need_refresh;
extern int delay_refresh;
#endif



/****************************************************************************/
/*                                                                          */
/* Function : VirtualScreenInit(void)                                       */
/*                                                                          */
/* Purpose  : Initializes the virtual screen buffer. Figures out how big    */
/*            it should be, then allocates the memory for the buffer and    */
/*            for the array of "row-bad" indicators.                        */
/*                                                                          */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VirtualScreenInit()
{
  LPSTR pScr;
#if defined(DOS286X)
  SEL   sel;
#elif defined(ERGO)
  static WORD selVirtScreen = 0;
  WORD  wSize;
#endif


  /*
    Free the old pbRowBad[]
  */
  if (VirtualScreen.pbRowBad)
    MyFree(VirtualScreen.pbRowBad);

  /*
    Free the old virtual screen buffer
  */
#if !defined(OS2)
  if (VirtualScreen.pVirtScreen)
  {
#if defined(__DPMI16__) || defined(__DPMI32__)
    GlobalUnlock(VirtualScreen.segVirtScreen);
    GlobalFree(VirtualScreen.segVirtScreen);
#elif defined(DOS286X)
    DosFreeSeg(VirtualScreen.segVirtScreen);
#elif defined(DOS16M)
    D16MemFree(VirtualScreen.pVirtScreen);
#elif defined(ERGO)
    FreeRealMem(selVirtScreen);
#else
    MyFree_far(VirtualScreen.pVirtScreen);
#endif
  }
#endif

  /*
    Figure out the size of the virtual screen
  */
  VirtualScreen.nRows = VideoInfo.length;
  VirtualScreen.nCols = VideoInfo.width;
  VirtualScreen.nScreenSize = VideoInfo.width*VideoInfo.length*SCREENCELLSIZE;

  /*
    Allocate the array of "row-bad" indicators
  */
  VirtualScreen.pbRowBad = (BOOL *) emalloc(VirtualScreen.nRows*sizeof(BOOL));

  /*
    Put the virtual screen into a state where nothing needs to be refreshed
  */
  VirtualScreen.fRedrawFlags = 0x0000;
  VirtualScreen.minBadCol    = 0x7FFF;
  VirtualScreen.maxBadCol    = 0;

#ifdef DOS
#if defined(__DPMI16__) || defined(__DPMI32__)
  VirtualScreen.segVirtScreen = GlobalAlloc(GPTR, VirtualScreen.nScreenSize);
  VirtualScreen.pVirtScreen = GlobalLock(VirtualScreen.segVirtScreen);
#elif defined(DOS286X)
  DosAllocSeg(VirtualScreen.nScreenSize, &sel, 0);
  VirtualScreen.segVirtScreen = sel;
  VirtualScreen.pVirtScreen = MK_FP(sel, 0);
#elif defined(DOS16M)
  VirtualScreen.pVirtScreen = D16MemAlloc(VirtualScreen.nScreenSize);
  VirtualScreen.segVirtScreen = FP_SEG(VirtualScreen.pVirtScreen);
#elif defined(ERGO)
  AllocRealMem(VirtualScreen.nScreenSize / 16, &VirtualScreen.segVirtScreen,
               &selVirtScreen, &wSize);
  VirtualScreen.pVirtScreen = MK_FP(VirtualScreen.segVirtScreen, 0);

#elif defined(DOS386) || defined(WC386) || defined(__GNUC__)
  VirtualScreen.pVirtScreen= emalloc_far((DWORD) VirtualScreen.nScreenSize);
  VirtualScreen.segVirtScreen = (WORD) VirtualScreen.pVirtScreen;

#else
  /*
    Under standard DOS, we want to get the paragraph-aligned segment of the 
    virtual screen. This allows us to do writes into both the virtual screen
    and the physical screen just by sticking the appropriate segment into
    VirtualScreen.segVirtScreen and then doing a lmemcpy().
    However, emalloc_far will most likely return a non-paragraph-aligned
    far address. So we play a trick to get a paragraph-aligned segment. We
    allocate a paragraph more of data and "move" the segment to the
    point of paragraph alignment.
  */
  VirtualScreen.pVirtScreen = emalloc_far((DWORD) VirtualScreen.nScreenSize+16);
  pScr = (LPSTR) VirtualScreen.pVirtScreen;
  VirtualScreen.segVirtScreen = FP_SEG(pScr) + ((FP_OFF(pScr)>>4) & 0x0FFF) + 1;
#endif

  VirtualScreen.segPhysScreen = Scr_Base;
#endif /* DOS */

#ifdef OS2
  VioGetBuf((PULONG)  &VirtualScreen.pVirtScreen,
            (PUSHORT) &VirtualScreen.nScreenSize, 0);
  VirtualScreen.segPhysScreen = pchPhysScreen;
#endif /* OS2 */

#if defined(UNIX) || defined(VAXC)
  VirtualScreen.pVirtScreen = emalloc(VirtualScreen.nScreenSize);
#endif


  bInitVS = TRUE;

  /*
    Fill the virtual screen with the desktop color
  */
  VirtualScreenFillBackground(WinQuerySysColor(NULLHWND, SYSCLR_BACKGROUND));
  bDidInitialFill++;
}


/****************************************************************************/
/*                                                                          */
/* Function : VirtualScreenFillBackground()                                 */
/*                                                                          */
/* Purpose  :                                                               */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VirtualScreenFillBackground(attr)
  COLOR attr;
{
  CELL cell;

  if (!bVirtScreenEnabled || !bUseVirtualScreen)
    return;

  VirtualScreenBad();

  if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
    attr &= 0x7F;  /* get rid of the high bit */

#ifdef DOS
#if defined(DOS386) || defined(WC386) || defined(__GNUC__) || defined(__DPMI32__)
  cell = (attr << 8) | WinGetSysChar(SYSCHAR_DESKTOP_BACKGROUND);
  lwmemset((LPCELL) VirtualScreen.pVirtScreen, cell, VideoInfo.width * VideoInfo.length);
#else
  cell = (WinGetSysChar(SYSCHAR_DESKTOP_BACKGROUND) << 8) | attr;
  lwmemset((LPCELL) MK_FP(Scr_Base, 0), cell, VideoInfo.width * VideoInfo.length);
#endif
#endif

#ifdef OS2
  cell = (attr << 8) | WinGetSysChar(SYSCHAR_DESKTOP_BACKGROUND);
  lwmemset((LPCELL) VirtualScreen.pVirtScreen, cell, VirtualScreen.nScreenSize >> 1);
#endif

#if defined(UNIX) || defined(VAXC)
  cell = (CELL) ( (attr << 8) | WinGetSysChar(SYSCHAR_DESKTOP_BACKGROUND) );
  lwmemset((LPCELL) VirtualScreen.pVirtScreen, cell, VirtualScreen.nScreenSize / sizeof(CELL));
#if 0	/* avoid annoying screen erase */
  VidClearScreen(attr);
#endif
#endif
}


/****************************************************************************/
/*                                                                          */
/* Function : VirtualScreenBad(void)                                        */
/*                                                                          */
/* Purpose  : Forces the entire virtual screen to be rendered onto the      */
/*            physical screen.                                              */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL VirtualScreenBad()
{
  VirtualScreen.fRedrawFlags |= VS_DRAWALL;
  VirtualScreen.minBadCol     = 0;
  VirtualScreen.maxBadCol     = VideoInfo.width-1;
}

VOID FAR PASCAL VirtualScreenRowBad(row)
  int row;
{
  if (VirtualScreen.pbRowBad && bUseVirtualScreen)
    VirtualScreen.pbRowBad[row] = TRUE;
}

VOID FAR PASCAL VirtualScreenSetBadCols(mincol, maxcol)
  int  mincol, maxcol;
{
  if (bVirtScreenEnabled && bUseVirtualScreen)
  {
    if (mincol < VirtualScreen.minBadCol)
      VirtualScreen.minBadCol = mincol;
    if (maxcol > VirtualScreen.maxBadCol)
      VirtualScreen.maxBadCol = maxcol;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : VirtualScreenEnable()                                         */
/*                                                                          */
/* Purpose  : Copies the contents of the physical screen to our virtual     */
/*            screen buffer.                                                */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
BOOL FAR PASCAL VirtualScreenEnable()
{
  if (!bInitVS)
    VirtualScreenInit();

  if (!bUseVirtualScreen)
    return FALSE;

  VirtualScreen.fRedrawFlags = 0x0000;
  VirtualScreen.minBadCol    = 0x7FFF;
  VirtualScreen.maxBadCol    = 0;
  bVirtScreenEnabled = TRUE;

#ifdef DOS
  Scr_Base = VirtualScreen.segVirtScreen;
#endif

#ifdef OS2
  VioGetBuf((PULONG)  &VirtualScreen.pVirtScreen,
            (PUSHORT) &VirtualScreen.nScreenSize, 0);
  VirtualScreen.segPhysScreen = pchPhysScreen;
  pchPhysScreen = VirtualScreen.pVirtScreen;
#endif

  /*
    Fill the virtual screen with the background color.
  */
  if (!bDidInitialFill)
  {
    BOOL   bHidMouse = FALSE;
    BOOL   bCaretPushed;

    /*
      Hide the mouse and the caret while refreshing
    */
    if (!TEST_PROGRAM_STATE(STATE_MOUSEHIDDEN))
      bHidMouse = MouseHide();
    bCaretPushed = CaretPush(0, VideoInfo.length-1);
  
    VirtualScreenFillBackground(WinQuerySysColor(NULLHWND, SYSCLR_BACKGROUND));
    bDidInitialFill++;

    /*
      Show the mouse and caret
    */
    if (bHidMouse)
      MouseShow();
    if (bCaretPushed)
      CaretPop();
  }

  return TRUE;
}


/****************************************************************************/
/*                                                                          */
/* Function : VirtualScreenFlush()                                          */
/*                                                                          */
/* Purpose  : Copies the contents of the virtual screen to the physical     */
/*            screen.                                                       */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/

#if defined(DOS386) || defined(WC386)
#ifdef lmemcpy
#undef lmemcpy
#endif
#define lmemcpy lmemcpy386
#define lcgacpy lmemcpy386
extern VOID FAR PASCAL lmemcpy386(BYTE TRUEFAR *, LPSTR, UINT);
#endif


VOID FAR PASCAL VirtualScreenFlush()
{
  int   row;
  int   width;
  BOOL *pRowBad;
  LPSTR pVirtScr;
  BOOL  bSnow;
  BOOL  bHidMouse = FALSE;
  BOOL  bCaretPushed;

  BYTE TRUEFAR *pPhysScr;


  if (!bUseVirtualScreen)
    return;

  /*
    Even if the virtual screen flag is not enabled, we should flush
    any pending bad rows. So, do a quick scan to see if any rows
    need to be flushed.
  */
  if (VirtualScreen.minBadCol == 0x7FFF)
  {
    goto bye;
  }
  if (!bVirtScreenEnabled)
  {
    pRowBad = VirtualScreen.pbRowBad;
    for (row = 0;  row < (int) VirtualScreen.nRows;  row++)
      if (*pRowBad++)
        break;
    if (row >= (int) VirtualScreen.nRows)
      return;
  }

  pRowBad = VirtualScreen.pbRowBad;
  width = VirtualScreen.nCols * SCREENCELLSIZE;

  /*
    Hide the mouse and the caret while blasting to the physical screen
  */
  if (!TEST_PROGRAM_STATE(STATE_MOUSEHIDDEN))
    bHidMouse = MouseHide();
  bCaretPushed = CaretPush(0, VideoInfo.length-1);


#ifdef DOS
  Scr_Base = VirtualScreen.segPhysScreen;

#if defined(__DPMI32__)
  pPhysScr = (LPSTR) VirtualScreen.segPhysScreen;
  pVirtScr = (LPSTR) VirtualScreen.segVirtScreen;
#elif defined(DOS386)
  pPhysScr = MK_FP(_x386_zero_base_selector, VirtualScreen.segPhysScreen);
  pVirtScr = (LPSTR) VirtualScreen.segVirtScreen;
#elif defined(__HIGHC__) || (defined(PL386) && defined(__WATCOMC__))
  MK_FARP(pPhysScr, 0x1C, 0);
  pVirtScr = (LPSTR) VirtualScreen.segVirtScreen;
#elif defined(WC386) || defined(PL386)
  pPhysScr = (LPSTR) (VirtualScreen.segPhysScreen << 4);
  pVirtScr = (LPSTR) (VirtualScreen.segVirtScreen << 0);
#elif defined(__GNUC__)
  pPhysScr = (LPSTR) (0xE0000000L | (VirtualScreen.segPhysScreen << 4));
  pVirtScr = (LPSTR) VirtualScreen.segVirtScreen;
#else
  pPhysScr = MK_FP(VirtualScreen.segPhysScreen, 0);
  pVirtScr = MK_FP(VirtualScreen.segVirtScreen, 0);
#endif


  bSnow = ((VideoInfo.flags & 0x0F) == CGA);

  for (row = 0;  row < (int) VirtualScreen.nRows;  row++)
  {
    if ((VirtualScreen.fRedrawFlags & VS_DRAWALL) || *pRowBad++)
    {
#if defined(__DPMI32__)
      VioWrtCellStr((LPCELL) (pVirtScr + (VirtualScreen.minBadCol << 1)),
                    (VirtualScreen.maxBadCol-VirtualScreen.minBadCol+1) << 1,
                    row, VirtualScreen.minBadCol, 0);
#else
      if (VID_IN_GRAPHICS_MODE() || VID_USE_BIOS())
      {
        VioWrtCellStr((LPCELL) (pVirtScr + (VirtualScreen.minBadCol << 1)),
                      (VirtualScreen.maxBadCol-VirtualScreen.minBadCol+1) << 1,
                      row, VirtualScreen.minBadCol, 0);
      }
      else
      {
        /*
          We are in text mode. Write to the video ram directly. 
          (Note : this must be changed for CGA).
        */
        if (bSnow)
          lcgacpy(pPhysScr + (VirtualScreen.minBadCol * sizeof(CELL)),
                  pVirtScr + (VirtualScreen.minBadCol * sizeof(CELL)),
                 (VirtualScreen.maxBadCol - VirtualScreen.minBadCol + 1) << 1);
        else
          lmemcpy(pPhysScr + (VirtualScreen.minBadCol * sizeof(CELL)),
                  pVirtScr + (VirtualScreen.minBadCol * sizeof(CELL)),
                 (VirtualScreen.maxBadCol - VirtualScreen.minBadCol + 1) << 1);
      }
#endif
    }
    pVirtScr += width;
    pPhysScr += width;
  }
#endif /* DOS */

#ifdef OS2
  VioShowBuf(0, VirtualScreen.nScreenSize, 0);
#endif

#if (defined(UNIX) || defined(VAXC)) && !defined(MOTIF) && !defined(DECWINDOWS)
  VirtualScreen.maxBadCol = min(VirtualScreen.maxBadCol, COLS-1);
#ifdef WZV /* ?minBadCol may be bad if maxBadCol == 0? */
  if (VirtualScreen.maxBadCol >= VirtualScreen.minBadCol)
#endif
  for (row = 0;  row < (int) VirtualScreen.nRows;  row++)
  {
    if ((VirtualScreen.fRedrawFlags & VS_DRAWALL) || *pRowBad++)
    {
      LPCELL pv, pEOL;
      CELL   ch;

      pv = (LPCELL) PointToScrAddress(row, VirtualScreen.minBadCol);
      pEOL = pv + (VirtualScreen.maxBadCol - VirtualScreen.minBadCol + 1);
      ch = *pEOL;
      *pEOL = (CELL) '\0';
      VIDmvaddstr(row, VirtualScreen.minBadCol, (LPSTR) pv);
      *pEOL = ch;
    }
  }

#ifdef MINIMIZE_CURSES_REFRESH
  if (delay_refresh)
    need_refresh = 1;
  else
#endif
  refresh();
#if 0
  move(LINES-1, COLS-1);
#endif
#endif

  /*
    Restore the mouse and the caret if we hid them
  */
  if (bHidMouse)
    MouseShow();
  if (bCaretPushed)
    CaretPop();
  WinUpdateCaret();

  /*
    Reset the 'row bad' flags and the column indicators
    We want to make sure that the Scr_Base is set back to the physical
    screen segment in case we got to thhis label by the 'goto' above.
  */
  memset(VirtualScreen.pbRowBad, 0, VirtualScreen.nRows * sizeof(BOOL));
bye:
#if defined(DOS)
  Scr_Base = VirtualScreen.segPhysScreen;
#elif defined(OS2)
  pchPhysScreen = VirtualScreen.segPhysScreen;
#endif
  VirtualScreen.fRedrawFlags = 0x0000;
  VirtualScreen.minBadCol = 0x7FFF;
  VirtualScreen.maxBadCol = 0;
  bVirtScreenEnabled = FALSE;
}


#if defined(UNIX) || defined(VAXC)

LPSTR PASCAL PointToScrAddress(row, col)
  int row, col;
{
  return (LPSTR) 
    &VirtualScreen.pVirtScreen[(row*VideoInfo.width + col) * SCREENCELLSIZE];
}

#endif

