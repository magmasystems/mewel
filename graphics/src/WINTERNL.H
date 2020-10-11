/*===========================================================================*/
/*                                                                           */
/* File    : WINTERNL.H                                                      */
/*                                                                           */
/* Purpose : Information which is used by MEWEL internally.                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

/*
  Define this if you want those ugly 3-char Borland TurboVision icons.
  (The files affected by this are WGDICHAR.C, WSTDPROC.C, and WINBORDR.C)
*/
#if 0
#define MULTICHAR_SYSICONS
#endif

/*
  Some defines for controlling the way that MEWEL looks...
*/
#define TWO_COLUMN_SHADOWS /* Use a 2-col-wide shadow border instead of 1 col */



/*
  HUGE definition
*/
#if defined(DOS) && !defined(MEWEL_32BITS) && !defined(__TSC__)
#define HUGE  huge
#else
#define HUGE  FAR
#endif

/*
  The FARDATA typedef is used in order to declare FAR arrays. This
  is mainly used to shut up the Microsoft compiler. We can also
  use this definition to put the far array into a different segment.
*/
#define FARDATA  FAR


/*
  Definitions of helper typedefs and functions for DOS extenders
*/

/*
  Pharlap 286
*/
#if defined(DOS286X)
#undef APIENTRY
#define APIENTRY PASCAL _far
typedef USHORT SEL;
typedef USHORT _far *PSEL;
typedef ULONG  REALPTR;

typedef struct
{
  USHORT es;
  USHORT ds;
  USHORT di;
  USHORT si;
  USHORT bp;
  USHORT sp;
  USHORT bx;
  USHORT dx;
  USHORT cx;
  USHORT ax;
  USHORT ip;
  USHORT cs;
  USHORT flags;
} REGS16;
typedef REGS16 _far *PREGS;

extern  USHORT APIENTRY DosAllocSeg(USHORT, PSEL, USHORT);
extern  USHORT APIENTRY DosFreeSeg(SEL);
extern  USHORT APIENTRY DosGetBIOSSeg(PSEL);
extern  USHORT APIENTRY DosMapRealSeg(USHORT, ULONG, PSEL);
extern  USHORT APIENTRY DosReallocSeg(USHORT, SEL);
extern  USHORT _far _cdecl DosRealIntr(USHORT, PREGS, REALPTR, SHORT, ...);

#define MAKEP(sel, off)   ((PVOID)((((ULONG)(sel)) << 16) + (off)))
/* #define SELECTOROF(fp)    ((SEL)(((ULONG)(fp)) >> 16)) */
#endif /* DOS286X */


#if defined(__DPMI32__)
/*
  Structures and functions for 32-bit DPMI
*/
typedef struct tagTRegs
{
  unsigned short AX, BX, CX, DX, BP, SI, DI, DS, ES, cFlags;
} TREGISTERS;

#if !defined(PLTNT)
unsigned char  PeekFarByte(void *);
unsigned short PeekFarWord(void *);
#endif
unsigned long  PeekFarLong(void *);
int            FreeMappedDPMIPtr(void *);
int            GetMappedDPMIPtr(void **, void *, int);
int            RealIntr(int, TREGISTERS *);
#endif


/*
  Rational DOS 16/M
*/
#if defined(DOS16M)
#define RSIAPI cdecl _loadds far
extern VOID FAR *RSIAPI D16MemAlloc(UINT);
extern INT RSIAPI D16MemFree(VOID FAR *);
#endif


/*
  The Zortech and High-C compilers need to have a 48-bit "far" pointer
  for some data accesses to real memory.
*/
#if defined(DOS386) || (defined(PL386) && defined(__WATCOMC__))
#define TRUEFAR  far
#elif defined(__HIGHC__)
#define TRUEFAR  _Dfar
#else
#define TRUEFAR  FAR
#endif

#if defined(DOS386) || defined(__HIGHC__) || (defined(PL386) && defined(__WATCOMC__))
typedef BYTE TRUEFAR *FARADDR;
#else
typedef LPSTR FARADDR;
#endif

extern FARADDR PASCAL GetBIOSAddress(WORD);

/*
  We might actually need near pointers for compiler optimization
*/
#if defined(DOS) && defined(MSC) && !defined(EXTENDED_DOS)
#define TRUENEAR  _near
#else
#define TRUENEAR
#endif

#if defined(PLTNT) || (defined(__GNUC__) && !defined(UNIX))
#define _osmajor  5
#endif


/*
  These are memory allocation/deallocation macros which MEWEL uses
  internally. For small allocations under a DOS extender, we do not want
  to use emalloc_far/MyFree_far, because it takes too much time to grab a
  selector. Instead, use emalloc/MyFree.
*/
#if defined(EXTENDED_DOS)
#define EMALLOC_FAR(n)          emalloc((UINT) (n))
#define EMALLOC_FAR_NOQUIT(n)   emalloc_noquit((UINT) (n))
#define MYFREE_FAR(s)           MyFree(s)
#else
#define EMALLOC_FAR(n)          emalloc_far((DWORD) (n))
#define EMALLOC_FAR_NOQUIT(n)   emalloc_far_noquit((DWORD) (n))
#define MYFREE_FAR(s)           MyFree_far(s)
#endif



/*
  Definitions of a screen cell - very important for compatibility between
  16 and 32 bit mode
*/
typedef unsigned short  CELL;
typedef CELL           *PCELL;
typedef CELL FAR       *LPCELL;

/*
  Rectangle stuff
*/
extern RECT RectEmpty;  /* An empty rect which you can use for compares */
#define RECTGEN(r, row1, col1, row2, col2) \
        {\
          (r).top    = row1;\
          (r).left   = col1;\
          (r).bottom = row2;\
          (r).right  = col2;\
        }

/*
  Height and Width macros
*/
#define RECT_HEIGHT(r)          ((r).bottom - (r).top)
#define RECT_WIDTH(r)           ((r).right - (r).left)
#define WIN_HEIGHT(w)           ((w)->rect.bottom-(w)->rect.top)
#define WIN_WIDTH(w)            ((w)->rect.right-(w)->rect.left)
#define WIN_CLIENT_HEIGHT(w)    ((w)->rClient.bottom-(w)->rClient.top)
#define WIN_CLIENT_WIDTH(w)     ((w)->rClient.right-(w)->rClient.left)

/*
  The main window structure!!!!
*/
typedef struct window
{
  struct window *next;          /* hook onto WindowList */
  struct window *parent;        /* ptr to parent window */
  struct window *children;      /* ptr to list of children */
  struct window *sibling;       /* ptr to next sibling window */
  struct window *prevSibling;   /* ptr to previous sibling window */
  HWND     win_id;       /* window handle */
  HWND     hWndOwner;    /* handle of a popup's owner window */
  INT      idCtrl;       /* control ID for control windows */
  HWND     hMenu;        /* handle to a menubar */
  HWND     hSB[2];       /* handles to scrollbars */
  RECT     rect;         /* window dimensions */
  RECT     rClient;      /* client area dimensions */
  COLOR    attr;         /* color attribute */
  int      fillchar;     /* char to draw window background with */
  int      iMonitor;     /* in case of multiple monitors, the monitor id */
  LPSTR    title;        /* title in border */
  int      idClass;      /* class id */
  WINPROC *winproc;      /* ptr to the window procedure     */
  DWORD    flags;        /* the WS_xxxx styles for a window */
  DWORD    ulStyle;      /* internal flags for a window     */
  DWORD    dwExtStyle;   /* Windows 3.0 extended style bits */
  PSTR     pWinExtra;    /* pointer to window-extra bytes area */
  HMENU    hSysMenu;     /* handle of the window's system menu */
  HICON    hIcon;        /* handle of the window's icon        */
  RECT     rUpdate;      /* update rectangle for PAINT messages */
  LPVOID   plistProp;    /* property list */
  PSTR     pPrivate;     /* pointer to misc info (like edit buffer) */
  PVOID    pRestoreInfo; /* Pointer to restore info structure */
  HDC      hDC;          /* if the window has the CS_OWNDC style */
  HFONT    hFont;        /* Set by WM_SETFONT */
  PSTR     pMDIInfo;     /* pointer to MDI info */
  HINSTANCE hInstance;   /* instance handle of the window */
#if defined(_WPRIVATE_H) && (defined(MOTIF) || defined(DECWINDOWS))
  Widget   widget;
  Widget   widgetShell;
  Widget   widgetFrame;
  Widget   widgetDrawingArea;
#endif
#if defined(_WPRIVATE_H) && defined(XWINDOWS)
  Window   Xwindow;      /* the actual X window associated with this*/
  RECT     rWindowRoot;  /* window rectangle in root coordinates    */
  RECT     rClientRoot;  /* client rectangle in root coordinates    */
  DWORD    dwXflags;     /* X Windows specific flags for the window */
#define WSX_USEMEWELSHELL     0x00000001L  /* draw our own shell */
#define WSX_POPUPMENU         0x00000002L  /* this is a popup menu */
#endif
} WINDOW, *PWINDOW, FAR *LPWINDOW;



/*
  Some constants to be used with windows
*/
#define NULLHWND      ((HWND)  0)
#define NULLHMENU     ((HMENU) 0)
#define HWND_DESKTOP  1

/*
  #defines to isolate MEWEL innards from the flags and ulStyle bits
*/
#define WIN_IS_DLG            0x00000001L
#define WIN_SEND_WMSIZE       0x00000002L
#define WIN_ACTIVE_BORDER     0x00000004L
#define WIN_IS_MDIDOC         0x00000008L
#define SHADOW_TOPLEFT        0x00000010L
#define SHADOW_BOTLEFT        0x00000020L
#define SHADOW_TOPRIGHT       0x00000040L
#define SHADOW_BOTRIGHT       0L
#define WIN_IS_COMMDLG        0x00000100L
#define WIN_IS_MDICLIENT      0x00000200L
#define LBS_IN_COMBOBOX       0x00000400L /* TRUE if part of a combo box */
#define WIN_IS_ICONTITLE      0x00000800L /* TRUE if title of an icon    */
#define WIN_NOCLIP_TO_PARENT  0x00001000L
#define WIN_IS_MSGBOX         0x00002000L
#define MFS_RESTOREICON       0x00004000L /* for menubars and MDI */
#define WIN_IS_MENU           0x00008000L
#define WIN_DIDDLED_COLOR     0x00010000L
#define WIN_UPDATE_NCAREA     0x00020000L
#define WIN_HAS_SHADOWED_KIDS 0x00040000L
#define HAS_SHADOWED_KIDS(w) (((w)->ulStyle & WIN_HAS_SHADOWED_KIDS) != 0L)
#define WIN_HIDDEN            0x40000000L
#define WIN_SEND_ERASEBKGND   0x80000000L

#define SET_WS_HIDDEN(w)   ((w)->ulStyle |=  WIN_HIDDEN)
#define CLR_WS_HIDDEN(w)   ((w)->ulStyle &= ~WIN_HIDDEN)
#define TEST_WS_HIDDEN(w)  (((w)->ulStyle & WIN_HIDDEN) != 0L)

#define IS_MENU(w)    (((w)->ulStyle & WIN_IS_MENU)   != 0L)
#define IS_DIALOG(w)  (((w)->ulStyle & WIN_IS_DLG)    != 0L)
#define IS_ICONTITLE(w) (((w)->ulStyle & WIN_IS_ICONTITLE) != 0L)
#define IS_MSGBOX(w)  (((w)->ulStyle & WIN_IS_MSGBOX) != 0L)
#define IS_MDIDOC(w)  (((w)->ulStyle & WIN_IS_MDIDOC) != 0L)
#define IS_MDICLIENT(w) (((w)->ulStyle & WIN_IS_MDICLIENT) != 0L)
#define IS_ZOOMED(w)  (((w)->ulStyle & WIN_IS_ZOOMED) != 0L)

#define WinGetFillChar(hWnd)         (WID_TO_WIN(hWnd)->fillchar)
#define WinSetFillChar(hWnd,ch)      (WID_TO_WIN(hWnd)->fillchar = (ch))
#define WinSetShadowStyle(hWnd,sh) \
 (WID_TO_WIN(hWnd)->ulStyle &= ~(0x00F0L), WID_TO_WIN(hWnd)->ulStyle |= (sh))


/*
  Border Styles for WinDrawBorder()
*/
#define BORDER_SINGLE   0
#define BORDER_DOUBLE   1
#define BORDER_DASHED   2
#define BORDER_3D       3
#define BORDER_3DPUSHED 4
#define BORDER_MODAL    5
#define BORDER_POPUP	6


/*
  Generic linked list
*/
typedef struct list
{
  struct list *next;
  LPSTR  data;
} LIST, *PLIST;

/*
  For message hooks
*/
typedef struct tagHookInfo
{
  HHOOK    hHook;                 /* id of this hook */
  HOOKPROC lpfnHook;              /* address of the hook procedure */
  struct tagHookInfo *pNextHook;  /* next hook in the chain */
} HOOKINFO, *PHOOKINFO, FAR *LPHOOKINFO;



/*
  This structure contains some important MEWEL global variables.
*/
typedef struct tagWinSysParams
{
  HWND hWndActive;         /* Currently active window */
  HWND hWndSysModal;       /* System-modal window */
  HWND hWndCaret;          /* Window which has the caret */
  HWND hWndCapture;        /* Window which has the mouse captured */
  HWND hWndFocus;          /* Window which has the input focus */
  BOOL bVGAFontsDone;      /* Special VGA fonts used? */
  PSTR pszMEWELcopyright;  /* Copyright notice     */
  HCURSOR hCursor;         /* Current mouse cursor */
  BOOL  bInMsgBox;         /* for the WM_CTLCOLOR message */
  INT   fOldBrkFlag;       /* Were control-breaks originally enabled? */
  UINT  wWindowsCompatibilityState;
  DWORD fProgramState;     /* Internal flags */

  WINDOW *WindowList;      /* linked list of all existing windows */
  WINDOW *wDesktop;        /* pointer to desktop window */

#define MAXHOOKS  (WH_LAST + 1)
  int nTotalHooks;
  PHOOKINFO pHooks[MAXHOOKS];
} WINSYSPARAMS;
extern WINSYSPARAMS InternalSysParams;

#define _SetActiveWindow(hWnd)   (InternalSysParams.hWndActive = (hWnd))

/*
  The global variable which holds the handle of the desktop window
*/
extern HWND _HwndDesktop;


/*===========================================================================*/
/*                                                                           */
/*                       VIDEO INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/
extern struct videoinfo
{
  INT  length,
       width;
  WORD segment;
  WORD starting_mode;
  WORD flags;
/*
  Adapter Types
  (odd numbers signify a mono adapter)
*/
#define CGA       0
#define MDA       1
#define EGACOLOR  2
#define EGAMONO   3
#define VGACOLOR  4
#define VGAMONO   5
#define MCGACOLOR 6
#define MCGAMONO  7
#define VIDEO_NOSNOW    ((VideoInfo.flags & 0x07) != CGA)
#define USEVIDEOBIOS      0x0100
#define IN_GRAPHICS_MODE  0x0200
#define IN_MONO_MODE      0x0400
  int  yFontHeight;
  int  xFontWidth;
} VideoInfo;

#define VID_USE_BIOS()             (VideoInfo.flags & USEVIDEOBIOS)
#define VID_USE_GRAPHICS_BIOS()    (VideoInfo.flags & USEVIDEOBIOS)
#define VID_IN_GRAPHICS_MODE()     (VideoInfo.flags & IN_GRAPHICS_MODE)
#define VID_IN_MONO_MODE()         (VideoInfo.flags & IN_MONO_MODE)

/*
  Various variables used by the low-level video routines
*/
#define MAXSCREENWIDTH    160
extern BYTE  HilitePrefix;
#define HILITE_PREFIX   '~'       /* default prefix for highliting */
extern COLOR HilitePrefixAttr;

extern WORD  Scr_Base;
extern BYTE  bUseMonoMap;
extern BOOL  bDrawingBorder;
extern BOOL  bVirtScreenEnabled;
extern BOOL  bUseVirtualScreen;  /* set to FALSE if you don't want the VS */
extern BOOL  bRestoreDirectory;

/*
  Window-list related
*/
extern WINDOW ** TRUENEAR HwndArray;
#define MAXWINDOWS    128
extern WINDOW *FAR PASCAL WID_TO_WIN(HWND);
#define _WinGetLowestClass(i)  (ClassIDToClassStruct(i)->idLowestClass)

/*
  Internal GDI stuff
*/
extern HBRUSH FARDATA SysBrush[];  /* brushes for the COLOR_xxx objects */
extern HPEN   FARDATA SysPen[];    /* pens for the COLOR_xxx objects */
#define SYSPEN_BTNHIGHLIGHT  0
#define SYSPEN_BTNSHADOW     1
#define SYSPEN_WINDOWFRAME   2

/*
  Control classes ids put out by the resource compiler
*/
#define CT_MASK       0x80
#define CT_BUTTON     0x80
#define CT_EDIT       0x81
#define CT_STATIC     0x82
#define CT_LISTBOX    0x83
#define CT_SCROLLBAR  0x84
#define CT_COMBOBOX   0x85

/*
  Window class info
*/
#define MAXCLASSES  64
extern EXTWNDCLASS *ClassIndex[MAXCLASSES];


/*
  Compiler-independent CW_USEDEFAULT testing
*/
#if defined(MEWEL_32BITS)
#define IS_CW_USEDEFAULT(n)  ((n) == CW_USEDEFAULT || (n) == 0xFFFF8000)
#else
#define IS_CW_USEDEFAULT(n)  ((n) == CW_USEDEFAULT)
#endif

/*
  Definition of the window restoration information used when a window
  is maximized or minimized. This is stored in the last 2 or 4 bytes
  of the window's szClassExtra[] array.
*/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#define USE_ICON_TITLE
#endif

typedef struct tagRestoreInfo
{
  RECT  rect;         /* Original window rect  */
  DWORD ulOldFlags;   /* Original window flags */
  HWND  hIconTitle;   /* Handle to icon title window */
} RESTOREINFO, *PRESTOREINFO, FAR *LPRESTOREINFO;


/*
  Virtual Screen stuff
*/
typedef struct virtScreen
{
  LPSTR pVirtScreen;
  WORD  segVirtScreen;
#if defined(DOS)
  WORD  segPhysScreen;
#elif defined(OS2)
  LPSTR segPhysScreen;
#endif
  UINT  nRows, nCols;
  UINT  nScreenSize;
  INT   minBadCol, maxBadCol;
  BOOL  *pbRowBad;
  UINT  fRedrawFlags;
#define VS_DRAWALL   0x0001
} VIRTUALSCREEN;
extern VIRTUALSCREEN VirtualScreen;


/*
  This structure records some information about the system events.
*/
typedef UINT (FAR PASCAL *EVENTCHECKPROC)(LPMSG);

typedef struct tagSysEventInfo
{
  UINT chLastKey;       /* VK code of last key pressed  */
  UINT chLastShift;     /* last shift state recorded    */
  UINT chLastScan;      /* last scan  state recorded    */
  UINT lastMessage;     /* last message recorded        */
  LONG lParam;          /* lParam for key messages      */
  INT  nAltKeyPresses;  /* Has the ALT key been tapped? */
  EVENTCHECKPROC pfExternalEventChecker;  /* event checking function */
  UINT DblClickPendingMsg;  /* pending double click message? */
} SYSEVENTINFO, *LPSYSEVENTINFO;
extern SYSEVENTINFO SysEventInfo;

#ifdef INTERNATIONAL_MEWEL
#define SPECIAL_CHAR ((WORD) 0x10)
#endif

/*
  MEWEL Printer interface structures and typedefs. Used by SysGDIInfo.

   Definitions for the printer hooks used to avoid pulling in the 
   printer routines in programs not using printing.
*/  
#include "wprndev.h"
#ifdef MSC
typedef HDC (FAR PASCAL *CREATPRDCPROC)(LPSTR,LPSTR,LPSTR,LPSTR,BOOL);
typedef LPPRDEVICE (FAR PASCAL *PRDCXLATPROC)(HDC);
typedef BOOL (FAR PASCAL *DELPRDCPROC)(HDC);
#else
typedef HDC FAR PASCAL (*CREATPRDCPROC)(LPSTR,LPSTR,LPSTR,LPSTR,BOOL);
typedef LPPRDEVICE FAR PASCAL (*PRDCXLATPROC)(HDC);
typedef BOOL FAR PASCAL (*DELPRDCPROC)(HDC);
#endif



/*
  This structure contains information which is needed by the entire GDI.
*/
typedef struct tagGDIInfo
{
  BOOL   bGraphicsSystemInitialized;  /* Graphics system initialized? */
  BOOL   bWasBlinkingEnabled; /* TRUE if blinking was originally on */
  INT    iVideoMode;          /* Current video mode      */
  INT    cxScreen, cyScreen;  /* Screen dimensions       */
  INT    nColors;             /* Number of colors        */
  INT    nBitsPerPixel;       /* Bits per pixel          */
  INT    tmHeight;            /* Height of system font   */
  INT    tmExternalLeading;   /* Leading of system font  */
  INT    tmHeightAndSpace;    /* The above two combined  */
  INT    tmAveCharWidth;      /* Avg char width of font  */
  INT    iSavedVideoMode;     /* Original video mode     */
  INT    StartingVideoMode;   /* same as above, but used in text mode */
  INT    StartingVideoLength; /* original video length   */
  HFONT  hFontDefault;        /* Handle of default font  */
  HBITMAP hDefMonoBitmap;     /* default 1x1 mono bitmap */
  RECT   rectLastClipping;    /* set by GraphSetViewport */
  RECT   rectScreen;          /* always 0,0,VideoInfo.width,VideoInfo.length */

  /*
    These 3 function pointers are used by the printing module.
  */
  CREATPRDCPROC lpfnCreatPrDCHook;
  DELPRDCPROC   lpfnDelPrDCHook;
  PRDCXLATPROC  lpfnPrDCXlatHook;
  HPALETTE      hSysPalette;
  INT           CurrFontID;

  DWORD         fFlags;
  /* 
    Special printing mode for BGI and Ryle,Fleming,etc. When this is on,
    all BGI functions are routed to the printer driver. So, don't
    do things like draw window bitmaps.
  */
#define GDISTATE_BGI_PRINTING      0x00000001L
  /*
    By default, the MSC graphics.lib version uses the "best match"
    indicator for _setfont(). If this is set, then MEWEL will not
    use the best match. This can be set in the MEWEL.INI file
    in the [boot] section using the 'NoBestMatch' variable.
  */
#define GDISTATE_MSC_NOBESTMATCH   0x00000002L
  /*
    This flag is turned ON when the GX virtual screen is on. It is
    used to compensate for a protection violation in the GX lib.
    (grSetVirtual(FALSE) reads from location 0:0 if there was never
     a virtual screen set).
  */
#define GDISTATE_GX_VIRTUALON      0x00000004L

  /*
    This flag is used by InvertRect()
  */
#define GDISTATE_INVERTINGRECT     0x00000008L

  /*
    This flag is used by GetSysColor() in order to determine whether
    the SysBrush[] array has valid values in it.
  */
#define GDISTATE_SYSBRUSH_INITIALIZED  0x00000010L

  /*
    Don't use the beveled look on listbox, edit, and groupbox borders?
  */
#define GDISTATE_NO_BEVELS             0x00000020L

  /*
    These two variabled maintain a cache of widths for the characters
    in the current font. Used to speed up GetCharWidth().
  */
  LPINT         lpFontWidthCache;
  HFONT         hFontCached;

  /* The height and width of the screen in millimeters & inches */
  INT           cxScreenMM, cyScreenMM;
  INT           cxScreenInches, cyScreenInches;
  /* The number of pixels per logical inch */
  INT           cxLogPixel, cyLogPixel;
} GDIINFO;
extern GDIINFO SysGDIInfo;
extern BOOL bGraphicsSystemInitialized;

#if defined(WAGNER_GRAPHICS)
#define FONT_WIDTH     (vidmode.char_width)
#else
#define FONT_WIDTH     8
#endif

/*
  FONT Info
*/
#ifdef INCLUDE_FONTS
typedef struct tagFontNameInfo
{
  LPSTR lpFontName;  /* logical font name (Helv, Tms Rmn, etc) */
  LPSTR lpPathName;  /* DOS path name of graphics-engine font  */
  INT   hFont;       /* numeric id of font (for BGI) or mem handle */
  UINT  uNameHash;   /* simple hash code of lpFontName         */
  LPVOID lpFontBuf;  /* address of locked font. NULL if not locked */
} FONTNAMEINFO, FAR *LPFONTNAMEINFO;

/*
  This is a linked list of FONTNAMEINFOs, one for each font installed
*/
extern LIST *MEWELFontNameList;
#endif


typedef struct tagDeviceInfo
{
  INT   cxPixels,    cyPixels;
  INT   cxMM,        cyMM;
  INT   cxInches,    cyInches;
  INT   cxLogPixels, cyLogPixels;
} DEVICEINFO, FAR *LPDEVICEINFO;

/*
  Device context
*/
typedef struct dc
{
  HWND   hWnd;       /* the window that this DC is associated with */
  UINT   fFlags;
#define DC_ISWINDOWDC       0x0001
#define DC_ISMEMDC          0x0002
#define DC_BITMAPSELECTED   0x0004
#define DC_RESETING_OWNDC   0x0008
#define DC_SYSPALNOSTATIC   0x0010
#define DC_ATOMICOPERATION  0x0100  /* atomic drawing op -- don't reset vwpt */
#define DC_CMAPINSTALLED    0x0200
#define DC_ISOWNDC          0x1000
#define DC_ISCLASSDC        0x2000
#define DC_ISPRTDC          0x4000
#define DC_ISUSED           0x8000
#define IS_WINDOWDC(lphDC)       ((lphDC)->fFlags & DC_ISWINDOWDC)
#define IS_MEMDC(lphDC)          ((lphDC)->fFlags & DC_ISMEMDC)
#define IS_OWNDC(lphDC)          ((lphDC)->fFlags & DC_ISOWNDC)
#define IS_CLASSDC(lphDC)        ((lphDC)->fFlags & DC_ISCLASSDC)
#define IS_OWN_OR_CLASSDC(lphDC) ((lphDC)->fFlags & (DC_ISCLASSDC|DC_ISOWNDC))
#define IS_PRTDC(lphDC)          ((lphDC)->fFlags & DC_ISPRTDC)
#define IS_BMPSELECTED(lphDC)    ((lphDC)->fFlags & DC_BITMAPSELECTED)
#define IS_RESETING_OWNDC(lphDC) ((lphDC)->fFlags & DC_RESETING_OWNDC)
#define IS_DCUSED(lphDC)         ((lphDC)->fFlags & DC_ISUSED)
#define IS_DCFREE(lphDC)         (!((lphDC)->fFlags & DC_ISUSED))
#define IS_DCATOMIC(lphDC)       ((lphDC)->fFlags & DC_ATOMICOPERATION)

  UINT   iLockCnt;   /* number of locks on the DC */

  HANDLE hCanvas;    /* ptr to the current drawing surface - NULL is screen */

  RECT   rClipping;       /* the window clipping region */

  HRGN   hRgnVis;         /* Visibility region - calc'ed by MEWEL */
  HRGN   hRgnClip;        /* Clipping region - set by app */

  COLOR  attr;            /* the window colors */
  COLORREF clrBackground, /* background & foreground rgb values  */
           clrText;       /* default is white back and black fg  */
  HPEN   hPen;            /* pen to draw lines with */
  HBRUSH hBrush;          /* brush to draw with */
  HFONT  hFont;           /* current font   */
  HBITMAP hBitmap;        /* current bitmap */
  POINT  ptOrg;           /* DC origin */
  POINT  ptPen;           /* current pen coordinates */
  POINT  ptViewOrg;       /* viewport origin & extent */
  SIZE   extView;
  POINT  ptWindowOrg;     /* window origin & extent */
  SIZE   extWindow;
  POINT  ptBrushOrg;      /* brush origin - (0,0) is default     */
  INT    wDrawingMode;    /* drawing mode - R2_COPY is default   */
  INT    wMappingMode;    /* mapping mode - MM_TEXT is default   */
  INT    wBackgroundMode; /* background mode - OPAQUE is default */
  INT    wAlignment;      /* text alignment - TA_LEFT is default */
  INT    wStretchMode;    /* blt stretching mode                 */
  INT    wPolyFillMode;   /* polygon filling mode                */
  INT    wTextCharExtra;  /* text extra value - default is 0     */
  INT    wBitsPerPixel;   /* bits per pixel for this DC          */
  HPALETTE hPalette;      /* color palette                       */

  DEVICEINFO devInfo;     /* Device driver info                  */

  HANDLE hPrDevice;  /* if we have a printer DC, this pts to a PrDevice */

  LIST   *listSavedDCs;   /* for Save/RestoreDC */

  /*
    These are pointer to functions within the "device driver" which
    handle the GDI functions. One can theoretically write a new
    "driver" (ie - printer, graphics, KANJI) by setting hooks to all
    of these functions.
  */
  INT  (FAR PASCAL *lpfnDevCapsProc)(HDC, INT);
  INT  (FAR PASCAL *lpfnEscapeProc)(HDC, INT, INT, LPSTR, LPSTR);
  INT  (FAR PASCAL *lpfnTextOutProc)(HDC, INT, INT, LPSTR, UINT);
  BOOL (FAR PASCAL *lpfnDeleteDCHook)(HDC);

  /*
    This is a pointer to extra data associated with this DC. This can
    be a link to an underlying graphics engine structure, such as a
    PORT in MetaWindows or a gxHeader in Genus' GX Graphics.
  */
  LPVOID lpDCExtra;

#ifdef WAGNER_GRAPHICS
   vidcontext context;
#endif

  /*
    Stuff for MOTIF and XWINDOWS
  */
#if defined(MOTIF) || defined(DECWINDOWS)
   Widget      widget;
#endif
#if defined(XWINDOWS)
   GC          gc;
   Drawable    drawable;   /* window or pixmap */
   XFontStruct *pXFontStruct;
#endif

} DC, *PDC, *LPHDC;

extern LPHDC FAR PASCAL _GetDC(HDC);

/*
  This constant is passed as the window handle to the various DC routines
  when the device context is associated with a non-display device, such
  as a printer, plotter, etc.
*/
#define HWND_NONDISPLAY   0xFFFF

/*
  Macro to determine if the DC is a valid DC for a display rather than a prt
*/
#define IS_DISPLAY_DC(hDC)  ((hDC) > 0 && !_GetDC(hDC)->hPrDevice)



/*
  Hardware cursor information
*/
typedef struct tagCursorInfo
{
  int  row, col;
  int  startscan, endscan;
  int  nWidth;
  int  fState;
#define CURSOR_VISIBLE   0x0001
#define MAXCURSORWIDTH   32
#define MAXCURSORHEIGHT  32
  BYTE achSavedBits[MAXCURSORHEIGHT][MAXCURSORWIDTH];
} CURSORINFO;

extern CURSORINFO CursorInfo;
#define ISCURSORON()  (CursorInfo.startscan < CursorInfo.endscan)


#ifdef INCLUDE_CURSORS
/*
  A cursor handle with SYSCURSOR_MAGIC or'ed into it tells MEWEL
  that a system cursor is involved.
*/
#define SYSCURSOR_MAGIC  0x0800

/*
  The cursor signature distinguishes a cursor created by CreateCursor()
  from one created by LoadCursor().
*/
#define CURSOR_SIGNATURE MAKEINTRESOURCE(0x1128)

#if (defined(MEWEL_GUI) && (defined(META) || defined(__GNUC__))) || defined(XWINDOWS)
#define USE_32x32_CURSORS
#endif

typedef struct tagSysCursorInfo
{
  LPCSTR dwID;
  INT    nWidth, nHeight;
  INT    xHotSpot, yHotSpot;
  BOOL   bFromResource;
#if defined(USE_32x32_CURSORS)
  BYTE   achANDMask[(32/8) * 32];   /* DOS cursors are 16x16 */
  BYTE   achXORMask[(32/8) * 32];   /* DOS cursors are 16x16 */
#else
  BYTE   achANDMask[(16/8) * 16];   /* DOS cursors are 16x16 */
  BYTE   achXORMask[(16/8) * 16];   /* DOS cursors are 16x16 */
#endif
  /*
    The first mask is the screen mask (AND).
    The second mask is the cursor mask (XOR).
  */

#if defined(XWINDOWS)
  int    idXCursor;
  Cursor xCursor;
#endif

} SYSCURSORINFO, FAR *LPSYSCURSORINFO;

#define NSYSCURSORS  (sizeof(SysCursorInfo) / sizeof(SysCursorInfo[0]))
VOID FAR PASCAL MOUSE_SetGraphicsCursor(LPSYSCURSORINFO,int,int,LPVOID,LPVOID);
#endif /* INCLUDE_CURSORS */

/*
  Definition of the cursors in text mode
*/
extern BYTE FARDATA achSysCursor[];


extern INT MewelCurrOpenResourceFile;  /* file descriptor */

/*
  Keyboard related stuff
  Mostly found in WINKBD.C
*/
typedef UINT   VKEYCODE;

extern VOID     FAR PASCAL KBDInit(void);
extern VOID     FAR PASCAL KBDTerminate(void);
extern VKEYCODE FAR PASCAL KBDRead(void);
extern INT      FAR PASCAL KBDGetShift(void);
extern VOID         PASCAL InitKeyboard(void);  /* for UNIX */

/*
  Char pushback functions found in WKBSTACK.C
*/
extern VOID     FAR PASCAL nungetc(VKEYCODE);
extern VKEYCODE FAR PASCAL ngetc(void);

typedef struct checkboxinfo
{
  int  state;
  int  checkchar;
  BOOL bHasFocus;
} CHECKBOXINFO;


#ifdef INCLUDE_COMBOBOX
/*
  Combobox structure
*/
typedef struct combobox
{
  HWND  hEdit,              /* Handle to the edit/static component */
        hListBox;           /* Handle to the listbox component */
  INT   yOrigBottom;        /* The original bottom y coordinate */
  INT   yAdjustedBottom;    /* The adjusted bottom y coordinate */
  BYTE  chComboBoxIcon;     /* The char used for the icon */
#if defined(MOTIF)
  Widget widgetShell;
  Widget widgetArrow;
#endif
} COMBOBOX;

/*
  See comments in wincombo.c
*/
#define CBN_INTERNALSELCHANGE  99

#if defined(MOTIF)
#define SM_CXCOMBOICON   20
#define SM_CYCOMBOICON   28
#elif defined(MEWEL_GUI) || defined(XWINDOWS)
#define SM_CXCOMBOICON   IGetSystemMetrics(SM_CYSIZE)
#define SM_CYCOMBOICON   IGetSystemMetrics(SM_CXSIZE)
#else
#define SM_CXCOMBOICON   1
#define SM_CYCOMBOICON   1
#endif
#endif /* INCLUDE_COMBOBOX */


#if defined(INCLUDE_LISTBOX)
typedef struct listbox
{
  int  nStrings;    /* number of strings in the listbox */
  int  iTopLine;    /* index of string currently at the top of the box */
  int  iCurrSel;    /* index of currently selected item */
  WINDOW *wListBox;
  LIST *strList;    /* linked list of strings */
  HWND hListBox;    /* back reference to listbox window handle */
  int  nColumns;
  int  iLeftCol;
  int  iColWidth;
  int  nVisibleStrings;  /* # of strings which can be shown in the listbox */
  LPINT pTabStops; 
  INT  nTabStops;
  INT  tmHeight;          /* font height */
  INT  tmHeightAndSpace;  /* height plus leading */
  INT  tmWidth;           /* font width  */
  INT  wHorizExtent;      /* horizontal extent for scrolling */
} LISTBOX;

extern INT FAR PASCAL ListBoxDir(LISTBOX *,LPSTR,unsigned);


/*
  Undefine this if you don't want owner-drawn stuff in MEWEL.
*/
#define OWNERDRAWN

/*
  Routines used by wlistbox and wincombo to control owner-drawn listboxes.
*/
HDC  FAR PASCAL _ListBoxSetupDIS(WINDOW *, DRAWITEMSTRUCT *, HDC, LPHDC *);
VOID FAR PASCAL _ListBoxDrawOwnerDrawnItem(WINDOW *, int, int, COLOR,
                          BOOL, BOOL, BOOL, LIST *, PDRAWITEMSTRUCT, INT, BOOL, BOOL);
VOID FAR PASCAL _ListBoxDeleteOwnerDrawnItem(WINDOW *, int, LIST *);
BOOL FAR PASCAL DrawOwnerDrawnStatic(HWND);

#define IS_OWNERDRAWN(fl) ((fl) & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))
#define IS_OWNERDRAWN_WITHOUT_STRINGS(fl) \
                      (IS_OWNERDRAWN(fl) && !((fl) & LBS_HASSTRINGS))

#endif


#if defined(INCLUDE_SCROLLBAR)
typedef struct scrollbar
{
  int    minpos, maxpos, currpos;
  int    thumbpos;   /* offset of thumb position from window top-left */
  DWORD  sb_flags;
#define SB_HASFOCUS    0x0040  /* to give a slider (a lonely scrollbar) focus */
#define SB_FORCEREDRAW 0x0020
#define SB_NOREDRAWARROWS 0x0080  /* don't redraw arrows */
  RECT   rectThumb;  /* client-relative coordinates of thumb */
} SCROLLBARINFO;

#endif


#ifdef INCLUDE_MENU
/*
  The data structure associated with a menu...
*/
typedef struct menu
{
  HWND     hWnd;                 /* back reference to the menu's handle */
  INT      id;                   /* id associated with the menu */
  LIST     *itemList;            /* linked list of menu items */
  INT      nItems;               /* number of items in the list */
  INT      iCurrSel;             /* 0-based index of the current selection */
  DWORD    flags;                /* attribute flags */
#define M_NOBORDER     0x0000
#define M_HASBORDER    0x0001       /* Has a border */
#define M_MENUBAR      0x0002       /* Is a menubar, not a pulldown */
#define M_POPUP        0x0004       /* Is a pulldown,not a menubar  */
#define M_SYSMENU      0x0008       /* Is a system menu             */
#define M_AUTOACCEPT   0x0010       /* Accept when the user types the letter */
#define M_NOSHOWPOPUPS 0x0020       /* Don't show the underlying popup */
  HWND     hOldFocus;            /* Handle of the window which had the    */
  INT      iLevel;               /*   focus before the menu was invoked   */
} MENU, *LPMENU;                 

#define IS_MENUBAR(m)   ((m)->flags & M_MENUBAR)
#define IS_POPUP(m)     ((m)->flags & M_POPUP)
#define IS_SYSMENU(m)   ((m)->flags & M_SYSMENU)
#define DONT_SHOW_POPUPS(m)  ((m)->flags & M_NOSHOWPOPUPS)


/*
  Data structure for each menu item
*/
typedef struct menu_item
{
  LPSTR text;                /* NULL-terminated string or bitmap handle */
  INT  letter;               /* HotKey which activates this item */
  INT  id;                   /* The id which the WM_COMMAND passes back */
  UINT flags;                /* Any status info... */
  INT  iRightJustPos;        /* index of part of string which is right-just */
  INT  cyItem;               /* Pixel height of menu item */
  RECT rectItem;             /* Rectangle enclosing the menu item */
  INT  xTab;                 /* Tab position */
  INT  wTextExtent;          /* length of string */
  INT  xULStart;             /* coordinate relative to left of item of underline */
  INT  xULLength;            /* Length of underline */
  HBITMAP hbmChecked;        /* handle of bitmap to use for checkmark */
  HBITMAP hbmUnchecked;      /* handle of bitmap to use for "uncheckmark" */
#define DEFAULT_MENU_CHECK   ((HBITMAP) 0xFFFF)

#if defined(MOTIF) || defined(DECWINDOWS)
  Widget widget;             /* Pointer to Motif widget for the item */
#endif
} MENUITEM, FAR *LPMENUITEM;

typedef struct tagMenuInvertInfo
{
  BOOL  bInverting;
  INT   iOldSel;
  HMENU hMenu;
} MENUINVERTINFO;
extern MENUINVERTINFO MenuInvertInfo;


extern MENU *FAR PASCAL _MenuHwndToStruct(HWND);
extern VOID  FAR PASCAL MenuAdjustPopupSize(MENU *);
extern LPMENUITEM FAR PASCAL MenuGetItem(MENU *, INT, UINT);

extern DWORD PASCAL MenuGetItemDimensions(HMENU, LPMENUITEM);

/*
  Menu-related system variables
*/
#define SM_CXBETWEENMENUITEMS     (2*SysGDIInfo.tmAveCharWidth)
#define SM_CXBEFOREFIRSTMENUITEM  (2*SysGDIInfo.tmAveCharWidth)
#define SM_RESTOREICONWIDTH       (2*SysGDIInfo.tmAveCharWidth)

#endif /* INCLUDE_MENU */




#ifdef INCLUDE_REGIONS
/* 
  This is the array of rectangles which comprised the visibility list.
  In theory, there should be a vislist attached to every window, but
  we don't want to use up too much memory!
*/
typedef struct tagRegion
{
  HRGN hRgn;          /* Reference to the handle */
  INT  nRects;        /* # of rects in the rectangle list */
  INT  maxRects;      /* # of rects which space is allocated for */
  RECT rectBounding;  /* union of all rects in the list */
  RECT rects[1];      /* list starts here and grows downwards */
} REGION, *PREGION, FAR *LPREGION;

extern HRGN FAR PASCAL WinCalcVisRgn(HWND, HDC);

#if defined(__DPMI32__)
#define LOCKREGION(h)     GlobalLock(h)
#define UNLOCKREGION(h)   GlobalUnlock(h)
#else
#define LOCKREGION(h)     LocalLock(h)
#define UNLOCKREGION(h)   LocalUnlock(h)
#endif

#endif


/*
  This is the definition of a XREGION structure. This is kept in the
  'lpExtra' field of a HRGN object.
*/
#if defined(XWINDOWS) && defined(INCLUDE_XREGION)
typedef struct tagXRegion
{
  Region xRgn;          /* the X region structure */
  int    bRgnIsPixmap;  /* 0 = series of rects, 1 = pixmap */
  RECT   rectBounding;  /* bounding rect */
} XREGION, FAR *LPXREGION;
#endif
VOID FAR PASCAL EngineDeleteRegion(HANDLE);



/*
  Typedef for an index into a palette
*/
typedef UINT PALINDEX, FAR *LPPALINDEX;

/*
  The main difference between a "public" palette-entry object and an
  internal one is the addition of the 'peSysIndex' entry. This is
  the index of the logical color into the system hardware palette.
*/
typedef struct tagInternalPALETTEENTRY
{
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
/*UINT peSysIndex;*/
} INTPALETTEENTRY, FAR *LPINTPALETTEENTRY;
 
typedef struct tagInternalLOGPALETTE
{
  UINT palVersion;
  UINT palNumEntries;
  INTPALETTEENTRY  palPalEntry[1];
/*
  INTPALETTEENTRY  palPalEntry[nColors-1];
  PALINDEX         palIndex[nColors];
*/
} INTLOGPALETTE, *PINTLOGPALETTE, *NPINTLOGPALETTE, FAR *LPINTLOGPALETTE;
 
BOOL FAR PASCAL InitPaletteManager(void);
INT  FAR PASCAL MapPaletteIndex(HDC, INT);
LPINTLOGPALETTE PASCAL HPaletteToPtr(HPALETTE, HGLOBAL *);


#ifdef INCLUDE_MEMMGR
/*
  Memory block info. Used in WMEMGLOB.C and WMEMLOCL.C
*/
typedef struct tagMemHandle
{
  union                /* real memory address */
  {
    LPSTR lpMem;
    PSTR  pMem;
  } uMem;
  DWORD dwBytes;
  DWORD dwLRUCount;
  UINT  wFlags;
  INT   iLockCount;
  /* The above fields takes up 16 bytes */

  /* The following fields are for the internal management of the memory. */
  /* They take up another 6 bytes */
  UINT  fMemFlags;
#define BLOCK_IN_CONV  0x0001
#define BLOCK_IN_EMS   0x0002
#define BLOCK_IN_XMS   0x0004
#define BLOCK_ON_DISK  0x0008
#define HALLOC_BLOCK   0x0100
#define GLOBAL_BLOCK   0x1000
#define LOCAL_BLOCK    0x2000
#define NOTIFY_BLOCK   0x4000  /* cause GMEM_DISCARDED == GMEM_NOTIFY */
  union
  {
    DWORD ulXMSMem;   /* Handle/offset to location in XMS */
    DWORD ulEMSMem;   /* page number (0-3) and offset within page */
    DWORD ulDiskSeekPos;  /* seek position on disk */
  } uOutMem;
} MEMHANDLEINFO, *PMEMHANDLEINFO, FAR *LPMEMHANDLEINFO;

typedef BOOL FAR PASCAL NOTIFYPROC(HANDLE);

extern HANDLE FAR PASCAL FindFreeHandle(void);
extern LPMEMHANDLEINFO FAR PASCAL DerefHandle(HANDLE);


typedef struct tagMemoryMgr
{
  LPMEMHANDLEINFO HandleTable;
  DWORD           ulLRUCount;
  NOTIFYPROC      *lpfnNotify;
  UINT            nHandlesAlloced;
  UINT            fFlags;
#define MEM_USE_XMSLIB       0x0001
#define MEM_USE_MSCVM        0x0002
#define MEM_USE_DISK         0x0004
#define MEMMGR_DISCARDING    0x0100

  int             fdTemp;            /* handle to temp file for disk VM */
  LPSTR           lpszTempFile;      /* name of temp file */
  UINT            uMaxDOSMem;        /* max # of paras to use for MSC VMM */
} MEMMGRINFO;
extern MEMMGRINFO MewelMemoryMgr;

/*
  Define USE_HALLOC if we want to try using halloc() instead of
  _falloc for Microsoft C. The USE_HALLOC define is referenced in
  wemalloc.c and wmemglob.c.
*/
#if defined(MSC)
#if 0
#define USE_HALLOC
#endif
#endif

#endif /* INCLUDE_MEMMGR */



/*
  Clipboard stuff used in WCLIPBRD.C
*/
#define MAXFORMATS  16

typedef struct tagClipboard
{
  GLOBALHANDLE hData;  /* pointer to the clipboard data */
  UINT fmtData;        /* the format which the data is in */
  HWND hWndOpen;       /* who opened the clipboard      */
  HWND hWndOwner;      /* the clipboard owner           */
  HWND hWndViewer;     /* the first clipboard viewer    */
  UINT fFlags;         /* status bits                   */
#define CLIP_OPEN   0x0001
  UINT nFormats;       /* # of formats currently defined */
  struct
  {
    UINT  idFormat;
    LPSTR pszFormat;
    int   iRefCount;
  } fmtInfo[MAXFORMATS];
} CLIPBOARD;
extern CLIPBOARD _Clipboard;

/*
  Vismap stuff
*/
extern LPUINT WinVisMap;
#if !defined(MEWEL_TEXT)
#define WinInitVisMap()
#define VisMapPointToHwnd(y, x)  (0)
#define WinUpdateVisMap()    (SET_PROGRAM_STATE(STATE_SYNC_CARET))
#define _WinUpdateVisMap(w)  (SET_PROGRAM_STATE(STATE_SYNC_CARET))
#else
VOID FAR PASCAL WinInitVisMap(void);
HWND FAR PASCAL VisMapPointToHwnd(int, int);
VOID FAR PASCAL WinUpdateVisMap(void);
VOID FAR PASCAL _WinUpdateVisMap(WINDOW *);
#endif
HRGN FAR PASCAL WinCalcVisRgn(HWND, HDC);
BOOL FAR PASCAL IsVisRegionEmpty(HDC, LPRECT);
VOID     PASCAL DumpWindowTreeRegions(void);


/*
  System Metrics
*/
extern INT FAR CDECL MEWELSysMetrics[];
#define IGetSystemMetrics(n)   (MEWELSysMetrics[n])


/*
  Constants used for determining the position of iconic windows.
*/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
#define ICONHEIGHT        IGetSystemMetrics(SM_CYICON)
#define ICONWIDTH         IGetSystemMetrics(SM_CXICON)
#define CY_BETWEEN_ICONS  8
#define CX_BETWEEN_ICONS  8
#else
#define ICONHEIGHT        3
#define ICONWIDTH         15
#define CY_BETWEEN_ICONS  1
#define CX_BETWEEN_ICONS  1
#endif




/*===========================================================================*/
/*                                                                           */
/*                       MOUSE INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/
#ifdef INCLUDE_MOUSE

#define ERROR_MOUSE_NO_DEVICE	385

#define MOVE_WITH_NO_BUTTONS    0x01
#define MOVE_WITH_BUTTON1       0x02
#define BUTTON1_DOWN            0x04
#define MOVE_WITH_BUTTON2       0x08
#define BUTTON2_DOWN            0x10
#define MOVE_WITH_BUTTON3       0x20
#define BUTTON3_DOWN            0x40

#ifndef MOU_NOWAIT
#define MOU_NOWAIT              0
#define MOU_WAIT                1
#endif


#if defined(DOS) || defined(UNIX)
typedef struct _MOUEVENTINFO
{
  UINT   fs;      /* State of mouse at the time the event was reported*/
  DWORD  time;    /* time since boot in milliseconds      */
  UINT   row;     /* absolute/relative row position       */
  UINT   col;     /* absolute/relative column position    */
} MOUEVENTINFO;
typedef MOUEVENTINFO *PMOUEVENTINFO;
#endif

struct EventInfo
{
  UINT Mask;
  DWORD Time;
  UINT Row;
  UINT Col;
};


#ifdef DOS
typedef struct _PTRLOC
{
  UINT row;     /* pointer row coordinate screen position*/
  UINT col;     /* pointer column coordinate screen position*/
} PTRLOC;
typedef PTRLOC *PPTRLOC;
#endif

struct PtrLoc
{
  UINT RowPos;
  UINT ColPos;
};

#ifdef DOS
typedef struct _MOUQUEINFO
{
  UINT cEvents;    /* current number of event queue elements*/
  UINT cmaxEvents; /* MaxNumQueElements value           */
} MOUQUEINFO;
typedef MOUQUEINFO *PMOUQUEINFO;
#endif

struct QueInfo
{
  UINT Events;
  UINT QSize;
};

typedef struct mouseparams
{
  int m1,m2,m3,m4;
} MOUSEPARAMS;

#ifndef OS2
extern UINT FAR PASCAL MouOpen(LPSTR,unsigned FAR *);
extern UINT FAR PASCAL MouReadEventQue(struct _MOUEVENTINFO *,LPUINT,unsigned);
extern UINT FAR PASCAL MouSetDevStatus(unsigned FAR *,unsigned);
extern UINT FAR PASCAL MouGetNumQueEl(struct _MOUQUEINFO *,unsigned);
extern UINT FAR PASCAL MouSetPtrPos(struct _PTRLOC *,unsigned);
extern INT      FAR PASCAL MouseWaitForClick(struct _MOUEVENTINFO *,unsigned,int);
#endif /* OS2 */

#ifdef DOS
/* WINMOUSE.c */
extern VOID FAR PASCAL MOUSECALL(struct mouseparams *);
extern INT  FAR PASCAL MOUSE_Init(BOOL);
#define MOUSE_INIT       0
#define MOUSE_TERMINATE  1
extern VOID FAR PASCAL MOUSE_GetPress(int,int *,int *,int *,int *);
extern VOID FAR PASCAL MOUSE_GetStatus(int *,MWCOORD *,MWCOORD *);
extern VOID FAR PASCAL MOUSE_SetPos(int,int);
extern VOID FAR PASCAL MOUSE_LightPen(int);
extern VOID FAR PASCAL MOUSE_SetSpeed(int,int);
extern INT  FAR PASCAL MOUSE_HideCursor(void);
extern VOID FAR PASCAL MOUSE_GetRelease(int,int *,int *,int *,int *);
extern VOID FAR PASCAL MOUSE_GetMotion(int *,int *);
extern VOID FAR PASCAL MOUSE_SetThreshold(int);
extern VOID FAR PASCAL MOUSE_SetSubroutine(int,void (FAR *)());
extern INT  FAR PASCAL MOUSE_SetCursorMode(int,int,int);
extern VOID FAR PASCAL MOUSE_ConditionalOff(int,int,int,int);
extern INT  FAR PASCAL MOUSE_ShowCursor(void);
extern VOID FAR PASCAL MOUSE_SetHBounds(int,int);
extern VOID FAR PASCAL MOUSE_SetVBounds(int,int);
extern VOID FAR PASCAL MOUSE_ClipCursor(int,int,int,int);
#endif
#endif /* INCLUDE_MOUSE */


/*
  System-dependent chars which map into GDI stuff
*/
extern BYTE FARDATA SysPenDrawingChars[][6];
extern BYTE FARDATA SysBrushDrawingChars[];
extern BYTE FARDATA SysHatchDrawingChars[];
extern BYTE FARDATA SysBoxDrawingChars[][8];

/*
  For Windows Hooks
*/
#if defined(__TURBOC__) && !defined(__BORLANDC__)
typedef int      PASCAL MYHOOKPROC(int, UINT, DWORD);
typedef LRESULT  PASCAL WNDPROCHOOKPROC(int, UINT, DWORD);
typedef LRESULT  PASCAL WNDPROCHOOKPROC31(HHOOK, int, WPARAM, LPARAM);
typedef UINT     PASCAL COMMDLGHOOKPROC(HWND, UINT, WPARAM, LPARAM);
#else
typedef int  FAR PASCAL MYHOOKPROC(int, UINT, DWORD);
typedef LRESULT FAR PASCAL WNDPROCHOOKPROC(int, UINT, DWORD);
typedef LRESULT FAR PASCAL WNDPROCHOOKPROC31(HHOOK, int, WPARAM, LPARAM);
typedef UINT FAR PASCAL COMMDLGHOOKPROC(HWND, UINT, WPARAM, LPARAM);
#endif
extern FARPROC _lpfnHookProc[];
extern VOID FAR PASCAL _WinProcessWndProcHook(HWND*, UINT*, WPARAM*, LPARAM*);

/****************************************************************************/
/*                                                                          */
/*     Private, internal declarations                                       */
/*                                                                          */
/****************************************************************************/

/* WACTIVE.c */
extern VOID FAR PASCAL _WinActivateFirstWindow(void);
extern HWND FAR PASCAL _WinGetRootWindow(HWND);

extern UINT FAR PASCAL MEWELArrangeIconicWindows(HWND, BOOL);

/* WCHECKBX.c */
extern HWND FAR PASCAL CheckBoxCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,UINT);
extern HWND FAR PASCAL RadioButtonCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,UINT);
extern LONG FAR PASCAL ButtonWinProc(HWND,UINT,WPARAM,LPARAM);
extern VOID FAR PASCAL MEWELCheckRadioGroup(HWND);

/* WCREATE.c */
extern HWND FAR PASCAL WinCreate(HWND,int,int,int,int,LPSTR, COLOR,DWORD,int,int);

BOOL  FAR PASCAL LPtoSP(HDC, LPPOINT, int);
BOOL  FAR PASCAL SPtoLP(HDC, LPPOINT, int);

/*
  WEMALLOC.C
*/
BOOL FAR PASCAL GmemRealloc(LPSTR FAR *, UINT *, INT, INT);

/*
  WGRAPHIC.C
*/
int  FAR PASCAL WinOpenGraphics(int);
int  FAR PASCAL WinCloseGraphics(void);
VOID FAR PASCAL _ClientRectToPixels(LPRECT);
VOID FAR PASCAL WindowRectToPixels(HWND, LPRECT);
LONG FAR PASCAL GetClientPixelCursorPos(HWND, LONG);


/*
  WGUI.C
*/
BOOL FAR PASCAL WinTrackSysIcon(HWND, INT);


/*
  WINACCEL.C
*/
extern INT  FAR PASCAL AltKeytoLetter(int);
extern INT  FAR PASCAL LetterToAltKey(int);

/* WINALT.c */
extern VOID Int9Init(void);
extern VOID Int9Terminate(void);
#if defined(MSC) && defined(DOS) && !defined(ZORTECH) && !defined(__TSC__) && !defined(WC386) && !defined(PL386)
extern  VOID FAR _interrupt CDECL NewInt9(unsigned int es,unsigned int ds,unsigned int di,unsigned int si,unsigned int bp,unsigned int sp,unsigned int bx,unsigned int dx,unsigned int cx,unsigned int ax,unsigned int ip,unsigned int cs,unsigned int flags);
#endif
#if (defined(__TURBOC__) || defined(__TSC__)) && !defined(__DPMI32__) && !defined(PLTNT)
extern  VOID interrupt NewInt9(unsigned int bp,unsigned int di,unsigned int si,
               unsigned int ds,unsigned int es,unsigned int dx,unsigned int cx,
               unsigned int bx,unsigned int ax,unsigned int ip,unsigned int cs,
               unsigned int flags);
#endif

/* WINBREAK.c */
extern INT    FAR PASCAL getbrk(void);
extern VOID   FAR PASCAL rstbrk(void);
extern VOID   FAR PASCAL setbrk(void);

/* WINCLICK.c */
extern HWND   FAR PASCAL DetermineClickOwner(int,int, int*);
extern INT    FAR PASCAL DoNCHitTest(HWND, POINT);
extern LPRECT FAR PASCAL WinCalcWindowRects(HWND);

/* WINCOMBO.c */
extern LONG FAR PASCAL ComboBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LONG FAR PASCAL ComboListBoxWndProc(HWND,UINT,WPARAM,LPARAM);
extern LONG FAR PASCAL ComboEditWndProc(HWND,UINT,WPARAM,LPARAM);
extern HWND FAR PASCAL ComboBoxCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,int);

/* WINDLG.c */
extern BOOL FAR PASCAL IsStaticClass(HWND);
extern LONG FAR PASCAL DlgGetSetWindowLong(HWND, int, LONG, BOOL);
extern BOOL FAR PASCAL _IsDialogModal(HWND);

/*
  Stack of modal dialogs currently active
*/
extern HWND FARDATA _HDlgStack[];
extern int  DlgStackSP;


/* WINDRAW.c */
extern int  FAR PASCAL _WinPuts(HWND, HDC, int, int, LPSTR, COLOR, int, BOOL);

/* WINDSTRY.c */
extern VOID FAR PASCAL _WinDestroy(HWND);
extern VOID FAR PASCAL WinDelete(HWND);

/* WINEDIT.c */
extern HWND FAR PASCAL EditCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,int);
extern LONG FAR PASCAL EditWinProc(HWND,UINT,WPARAM,LPARAM);
extern VOID FAR PASCAL bytedel(LPSTR,UINT,UINT);
extern VOID FAR PASCAL byteinsert(LPSTR,UINT);


/* WINEVENT.c */
extern UINT FAR PASCAL GetEvent(LPMSG);
extern EVENTCHECKPROC FAR PASCAL EventSetHandler(EVENTCHECKPROC);
#if !defined(MAC)
extern INT  FAR PASCAL PostEvent(HWND,UINT,WPARAM,LPARAM,DWORD);
#endif

/* WININIT.c */
extern VOID FAR PASCAL _WinInit(void);
extern VOID FAR PASCAL _WinSaveDirectory(void);
#if defined(__WATCOMC__)
extern VOID           _WinRestoreDirectory(void);
#else
extern VOID     CDECL _WinRestoreDirectory(void);
#endif
extern HWND FAR PASCAL WinCreateDesktopWindow(void);
extern LONG FAR PASCAL _DesktopWndProc(HWND,UINT,WPARAM,LPARAM);

/* WININT24.c */
extern VOID FAR PASCAL Int24Install(void);
extern VOID FAR CDECL  Int24Restore(void);

/* WINLBDIR.c */
extern DWORD FAR PASCAL _DOSEnumDisks(void);

/*
  WINMENU.C
*/
extern LONG  FAR PASCAL MenuWndProc(HWND,UINT,UINT,LONG);
extern INT   FAR PASCAL IsMenuBarAccel(HMENU,UINT, int *, int *);
extern INT   FAR PASCAL MenuBarShowPopup(HMENU,int);
extern INT   FAR PASCAL MenuChar(PWINDOW,UINT);
extern INT   FAR PASCAL MenuRefresh(WINDOW *);
extern INT   FAR PASCAL MenuSaveScreen(HWND,BOOL);
extern HWND  FAR PASCAL WinGetMenuRoot(HWND);
extern VOID  FAR PASCAL _MenuFixPopups(PWINDOW,HWND);
extern INT FAR PASCAL InternalDrawMenuBar(HWND);

/*
  WMODULE.C
*/
extern INT  FAR PASCAL InternalGetModuleFileName(LPSTR, int);

/* WINQUEUE.c */
extern VOID FAR PASCAL InitEventQueue(void);

/* WINSCROL.C */
BOOL FAR PASCAL ScrollWindowOrDC(HWND,HDC,int,int,
                        CONST RECT FAR *,CONST RECT FAR *,LPRECT);

/* WINSHOW.c */
extern VOID FAR PASCAL _WinShowWindow(PWINDOW,BOOL);

/* WINTIMER.c */
extern VOID  FAR PASCAL KillWindowTimers(HWND);
extern INT   FAR PASCAL TimerCheck(LPMSG, BOOL);

/* WINVID.c */
extern INT   FAR PASCAL ToggleEGA(void);
extern INT   FAR PASCAL VidBlankRect(int,int,int,int,COLOR);
extern VOID  FAR PASCAL VidClearAllMonitors(COLOR);
extern INT   FAR PASCAL VidClearLine(int,int,int,COLOR,int,int);
extern VOID  FAR PASCAL VidClearScreen(COLOR);
extern INT   FAR PASCAL VidFillFrame(int,int,int,int,COLOR,COLOR);
extern INT   FAR PASCAL VidFrame(int,int,int,int,COLOR,int);
extern VOID  FAR PASCAL VidScroll(int,int,int,int,int,COLOR);
extern INT   FAR PASCAL MonitorSetRoutine(void (*)(int));
extern INT   FAR PASCAL MonitorSwitch(int,int);
extern VOID  FAR PASCAL VidSetPos(int,int);
extern VOID  FAR PASCAL VidGetPos(int *,int *);
extern INT   FAR PASCAL VidHideCursor(void);
extern INT   FAR PASCAL VidSetCursorScanLines(int,int,int);
extern INT   FAR PASCAL VidSetCursorMode(int,int);
extern INT   FAR PASCAL VidShowCursor(void);
extern LPSTR FAR PASCAL PointToScrAddress(int,int);
extern INT   FAR PASCAL VidGetMode(void);
extern INT   FAR PASCAL VidGetCurrVideoMode(void);
extern VOID  FAR PASCAL VidSetVideoMode(int);
extern VOID  FAR PASCAL _VidInitNewVideoMode(void);
extern INT   FAR PASCAL VidQueryAdapterType(void);
extern INT   FAR PASCAL VidQueryFontHeight(int);
extern VOID  FAR PASCAL VidTerminate(void);
#ifdef DOS
extern BOOL  FAR PASCAL VidSetBlinking(BOOL);
extern int   FAR PASCAL VioReadCellStr(LPCELL, int *, int, int, int);
extern VOID  FAR PASCAL VioWrtNCell(CELL, int, int, int, int);
extern VOID  FAR PASCAL VioWrtCellStr(LPCELL, int, int, int, int);
extern VOID  FAR PASCAL VGAVioWrtCellStr(LPCELL,int,int,int,int);
extern VOID  FAR PASCAL _VGAVioWrtCellStr(LPCELL,WORD,WORD,WORD,WORD);
#endif

/* WINVIRT.c */
#if !defined(MEWEL_TEXT)
#define VirtualScreenInit()
#define VirtualScreenFillBackground(attr)
#define VirtualScreenBad()
#define VirtualScreenRowBad()
#define VirtualScreenSetBadCols(c1, x2)
#define VirtualScreenEnable()            (TRUE)
#define VirtualScreenFlush()
#else
extern VOID FAR PASCAL VirtualScreenInit(void);
extern VOID FAR PASCAL VirtualScreenFillBackground(COLOR);
extern VOID FAR PASCAL VirtualScreenBad(void);
extern VOID FAR PASCAL VirtualScreenRowBad(int);
extern VOID FAR PASCAL VirtualScreenSetBadCols(int,int);
extern BOOL FAR PASCAL VirtualScreenEnable(void);
extern VOID FAR PASCAL VirtualScreenFlush(void);
#endif

/*
  WINZOOM.C
*/
extern VOID FAR PASCAL _GetIconXY(INT, HWND, INT *, INT *, INT *, INT *);
extern VOID FAR PASCAL WinDestroyIconTitle(WINDOW *);


/* WLISTBOX.c */
extern LONG FAR PASCAL ListBoxWinProc(HWND,UINT,WPARAM,LPARAM);
extern HWND FAR PASCAL ListBoxCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,UINT);
extern INT  FAR PASCAL ListBoxSetRedraw(HWND,BOOL);

/* WLNKLIST.c */
extern VOID FAR PASCAL ListInsert(LIST **,LIST *,LIST *);
extern INT  FAR PASCAL ListGetLength(LIST *);
extern VOID FAR PASCAL ListAdd(LIST **,LIST *);
extern LIST *FAR PASCAL ListGetNth(LIST *,int);
extern LIST *FAR PASCAL ListFindData(LIST *,LPSTR);
extern VOID FAR PASCAL ListDelete(LIST **,LIST *);
extern INT  FAR PASCAL ListFree(LIST **,int);
extern LIST *FAR PASCAL ListCreate(LPSTR);

/* WMEM.c */
extern VOID FAR PASCAL MemoryInit(void);
 

/* WMOUAPI.c */
extern BOOL FAR PASCAL MouseHideIfInRange (int, int);
extern INT  FAR PASCAL MouseInitialize(void);
extern VOID FAR PASCAL MouseTerminate(void);
extern INT  FAR PASCAL MouseShow(void);
extern INT  FAR PASCAL MouseHide(void);
extern VOID FAR PASCAL WinUpdateCaret(void);
extern BOOL FAR PASCAL CaretPush(int, int);
extern VOID FAR PASCAL CaretPop(void);
extern VOID FAR PASCAL MouseSetCursorChar(INT);
extern BOOL FAR PASCAL IsMouseLeftButtonDown(void);
extern INT  FAR PASCAL GetMouseMessage(LPMSG);
extern VOID     PASCAL MouseGetStatus(int *, MWCOORD *, MWCOORD *);

/* WNOTIFY.C */
extern VOID FAR PASCAL WinParentNotify(HWND, UINT, DWORD);

/* WOBJECT.C */
extern HBRUSH FAR PASCAL SysCreateSolidBrush(DWORD);
extern VOID   FAR PASCAL InitSysBrushes(void);

/* WPUSHBUT.c */
extern HWND FAR PASCAL PushButtonCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,INT);
extern VOID FAR PASCAL PushButtonDraw(HWND);

/* WREFRESH.c */
extern VOID PASCAL RefreshInvalidWindows(HWND);
extern VOID PASCAL WinGenInvalidRects(HWND, LPRECT);
extern VOID FAR PASCAL InternalInvalidateWindow(HWND, BOOL);
extern VOID FAR PASCAL InvalidateNCArea(HWND);
extern VOID FAR PASCAL _InvalidateRect(HWND, CONST RECT FAR *, BOOL);

/* WRUBBER.c */
extern INT  FAR PASCAL WinRubberband(HWND,UINT,int,int,int);

/* WSCRLBAR.C */
extern BOOL FAR PASCAL WinHasScrollbars(HWND, UINT);

/* WSETRECT.C */
extern VOID FAR PASCAL _WinAdjustRectForShadow(WINDOW *, LPRECT);

/* WSTATIC.c */
extern HWND FAR PASCAL StaticCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,INT,HINSTANCE);
extern LONG FAR PASCAL StaticWinProc(HWND,UINT,WPARAM,LPARAM);
extern BOOL FAR PASCAL IsFrameClass(WINDOW *);

/* WPOS.c */
extern BOOL FAR PASCAL ShouldWeUpdateVismap(WINDOW *);
extern INT  FAR PASCAL MEWELSend_WMMOVE(WINDOW *);
extern INT  FAR PASCAL MEWELSend_WMSIZE(WINDOW *);

/* WSTDPROC.c */
extern LONG FAR PASCAL StdWindowWinProc(HWND,UINT,WPARAM,LPARAM);

/* WSTFOCUS.c */
extern INT  FAR PASCAL WinSetFocus(HWND);
extern VOID FAR PASCAL WinRefreshActiveTitleBar(HWND);
extern HWND FAR PASCAL _WinActivate(HWND, BOOL);

/* WSYSMENU.c */
extern BOOL  FAR PASCAL _WinFindandInvokeSysMenu(HWND);
extern HMENU FAR PASCAL WinCreateSysMenu(HWND);
extern BOOL  FAR PASCAL WinActivateSysMenu(HWND);
#ifdef INTERNATIONAL_MEWEL
extern VOID  FAR PASCAL WinGetIntlMenuStrings(void);
#else
#define WinGetIntlMenuStrings()
#endif

/* WZORDER.C */
extern VOID FAR PASCAL _WinUnlinkChildFromParent(PWINDOW);
extern VOID FAR PASCAL _WinAddChild(HWND, HWND);
extern HWND FAR PASCAL GetParentOrDT(HWND);


#if defined(UNIX) || defined(VAXC)
extern int  FAR PASCAL VioReadCellStr(LPSTR, int *, int, int, int);
extern VOID FAR PASCAL VioWrtCell(CELL, int, int, int);
extern VOID FAR PASCAL VioWrtNCell(CELL, int, int, int, int);
extern VOID FAR PASCAL VioWrtCellStr(LPCELL, int, int, int, int);
extern VOID FAR PASCAL VIDmvaddstr(int, int, LPSTR);
#endif

#if !defined(XWINDOWS)
#define GUIInitialize()
#endif


extern VOID  FAR CDECL int23ini(LPSTR);
extern VOID  FAR CDECL int23res(void);
extern VOID  FAR CDECL int23rtn(void);
extern VOID  FAR CDECL int1brtn(void);
extern INT   FAR CDECL dosioctl(UINT, UINT, UINT, UINT, UINT, LPUINT, LPUINT);

extern LPSTR FAR PASCAL lcgacpy(LPSTR,LPSTR,UINT);

#if defined(OS2)
int CDECL InitEventThreads(void);
int CDECL SuspendEventThreads(VOID);
int CDECL ResumeEventThreads(VOID);
int CDECL TerminateKeyboard(VOID);
int CDECL MouseThreadInit(VOID);
int CDECL StartTimerThread(VOID);
int CDECL InitEventThreads(VOID);
#endif

VOID FAR PASCAL _TranslatePrefix(LPSTR szWindow);

extern VOID  FAR PASCAL lwmemset(LPCELL, CELL, UINT);
#if defined(OS2)
#define limemset   lwmemset
#else
extern VOID  FAR PASCAL limemset(LPUINT, UINT, UINT);
#endif

extern VOID PASCAL MEWELWriteDebugString(PSTR);

extern VOID FAR PASCAL GUIGetDCHook(HDC);
extern INT  FAR PASCAL GUIReleaseDCHook(HDC);


/*
  Starting with MSC 7, several DOS-specific functions have been un-ANSIfied.
*/
#if 0
#if defined(_MSC_VER) && (_MSC_VER >= 700)
#define INT86   _int86
#define INT86X  _int86x
#else
#define INT86   int86
#define INT86X  int86x
#endif
#endif

#define INT86   int86
#define INT86X  int86x

#if (defined(__DPMI16__) || defined(__DPMI32__))
/*
  RTM patching functions in WBORRTM.C
*/
VOID FAR PASCAL InitRTMSwitches(void);
VOID FAR PASCAL TerminateRTMSwitches(void);
#endif

/*
  Include resource info
*/
#ifdef MEWEL_RESOURCES
#include "winrc.h"
#endif

/*
  CRITICAL ERROR INFORMATION
*/
#define WRITE_PROTECT_ERR  0x0
#define UNKNOWN_UNIT       0x1
#define DRIVE_NOT_READY    0x2
#define UNKNOWN_CMD        0x3
#define CRC_ERR            0x4
#define BAD_DRV_REQUEST    0x5
#define SEEK_ERR           0x6
#define UNKNOWN_MEDIA      0x7
#define SECTOR_NOT_FOUND   0x8
#define OUT_OF_PAPER       0x9
#define WRITE_FAULT        0xA
#define READ_FAULT         0xB
#define GENERAL_FAILURE    0xC

extern  UINT Int24Err;
extern  PSTR Int24ErrMsg;
#define SET_INT24_ERR(code)     Int24Err = ((1 << 8) | (code))
#define GET_INT24_ERR()         (Int24Err & 0x00FF)
#define CLR_INT24_ERR()         Int24Err = 0
#define IS_INT24_ERR()          (Int24Err)

/*
  Internal color stuff
*/
extern COLORREF AttrToRGBMap[16];
#define AttrToRGB(attr)   (AttrToRGBMap[attr & 0x0F])

/*
  Private state info for MEWEL kernel
*/
VOID FAR PASCAL SET_PROGRAM_STATE(DWORD);
VOID FAR PASCAL CLR_PROGRAM_STATE(DWORD);
BOOL FAR PASCAL TEST_PROGRAM_STATE(DWORD);

#define STATE_SPAWNING                 0x000001L
#define STATE_USECLIPRECT              0x000002L
#define STATE_CACHEDSTRINGS            0x000004L
#define STATE_MOUSEHIDDEN              0x000008L
#define STATE_DONT_DIE                 0x000010L
#define STATE_MOUSED_ON_MENU           0x000040L
#define STATE_GRAPHICS_MOUSE           0x000080L
#define STATE_EXTENDED_ATTRS           0x000100L
#define STATE_INVALIDRECTS_EXIST       0x000200L
#define STATE_NO_WMSETVISIBLE          0x000400L
#define STATE_NO_WMSETCURSOR           0x000800L
#define STATE_NONBLOCKING_PEEKMSG      0x001000L
#define STATE_NO_UPDATEVISMAP          0x002000L
#define STATE_MUST_READ_SCREEN         0x004000L
#define STATE_DRAWINGSHADOW            0x008000L
#define STATE_NO_SAVESCREENUNDERCOMBO  0x010000L
#define STATE_OLD_STYLE_DLG_CONTROLS   0x020000L
#define STATE_HARDWARE_OFF             0x040000L
#define STATE_JUST_POLL_EVENTS         0x080000L
#define STATE_MUST_CHECK_EVENT         0x00100000L
#define STATE_NO_SAVEBITS              0x00200000L
#define STATE_PUSHBACKCHAR_WAITING     0x00400000L
#define STATE_USE_102KEY               0x00800000L
#define STATE_NO_XMS                   0x01000000L
#define STATE_SYNC_CARET               0x02000000L
#define STATE_DEFER_VISMAP             0x04000000L
#define STATE_NORTON_BUTTON            0x08000000L
#define STATE_FORCE_MONO               0x10000000L
#define STATE_EXITING                  0x80000000L

/*
  GUI flags
*/
#define STATE_USE_CLIPREGIONS          0x40000000L
#define STATE_META_CLIP                0x20000000L

/*
  Turn this variable ON to do heap-checking before malloc() and after free()
*/
extern BOOL MemDebug;

/*
  Internal string table
*/
extern LPSTR FARDATA SysStrings[];
#define SYSSTR_RESTORE  0
#define SYSSTR_MOVE     1
#define SYSSTR_SIZE     2
#define SYSSTR_MINIMIZE 3
#define SYSSTR_MAXIMIZE 4
#define SYSSTR_CLOSE    5
#define SYSSTR_CLOSEMDI 6
#define SYSSTR_OK       7
#define SYSSTR_CANCEL   8
#define SYSSTR_ABORT    9
#define SYSSTR_RETRY   10
#define SYSSTR_IGNORE  11
#define SYSSTR_YES     12
#define SYSSTR_NO      13
#define SYSSTR_HELP    14
#define SYSSTR_OUTOFMEMORY 15
#define SYSSTR_ERROR   16

 
