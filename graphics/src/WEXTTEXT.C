/*===========================================================================*/
/*                                                                           */
/* File    : WEXTTEXT.C                                                      */
/*                                                                           */
/* Purpose : Implements the ExtTextOut function                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"


BOOL FAR PASCAL ExtTextOut(hDC, X, Y, wOptions, lpRect, pText, cchText, lpDx)
  HDC    hDC;
  INT    X, Y;
  UINT   wOptions;           /* ETO_CLIPPED, ETO_OPAQUE */
  CONST RECT FAR *lpRect;    /* Region to write into. Can be NULL */
  LPCSTR pText;
  UINT   cchText;
  LPINT lpDx;                /* distance between char cells. Not used */
{
#if defined(MEWEL_GUI) && defined(META)
  rect  mwrOldClip;
  grafPort *pPort;
#endif

  LPHDC lphDC;
  RECT  rOldClip;
  BOOL  rc;


  (void) lpDx;

  /*
    Get a pointer to the device context structure
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Save the clipping rectangle of the DC
  */
  rOldClip = lphDC->rClipping;

  if (lpRect)
  {
    RECT r;

    /*
      The passed rectangle is in logical coordinates. We must xform
      it to device coordinates.
    */
    r = *lpRect;
    LPtoSP(hDC, (LPPOINT) &r, 2);

    /*
      Set the new clipping rectangle.
    */
    if (wOptions & ETO_CLIPPED)
    {
      IntersectRect(&lphDC->rClipping, &lphDC->rClipping, (LPRECT) &r);
      r = lphDC->rClipping;   /* 'r' is in screen coords */
#if defined(MEWEL_GUI) && defined(META)
      /*
        We need to save the current MetaWindows clipping rect.
      */
      mwGetPort(&pPort);
      mwrOldClip = pPort->portClip;
#endif
    }


    if (wOptions & ETO_OPAQUE)
    {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
      HBRUSH hBr = CreateSolidBrush(lphDC->clrBackground);
      FillRect(hDC, lpRect, hBr);
      DeleteObject(hBr);
#else
      WinFillRect(lphDC->hWnd, hDC, (LPRECT) &r, ' ', lphDC->attr);
#endif
    }

#if 0
    /*
      Get the clipping rect back to logical coords and perform further
      clipping and alignment on the text string
    */
    {
    POINT pt;
    SPtoLP(hDC, (LPPOINT) &r, 2);
    pt.x = X;   pt.y = Y;
    if (!PtInRect((LPRECT) &r, pt))
      goto bye;
    }
#endif
  }

  rc = TextOut(hDC, X, Y, pText, cchText);

  /*
    Restore the old clipping rectangle, and exit.
  */
bye:
  lphDC->rClipping = rOldClip;

#if defined(MEWEL_GUI)
  /*
    In graphics mode, we need to reset some SysGDIInfo vars and also
    the MetaWindows clipping rect.
  */
  if ((wOptions & ETO_CLIPPED) && lpRect)
  {
    _GraphicsSetViewport(hDC);
#if defined(META)
    mwClipRect(&mwrOldClip);
#endif
  }
#endif

  return rc;
}

