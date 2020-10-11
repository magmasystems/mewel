/*===========================================================================*/
/*                                                                           */
/* File    : WREADINI.C                                                      */
/*                                                                           */
/* Purpose : Reads startup info from the INI file                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1992-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


/*
  This array of strings contains the names of system colors which can be
  found under the [colors].
  The index of the string in the array matches the COLOR_xxx constant.
*/

static PSTR pszSysColors[] =
{
  "Scrollbar",
  "Background",
  "ActiveTitle",
  "InactiveTitle",
  "Menu",
  "Window",
  "WindowFrame",
  "MenuText",
  "WindowText",
  "TitleText",
  "ActiveBorder",
  "InactiveBorder",
  "AppWorkspace",
  "Hilight",
  "HilightText",
  "ButtonFace",
  "ButtonShadow",
  "GrayText",
  "ButtonText",
  "InactiveTitleText",
  "ButtonHilight",
};

#if defined(XWINDOWS)
static PSTR pszDefColors[] =
{
  "192 192 192",
  "192 192 192",
  "0 0 128",
  "255 255 255",
  "255 255 255",
  "192 192 192",
  "0 0 0",
  "0 0 0",
  "0 0 0",
  "255 255 255",
  "192 192 192",
  "192 192 192",
  "255 000 000",
  "0 0 0",
  "255 255 255",
  "192 192 192",
  "128 128 128",
  "192 192 192",
  "0 0 0",
  "0 0 0",
  "255 255 255",
};
#endif


VOID FAR PASCAL WinReadINI(void)
{
  char  szBuf[128];
  DWORD adwColors[32];
  int   aiIndex[32];
  int   i, nColors;

  nColors = 0;
  szBuf[0] = ' ';   /* for the first call to next_token to work */

  for (i = 0;  i < sizeof(pszSysColors)/sizeof(pszSysColors[0]);  i++)
  {
#if defined(XWINDOWS)
    if (GetProfileString("colors", pszSysColors[i], pszDefColors[i], szBuf+1, sizeof(szBuf)-1))
#else
    if (GetProfileString("colors", pszSysColors[i], "", szBuf+1, sizeof(szBuf)-1))
#endif
    {
      int   r, g, b;
      PSTR  pBuf;
      /*
        The string looks like "InactiveText=255 0 128"
                                             R  G  B
      */
      if ((pBuf = next_int_token(szBuf, &r)) != NULL  &&
          (pBuf = next_int_token(pBuf,  &g)) != NULL  &&
          (pBuf = next_int_token(pBuf,  &b)) != NULL)
      {
        aiIndex[nColors]   = i;
        adwColors[nColors] = RGB(r, g, b);
        nColors++;
      }
    }
  }

  SetSysColors(nColors, aiIndex, adwColors);

  /*
    Get the default menu checkmark character
  */
  if ((i = GetProfileInt("fonts", "menu.check", 0)) != 0)
    WinSetSysChar(SYSCHAR_MENUCHECK, i);

  /*
    See if we want the beveled look (default is TRUE)
  */
  if (GetProfileInt("boot", "NoBevels", 0))
    SysGDIInfo.fFlags |= GDISTATE_NO_BEVELS;
}

