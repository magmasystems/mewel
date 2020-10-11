%{
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

%}

%token NUMBER LNUMBER STRING ID
%token BEGIN END
%token STRINGTABLE
%token ACCELERATORS KEYCODE
%token MENU MENUITEM POPUP SEPARATOR 
%token MENUBREAK CHECKED INACTIVE GRAYED HELP SHADOW
%token DIALOG STYLE CLASS CAPTION
%token TEXT EDIT CHECKBOX RADIOBUTTON PUSHBUTTON STATIC LISTBOX
%token FRAME BOX SCROLLBAR ICON COMBOBOX RCDATA

/* for MS Windows compatibility... */
%token PRELOAD  LOADONCALL  MOVEABLE  DISCARDABLE  FIXED  FONT  CONTROL
%token BITMAP   CURSOR      DEFPUSHBUTTON

/* Preprocessor */
%token IF IFDEF IFNDEF ELSE ELIF ENDIF

/* Accelerators */
%token ALT  ASCII  SHIFT VIRTKEY  NOINVERT  

%token RCINCLUDE
%token MSGBOX OK OKCANCEL YESNO YESNOCANCEL

%token MENUBARBREAK

%token CTEXT RTEXT GROUPBOX
%token UNCHECKED   ENABLED
%token PURE NOT IMPURE

/* VERSIONINFO */
%token VERSIONINFO FILEVERSION PRODUCTVERSION FILEFLAGSMASK
%token FILEFLAGS FILEOS FILETYPE FILESUBTYPE BLOCK VALUE

%%

/*---------------------------------------------------------------------------*/

objlist  : /* empty */
           {
             bExpectingRESID = 1;
             memset((char *) &StringTblHeader, '\0', sizeof(StringTblHeader));
             StringTblHeader.ulSignature = RC_SIGNATURE;
             nwrite(resFD, (char *) &StringTblHeader, sizeof(StringTblHeader));
           }
         | objlist obj
           {
             bExpectingRESID = (bExpectingRESID == JUST_DID_USERRES) ? 0 : 1;
           }
         ;

obj      : stringtbl
         | acceltbl
         | menu
         | dialog
         | icon
         | rcdata
         | userres
         | bitmap
         | cursor
         | font
         | rcinclude
         | msgbox
         | end
         | versioninfo
         ;

/*---------------------------------------------------------------------------*/
stringtbl: STRINGTABLE $loadopts
           {
             bSawStringtable = TRUE;
             StringInfoInit();
           }
           BEGIN stringlist END
         ;

stringlist: /* empty */
         |  stringlist nexpr $comma STRING
            {
              StringInfoAddString($4, $2);
#ifdef RC_TRANS
              AddString(Symtab[$4].u.sval, (WORD) $2);
#endif
            }
         ;

$comma   :  /* empty */
         |  ','
         ;

nexpr    : NUMBER
           {
             $$ = $1;        /* return the id */
           }
         | nexpr '+' nexpr
           {
             $$ = $1 + $3;
           }
         | nexpr '-' nexpr
           {
             $$ = $1 - $3;
           }
         | nexpr '*' nexpr
           {
             $$ = $1 * $3;
           }
         | nexpr '/' nexpr
           {
             $$ = $1 / $3;
           }
         | nexpr '%' nexpr
           {
             $$ = $1 % $3;
           }
         | '(' nexpr ')'
           {
             $$ = $2;
           }
         | nexpr '|' nexpr
           {
             $$ = $1 | $3;
           }
         ;

/*---------------------------------------------------------------------------*/
acceltbl : resID ACCELERATORS $loadopts
           {
             fposStartOfResource = WriteResourceHeader(RT_ACCELERATOR, $1);
           }
           BEGIN accellist END
           {
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
             AddAcceleratorTable(Symtab[-($1)].name);
#endif
           }
         ;

accellist: /* empty */
           {
             $$ = 0;
             CurrAccelState = 0;
           }
         | accellist accel
           {
             $$ = $1 + 1;
             CurrAccelState = 0;
           }
         ;

accel    : accelevent $comma nexpr $comma $accelopts
           {
             WORD key, cmd;
             BYTE fFlags;
             int  iKey;

             key = $1;  cmd = $3;

             /*
               If iKey is less than 0, then the user specified a
               key string. Check the first character to see if
               it is a '^', and if it is, controlify the letter.
             */
             if (bKeyIsString)
             {
               char *szKey;

               iKey = (int) $1;  /* for signed compares .... */
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
           }
         ;

accelevent:STRING
           {
             $$ = -($1);
             bKeyIsString = TRUE;
           }
         | NUMBER
           {
             $$ = $1;
             bKeyIsString = FALSE;
           }
         ;

$accelopts :  /* empty */
         | $accelopts accelopt
         | $accelopts $comma accelopt
         ;

accelopt : ASCII
         | VIRTKEY
         | NOINVERT    { CurrAccelState |= STATE_NOINVERT; }
         | ALT         { CurrAccelState |= STATE_ALT;      }
         | SHIFT       { CurrAccelState |= STATE_SHIFT;    }
         | CONTROL     { CurrAccelState |= STATE_CONTROL;  }
         ;

/*---------------------------------------------------------------------------*/
menu     : resID MENU $loadopts
           {
#ifdef USE_WINDOWS_MENU
             MENUITEMTEMPLATEHEADER mit;
             fposStartOfResource = WriteResourceHeader(RT_MENU, $1);
             mit.versionNumber = mit.offset = 0;
             nwrite(resFD, (char *) &mit, sizeof(mit));
#else
             WORD nmenu = 0;
             fposStartOfResource = WriteResourceHeader(RT_MENU, $1);
             push(fposResource = tell(resFD));
             nwrite(resFD, (char *) &nmenu, sizeof(nmenu));
#endif
           }
           BEGIN menuitems END
           {
             WORD nItems = $6;
             DWORD nBytes;

#ifdef USE_WINDOWS_MENU
             WriteMenuTree($6);
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
             AddMenu(Symtab[-($1)].name);
#endif
           }
         ;

menuitems: /* empty */
           {
             $$ = 0;
           }
         | menuitems menuitem
           {
             $$ = $1 + 1;
#ifdef USE_WINDOWS_MENU
             $$ = LinkMenuNodeToSibling($1, $2);
#endif
           }
         ;

menuitem : MENUITEM SEPARATOR
           {
#ifdef USE_WINDOWS_MENU
             $$ = CreateMenuNode(0, 0, NULL);
#else
             MTIheader.style  = MF_SEPARATOR; 
             MTIheader.attr   = 0;
             MTIheader.idItem = 0;
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_SEPARATOR, 0, 0, NULL);
#endif
           }

         | MENUITEM STRING $comma nexpr miopts
           {
             char *str = Symtab[$2].u.sval;
             WORD len = word_pad(strlen(str));

             WindowizeString(str);

#ifdef USE_WINDOWS_MENU
             $$ = CreateMenuNode($5 | MF_STRING, $4, str);
#else
             MTIheader.style  = MF_STRING; 
             MTIheader.attr   = $5;
             MTIheader.idItem = $4;
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
             nwrite(resFD, (char *) &len, sizeof(len));
             nwrite(resFD, (char *) str, len);
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_STRING, $5, $4, str);
#endif
           }

         | POPUP STRING $comma miopts
           {
             char *str = Symtab[$2].u.sval;
             WORD len = word_pad(strlen(str));
             WORD nItems = 0;

             WindowizeString(str);

#ifdef USE_WINDOWS_MENU
             $$ = CreateMenuNode($4 | MF_POPUP, 0, str);
#else
             MTIheader.style  = MF_POPUP; 
             MTIheader.attr   = $4;
             MTIheader.idItem = 0;
             nwrite(resFD, (char *) &MTIheader, sizeof(MTIheader));
             nwrite(resFD, (char *) &len, sizeof(len));
             nwrite(resFD, (char *) str, len);

             fposResource = tell(resFD);
             push(fposResource);
             nwrite(resFD, (char *) &nItems, sizeof(nItems));
#endif

#ifdef RC_TRANS
             AddMenuItem(MF_POPUP, $4, 0, str);
#endif
           }
           BEGIN menuitems END
           {
#ifdef USE_WINDOWS_MENU
             $$ = LinkMenuNodeToParent($5, $7);
#else
             /*
               Fill in the number of items for this submenu
             */
             WORD nItems = $7;
             fposResource = pop();
             lseek(resFD, fposResource, 0);
             nwrite(resFD, (char *) &nItems, sizeof(nItems));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
#endif

#ifdef RC_TRANS
             EndPopup();
#endif
           }
         ;

miopts   : /* empty */
           {
             $$ = 0x0000;
           }
         | miopt
           {
             $$ = $1;
           }
         | miopts $comma miopt
           {
             $$ = $1 | $3;
           }
         ;

miopt    : MENUBREAK       { $$ = MF_MENUBREAK; }
         | CHECKED         { $$ = MF_CHECKED;   }
         | INACTIVE        { $$ = MF_DISABLED;  }
         | GRAYED          { $$ = MF_DISABLED;  }
         | HELP            { $$ = MF_HELP;      }
         | SHADOW          { $$ = MF_SHADOW;    }
         | MENUBARBREAK    { $$ = 0;            }
         | UNCHECKED       { $$ = MF_UNCHECKED; }
         | ENABLED         { $$ = MF_ENABLED;   }
         ;

/*---------------------------------------------------------------------------*/
dialog   : resID DIALOG $loadopts
                 nexpr ',' nexpr ',' nexpr ',' nexpr $color $comma dlgopts
           {
             WORD  nItems = 0;
             WORD  len;
             DWORD dwStyle;
             char *szDlg = NULL;
             char *pszClass = NULL;
             BYTE  bnItems;

             fposStartOfResource = WriteResourceHeader(RT_DIALOG, $1);
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

             xDlg = (xTranslated) ? (($4 + xTranslated/2-iRounding) / xTranslated) : $4;
             yDlg = (yTranslated) ? (($6 + yTranslated/2-iRounding) / yTranslated) : $6;
             cxDlg = (xTranslated) ? (($8 + xTranslated/2-iRounding) / xTranslated) : $8;
             cyDlg = (yTranslated) ? (($10 + yTranslated/2-iRounding) / yTranslated) : $10;
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
                      $4, $6, $8, $10,
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
               len = $11;
               nwrite(resFD, (char *) &len, sizeof(len));        /* attr */
             }

#ifdef RC_TRANS
             AddDialogBox(xDlg, yDlg, cxDlg, cyDlg, dwStyle, 
                          Symtab[-($1)].name, szDlg, pszClass);
#endif
             CurrDlgStyle = 0L;
             iDlgPtSize   = 0;
             iDlgFont     = 0;
           }
           BEGIN dlgcontrols END
           {
             WORD  nItems = $16;
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
           }
         ;


$loadopts: /* empty */
         | $loadopts   PRELOAD
         | $loadopts   LOADONCALL
         | $loadopts   PURE
         | $loadopts   IMPURE
         | $loadopts   FIXED
         | $loadopts   MOVEABLE
         | $loadopts   DISCARDABLE
         ;


dlgopts  : /* empty */
           {
             iDlgCaption = -1;
             iDlgClass   = -1;
             iDlgMenu    = 0;
             $$ = 0;
           }
         | dlgopts dlgopt
           {
             $$ = $1 + 1;
           }
         ;

dlgopt   : STYLE ctrlstyles
           {
             CurrDlgStyle = CurrStyle;
             CurrStyle = CurrNotStyle = 0L;
           }
         | CAPTION STRING
           {
             /*
               9/5/92 (maa)
               It seems that Windows adds a WS_CAPTION to a window if
               the CAPTION clause is present, even if WS_CAPTION is
               not specified in the STYLE clause.
             */
             CurrDlgStyle |= WS_CAPTION;
             iDlgCaption = $2;
           }
         | MENU  resID
           {
             iDlgMenu = $2;
           }
         | CLASS STRING
           {
             iDlgClass = $2;
           }
         | FONT  NUMBER $comma STRING
           {
             iDlgPtSize = $2;
             iDlgFont   = $4;
             CurrDlgStyle |= DS_SETFONT;
           }
         ;


dlgcontrols: /* empty */
           {
             $$ = 0;
           }
         | dlgcontrols dlgcontrol
           {
             $$ = $1 + 1;
           }
         ;

dlgcontrol: ctrltype ctrlinfo
            {
              CurrStyle = CurrNotStyle = 0;
            }


/*         CONTROL "text",    id,         class,     style,         x,y,cx,cy */
         | CONTROL $string $comma nexpr $comma  STRING $comma $ctrlstyles ','
                              nexpr ',' nexpr ',' nexpr ',' nexpr  $color

           {
             char *szText;
             WORD len, i, iClass;
             WORD x, y, cx, cy;
             int  ctMask;

             szText = Symtab[$2].u.sval;
             if (!bScreenRelativeCoords)
               xDlg = yDlg = 0;   /* for relative coordinates */

             iClass = StringToClassID(Symtab[$6].u.sval, &CurrStyle, &ctMask);

             /*
               Make all individual scrollbar control SBS_CTL
             */
             if (iClass == SCROLLBAR_CLASS)
               CurrStyle |= SBS_CTL;

             /*
               Now is the time to fix up the coordinates if we are
               processing a dialog box created by Windows.
             */
             x = $10;
             y = $12;
             cx = $14;
             cy = $16;
             MEWELizeCoords(&x, &y, &cx, &cy, iClass, szText);

             nwrite(resFD, (char *) &x, sizeof(x));         /* x  */
             nwrite(resFD, (char *) &y, sizeof(y));         /* y  */
             nwrite(resFD, (char *) &cx, sizeof(cx));       /* cx */
             nwrite(resFD, (char *) &cy, sizeof(cy));       /* cy */
             i = $4;
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
               len = word_pad(strlen(Symtab[$6].u.sval) + 1);
               nwrite(resFD, (char *) Symtab[$6].u.sval, len);
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
               i = $17;
               nwrite(resFD, (char *) &i, sizeof(i));         /* attr */
             }

#ifdef RC_TRANS
             AddDialogControl(x, y, cx, cy, CurrStyle, szText, (int) $4, iClass);
#endif

             CurrStyle = CurrNotStyle = 0;

           }

         ;

ctrltype : TEXT
           { iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_LEFT;
             szUserClass = "Text";
             $$ = iCurrCtrlType;
           }
         | RTEXT
           { iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_RIGHT;
             szUserClass = "Text";
             $$ = iCurrCtrlType;
           }
         | CTEXT
           { iCurrCtrlType=TEXT_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_CENTER;
             szUserClass = "Text";
             $$ = iCurrCtrlType;
           }
         | EDIT
           { iCurrCtrlType=EDIT_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | ES_LEFT;
             if (!bNoBorders)
               CurrStyle |= WS_BORDER;
             szUserClass = "Edit";
             $$ = iCurrCtrlType;
           }
         | CHECKBOX
           { iCurrCtrlType=CHECKBOX_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP;
             szUserClass = "Checkbox";
             $$ = iCurrCtrlType;
           }
         | PUSHBUTTON
           { iCurrCtrlType=PUSHBUTTON_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON;
             szUserClass = "Pushbutton";
             $$ = iCurrCtrlType;
           }
         | DEFPUSHBUTTON
           { iCurrCtrlType=PUSHBUTTON_CLASS;
             CurrStyle |= WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP;
             szUserClass = "Pushbutton";
             $$ = iCurrCtrlType;
           }
         | RADIOBUTTON
           { iCurrCtrlType=RADIOBUTTON_CLASS;
             CurrStyle |= WS_CHILD;
             szUserClass = "Radiobutton";
             $$ = iCurrCtrlType;
           }
         | LISTBOX
           { iCurrCtrlType=LISTBOX_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY;
             if (!bNoBorders)
               CurrStyle |= WS_BORDER;
             szUserClass = "Listbox";
             $$ = iCurrCtrlType;
           }
         | COMBOBOX
           { iCurrCtrlType=COMBO_CLASS;
             CurrStyle |= WS_CHILD | WS_TABSTOP;
             szUserClass = "Combobox";
             $$ = iCurrCtrlType;
           }
         | STATIC
           { iCurrCtrlType=STATIC_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_LEFT;
             szUserClass = "Static";
             $$ = iCurrCtrlType;
           }
         | GROUPBOX
           { iCurrCtrlType=FRAME_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Frame";
             $$ = iCurrCtrlType;
           }
         | FRAME
           { iCurrCtrlType=FRAME_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Frame";
             $$ = iCurrCtrlType;
           }
         | BOX
           { iCurrCtrlType=BOX_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP;
             szUserClass = "Box";
             $$ = iCurrCtrlType;
           }
         | ICON
           { iCurrCtrlType=ICON_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SS_ICON;
             szUserClass = "Icon";
             $$ = iCurrCtrlType;
           }
         | SCROLLBAR
           { iCurrCtrlType=SCROLLBAR_CLASS;
             CurrStyle |= WS_CHILD | WS_GROUP | SBS_CTL | SBS_HORZ;
             szUserClass = "Scrollbar";
             $$ = iCurrCtrlType;
           }
         | STRING
           { iCurrCtrlType=USER_CLASS;
             CurrStyle |= WS_CHILD;
             szUserClass = Symtab[$1].u.sval;
             $$ = iCurrCtrlType;
           }
         ;

ctrlinfo : $string $comma nexpr $comma nexpr ',' nexpr ',' nexpr ',' nexpr 
                                             $ctrlstyles $color
           {
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

             x  = $5;
             y  = $7;
             cx = $9;
             cy = $11;
             MEWELizeCoords(&x, &y, &cx, &cy, iCurrCtrlType, 
                            ($1 >= 0) ? Symtab[$1].u.sval : "");

             nwrite(resFD, (char *) &x, sizeof(x));            /* x  */
             nwrite(resFD, (char *) &y, sizeof(y));            /* y  */
             nwrite(resFD, (char *) &cx, sizeof(cx));          /* cx */
             nwrite(resFD, (char *) &cy, sizeof(cy));          /* cy */
             idCtrl = $3;
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
             if ($1 >= 0)
             {
               sz = Symtab[$1].u.sval;
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
               i = $13;
               nwrite(resFD, (char *) &i, sizeof(i));       /* attr */
             }

#ifdef RC_TRANS
             AddDialogControl(x, y, cx, cy, dwStyle, sz, (int)idCtrl,iCurrCtrlType);
#endif
           }
         ;

$ctrlstyles: /* empty */
         | ctrlstyles
         | ',' ctrlstyles
         ;

ctrlstyles: ctrlstyle
         | ctrlstyles '|' ctrlstyle
         ;

ctrlstyle: LNUMBER
           {
             CurrStyle |= CurrLong;
           }
         | NUMBER
           {
             CurrStyle |= $1;
           }
         | NOT LNUMBER
           {
             CurrNotStyle |= CurrLong;
           }
         | NOT NUMBER
           {
             CurrNotStyle |= $2;
           }
         | '(' ctrlstyles ')'
         ;

$color   : /* empty */
           {
             $$ = SYSTEM_COLOR;
           }
         | ',' NUMBER
           {
             /*
               If someone uses the MEWEL color, then clear the
               'Windows-compatible' flag.
             */
             bWindowsCompatDlg = 0;
             $$ = $2;
           }
         ;

resID    : NUMBER
           {
             $$ = $1;
           }
         | ID
           {
             $$ = -($1);
           }
         ;

literal_resID : $literal_resID resID
           {
             $$ = $2;
           }
         ;

$literal_resID : /* empty - semantic actions only */
           {
             bNextSymbolIsLiteral++;  /* next symbol is taken verbatim */
           }

$string  : /* empty */
           {
             $$ = -1;
           }
         | STRING
            {
              $$ = $1;
            }
         | STRING ','
           {
             $$ = $1;
           }

/*
         2/1/93 (maa)
         Commented this out, because it caused something like 
           EDITTEXT ID_EDITBOX, 86, 7, 90, 11
         to fail. Yacc reduced ID_EDITBOX to $string, and used the
         number '86' as the control identifier instead of using it
         as the 'x' coordinate.

         | NUMBER $comma
           {
             char szNum[16];
             int  sym;

             // Transform number into "#number"
             sprintf(szNum, "#%d", $1);
             if ((sym = litlookup(STRING, szNum)) == 0)
               $$ = install(NULL, STRING, strsave(szNum));
             else
               $$ = sym;
           }
*/
         ;

quote    : '\''
         ;

/*--------------------------------------------------------------------------*/

rcdata   : resID RCDATA $loadopts
           {
             fposStartOfResource = WriteResourceHeader(RT_RCDATA, $1);
             bReadingRawDataList = 1;
           }
           BEGIN $rcdata END
         ;

$rcdata  : rawdatalist
           {
             DWORD nBytes;

             fposResource = tell(resFD);
             lseek(resFD, fposStartOfResource, 0);
             lseek(resFD, offsetof(RESOURCE,nResBytes), 1);
             nBytes = fposResource - fposStartOfResource - sizeof(RESOURCE);
             nwrite(resFD, (char *) &nBytes, sizeof(nBytes));
             lseek(resFD, 0L, 2);
             fposResource = tell(resFD);
             bReadingRawDataList = 0;
           }
         ;


rawdatalist: /* empty */
         | rawdata $comma
         | rawdatalist $comma rawdata
         ;

rawdata  : nexpr
           {
             /*
               For both 16 and 32-bit compilers, we need to treat each
               number in a user-defined resource as a 16-bit value.
             */
             unsigned short i = (unsigned short) $1;
             nwrite(resFD, (char *) &i, sizeof(i));
           }
         | STRING
           {
             char *sz = Symtab[$1].u.sval;
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
           }
         | quote
           {
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
           }
         ;


userres  : resID literal_resID 
           {
             fposStartOfResource = WriteResourceHeader($2, $1);
             bNextSymbolIsLiteral--;
           }
           $loadopts $userres $loadopts
           {
             bExpectingRESID = JUST_DID_USERRES;
           }

         | resID TEXT $loadopts
           {
             int sym;
             int num;
             if ((sym = ylookup("TEXT")) == 0)   
               sym = install("TEXT", ID, &num);  

             fposStartOfResource = WriteResourceHeader(-(sym), $1);
           }
           $userres
          ;

$userres  : BEGIN 
            {
              bReadingRawDataList = 1;
            }
            $rcdata END
          | ID
            {
              ReadUserResource(Symtab[$1].name);
            }
          | STRING
            {
              ReadUserResource(Symtab[$1].u.sval);
            }
          ;
/*--------------------------------------------------------------------------*/

icon     : resID ICON $loadopts literal_resID
           {
             bNextSymbolIsLiteral--;
#ifdef RC_TRANS
             AddIcon(Symtab[-($1)].name, Symtab[-($4)].name);
#endif
             ReadBinaryResourceFromFile($1, ($4), RT_ICON);
           }
         | resID ICON $loadopts STRING
           {
#ifdef RC_TRANS
             AddIcon(Symtab[-($1)].name, Symtab[($4)].name);
#endif
             ReadBinaryResourceFromFile($1, ($4), RT_ICON);
           }
         | resID ICON   $loadopts BEGIN
            {
              fposStartOfResource = WriteResourceHeader(RT_ICON, $1);
              bReadingRawDataList = 1;
            }
            rawdatalist END
            {
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
            }
         ;


bitmap    : resID BITMAP $loadopts literal_resID
            {
              bNextSymbolIsLiteral--;
#ifdef RC_TRANS
              AddIcon(Symtab[-($1)].name, Symtab[-($4)].name);
#endif
              ReadBinaryResourceFromFile($1, ($4), RT_BITMAP);
            }
          | resID BITMAP $loadopts STRING
            {
#ifdef RC_TRANS
              AddIcon(Symtab[-($1)].name, Symtab[($4)].name);
#endif
              ReadBinaryResourceFromFile($1, ($4), RT_BITMAP);
            }
          | resID BITMAP $loadopts BEGIN
            {
              fposStartOfResource = WriteResourceHeader(RT_BITMAP, $1);
              bReadingRawDataList = 1;
            }
            rawdatalist END
            {
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
            }
          ;


cursor    : resID CURSOR $loadopts literal_resID
            {
              bNextSymbolIsLiteral--;
#ifdef RC_TRANS
              AddIcon(Symtab[-($1)].name, Symtab[-($4)].name);
#endif
              ReadBinaryResourceFromFile($1, ($4), RT_CURSOR);
            }
          | resID CURSOR $loadopts STRING
            {
#ifdef RC_TRANS
              AddIcon(Symtab[-($1)].name, Symtab[($4)].name);
#endif
              ReadBinaryResourceFromFile($1, ($4), RT_CURSOR);
            }

          | resID CURSOR $loadopts BEGIN
            {
              fposStartOfResource = WriteResourceHeader(RT_CURSOR, $1);
              bReadingRawDataList = 1;
            }
            rawdatalist END
            {
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
            }
          ;


font      : resID FONT $loadopts literal_resID
          ;

rcinclude : RCINCLUDE
            {
              nungets("#include");
            }
          ;


/*---------------------------------------------------------------------------*/
/* resID MSGBOX "FormatString" "Caption" {style}                             */
msgbox   : resID MSGBOX STRING STRING msgboxtp
           {
             char *szCaption = Symtab[$4].u.sval;
             char *szFmt = Symtab[$3].u.sval;
             WORD  len;
             DWORD nBytes;
             DWORD wStyle;

             fposStartOfResource = WriteResourceHeader(RT_MSGBOX, $1);
             fposResource = tell( resFD );

             len = $5;
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
           }
         ;


msgboxtp : /* Empty */
           {
             $$ = MB_OK;
           }
         | msgopt
           {
             $$ = $1;
           }
         ;

msgopt   : OK              { $$ = MB_OK;         }
         | OKCANCEL        { $$ = MB_OKCANCEL;   }
         | YESNO           { $$ = MB_YESNO;      }
         | YESNOCANCEL     { $$ = MB_YESNOCANCEL;}
         ;

end      : END
         ;


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                          VERSIONINFO                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
versioninfo   : versionid VERSIONINFO fixedinfo
                BEGIN
                  blocks
                END
              ;

versionid     : NUMBER
              ;

fixedinfo     : /* empty */
              | fixedinfo fixedinfoitem
              ;

fixedinfoitem : FILEVERSION vernum
              | PRODUCTVERSION vernum
              | FILEFLAGSMASK fileflagmask
              | FILEFLAGS fileflags
              | FILEOS fileos
              | FILETYPE filetype
              | FILESUBTYPE filesubtype
              ;

vernum        : NUMBER ',' NUMBER ',' NUMBER ',' NUMBER
              | NUMBER ',' NUMBER 
              ;


blocks        : /* empty */
              | blocks blockstatement
              ;

blockstatement: BLOCK STRING
                BEGIN
                  BLOCK STRING
                  BEGIN
                    valuelist
                  END
                END

              | BLOCK STRING
                BEGIN
                  valuelist
                END
              ;

valuelist     : /* empty */
              | valuelist value
              ;

value         : VALUE STRING $comma STRING
              | VALUE STRING $comma NUMBER $comma NUMBER
              ;

fileflagmask  : lexpr
              ;

fileflags     : lexpr
              ;

fileos        : lexpr
              ;

filetype      : lexpr
              ;

filesubtype   : lexpr
              ;

lexpr         : LNUMBER
              | NUMBER
              | lexpr '|' lexpr
              | '(' lexpr ')'
              ;

%%

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


