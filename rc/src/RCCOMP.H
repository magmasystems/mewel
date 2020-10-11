/*===========================================================================*/
/*                                                                           */
/* File    : RCCOMP.H                                                        */
/*                                                                           */
/* Purpose : Include file for the resource compiler                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/

#define FALSE   0
#define TRUE    1

#define NORMAL_CLASS      0
#define BUTTON_CLASS      1
#define EDIT_CLASS        2
#define LISTBOX_CLASS     3
#define SCROLLBAR_CLASS   4
#define STATIC_CLASS      5
#define PUSHBUTTON_CLASS  6
#define CHECKBOX_CLASS    7
#define RADIOBUTTON_CLASS 8
#define TEXT_CLASS        9
#define FRAME_CLASS       10
#define BOX_CLASS         11
#define ICON_CLASS        12
#define COMBO_CLASS       13
#define USER_CLASS        20


#ifdef ZORTECH
#pragma ZTC align 1
#endif /* */

#pragma pack(1)

#define MAKEINTRESOURCE(i)           (i)

typedef struct resource
{
  BYTE  bResType;   /* 0 if iResType is a string-table offset - always 0xFF here */
#define RES_WINCOMPATDLG   0xFE
  WORD  iResType;
#define RT_CURSOR	  MAKEINTRESOURCE(1)
#define RT_BITMAP	  MAKEINTRESOURCE(2)
#define RT_ICON		  MAKEINTRESOURCE(3)
#define RT_MENU		  MAKEINTRESOURCE(4)
#define RT_DIALOG	  MAKEINTRESOURCE(5)
#define RT_STRING	  MAKEINTRESOURCE(6)
#define RT_FONTDIR	  MAKEINTRESOURCE(7)
#define RT_FONT		  MAKEINTRESOURCE(8)
#define RT_ACCELERATOR	  MAKEINTRESOURCE(9)
#define RT_RCDATA	  MAKEINTRESOURCE(10)
#define RT_GROUP_CURSOR   MAKEINTRESOURCE(12)
#define RT_GROUP_ICON	  MAKEINTRESOURCE(14)
#define RT_MSGBOX         MAKEINTRESOURCE(15)

  BYTE  bResID;    /* 0 if iResID is a string-table offset - always 0xFF here */
  WORD  iResID;

  WORD  fResFlags;
#define RES_MOVEABLE     0x0010
#define RES_PRELOAD      0x0040
#define RES_DISCARDABLE  0x1000

  DWORD nResBytes;
  BYTE  szResData[1];
} RESOURCE;


typedef struct rcstring
{
  WORD  idString;
  WORD  nChars;
  BYTE  string[1];
} RCSTRING;

typedef struct stringtable
{
  WORD nStrings;
  RCSTRING strings[1];
} STRINGTABLE;

typedef struct tagStringTblHeader
{
  DWORD ulSignature;
#define RC_SIGNATURE  0x12340031L   /* last two digits are version number */
  DWORD ulSeekPos;
  DWORD ulBytes;
} STRINGTBLHEADER;

typedef struct accel
{
  BYTE fFlags;
  WORD wEvent;
  WORD wId;
} ACCEL;


typedef struct mti
{
  WORD style;
  WORD attr;
  WORD idItem;
#ifdef FOOBAZ
  union
  {
    struct item_string           /* MF_STRING     */
    {
      WORD nChars;
      char   string[1];
    };
    MENUTEMPLATE item_popup;     /* MF_POPUP      */
    void foo;                    /* MF_SEPARATEOR */
  } uItem;
#endif
} MTI;

typedef struct menutemplate
{
/*WORD nBytes;*/
  WORD nItems;
  MTI    items[1];
} MENUTEMPLATE;


/*
  Windows-comaptible menu resource
*/
/* Menu template header */
typedef struct
{
  WORD versionNumber;
  WORD offset;
} MENUITEMTEMPLATEHEADER;

/* Menu template item struct */
typedef struct
{
  WORD mtOption;
  WORD mtID;      /* not in MF_POPUP */
  char mtString[1];
} MENUITEMTEMPLATE;



#define MF_INSERT	    0x0000
#define MF_CHANGE	    0x0080
#define MF_APPEND	    0x0100
#define MF_DELETE	    0x0200
#define MF_REMOVE	    0x1000

/* Menu flags for Add/Check/EnableMenuItem() */
#define MF_BYCOMMAND	    0x0000
#define MF_BYPOSITION	    0x0400

#define MF_SEPARATOR	    0x0800

#define MF_ENABLED	      0x0000
#define MF_GRAYED	        0x0001
#define MF_DISABLED	      0x0002

#define MF_UNCHECKED	    0x0000
#define MF_CHECKED	      0x0008
#define MF_USECHECKBITMAPS  0x0200

#define MF_STRING         0x0000
#define MF_BITMAP         0x0004
#define MF_OWNERDRAW      0x0100

#define MF_POPUP          0x0010
#define MF_MENUBARBREAK   0x0020
#define MF_MENUBREAK	    0x0040

#define MF_UNHILITE	      0x0000
#define MF_HILITE         0x0080

#define MF_SYSMENU        0x2000
#define MF_HELP           0x4000
#define MF_MOUSESELECT    0x8000

#define MF_END            0x0080  /* Only valid in menu resource templates */

#define MF_SHADOW         0x0020  /* same as MF_MENUBARBREAK */
#define MF_RIGHTJUST      0x8000  /* same as MF_MOUSESELECT  */


typedef struct dlgitem
{
  WORD  iClass;
  WORD  id;
  WORD  x, y, cx, cy;
  WORD  attr;
  WORD  nChars;
  char    szTitle[1];
  DWORD   dwStyle;
} DLGITEM;

typedef struct dlgtemplate
{
  WORD  nItems;
  WORD  id;
  WORD  x, y, cx, cy;
  WORD  attr;
  DWORD   dwStyle;
  WORD  nChars;
  char    szTitle[1];
  DLGITEM items[1];
} DIALOGTEMPLATE;


/*
  Microsoft Windows compatible dialog template
*/
typedef struct tagDlgTemplate
{
  long dtStyle;
  BYTE dtItemCount;
  int  dtX;
  int  dtY;
  int  dtCX;
  int  dtCY;
  /*
    NULL-terminated strings
  char dtMenuName[];
  char dtClassName[];
  char dtCaptionText[];
  */
} DLGTEMPLATE;


typedef struct tagDlgItemTemplate
{
  int  dtilX;
  int  dtilY;
  int  dtilCX;
  int  dtilCY;
  int  dtilID;
  long dtilStyle;
  /*
    NULL-terminated strings
  char dtilClass[];
  char dtilText[];

  BYTE dtilInfo;
  char dtilData[];
  */
} DLGITEMTEMPLATE;


extern int  xTranslated,  yTranslated;
extern int  cxTranslated, cyTranslated;
extern int  iRounding;
extern int  nPushButtonHeight;
extern WORD xDlg, yDlg, cxDlg, cyDlg;
extern WORD bEchoTranslation;
extern WORD bWindowsCompatDlg;
extern WORD bNoClipping;
extern WORD bNoBorders;
extern WORD bScreenRelativeCoords;
extern WORD bNoHeuristics;
extern WORD bUseCTMASK;
extern char szCustomIncludePath[];
extern DWORD CurrStyle, CurrDlgStyle;

