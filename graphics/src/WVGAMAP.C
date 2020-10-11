/*===========================================================================*/
/*                                                                           */
/* File    : WVGAMAP.C                                                       */
/*                                                                           */
/* Purpose : VGA/EGA font remapping                                          */
/*                                                                           */
/* History :                                                                 */
/*           7/8/93 (maa) Put in #ifdef TELEVOICE for the "wider" look       */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if 0
#define TEST
#endif

#ifdef TEST
#undef IsVGA
#undef IsEGA
#define IsVGA()   (1)
#define IsEGA()   (1)
WINSYSPARAMS InternalSysParams;
#endif

extern VOID FAR PASCAL VioMapCharBitmap(UINT, UINT, LPSTR);
extern BOOL FAR PASCAL VidInitVGAFonts(void);
extern VOID CDECL VidRestoreVGAFonts(void);
extern VOID FAR PASCAL GetCharBitmap(int ch, BYTE *pBitmap);
extern VOID FAR PASCAL SetCharBitmap(int ch, BYTE *pBitmap);
extern VOID FAR PASCAL CharacterGenSetMode(void);
extern VOID FAR PASCAL CharacterGenClearMode(void);


/*
  Define the constant TELEVOICE if you want a wider checkbox and radiobutton
*/
#define TELEVOICE


typedef struct tagRemapCharInfo
{
  UINT chCharToMap;
  UINT chCharToSave;
  int  idxSysChar;
  BYTE chBitmap16[16];
  BYTE chBitmap8[8];
  BYTE chSavedBitmap16[16];
} REMAPCHARINFO;

REMAPCHARINFO _RemapCharInfo[] =
{
#if 1
  { /* system menu */
    240,
    240,
    SYSCHAR_SYSMENU,
    {
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */ 
      0x7E,/* ".######.", */ 
      0x42,/* ".#....#.", */
      0x42,/* ".#....#.", */
      0x42,/* ".#....#.", */
      0x42,/* ".#....#.", */
      0x7E,/* ".######.", */ 
      0x00,/* "........", */ 
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........"  */
    },
    {
      0x00,/* "........", */
      0x00,/* "........", */
      0xFF,/* "########", */
      0x81,/* "#......#", */
      0x81,/* "#......#", */
      0xFF,/* "########", */
      0x00,/* "........", */
      0x00,/* "........", */
    }
  },
#endif

  { /* radio button left side */
    181,
    181,
    SYSCHAR_RADIOBUTTON_LBORDER,
    {
#if 0
      0x00,/* "........", */
      0x01,/* ".......#", */
      0x02,/* "......#.", */
      0x04,/* ".....#..", */
      0x04,/* ".....#..", */
      0x08,/* "....#...", */
      0x08,/* "....#...", */
      0x08,/* "....#...", */
      0x08,/* "....#...", */
      0x04,/* ".....#..", */
      0x04,/* ".....#..", */
      0x02,/* "......#.", */
      0x01,/* ".......#", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........"  */
#else
      0x00,/* "........", */
      0x00,/* "........", */
      0x01,/* ".......#", */
      0x02,/* "......#.", */
      0x02,/* "......#.", */
      0x04,/* ".....#..", */
      0x04,/* ".....#..", */
      0x04,/* ".....#..", */
      0x04,/* ".....#..", */
      0x02,/* "......#.", */
      0x02,/* "......#.", */
      0x01,/* ".......#", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */
      0x00,/* "........", */
#endif
    },
    {
      0x01,/* ".......#", */
      0x02,/* "......#.", */
      0x04,/* ".....#..", */
      0x08,/* "....#...", */
      0x08,/* "....#...", */
      0x04,/* ".....#..", */
      0x02,/* "......#.", */
      0x01,/* ".......#", */
    }
  },

  { /* radio button right side */
    182,
    182,
    SYSCHAR_RADIOBUTTON_RBORDER,
   {
#if 0
     0x00,/* "........", */
     0x80,/* "#.......", */
     0x40,/* ".#......", */
     0x20,/* "..#.....", */
     0x20,/* "..#.....", */
     0x10,/* "...#....", */
     0x10,/* "...#....", */
     0x10,/* "...#....", */
     0x10,/* "...#....", */
     0x20,/* "..#.....", */
     0x20,/* "..#.....", */
     0x40,/* ".#......", */
     0x80,/* "#.......", */
     0x00,/* "........", */
     0x00,/* "........", */
     0x00,/* "........"  */
#else
     0x00,/* "........", */
     0x00,/* "........", */
     0x00,/* "........", */
     0x80,/* "#.......", */
     0x80,/* "#.......", */
     0x40,/* ".#......", */
     0x40,/* ".#......", */
     0x40,/* ".#......", */
     0x40,/* ".#......", */
     0x80,/* "#.......", */
     0x80,/* "#.......", */
     0x00,/* "........", */
     0x00,/* "........", */
     0x00,/* "........", */
     0x00,/* "........"  */
     0x00,/* "........"  */
#endif
   },
   {
     0x80,/* "#.......", */
     0x40,/* ".#......", */
     0x20,/* "..#.....", */
     0x10,/* "...#....", */
     0x10,/* "...#....", */
     0x20,/* "..#.....", */
     0x40,/* ".#......", */
     0x80,/* "#.......", */
   }
  },

  { /* radio button middle in OFF state */
    189,
    189,
    SYSCHAR_RADIOBUTTON_OFF,
    {
#if 0
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........"  */
#else
      0x00, /* "........", */
      0xFE, /* "#######.", */
      0x01, /* ".......#", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x01, /* ".......#", */
      0xFE, /* "#######.", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........"  */
#endif
    },
    {
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0xFF, /* "########", */
    }
  },

  { /* radio button middle in ON state */
    190,
    190,
    SYSCHAR_RADIOBUTTONCHECK,
    {
#if 0
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x3C, /* "..####..", */
      0x7E, /* ".######.", */
      0x7E, /* ".######.", */
      0x3C, /* "..####..", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........"  */
#else
      0x00, /* "........", */
      0xFE, /* "#######.", */
      0x01, /* ".......#", */
      0x00, /* "........", */
      0x7C, /* ".#####..", */
      0xFE, /* "#######.", */
      0xFE, /* "#######.", */
      0xFE, /* "#######.", */
      0xFE, /* "#######.", */
      0x7C, /* ".#####..", */
      0x00, /* "........", */
      0x01, /* ".......#", */
      0xFE, /* "#######.", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........"  */
#endif
    },
    {
      0xFF, /* "########", */
      0x00, /* "........", */
      0x3C, /* "..####..", */
      0x7E, /* ".######.", */
      0x7E, /* ".######.", */
      0x3C, /* "..####..", */
      0x00, /* "........", */
      0xFF, /* "########", */
    }
  },

  { /* checkbox left side OFF */
    206,
    206,
    SYSCHAR_CHECKBOX_LBORDER,
    {
#if !defined(TELEVOICE)
      0x00, /* "........", */
      0x03, /* "......##", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x03, /* "......##", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#else
      0x00, /* "........", */
      0x0F, /* "....####", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x0F, /* "....####", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#endif
    },
    {
      0x0F, /* "....####", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x0F, /* "....####", */
    }
  },

  { /* checkbox middle OFF */
    207,
    207,
    SYSCHAR_CHECKBOX_OFF,
    {
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
    },
    {
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
      0xFF, /* "########", */
    }
  },

  { /* checkbox right side OFF */
    208,
    208,
    SYSCHAR_CHECKBOX_RBORDER,
    {
#if !defined(TELEVOICE)
      0x00, /* "........", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#else
      0x00, /* "........", */
      0xF0, /* "####....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0xF0, /* "####....", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#endif
    },
    {
      0xF0, /* "####....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0xF0, /* "####....", */
    }
  },

  { /* checkbox left side ON */
    252, //209,
    252, //209,
    SYSCHAR_CHECKBOX_LBORDER_ON,
    {
#if !defined(TELEVOICE)
      0x00, /* "........", */
      0x03, /* "......##", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x02, /* "......#.", */
      0x03, /* "......##", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#else
      0x00, /* "........", */
      0x0F, /* "....####", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x0A, /* "....#.#.", */
      0x0B, /* "....#.##", */
      0x09, /* "....#..#", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x0F, /* "....####", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#endif
    },
    {
      0x0F, /* "....####", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x09, /* "....#..#", */
      0x08, /* "....#...", */
      0x08, /* "....#...", */
      0x0F, /* "....####", */
    }
  },

  { /* checkbox middle ON */
    253, //210,
    253, //210,
    SYSCHAR_CHECKBOXCHECK,
    {
#if !defined(TELEVOICE)
      0x00, /* "........", */
      0xFF, /* "########", */
      0x82, /* "#.....#.", */
      0x44, /* ".#...#..", */
      0x44, /* ".#...#..", */
      0x28, /* "..#.#...", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x28, /* "..#.#...", */
      0x44, /* ".#...#..", */
      0x44, /* ".#...#..", */
      0x82, /* "#.....#.", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#else
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x01, /* ".......#", */
      0x03,/* "......##",*/
      0x06,/* ".....##.",*/
      0x0c,/* "....##..",*/
      0x18,/* "...##...",*/
      0x30,/* "..##....",*/
      0xE0,/* "###.....",*/
      0xc0,/* "##......",*/
      0x00, /* "........", */
      0xFF, /* "########", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#endif
    },
    {
      0xFF, /* "########", */
      0x03, /* "......##", */
      0x06, /* ".....##.", */
      0x0C, /* "....##..", */
      0x98, /* "#..##...", */
      0xF0, /* "####....", */
      0x60, /* ".##.....", */
      0xFF, /* "########", */
    }
  },

  { /* checkbox right side ON */
    254, //213,
    254, //213,
    SYSCHAR_CHECKBOX_RBORDER_ON,
    {
#if !defined(TELEVOICE)
      0x00, /* "........", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x80, /* "#.......", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#else
      0x00, /* "........", */
      0xF0, /* "####....", */
      0x10, /* "...#....", */
      0x90, /* "#..#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0xF0, /* "####....", */
      0x00, /* "........", */
      0x00, /* "........", */
      0x00, /* "........", */
#endif
    },
    {
      0xF0, /* "####....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0x10, /* "...#....", */
      0xF0, /* "####....", */
    }
  },
};


BOOL FAR PASCAL VidInitVGAFonts(void)
{
  int  i;

#ifndef TEST
  if (!IsVGA() && !IsEGA())
    return FALSE;
#endif

  if (InternalSysParams.bVGAFontsDone)
    return TRUE;

  /* 
    VioMapCharBitmap causes a video reset without clearing the buffers
    (which mouse drivers react to by hiding the mouse)
  */
  MouseHide();

  for (i = 0;  i < sizeof(_RemapCharInfo) / sizeof(_RemapCharInfo[0]);  i++)
  {
    if (VideoInfo.length == 25)
    {
      GetCharBitmap(_RemapCharInfo[i].chCharToMap,
                    _RemapCharInfo[i].chSavedBitmap16);
      VioMapCharBitmap(_RemapCharInfo[i].chCharToMap, 16,
                       (LPSTR) _RemapCharInfo[i].chBitmap16);
    }
    else
    {
      GetCharBitmap(_RemapCharInfo[i].chCharToMap,
                    _RemapCharInfo[i].chSavedBitmap16);
      VioMapCharBitmap(_RemapCharInfo[i].chCharToMap, 8,
                       (LPSTR) _RemapCharInfo[i].chBitmap8);
    }

#ifndef TEST
    _RemapCharInfo[i].chCharToSave= WinGetSysChar
	             (_RemapCharInfo[i].idxSysChar);
    WinSetSysChar(_RemapCharInfo[i].idxSysChar, 
                  (BYTE) _RemapCharInfo[i].chCharToMap);
#endif
  }
  MouseShow();
  VidSetBlinking(TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) ? FALSE : TRUE);

  InternalSysParams.bVGAFontsDone = TRUE;
#if defined(__TSC__) || defined(__WATCOMC__)
  atexit((void (*)()) VidRestoreVGAFonts);
#else
  atexit(VidRestoreVGAFonts);
#endif
  return TRUE;
}


VOID CDECL VidRestoreVGAFonts(void)
{
  int  i;

  if (InternalSysParams.bVGAFontsDone)
  {
    MouseHide(); 
    for (i = 0;  i < sizeof(_RemapCharInfo) / sizeof(_RemapCharInfo[0]);  i++)
    {
      if (VideoInfo.length == 25)
        VioMapCharBitmap(_RemapCharInfo[i].chCharToMap, 16,
                         (LPSTR) _RemapCharInfo[i].chSavedBitmap16);
      else
        VioMapCharBitmap(_RemapCharInfo[i].chCharToMap, 8,
                         (LPSTR) _RemapCharInfo[i].chSavedBitmap16);
      WinSetSysChar(_RemapCharInfo[i].idxSysChar, 
                       (BYTE) _RemapCharInfo[i].chCharToSave);
    }

    InternalSysParams.bVGAFontsDone = FALSE;
    MouseShow();
    VidSetBlinking(TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) ? FALSE : TRUE);
  }
}


VOID FAR PASCAL GetCharBitmap(int ch, BYTE *pBitmap)
{
  LPSTR lpPattern;
  int   i;

  CharacterGenSetMode();
  /*
    Model-independent code
  */
  lpPattern = (LPSTR) MK_FP(0xA000, ch*32);
  for (i = 16;  i--;  *pBitmap++ = *lpPattern++)
    ;
  CharacterGenClearMode();
}

#ifdef NEVER
VOID FAR PASCAL SetCharBitmap(int ch, BYTE *pBitmap)
{
  LPSTR lpPattern;
  int   i;

  CharacterGenSetMode();
  /*
    Model-independent code
  */
  lpPattern = (LPSTR) MK_FP(0xA000, ch*32);
  for (i = 16;  i--;  *lpPattern++ = *pBitmap++)
    ;
  CharacterGenClearMode();
}
#endif

#if defined(__TURBOC__) && !defined(__BORLANDC__)
#define outpw  outport
#endif

#define EGASetSequenceRegs(i,j)  outpw(0x3C4, ((j) << 8) | (i))
#define EGASetGraphicsRegs(i,j)  outpw(0x3CE, ((j) << 8) | (i))

/*
  From Wilton book Pg. 306-308
*/
VOID FAR PASCAL CharacterGenSetMode(void)
{
  EGASetSequenceRegs(2, 4);  /* enable bit plane 2 for writing */
  EGASetSequenceRegs(4, 7);  /* odd/even mode      */
  EGASetGraphicsRegs(5, 0);  /* read mode 0        */
  EGASetGraphicsRegs(6, 4);  /* A000, 64K          */
  EGASetGraphicsRegs(4, 2);  /* select plane 2 for reading */
}

VOID FAR PASCAL CharacterGenClearMode(void)
{
#ifdef TEST
  int iMode = VGACOLOR;
#else
  int iMode = VideoInfo.flags;
#endif

  EGASetSequenceRegs(2, 3);  /* enable bit planes 0 and 1 */
  EGASetSequenceRegs(4, 3);  /* alpha mode, extended mem */
  EGASetGraphicsRegs(5, 16); /* odd/even mode */
  if (iMode == VGACOLOR || iMode == EGACOLOR)
    EGASetGraphicsRegs(6, 14); /* chain odd maps to even, B800,32K */
  else
    EGASetGraphicsRegs(6, 10); /* chain odd maps to even, B000,32K */
  EGASetGraphicsRegs(4, 0);  /* select plane 0 for reading */
}



#ifdef TEST
main()
{
  VidInitVGAFonts();

  printf("\n\n\%c SYSMENU\n", 240);
  printf("\n\n\%c%c%c RADIOBUTTON", 181, 189, 182);
  printf("\n\n\%c%c%c RADIOBUTTON\n\n\n", 181, 190, 182);
  printf("\n\n\%c%c%c CHECKBOX\n\n\n", 206, 207, 208);
  printf("\n\n\%c%c%c CHECKBOX\n\n\n", 209, 210, 213);

  getch();
  return TRUE;
}
#endif
