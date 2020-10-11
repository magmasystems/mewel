/*===========================================================================*/
/*                                                                           */
/* File    : WGDIDDA.C                                                       */
/*                                                                           */
/* Purpose : Implements the LineDDA function                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define STRICT

#include "wprivate.h"
#include "window.h"

#define ABS(a)   ((a < 0) ? -a : a)
#define SGN(a)   ((a < 0) ? -1 : 1)


VOID WINAPI LineDDA(x1, y1, x2, y2, lpfnDDA, lParam)
  INT         x1, y1, x2, y2;
  LINEDDAPROC lpfnDDA;
  LPARAM      lParam;
{
  INT d, x, y, ax, ay, sx, sy, dx, dy;

  dx = x2 - x1;
  ax = ABS(dx) << 1;
  sx = SGN(dx);
  dy = y2 - y1;
  ay = ABS(dy) << 1;
  sy = SGN(dy);

  x = x1;
  y = y1;

  if (ax > ay)
  {
    d = ay - (ax >> 1);
    while (x != x2)
    {
      (*lpfnDDA)(x, y, lParam);
      if (d >= 0)
      {
	y += sy;
	d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {
    d = ax - (ay >> 1);
    while (y != y2)
    {
      (*lpfnDDA)(x, y, lParam);
      if (d >= 0)
      {
	x += sx;
	d -= ay;
      }
      y += sy;
      d += ax;
    }
  }
}

