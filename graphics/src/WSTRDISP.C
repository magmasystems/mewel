/*===========================================================================*/
/*                                                                           */
/* File    : NEWSTRDP.C                                                      */
/*                                                                           */
/* Purpose : Lowest-level routine for blasting a string to either video RAM  */
/*           or to a shadow buffer.                                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

#if 32492
#define WAGNER_GRAPHICS
#endif

extern WORD ScrBaseOrig;

#define MODE_SNOW          0x0001
#define MODE_TRANSPARENT   0x0002
#define MODE_XOR           0x0004

typedef CELL TRUEFAR *FARCELL;


/*
  Helper routine, implemented in VBTSHLP.ASM.  It takes a detination,
  source, attribute, and count.  It reads count bytes from the source.
  For each byte, if the byte is 0xFF, the word at the destination
  is unaffected.  If the byte is not 0xFF, then it is combined with
  the attribute, and placed as a word at the destination.
*/
typedef void (FAR PASCAL *vbtshlpproc)(LPCELL dest,
                                      LPSTR src,
                                      UINT attr,
                                      UINT len,
                                      UINT wFlags);
typedef void (FAR PASCAL *vbtshlpproc386)(FARCELL dest,
                                      LPSTR src,
                                      UINT attr,
                                      UINT len,
                                      UINT wFlags);
extern void FAR PASCAL vbtshlp(LPCELL dest,
                               LPSTR src,
                               UINT attr,
                               UINT len,
                               UINT wFlags);
extern void FAR PASCAL vbtshlp2(LPCELL dest,
                               LPSTR src,
                               UINT attr,
                               UINT len,
                               UINT wFlags);

#if defined(MEWEL_32BITS)
extern void FAR PASCAL vbtshlp386(FARCELL dest,
                               LPSTR src,
                               UINT attr,
                               UINT len,
                               UINT wFlags);
extern void FAR PASCAL vbtshlp2_386(FARCELL dest,
                               LPSTR src,
                               UINT attr,
                               UINT len,
                               UINT wFlags);
#endif


/*
  This version of VidBlastToScreen() does *not* handle hilite characters.
  Handling them would slow this routine down too much.  Hilites should be
  handled at a higher level than VidBlastToScreen() - WinPuts() has already
  been modified to handle them itself.

  This version of VidBlastToScreen() continues to attempt to handle ClrToEOL.
  This support should definitely be removed, and routines which expect
  VidBlastToScreen() to handle ClrToEOL should be modified to do it
  themselves.

  This version of VidBlastToScreen() has not been modified for DOS-Extenders.

  This routine makes an assumption that no more than 160 characters will be
  written.  This assumption should be moved into a #define in wprivate.h,
  and WinPuts() should be changed to use the same define.

  Various assumptions about screen modes and graphics support are wrong,
  throughout MEWEL.   Some super-VGA display adapters support hi-res text
  modes with mode numbers above the normally defined ones - a mode number
  greater than 3 does *not*  imply that the adapter is in graphics mode.
*/

VOID FAR PASCAL VidBlastToScreen(HDC hDC, int row, int col, int maxcol, 
                                 COLOR attr, LPSTR string)
{
  CELL   szVirtualLine[161];
  CELL   cell;
  BYTE   c;
  LPCELL pVirtLine;
  LPCELL pScreen;
  BOOL   bHidMouse      = FALSE;
  BOOL   bPushedCaret   = FALSE;
  BOOL   bSnow          = FALSE;
  BOOL   bIsTransparent = FALSE;
  BOOL   bIsXOR         = FALSE;
  int    origCol = col, origCol2 = col, strLen;

  vbtshlpproc vproc;


  /*
    See if we have a NULL string, or if we are writing outside of the screen.
  */
  if (!string || row < 0 || col > maxcol ||
       row >= (int) VideoInfo.length || col >= (int) VideoInfo.width)
    return;


  /*
    Find the length of the string or the right border, whichever comes first
  */
  strLen = lmemscan(string, '\0', maxcol-col+1);

  /*
    Adjust the color for a mono system if we forgot to map the attributes.
  */
  if (VID_IN_MONO_MODE() || TEST_PROGRAM_STATE(STATE_FORCE_MONO))
    attr = WinMapAttr(attr);
  else if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
    attr &= 0x7F;  /* get rid of the high bit */


  /*
    See if we are overwriting the mouse cursor. If so, and we are not
    writing to the virtual screen, hide the mouse.
  */
  if (!TEST_PROGRAM_STATE(STATE_MOUSEHIDDEN) &&
/*    !TEST_PROGRAM_STATE(STATE_GRAPHICS_MOUSE) && */
      !VID_IN_GRAPHICS_MODE() && !VID_USE_BIOS() &&
#if defined(DOS286X) || defined(DOS16M) || defined(ERGO) || defined(__DPMI16__) || defined(__DPMI32__)
      !bVirtScreenEnabled)
#else
       Scr_Base == ScrBaseOrig)        /* for Desqview compatibility */
#endif
  {
    bHidMouse = MouseHideIfInRange(row, row);
    bPushedCaret = CaretPush(row, row);
  }

  /*
    Clip maxcol to the edge of the screen
  */
  if (maxcol >= (int) VideoInfo.width)
    maxcol = VideoInfo.width - 1;

  /*
    If we were passed a device context, then figure out some stuff about
    rendering the text. We want to deal with transparency vs opaqueness,
    different raster-ops, etc.
  */
  if (hDC)
  {
    LPHDC lphDC = _GetDC(hDC);
    bIsTransparent = (lphDC->wBackgroundMode==TRANSPARENT)?MODE_TRANSPARENT:0;
    bIsXOR = (lphDC->wDrawingMode != R2_COPYPEN) ? MODE_XOR : 0;
  }

  /*
    See if we are in graphics mode or text mode
  */
  /*
    Assume we are not in graphics mode.  Actually detecting graphics mode is
    tricky - we can't just examine the mode, since super-VGA display adapters
    provide new text modes which have their own mode numbers.
  */
  if (VID_IN_GRAPHICS_MODE() || VID_USE_BIOS())
  {
    /*
      Transfer the string and attribute to the virtual line
    */
    for (pVirtLine=szVirtualLine; (c = *string++) != '\0' && col<=maxcol; col++)
    {
      if (c == 0xFF)
      {
        /*
          If we have the indicator that we should skip over this column, we
          read what cell is on the screen at the current position and use that
          cell.
        */
#if defined(WAGNER_GRAPHICS)
        cell = 0xFFFF;
#elif defined(MEWEL_32BITS)
        cell = * (LPCELL) (VirtualScreen.segVirtScreen + ((row*VideoInfo.width+col) << 1));
#else
        cell = * (LPCELL) MK_FP(VirtualScreen.segVirtScreen, ((row*VideoInfo.width+col) << 1));
#endif
      }
      else if (bIsTransparent)
      {
#if defined(DOS386) 
        cell = * (LPCELL) (VirtualScreen.segVirtScreen + ((row*VideoInfo.width+col) << 1));
#elif defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
        cell = * (LPCELL) ((VirtualScreen.segVirtScreen << 0) + ((row*VideoInfo.width+col) << 1));
#else
        cell = * (LPCELL) MK_FP(VirtualScreen.segVirtScreen, ((row*VideoInfo.width+col) << 1));
#endif
        cell = (cell & 0xFF00) | c;
      }
      else
      {
        cell = (attr << 8) | c;
      }

      /*
        If we are trying to write before the left side of the window, then
        don't actually stuff a char in the output buffer.
      */
      if (++origCol2 <= 0)
        continue;

      *pVirtLine++ = cell;
    }


#if defined(DOS286X) || defined(DOS16M) || defined(ERGO) || defined(__DPMI16__) || defined(__DPMI32__)
    if (!bVirtScreenEnabled)
#else
    if (Scr_Base == ScrBaseOrig)       /* for Desqview compatibility */
#endif
    {
      VioWrtCellStr((LPCELL) szVirtualLine, 
                    (pVirtLine - (LPCELL) szVirtualLine) << 1,
                    row, origCol, 0);
#if defined(__DPMI32__)
      goto write_to_virtual_screen; /* we also need a copy in the virt screen */
#endif
    }
    else
    {
      /*
        Not writing to the screen just yet. Transfer the cell-string
        into the virtual screen buffer.
      */
      LPCELL pVirtScr;
      int i, _col, nCols;

write_to_virtual_screen:
      /*
        Get a pointer to the starting destination in the virtual screen buffer
      */
#if defined(DOS386)
      pVirtScr = (LPCELL) VirtualScreen.segVirtScreen;
#elif defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
      pVirtScr = (LPCELL) VirtualScreen.segVirtScreen;
#else
      pVirtScr = (LPCELL) MK_FP(VirtualScreen.segVirtScreen, 0);
#endif
      pVirtScr += row*VideoInfo.width + origCol;

      nCols = pVirtLine - (LPCELL) szVirtualLine;

      /*
        Go through the virtual line and transfer any cells which are
        not 0xFFFF
      */
#ifdef WAGNER_GRAPHICS
      for (i = 0, _col = origCol;  _col < origCol + nCols;  _col++)
      {
        if ((cell = szVirtualLine[i++]) != 0xFFFF)
          *pVirtScr = cell;
        pVirtScr++;
      }
#else
      lmemcpy(pVirtScr, szVirtualLine, nCols * sizeof(CELL));
      *pVirtScr++ = szVirtualLine[_col - origCol];
#endif
    }
  }


  /*
    Not in graphics nor using the BIOS
  */
  else   /* direct write to the video buffer */
  {
    /*
      Get a pointer to the place in the physical or virtual screen to start
      writing to.
    */
  #if defined(DOS386) || defined(__HIGHC__) || defined(__DPMI32__) || (defined(PL386) && defined(__WATCOMC__))
    pScreen = (LPCELL) (Scr_Base + ((row * VideoInfo.width + origCol) << 1));
  #elif defined(WC386)
    pScreen = (LPCELL) ((Scr_Base << 4) + ((row * VideoInfo.width + origCol) << 1));
  #elif defined(__GNUC__)
    pScreen = (LPCELL) ((Scr_Base << 4) + 0xE0000000 + ((row * VideoInfo.width + origCol) << 1));
  #else
    pScreen = (LPCELL) MK_FP(Scr_Base, ((row * VideoInfo.width + origCol) << 1));
  #endif
  
    /*
      Do a snow check if we are writing to the physical screen on a CGA
    */
    bSnow = (VirtualScreen.segVirtScreen != Scr_Base && 
             (VideoInfo.flags & 0x0F) == CGA);

    vproc = (bIsXOR || bIsTransparent) ? vbtshlp2 : vbtshlp;

#if defined(MEWEL_32BITS)
    /*
      Write directly to the physical screen?
    */
    if (Scr_Base == ScrBaseOrig)
    {
      vbtshlpproc386 vproc386;
#if defined(__HIGHC__) || (defined(PL386) && defined(__WATCOMC__))
      FARCELL lpScreen;
      MK_FARP(lpScreen, 0x1C, ((row * VideoInfo.width + origCol) << 1));
#elif defined(WC386) || defined(PL386) || defined(__DPMI32__)
      LPCELL lpScreen = (LPCELL) ((Scr_Base << 4) +
                      ((row * VideoInfo.width + origCol) << 1));
#elif defined(__GNUC__)
      LPCELL lpScreen = (LPCELL) ((Scr_Base << 4) + 0xE0000000 +
                      ((row * VideoInfo.width + origCol) << 1));
#else
      FARCELL lpScreen = MK_FP(_x386_zero_base_selector,
                      Scr_Base + ((row * VideoInfo.width + origCol) << 1));
#endif
      vproc386 = (bIsXOR || bIsTransparent) ? vbtshlp2_386 : vbtshlp386;
      (*vproc386)(lpScreen, (LPSTR) string, attr, min(strLen, maxcol-col+1),
                  bSnow | bIsTransparent | bIsXOR);
    }
    /*
      Also update the virtual screen with the image
    */
    pScreen = (LPCELL) (VirtualScreen.pVirtScreen +
                           ((row * VideoInfo.width + origCol) << 1));
    (*vproc)(pScreen, (LPSTR) string, attr, min(strLen, maxcol-col+1),
             bSnow | bIsTransparent | bIsXOR);
#else
    (*vproc)(pScreen, (LPSTR) string, attr, min(strLen, maxcol-col+1),
             bSnow | bIsTransparent | bIsXOR);
#endif

    /*
      Since we always keep a mirror image of the screen in the virtual
      screen buffer, we need to write this line back to the virtual
      screen buffer if the virtual screen is not currently enabled.
    */
#if !defined(MEWEL_32BITS)
    if (VirtualScreen.segVirtScreen != Scr_Base)
    {
      LPCELL pVirtScr;
      pVirtScr = (LPCELL) MK_FP(VirtualScreen.segVirtScreen, 0);
      pVirtScr += row*VideoInfo.width + origCol;
      (*vproc)(pVirtScr, (LPSTR) string, attr, min(strLen,maxcol-col+1), 
               bIsTransparent | bIsXOR);
    }
#endif
  }

  if (bVirtScreenEnabled && bUseVirtualScreen)
  {
    /*
      Try speeding up things by putting these in-line
    */
    VirtualScreen.pbRowBad[row] = TRUE;
    if (origCol < VirtualScreen.minBadCol)
      VirtualScreen.minBadCol = origCol;
    if (maxcol > VirtualScreen.maxBadCol)
      VirtualScreen.maxBadCol = maxcol;
  }

  /*
    Restore the mouse cursor
  */
  if (bHidMouse)
    MouseShow ();
  if (bPushedCaret)
    CaretPop();
}


/*
  This is what vbtshlp() looks like in C. We can add stuff to this to
  make it handle things like transparency, etc.
*/
VOID FAR PASCAL vbtshlp2(LPCELL lpDest, LPSTR lpSrc, COLOR attr, UINT nBytes, 
                         UINT wFlags)
{
  BYTE ch;

  attr <<= 8;  /* get the attr into the high-order byte */

  while (nBytes-- > 0)
  {
    if ((ch = *lpSrc++) != 0xFF)             /* ignore all 0xFF chars */
    {
      if (wFlags & MODE_TRANSPARENT)
        *lpDest = (*lpDest & 0xFF00) | ch;  /* use existing attr */
      else if (wFlags & MODE_XOR)
      {
        *lpDest ^= 0xFF00;                  /* xor the attr */
        if ((*lpDest & 0x8000) &&           /* get rid of blinking */
            !TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
          *lpDest &= 0x7FFF;
      }
      else
        *lpDest = attr | ch;
    }
    lpDest++;
  }
}



#if defined(MEWEL_32BITS)
VOID FAR PASCAL vbtshlp(LPCELL lpDest, LPSTR lpSrc, COLOR attr, UINT nBytes, 
                         UINT wFlags)
{
  BYTE ch;

  (void) wFlags;

  attr <<= 8;  /* get the attr into the high-order byte */

  while (nBytes-- > 0)
  {
    if ((ch = *lpSrc++) != 0xFF)             /* ignore all 0xFF chars */
      *lpDest = attr | ch;
    lpDest++;
  }
}

VOID FAR PASCAL vbtshlp386(FARCELL lpDest, LPSTR lpSrc, COLOR attr, UINT nBytes, 
                         UINT wFlags)
{
  BYTE ch;

  (void) wFlags;

  attr <<= 8;  /* get the attr into the high-order byte */

  while (nBytes-- > 0)
  {
    if ((ch = *lpSrc++) != 0xFF)             /* ignore all 0xFF chars */
      *lpDest = attr | ch;
    lpDest++;
  }
}


VOID FAR PASCAL vbtshlp2_386(FARCELL lpDest, LPSTR lpSrc, COLOR attr, UINT nBytes, 
                         UINT wFlags)
{
  BYTE ch;

  attr <<= 8;  /* get the attr into the high-order byte */

  while (nBytes-- > 0)
  {
    if ((ch = *lpSrc++) != 0xFF)             /* ignore all 0xFF chars */
    {
      if (wFlags & MODE_TRANSPARENT)
        *lpDest = (*lpDest & 0xFF00) | ch;  /* use existing attr */
      else if (wFlags & MODE_XOR)
      {
        *lpDest ^= 0xFF00;                  /* xor the attr */
        if ((*lpDest & 0x8000) &&           /* get rid of blinking */
            !TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
          *lpDest &= 0x7FFF;
      }
      else
        *lpDest = attr | ch;
    }
    lpDest++;
  }
}

#endif
