/*===========================================================================*/
/*                                                                           */
/* File    : WINICON.C                                                       */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#ifndef ZORTECH
extern FARPROC lpfnRestoreRectHook;
#endif


typedef struct icon
{
  /* The following fields are compatible with MS-Windows 2.1 */

  BYTE bFigure;        /* 1 = cursor, 2 = bitmap, 3 = icon    */
  BYTE bIndependent;   /* 0 = dev dependent, 1 = device indep */
  UINT xHotSpot,       /* hotspots - not used for icons */
       yHotSpot;
  UINT cx,             /* width and height of the icon */
       cy;
  UINT widthBytes;     /* number of bytes per row */
  UINT nColorPlanes;

  /* The following are used by the MEWEL and are set at run-time */

  LPCSTR idIcon;       /* the id of the icon */
  HICON hIcon;         /* the handle of the icon */
  struct icon FAR *next;   /* link to the next icon in the list */

  /* The "bitmap" comes next */
  /*
  char szBitmap[1];
  */
} ICON, *PICON, FAR *LPICON;

static LPICON IconList = (LPICON) NULL;
static UINT   TotalIcons = 0;
static BOOL   bSysIconsInstalled = FALSE;

static LPICON PASCAL _FindIcon(HICON);
static VOID   PASCAL _InitSystemIcons(void);


HICON FAR PASCAL LoadIcon(hModule, idIcon)
  HMODULE hModule;
  LPCSTR  idIcon;
{
  LPICON pIcon;
  
  if (!bSysIconsInstalled)
    _InitSystemIcons();

  /*
    See if the icon has already been loaded
  */
#if !defined(MEWEL_32BITS)
  if (FP_SEG(idIcon) == 0)
#endif
    for (pIcon = IconList;  pIcon;  pIcon = pIcon->next)
      if (pIcon->idIcon == idIcon)
        return pIcon->hIcon;

  /*
    Not loaded yet. Try to load it.
  */
  if ((pIcon = (LPICON) GetResource(hModule, (LPSTR)idIcon, RT_ICON)) == NULL)
    return NULLHWND;

  /*
    Successful load. Add it to the head of the list of icons.
  */
  pIcon->next = IconList;
  IconList = pIcon;

  return (pIcon->hIcon = ++TotalIcons);
}


BOOL FAR PASCAL DrawIcon(hDC, x, y, hIcon)
  HDC   hDC;
  int   x, y;
  HICON hIcon;
{
  LPICON pIcon;

  (void) hDC;

  if ((pIcon = _FindIcon(hIcon)) == NULL)
    return FALSE;

#if 51892
  {
  /*
    05/18/92 (maa)
      Use WinPuts() to draw the icons. The icons are not cells.
  */
  int   iRow;
  LPSTR lpData = (LPSTR) ((LPSTR) pIcon + sizeof(ICON));
  LPHDC lphDC  = _GetDC(hDC);
  HWND  hWnd   = (lphDC) ? lphDC->hWnd : 0;

  for (iRow = 0;  iRow < (int) pIcon->cy;  iRow++)
  {
#ifdef MEWEL_GUI
    TextOut(hDC, x, y+(iRow*8), lpData, lstrlen(lpData));
#else
    WinPuts(hWnd, y+iRow, x, lpData, 0x07);
#endif
    lpData += pIcon->cx + 1;
  }
  }

#else
  RECTGEN(r, y, x, y + pIcon->cy - 1, x + pIcon->cx - 1);
#ifndef ZORTECH
#ifndef WAGNER_GRAPHICS
  if (lpfnRestoreRectHook)
  {
    FARPROC lpfn = lpfnRestoreRectHook;
    lpfnRestoreRectHook = (FARPROC) 0;
    WinRestoreRect(NULLHWND, &r, (LPSTR) ((LPSTR) pIcon + sizeof(ICON)));
    lpfnRestoreRectHook = lpfn;
  }
  else
#endif
#endif
    WinRestoreRect(NULLHWND, &r, (LPSTR) ((LPSTR) pIcon + sizeof(ICON)));
#endif

  return TRUE;
}


static LPICON PASCAL _FindIcon(hIcon)
  HICON hIcon;
{
  LPICON p;

  for (p = IconList;  p;  p = p->next)
    if (p->hIcon == hIcon)
      break;
  return p;
}



ICON SystemIcons[] =
{
/* bFig,  bInd,  xHot, yHot, cx, cy, wid, nP, idIcon,            hIcon, next */
{  3,     1,     0,    0,    8,  4,  0,   0,  IDI_APPLICATION,   0,     (LPICON) NULL },
{  3,     1,     0,    0,    8,  4,  0,   0,  IDI_ASTERISK,      0,     (LPICON) NULL },
{  3,     1,     0,    0,    8,  4,  0,   0,  IDI_EXCLAMATION,   0,     (LPICON) NULL },
{  3,     1,     0,    0,    8,  4,  0,   0,  IDI_HAND,          0,     (LPICON) NULL },
{  3,     1,     0,    0,    8,  4,  0,   0,  IDI_QUESTION,      0,     (LPICON) NULL },
};

PSTR SystemIconData[][4]=
{

{
#ifdef ASCII_ONLY
"--------",
"| APPL |",
"| ---- |",
"--------" 
#else
"€ﬂﬂﬂﬂﬂﬂ€",
"€ APPL €",
"€ ---- €",
"€‹‹‹‹‹‹€" 
#endif
},

{
#ifdef ASCII_ONLY
"\\\\ || //",
"--    --",
"--    --",
"// || \\\\"
#else
"ﬂ‹ ﬁ› ‹ﬂ",
"‹‹€€€€‹‹",
"ﬂﬂ€€€€ﬂﬂ",
"‹ﬂ ﬁ› ﬂ‹"
#endif
},

{
#ifdef ASCII_ONLY
"--------",
"|  !!  |",
"|  !!  |",
"--------"
#else
"   ‹‹   ",
"   €€   ",
"   ﬂﬂ   ",
"   ﬂﬂ   "
#endif
},

{
#ifdef ASCII_ONLY
"--------",  
"| WAIT |", 
"| ==== |", 
"--------"
#else
"  ››››  ",
"  €€€›ﬁ ",
"  ﬂ€€ﬂﬂ ",
"   ﬂﬂ   "
#endif
},

{
#ifdef ASCII_ONLY
"--------",
"|  ??  |",
"|  ??  |",
"--------",
#else
"  ‹‹‹‹  ",
" ﬂﬂ ‹€ﬂ ",
"   ﬂﬂ   ",
"   ﬂﬂ   "
#endif
},

};


static VOID PASCAL _InitSystemIcons()
{
  int    i, j;
  LPICON pIcon;
  LPSTR  pBits;

  for (i = 0;  i < sizeof(SystemIcons)/sizeof(ICON);  i++)
  {
#if 51892
    pIcon = (LPICON) emalloc_far((DWORD) sizeof(ICON) + (SystemIcons[i].cx+1)*SystemIcons[i].cy);
#else
    pIcon = (LPICON) emalloc_far((DWORD) sizeof(ICON) + SystemIcons[i].cx*SystemIcons[i].cy*SCREENCELLSIZE);
#endif
    if (pIcon == NULL)
      return;

    lmemcpy((LPSTR) pIcon, (LPSTR) &SystemIcons[i], sizeof(ICON));
    pIcon->next = IconList;
    IconList = pIcon;
    pIcon->hIcon = ++TotalIcons;

    pBits = (LPSTR) ((LPSTR) pIcon + sizeof(ICON));

#if 51892
    for (j = 0;  j < 4;  j++)
    {
      LPSTR s = SystemIconData[i][j];
      lstrcpy(pBits, s);
      pBits += lstrlen(s) + 1;
    }
#else
    for (j = 0;  j < 4;  j++)
    {
      PSTR s = SystemIconData[i][j];
      while (*s)
      {
        *pBits++ = *s++;
        pBits += (SCREENCELLSIZE - 1);
      }
    }
    pBits = (LPSTR) ((LPSTR) pIcon + sizeof(ICON)) + 1;
    for (j = 0;  j < (int) (pIcon->cx * pIcon->cy);  j++)
    {
      *pBits = 0x07;
      pBits += SCREENCELLSIZE;
    }
#endif
  }

  bSysIconsInstalled = TRUE;
}

