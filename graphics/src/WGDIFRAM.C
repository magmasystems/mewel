/*===========================================================================*/
/*                                                                           */
/* File    : WGDIFRAM.C                                                      */
/*                                                                           */
/* Purpose : Implements the FrameRect() function for the GUI version of MEWEL*/
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1991-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"
#include "wobject.h"

/****************************************************************************/
/*                                                                          */
/* Function : FrameRect()                                                   */
/*                                                                          */
/* Purpose  : The graphical version of FrameRect()                          */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL FrameRect(hDC, lpRect, hBrush)
  HDC    hDC;
  CONST RECT FAR *lpRect;
  HBRUSH hBrush;
{
#if 122992
  /*
    We define FrameRect() in terms of Rectangle(), since Rectangle does
    all of the hard clipping work. We create a pen which has the same color
    as the passed brush in order to draw the frame, and we use a NULL
    brush so the interior of the rectangle is not touched.
  */

  LPOBJECT lpObj;
  BOOL     rc;
  HPEN     hPen, hOldPen;
  HBRUSH   hOldBrush;

  /*
    Get the brush info.
  */
  if ((lpObj = _ObjectDeref(hBrush)) == NULL)
    return FALSE;

  /*
    Create the solid colored pen to be used for the frame
  */
  hPen = CreatePen(PS_SOLID, 1, lpObj->uObject.uLogBrush.lbColor);

  /*
    Use the pen for the frame and a null brush
  */
  hOldPen   = SelectObject(hDC, hPen);
  hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));

  /*
    Draw the frame
  */
  rc = Rectangle(hDC, lpRect->left,lpRect->top,lpRect->right,lpRect->bottom);

  /*
    Restore the old brush and pen, and delete the newly created pen.
  */
  SelectObject(hDC, hOldBrush);
  SelectObject(hDC, hOldPen);
  DeleteObject(hPen);

  return rc;

#else
  RECT   r;
  LPOBJECT lpObj;
  LPHDC  lphDC;
  COLOR  attr;

  if (!SysGDIInfo.bGraphicsSystemInitialized || (lphDC = _GetDC(hDC)) == NULL)
    return FALSE;

  /*
    Set the vieport and the rop2 code
  */
  if (!_GraphicsSetViewport(hDC))
    return FALSE;
  RealizeROP2(hDC);

  if ((lpObj = _ObjectDeref(hBrush)) == NULL)
    return FALSE;

  /*
    Translate logical coordinates to device coordinates
  */
  r = *lpRect;
  GrLPtoSP(hDC, (LPPOINT) &r, 2);

  /*
    Get the brush color as a BIOS color value
  */
  attr = RGBtoAttr(hDC, lpObj->uObject.uLogBrush.lbColor);
  _setcolor(attr);

  MOUSE_ConditionalOffDC(lphDC, r.left, r.top, r.right, r.bottom);

  /*
    Draw the frame
  */
#if defined(META)
  mwPenDash(0);
  mwFrameRect((rect *) &r);

#elif defined(GX)
  grSetLineStyle(grLSOLID, 1);
  grDrawRect(r.left, r.top, r.right, r.bottom, grOUTLINE);

#elif defined(MSC)
  _setlinestyle(0xFFFF);
  _rectangle(_GBORDER, r.left, r.top, r.right, r.bottom);

#elif defined(BGI)
  setlinestyle(SOLID_LINE, 0, NORM_WIDTH);
  rectangle(r.left, r.top, r.right, r.bottom);

#endif

  MOUSE_ShowCursorDC();
  return TRUE;

#endif /* 123092 */
}

