/*===========================================================================*/
/*                                                                           */
/* File    : WGDIRECT.C                                                      */
/*                                                                           */
/* Purpose : Implements the Rectangle, InvertRect, FrameRect, FillRect funcs */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static LPHDC PASCAL PrepareGDIRect(HDC, LPRECT, LPRECT);
static BOOL PASCAL GDIRectFill(HDC, LPRECT, HBRUSH);
static BOOL PASCAL GDIRectFrame(HDC, LPRECT, HPEN, HBRUSH);
static INT  PASCAL WinFrame(HWND, HDC, int, int, int, int, COLOR, int);


/*
  Hook to GUIs and graphics engines
*/
typedef int (FAR PASCAL *FILLRECTPROC)(HDC, LPRECT, HBRUSH);
FILLRECTPROC lpfnFillRectHook = (FILLRECTPROC) 0;

typedef int (FAR PASCAL *FRAMERECTPROC)(HDC, LPRECT, HBRUSH);
FILLRECTPROC lpfnFrameRectHook = (FRAMERECTPROC) 0;

typedef VOID (FAR PASCAL *INVERTRECTPROC)(HDC, LPRECT);
INVERTRECTPROC lpfnInvertRectHook = (INVERTRECTPROC) 0;

typedef BOOL (FAR PASCAL *RECTANGLEPROC)(HDC, INT, INT, INT, INT);
RECTANGLEPROC lpfnRectangleHook = (RECTANGLEPROC) 0;


static LPHDC PASCAL PrepareGDIRect(hDC, lpRect, lprcClipping)
  HDC    hDC;
  LPRECT lpRect;      /* logical coords in, screen coords out */
  LPRECT lprcClipping;
{
  RECT   r;
  LPHDC  lphDC;

  /*
    Check the passed rectangle
  */
  if (!lpRect || IsRectEmpty(lpRect))
    return (LPHDC) NULL;

  /*
    Get a ptr to the DC
  */
  if ((lphDC = _GetDC(hDC)) == NULL)
    return (LPHDC) NULL;

  /*
    Transform the rectangle coordinates to device points
  */
  r = *lpRect;  /* don't destroy the original rect */
  LPtoSP(hDC, (LPPOINT) &r, 2);

#if !40192
  /*
    We do not want to clip this area to the DC's clipping rectangle,
    because we might end up drawing a wrongly dimensioned rectangle.
    For instance, if the DC's clipping region is smaller than the
    desired rectangle, then a frame will be drawn the size of
    the smaller area. In reality, it should be "virtually" drawn
    to the original area, and the low-level MEWEL routines should
    clip it to the DC's clipping region.
  */

  /*
    Intersect this with the DC's clipping rectangle, which should
    already be in device points.
  */
  rClipping = lphDC->rClipping;
  IntersectRect((LPRECT) &r, (LPRECT) &r, (LPRECT) &rClipping);
#endif

  /*
    Return a rectangle to operate on, in device coordinates.
  */
  *lprcClipping = r;
  return lphDC;
}


static BOOL PASCAL GDIRectFill(hDC, lpRect, hBrush)
  HDC    hDC;
  LPRECT lpRect;  /* screen-based coordinates */
  HBRUSH hBrush;
{
  LPHDC lphDC = _GetDC(hDC);

  /*
    Fill the rectangle with the current brush
  */
  if (hBrush && hBrush != GetStockObject(NULL_BRUSH))
  {
    LOGBRUSH lb;
    GetObject(hBrush, sizeof(lb), (LPSTR) &lb);

    if (lb.lbStyle != BS_NULL)
    {
      COLOR  attr, attrWin;
      WINDOW *w;
      int    chFill;

      attr = RGBtoAttr(hDC, lb.lbColor);
      w = WID_TO_WIN(lphDC->hWnd);
      if ((attrWin = w->attr) == SYSTEM_COLOR)
        attrWin = WinGetClassBrush(w->win_id);

      /*
        We should really access a system-dependent table which maps a
        brush style to a fill character.
      */
      if (lb.lbStyle == BS_HATCHED)
        chFill = SysHatchDrawingChars[lb.lbHatch];
      else
        chFill = SysBrushDrawingChars[lb.lbStyle];

      /*
        Here is something strange. If we fill an edit control with
        character 219 (the solid box char), then the cursor
        will not appear when it moves into 'virtual space' (such
        as when it moves to the end of a line). So, force the
        brush to be a blank.
      */
      if (_WinGetLowestClass(w->idClass) == EDIT_CLASS)
        chFill = ' ';

      /*
        There is a littlecomplexity here between the fill char and
        high intensity colors. If we are using the PC's solid box
        (ASCII 219), then use an attribute where the foreground
        color is the brush color and the background color is the
        underlying window color. This way, we can use high-intensity
        colors for the brush, since the foreground color can range
        from colors 0 through 15.
        However, if we are using a blank (ASCII 32) for the brush,
        then we need to set the background attribute to the brush
        color. In this case, we can only use high-intensity colors
        if the user called the VidSetBlinking(FALSE) function.
      */
      if (chFill == 219)
        attr = (attrWin & 0xF0) | (attr & 0x0F);
      else
      {
        attr = (attr << 4) | (attrWin & 0x0F);
        if (!TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS))
          attr &= 0x7F;
      }

      /*
        Account for the abberant case where the attribute is 0xFF. 
        Don't send WinFillRect() an attribute of 0xFF, cause it
        will think that you are sending it SYSTEM_COLOR and it will
        then use the class brush, not this brush.
      */
      if (attr == SYSTEM_COLOR)
        attr &= 0x7F;

      WinFillRect(lphDC->hWnd, hDC, lpRect, chFill, attr);
      return TRUE;
    }
  }

  return FALSE;
}


static BOOL PASCAL GDIRectFrame(hDC, lpRect, hPen, hBrush)
  HDC    hDC;
  LPRECT lpRect;
  HPEN   hPen;
  HBRUSH hBrush;
{
  LPHDC  lphDC;
  LOGBRUSH lb;
  COLOR  attr, attrWin;
  WINDOW *w;
  UINT   chFill;
  RECT   r;

  lphDC = _GetDC(hDC);

  w = WID_TO_WIN(lphDC->hWnd);
  if ((attrWin = w->attr) == SYSTEM_COLOR)
    attrWin = WinGetClassBrush(w->win_id);

  /*
    WinFrame() takes points which are in client coordinates. So, transform
    the points in lpRect to client coordinates.
  */
  r = *lpRect;
  WinScreenRectToClient(lphDC->hWnd, (LPRECT) &r);

  /*
    Frame the rectangle with the current pen
  */
  if (hPen && hPen != NULL_PEN)
  {
    LOGPEN lp;
    GetObject(hPen, sizeof(lp), (LPSTR) &lp);

    if (lp.lopnStyle != PS_NULL)
    {
      attr = RGBtoAttr(hDC, lp.lopnColor);

      /*
        If the DC has a brush associated with it, then we are going
        to be filling the rectangle with that brush. So, combine
        the foreground color of the pen with a background color of the
        brush to create the frame attribute.
        If there is no brush, then use the background color of the
        window as the background color of the frame.
      */
      if (hBrush && hBrush != GetStockObject(NULL_BRUSH))
      {
        GetObject(hBrush, sizeof(lb), (LPSTR) &lb);
        attr = (RGBtoAttr(hDC, lb.lbColor) << 4) | (attr & 0x0F);
      }
      else
        attr = (attrWin & 0xF0) | (attr & 0x0F);

      WinFrame(lphDC->hWnd, hDC,
               r.top,r.left,r.bottom,r.right,
               attr, lp.lopnStyle);
      return TRUE;
    }
  }

  /*
    Frame the rectangle with the passed brush
  */
  if (hBrush && hBrush != GetStockObject(NULL_BRUSH))
  {
    GetObject(hBrush, sizeof(lb), (LPSTR) &lb);

    if (lb.lbStyle != BS_NULL)
    {
      attr = RGBtoAttr(hDC, lb.lbColor);

      if (lb.lbStyle == BS_HATCHED)
        chFill = SysHatchDrawingChars[lb.lbHatch];
      else
        chFill = SysBrushDrawingChars[lb.lbStyle];

      WinFrame(lphDC->hWnd, hDC,
               r.top,r.left,r.bottom,r.right,
               (attr & 0xF0) | (attr & 0x0F), - ((INT) chFill));
      return TRUE;
    }
  }

  return FALSE;
}


/*
  This is like VidFrame() above, but it uses WinPuts to perform
  clipping. Also, the 'type' is a pen style, and the system-dependent
  drawing characters should be gotten from SysPenDrawingChars[].
*/
static INT PASCAL WinFrame(hWnd, hDC, row1, col1, row2, col2, attr, type)
  HWND  hWnd;
  HDC   hDC;
  int   row1, col1, row2, col2;
  COLOR attr;
  int   type;
{
  register int i;
  register int height = row2 - row1 - 2;  /* height minus the corners */
  register int width  = col2 - col1 - 2;  /* width minus the corners  */

  char     szLineBuf[MAXSCREENWIDTH+1];
  PSTR     pchPen = NULL;
  BYTE     chBrush;
  char     s[2];

  if (height < -1 || width < -1)
    return FALSE;

  /*
    If we send in a negative value for 'type', then this is really a
    brush character. Otherwise, 'type' is a pen style.
  */
  if (type >= 0)
    pchPen = &SysPenDrawingChars[type][0];
  else
    chBrush = (BYTE) -type;
    
  /*
    Init the default line drawing string for single-char writes
  */
  s[0] = chBrush;
  s[1] = '\0';

  /*
    Fill in a buffer with the horizontal line
  */
  width = min(width, sizeof(szLineBuf)-1);
  if (width > 0)
  {
    memset(szLineBuf, pchPen ? pchPen[0] : chBrush, width);
    szLineBuf[width] = '\0';
  }
  else
    szLineBuf[0] = '\0';

  /*
    Draw the top
  */
  if (pchPen)
    s[0] = pchPen[2];
  _WinPuts(hWnd, hDC, row1,  col1, s, attr, 1, FALSE);
  if (width > 0)
    _WinPuts(hWnd, hDC, row1, col1+1, szLineBuf, attr, width, FALSE);
  if (pchPen)
    s[0] = pchPen[4];
  _WinPuts(hWnd, hDC, row1,  col1+width+1, s, attr, 1, FALSE);

  /*
    Draw the vertical sides
  */
  if (pchPen)
    s[0] = pchPen[1];
  for (i = height;  i-- > 0;  )
  {
    _WinPuts(hWnd, hDC, ++row1, col1,         s, attr, 1, FALSE);
    _WinPuts(hWnd, hDC, row1,   col1+width+1, s, attr, 1, FALSE);
  }
  
  /*
    Draw the bottom
  */
  if (row2 > row1)
  {
    if (pchPen)
      s[0] = pchPen[3];
    _WinPuts(hWnd, hDC, ++row1,col1, s, attr, 1, FALSE);
    if (width > 0)
      _WinPuts(hWnd, hDC, row1, col1+1, szLineBuf, attr, width, FALSE);
    if (pchPen)
      s[0] = pchPen[5];
    _WinPuts(hWnd, hDC, row1,  col1+width+1, s, attr, 1, FALSE);
  }
  return TRUE;
}



INT FAR PASCAL FillRect(hDC, lpRect, hBrush)
  HDC    hDC;
  CONST RECT FAR *lpRect;
  HBRUSH hBrush;
{
  RECT   rClient;

  /*
    Hook to GUIs
  */
  if (lpfnFillRectHook)
    return (*lpfnFillRectHook)(hDC, (LPRECT) lpRect, hBrush);

  if (!PrepareGDIRect(hDC, (LPRECT) lpRect, (LPRECT) &rClient))
    return FALSE;

  return GDIRectFill(hDC, (LPRECT) &rClient, hBrush);
}


INT FAR PASCAL FrameRect(hDC, lpRect, hBrush)
  HDC    hDC;
  CONST RECT FAR *lpRect;
  HBRUSH hBrush;
{
  RECT     rClient;

  /*
    Hook to GUIs
  */
  if (lpfnFrameRectHook)
    return (*lpfnFrameRectHook)(hDC, (LPRECT) lpRect, hBrush);

  if (!PrepareGDIRect(hDC, (LPRECT) lpRect, (LPRECT) &rClient))
    return FALSE;

  return GDIRectFrame(hDC, (LPRECT) &rClient, (HPEN) 0, hBrush);
}


VOID FAR PASCAL InvertRect(hDC, lpRect)
  HDC    hDC;
  CONST RECT FAR *lpRect;
{
  LPHDC lphDC;
  RECT  rClient;

  /*
    Hook to GUIs
  */
  if (lpfnInvertRectHook)
  {
    (*lpfnInvertRectHook)(hDC, (LPRECT) lpRect);
    return;
  }

  if ((lphDC = PrepareGDIRect(hDC, (LPRECT) lpRect, (LPRECT) &rClient)) == NULL)
    return;

  WinInvertRect(lphDC->hWnd, hDC, &rClient);
}


BOOL FAR PASCAL Rectangle(hDC, x1, y1, x2, y2)
  HDC hDC;
  INT x1, y1, x2, y2;
{
  LPHDC lphDC;
  RECT  rClient, rOrig;

  /*
    Hook to GUIs
  */
  if (lpfnRectangleHook)
    return (*lpfnRectangleHook)(hDC, x1, y1, x2, y2);

  SetRect((LPRECT) &rClient, x1, y1, x2, y2);
  rOrig = rClient;
  if ((lphDC = PrepareGDIRect(hDC, (LPRECT) &rOrig, (LPRECT) &rClient)) == NULL)
    return FALSE;

  if (GDIRectFrame(hDC, (LPRECT) &rClient, lphDC->hPen, lphDC->hBrush))
    InflateRect(&rClient, -1, -1);
  return GDIRectFill(hDC, (LPRECT) &rClient, lphDC->hBrush);
}

