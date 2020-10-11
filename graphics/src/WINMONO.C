/*===========================================================================*/
/*                                                                           */
/* File    : WINMONO.C                                                       */
/*                                                                           */
/* Purpose : Defines the color-to-mono color mapping for MEWEL               */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

BYTE bUseMonoMap = 0;

BYTE MonoMap[16][2] =
{
  /* FOREGROUND */         /* BACKGROUND */
      { 0,                       0  },                /* BLACK */
      { 0,                       0  },                /* BLUE  */
      { 7,                       7  },                /* GREEN */
      { 7,                       7  },                /* CYAN  */
      { 0,                       0  },                /* RED   */
      { 0,                       0  },                /* MAGENTA */
      { 7,                       7  },                /* YELLOW */
      { 7,                       7  },                /* WHITE */
       
      { 0,                       0  },                /* BLACK */
      { 0,                       0  },                /* BLUE  */
      { 7,                       7  },                /* GREEN */
      { 7,                       7  },                /* CYAN  */
      { 0,                       0  },                /* RED   */
      { 0,                       0  },                /* MAGENTA */
      { 7,                       7  },                /* YELLOW */
      { 7,                       7  }                 /* WHITE */
};


COLOR FAR PASCAL WinMapAttr(attr)
  COLOR attr;
{
  int fg, bg;

  if (bUseSysColors && !bUseMonoMap)
     return attr;

  fg = MonoMap[attr & 0x0F][0];
  bg = MonoMap[(attr >> 4) & 0x0F][1];
  if (fg == bg)
    bg ^= 0x07;

  return (COLOR) (attr == SYSTEM_COLOR) ? attr : (fg | (bg << 4));
}

VOID FAR PASCAL WinSetMonoMapColor(idxColor, bForeground, inewColor)
  INT idxColor;
  INT bForeground;
  COLOR inewColor;
{
  if (idxColor >= 0 && idxColor < 16 &&
      (inewColor == 0 || inewColor == 7 || inewColor == 8 || inewColor == 15))
    MonoMap[idxColor][bForeground ? 0 : 1] = (BYTE) inewColor;
}

/*
  WinUseMonoMap()
    Simply sets the mono mapping flag to TRUE or FALSE.
*/
VOID FAR PASCAL WinUseMonoMap(bUseit)
  INT bUseit;
{
  bUseMonoMap = (BYTE) bUseit;
}

