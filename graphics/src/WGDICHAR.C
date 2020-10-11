/*===========================================================================*/
/*                                                                           */
/* File    : WGDICHAR.C                                                      */
/*                                                                           */
/* Purpose : System-dependent GDI mapping stuff                              */
/*           This file also contains all of the system-dependent drawing     */
/*           characters.                                                     */
/*                                                                           */
/* History :                                                                 */
/*     1/6/92 (maa) - major reorganization                                   */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

/*
  The constant EXTENDED_ASCII is defined if we are working with a PC-type
  console and we have access to the 256 character set.
*/
#if defined(DOS) || defined(OS2) || defined(i386) || defined(PC_CONSOLE)
#define EXTENDED_ASCII
#endif


/* normal ASCII characters */
#define CH_V                'V'
#define CH_X                'X'
#define CH_PIPE             '|'
#define CH_MINUS            '-'
#define CH_PLUS             '+'
#define CH_EQUAL            '='
#define CH_TILDE            '~'
#define CH_CARET            '^'
#define CH_SPACE            ' '
#define CH_LESSTHAN         '<'
#define CH_GREATERTHAN      '>'
#define CH_LEFTBRACKET      '['
#define CH_RIGHTBRACKET     ']'
#define CH_LEFTPAREN        '('
#define CH_RIGHTPAREN       ')'
#define CH_FSLASH           '/'
#define CH_BSLASH           '\\'

/* triangular characters */
#if defined(EXTENDED_ASCII) && !defined(MEWEL_GUI)
#define CH_RIGHTTRIANGLE    16    /* submenu indicator */
#define CH_LEFTTRIANGLE     17    /* multi-sel focus   */
#define CH_UPTRIANGLE       30    /* for maximize icon */
#define CH_DOWNTRIANGLE     31    /* for minimize icon */
#else
#define CH_RIGHTTRIANGLE    CH_GREATERTHAN
#define CH_LEFTTRIANGLE     CH_LESSTHAN
#define CH_UPTRIANGLE       CH_CARET
#define CH_DOWNTRIANGLE     CH_V
#endif

/* arrow characters */
#ifdef EXTENDED_ASCII
#define CH_UPDOWNARROW      18    /* for restore icon   */
#define CH_UPARROW          24    /* for vert scrollbar */
#define CH_DOWNARROW        25    /* for vert scrollbar */
#define CH_RIGHTARROW       26    /* for horz scrollbar */
#define CH_LEFTARROW        27    /* for horz scrollbar */
#else
#define CH_UPDOWNARROW      CH_PIPE
#define CH_UPARROW          CH_CARET
#define CH_DOWNARROW        CH_V
#define CH_RIGHTARROW       CH_GREATERTHAN
#define CH_LEFTARROW        CH_LESSTHAN
#endif

/* miscellaneous chars */
#ifdef EXTENDED_ASCII
#define CH_DOT              7     /* for dotted pen */
#define CH_CENTEREDDOT      7     /* for checked radio buttons */
#define CH_HATCH            176   /* used all over the place */
#define CH_DASHBOX          240   /* for system menu */
#define CH_BOLDGREATERTHAN  242   /* for def pushbutton border */
#define CH_BOLDLESSTHAN     243   /* for def pushbutton border */
#define CH_CHECKMARK        251   /* for checked menu items */
#if defined(UNIX)
#define CH_THUMB            219
#else
#define CH_THUMB            CH_SPACE
#endif
#else
#define CH_DOT              '.'
#define CH_CENTEREDDOT      CH_X
#define CH_HATCH            CH_SPACE
#define CH_DASHBOX          CH_EQUAL
#define CH_BOLDGREATERTHAN  CH_GREATERTHAN
#define CH_BOLDLESSTHAN     CH_LESSTHAN
#define CH_CHECKMARK        CH_X
#define CH_THUMB            CH_SPACE
#endif

/* solid line drawing chars */
#ifdef EXTENDED_ASCII
#define CH_SOLIDBOX         219   /* for scrollbar thumb */
#define CH_SOLIDTOP         220
#define CH_SOLIDRIGHT       221
#define CH_SOLIDLEFT        222
#define CH_SOLIDBOTTOM      223
#else
#define CH_SOLIDBOX         CH_SPACE
#define CH_SOLIDTOP         CH_SPACE
#define CH_SOLIDRIGHT       CH_SPACE
#define CH_SOLIDLEFT        CH_SPACE
#define CH_SOLIDBOTTOM      CH_SPACE
#endif

/* single-line drawing chars */
#ifdef EXTENDED_ASCII
#define CH_HORZLINE1        196
#define CH_VERTLINE1        179
#define CH_TOPLEFT1         218
#define CH_TOPRIGHT1        191
#define CH_BOTLEFT1         192
#define CH_BOTRIGHT1        217
#else
#define CH_HORZLINE1        CH_MINUS
#define CH_VERTLINE1        CH_PIPE
#define CH_TOPLEFT1         CH_PLUS
#define CH_TOPRIGHT1        CH_PLUS
#define CH_BOTLEFT1         CH_PLUS
#define CH_BOTRIGHT1        CH_PLUS
#endif

/* double-line drawing chars */
#ifdef EXTENDED_ASCII
#define CH_HORZLINE2        205
#define CH_VERTLINE2        186
#define CH_TOPLEFT2         201
#define CH_TOPRIGHT2        187
#define CH_BOTLEFT2         200
#define CH_BOTRIGHT2        188
#else
#define CH_HORZLINE2        CH_MINUS
#define CH_VERTLINE2        CH_PIPE
#define CH_TOPLEFT2         CH_PLUS
#define CH_TOPRIGHT2        CH_PLUS
#define CH_BOTLEFT2         CH_PLUS
#define CH_BOTRIGHT2        CH_PLUS
#endif

/* miscellaneous line-drawing chars */
#ifdef EXTENDED_ASCII
#define CH_TOPRIGHT12       183  /* single line to the left, dbl line down */
#define CH_TOPRIGHT21       184  /* dbl line to the left, single line down */
#define CH_BOTLEFT12        211  /* single line to the right, dbl line up  */
#define CH_BOTLEFT21        212  /* dbl line to the right, single line up  */
#else
#define CH_TOPRIGHT12       CH_PLUS
#define CH_TOPRIGHT21       CH_PLUS
#define CH_BOTLEFT12        CH_PLUS
#define CH_BOTLEFT21        CH_PLUS
#endif


/*
  These chars are used whenever we draw with a GDI pen object.

  The order of these chars are
  [0] horizontal line
  [1] vertical line
  [2] top-left corner
  [3] bottom-left corner
  [4] top-right corner
  [5] bottom-right corner
*/
BYTE FARDATA SysPenDrawingChars[][6] =
{
  /* PS_SOLID */
  { CH_HORZLINE1, CH_VERTLINE1, CH_TOPLEFT1, CH_BOTLEFT1, CH_TOPRIGHT1, CH_BOTRIGHT1 },

  /* PS_DASH */
  { CH_MINUS, CH_PIPE, CH_MINUS, CH_MINUS, CH_MINUS, CH_MINUS },

  /* PS_DOT */
  { CH_DOT, CH_DOT, CH_DOT, CH_DOT, CH_DOT, CH_DOT },

  /* PS_DASHDOT    */
  { CH_MINUS, CH_PIPE, CH_MINUS, CH_MINUS, CH_MINUS, CH_MINUS },

  /* PS_DASHDOTDOT */
  { CH_MINUS, CH_PIPE, CH_MINUS, CH_MINUS, CH_MINUS, CH_MINUS },

  /* PS_NULL */
  { CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE },
};


/*
  These chars are used whenever we draw with a GDI brush object.
*/
BYTE FARDATA SysBrushDrawingChars[] =
{
#if 80492
  CH_SPACE,        /* BS_SOLID     */
#else
  CH_SOLIDBOX,     /* BS_SOLID     */
#endif
  CH_SPACE,        /* BS_NULL      */
  CH_HATCH,        /* BS_HATCHED   */
  CH_HATCH,        /* BS_PATTERN   */
  CH_SOLIDBOX,     /* BS_INDEXED   */
  CH_HATCH,        /* BS_DIBPATTERN*/
};


/*
  These chars are used to fill a hatched brush
*/
BYTE FARDATA SysHatchDrawingChars[] =
{
  CH_HORZLINE1,    /* HS_HORIZONTAL */
  CH_VERTLINE1,    /* HS_VERTICAL   */
  CH_BSLASH,       /* HS_FDIAGONAL  */
  CH_FSLASH,       /* HS_BDIAGONAL  */
  CH_PLUS,         /* HS_CROSS      */
  CH_X,            /* HS_DIAGCROSS  */
};


/*
  These characters are used to make boxes. The order of the chars
  are :
  [0] top
  [1] bottom
  [2] left side
  [3] right side
  [4] top-left corner
  [5] bottom-left corner
  [6] top-right corner
  [7] bottom-right corner
*/
BYTE FARDATA SysBoxDrawingChars[][8] =
{
  /* BORDER_SINGLE */
  { CH_HORZLINE1, CH_HORZLINE1, CH_VERTLINE1, CH_VERTLINE1, CH_TOPLEFT1, CH_BOTLEFT1, CH_TOPRIGHT1, CH_BOTRIGHT1 },

  /* BORDER_DOUBLE */
  { CH_HORZLINE2, CH_HORZLINE2, CH_VERTLINE2, CH_VERTLINE2, CH_TOPLEFT2, CH_BOTLEFT2, CH_TOPRIGHT2, CH_BOTRIGHT2 },

  /* BORDER_DASHED */
  { CH_MINUS, CH_MINUS, CH_PIPE, CH_PIPE, CH_MINUS, CH_MINUS, CH_MINUS, CH_MINUS },

  /* BORDER_3D */
  { CH_HORZLINE1, CH_HORZLINE2, CH_VERTLINE1, CH_VERTLINE2, CH_TOPLEFT1, CH_BOTLEFT21, CH_TOPRIGHT12, CH_BOTRIGHT2 },

  /* BORDER_3DPUSHED */
  { CH_HORZLINE2, CH_HORZLINE1, CH_VERTLINE2, CH_VERTLINE1, CH_TOPLEFT2, CH_BOTLEFT12, CH_TOPRIGHT21, CH_BOTRIGHT1 },

#ifdef COMPUSERVE_MEWEL
  /* BORDER_MODAL */
  { CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE, CH_SPACE },

  /* BORDER_POPUP */
  { CH_SOLIDBOTTOM, CH_SOLIDTOP, CH_SOLIDBOX, CH_SOLIDBOX, CH_SOLIDBOX, CH_SOLIDBOX, CH_SOLIDBOX, CH_SOLIDBOX }
#endif
};


/*
  SYSTEM CHAR INFO
*/
BYTE FARDATA chSysChars[] =
{
/* SYSCHAR_WINDOW_BACKGROUND     */  CH_SPACE,
/* SYSCHAR_ACTIVE_CAPTION        */  CH_SPACE,
/* SYSCHAR_INACTIVE_CAPTION      */  CH_HATCH,
/* SYSCHAR_SYSMENU               */  CH_DASHBOX,
/* SYSCHAR_MINICON               */  CH_DOWNTRIANGLE,
/* SYSCHAR_MAXICON               */  CH_UPTRIANGLE,
/* SYSCHAR_EDIT_BACKGROUND       */  CH_SPACE,
/* SYSCHAR_EDIT_LBORDER          */  CH_LEFTBRACKET,
/* SYSCHAR_EDIT_RBORDER          */  CH_RIGHTBRACKET,
/* SYSCHAR_COMBO_ICON            */  CH_DOWNARROW,
/* SYSCHAR_RADIOBUTTONCHECK      */  CH_CENTEREDDOT,
/* SYSCHAR_RADIOBUTTON_LBORDER   */  CH_LEFTPAREN,
/* SYSCHAR_RADIOBUTTON_RBORDER   */  CH_RIGHTPAREN,
/* SYSCHAR_CHECKBOX_LBORDER      */  CH_LEFTBRACKET,
/* SYSCHAR_CHECKBOX_RBORDER      */  CH_RIGHTBRACKET,
/* SYSCHAR_PUSHBUTTON_LBORDER    */  CH_LESSTHAN,
/* SYSCHAR_PUSHBUTTON_RBORDER    */  CH_GREATERTHAN,
/* SYSCHAR_DEFPUSHBUTTON_LBORDER */  CH_BOLDLESSTHAN,
/* SYSCHAR_DEFPUSHBUTTON_RBORDER */  CH_BOLDGREATERTHAN,
/* SYSCHAR_SCROLLBAR_THUMB       */  CH_THUMB,
/* SYSCHAR_SCROLLBAR_LEFT        */  CH_LEFTARROW,
/* SYSCHAR_SCROLLBAR_RIGHT       */  CH_RIGHTARROW,
/* SYSCHAR_SCROLLBAR_UP          */  CH_UPARROW,
/* SYSCHAR_SCROLLBAR_DOWN        */  CH_DOWNARROW,
/* SYSCHAR_SCROLLBAR_FILL        */  CH_HATCH,
/* SYSCHAR_SUBMENU               */  CH_RIGHTTRIANGLE,
#if defined(MEWEL_GUI) && defined(MSC) && !defined(GX) && !defined(META)
/* SYSCHAR_MENUCHECK             */  150, /* checkmark in the MSC graphics font set */
#else
/* SYSCHAR_MENUCHECK             */  CH_CHECKMARK,
#endif
/* SYSCHAR_SHADOW                */  CH_SPACE,
/* SYSCHAR_HILITEPREFIX          */  CH_TILDE,
/* SYSCHAR_CHECKBOXNCHECK        */  CH_X,
/* SYSCHAR_RESTOREICON           */  CH_UPDOWNARROW,
/* SYSCHAR_MULTISELFOCUS         */  CH_RIGHTTRIANGLE,
/* SYSCHAR_3STATECHECK           */  CH_HATCH,
/* SYSCHAR_MDISYSMENU            */  CH_MINUS,
/* SYSCHAR_RADIOBUTTON_OFF       */  CH_SPACE,
/* SYSCHAR_CHECKBOX_OFF          */  CH_SPACE,
/* SYSCHAR_CHECKBOX_LBORDER_ON   */  CH_LEFTBRACKET,
/* SYSCHAR_CHECKBOX_RBORDER_ON   */  CH_RIGHTBRACKET,
/* SYSCHAR_BUTTONBOTTOMSHADOW    */  CH_SOLIDBOTTOM,
/* SYSCHAR_BUTTONRIGHTSHADOW     */  CH_SOLIDTOP, 
/* SYSCHAR_MULTISELFOCUS_RIGHT   */  CH_LEFTTRIANGLE,
/* SYSCHAR_DESKTOP_BACKGROUND    */  CH_SPACE,
};


#if defined(MULTICHAR_SYSICONS)
PSTR FARDATA pszSysCharStrs[] =
{
  /* SYSCHAR_SYSMENU     */  "[\xFE]",
  /* SYSCHAR_MINICON     */  "[\x19]",
  /* SYSCHAR_MAXICON     */  "[\x18]",
  /* SYSCHAR_RESTOREICON */  "[\x12]",
};
#endif

