#include "int.h"
#include "rccomp.h"
#define RC_INVOKED
#include "style.h"
#include <ctype.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#if defined(sun)
#define RCSHORT
#else
#define RCSHORT  short
#endif

#define USE_WINDOWS_MENU

#define SYSTEM_COLOR      0xFF

long  fposStartOfResource;
long  fposResource;
int   resFD;
int   iDlgCaption;
int   iDlgClass;
int   iDlgMenu;
WORD  iMsgBoxTp;
DWORD CurrStyle, CurrDlgStyle, CurrNotStyle;
WORD  iCurrCtrlType;
char *szUserClass;

WORD CurrAccelState = 0;
WORD PrevAccelState = 0;
#define STATE_NOINVERT  0x02
#define STATE_SHIFT     0x04
#define STATE_CONTROL   0x08
#define STATE_ALT       0x10
#define STATE_END       0x80

/*
  Control classes
*/
#define CT_MASK       0x80
#define CT_BUTTON     0x80
#define CT_EDIT       0x81
#define CT_STATIC     0x82
#define CT_LISTBOX    0x83
#define CT_SCROLLBAR  0x84
#define CT_COMBOBOX   0x85

extern void push(long);
extern long pop(void);
extern void ReadUserResource(char *);
extern long CurrLong;
extern int  AddShiftToKey(unsigned int key, unsigned int shift);

/*
  These two variable can be set by the cmd line to translated Windows
  pixel units to character units. These values are number to divide the
  x coordinates and y coordinates by respectively.
*/
int xTranslated  = 0,
    yTranslated  = 0,
    cxTranslated = 0,
    cyTranslated = 0,
    iRounding    = 1;   /* default is to round down */
int nPushButtonHeight = 1;

WORD bKeyIsString = FALSE;

#ifndef max
#define max(a,b)        ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a,b)        ( ((a) < (b)) ? (a) : (b) )
#endif



/*
  These four vars record the dimensions of the dialog box. If we are in
  translation mode, then these values are added to the control coordinates,
  since MEWEL needs absolute screen coordinates.
*/
WORD xDlg, yDlg, cxDlg, cyDlg;
WORD xDlgOrig, yDlgOrig;
WORD bEchoTranslation  = 0;
RCSHORT int iDlgPtSize = 0;
RCSHORT int iDlgFont   = 0;

WORD bWindowsCompatDlg = 1;

/*
  Variables to assist the lexical analyzer
*/
WORD bNextSymbolIsLiteral = 0;
WORD bReadingRawDataList = 0;

/*
  JUST_DID_USERRES is a hack which is set when we just processed a userres.
  Set bExpectingRESID back to 0, since we probably gobbled up the
  first token of the next resource entry.
*/
WORD bExpectingRESID = 1;
#define JUST_DID_USERRES  (0xFF)

WORD bNoClipping = 0;
WORD bNoBorders  = 0;
WORD bScreenRelativeCoords = 0;
WORD bUseCTMASK = 0;

STRINGTBLHEADER StringTblHeader;
static BOOL     bSawStringtable = FALSE;

struct
{
  WORD style;
  WORD attr;
  WORD idItem;
} MTIheader;

# define NUMBER 257
# define LNUMBER 258
# define STRING 259
# define ID 260
# define BEGIN 261
# define END 262
# define STRINGTABLE 263
# define ACCELERATORS 264
# define KEYCODE 265
# define MENU 266
# define MENUITEM 267
# define POPUP 268
# define SEPARATOR 269
# define MENUBREAK 270
# define CHECKED 271
# define INACTIVE 272
# define GRAYED 273
# define HELP 274
# define SHADOW 275
# define DIALOG 276
# define STYLE 277
# define CLASS 278
# define CAPTION 279
# define TEXT 280
# define EDIT 281
# define CHECKBOX 282
# define RADIOBUTTON 283
# define PUSHBUTTON 284
# define STATIC 285
# define LISTBOX 286
# define FRAME 287
# define BOX 288
# define SCROLLBAR 289
# define ICON 290
# define COMBOBOX 291
# define RCDATA 292
# define PRELOAD 293
# define LOADONCALL 294
# define MOVEABLE 295
# define DISCARDABLE 296
# define FIXED 297
# define FONT 298
# define CONTROL 299
# define BITMAP 300
# define CURSOR 301
# define DEFPUSHBUTTON 302
# define IF 303
# define IFDEF 304
# define IFNDEF 305
# define ELSE 306
# define ELIF 307
# define ENDIF 308
# define ALT 309
# define ASCII 310
# define SHIFT 311
# define VIRTKEY 312
# define NOINVERT 313
# define RCINCLUDE 314
# define MSGBOX 315
# define OK 316
# define OKCANCEL 317
# define YESNO 318
# define YESNOCANCEL 319
# define MENUBARBREAK 320
# define CTEXT 321
# define RTEXT 322
# define GROUPBOX 323
# define UNCHECKED 324
# define ENABLED 325
# define PURE 326
# define NOT 327
# define IMPURE 328
# define VERSIONINFO 329
# define FILEVERSION 330
# define PRODUCTVERSION 331
# define FILEFLAGSMASK 332
# define FILEFLAGS 333
# define FILEOS 334
# define FILETYPE 335
# define FILESUBTYPE 336
# define BLOCK 337
# define VALUE 338
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval, yyval;
typedef int yytabelem;
# define YYERRCODE 256


/*--------------------------------------------------------------------------*/
long posStack[16];
int  posStackSP = -1;

void push(pos)
  long pos;
{
  if (++posStackSP < 16)
    posStack[posStackSP] = pos;
}

long pop()
{
  if (posStackSP >= 0)
    return posStack[posStackSP--];
  else
    return 0L;
}


unsigned long WriteResourceHeader(resType, resID)
  int resType;
  int resID;
{
  RESOURCE r;
  unsigned long posStart;

  posStart = tell(resFD);
  r.bResType  = (bWindowsCompatDlg) ? RES_WINCOMPATDLG : 0xFF;
  r.iResType  = resType;
  r.bResID    = 0xFF;
  r.iResID    = (WORD) resID;
  r.fResFlags = 0x0000;
  r.nResBytes = 0L;
  nwrite(resFD, (char *) &r, sizeof(r));
  return posStart;
}


void ReadUserResource(szFile)
  char *szFile;
{
  char buf[256];
  int  fd;

  WORD  len;
  DWORD nResBytes = 0L;
  char  *szBuf;


  /*
    Find the resource in the current directory or along the INCLUDE path
  */
  if (!DosSearchPath(NULL, szFile, buf) &&
      !DosSearchPath("INCLUDE", szFile, buf))
    goto err;

#ifdef VAXC
  fd = open(buf, O_RDONLY | O_BINARY, 0);
#else
  fd = open(buf, O_RDONLY | O_BINARY);
#endif

  if (fd < 0)
  {
err:
    yywhere();
    printf("Cannot open data file [%s]\n", szFile);
    exit(1);
  }

  szBuf = emalloc(256);
  while ((len = read(fd, szBuf, 256)) > 0)
  {
    nwrite(resFD, szBuf, len);
    nResBytes += len;
  }
  close(fd);
  free(szBuf);

 /*
    Seek back to the RESOURCE.nResBytes field and fill in the
    number of bytes in the stringtable.
 */
 fposResource = tell(resFD);
 lseek(resFD, fposStartOfResource, 0);
 lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
 nResBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
 nwrite(resFD, (char *) &nResBytes, sizeof(nResBytes));

 /*
   Return to the end of the resource file...
 */
 lseek(resFD, 0L, 2);
 fposResource = tell(resFD);
}

ReadBinaryResourceFromFile(idIcon, idFile, resType)
  int idIcon;
  int idFile;
  int resType;
{
  char buf[256];
  unsigned char *pBuf;
  char *pszFile;
  char c;
  int  len;
  int  fd;
  DWORD nBytes;

  if ((pBuf = malloc(2048)) == NULL)
    return 0;

  /*
    Try to open the icon file
  */
  if (idFile < 0)
    pszFile = Symtab[-idFile].name;
  else
    pszFile = Symtab[idFile].u.sval;

  /*
    Find the resource in the current directory or along the INCLUDE path
  */
  if (!DosSearchPath(NULL, pszFile, buf) &&
      !DosSearchPath("INCLUDE", pszFile, buf))
    goto err;

#ifdef VAXC
  if ((fd = open(buf, O_RDONLY | O_BINARY, 0)) < 0)
#else
  if ((fd = open(buf, O_RDONLY | O_BINARY)) < 0)
#endif
  {
err:
    free(pBuf);
    return 0;
  }

  /*
    Write out a resource header with resType.
    The write out the icon header and bitmap.
  */
  fposStartOfResource = WriteResourceHeader(resType, idIcon);

  while ((len = read(fd, pBuf, 2048)) > 0)
    nwrite(resFD, (char *) pBuf, len);

  /*
    Go the the end of the resource file and record the position.
  */
  lseek(resFD, 0L, 2);
  fposResource = tell(resFD);

  /*
     Seek back to the RESOURCE.nResBytes field and fill in the
     number of bytes in the resource.
  */
  lseek(resFD, fposStartOfResource, 0);
  lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
  nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
  nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

  /*
    Return to the end of the resource file...
  */
  lseek(resFD, 0L, 2);
  fposResource = tell(resFD);


  /*
    Clean up and leave
  */
  free(pBuf);
  close(fd);
  return 1;
}




WriteIDTable()
{
  register int  sp;
  SYMBOL   *sym;
  RCSTRING rc;

  StringTblHeader.ulSeekPos = tell(resFD);
  StringTblHeader.ulBytes = 0;
  StringTblHeader.ulSignature = RC_SIGNATURE;

  for (sp = 1;  sp < Sym_sp;  sp++)     /* look through the entire table */
  {
    sym = &Symtab[sp];
    if (sym->type == ID)
    {
      rc.idString = -(sp);
#ifdef WORD_ALIGNED /* Keep resource data word-aligned */
      rc.nChars = word_pad(strlen(sym->name) + 1);
#else
      rc.nChars = strlen(sym->name) + 1;
#endif
      nwrite(resFD, (char *) &rc.idString, sizeof(rc.idString));
      nwrite(resFD, (char *) &rc.nChars,   sizeof(rc.nChars));
      nwrite(resFD, sym->name, rc.nChars);
      StringTblHeader.ulBytes += 
               sizeof(rc.idString) + sizeof(rc.nChars) + rc.nChars;
    }
  }

  /*
    Write the header at the beginning of the file
  */
  lseek(resFD, 0L, 0);
  nwrite(resFD, (char *) &StringTblHeader, sizeof(StringTblHeader));

  return 1;
}


/*
  MS WINDOWS Compatibility Functions
*/

struct
{
  char *pszName;
  int  idClass;
  int  ctMask;
} ClassInfo[] =
{
  "button",     BUTTON_CLASS,     CT_BUTTON,
  "combobox",   COMBO_CLASS,      CT_COMBOBOX,
  "edit",       EDIT_CLASS,       CT_EDIT,
  "listbox",    LISTBOX_CLASS,    CT_LISTBOX,
  "scrollbar",  SCROLLBAR_CLASS,  CT_SCROLLBAR,
  "static",     STATIC_CLASS,     CT_STATIC,
  NULL,         0,                0,
};

StringToClassID(szClass, pStyle, pCTMask)
  char *szClass;
  unsigned long *pStyle;  /* This will be changed to MEWEL style bits */
  int  *pCTMask;
{
  int  i;
  int  idClass;

  /*
    Assume we have a non-standard class
  */
  *pCTMask = 0;

  /*
    Walk down the list of pre-defined classes and map the class name to an id
  */
  for (i = 0;  ClassInfo[i].idClass;  i++)
    if (!stricmp(ClassInfo[i].pszName, szClass))
    {
      idClass  = ClassInfo[i].idClass;
      *pCTMask = ClassInfo[i].ctMask;
      break;
    }

  /*
    If we didn't find a pre-defined class name, then it's a user-defined
    class. Just return the USER_CLASS id and leave the style bits intact.
  */
  if (!ClassInfo[i].idClass)
  {
    return USER_CLASS;
  }

  /*
    Use the style bits to further hone in on a MEWEL class. Remove the
    offending Windows style bits from the MEWEL style.
  */
  switch (idClass)
  {
    /*
      For combo boxes, we don't need to mess around with the style bits
    */
    case COMBO_CLASS :
      return COMBO_CLASS;

    /*
      For edit fields, we don't need to mess around with the style bits
    */
    case EDIT_CLASS  :
      return EDIT_CLASS;

    /*
      For scrollbars, we don't need to mess around with the style bits
    */
    case SCROLLBAR_CLASS :
      return SCROLLBAR_CLASS;

    /*
      For list boxes, we don't need to mess around with the style bits
    */
    case LISTBOX_CLASS :
      return LISTBOX_CLASS;

    /*
    */
    case BUTTON_CLASS :
    {
      unsigned long sStyle = (*pStyle & 0x0F);
      if (sStyle == BS_PUSHBUTTON || sStyle == BS_DEFPUSHBUTTON)
      {
        idClass = PUSHBUTTON_CLASS;
      }
      else if (sStyle == BS_CHECKBOX || sStyle == BS_AUTOCHECKBOX ||
               sStyle == BS_3STATE   || sStyle == BS_AUTO3STATE)
      {
        idClass = CHECKBOX_CLASS;
      }
      else if (sStyle == BS_RADIOBUTTON || sStyle == BS_AUTORADIOBUTTON)
      {
        idClass = RADIOBUTTON_CLASS;
      }
      else if (sStyle == BS_GROUPBOX)
#if 12194
        idClass = BUTTON_CLASS;
#else
        idClass = FRAME_CLASS;
#endif
      else
        idClass = PUSHBUTTON_CLASS;
      if (!bNoBorders)
        if (idClass == PUSHBUTTON_CLASS &&
            !(*pStyle & WS_SHADOW) && (*pStyle & 0x0F) != BS_OWNERDRAW)
          *pStyle |= WS_BORDER;
      return idClass;
    }

    /*
    */
    case STATIC_CLASS :
    {
      unsigned long sStyle = (*pStyle & 0x0F);
      if (sStyle == SS_LEFT || sStyle == SS_CENTER || sStyle == SS_RIGHT || 
          sStyle == SS_SIMPLE)
        idClass = TEXT_CLASS;
      else
      if (sStyle == (SS_BLACKRECT & 0x0F) || 
          sStyle == (SS_WHITERECT & 0x0F) ||
          sStyle == (SS_GRAYRECT  & 0x0F))
      {
        idClass = BOX_CLASS;
        *pStyle |= SS_BOX;
      }
      else
      if (sStyle == (SS_BLACKFRAME & 0x0F) ||
          sStyle == (SS_WHITEFRAME & 0x0F) ||
          sStyle == (SS_GRAYFRAME  & 0x0F))
      {
        idClass = FRAME_CLASS;
        *pStyle |= SS_FRAME;
      }
      return idClass;
    }
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : WindowizeString(char *str)                                    */
/*                                                                          */
/* Purpose  : Translates embedded '&' to the MEWEL hot-character '~'        */
/*                                                                          */
/* Returns  : TRUE if successful, FALSE if not.                             */
/*                                                                          */
/****************************************************************************/
WindowizeString(str)
  char *str;
{
  /*
    Don't translate static controls with SS_NOPREFIX
  */
  if ((iCurrCtrlType == STATIC_CLASS || iCurrCtrlType == TEXT_CLASS) &&
       (CurrStyle & SS_NOPREFIX))
    return FALSE;

  /*
    Go through each character in the string
  */
  for (  ;  *str;  str++)
  {
    /*
      We found an ampersand which is not the last char in the string...
    */
    if (*str == '&' && str[1] != '\0')
    {
      /*
        &x => ~x
        && => &&
      */
      if (str[1] == '&')
        str++;
      else
        *str = '~';
    }
  }

  return TRUE;
}



/*************************************************************************/
/*                                                                       */
/*     ACCELERATOR TRANSLATION TABLES FOR WINDOWS COMPATIBILITY          */
/*                                                                       */
/*************************************************************************/

unsigned int KeyCodeValues[][4] =
{
  /* normal */   /* alt */        /* shift */    /* control */
{ 'a',           VK_ALT_A,        'A',           VK_CTRL_A       },
{ 'b',           VK_ALT_B,        'B',           VK_CTRL_B       },
{ 'c',           VK_ALT_C,        'C',           VK_CTRL_C       },
{ 'd',           VK_ALT_D,        'D',           VK_CTRL_D       },
{ 'e',           VK_ALT_E,        'E',           VK_CTRL_E       },
{ 'f',           VK_ALT_F,        'F',           VK_CTRL_F       },
{ 'g',           VK_ALT_G,        'G',           VK_CTRL_G       },
{ 'h',           VK_ALT_H,        'H',           VK_CTRL_H       },
{ 'i',           VK_ALT_I,        'I',           VK_CTRL_I       },
{ 'j',           VK_ALT_J,        'J',           VK_CTRL_J       },
{ 'k',           VK_ALT_K,        'K',           VK_CTRL_K       },
{ 'l',           VK_ALT_L,        'L',           VK_CTRL_L       },
{ 'm',           VK_ALT_M,        'M',           VK_CTRL_M       },
{ 'n',           VK_ALT_N,        'N',           VK_CTRL_N       },
{ 'o',           VK_ALT_O,        'O',           VK_CTRL_O       },
{ 'p',           VK_ALT_P,        'P',           VK_CTRL_P       },
{ 'q',           VK_ALT_Q,        'Q',           VK_CTRL_Q       },
{ 'r',           VK_ALT_R,        'R',           VK_CTRL_R       },
{ 's',           VK_ALT_S,        'S',           VK_CTRL_S       },
{ 't',           VK_ALT_T,        'T',           VK_CTRL_T       },
{ 'u',           VK_ALT_U,        'U',           VK_CTRL_U       },
{ 'v',           VK_ALT_V,        'V',           VK_CTRL_V       },
{ 'w',           VK_ALT_W,        'W',           VK_CTRL_W       },
{ 'x',           VK_ALT_X,        'X',           VK_CTRL_X       },
{ 'y',           VK_ALT_Y,        'Y',           VK_CTRL_Y       },
{ 'z',           VK_ALT_Z,        'Z',           VK_CTRL_Z       },
{ '1',           VK_ALT_1,        '1',           0               },
{ '2',           VK_ALT_2,        '2',           0               },
{ '3',           VK_ALT_3,        '3',           0               },
{ '4',           VK_ALT_4,        '4',           0               },
{ '5',           VK_ALT_5,        VK_SH_FIVE,    0               },
{ '6',           VK_ALT_6,        '6',           0               },
{ '7',           VK_ALT_7,        '7',           0               },
{ '8',           VK_ALT_8,        '8',           0               },
{ '9',           VK_ALT_9,        '9',           0               },
{ '0',           VK_ALT_0,        '0',           0               },
{ '-',           VK_ALT_MINUS,    '-',           0               },
{ '+',           VK_ALT_PLUS,     '+',           0               },
{ '^',           0,               '^',           '^' & 0x1F      },
{ '_',           0,               '_',           '_' & 0x1F      },
{ ']',           0,               ']',           ']' & 0x1F      },
{ '\\',          0,               '\\',          '\\'& 0x1F      },
#ifdef INTERNATIONAL_MEWEL
{ 'Ñ',           0,               'é',           0               },
{ 'î',           0,               'ô',           0               },
{ 'Å',           0,               'ö',           0               },
{ 'ë',           0,               'í',           0               },
{ 'Ü',           0,               'è',           0               },
{ 'á',           0,               'Ä',           0               },
{ '§',           0,               '•',           0               },
{ 'Ç',           0,               'ê',           0               },
#endif


{ VK_F1,         VK_ALT_F1,       VK_SH_F1,      VK_CTRL_F1      },
{ VK_F2,         VK_ALT_F2,       VK_SH_F2,      VK_CTRL_F2      },
{ VK_F3,         VK_ALT_F3,       VK_SH_F3,      VK_CTRL_F3      },
{ VK_F4,         VK_ALT_F4,       VK_SH_F4,      VK_CTRL_F4      },
{ VK_F5,         VK_ALT_F5,       VK_SH_F5,      VK_CTRL_F5      },
{ VK_F6,         VK_ALT_F6,       VK_SH_F6,      VK_CTRL_F6      },
{ VK_F7,         VK_ALT_F7,       VK_SH_F7,      VK_CTRL_F7      },
{ VK_F8,         VK_ALT_F8,       VK_SH_F8,      VK_CTRL_F8      },
{ VK_F9,         VK_ALT_F9,       VK_SH_F9,      VK_CTRL_F9      },
{ VK_F10,        VK_ALT_F10,      VK_SH_F10,     VK_CTRL_F10     },
{ VK_F11,        VK_ALT_F11,      VK_SH_F11,     VK_CTRL_F11     },
{ VK_F12,        VK_ALT_F12,      VK_SH_F12,     VK_CTRL_F12     },

{ VK_HOME,       VK_ALT_HOME,     VK_SH_HOME,    VK_CTRL_HOME    },
{ VK_END,        VK_ALT_END,      VK_SH_END,     VK_CTRL_END     },
{ VK_PGUP,       VK_ALT_PGUP,     VK_SH_PGUP,    VK_CTRL_PGUP    },
{ VK_PGDN,       VK_ALT_PGDN,     VK_SH_PGDN,    VK_CTRL_PGDN    },
{ VK_UP,         VK_ALT_UP,       VK_SH_UP,      VK_CTRL_UP      },
{ VK_DOWN,       VK_ALT_DOWN,     VK_SH_DOWN,    VK_CTRL_DOWN    },
{ VK_LEFT,       VK_ALT_LEFT,     VK_SH_LEFT,    VK_CTRL_LEFT    },
{ VK_RIGHT,      VK_ALT_RIGHT,    VK_SH_RIGHT,   VK_CTRL_RIGHT   },
{ VK_INS,        VK_ALT_INS,      VK_SH_INS,     VK_CTRL_INS     },
{ VK_DEL,        VK_ALT_DEL,      VK_SH_DEL,     VK_CTRL_DEL     },

{ VK_TAB,        VK_ALT_TAB,      VK_BACKTAB,    VK_CTRL_TAB     },
{ VK_BACKSPACE,  VK_ALT_BACKSPACE,VK_SH_BACKSPACE,VK_CTRL_BACKSPACE }
};

int AddShiftToKey(key, shift)
  unsigned int key, shift;
{
#ifdef INTERNATIONAL_MEWEL
   int i, idxShift, nkey;

#define SHIFT_SHIFT ((WORD) 0x03)
#define CTL_SHIFT   ((WORD) 0x04)
#define ALT_SHIFT   ((WORD) 0x08)
#define SPECIAL_CHAR ((WORD) 0x10)

   if (!(shift &= STATE_ALT | STATE_SHIFT | STATE_CONTROL))
     return key;

   switch (shift)
   {
     case STATE_ALT:      idxShift = 1; break;
     case STATE_SHIFT:    idxShift = 2; break;
     case STATE_CONTROL:  idxShift = 3; break;
     default:             idxShift = (shift & STATE_SHIFT) ? 2 : 0; break;
   }

   nkey = 0;
   if (idxShift)
     for (i = 0;  i < sizeof(KeyCodeValues) / 4;  i++)
       if (key == KeyCodeValues[i][0])
       {
         nkey = KeyCodeValues[i][idxShift];
         break;
       }

   if (!nkey || !idxShift || (idxShift == 2 && shift != STATE_SHIFT))
   {
      if (nkey & 0xff)
         key = nkey;
      if (!(key = (key << 8) & 0xff00))
         return 0;
      if (shift & STATE_ALT)
         key |= ALT_SHIFT;
      if (shift & STATE_SHIFT)
         key |= SHIFT_SHIFT;
      if (shift & STATE_CONTROL)
         key |= CTL_SHIFT;
      key |= SPECIAL_CHAR;
   }
   else
      key = nkey;

  return key;

#else

  int i, idxShift;
  unsigned int key2 = key;

  if (shift & STATE_ALT)
    idxShift = 1;
  else if (shift & STATE_SHIFT)
    idxShift = 2;
  else if (shift & STATE_CONTROL)
    idxShift = 3;
  else
    idxShift = 0;

  if (idxShift && key < 128)
    key2 = tolower(key);

  for (i = 0;  i < sizeof(KeyCodeValues) / 4;  i++)
    if (key2 == KeyCodeValues[i][0])
      return KeyCodeValues[i][idxShift];
  return key;
#endif
}


ProcessStringTables()
{
  DWORD nBytes;
  WORD  nIndex = 0;

  if (!bSawStringtable)
    return FALSE;

  fposStartOfResource = WriteResourceHeader(RT_STRING, 0);
  fposResource = tell(resFD);  /* pos after the header */

  /*
    After the resource header, we output a dummy value for the
    number of strings. We will go back and fill this value in
    after the stringtable is processed.
  */
  nwrite(resFD, (char *) &nIndex, sizeof(nIndex));

  /*
    Dump out all of the strings
  */
  nIndex = StringInfoDumpStrings();

  /*
    Go back to just after the resource header, and fill in the
    number of strings.
  */
  lseek(resFD, fposResource, 0);
  nwrite(resFD, (char *) &nIndex, sizeof(nIndex));

  /*
    Go the the end of the resource file and record the position.
  */
  lseek(resFD, 0L, 2);
  fposResource = tell(resFD);

  /*
    Seek back to the RESOURCE.nResBytes field and fill in the
    number of bytes in the stringtable.
  */
  lseek(resFD, fposStartOfResource, 0);
  lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
  nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
  nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

  /*
    Return to the end of the resource file...
  */
  lseek(resFD, 0L, 2);
  fposResource = tell(resFD);

#ifdef RC_TRANS
  AddStringTable();
#endif

  return TRUE;
}


int CtrlTypeToCT(iCurrCtrlType)
  int iCurrCtrlType;
{
  switch (iCurrCtrlType)
  {
    case CHECKBOX_CLASS:
      return CT_BUTTON;
    case PUSHBUTTON_CLASS:
      return CT_BUTTON;
    case RADIOBUTTON_CLASS:
      return CT_BUTTON;
    case FRAME_CLASS:
      CurrStyle &= ~0x0FL;
      CurrStyle |= BS_GROUPBOX;
      return CT_BUTTON;
    case TEXT_CLASS :
      return CT_STATIC;
    case STATIC_CLASS:
      return CT_STATIC;
    case ICON_CLASS:
      CurrStyle &= ~0x0FL;
      CurrStyle |= SS_ICON;
      return CT_STATIC;
    case EDIT_CLASS :
      return CT_EDIT;
    case LISTBOX_CLASS:
      return CT_LISTBOX;
    case SCROLLBAR_CLASS:
      return CT_SCROLLBAR;
    case COMBO_CLASS:
      return CT_COMBOBOX;
    default :
      return 0;
  }
}


yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 22,
	329, 166,
	-2, 118,
-1, 145,
	262, 128,
	-2, 21,
-1, 204,
	261, 53,
	-2, 21,
-1, 222,
	262, 52,
	267, 52,
	268, 52,
	-2, 21,
	};
# define YYNPROD 195
# define YYLAST 616
yytabelem yyact[]={

   147,    80,   199,   247,    96,   298,   217,   225,   278,    94,
    92,   156,    93,    96,    95,    37,   266,   318,    94,    92,
   320,    93,   303,    95,   258,   163,   282,   285,   286,   289,
   287,   292,   290,   294,   295,   297,   296,   291,   266,   319,
   262,    65,   320,    65,   192,   281,   174,   266,   288,    63,
   111,   112,   113,   114,    39,    40,    44,    45,    43,   250,
   163,   163,   150,    65,   171,   172,    99,   284,   283,   293,
    82,    83,    84,    85,    86,    87,    88,   260,    76,   219,
    77,   238,   219,   104,   103,   102,   158,    41,   227,    42,
   133,    97,   188,   136,   137,   138,   139,   140,   141,   142,
    97,    73,   187,    74,   163,   169,    70,   193,    71,   100,
   171,   172,    39,    40,    44,    45,    43,    39,    40,    44,
    45,    43,   206,   207,   208,   209,   210,   211,   175,   168,
    58,   167,    91,    23,   164,    39,    40,    44,    45,    43,
    39,    40,    44,    45,    43,    41,    90,    42,   237,    60,
    41,   249,    42,   248,   314,    25,   304,    26,   235,   232,
   236,   233,   234,   104,   103,   102,   123,    27,    41,   301,
    42,    30,   212,    41,   190,    42,   213,   214,   269,    22,
   267,    31,    23,    28,    20,    17,   228,   195,   216,    34,
   255,    32,    33,   275,   274,    65,   189,   203,   185,   215,
   181,   253,   256,   254,    79,    56,    35,   271,   270,   241,
   229,   201,   182,   117,    18,   135,   230,   200,   204,   205,
   146,   144,   257,   273,   160,   300,   226,   322,   218,   163,
    29,   307,   101,   264,   263,   116,    19,   206,   207,   208,
   209,   210,   211,   180,   220,   194,   159,   184,   157,   130,
   128,    57,   126,   120,   145,   264,   263,   124,    64,   119,
    64,    96,   162,   132,   264,   263,    94,    92,   226,    93,
    81,    95,   115,    59,    21,   176,   110,   109,   179,   178,
    64,   108,   148,    69,    72,    75,    78,   212,   107,   106,
   152,   213,   214,   261,    68,   186,    39,    40,    44,    45,
    43,   105,    50,   265,   305,    96,   276,   309,   273,   310,
    94,    92,   313,    93,   299,    95,   317,   196,   197,   118,
    66,   149,   324,   325,   202,   265,    36,   328,   329,    41,
   160,    42,   333,   224,   265,   335,   302,   280,   332,   125,
   127,   129,   131,   336,   279,   252,   330,   272,    97,   251,
   231,   224,   246,   223,   240,   170,    62,   243,   221,   237,
   323,   153,   154,   155,   166,   165,   134,    61,    89,   235,
   232,   236,   233,   234,   177,    38,    16,   161,   259,    15,
    96,    14,    13,   122,   121,    94,    92,   334,    93,    12,
    95,    11,    97,    10,     9,    96,   191,     8,     7,   277,
    94,    92,   331,    93,     6,    95,     5,     4,     3,     2,
    96,     1,    64,     0,   183,    94,    92,   327,    93,    96,
    95,     0,   222,     0,    94,    92,   326,    93,     0,    95,
     0,     0,   306,     0,   308,     0,     0,     0,   311,   312,
     0,     0,     0,   316,   239,   242,     0,     0,   244,     0,
    96,     0,     0,     0,   245,    94,    92,   321,    93,     0,
    95,     0,     0,     0,     0,     0,    96,    97,     0,     0,
   268,    94,    92,   315,    93,     0,    95,     0,    96,     0,
     0,     0,    97,    94,    92,   163,    93,     0,    95,     0,
     0,     0,     0,    96,     0,     0,     0,    97,    94,    92,
   198,    93,    96,    95,     0,     0,    97,    94,    92,   173,
    93,    96,    95,    24,     0,   143,    94,    92,    96,    93,
     0,    95,     0,    94,    92,    98,    93,     0,    95,     0,
     0,     0,     0,     0,     0,     0,     0,    97,     0,    46,
    47,    48,    49,     0,    51,    52,    53,    54,    55,     0,
     0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    67,    97,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    97,     0,     0,     0,     0,     0,     0,     0,     0,    97,
     0,     0,     0,     0,     0,     0,     0,     0,    97,     0,
     0,     0,     0,     0,     0,    97,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   151 };
yytabelem yypact[]={

 -1000,   -78, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,  -109, -1000,
 -1000,  -314, -1000, -1000,  -239, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000,   -54,  -127, -1000,  -112, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000,  -239,  -239,     3,  -239,
 -1000,  -239,  -153,  -158,  -181,  -239,   -55, -1000, -1000,  -260,
 -1000,  -115,  -129,   481, -1000,   155,  -152,  -176,   -96, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,  -266,
 -1000, -1000,   -44,   -44,   126,   126,   126,   126,   126,     1,
 -1000, -1000,   155,   155,   155,   155,   155,   155,   155,   474,
    23, -1000, -1000, -1000, -1000, -1000,    23,    23,    23, -1000,
 -1000, -1000, -1000, -1000, -1000,  -251, -1000,   202, -1000, -1000,
   100, -1000, -1000,   126, -1000,   100, -1000,   100, -1000,   100,
 -1000,   100, -1000,   441,  -128,  -157,   268,   268,   268,   268,
   268,   268,   465, -1000,  -216,   185,   185,   268, -1000, -1000,
 -1000,  -239,    23,    17,    16,   -19, -1000, -1000,   -59,   -45,
   126,   206,   -61, -1000, -1000, -1000,   185, -1000, -1000, -1000,
 -1000,  -167,   -63,   155, -1000,    23, -1000,  -218, -1000, -1000,
 -1000,  -154,   201,   100, -1000, -1000,   155, -1000,   185,   185,
   456, -1000, -1000,  -335,   -46,   441,   155,  -148,   155,   -71,
  -256,   200, -1000,   -33,   185, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000,   224,  -173, -1000, -1000,   -73,
   -47,    60,   185,  -180,  -148,   185,   -48, -1000,   185, -1000,
 -1000,  -151, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000,  -259,  -106, -1000,  -203,   -76,  -238, -1000,   185,
 -1000,  -184, -1000,     7,   -79,  -127,   -81,   -49, -1000,   -50,
 -1000,    99, -1000, -1000, -1000,   -64,     7, -1000, -1000, -1000,
   185, -1000,  -254,     7, -1000, -1000,   184,   -90, -1000, -1000,
  -103,  -103, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000,   185,   187,   185,   155, -1000,   155,   441,
   441,   155,  -105,   429,   185,   155,    -2,   413,   183,    99,
     7,   155,   155,    99,   382,   373,   155,   155,   -24,   358,
   182,   155, -1000,   343,   155,   224, -1000 };
yytabelem yypgo[]={

     0,   411,   409,   408,   407,   406,   404,   398,   397,   394,
   393,   391,   389,   382,   381,   379,   376,   513,   375,   368,
     0,   128,   214,   367,   366,   365,   364,   358,   216,   356,
   215,   355,   218,   353,   219,     7,   352,   349,   347,   345,
    39,   344,   337,   336,    22,    17,    40,   230,   326,   321,
   320,   221,   254,   220,   302,   232,   294,   290,   289,   288,
   281,   277,   276,   274,   273,   272,   270,   235,   259,   257,
   252,   250,   249,   248,   217,   228,   253 };
yytabelem yyr1[]={

     0,     1,     1,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    18,     3,    19,
    19,    21,    21,    20,    20,    20,    20,    20,    20,    20,
    20,    23,     4,    24,    24,    25,    26,    26,    27,    27,
    27,    28,    28,    28,    28,    28,    28,    29,     5,    30,
    30,    31,    31,    33,    31,    32,    32,    32,    34,    34,
    34,    34,    34,    34,    34,    34,    34,    37,     6,    17,
    17,    17,    17,    17,    17,    17,    17,    36,    36,    39,
    39,    39,    39,    39,    38,    38,    41,    41,    42,    42,
    42,    42,    42,    42,    42,    42,    42,    42,    42,    42,
    42,    42,    42,    42,    42,    43,    45,    45,    45,    40,
    40,    46,    46,    46,    46,    46,    35,    35,    22,    22,
    47,    48,    44,    44,    44,    49,    50,     8,    51,    52,
    52,    52,    53,    53,    53,    54,     9,    56,     9,    57,
    55,    55,    55,     7,     7,    58,     7,    10,    10,    59,
    10,    11,    11,    60,    11,    12,    13,    14,    61,    61,
    62,    62,    62,    62,    15,    16,    63,    64,    64,    66,
    66,    66,    66,    66,    66,    66,    67,    67,    65,    65,
    73,    73,    74,    74,    75,    75,    68,    69,    70,    71,
    72,    76,    76,    76,    76 };
yytabelem yyr2[]={

     0,     1,     5,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     1,    12,     0,
     9,     0,     2,     3,     7,     7,     7,     7,     7,     7,
     7,     1,    15,     1,     5,    11,     3,     3,     0,     4,
     6,     2,     2,     3,     3,     3,     3,     1,    15,     1,
     5,     5,    11,     1,    17,     1,     3,     7,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     1,    35,     0,
     4,     4,     4,     4,     4,     4,     4,     1,     5,     5,
     5,     5,     5,     9,     1,     5,     5,    35,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,    27,     0,     2,     4,     2,
     6,     3,     3,     5,     5,     6,     1,     5,     3,     3,
     5,     1,     1,     3,     5,     2,     1,    14,     3,     0,
     4,     6,     3,     3,     3,     1,    13,     1,    10,     1,
     8,     3,     3,     9,     9,     1,    15,     9,     9,     1,
    15,     9,     9,     1,    15,     8,     3,    11,     1,     3,
     3,     3,     3,     3,     2,    12,     2,     0,     4,     4,
     4,     4,     4,     4,     4,     4,    14,     6,     0,     4,
    18,    10,     0,     4,     8,    12,     2,     2,     2,     2,
     2,     2,     2,     6,     6 };
yytabelem yychk[]={

 -1000,    -1,    -2,    -3,    -4,    -5,    -6,    -7,    -8,    -9,
   -10,   -11,   -12,   -13,   -14,   -15,   -16,   263,   -22,   314,
   262,   -63,   257,   260,   -17,   264,   266,   276,   292,   -47,
   280,   290,   300,   301,   298,   315,   -48,   329,   -18,   293,
   294,   326,   328,   297,   295,   296,   -17,   -17,   -17,   -17,
   -54,   -17,   -17,   -17,   -17,   -17,   259,   -22,   257,   -64,
   261,   -23,   -29,   -20,   257,    40,   -50,   -17,   -56,   -47,
   259,   261,   -47,   259,   261,   -47,   259,   261,   -47,   259,
   261,   -66,   330,   331,   332,   333,   334,   335,   336,   -19,
   261,   261,    43,    45,    42,    47,    37,   124,    44,   -20,
   261,   -55,   261,   260,   259,   -55,   -58,   -59,   -60,   -61,
   -62,   316,   317,   318,   319,   -65,   -67,   257,   -67,   -68,
   -76,   258,   257,    40,   -69,   -76,   -70,   -76,   -71,   -76,
   -72,   -76,   262,   -20,   -24,   -30,   -20,   -20,   -20,   -20,
   -20,   -20,   -20,    41,   -51,   -52,   -53,   -20,   259,   -49,
    39,   -17,   -57,   -52,   -52,   -52,   262,   -73,   337,    44,
   124,   -76,   -21,    44,   262,   -25,   -26,   259,   257,   262,
   -31,   267,   268,    44,   262,   -21,   -21,   -51,   262,   262,
   262,   259,   257,   -76,    41,   259,   -21,   269,   259,   259,
   -20,   -53,   262,   261,    44,   -20,   -21,   -21,    44,   337,
   -74,   257,   -21,   -20,   -32,   -34,   270,   271,   272,   273,
   274,   275,   320,   324,   325,   -20,   259,   262,   -75,   338,
    44,   -27,   -32,   -33,   -21,   -35,    44,   261,   259,   257,
   -28,   -21,   310,   312,   313,   309,   311,   299,   261,   -34,
   -21,   257,   -74,   -21,   -28,   -30,   -36,   262,   259,   257,
   262,   -37,   -39,   277,   279,   266,   278,   298,   262,   -21,
   261,   -40,   -46,   258,   257,   327,    40,   259,   -22,   259,
   257,   257,   -38,   124,   258,   257,   -40,   -21,   262,   -41,
   -42,   299,   280,   322,   321,   281,   282,   284,   302,   283,
   286,   291,   285,   323,   287,   288,   290,   289,   259,   -46,
    41,   259,   -43,   -44,   259,   -44,   -21,    44,   -21,   -20,
   -20,   -21,   -21,   -20,   259,    44,   -21,   -20,   -45,   -40,
    44,    44,    44,   -40,   -20,   -20,    44,    44,   -20,   -20,
   -45,    44,   -35,   -20,    44,   -20,   -35 };
yytabelem yydef[]={

     1,    -2,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    69,   121,   156,
   164,     0,    -2,   119,    17,    69,    69,    69,    69,   135,
    69,    69,    69,    69,    69,     0,     0,   167,     0,    70,
    71,    72,    73,    74,    75,    76,    31,    47,     0,   126,
    69,   137,   121,   121,   121,   121,     0,   120,   118,     0,
    19,     0,     0,     0,    23,     0,     0,     0,     0,   143,
   144,   145,   147,   148,   149,   151,   152,   153,   155,   158,
   178,   168,     0,     0,     0,     0,     0,     0,     0,     0,
    33,    49,     0,     0,     0,     0,     0,     0,     0,     0,
   129,    69,   139,   141,   142,   138,   129,   129,   129,   157,
   159,   160,   161,   162,   163,     0,   169,     0,   170,   171,
   186,   191,   192,     0,   172,   187,   173,   188,   174,   189,
   175,   190,    18,    21,     0,     0,    24,    25,    26,    27,
    28,    30,     0,    29,     0,    -2,    21,   132,   133,   134,
   125,   136,   129,    21,    21,    21,   165,   179,     0,     0,
     0,     0,     0,    22,    32,    34,    21,    36,    37,    48,
    50,     0,     0,     0,   127,     0,   130,     0,   146,   150,
   154,     0,   177,   193,   194,    20,     0,    51,    21,    21,
     0,   131,   140,   182,     0,    21,     0,    55,     0,     0,
     0,     0,    38,    55,    -2,    56,    58,    59,    60,    61,
    62,    63,    64,    65,    66,   116,     0,   181,   183,     0,
     0,    35,    -2,     0,     0,    21,     0,   182,    21,   176,
    39,     0,    41,    42,    43,    44,    45,    46,    49,    57,
    77,   117,     0,     0,    40,     0,    67,     0,   184,    21,
    54,     0,    78,     0,     0,     0,     0,     0,   180,     0,
    84,    79,   109,   111,   112,     0,     0,    80,    81,    82,
    21,   185,     0,     0,   113,   114,     0,     0,    68,    85,
   122,   122,    88,    89,    90,    91,    92,    93,    94,    95,
    96,    97,    98,    99,   100,   101,   102,   103,   104,   110,
   115,    83,    86,    21,   123,    21,     0,   124,     0,    21,
    21,     0,     0,     0,    21,     0,     0,     0,     0,   107,
     0,     0,     0,   108,     0,     0,     0,     0,   106,     0,
   116,     0,   105,     0,     0,   116,    87 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	1	/* allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"NUMBER",	257,
	"LNUMBER",	258,
	"STRING",	259,
	"ID",	260,
	"BEGIN",	261,
	"END",	262,
	"STRINGTABLE",	263,
	"ACCELERATORS",	264,
	"KEYCODE",	265,
	"MENU",	266,
	"MENUITEM",	267,
	"POPUP",	268,
	"SEPARATOR",	269,
	"MENUBREAK",	270,
	"CHECKED",	271,
	"INACTIVE",	272,
	"GRAYED",	273,
	"HELP",	274,
	"SHADOW",	275,
	"DIALOG",	276,
	"STYLE",	277,
	"CLASS",	278,
	"CAPTION",	279,
	"TEXT",	280,
	"EDIT",	281,
	"CHECKBOX",	282,
	"RADIOBUTTON",	283,
	"PUSHBUTTON",	284,
	"STATIC",	285,
	"LISTBOX",	286,
	"FRAME",	287,
	"BOX",	288,
	"SCROLLBAR",	289,
	"ICON",	290,
	"COMBOBOX",	291,
	"RCDATA",	292,
	"PRELOAD",	293,
	"LOADONCALL",	294,
	"MOVEABLE",	295,
	"DISCARDABLE",	296,
	"FIXED",	297,
	"FONT",	298,
	"CONTROL",	299,
	"BITMAP",	300,
	"CURSOR",	301,
	"DEFPUSHBUTTON",	302,
	"IF",	303,
	"IFDEF",	304,
	"IFNDEF",	305,
	"ELSE",	306,
	"ELIF",	307,
	"ENDIF",	308,
	"ALT",	309,
	"ASCII",	310,
	"SHIFT",	311,
	"VIRTKEY",	312,
	"NOINVERT",	313,
	"RCINCLUDE",	314,
	"MSGBOX",	315,
	"OK",	316,
	"OKCANCEL",	317,
	"YESNO",	318,
	"YESNOCANCEL",	319,
	"MENUBARBREAK",	320,
	"CTEXT",	321,
	"RTEXT",	322,
	"GROUPBOX",	323,
	"UNCHECKED",	324,
	"ENABLED",	325,
	"PURE",	326,
	"NOT",	327,
	"IMPURE",	328,
	"VERSIONINFO",	329,
	"FILEVERSION",	330,
	"PRODUCTVERSION",	331,
	"FILEFLAGSMASK",	332,
	"FILEFLAGS",	333,
	"FILEOS",	334,
	"FILETYPE",	335,
	"FILESUBTYPE",	336,
	"BLOCK",	337,
	"VALUE",	338,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
      "objlist : /* empty */",
      "objlist : objlist obj",
      "obj : stringtbl",
      "obj : acceltbl",
      "obj : menu",
      "obj : dialog",
      "obj : icon",
      "obj : rcdata",
      "obj : userres",
      "obj : bitmap",
      "obj : cursor",
      "obj : font",
      "obj : rcinclude",
      "obj : msgbox",
      "obj : end",
      "obj : versioninfo",
      "stringtbl : STRINGTABLE $loadopts",
      "stringtbl : STRINGTABLE $loadopts BEGIN stringlist END",
      "stringlist : /* empty */",
      "stringlist : stringlist nexpr $comma STRING",
      "$comma : /* empty */",
      "$comma : ','",
      "nexpr : NUMBER",
      "nexpr : nexpr '+' nexpr",
      "nexpr : nexpr '-' nexpr",
      "nexpr : nexpr '*' nexpr",
      "nexpr : nexpr '/' nexpr",
      "nexpr : nexpr '%' nexpr",
      "nexpr : '(' nexpr ')'",
      "nexpr : nexpr '|' nexpr",
      "acceltbl : resID ACCELERATORS $loadopts",
      "acceltbl : resID ACCELERATORS $loadopts BEGIN accellist END",
      "accellist : /* empty */",
      "accellist : accellist accel",
      "accel : accelevent $comma nexpr $comma $accelopts",
      "accelevent : STRING",
      "accelevent : NUMBER",
      "$accelopts : /* empty */",
      "$accelopts : $accelopts accelopt",
      "$accelopts : $accelopts $comma accelopt",
      "accelopt : ASCII",
      "accelopt : VIRTKEY",
      "accelopt : NOINVERT",
      "accelopt : ALT",
      "accelopt : SHIFT",
      "accelopt : CONTROL",
      "menu : resID MENU $loadopts",
      "menu : resID MENU $loadopts BEGIN menuitems END",
      "menuitems : /* empty */",
      "menuitems : menuitems menuitem",
      "menuitem : MENUITEM SEPARATOR",
      "menuitem : MENUITEM STRING $comma nexpr miopts",
      "menuitem : POPUP STRING $comma miopts",
      "menuitem : POPUP STRING $comma miopts BEGIN menuitems END",
      "miopts : /* empty */",
      "miopts : miopt",
      "miopts : miopts $comma miopt",
      "miopt : MENUBREAK",
      "miopt : CHECKED",
      "miopt : INACTIVE",
      "miopt : GRAYED",
      "miopt : HELP",
      "miopt : SHADOW",
      "miopt : MENUBARBREAK",
      "miopt : UNCHECKED",
      "miopt : ENABLED",
      "dialog : resID DIALOG $loadopts nexpr ',' nexpr ',' nexpr ',' nexpr $color $comma dlgopts",
      "dialog : resID DIALOG $loadopts nexpr ',' nexpr ',' nexpr ',' nexpr $color $comma dlgopts BEGIN dlgcontrols END",
      "$loadopts : /* empty */",
      "$loadopts : $loadopts PRELOAD",
      "$loadopts : $loadopts LOADONCALL",
      "$loadopts : $loadopts PURE",
      "$loadopts : $loadopts IMPURE",
      "$loadopts : $loadopts FIXED",
      "$loadopts : $loadopts MOVEABLE",
      "$loadopts : $loadopts DISCARDABLE",
      "dlgopts : /* empty */",
      "dlgopts : dlgopts dlgopt",
      "dlgopt : STYLE ctrlstyles",
      "dlgopt : CAPTION STRING",
      "dlgopt : MENU resID",
      "dlgopt : CLASS STRING",
      "dlgopt : FONT NUMBER $comma STRING",
      "dlgcontrols : /* empty */",
      "dlgcontrols : dlgcontrols dlgcontrol",
      "dlgcontrol : ctrltype ctrlinfo",
      "dlgcontrol : CONTROL $string $comma nexpr $comma STRING $comma $ctrlstyles ',' nexpr ',' nexpr ',' nexpr ',' nexpr $color",
      "ctrltype : TEXT",
      "ctrltype : RTEXT",
      "ctrltype : CTEXT",
      "ctrltype : EDIT",
      "ctrltype : CHECKBOX",
      "ctrltype : PUSHBUTTON",
      "ctrltype : DEFPUSHBUTTON",
      "ctrltype : RADIOBUTTON",
      "ctrltype : LISTBOX",
      "ctrltype : COMBOBOX",
      "ctrltype : STATIC",
      "ctrltype : GROUPBOX",
      "ctrltype : FRAME",
      "ctrltype : BOX",
      "ctrltype : ICON",
      "ctrltype : SCROLLBAR",
      "ctrltype : STRING",
      "ctrlinfo : $string $comma nexpr $comma nexpr ',' nexpr ',' nexpr ',' nexpr $ctrlstyles $color",
      "$ctrlstyles : /* empty */",
      "$ctrlstyles : ctrlstyles",
      "$ctrlstyles : ',' ctrlstyles",
      "ctrlstyles : ctrlstyle",
      "ctrlstyles : ctrlstyles '|' ctrlstyle",
      "ctrlstyle : LNUMBER",
      "ctrlstyle : NUMBER",
      "ctrlstyle : NOT LNUMBER",
      "ctrlstyle : NOT NUMBER",
      "ctrlstyle : '(' ctrlstyles ')'",
      "$color : /* empty */",
      "$color : ',' NUMBER",
      "resID : NUMBER",
      "resID : ID",
      "literal_resID : $literal_resID resID",
      "$literal_resID : /* empty */",
      "$string : /* empty */",
      "$string : STRING",
      "$string : STRING ','",
      "quote : '\''",
      "rcdata : resID RCDATA $loadopts",
      "rcdata : resID RCDATA $loadopts BEGIN $rcdata END",
      "$rcdata : rawdatalist",
      "rawdatalist : /* empty */",
      "rawdatalist : rawdata $comma",
      "rawdatalist : rawdatalist $comma rawdata",
      "rawdata : nexpr",
      "rawdata : STRING",
      "rawdata : quote",
      "userres : resID literal_resID",
      "userres : resID literal_resID $loadopts $userres $loadopts",
      "userres : resID TEXT $loadopts",
      "userres : resID TEXT $loadopts $userres",
      "$userres : BEGIN",
      "$userres : BEGIN $rcdata END",
      "$userres : ID",
      "$userres : STRING",
      "icon : resID ICON $loadopts literal_resID",
      "icon : resID ICON $loadopts STRING",
      "icon : resID ICON $loadopts BEGIN",
      "icon : resID ICON $loadopts BEGIN rawdatalist END",
      "bitmap : resID BITMAP $loadopts literal_resID",
      "bitmap : resID BITMAP $loadopts STRING",
      "bitmap : resID BITMAP $loadopts BEGIN",
      "bitmap : resID BITMAP $loadopts BEGIN rawdatalist END",
      "cursor : resID CURSOR $loadopts literal_resID",
      "cursor : resID CURSOR $loadopts STRING",
      "cursor : resID CURSOR $loadopts BEGIN",
      "cursor : resID CURSOR $loadopts BEGIN rawdatalist END",
      "font : resID FONT $loadopts literal_resID",
      "rcinclude : RCINCLUDE",
      "msgbox : resID MSGBOX STRING STRING msgboxtp",
      "msgboxtp : /* empty */",
      "msgboxtp : msgopt",
      "msgopt : OK",
      "msgopt : OKCANCEL",
      "msgopt : YESNO",
      "msgopt : YESNOCANCEL",
      "end : END",
      "versioninfo : versionid VERSIONINFO fixedinfo BEGIN blocks END",
      "versionid : NUMBER",
      "fixedinfo : /* empty */",
      "fixedinfo : fixedinfo fixedinfoitem",
      "fixedinfoitem : FILEVERSION vernum",
      "fixedinfoitem : PRODUCTVERSION vernum",
      "fixedinfoitem : FILEFLAGSMASK fileflagmask",
      "fixedinfoitem : FILEFLAGS fileflags",
      "fixedinfoitem : FILEOS fileos",
      "fixedinfoitem : FILETYPE filetype",
      "fixedinfoitem : FILESUBTYPE filesubtype",
      "vernum : NUMBER ',' NUMBER ',' NUMBER ',' NUMBER",
      "vernum : NUMBER ',' NUMBER",
      "blocks : /* empty */",
      "blocks : blocks blockstatement",
      "blockstatement : BLOCK STRING BEGIN BLOCK STRING BEGIN valuelist END END",
      "blockstatement : BLOCK STRING BEGIN valuelist END",
      "valuelist : /* empty */",
      "valuelist : valuelist value",
      "value : VALUE STRING $comma STRING",
      "value : VALUE STRING $comma NUMBER $comma NUMBER",
      "fileflagmask : lexpr",
      "fileflags : lexpr",
      "fileos : lexpr",
      "filetype : lexpr",
      "filesubtype : lexpr",
      "lexpr : LNUMBER",
      "lexpr : NUMBER",
      "lexpr : lexpr '|' lexpr",
      "lexpr : '(' lexpr ')'",
};
#endif /* YYDEBUG */
/*      @(#)yaccpar     1.9     */
 
/*
** Skeleton parser driver for yacc output
*/
 
/*
** yacc user known macros and defines
*/
#define YYERROR         goto yyerrlab
#define YYACCEPT        return(0)
#define YYABORT         return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
        if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
        {\
                yyerror( "syntax error - cannot backup", NULL );\
                goto yyerrlab;\
        }\
        yychar = newtoken;\
        yystate = *yyps;\
        yylval = newvalue;\
        goto yynewstate;\
}
#define YYRECOVERING()  (!!yyerrflag)
#ifndef YYDEBUG
#       define YYDEBUG  1       /* make debugging available */
#endif
 
/*
** user known globals
*/
int yydebug;                    /* set to 1 to get debugging */
 
/*
** driver internal defines
*/
#define YYFLAG          (-1000)
 
/*
** global variables used by the parser
*/
YYSTYPE yyv[ YYMAXDEPTH ];      /* value stack */
int yys[ YYMAXDEPTH ];          /* state stack */
char *yydisplay(int);
 
YYSTYPE *yypv;                  /* top of value stack */
int *yyps;                      /* top of state stack */
 
int yystate;                    /* current state */
int yytmp;                      /* extra var (lasts between blocks) */
 
int yynerrs;                    /* number of errors */
int yyerrflag;                  /* error recovery flag */
int yychar;                     /* current input token number */
 
 
 
/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
        register YYSTYPE *yypvt;        /* top of value stack for $vars */
 
        /*
        ** Initialize externals - yyparse may be called more than once
        */
        yypv = &yyv[-1];
        yyps = &yys[-1];
        yystate = 0;
        yytmp = 0;
        yynerrs = 0;
        yyerrflag = 0;
        yychar = -1;
 
        goto yystack;
        {
                register YYSTYPE *yy_pv;        /* top of value stack */
                register int *yy_ps;            /* top of state stack */
                register int yy_state;          /* current state */
                register int  yy_n;             /* internal state number info */
 
                /*
                ** get globals into registers.
                ** branch to here only if YYBACKUP was called.
                */
        yynewstate:
                yy_pv = yypv;
                yy_ps = yyps;
                yy_state = yystate;
                goto yy_newstate;
 
                /*
                ** get globals into registers.
                ** either we just started, or we just finished a reduction
                */
        yystack:
                yy_pv = yypv;
                yy_ps = yyps;
                yy_state = yystate;
 
                /*
                ** top of for (;;) loop while no reductions done
                */
        yy_stack:
                /*
                ** put a state and value onto the stacks
                */
#if YYDEBUG
                /*
                ** if debugging, look up token value in list of value vs.
                ** name pairs.  0 and negative (-1) are special values.
                ** Note: linear search is used since time is not a real
                ** consideration while debugging.
                */
                if ( yydebug )
                {
                        register int yy_i;
 
                        printf( "State %d, token ", yy_state );
                        if ( yychar == 0 )
                                printf( "end-of-file\n" );
                        else if ( yychar < 0 )
                                printf( "-none-\n" );
                        else
                        {
                                for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
                                        yy_i++ )
                                {
                                        if ( yytoks[yy_i].t_val == yychar )
                                                break;
                                }
                                printf( "%s\n", yytoks[yy_i].t_name );
                        }
                }
#endif /* YYDEBUG */
                if ( ++yy_ps >= &yys[ YYMAXDEPTH ] )    /* room on stack? */
                {
                        yyerror( "yacc stack overflow", NULL );
                        YYABORT;
                }
                *yy_ps = yy_state;
                *++yy_pv = yyval;
 
                /*
                ** we have a new state - find out what to do
                */
        yy_newstate:
                if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
                        goto yydefault;         /* simple state */
#if YYDEBUG
                /*
                ** if debugging, need to mark whether new token grabbed
                */
                yytmp = yychar < 0;
#endif
                if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
                        yychar = 0;             /* reached EOF */
#if YYDEBUG
                if ( yydebug && yytmp )
                {
                        register int yy_i;
 
                        printf( "Received token " );
                        if ( yychar == 0 )
                                printf( "end-of-file\n" );
                        else if ( yychar < 0 )
                                printf( "-none-\n" );
                        else
                        {
                                for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
                                        yy_i++ )
                                {
                                        if ( yytoks[yy_i].t_val == yychar )
                                                break;
                                }
                                printf( "%s\n", yytoks[yy_i].t_name );
                        }
                }
#endif /* YYDEBUG */
                if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
                        goto yydefault;
                if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )  /*valid shift*/
                {
                        yychar = -1;
                        yyval = yylval;
                        yy_state = yy_n;
                        if ( yyerrflag > 0 )
                                yyerrflag--;
                        goto yy_stack;
                }
 
        yydefault:
                if ( ( yy_n = yydef[ yy_state ] ) == -2 )
                {
#if YYDEBUG
                        yytmp = yychar < 0;
#endif
                        if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
                                yychar = 0;             /* reached EOF */
#if YYDEBUG
                        if ( yydebug && yytmp )
                        {
                                register int yy_i;
 
                                printf( "Received token " );
                                if ( yychar == 0 )
                                        printf( "end-of-file\n" );
                                else if ( yychar < 0 )
                                        printf( "-none-\n" );
                                else
                                {
                                        for ( yy_i = 0;
                                                yytoks[yy_i].t_val >= 0;
                                                yy_i++ )
                                        {
                                                if ( yytoks[yy_i].t_val
                                                        == yychar )
                                                {
                                                        break;
                                                }
                                        }
                                        printf( "%s\n", yytoks[yy_i].t_name );
                                }
                        }
#endif /* YYDEBUG */
                        /*
                        ** look through exception table
                        */
                        {
                                register int *yyxi = yyexca;
 
                                while ( ( *yyxi != -1 ) ||
                                        ( yyxi[1] != yy_state ) )
                                {
                                        yyxi += 2;
                                }
                                while ( ( *(yyxi += 2) >= 0 ) &&
                                        ( *yyxi != yychar ) )
                                        ;
                                if ( ( yy_n = yyxi[1] ) < 0 )
                                        YYACCEPT;
                        }
                }
 
                /*
                ** check for syntax error
                */
                if ( yy_n == 0 )        /* have an error */
                {
                        /* no worry about speed here! */
                        switch ( yyerrflag )
                        {
                        int yyn;
                        case 0:         /* new error */
    if ((yyn = yypact[yystate]) > YYFLAG && yyn < YYLAST)
    {
      register int x;
      for (x = (yyn > 0) ? yyn : 0;  x < YYLAST;  x++)
        if (yychk[yyact[x]] == x - yyn && x - yyn != YYERRCODE)
          yyerror(NULL, yydisplay(x - yyn));
    }
    yyerror(NULL, NULL);
                                goto skip_init;
                        yyerrlab:
                                /*
                                ** get globals into registers.
                                ** we have a user generated syntax type error
                                */
                                yy_pv = yypv;
                                yy_ps = yyps;
                                yy_state = yystate;
                                yynerrs++;
                        skip_init:
                        case 1:
                        case 2:         /* incompletely recovered error */
                                        /* try again... */
                                yyerrflag = 3;
                                /*
                                ** find state where "error" is a legal
                                ** shift action
                                */
                                while ( yy_ps >= yys )
                                {
                                        yy_n = yypact[ *yy_ps ] + YYERRCODE;
                                        if ( yy_n >= 0 && yy_n < YYLAST &&
                                                yychk[yyact[yy_n]] == YYERRCODE)                                        {
                                                /*
                                                ** simulate shift of "error"
                                                */
                                                yy_state = yyact[ yy_n ];
                                                goto yy_stack;
                                        }
                                        /*
                                        ** current state has no shift on
                                        ** "error", pop stack
                                        */
#if YYDEBUG
#       define _POP_ "Error recovery pops state %d, uncovers state %d\n"
                                        if ( yydebug )
                                                printf( _POP_, *yy_ps,
                                                        yy_ps[-1] );
#       undef _POP_
#endif
                                        yy_ps--;
                                        yy_pv--;
                                }
                                /*
                                ** there is no state on stack with "error" as
                                ** a valid shift.  give up.
                                */
                                YYABORT;
                        case 3:         /* no shift yet; eat a token */
#if YYDEBUG
                                /*
                                ** if debugging, look up token in list of
                                ** pairs.  0 and negative shouldn't occur,
                                ** but since timing doesn't matter when
                                ** debugging, it doesn't hurt to leave the
                                ** tests here.
                                */
                                if ( yydebug )
                                {
                                        register int yy_i;
 
                                        printf( "Error recovery discards " );
                                        if ( yychar == 0 )
                                                printf( "token end-of-file\n" );
                                        else if ( yychar < 0 )
                                                printf( "token -none-\n" );
                                        else
                                        {
                                                for ( yy_i = 0;
                                                        yytoks[yy_i].t_val >= 0;
                                                        yy_i++ )
                                                {
                                                        if ( yytoks[yy_i].t_val
                                                                == yychar )
                                                        {
                                                                break;
                                                        }
                                                }
                                                printf( "token %s\n",
                                                        yytoks[yy_i].t_name );
                                        }
                                }
#endif /* YYDEBUG */
                                if ( yychar == 0 )      /* reached EOF. quit */
                                        YYABORT;
                                yychar = -1;
                                goto yy_newstate;
                        }
                }/* end if ( yy_n == 0 ) */
                /*
                ** reduction by production yy_n
                ** put stack tops, etc. so things right after switch
                */
#if YYDEBUG
                /*
                ** if debugging, print the string that is the user's
                ** specification of the reduction which is just about
                ** to be done.
                */
                if ( yydebug )
                        printf( "Reduce by (%d) \"%s\"\n",
                                yy_n, yyreds[ yy_n ] );
#endif
                yytmp = yy_n;                   /* value to switch over */
                yypvt = yy_pv;                  /* $vars top of value stack */
                /*
                ** Look in goto table for next state
                ** Sorry about using yy_state here as temporary
                ** register variable, but why not, if it works...
                ** If yyr2[ yy_n ] doesn't have the low order bit
                ** set, then there is no action to be done for
                ** this reduction.  So, no saving & unsaving of
                ** registers done.  The only difference between the
                ** code just after the if and the body of the if is
                ** the goto yy_stack in the body.  This way the test
                ** can be made before the choice of what to do is needed.
                */
                {
                        /* length of production doubled with extra bit */
                        register int yy_len = yyr2[ yy_n ];
 
                        if ( !( yy_len & 01 ) )
                        {
                                yy_len >>= 1;
                                yyval = ( yy_pv -= yy_len )[1]; /* $$ = $1 */
                                yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
                                        *( yy_ps -= yy_len ) + 1;
                                if ( yy_state >= YYLAST ||
                                        yychk[ yy_state =
                                        yyact[ yy_state ] ] != -yy_n )
                                {
                                        yy_state = yyact[ yypgo[ yy_n ] ];
                                }
                                goto yy_stack;
                        }
                        yy_len >>= 1;
                        yyval = ( yy_pv -= yy_len )[1]; /* $$ = $1 */
                        yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
                                *( yy_ps -= yy_len ) + 1;
                        if ( yy_state >= YYLAST ||
                                yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
                        {
                                yy_state = yyact[ yypgo[ yy_n ] ];
                        }
                }
                                        /* save until reenter driver code */
                yystate = yy_state;
                yyps = yy_ps;
                yypv = yy_pv;
        }
        /*
        ** code supplied by user is placed in this switch
        */
        switch( yytmp )
        {
                
case 1:{
             bExpectingRESID = 1;
             memset((char *) &StringTblHeader, '\0', sizeof(StringTblHeader));
             StringTblHeader.ulSignature = RC_SIGNATURE;
             nwrite(resFD, (char *) &StringTblHeader, sizeof(StringTblHeader));
           } break;
case 2:{
             bExpectingRESID = (bExpectingRESID == JUST_DID_USERRES) ? 0 : 1;
           } break;
case 17:{
             bSawStringtable = TRUE;
             StringInfoInit();
           } break;
case 20:{
              StringInfoAddString(yypvt[-0], yypvt[-2]);
#ifdef RC_TRANS
              AddString(Symtab[yypvt[-0]].u.sval, (WORD) yypvt[-2]);
#endif
            } break;
case 23:{
             yyval = yypvt[-0];        /* return the id */
           } break;
case 24:{
             yyval = yypvt[-2] + yypvt[-0];
           } break;
case 25:{
             yyval = yypvt[-2] - yypvt[-0];
           } break;
case 26:{
             yyval = yypvt[-2] * yypvt[-0];
           } break;
case 27:{
             yyval = yypvt[-2] / yypvt[-0];
           } break;
case 28:{
             yyval = yypvt[-2] % yypvt[-0];
           } break;
case 29:{
             yyval = yypvt[-1];
           } break;
case 30:{
             yyval = yypvt[-2] | yypvt[-0];
           } break;
case 31:{
             fposStartOfResource = WriteResourceHeader(RT_ACCELERATOR, yypvt[-2]);
           } break;
case 32:{
             DWORD nBytes;
             BYTE  fFlags;

             /*
               We need to terminate the accelerator table by 
               OR'ing the last accelerator entry's flags with 0x80.
             */
             lseek(resFD, fposResource, 0);
             fFlags = (BYTE) (PrevAccelState | STATE_END);
             nwrite(resFD, (char *) &fFlags, sizeof(fFlags));

             /*
               Write out the size of the resource
             */
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
#ifdef RC_TRANS
             AddAcceleratorTable(Symtab[-(yypvt[-6])].name);
#endif
           } break;
case 33:{
             yyval = 0;
             CurrAccelState = 0;
           } break;
case 34:{
             yyval = yypvt[-1] + 1;
             CurrAccelState = 0;
           } break;
case 35:{
             WORD key, cmd;
             BYTE fFlags;
             int  iKey;

             key = yypvt[-4];  cmd = yypvt[-2];

             /*
               If iKey is less than 0, then the user specified a
               key string. Check the first character to see if
               it is a '^', and if it is, controlify the letter.
             */
             if (bKeyIsString)
             {
               char *szKey;

               iKey = (int) yypvt[-4];  /* for signed compares .... */
               szKey = Symtab[-iKey].u.sval;
               if ((key = szKey[0]) == '^' && szKey[1] != '\0')
                 key = szKey[1] & 0x1F;
             }

             /*
               Add any of the VIRTKEY,CONTROL,SHIFT,ALT modifiers
               to the key.
             */
             if (CurrAccelState != 0)
               key = AddShiftToKey(key, CurrAccelState);

             /*
               Write out the accelerator table entry....
                 The flags followed by the key followed by the command id.
             */
             fposResource = tell(resFD);
             fFlags = (BYTE) CurrAccelState;
             nwrite(resFD, (char *) &fFlags, sizeof(fFlags));
#ifdef sun
             {
             char szTmp[3];
             nwrite(resFD, szTmp, 3); /* pad to 4 bytes boundary for sun */
             }
#endif
             nwrite(resFD, (char *) &key, sizeof(key));
             nwrite(resFD, (char *) &cmd, sizeof(cmd));

             bKeyIsString = FALSE;
             PrevAccelState = CurrAccelState;

#ifdef RC_TRANS
             AddAcceleratorKey(key, cmd);
#endif
           } break;
case 36:{
             yyval = -(yypvt[-0]);
             bKeyIsString = TRUE;
           } break;
case 37:{
             yyval = yypvt[-0];
             bKeyIsString = FALSE;
           } break;
case 43:{ CurrAccelState |= STATE_NOINVERT; } break;
case 44:{ CurrAccelState |= STATE_ALT;      } break;
case 45:{ CurrAccelState |= STATE_SHIFT;    } break;
case 46:{ CurrAccelState |= STATE_CONTROL;  } break;
case 47:{
#ifdef USE_WINDOWS_MENU
             MENUITEMTEMPLATEHEADER mit;
             fposStartOfResource = WriteResourceHeader(RT_MENU, yypvt[-2]);
             mit.versionNumber = mit.offset = 0;
             nwrite(resFD, (char *) &mit, sizeof(mit));
#else
             WORD nmenu = 0;
             fposStartOfResource = WriteResourceHeader(RT_MENU, yypvt[-2]);
             push(fposResource = tell(resFD));
             nwrite(resFD, (char *) &nmenu, sizeof(nmenu));
#endif
           } break;
case 48:{
             WORD nItems = yypvt[-1];
             DWORD nBytes;

#ifdef USE_WINDOWS_MENU
             WriteMenuTree(yypvt[-1]);
             FreeMenuTree();
#else
             /*
               Backtrack and fill in the number of menu items
             */
             fposResource = pop();
             lseek(resFD, fposResource, 0);
             nwrite(resFD, (char *) &nItems, sizeof(nItems));
#endif

             /*
               Record the position of the end of the menu resource
             */
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);

             /*
               Fill in the number of bytes in the menu resource
             */
             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

             /*
               Return back to the end of the file
             */
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);

#ifdef RC_TRANS
             AddMenu(Symtab[-(yypvt[-6])].name);
#endif
           } break;
case 49:{
             yyval = 0;
           } break;
case 50:{
             yyval = yypvt[-1] + 1;
#ifdef USE_WINDOWS_MENU
             yyval = LinkMenuNodeToSibling(yypvt[-1], yypvt[-0]);
#endif
           } break;
case 51:{
#ifdef USE_WINDOWS_MENU
             yyval = CreateMenuNode(0, 0, NULL);
#else
             MTIheader.style  = MF_SEPARATOR; 
             MTIheader.attr   = 0;
             MTIheader.idItem = 0;
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_SEPARATOR, 0, 0, NULL);
#endif
           } break;
case 52:{
             char *str = Symtab[yypvt[-3]].u.sval;
             WORD len = word_pad(strlen(str));

             WindowizeString(str);

#ifdef USE_WINDOWS_MENU
             yyval = CreateMenuNode(yypvt[-0] | MF_STRING, yypvt[-1], str);
#else
             MTIheader.style  = MF_STRING; 
             MTIheader.attr   = yypvt[-0];
             MTIheader.idItem = yypvt[-1];
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
             nwrite(resFD, (char *) &len, sizeof(len));
             nwrite(resFD, (char *) str, len);
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_STRING, yypvt[-0], yypvt[-1], str);
#endif
           } break;
case 53:{
             char *str = Symtab[yypvt[-2]].u.sval;
             WORD len = word_pad(strlen(str));
             WORD nItems = 0;

             WindowizeString(str);

#ifdef USE_WINDOWS_MENU
             yyval = CreateMenuNode(yypvt[-0] | MF_POPUP, 0, str);
#else
             MTIheader.style  = MF_POPUP; 
             MTIheader.attr   = yypvt[-0];
             MTIheader.idItem = 0;
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
             nwrite(resFD, (char *) &len, sizeof(len));
             nwrite(resFD, (char *) str, len);

             fposResource = tell(resFD);
             push(fposResource);
             nwrite(resFD, (char *) &nItems, sizeof(nItems));
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_POPUP, yypvt[-0], 0, str);
#endif
           } break;
case 54:{
#ifdef USE_WINDOWS_MENU
             yyval = LinkMenuNodeToParent(yypvt[-3], yypvt[-1]);
#else
             /*
               Fill in the number of items for this submenu
             */
             WORD nItems = yypvt[-1];
             fposResource = pop();
             lseek(resFD, fposResource, 0);
             nwrite(resFD, (char *) &nItems, sizeof(nItems));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
#endif

#ifdef RC_TRANS
             EndPopup();
#endif
           } break;
case 55:{
             yyval = 0x0000;
           } break;
case 56:{
             yyval = yypvt[-0];
           } break;
case 57:{
             yyval = yypvt[-2] | yypvt[-0];
           } break;
case 58:{ yyval = MF_MENUBREAK; } break;
case 59:{ yyval = MF_CHECKED;   } break;
case 60:{ yyval = MF_DISABLED;  } break;
case 61:{ yyval = MF_DISABLED;  } break;
case 62:{ yyval = MF_HELP;      } break;
case 63:{ yyval = MF_SHADOW;    } break;
case 64:{ yyval = 0;            } break;
case 65:{ yyval = MF_UNCHECKED; } break;
case 66:{ yyval = MF_ENABLED;   } break;
case 67:{
             WORD  nItems = 0;
             WORD  len;
             DWORD dwStyle;
             char *szDlg = NULL;
             char *pszClass = NULL;
             BYTE  bnItems;

             fposStartOfResource = WriteResourceHeader(RT_DIALOG, yypvt[-12]);
             push(fposResource = tell(resFD));

             /*
               If we are processing a Windows dialog box, then they all
               have borders.
             */
#if 0
             /*
               Ooops - Windows dialog boxes do not have to have a
               border!
             */
             if (xTranslated || yTranslated || bWindowsCompatDlg)
               CurrDlgStyle |= WS_BORDER;
#endif

             dwStyle = CurrDlgStyle;
             nwrite(resFD, (char *) &dwStyle, sizeof(DWORD));  /* style */
             bnItems = (BYTE) nItems;
             nwrite(resFD, (char *) &bnItems, sizeof(bnItems));/* nItems */

#ifdef sun
             {
             char szTmp[3];
             nwrite(resFD, szTmp, 3); /* pad to 4 bytes boundary for sun */
             }
#endif

             if (xTranslated && cxTranslated == 0)
               cxTranslated = xTranslated;
             if (yTranslated && cyTranslated == 0)
               cyTranslated = yTranslated;

             xDlg = (xTranslated) ? ((yypvt[-9] + xTranslated/2-iRounding) / xTranslated) : yypvt[-9];
             yDlg = (yTranslated) ? ((yypvt[-7] + yTranslated/2-iRounding) / yTranslated) : yypvt[-7];
             cxDlg = (xTranslated) ? ((yypvt[-5] + xTranslated/2-iRounding) / xTranslated) : yypvt[-5];
             cyDlg = (yTranslated) ? ((yypvt[-3] + yTranslated/2-iRounding) / yTranslated) : yypvt[-3];
             if (xTranslated && !bNoClipping)
             {
               xDlg = max(xDlg, 0);
               xDlg = min(xDlg, 78);
               cxDlg = min(cxDlg+2, 80);  /* +2 for the borders */
             }
             if (yTranslated && !bNoClipping)
             {
               yDlg = max(yDlg, 0);
               yDlg = min(yDlg, 22);
               cyDlg = min(cyDlg+2, 25);  /* +2 for the borders */
             }
             xDlgOrig = xDlg;
             yDlgOrig = yDlg;
             nwrite(resFD, (char *) &xDlg, sizeof(xDlg));      /* x  */
             nwrite(resFD, (char *) &yDlg, sizeof(yDlg));      /* y  */
             nwrite(resFD, (char *) &cxDlg, sizeof(cxDlg));    /* cx */
             nwrite(resFD, (char *) &cyDlg, sizeof(cyDlg));    /* cy */
             if (bEchoTranslation)
             {
               printf("\nDialog Box [%d %d %d %d] to [%d %d %d %d]\n",
                      yypvt[-9], yypvt[-7], yypvt[-5], yypvt[-3],
                      xDlg, yDlg, cxDlg, cyDlg);
             }

             /*
               Write out the menu name
             */
             if (iDlgMenu == 0)
             {
               BYTE ch = '\0';
               len = word_pad(1);
               nwrite(resFD, &ch, len); 
             }
             else
             {
               char *pszMenu = (iDlgMenu > 0) ? Symtab[iDlgMenu].u.sval
                                              : Symtab[-iDlgMenu].name;
               len = word_pad(strlen(pszMenu)+1);
               nwrite(resFD, pszMenu, len); 
             }

             /*
               Write out the class name
             */
             if (iDlgClass >= 0)
             {
               pszClass = Symtab[iDlgClass].u.sval;
               len = word_pad(strlen(pszClass)+1);
               nwrite(resFD, pszClass, len); 
             }
             else
             {
               BYTE ch = '\0';
               len = word_pad(1);
               nwrite(resFD, &ch, len); 
             }

             /*
               Write out the caption name
             */
             if (iDlgCaption >= 0)
             {
               szDlg = Symtab[iDlgCaption].u.sval;
               len = word_pad(strlen(szDlg)+1);
               nwrite(resFD, szDlg, len); 
             }
             else
             {
               BYTE ch = '\0';
               len = word_pad(1);
               nwrite(resFD, &ch, len); 
             }


             /*
               If DS_SETFONT is specified, then we must output a FONTINFO
               structure which looks like this :
                 struct fontinfo
                 {
                   short int pointSize;
                   char      szTypeFace[];  (null-terminated string)
                 }
             */
             if (CurrDlgStyle & DS_SETFONT)
             {
               if (iDlgPtSize)
               {
                 char *szFont;

                 nwrite(resFD, &iDlgPtSize, sizeof(iDlgPtSize));
                 szFont = Symtab[iDlgFont].u.sval;
                 len = word_pad(strlen(szFont)+1);
                 nwrite(resFD, szFont, len); 
               }
               else
               {
                 BYTE ch = '\0';
                 iDlgPtSize = 8;
                 nwrite(resFD, &iDlgPtSize, sizeof(iDlgPtSize));
                 len = word_pad(1);
                 nwrite(resFD, &ch, len); 
               }
             }

             /*
               For dialog boxes which aren't strictly Windows compatible,
               write the optional color attribute. The attribute defaults
               to SYSTEM_COLOR (0xFF).
             */
             if (!bWindowsCompatDlg)
             {
               len = yypvt[-2];
               nwrite(resFD, (char *) &len, sizeof(len));        /* attr */
             }

#ifdef RC_TRANS
             AddDialogBox(xDlg, yDlg, cxDlg, cyDlg, dwStyle, 
                          Symtab[-(yypvt[-12])].name, szDlg, pszClass);
#endif
             CurrDlgStyle = 0L;
             iDlgPtSize   = 0;
             iDlgFont     = 0;
           } break;
case 68:{
             WORD  nItems = yypvt[-1];
             DWORD nBytes;
             BYTE  bnItems;

             fposResource = pop();

             /*
               Go back and fill in the number of control windows
             */
             bnItems = (BYTE) nItems;
             lseek(resFD, fposResource + sizeof(DWORD), 0);
             nwrite(resFD, (char *) &bnItems, sizeof(bnItems));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);

             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
#ifdef RC_TRANS
             EndDialogBox();
#endif
           } break;
case 77:{
             iDlgCaption = -1;
             iDlgClass   = -1;
             iDlgMenu    = 0;
             yyval = 0;
           } break;
case 78:{
             yyval = yypvt[-1] + 1;
           } break;
case 79:{
             CurrDlgStyle = CurrStyle;
             CurrStyle = CurrNotStyle = 0L;
           } break;
case 80:{
             /*
               9/5/92 (maa)
               It seems that Windows adds a WS_CAPTION to a window if
               the CAPTION clause is present, even if WS_CAPTION is
               not specified in the STYLE clause.
             */
             CurrDlgStyle |= WS_CAPTION;
             iDlgCaption = yypvt[-0];
           } break;
case 81:{
             iDlgMenu = yypvt[-0];
           } break;
case 82:{
             iDlgClass = yypvt[-0];
           } break;
case 83:{
             iDlgPtSize = yypvt[-2];
             iDlgFont   = yypvt[-0];
             CurrDlgStyle |= DS_SETFONT;
           } break;
case 84:{
             yyval = 0;
           } break;
case 85:{
             yyval = yypvt[-1] + 1;
           } break;
case 86:{
              CurrStyle = CurrNotStyle = 0;
            } break;
case 87:{
             char *szText;
             WORD len, i, iClass;
             WORD x, y, cx, cy;
             int  ctMask;

             szText = Symtab[yypvt[-15]].u.sval;
             if (!bScreenRelativeCoords)
               xDlg = yDlg = 0;   /* for relative coordinates */

             iClass = StringToClassID(Symtab[yypvt[-11]].u.sval, &CurrStyle, &ctMask);

             /*
               Make all individual scrollbar control SBS_CTL
             */
             if (iClass == SCROLLBAR_CLASS)
               CurrStyle |= SBS_CTL;

             /*
               Now is the time to fix up the coordinates if we are
               processing a dialog box created by Windows.
             */
             x = yypvt[-7];
             y = yypvt[-5];
             cx = yypvt[-3];
             cy = yypvt[-1];
             MEWELizeCoords(&x, &y, &cx, &cy, iClass, szText);

             nwrite(resFD, (char *) &x, sizeof(x));         /* x  */
             nwrite(resFD, (char *) &y, sizeof(y));         /* y  */
             nwrite(resFD, (char *) &cx, sizeof(cx));       /* cx */
             nwrite(resFD, (char *) &cy, sizeof(cy));       /* cy */
             i = yypvt[-13];
             nwrite(resFD, (char *) &i, sizeof(i));         /* id */

             /*
               Write out the style
             */
             CurrStyle |= WS_CHILD;
             if (!bNoBorders)
               if (iClass == PUSHBUTTON_CLASS && cy > 2 && 
                   !(CurrStyle & WS_SHADOW) && 
                    (CurrStyle & 0x0F) != BS_OWNERDRAW)
                 CurrStyle |= WS_BORDER;

             CurrStyle &= ~CurrNotStyle;

             nwrite(resFD, (char *) &CurrStyle, sizeof(CurrStyle));

             /*
               Write out the class name
             */
             if (bUseCTMASK && ctMask)
             {
               unsigned char ch;
               ch = (unsigned char) ctMask;
               len = word_pad(1);
               nwrite(resFD, &ch, len);
             }
             else
             {
               len = word_pad(strlen(Symtab[yypvt[-11]].u.sval) + 1);
               nwrite(resFD, (char *) Symtab[yypvt[-11]].u.sval, len);
             }

             /*
               Write out the text string
             */
             WindowizeString(szText);
             len = word_pad(strlen(szText) + 1);
             nwrite(resFD, (char *) szText, len);

             /*
               Write out the extra bytes. It's 0 for now.
             */
             len = 0;
#ifdef sun
             nwrite(resFD, (char *) &len, 4);
#else
             nwrite(resFD, (char *) &len, 1);
#endif

             /*
               Write the color attribute
             */
             if (!bWindowsCompatDlg)
             {
               i = yypvt[-0];
               nwrite(resFD, (char *) &i, sizeof(i));         /* attr */
             }

#ifdef RC_TRANS
             AddDialogControl(x, y, cx, cy, CurrStyle, szText, (int) yypvt[-13], iClass);
#endif

             CurrStyle = CurrNotStyle = 0;

           } break;
case 88:{ iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_LEFT;
             szUserClass = "Text";
             yyval = iCurrCtrlType;
           } break;
case 89:{ iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_RIGHT;
             szUserClass = "Text";
             yyval = iCurrCtrlType;
           } break;
case 90:{ iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_CENTER;
             szUserClass = "Text";
             yyval = iCurrCtrlType;
           } break;
case 91:{ iCurrCtrlType=EDIT_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | ES_LEFT;
             if (!bNoBorders)
               CurrStyle |= WS_BORDER;
             szUserClass = "Edit";
             yyval = iCurrCtrlType;
           } break;
case 92:{ iCurrCtrlType=CHECKBOX_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP;
             szUserClass = "Checkbox";
             yyval = iCurrCtrlType;
           } break;
case 93:{ iCurrCtrlType=PUSHBUTTON_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON;
             szUserClass = "Pushbutton";
             yyval = iCurrCtrlType;
           } break;
case 94:{ iCurrCtrlType=PUSHBUTTON_CLASS;
             CurrStyle |= WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP;
             szUserClass = "Pushbutton";
             yyval = iCurrCtrlType;
           } break;
case 95:{ iCurrCtrlType=RADIOBUTTON_CLASS;
             CurrStyle |= WS_CHILD;
             szUserClass = "Radiobutton";
             yyval = iCurrCtrlType;
           } break;
case 96:{ iCurrCtrlType=LISTBOX_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY;
             if (!bNoBorders)
               CurrStyle |= WS_BORDER;
             szUserClass = "Listbox";
             yyval = iCurrCtrlType;
           } break;
case 97:{ iCurrCtrlType=COMBO_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP;
             szUserClass = "Combobox";
             yyval = iCurrCtrlType;
           } break;
case 98:{ iCurrCtrlType=STATIC_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_LEFT;
             szUserClass = "Static";
             yyval = iCurrCtrlType;
           } break;
case 99:{ iCurrCtrlType=FRAME_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Frame";
             yyval = iCurrCtrlType;
           } break;
case 100:{ iCurrCtrlType=FRAME_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Frame";
             yyval = iCurrCtrlType;
           } break;
case 101:{ iCurrCtrlType=BOX_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Box";
             yyval = iCurrCtrlType;
           } break;
case 102:{ iCurrCtrlType=ICON_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_ICON;
             szUserClass = "Icon";
             yyval = iCurrCtrlType;
           } break;
case 103:{ iCurrCtrlType=SCROLLBAR_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SBS_CTL | SBS_HORZ;
             szUserClass = "Scrollbar";
             yyval = iCurrCtrlType;
           } break;
case 104:{ iCurrCtrlType=USER_CLASS;
             CurrStyle |= WS_CHILD;
             szUserClass = Symtab[yypvt[-0]].u.sval;
             yyval = iCurrCtrlType;
           } break;
case 105:{
             char *sz;
             WORD  len;
             WORD  i;
             DWORD dwStyle;
             WORD  iClass, idCtrl;
             WORD  x, y, cx, cy;

             /*
               Possibly get the CT_xxx class value
             */
             if (bUseCTMASK)
             {
               int ctMask;
               if ((ctMask = CtrlTypeToCT(iCurrCtrlType)) != 0)
               {
                 szUserClass[0] = (unsigned char) ctMask;
                 szUserClass[1] = '\0';
               }
             }

             /*
               If the button style for a checkbox or a radio button
               wasn't specified, default to the non-auto style.
             */
             if (iCurrCtrlType == CHECKBOX_CLASS)
             {
               if ((CurrStyle & 0x0FL) == 0)
                 CurrStyle |= BS_CHECKBOX;
             }
             else
             if (iCurrCtrlType == RADIOBUTTON_CLASS)
             {
               if ((CurrStyle & 0x0FL) == 0)
                 CurrStyle |= BS_RADIOBUTTON;
             }

             if (!bScreenRelativeCoords)
               xDlg = yDlg = 0;   /* for relative coordinates */

             x  = yypvt[-8];
             y  = yypvt[-6];
             cx = yypvt[-4];
             cy = yypvt[-2];
             MEWELizeCoords(&x, &y, &cx, &cy, iCurrCtrlType, 
                            (yypvt[-12] >= 0) ? Symtab[yypvt[-12]].u.sval : "");

             nwrite(resFD, (char *) &x, sizeof(x));            /* x  */
             nwrite(resFD, (char *) &y, sizeof(y));            /* y  */
             nwrite(resFD, (char *) &cx, sizeof(cx));          /* cx */
             nwrite(resFD, (char *) &cy, sizeof(cy));          /* cy */
             idCtrl = yypvt[-10];
             nwrite(resFD, (char *) &idCtrl, sizeof(idCtrl));  /* id */
             dwStyle = (DWORD) CurrStyle;
             dwStyle &= ~CurrNotStyle;
             nwrite(resFD, (char *) &dwStyle, sizeof(dwStyle));/* style */

             /*
               Write out the class name
             */
             if (bUseCTMASK && (szUserClass[0] & CT_MASK))
             {
               len = word_pad(1);
               nwrite(resFD, (char *) szUserClass, len);
             }
             else
             {
               len = word_pad(strlen(szUserClass) + 1);
               nwrite(resFD, (char *) szUserClass, len);
             }

             /*
               Write out the text string
             */
             if (yypvt[-12] >= 0)
             {
               sz = Symtab[yypvt[-12]].u.sval;
               len = word_pad(strlen(sz) + 1);
               WindowizeString(sz);
               nwrite(resFD, (char *) sz, len);
             }
             else
             {
               BYTE ch = '\0';
               len = word_pad(1);
               nwrite(resFD, (char *) &ch, len);
             }

             /*
               Write out the extra bytes. It's 0 for now.
             */
             len = 0;
#ifdef sun
             nwrite(resFD, (char *) &len, 4);
#else
             nwrite(resFD, (char *) &len, 1);
#endif

             if (!bWindowsCompatDlg)
             {
               i = yypvt[-0];
               nwrite(resFD, (char *) &i, sizeof(i));       /* attr */
             }

#ifdef RC_TRANS
             AddDialogControl(x, y, cx, cy, dwStyle, sz, (int)idCtrl,iCurrCtrlType);
#endif
           } break;
case 111:{
             CurrStyle |= CurrLong;
           } break;
case 112:{
             CurrStyle |= yypvt[-0];
           } break;
case 113:{
             CurrNotStyle |= CurrLong;
           } break;
case 114:{
             CurrNotStyle |= yypvt[-0];
           } break;
case 116:{
             yyval = SYSTEM_COLOR;
           } break;
case 117:{
             /*
               If someone uses the MEWEL color, then clear the
               'Windows-compatible' flag.
             */
             bWindowsCompatDlg = 0;
             yyval = yypvt[-0];
           } break;
case 118:{
             yyval = yypvt[-0];
           } break;
case 119:{
             yyval = -(yypvt[-0]);
           } break;
case 120:{
             yyval = yypvt[-0];
           } break;
case 121:{
             bNextSymbolIsLiteral++;  /* next symbol is taken verbatim */
           } break;
case 122:{
             yyval = -1;
           } break;
case 123:{
              yyval = yypvt[-0];
            } break;
case 124:{
             yyval = yypvt[-1];
           } break;
case 126:{
             fposStartOfResource = WriteResourceHeader(RT_RCDATA, yypvt[-2]);
             bReadingRawDataList = 1;
           } break;
case 128:{
             DWORD nBytes;

             fposResource = tell(resFD);
             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
             bReadingRawDataList = 0;
           } break;
case 132:{
             /*
               For both 16 and 32-bit compilers, we need to treat each
               number in a user-defined resource as a 16-bit value.
             */
             unsigned short i = (unsigned short) yypvt[-0];
             nwrite(resFD, (char *) &i, sizeof(i));
           } break;
case 133:{
             char *sz = Symtab[yypvt[-0]].u.sval;
             char *s;

             /*
               Re-translate the -1 to \0. The -1's were put in there
               by yylex. Do *NOT* write out the terminating \0 because,
               according to the SDK, the NULL terminator must be explicitly
               specified. ie : "ABC\0" produces a 4 character string.
             */
             for (s = sz;  *s;  s++)
               if (*s == 0xFF)
                 *s = '\0';

             nwrite(resFD, sz, s-sz);
           } break;
case 134:{
             /*
               We have a string of hex characters like this :
               '8F 42 67 DF 5A'
             */
             char buf[32];
             unsigned long ul;
             char ch, chByte;

             while ((ch = get_alpha_token(buf)) != EOF)
             {
               ul = xtol(buf);
               chByte = (BYTE) (ul & 0x00FFL);
               nwrite(resFD, (char *) &chByte, sizeof(BYTE));
               if (ch == '\'')
                 break;
             }
           } break;
case 135:{
             fposStartOfResource = WriteResourceHeader(yypvt[-0], yypvt[-1]);
             bNextSymbolIsLiteral--;
           } break;
case 136:{
             bExpectingRESID = JUST_DID_USERRES;
           } break;
case 137:{
             int sym;
             int num;
             if ((sym = ylookup("TEXT")) == 0)   
               sym = install("TEXT", ID, &num);  

             fposStartOfResource = WriteResourceHeader(-(sym), yypvt[-2]);
           } break;
case 139:{
              bReadingRawDataList = 1;
            } break;
case 141:{
              ReadUserResource(Symtab[yypvt[-0]].name);
            } break;
case 142:{
              ReadUserResource(Symtab[yypvt[-0]].u.sval);
            } break;
case 143:{
             bNextSymbolIsLiteral--;
#ifdef RC_TRANS
             AddIcon(Symtab[-(yypvt[-3])].name, Symtab[-(yypvt[-0])].name);
#endif
             ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_ICON);
           } break;
case 144:{
#ifdef RC_TRANS
             AddIcon(Symtab[-(yypvt[-3])].name, Symtab[(yypvt[-0])].name);
#endif
             ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_ICON);
           } break;
case 145:{
              fposStartOfResource = WriteResourceHeader(RT_ICON, yypvt[-3]);
              bReadingRawDataList = 1;
            } break;
case 146:{
              DWORD nBytes;

              /*
                Find out where the end of the resource is.
              */
              fposResource = tell(resFD);

              /*
                Go back to the resource header for this resource and
                fill in the number of bytes in the resource.
              */
              lseek(resFD, fposStartOfResource, 0);
              lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
              nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
              nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

              /*
                Now go back to the next place to start writing in the file
              */
              lseek(resFD, 0L, 2);
              fposResource = tell(resFD);
              bReadingRawDataList = 0;
            } break;
case 147:{
              bNextSymbolIsLiteral--;
#ifdef RC_TRANS
              AddIcon(Symtab[-(yypvt[-3])].name, Symtab[-(yypvt[-0])].name);
#endif
              ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_BITMAP);
            } break;
case 148:{
#ifdef RC_TRANS
              AddIcon(Symtab[-(yypvt[-3])].name, Symtab[(yypvt[-0])].name);
#endif
              ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_BITMAP);
            } break;
case 149:{
              fposStartOfResource = WriteResourceHeader(RT_BITMAP, yypvt[-3]);
              bReadingRawDataList = 1;
            } break;
case 150:{
              DWORD nBytes;

              /*
                Find out where the end of the resource is.
              */
              fposResource = tell(resFD);

              /*
                Go back to the resource header for this resource and
                fill in the number of bytes in the resource.
              */
              lseek(resFD, fposStartOfResource, 0);
              lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
              nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
              nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

              /*
                Now go back to the next place to start writing in the file
              */
              lseek(resFD, 0L, 2);
              fposResource = tell(resFD);
              bReadingRawDataList = 0;
            } break;
case 151:{
              bNextSymbolIsLiteral--;
#ifdef RC_TRANS
              AddIcon(Symtab[-(yypvt[-3])].name, Symtab[-(yypvt[-0])].name);
#endif
              ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_CURSOR);
            } break;
case 152:{
#ifdef RC_TRANS
              AddIcon(Symtab[-(yypvt[-3])].name, Symtab[(yypvt[-0])].name);
#endif
              ReadBinaryResourceFromFile(yypvt[-3], (yypvt[-0]), RT_CURSOR);
            } break;
case 153:{
              fposStartOfResource = WriteResourceHeader(RT_CURSOR, yypvt[-3]);
              bReadingRawDataList = 1;
            } break;
case 154:{
              DWORD nBytes;

              /*
                Find out where the end of the resource is.
              */
              fposResource = tell(resFD);

              /*
                Go back to the resource header for this resource and
                fill in the number of bytes in the resource.
              */
              lseek(resFD, fposStartOfResource, 0);
              lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
              nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
              nwrite(resFD, (char *) &nBytes, sizeof(nBytes));

              /*
                Now go back to the next place to start writing in the file
              */
              lseek(resFD, 0L, 2);
              fposResource = tell(resFD);
              bReadingRawDataList = 0;
            } break;
case 156:{
              nungets("#include");
            } break;
case 157:{
             char *szCaption = Symtab[yypvt[-1]].u.sval;
             char *szFmt = Symtab[yypvt[-2]].u.sval;
             WORD  len;
             DWORD nBytes;
             DWORD wStyle;

             fposStartOfResource = WriteResourceHeader(RT_MSGBOX, yypvt[-4]);
             fposResource = tell( resFD );

             len = yypvt[-0];
             nwrite(resFD, (char *) &len, sizeof(len));        /* Type */
             len = 0;
             nwrite(resFD, (char *) &len, sizeof(len));        /* FmtStringTblId */
             len = 0;
             nwrite(resFD, (char *) &len, sizeof(len));        /* CaptionStringTblId */

#ifdef WORD_ALIGNED /* Keep resource data word-aligned */
             len = word_pad(strlen( szFmt ));
             nwrite(resFD, (char *) &len, sizeof(len));        /* Fmt Len */
             len = word_pad(strlen( szCaption ));
             nwrite(resFD, (char *) &len, sizeof(len));        /* Caption Len */
             nwrite(resFD, szFmt, word_pad(strlen( szFmt ) )); /* Fmt String */
             nwrite(resFD, szCaption, word_pad(strlen( szCaption ) ));   /* Caption string */
#else
             len = strlen( szFmt );
             nwrite(resFD, (char *) &len, sizeof(len));        /* Fmt Len */
             len = strlen( szCaption );
             nwrite(resFD, (char *) &len, sizeof(len));        /* Caption Len */
             nwrite(resFD, szFmt, strlen( szFmt ) );           /* Fmt String */
             nwrite(resFD, szCaption, strlen( szCaption ) );   /* Caption string */
#endif

             lseek(resFD, 0L, 2);                             /* Figger out where we are */
             fposResource = tell(resFD);
             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
             CurrDlgStyle = 0L;
           } break;
case 158:{
             yyval = MB_OK;
           } break;
case 159:{
             yyval = yypvt[-0];
           } break;
case 160:{ yyval = MB_OK;         } break;
case 161:{ yyval = MB_OKCANCEL;   } break;
case 162:{ yyval = MB_YESNO;      } break;
case 163:{ yyval = MB_YESNOCANCEL;} break;
        }
        goto yystack;           /* reset registers in driver code */
}

