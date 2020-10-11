/*===========================================================================*/
/*                                                                           */
/* File    : WGDISET.C                                                       */
/*                                                                           */
/* Purpose : Implements MS Windows-compatible Object functions               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#ifdef NOGDI
#undef NOGDI
#endif
#define NO_STD_INCLUDES
#define NOKERNEL

#include "wprivate.h"
#include "window.h"


int FAR PASCAL SetROP2(hDC, rop)
  HDC hDC;
  int rop;
{
  int oldROP;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldROP = lphDC->wDrawingMode;
  lphDC->wDrawingMode = rop;
  return oldROP;
}

int FAR PASCAL GetROP2(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  return lphDC->wDrawingMode;
}


int  FAR PASCAL SetBkMode(hDC, iMode)
  HDC hDC;
  int iMode;
{
  int oldBk;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldBk = lphDC->wBackgroundMode;
  lphDC->wBackgroundMode = iMode;
  return oldBk;
}

int  FAR PASCAL GetBkMode(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  return lphDC->wBackgroundMode;
}


int  FAR PASCAL SetMapMode(hDC, iMode)
  HDC hDC;
  int iMode;
{
  int oldMode;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldMode = lphDC->wMappingMode;
  lphDC->wMappingMode = iMode;

  lphDC->ptWindowOrg.x = lphDC->ptWindowOrg.y = 0;
  lphDC->ptViewOrg.x   = lphDC->ptViewOrg.y   = 0;

  switch (iMode)
  {
    case MM_TEXT     :
      lphDC->extWindow.cx  = 1;
      lphDC->extWindow.cy  = 1;
      lphDC->extView.cx    = 1;
      lphDC->extView.cy    = 1;
      break;

    case MM_HIMETRIC :
    case MM_LOMETRIC :
    case MM_ISOTROPIC:
      lphDC->extWindow.cx  = lphDC->devInfo.cxMM * 
                           ((iMode == MM_HIMETRIC) ? 10 : 1);
      lphDC->extWindow.cy  = lphDC->devInfo.cyMM *
                           ((iMode == MM_HIMETRIC) ? 10 : 1);
      lphDC->extView.cx    = lphDC->devInfo.cxPixels / 10;
      lphDC->extView.cy    = lphDC->devInfo.cyPixels / -10;
      break;

    case MM_HIENGLISH :
    case MM_LOENGLISH :
      lphDC->extWindow.cx  = lphDC->devInfo.cxMM * 
                           ((iMode == MM_HIENGLISH) ? 10 : 1);
      lphDC->extWindow.cy  = lphDC->devInfo.cyMM *
                           ((iMode == MM_HIENGLISH) ? 10 : 1);
      lphDC->extView.cx    = (int) (254L  * lphDC->devInfo.cxPixels / 1000L);
      lphDC->extView.cy    = (int) (-254L * lphDC->devInfo.cyPixels / 1000L);
      break;

    case MM_TWIPS     :
      lphDC->extWindow.cx  = (int) (144L * lphDC->devInfo.cxMM / 10L);
      lphDC->extWindow.cy  = (int) (144L * lphDC->devInfo.cyMM / 10L);
      lphDC->extView.cx    = (int) (254L  * lphDC->devInfo.cxPixels / 1000L);
      lphDC->extView.cy    = (int) (-254L * lphDC->devInfo.cyPixels / 1000L);
      break;
  }

  return oldMode;
}

int  FAR PASCAL GetMapMode(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  return lphDC->wMappingMode;
}


UINT FAR PASCAL SetTextAlign(hDC, nAlign)
  HDC hDC;
  UINT nAlign;
{
  int oldMode;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldMode = lphDC->wAlignment;
  lphDC->wAlignment = nAlign;
  return oldMode;
}

UINT FAR PASCAL GetTextAlign(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;
  return lphDC->wAlignment;
}


int FAR PASCAL SetPolyFillMode(hDC, iMode)
  HDC hDC;
  int iMode;
{
  int   oldMode;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldMode = lphDC->wPolyFillMode;
  lphDC->wPolyFillMode = iMode;
  return oldMode;
}

int FAR PASCAL GetPolyFillMode(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;
  return lphDC->wPolyFillMode;
}


int FAR PASCAL SetTextCharacterExtra(hDC, nExtra)
  HDC hDC;
  int nExtra;
{
  int   oldVal;
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;

  oldVal = lphDC->wTextCharExtra;
  lphDC->wTextCharExtra = nExtra;
  return oldVal;
}

int FAR PASCAL GetTextCharacterExtra(hDC)
  HDC hDC;
{
  LPHDC lphDC;

  if ((lphDC = _GetDC(hDC)) == NULL)
    return 0;
  return lphDC->wTextCharExtra;
}

int FAR PASCAL SetTextJustification(hDC, nExtra, cBreak)
  HDC  hDC;
  int  nExtra;
  int  cBreak;
{
  (void) hDC;
  (void) nExtra;
  (void) cBreak;

  return 1;
}

