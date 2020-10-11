/*===========================================================================*/
/*                                                                           */
/* File    : WGDISHAP.C                                                      */
/*                                                                           */
/* Purpose : Stubs for the GDI shape routines                                */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"
#include "wgraphic.h"

#ifndef MEWEL_GUI

BOOL FAR PASCAL Ellipse(hDC, x1, y1, x2, y2)
  HDC hDC;
  int  x1, y1, x2, y2;
{
  return Rectangle(hDC, x1, y1, x2, y2);
}

BOOL FAR PASCAL Arc(hDC, x1, y1, x2, y2, x3, y3, x4, y4)
  HDC hDC;
  int  x1, y1, x2, y2, x3, y3, x4, y4;
{
  (void) hDC;
  (void) x1;  (void) y1;  (void) x2;  (void) y2;  (void) x3;  (void) y3;
  (void) x4;  (void) y4;
  return FALSE;
}

BOOL FAR PASCAL Pie(hDC, x1, y1, x2, y2, x3, y3, x4, y4)
  HDC hDC;
  int  x1, y1, x2, y2, x3, y3, x4, y4;
{
  (void) hDC;
  (void) x1;  (void) y1;  (void) x2;  (void) y2;  (void) x3;  (void) y3;
  (void) x4;  (void) y4;
  return FALSE;
}

BOOL FAR PASCAL Polyline(hDC, lpPoint, nPoints)
  HDC hDC;
  CONST POINT FAR *lpPoint;
  INT nPoints;
{
  (void) hDC;  (void) lpPoint;  (void) nPoints;
  return FALSE;
}

BOOL FAR PASCAL Polygon(hDC, lpPoints, nCount)
  HDC hDC;
  CONST POINT FAR *lpPoints;
  INT nCount;
{
  (void) hDC;  (void) lpPoints;  (void) nCount;
  return FALSE;
}

BOOL FAR PASCAL PolyPolygon(hDC, lppt, lpnPolyCounts, cPolygons)
  HDC              hDC;
  CONST POINT FAR *lppt;
  int         FAR *lpnPolyCounts;
  int              cPolygons;
{
  int i;

  for (i = 0;  i < cPolygons;  i++)
  {
    Polygon(hDC, lppt, lpnPolyCounts[i]);
    lppt += lpnPolyCounts[i];
  }

  return TRUE;
}

DWORD FAR PASCAL SetPixel(hDC, x, y, clr)
  HDC  hDC;
  INT  x, y;
  COLORREF clr;
{
  (void) hDC;
  (void) x;  (void) y;
  (void) clr;
  return 0L;
}

DWORD FAR PASCAL GetPixel(hDC, x, y)
  HDC  hDC;
  INT  x, y;
{
  (void) hDC;
  (void) x;  (void) y;
  return 0L;
}

BOOL FAR PASCAL Chord(hDC, X1, Y1, X2, Y2, X3, Y3, X4, Y4)
  HDC hDC;
  INT X1, Y1, X2, Y2, X3, Y3, X4, Y4;
{
  (void) hDC;
  (void) X1;  (void) Y1;  (void) X2;  (void) Y2;  (void) X3;  (void) Y3;
  (void) X4;  (void) Y4;
  return TRUE;
}

#ifndef MOTIF
BOOL FAR PASCAL RoundRect(hDC, X1, Y1, X2, Y2, X3, Y3)
  HDC hDC;
  INT X1, Y1, X2, Y2, X3, Y3;
{
  (void) X3;  (void) Y3;
  return Rectangle(hDC, X1, Y1, X2, Y2);
}

VOID FAR PASCAL LineDDA(X1, Y1, X2, Y2, lpLineFunc, lpData)
  INT X1, Y1, X2, Y2;
  LINEDDAPROC lpLineFunc;
  LPARAM      lpData;
{
  (void) X1;  (void) Y1;  (void) X2;  (void) Y2;
  (void) lpLineFunc;  (void) lpData;
  return;
}
#endif

BOOL FAR PASCAL ExtFloodFill(hDC, X, Y, crColor, wFillType)
  HDC  hDC;           /* handle to device context               */
  INT  X, Y;          /* logical point where filling begins     */
  COLORREF crColor;   /* color of boundary or area to be filled */
  UINT wFillType;     /* FLOODFILLBORDER or FLOODFILLSURFACE    */
{
  (void) wFillType;
  return FloodFill(hDC, X, Y, crColor);
}

BOOL FAR PASCAL FloodFill(hDC, X, Y, crColor)
  HDC  hDC;           /* handle to device context               */
  INT  X, Y;          /* logical point where filling begins     */
  COLORREF crColor;   /* color of boundary or area to be filled */
{
  return FALSE;
}

#endif

