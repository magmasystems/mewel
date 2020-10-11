/*===========================================================================*/
/*                                                                           */
/* File    : WINBLIT.C                                                       */
/*                                                                           */
/* Purpose : Blitting routines for MEWEL. Fast movement of cells between     */
/*           areas of the screen and between the screen and memory.          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_OS2
#define INCLUDE_CURSES

#include "wprivate.h"
#include "window.h"

/*
  Hook to GUIs and graphics engines
*/
typedef int (FAR PASCAL *BLTPROC)(LPRECT, LPRECT);
BLTPROC lpfnBltRectHook = (BLTPROC) 0;

typedef int (FAR PASCAL *FILLPROC)(HWND, LPRECT, INT, COLOR);
FILLPROC lpfnWinFillRectHook = (FILLPROC) 0;

typedef int (FAR PASCAL *INVPROC)(HWND, LPRECT);
INVPROC lpfnWinInvertRectHook = (INVPROC) 0;

typedef int (FAR PASCAL *COLORPROC)(LPRECT, INT, COLOR);
COLORPROC lpfnColorRectHook = (COLORPROC) 0;

typedef int (FAR PASCAL *RESTOREPROC)(LPRECT, HANDLE);
RESTOREPROC lpfnRestoreRectHook = (RESTOREPROC) 0;

typedef HANDLE (FAR PASCAL *SAVEPROC)(LPRECT);
SAVEPROC lpfnSaveRectHook = (SAVEPROC) 0;


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


#if defined(DOS) && !defined(DOS386) && !defined(WC386) && !defined(__GNUC__) && !defined(__DPMI32__)
#define GETVIRTSCREENBASE()  (MK_FP(VirtualScreen.segVirtScreen, 0))
#else
#define GETVIRTSCREENBASE()  ((LPCELL) VirtualScreen.pVirtScreen)
#endif


static INT PASCAL _WinValidateBlitRect(HWND,HDC,LPRECT,int *,BOOL *, BOOL *);

#define TEST_OVERLAPPED_WINDOWS

/*===========================================================================*/
/*                                                                           */
/*  WinBltRect()                                                             */
/*                                                                           */
/*  Moves the char/attr pairs from one rectangular area on the screen to     */
/*  another area.                                                            */
/*                                                                           */
/*  Called from : WinScrollWindow()                                          */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinBltRect(hWnd, hDC, prDest, prSrc)
  HWND   hWnd;
  HDC    hDC;
  LPRECT prDest, prSrc;
{
  int    width;
  BOOL   bHidMouse, bPushedCaret;
#ifdef MEWEL_TEXT
  int    row, rowDest;
  LPCELL pVirtSrc, pVirtDest;
  BOOL   bDidVirtScreen;
#endif

#ifdef TEST_OVERLAPPED_WINDOWS
#ifdef MEWEL_TEXT
  int    col;
  LPCELL pTmpSrc, pTmpDest;
  LPUINT pVisDest, pTmpVisDest;
  LPUINT pVisSrc, pTmpVisSrc;
  RECT   rCell;
#endif
  RECT   rInvalid;
#endif
  

#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif


  /*
    Validate the blitting rect and do some setup
  */
  if (!_WinValidateBlitRect(hWnd, hDC, prSrc, &width, &bHidMouse, &bPushedCaret))
    return FALSE;

#ifdef TEST_OVERLAPPED_WINDOWS
  SetRectEmpty(&rInvalid);
#endif

  /*
    Hook to other GUIs
  */
  if (lpfnBltRectHook)
  {
#ifdef TEST_OVERLAPPED_WINDOWS
    if (hWnd == InternalSysParams.hWndFocus)
      (*lpfnBltRectHook)(prDest, prSrc);
    else
      UnionRect(&rInvalid, &rInvalid, prDest);
#else
    (*lpfnBltRectHook)(prDest, prSrc);
#endif
    goto showmouse;
  }


#ifdef MEWEL_TEXT

  bDidVirtScreen = FALSE;
  if (!bVirtScreenEnabled)
  {
    VirtualScreenEnable();
    bDidVirtScreen++;
  }

#ifdef TEST_OVERLAPPED_WINDOWS
  pVisDest = WinVisMap + (prDest->top * VideoInfo.width + prDest->left);
  pVisSrc  = WinVisMap + (prSrc->top  * VideoInfo.width + prSrc->left);
#endif

  if (prSrc->top < prDest->top)   /* moving downwards */
  {
    /*
      If we are blitting the physical screen, we must also reflect the
      changes in the virtual screen as well.
    */
    pVirtSrc  = GETVIRTSCREENBASE();
    pVirtSrc += (prSrc->bottom-1)*VideoInfo.width + prSrc->left;
    pVirtDest = GETVIRTSCREENBASE();
    pVirtDest+= (prDest->bottom-1)*VideoInfo.width + prDest->left;

    for (row = prSrc->bottom-1, rowDest = prDest->bottom-1;  
         row >= prSrc->top;  row--)
    {
      /*
        Show the changes in the virtual screen
      */
#ifdef TEST_OVERLAPPED_WINDOWS
      if (hWnd == InternalSysParams.hWndFocus)
        lmemmove((LPSTR) pVirtDest, (LPSTR) pVirtSrc, width * SCREENCELLSIZE);
      else
      {
        /*
          Check window ownership if on background (slower)
        */
        for (pTmpSrc = pVirtSrc, pTmpDest = pVirtDest,
             pTmpVisDest = pVisDest, pTmpVisSrc = pVisSrc, col = 0;
             col < width; 
             col++, pTmpSrc++, pTmpDest++, pTmpVisSrc++)
        {
          /*
            If both destination and source visible, copy it directly
          */
          if (*pTmpVisDest++ == hWnd)
            if (*pTmpVisSrc == hWnd)
            {
              *pTmpDest = *pTmpSrc;
            }
            /*
              Else invalidate rectangle that includes current cell
            */
            else
            {
              rCell.top = rowDest - prDest->top;
              rCell.bottom = rCell.top + 1;
              rCell.left = col;
              rCell.right = rCell.left + 1;
              UnionRect(&rInvalid, &rInvalid, &rCell);
            }
        } /* for */
      } /* else */
#else
      lmemmove((LPSTR) pVirtDest, (LPSTR) pVirtSrc, width * SCREENCELLSIZE);
#endif

      pVirtSrc  -= VideoInfo.width;
      pVirtDest -= VideoInfo.width;
#ifdef TEST_OVERLAPPED_WINDOWS
      pVisSrc   -= VideoInfo.width;
      pVisDest  -= VideoInfo.width;
#endif
      VirtualScreenRowBad(rowDest--);
    }
    VirtualScreenSetBadCols(prDest->left, prDest->left + width - 1);
  }

  else                          /* moving upwards */
  {
    pVirtSrc  = GETVIRTSCREENBASE();
    pVirtSrc += prSrc->top*VideoInfo.width + prSrc->left;
    pVirtDest = GETVIRTSCREENBASE();
    pVirtDest+= prDest->top*VideoInfo.width + prDest->left;

    for (row = prSrc->top, rowDest = prDest->top;
         row < prSrc->bottom;  row++)
    {
      /*
        Show the changes in the virtual screen
      */
#ifdef TEST_OVERLAPPED_WINDOWS
      if (hWnd == InternalSysParams.hWndFocus)
        lmemmove((LPSTR) pVirtDest, (LPSTR) pVirtSrc, width * SCREENCELLSIZE);
      else
      {
        /*
          Check window ownership if on background (slower)
        */
        for (pTmpSrc = pVirtSrc, pTmpDest = pVirtDest,
             pTmpVisDest = pVisDest, pTmpVisSrc = pVisSrc, col = 0;
             col < width;
             col++, pTmpSrc++, pTmpDest++, pTmpVisSrc++)
        {
          /*
            If both destination and source visible, copy it directly
          */
          if (*pTmpVisDest++ == hWnd)
            if (*pTmpVisSrc == hWnd)
            {
              *pTmpDest = *pTmpSrc;
            }
            /*
              Else invalidate rectangle that includes current cell
            */
            else
            {
              rCell.top = rowDest - prDest->top;
              rCell.bottom = rCell.top + 1;
              rCell.left = col;
              rCell.right = rCell.left + 1;
              UnionRect(&rInvalid, &rInvalid, &rCell);
            }
        } /* for */
      } /* else */
#else
      lmemmove((LPSTR) pVirtDest, (LPSTR) pVirtSrc, width * SCREENCELLSIZE);
#endif
      pVirtSrc  += VideoInfo.width;
      pVirtDest += VideoInfo.width;
#ifdef TEST_OVERLAPPED_WINDOWS
      pVisSrc   += VideoInfo.width;
      pVisDest  += VideoInfo.width;
#endif
      VirtualScreenRowBad(rowDest++);
    }
    VirtualScreenSetBadCols(prDest->left, prDest->left + width - 1);
  }

  if (bDidVirtScreen)
    VirtualScreenFlush();

#endif /* MEWEL_TEXT */


showmouse:
#ifdef TEST_OVERLAPPED_WINDOWS
  if (!IsRectEmpty(&rInvalid))
  {
    WinScreenRectToClient(hWnd, &rInvalid);
    InvalidateRect(hWnd, &rInvalid, FALSE);
  }
#endif
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();

  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WFILRECT.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinFillRect(hWnd, hDC, lpRect, ch, attr)
  HWND   hWnd;
  HDC    hDC;
  LPRECT lpRect;  /* screen coordinates of rect to fill */
  INT    ch;
  COLOR  attr;
{
  int    width;
  BOOL   bHidMouse, bPushedCaret;
#ifdef MEWEL_TEXT
  int    row;
  CELL   cell;
  BOOL   bNoClip;
  LPCELL lpVirt;
  LPUINT lpVis;
  BOOL   bDidVirtScreen;
#endif

  
#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif

  /*
    Validate the blitting rect against the screen
  */
  if (!_WinValidateBlitRect(hWnd, hDC, lpRect, &width, &bHidMouse, &bPushedCaret))
    return FALSE;

  /*
    Hook to other GUIs
  */
  if (lpfnWinFillRectHook)
  {
    (*lpfnWinFillRectHook)(hWnd, lpRect, ch, attr);
    goto showmouse;
  }


#ifdef MEWEL_TEXT

 bDidVirtScreen = FALSE;
  /*
    Turn on the virtual screen if we are not using it already. So, we
    will fill the appropriate area in the virtual screen, and then
    blast it to the physical screen. This will prevent CGA snow.
  */
  if (!bVirtScreenEnabled)
  {
    VirtualScreenEnable();
    bDidVirtScreen++;
  }
  lpVirt   = GETVIRTSCREENBASE();
  lpVirt  += lpRect->top*VideoInfo.width + lpRect->left;
  lpVis    = WinVisMap + (lpRect->top * VideoInfo.width + lpRect->left);
  bNoClip  = TEST_PROGRAM_STATE(STATE_DRAWINGSHADOW);


  /*
    Figure out the color to fill the window with.
  */
  if (attr == SYSTEM_COLOR)
    attr = WinGetClassBrush(hWnd);
  if (VID_IN_MONO_MODE() || TEST_PROGRAM_STATE(STATE_FORCE_MONO))
    attr = WinMapAttr(attr);
  else if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
    attr &= 0x7F;  /* get rid of the high bit */
#if defined(DOS) && !defined(OS2) && !defined(DOS386) && !defined(WC386) && !defined(__GNUC__) && !defined(__DPMI32__)
  if (bNoClip)
    cell = (CELL) (ch << 8) | attr;   /* cause lwmemset() flips the bytes */
  else
#endif
#if defined(unix) && !defined(BYTE_SWAP)
    cell = (CELL) (ch << 8) | attr;  /* sun, aix, etc */
#else
    cell = (CELL) (attr << 8) | ch;  /* PCs */
#endif

  for (row = lpRect->top;  row < lpRect->bottom;  row++)
  {
    /*
      Set the area to the char/attr pair
    */
    if (bNoClip)
    {
      /*
        Don't clip to the window when drawing shadows. 
      */
      lwmemset(lpVirt, cell, width);
    }
    else
    {
      /*
        This is slower, but it clips.
      */
      int    col;
      LPCELL lpS;  /* don't destroy the lpScreen and the lpVis vars */
      LPUINT lpV;
      for (lpS = lpVirt, lpV = lpVis, col = width;  col-- > 0;  lpS++)
        if (*lpV++ == hWnd)
          *lpS = cell;
    }

    lpVirt += VideoInfo.width;
    lpVis  += VideoInfo.width;

    if (VirtualScreen.pbRowBad)
      VirtualScreen.pbRowBad[row] = TRUE;
  }
  VirtualScreenSetBadCols(lpRect->left, lpRect->left + width - 1);

  if (bDidVirtScreen)
    VirtualScreenFlush();

#endif /* MEWEL_TEXT */

showmouse:
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* Function: WinInvertRect()                                                 */
/*                                                                           */
/* Purpose : Invert the color attributes of a screen area                    */
/*                                                                           */
/* Called by: InvertRect()                                                   */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinInvertRect(hWnd, hDC, lpRect)
  HWND   hWnd;
  HDC    hDC;
  LPRECT lpRect;
{
  int    width;
  BOOL   bHidMouse, bPushedCaret;
#ifdef MEWEL_TEXT
  int    row, col;
  BYTE   fXORMask;
  LPUINT pVis;
  LPCELL pVirt;
  BOOL   bDidVirtScreen;
#endif

  
#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif

  /*
    Validate the blitting rect against the screen
  */
  if (!_WinValidateBlitRect(hWnd, hDC, lpRect, &width, &bHidMouse, &bPushedCaret))
    return FALSE;

  /*
    Hook to other GUIs
  */
  if (lpfnWinInvertRectHook)
  {
    (*lpfnWinInvertRectHook)(hWnd, lpRect);
    goto showmouse;
  }

#ifdef MEWEL_TEXT

  bDidVirtScreen = FALSE;
  fXORMask = (BYTE) (TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) ? 0xFF : 0x7F);

  if (!bVirtScreenEnabled)
  {
    VirtualScreenEnable();
    bDidVirtScreen++;
  }


  for (row = lpRect->top;  row < lpRect->bottom;  row++)
  {
    /*
      In DOS, use the BIOS if we are in graphics mode and the virtual
      screen is not being used. In OS/2, use the BIOS if the virtual
      screen is not enabled.
    */
#if defined (UNIX) || defined(VAXC)
    BYTE buf[(MAXBUFSIZE+1)*SCREENCELLSIZE];   /* assume the screen is <= 132 columns wide */
    LPSTR lpScreen;

    VioReadCellStr((LPSTR) buf, &width, row, lpRect->left, 0);
    HIGHLITE_ON();
    lpScreen = buf + 1;
    for (col = width/SCREENCELLSIZE;  col-- > 0;  lpScreen += SCREENCELLSIZE)
      *lpScreen ^= fXORMask;
    VioWrtCellStr((LPCELL) buf, width, row, lpRect->left, 0);
    HIGHLITE_OFF();
    continue;
#endif

    /*
      Invert the attr in the area
    */
    pVis = WinVisMap + (row * VideoInfo.width + lpRect->left);
    pVirt = (LPCELL) GETVIRTSCREENBASE();
    pVirt += row*VideoInfo.width + lpRect->left;


    for (col = width;  col-- > 0;  pVirt++)
    {
      if (*pVis++ == hWnd)
        * (LPSTR) (((LPSTR) pVirt) + 1) ^= fXORMask;
    }

    VirtualScreenRowBad(row);
  }
  VirtualScreenSetBadCols(lpRect->left, lpRect->left + width - 1);

  if (bDidVirtScreen)
    VirtualScreenFlush();

#endif /* MEWEL_TEXT */


showmouse:
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WRESRECT.C                                                      */
/*                                                                           */
/* Purpose : Blasts a saved screen image from memory to the screen           */
/*                                                                           */
/*===========================================================================*/
INT FAR PASCAL WinRestoreRect(hWnd, lpRect, hImage)
  HWND   hWnd;
  LPRECT lpRect;
  HANDLE hImage;
{
  int      width;
  BOOL     bHidMouse, bPushedCaret;
  LPIMAGE  lpImg;
#ifdef MEWEL_TEXT
  int      row;
  LPSTR    lpVirtScreen;
  LPSTR    lpBuffer;
  BOOL     bDidVirtScreen;
  BOOL     bHookProc;
#endif


#if defined(MOTIF) || defined(DECWINDOWS)
  return TRUE;
#endif

  
  /*
    Validate the blitting rect against the screen
  */
  if (!_WinValidateBlitRect(hWnd, (HDC) 0, lpRect, &width, &bHidMouse, &bPushedCaret))
    return FALSE;

  if ((lpImg = (LPIMAGE) GlobalLock(hImage)) == NULL)
    return FALSE;

#if defined(MEWEL_GUI)
  (void) lpImg;
  lpfnRestoreRectHook(lpRect, hImage);
  goto showmouse;

#elif defined(WAGNER_GRAPHICS)
  if (lpfnRestoreRectHook)
  {
    lpfnRestoreRectHook(lpRect, hImage);
    /*
      3/25/92 (maa)
        We should still save what our conception of the virtual screen
      is, even in graphics mode. The display will look cleaner if we do this.
    */
    goto showmouse;
  }
  lpBuffer = (LPSTR) lpImg;
  if (lpImg->tag == IMAGE_SIGNATURE)
    lpBuffer += sizeof(IMAGE);

#else
  bHookProc = FALSE;
  lpBuffer = (LPSTR) lpImg;
  if (lpImg->tag == IMAGE_SIGNATURE)
  {
    if (lpfnRestoreRectHook)
    {
      lpfnRestoreRectHook(lpRect, hImage);
      bHookProc = TRUE;
      lpBuffer += sizeof(IMAGE);
    }
    else
      lpBuffer += sizeof(IMAGE);
  }
#endif


#ifdef MEWEL_TEXT

  bDidVirtScreen = FALSE;

  /*
    Since the virtual screen contains an exact replica of the screen image,
    then why not copy the virtual screen into the buffer, not the physical
    screen into the buffer.
  */
  if (!bVirtScreenEnabled && !bHookProc)
  {
    VirtualScreenEnable();
    bDidVirtScreen++;
  }
  lpVirtScreen = (LPSTR) GETVIRTSCREENBASE();
  lpVirtScreen += (lpRect->top * VideoInfo.width * SCREENCELLSIZE) + 
              (lpRect->left * SCREENCELLSIZE);



  for (row = lpRect->top;  row < lpRect->bottom;  row++)
  {
    /*
      Move the data from the buffer to the screen...
    */
#if defined(UNIX) || defined(VAXC) || defined(DOS386) || defined(WC386) || defined(__GNUC__) || defined(__DPMI32__)
    lmemcpy(lpVirtScreen, lpBuffer, (UINT) (width*SCREENCELLSIZE));
#else
    movedata(FP_SEG(lpBuffer), FP_OFF(lpBuffer),
             FP_SEG(lpVirtScreen), FP_OFF(lpVirtScreen), 
             width*SCREENCELLSIZE);
#endif

    lpVirtScreen += VideoInfo.width * SCREENCELLSIZE;
    lpBuffer += width * SCREENCELLSIZE;

    if (!bHookProc)
      VirtualScreenRowBad(row);
  }
  if (!bHookProc)
    VirtualScreenSetBadCols(lpRect->left, lpRect->left + width - 1);

  if (bDidVirtScreen)
    VirtualScreenFlush();

#endif /* MEWEL_TEXT */


showmouse:
  GlobalUnlock(hImage);
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();
  return TRUE;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WSAVRECT.C                                                      */
/*                                                                           */
/* Purpose : Saves a rectangular area of the screen                          */
/*                                                                           */
/*===========================================================================*/
HANDLE FAR PASCAL _WinSaveRect(hWnd, lpRect)
  HWND   hWnd;
  LPRECT lpRect;
{
  int      width;
  BOOL     bHidMouse, bPushedCaret;
  HANDLE   hImage;
#ifdef MEWEL_TEXT
  int      row;
  int      iArea;
  LPIMAGE  sp;
  LPSTR    lpVirtScreen;
  LPSTR    lpBuffer;
#endif


#if defined(MOTIF) || defined(DECWINDOWS)
  return NULL;
#endif

  /*
    Validate the blitting rect against the screen
  */
  if (!_WinValidateBlitRect(hWnd, (HDC) 0, lpRect, &width, &bHidMouse, &bPushedCaret))
    return NULL;

  /*
    If we are in graphics mode using the MEWEL GDI wrapper, then we
    might have a hook function which saves the underlying graphics
    screen.
  */
  if (lpfnSaveRectHook)
  {
    hImage = lpfnSaveRectHook(lpRect);
#if defined(WAGNER_GRAPHICS) || defined(MEWEL_GUI)
    /*
      3/25/92 (maa)
        We should still save what our conception of the virtual screen
      is, even in graphics mode. The display will look cleaner if we do this.
    */
    goto showmouse;
#endif
  }


#ifdef MEWEL_TEXT
  /*
    Since the virtual screen contains an exact replica of the screen image,
    then why not copy the virtual screen into the buffer, not the physical
    screen into the buffer.
  */
  lpVirtScreen = (LPSTR) GETVIRTSCREENBASE();
  lpVirtScreen += (lpRect->top * VideoInfo.width * SCREENCELLSIZE) + 
                  (lpRect->left * SCREENCELLSIZE);


  /*
    Allocate space for the saving of the virtual screen image.
  */
  iArea = (lpRect->right - lpRect->left) * (lpRect->bottom - lpRect->top);
  if ((hImage = GlobalAlloc(GMEM_MOVEABLE, (DWORD) 
                 iArea*SCREENCELLSIZE + sizeof(IMAGE))) == NULL)
    return NULL;
  if ((lpBuffer = GlobalLock(hImage)) == NULL)
    return NULL;

  /*
    Fill in the header of the saved image. 
  */
  sp = (LPIMAGE) lpBuffer;
  sp->tag   = IMAGE_SIGNATURE;
  sp->width = width;
  sp->rows  = lpRect->bottom - lpRect->top;
  lpBuffer += sizeof(IMAGE);


  /*
    Go through all of the rows in the saved area and copy the
    contents of the virtual screen to the save buffer.
  */
  for (row = lpRect->top;  row < lpRect->bottom;  row++)
  {
#if defined(UNIX) || defined(VAXC) || defined(DOS386) || defined(WC386) || defined(__GNUC__) || defined(__DPMI32__)
    lmemcpy(lpBuffer, lpVirtScreen, (UINT) (width*SCREENCELLSIZE));
#else
    movedata(FP_SEG(lpVirtScreen), FP_OFF(lpVirtScreen), 
             FP_SEG(lpBuffer), FP_OFF(lpBuffer),
             width*SCREENCELLSIZE);
#endif

    lpVirtScreen += VideoInfo.width * SCREENCELLSIZE;
    lpBuffer += width * SCREENCELLSIZE;
  }
  GlobalUnlock(hImage);

#endif /* MEWEL_TEXT */


showmouse:
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();
  return hImage;
}


/*===========================================================================*/
/*                                                                           */
/* File    : WCLRRECT.C                                                      */
/*                                                                           */
/* Purpose : Changes the color attribute portion of a character cell to the  */
/*           specified color. Used to produce a shadowing effect on PC's.    */
/*                                                                           */
/*===========================================================================*/
#if defined(MEWEL_TEXT)
INT FAR PASCAL WinColorRect(hWnd, hDC, lpRect, ch, attr)
  HWND   hWnd;
  HDC    hDC;
  LPRECT lpRect;
  INT    ch;      /* not used... */
  COLOR  attr;
{
  PWINDOW w, wParent;
  int    width;
  int    row, col;
  COLOR  attrRev;
  BOOL   bHidMouse, bPushedCaret;
  LPSTR  lpScreen;
  LPCELL lpVirt;
  BOOL   bDidVirtScreen;

  if ((w = WID_TO_WIN(hWnd)) == (WINDOW *) NULL)
    return FALSE;

  bDidVirtScreen = FALSE;

  /*
    Validate the blitting rect against the screen
  */
  if (!_WinValidateBlitRect(NULLHWND, hDC, lpRect, &width, &bHidMouse, &bPushedCaret))
    return FALSE;

  /*
    Clip the shadow to the parent window
  */
  if (!(w->ulStyle & WIN_IS_MENU))
  {
    /*
      Go up the ancestor tree
    */
    for (wParent = w->parent;  wParent;  wParent = wParent->parent)
    {
      /*
        Clip non-menu windows vertically
      */
      if (!(w->ulStyle & WIN_IS_MENU))
      {
        if (wParent->rClient.top >= lpRect->top)
          lpRect->top = wParent->rClient.top;
        if (wParent->rClient.bottom < lpRect->bottom)
          lpRect->bottom = wParent->rClient.bottom;
        if (lpRect->top >= lpRect->bottom)
          return FALSE;
      }

      /*
        Clip the shadow horizontally
      */
      if (wParent->rClient.right < lpRect->right)
      {
        lpRect->right = wParent->rClient.right;
        width = lpRect->right - lpRect->left;
      }
      if (wParent->rClient.left > lpRect->left)
      {
        lpRect->left = wParent->rClient.left;
        width = lpRect->right - lpRect->left;
      }
    }
  }


  /*
    Hook into a GUI?
  */
  if (lpfnColorRectHook)
  {
    (*lpfnColorRectHook)(lpRect, ch, attr);
    goto showmouse;
  }

  /*
    We want to update the virtual screen as well.
  */
  if (!bVirtScreenEnabled)
  {
    VirtualScreenEnable();
    bDidVirtScreen++;
  }
  lpVirt = GETVIRTSCREENBASE();
  lpVirt += lpRect->top*VideoInfo.width + lpRect->left;


  if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
    attr &= 0x7F;  /* get rid of the high bit */


  /*
    Do the coloring....
  */
  attrRev = MAKE_HIGHLITE(attr);
  for (row = lpRect->top;  row < lpRect->bottom;  row++)
  {
    LPSTR lpChar = (LPSTR) lpVirt;
    lpScreen = lpChar+1;

    /*
      On the PC, if we are coloring a screen cell whose character portion
      is ASCII 219 (a solid box), then we must use the reverse of the
      attribute.
    */
    for (col = width;
         col-- > 0; 
         lpScreen += SCREENCELLSIZE, lpChar += SCREENCELLSIZE)
    {
      *lpScreen = (BYTE) ((*lpChar == 219) ? attrRev : attr);
    }
    lpVirt += VideoInfo.width;
    VirtualScreenRowBad(row);
  }
  VirtualScreenSetBadCols(lpRect->left, lpRect->left + width - 1);

  if (bDidVirtScreen)
    VirtualScreenFlush();

showmouse:
  if (bHidMouse)
    MouseShow();
  if (bPushedCaret)
    CaretPop();

  return TRUE;
}
#endif /* MEWEL_TEXT */



static INT PASCAL _WinValidateBlitRect(hWnd,hDC,lpRect,piWidth,pfHidMouse,pfPushedCaret)
  HWND   hWnd;
  HDC    hDC;
  LPRECT lpRect;
  INT   *piWidth;
  BOOL  *pfHidMouse;
  BOOL  *pfPushedCaret;
{
  RECT    rClip;
  PWINDOW w;

  if (pfHidMouse)
    *pfHidMouse = FALSE;
  
  /*
    The rectangle is empty or it starts off the screen?
  */
  if (!IntersectRect(lpRect, lpRect, &SysGDIInfo.rectScreen))
    return FALSE;

  /*
    Clip the blitting rect to the parent, but only if it's not a menu,
    is not a dialog, and is not the listbox of a combo box class.
  */
  if ((w = WID_TO_WIN(hWnd)) != NULL)
  {
    if (!(w->ulStyle & (LBS_IN_COMBOBOX | WIN_IS_DLG | WIN_IS_MENU))
        && !(w->flags & WS_POPUP))
    {
      if (!WinGenAncestorClippingRect(hWnd, hDC, &rClip))
        return FALSE;
      if (!TEST_PROGRAM_STATE(STATE_DRAWINGSHADOW) &&
          !IntersectRect(lpRect, lpRect, &rClip))
        return FALSE;
    }
  }

  /*
    Calculate and return the width of the blitting operation...
  */
  *piWidth = lpRect->right - lpRect->left;

  /*
    Hide the mouse for the blitting operation
  */
  if (!bVirtScreenEnabled && 
#if 12593
      !TEST_PROGRAM_STATE(STATE_MOUSEHIDDEN))
#else
      !TEST_PROGRAM_STATE(STATE_MOUSEHIDDEN | STATE_GRAPHICS_MOUSE))
#endif
  {
    BOOL rc = MouseHideIfInRange(lpRect->top, lpRect->bottom);
    if (pfHidMouse)
      *pfHidMouse = rc;
    *pfPushedCaret = CaretPush(lpRect->top, lpRect->bottom);
  }

  return TRUE;
}


#if defined(OS2) || defined(DOS386) || defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
VOID FAR PASCAL lwmemset(LPCELL lpwDest, CELL cell, UINT len)
{
  while (len--)
    *lpwDest++ = cell;
}
#endif


#if 0

/*
  'C' implementation of lmemmove()
*/

VOID FAR PASCAL lmemmove(LPWORD lpwDest, LPWORD lpwSrc, WORD nBytes)
{
  if (FP_OFF(lpwDest) <= FP_OFF(lpwSrc))  /* dest is to the left */
  {
    lmemcpy(lpwDest, lpwSrc, nBytes);
  }
  else    /* dest is to the right of the source -- take care of overlap */
  {
    nBytes >> 1;  /* get number of words to move */
    lpwDest += nBytes;
    lpwSrc  += nBytes;
    while (nBytes--)
      *--lpwDest = *--lpwSrc;
  }
}

#endif

