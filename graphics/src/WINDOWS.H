/*===========================================================================*/
/*                                                                           */
/* File    : WINDOWS.H                                                       */
/*                                                                           */
/* Purpose : Main header file for the window manager                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#ifndef _INC_WINDOWS
#ifndef __WINDOWS_H

#define _INC_WINDOWS
#define __WINDOWS_H


#ifndef RC_INVOKED
#if !defined(__WINDOW_H) && !defined(__MEWEL_WINDOW_H)

/*
  OWL must think that this here file is the WINDOWS.H file. But we have to
  leave __WINDOW_H alone, or OWL will never be able to include its own
  window.h file.
*/
#if defined(OWL)
#define __WINDOWS_H
#else
#define __WINDOW_H
#endif

#define __MEWEL_WINDOW_H


/*
  These defines are for the "lite" version of MEWEL
*/
#ifdef MEWELJR
#define NOALT                 /* no alt-key taps         */
#define NOCLIPBOARD           /* no clipboard            */
#define NOCOMBOBOXES          /* no comboboxes           */
#define NODC                  /* no windows porting layer stuff         */
#define NOGRAPHICS            /* can't write in graphics mode */
#define NOICONS               /* no character-mode icons */
#define NOKERNEL              /* no kernel related stuff */
#define NOMDI                 /* no multiple-document-interface */
#define NOMINMAX              /* no minimizing and maximizing of windows */
#define NOOBJECTS             /* no SelectObject, CreatePen/Brush/Font  */
#define NOOWNERDRAWN          /* no owner-drawn controls */
#define NORESOURCES           /* no resources            */
#define NORUBBERBANDING       /* can move nor resize window by dragging */
#define NOTIMERS              /* no software timers      */
#endif

/*
  Constant which defines the version of Windows which MEWEL is compatible with
*/
#define WINVER              0x030a     /* Windows 3.1 */

/*
  Using MOTIF implies using XWINDOWS
*/
#if defined(MOTIF)
#ifndef XWINDOWS
#define XWINDOWS
#endif
#define WMM
#ifdef WMMMAIN
#define MEWELMAIN
#endif
#endif

/*
  GUI or text version of MEWEL?
*/
#if !defined(MEWEL_GUI) && !defined(XWINDOWS)
#define MEWEL_TEXT
#endif

/*
  The constant WC386 is defined for WATCOM/386
*/
#if defined(__WATCOMC__) && defined(__FLAT__)
#define WC386
#define DOS
#define _MSC_VER  600
#endif

/*
  Metaware High C with Pharlap 386
*/
#if defined(__HIGHC__)
#define __FLAT__
#define PL386
#define WC386
#ifndef DOS
#define DOS
#endif
#define _MSC_VER  600
#endif

/*
  Microsoft C++ with Pharlap TNT
*/
#if defined(PLTNT) && defined(_MSC_VER)
#define PL386
#ifndef DOS
#define DOS
#endif
#endif

/*
  Microsoft C/C++
*/
#if defined(_MSC_VER)
#if !defined(MSC)
#define MSC            1
#endif
#endif

/*
  JPI Topspeed C/C++
*/
#ifdef __TSC__
#ifndef DOS
#define DOS                  1
#define MSC                  1
#endif
#endif

/*
  Borland's Turbo C/C++
*/
#ifdef __TURBOC__
#define TC
#ifndef DOS
#define DOS
#endif
#endif


#ifndef MEWEL
#define MEWEL                1
#endif



/*
  MEWEL_32BITS is defined when we are dealing with a 32-bit compiler,
  such as the ones on UNIX, VMS and 32-bits DOS extenders.
*/
#if !defined(MEWEL_32BITS)
#if defined(UNIX) || defined(VAXC) || defined(DOS386) || defined(WC386) || defined(PL386) || defined(__GNUC__) || defined(__DPMI32__)
#define MEWEL_32BITS
#endif
#endif


/*
  Workaround for old headers on SPARCompiler with C++
*/
#if defined(sunos) && defined(__cplusplus) && !defined(__GNUC__)
#define OLD_HEADERS
#endif

/*
  For certain UNIX architectures, the bytes need to be swapped in certain structures
*/
#if defined(__i486__) && !defined(i386)
#define i386
#endif
#if (defined(UNIX) || defined(unix)) && !defined(i386) && !defined(vax)
unsigned short swis(unsigned short);
unsigned long  swil(unsigned long);
#else
#ifndef BYTE_SWAP
#define BYTE_SWAP
#endif
#define swis(x)  (x)
#define swil(x)  (x)
#endif /* UNIX && !i386 && !vax */


/*
  Is this a DOS-extender version?
*/
#if defined(DOS286X)||defined(DOS16M)||defined(ERGO)||defined(DOS386)||defined(WC386)||defined(__HIGHC__)||defined(PL386) || (defined(__GNUC__) && !defined(UNIX))||defined(__DPMI16__)||defined(__DPMI32__)
#define EXTENDED_DOS
#endif

/*
  Define the MEWEL major and minor versions
*/
#define MEWEL_VERSION_MAJOR  4
#define MEWEL_VERSION_MINOR  0
/* For the GetVersion() call, minor in HIBYTE, major in LOBYTE */
#define MEWEL_VERSION        ((MEWEL_VERSION_MINOR << 8) | MEWEL_VERSION_MAJOR)

#ifdef __TSC__
#pragma data(ss_in_dgroup => on)
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
  Define YES, NO, TRUE, FALSE
*/
#define NO      0
#define YES     1
#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

/*
  Define NULL
*/
#ifdef NULL
#undef NULL
#endif
#define NULL    0

/*
  Keywords
*/

/*
  Definitions of the PC-specific near/far and cdecl/pascal
*/
#if defined(UNIX) || defined(VAXC) || defined(MAC) || (defined(PLTNT) && !defined(__WATCOMC__)) || defined(__GNUC__) || defined(__HIGHC__)
#define far
#define near
#define pascal
#define cdecl
#endif

#if defined(__DPMI32__)
#define near
#ifdef pascal
#undef pascal
#endif
#endif

#if !defined(FAR)
#if defined(MEWEL_32BITS)
#define FAR         
#define NEAR
#else
#define FAR         far
#define NEAR
#endif
#endif

#ifndef PASCAL
#if defined(__DPMI32__)
#define PASCAL      __stdcall
#else
#define PASCAL      pascal
#endif
#endif

#ifndef CDECL
#define CDECL       cdecl
#endif

/*
  MEWEL programs should ignore "export" cause the linker makes them into
  Windows EXEs.
*/
#define EXPORT
#if !defined(__DPMI16__) && !defined(__DPMI32__)
#ifdef _export
#undef _export
#endif
#ifdef __export
#undef __export
#endif
#define _export
#define __export
#endif

/*
  We virtualize the word CONST in case of any non-ANSI compilers out there
*/
#if 1
#define CONST       const
#else
#define CONST
#endif

/*
  Virtualize UNSIGNED. On most systems, we need unsigned chars, but
  C++ freaks at this.
*/
#ifdef __cplusplus
#define UNSIGNED       unsigned
#else
#define UNSIGNED       unsigned
#endif

#define VOID           void

/*
  Typedefs for BYTE and CHAR
  A BYTE is signed under C++ and unsigned under normal C
*/
#if defined(AFX)
#if !defined(__AFX_H__) || defined(AFXNT)
typedef unsigned char  BYTE;
#endif /* __AFX_H__ */
#elif (!defined(OS2) || !defined(INCLUDE_OS2)) && !defined(XMD_H)
typedef UNSIGNED char  BYTE;
#endif

/*
  Typedefs for WORD, INT, and UINT
  A WORD is 16 bits in DOS and 32 bits under UNIX, VMS, and 32-bit DOS
*/
#if defined(AFX) || defined(CPPV) || defined(OWL)
#if !defined(__AFX_H__) || defined(AFXNT)
typedef unsigned short WORD;
#endif
#elif defined(DDE_SHMEM) /* DDE extension */
typedef unsigned long  WORD;
#elif defined(UNIX)
typedef unsigned short WORD;
#else
typedef unsigned int   WORD;
#endif

#define INT            int 
#if !defined(__AFX_H__) || defined(AFXNT)
typedef unsigned int   UINT;
#endif

/*
  typedef for BOOL and USHORT
  A USHORT *must* be an 'unsigned short' in case WORD is defined
  to be a 32 bit integer on UNIX
*/
#if defined(OS2)
#define BOOL           INT 
#elif defined(UNIX)
#if !defined(__AFX_H__) && !defined(XMD_H)
typedef unsigned char  BOOL;
#endif
#elif (!defined(__AFX_H__) || defined(AFXNT)) && !(defined(PL386) && defined(_WPRIVATE_H)) || defined(__HIGHC__)
typedef int            BOOL;
#endif

#if !defined(__AFX_H__) || defined(AFXNT)
#ifndef SHORT
#define SHORT          short
#endif
#endif

#if !(defined(PL386) && defined(_WPRIVATE_H)) || defined(__HIGHC__)
typedef unsigned short USHORT;
#endif

/*
  Typedef's for 32-bit numbers
*/
#if defined(OS2)
#define LONG          long
#elif (!defined(__AFX_H__) || defined(AFXNT)) && !(defined(PL386) && defined(_WPRIVATE_H)) || defined(__HIGHC__)
typedef long          LONG;
#endif

#if !defined(__AFX_H__) || defined(AFXNT)
typedef unsigned long DWORD;
#endif
#if !defined(PLTNT)
typedef DWORD         ULONG;
#endif

/*
  Typedefs for coordinate values.
*/
#if defined(MEWEL_32BITS)
typedef SHORT         MWCOORD;
#else
typedef INT           MWCOORD;
#endif

/*
  Various pointers to strings
*/
#if !defined(__AFX_H__) || defined(AFXNT)
typedef char FAR      *LPSTR;
typedef CONST char FAR*LPCSTR;
#if defined(AFXNT) || defined(OWL)
typedef LPSTR         LPWSTR;
typedef LPSTR         LPTSTR;
typedef LPCSTR        LPCWSTR;
typedef LPCSTR        LPCTSTR;
#endif
#endif
typedef char          *PSZ;
typedef char          *PSTR;
typedef CONST char    *PCSTR;
typedef char          *NPSTR;

/*
  Pointers to data types
*/
typedef BYTE          *PBYTE;
typedef BYTE FAR      *LPBYTE;
typedef INT           *PINT;
typedef INT FAR       *LPINT;
typedef UINT FAR      *LPUINT;
typedef WORD          *PWORD;
typedef WORD FAR      *LPWORD;
typedef LONG          *PLONG;
typedef LONG FAR      *LPLONG;
typedef DWORD         *PDWORD;
typedef DWORD FAR     *LPDWORD;
typedef VOID          *PVOID;
typedef VOID FAR      *LPVOID;

typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;

/*
  Macros to extract words and bytes and to build long values
*/
#if defined(STRICT) && defined(MEWEL_32BITS)
#define MAKELONG(lo,hi) ((LONG)(((USHORT)(DWORD)(lo)) | (((DWORD)((USHORT)(DWORD)(hi))) << 16)))
#else
#define MAKELONG(lo,hi) ((LONG)(((USHORT)(lo)) | (((DWORD)((USHORT)(hi))) << 16)))
#endif
#define LOWORD(l)       ((USHORT) ((DWORD) (l)))
#define HIWORD(l)       ((USHORT)(((DWORD)(l) >> 16) & ((USHORT) 0xFFFF)))
#ifndef LOBYTE
#define LOBYTE(w)       ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)       ((BYTE)(((USHORT)(w) >> 8) & ((BYTE) 0xFF)))
#endif


/*
  Various Windows 3.1 macros
*/
#if defined(OS2)
#undef  CALLBACK
#undef  SELECTOROF
#undef  OFFSETOF
#undef  FIELDOFFSET
#endif

#if defined(__DPMI32__)
#define WINAPI      PASCAL __import
#define CALLBACK    PASCAL
#else
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL
#endif

#define MAKELPARAM(low, high)       ((LPARAM)MAKELONG(low, high))
#define MAKELRESULT(low, high)      ((LRESULT)MAKELONG(low, high))
#define MAKELP(sel, off)            ((VOID FAR*)MAKELONG((off), (sel)))
#define SELECTOROF(lp)              HIWORD(lp)
#define OFFSETOF(lp)                LOWORD(lp)
#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

/*
  For NT
*/
#define APIENTRY  WINAPI


/*
  HANDLE definition and all of its various offspring.
  Some of these are included merely for MS-Windows compatibility
*/
#ifdef STRICT
typedef const void near*        HANDLE;
#define DECLARE_HANDLE(name)    struct name##__ { int unused; }; \
                                typedef const struct name##__ near* name
#define DECLARE_HANDLE32(name)  struct name##__ { int unused; }; \
                                typedef const struct name##__ FAR* name
#else   /* STRICT */
typedef UINT            HANDLE;
#define DECLARE_HANDLE(name)    typedef UINT name
#define DECLARE_HANDLE32(name)  typedef DWORD name
#endif

typedef HANDLE          *PHANDLE;
typedef HANDLE NEAR     *SPHANDLE;
typedef HANDLE FAR      *LPHANDLE;



/*
  Various data types which are typedef'ed off of HANDLE
*/
#ifdef OS2
#define HWND   HANDLE
#else
DECLARE_HANDLE(HWND);
#endif
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HRSRC);
typedef HINSTANCE HMODULE;

#if defined(INCLUDE_OS2) && defined(OS2)
#define HFILE    int
#define HDC      HANDLE
#define HBITMAP  HANDLE
#define HRGN     HANDLE
#else
typedef int      HFILE;
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HRGN);
#endif

DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HSTR);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HFONT);
typedef HICON    HCURSOR;
#define NORMAL_CURSOR   0
DECLARE_HANDLE(HPALETTE);
typedef DWORD    COLORREF;

#ifdef STRICT
typedef const void near* HGDIOBJ;
#else
DECLARE_HANDLE(HGDIOBJ);
#endif

/*
  Typedefs for various functions pointers
*/
typedef INT          (FAR PASCAL *PFI)();
typedef BOOL         (FAR PASCAL *PFBOOL)();
typedef WORD         (FAR PASCAL *PFUI)();
typedef long         (FAR PASCAL *PFLONG)();
typedef DWORD        (FAR PASCAL *PFULONG)();
typedef INT          (FAR PASCAL *FARPROC)();
typedef INT          (    PASCAL *NEARPROC)();
typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
#if defined(STRICT)
typedef BOOL (CALLBACK* ABORTPROC)(HDC, int);
#else
typedef FARPROC                   ABORTPROC;
#endif
typedef FARPROC                   GRAYSTRINGPROC;
typedef FARPROC                   TIMERPROC;


/*
  Define min() and max()
  (Note : Borland doesn't like this in C++ mode)
*/
#if !defined(NOMINMAX) && (!defined(__cplusplus) || defined(_MSC_VER) || defined(UNIX))
#ifndef max
#define max(a,b)        ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a,b)        ( ((a) < (b)) ? (a) : (b) )
#endif
#endif /* NOMINMAX */


/*
  The following typedef defines a color in MEWEL. It is currently a
  8-bit BIOS color value, but it can change eventually to a 24-bit
  RGB value.  (3/27/91 - maa)
*/
#if defined(INCLUDE_OS2) && defined(OS2)
#define COLOR   WORD 
#else
typedef UINT    COLOR;
#endif

typedef LPARAM FAR PASCAL WINPROC(HWND, UINT, WPARAM, LPARAM); /* window procedure */
typedef BOOL FAR PASCAL DIALOGPROC(HWND, UINT, WPARAM, LPARAM); /* Dialog procedure */
typedef FARPROC DLGPROC;
typedef FARPROC HOOKPROC;
DECLARE_HANDLE32(HHOOK);


/*
  The POINT structure and MAKEPOINT() macro. Note that under systems where
  sizeof(int) == 32, we need to make x and y SHORT values so that the
  MAKEPOINT macro can work.
*/
typedef struct tagPOINT
{
  MWCOORD x, y;
} POINT, *PPOINT, FAR *LPPOINT;
#if !defined(BYTE_SWAP)
extern POINT MAKEPOINT(DWORD);
#else
#define MAKEPOINT(lParam)    (*((POINT *)&(lParam)))
#endif


typedef struct tagSIZE
{
  MWCOORD cx, cy;
} SIZE, *PSIZE, *NPSIZE, FAR *LPSIZE;

typedef struct tagRECT
{
  /*
    For DOSX, need short because of LPToDP(hDC, (LPPOINT) &rect, 2);
  */
  MWCOORD left,
        top,
        right,
        bottom;
} RECT, *PRECT, *NPRECT, FAR *LPRECT;
#if !defined(__AFX_H__) || defined(AFXNT)
typedef const RECT *LPCRECT;
#endif /* __AFX_H__ */


/****************************************************************************/
/*                                                                          */
/*                       WINDOW CLASS INFORMATION                           */
/*                                                                          */
/****************************************************************************/
typedef struct tagWNDCLASS
{
  UINT    style;                    /* style bits                     */
  WINPROC *lpfnWndProc;             /* pointer to window proc         */
  int     cbClsExtra;               /* # of extra bytes for the class */
  int     cbWndExtra;               /* # of extra bytes per window    */
  HINSTANCE hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCSTR  lpszMenuName;
  LPCSTR  lpszClassName;            /* the class name                 */
} WNDCLASS, *PWNDCLASS, FAR *LPWNDCLASS, *NPWNDCLASS;


typedef struct extWndClass
{
  UINT   style;                     /* style bits                     */
  WINPROC *lpfnWndProc;             /* pointer to window proc         */
  int     cbClsExtra;               /* # of extra bytes for the class */
  int     cbWndExtra;               /* # of extra bytes per window    */
  HINSTANCE hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCSTR  lpszMenuName;
  LPCSTR  lpszClassName;            /* the class name                 */
  /*------- extra fields --------*/
  struct extWndClass *next;
  WINPROC *lpfnDefWndProc;
  LPSTR  lpszBaseClass;             /* the name of the base class     */
  UINT   idClass;                   
  UINT   idBaseClass;               /* the id of the derived class    */
  UINT   idLowestClass;
  HDC    hDC;                       /* if it has CS_CLASSDC           */
  PSTR   lpszClassExtra;
} EXTWNDCLASS, *PEXTWNDCLASS, FAR *LPEXTWNDCLASS, *NPEXTWNDCLASS;


/* Class styles */
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_SAVEBITS         0x0800
#define CS_DBLCLKS          0x0008
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_NOCLOSE          0x0200
#define CS_KEYCVTWINDOW     0x0004
#define CS_NOKEYCVT         0x0100
#define CS_GLOBALCLASS      0x4000

/* Values for Get/SetClassWord() and Get/SetClassLong() */
#define GCL_MENUNAME        (-8)
#define GCW_HBRBACKGROUND   (-10)
#define GCW_HCURSOR         (-12)
#define GCW_HICON           (-14)
#define GCW_HMODULE         (-16)
#define GCW_CBWNDEXTRA      (-18)
#define GCW_CBCLSEXTRA      (-20)
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)
#define GCW_ATOM            (-32)
/*
  MEWEL-specific
*/
#define GCL_DEFPROC         (-36)


/*
  These are the values of WINDOW.idCLass
*/
#define NORMAL_CLASS      0
#ifndef OWL
#define BUTTON_CLASS      1
#endif
#define EDIT_CLASS        2
#define LISTBOX_CLASS     3
#define SCROLLBAR_CLASS   4
#ifndef OWL
#define STATIC_CLASS      5
#endif
#define PUSHBUTTON_CLASS  6
#define CHECKBOX_CLASS    7
#define RADIOBUTTON_CLASS 8
#define TEXT_CLASS        9
#define FRAME_CLASS       10
#define BOX_CLASS         11
#define ICON_CLASS        12
#define COMBO_CLASS       13
#define DIALOG_CLASS      14
#define MENU_CLASS        15
#define USER_CLASS        (MENU_CLASS+1)

/*
  Window field offsets for Get/SetWindowLong() and Get/SetWindowWord()
*/
#define GWW_HINSTANCE   (-6)
#define GWW_HWNDPARENT  (-8)
#define GWW_ID          (-12)
#define GWL_EXSTYLE     (-20)
#define GWL_STYLE       (-16)
#define GWL_WNDPROC     (-4)

#if defined(AFXNT)
#define GWL_HINSTANCE   GWW_HINSTANCE
#define GWL_HWNDPARENT  GWW_HWNDPARENT
#define GWL_ID          GWW_ID
#define GWL_USERDATA    (-21)
#endif

/* Get/SetWindowWord/Long offsets for use with WC_DIALOG windows */
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     4
#define DWL_USER        8


typedef struct tagCREATESTRUCT
{
  VOID FAR *lpCreateParams;
  HINSTANCE hInstance;
  HMENU     hMenu;
  HWND      hwndParent;
  int       cy;
  int       cx;
  int       y;
  int       x;
  LONG      style;
  LPCSTR    lpszName;
  LPCSTR    lpszClass;
  DWORD     dwExStyle;
} CREATESTRUCT, FAR *LPCREATESTRUCT;


/*
  Flags for GetNextWindow() and GetWindow()
*/
#define GW_HWNDFIRST   0
#define GW_HWNDLAST    1
#define GW_HWNDNEXT    2
#define GW_HWNDPREV    3
#define GW_OWNER       4
#define GW_CHILD       5
  
/*
  Flags for ShowWindow()
*/
#define SW_HIDE	          0
#define SW_SHOWNORMAL     1
#define SW_NORMAL         1
#define SW_SHOWMINIMIZED  2
#define SW_SHOWMAXIMIZED  3
#define SW_MAXIMIZE       3
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW	          5
#define SW_MINIMIZE       6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA         8
#define SW_RESTORE        9
/* Obsolete ShowWindow() command names */
#define HIDE_WINDOW       0
#define SHOW_OPENWINDOW   1
#define SHOW_ICONWINDOW   2
#define SHOW_FULLSCREEN   3
#define SHOW_OPENNOACTIVATE 4


/*
  Memory manager definitions
*/
typedef HANDLE      LOCALHANDLE;
typedef LOCALHANDLE MEMHANDLE;
typedef HANDLE      GLOBALHANDLE;
typedef HANDLE	    HGLOBAL;
typedef HANDLE	    HLOCAL;

/*
  Memory Manager Functions
*/
#define GMEM_FIXED         0x0000
#define GMEM_MOVEABLE      0x0002
#define GMEM_NOCOMPACT     0x0010
#define GMEM_NODISCARD     0x0020
#define GMEM_ZEROINIT      0x0040
#define GMEM_MODIFY        0x0080
#define GMEM_DISCARDABLE   0x0100
#define GMEM_NOT_BANKED    0x1000
#define GMEM_SHARE         0x2000
#define GMEM_DDESHARE      0x2000
#define GMEM_NOTIFY        0x4000

#define GMEM_LOCKCOUNT     0x00FF
#define GMEM_DISCARDED	   0x4000

#define GHND               (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR               (GMEM_FIXED | GMEM_ZEROINIT)

#define LMEM_FIXED         0x0000
#define LMEM_MOVEABLE      0x0002
#define LMEM_NOCOMPACT     0x0010
#define LMEM_NODISCARD     0x0020
#define LMEM_ZEROINIT      0x0040
#define LMEM_MODIFY        0x0080
#define LMEM_DISCARDABLE   0x0F00

#define LMEM_DISCARDED	    0x4000
#define LMEM_LOCKCOUNT	    0x00FF

#define LHND		    (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR		    (LMEM_FIXED | LMEM_ZEROINIT)


/*
  PAINTSTRUCT
*/
typedef struct paintstruct
{
  HDC hdc;         /* The DC returned by BeginPaint()    */
  BOOL fErase;     /* TRUE if the window had been erased */
  RECT rcPaint;    /* rect in which painting is required */
} PAINTSTRUCT, *PPAINTSTRUCT, FAR *LPPAINTSTRUCT;


/* Owner draw control types */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4

/* Owner draw actions */
#define ODA_DRAWENTIRE  0x0001
#define ODA_SELECT      0x0002
#define ODA_FOCUS       0x0004

/* Owner draw state */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010

/* MEASUREITEMSTRUCT for ownerdraw */
typedef struct tagMEASUREITEMSTRUCT
{
  UINT  CtlType;
  UINT  CtlID;
  UINT  itemID;
  UINT  itemWidth;
  UINT  itemHeight;
  DWORD itemData;
} MEASUREITEMSTRUCT, *PMEASUREITEMSTRUCT, FAR *LPMEASUREITEMSTRUCT;


/* DRAWITEMSTRUCT for ownerdraw */
typedef struct tagDRAWITEMSTRUCT
{
  UINT  CtlType;
  UINT  CtlID;
  UINT  itemID;
  UINT  itemAction;
  UINT  itemState;
  HWND  hwndItem;
  HDC   hDC;
  RECT  rcItem;
  DWORD itemData;
} DRAWITEMSTRUCT, *PDRAWITEMSTRUCT, FAR *LPDRAWITEMSTRUCT;

/* DELETEITEMSTRUCT for ownerdraw */
typedef struct tagDELETEITEMSTRUCT
{
  UINT  CtlType;
  UINT  CtlID;
  UINT  itemID;
  HWND  hwndItem;
  DWORD itemData;
} DELETEITEMSTRUCT, *PDELETEITEMSTRUCT, FAR *LPDELETEITEMSTRUCT;

/* COMPAREITEMSTUCT for ownerdraw sorting */
typedef struct tagCOMPAREITEMSTRUCT
{
  UINT  CtlType;
  UINT  CtlID;
  HWND  hwndItem;
  UINT  itemID1;
  DWORD itemData1;
  UINT  itemID2;
  DWORD itemData2;
} COMPAREITEMSTRUCT, *PCOMPAREITEMSTRUCT, FAR *LPCOMPAREITEMSTRUCT;

/*
  For Catch() and Throw()
*/
#include <setjmp.h>

/*
  MSC 8 ignores everything in setjmp.h if __cplusplus is defined, so let's
  define it here ourselves.
*/
#if defined(AFX) && !defined(__HIGHC__) && !defined(AFXNT) && !defined(UNIX)
#define _JBLEN	9  /* bp, di, si, sp, ret addr, ds */
typedef  int  jmp_buf[_JBLEN];
int  __cdecl setjmp(jmp_buf);
void __cdecl longjmp(jmp_buf, int);
#endif

#if defined(__ZTC__) || (defined(MSC) && (_MSC_VER <= 600)) || defined(__TURBOC__) || defined(__GNUC__) || defined(hpux)
typedef jmp_buf  CATCHBUF;
typedef jmp_buf  LPCATCHBUF;
#else
typedef int CATCHBUF[9];
typedef int FAR* LPCATCHBUF;
#endif


#ifdef VAXC
#pragma nostandard
#endif

/*
  CW_USEDEFAULT
*/
#if defined(MEWEL_32BITS)
#define CW_USEDEFAULT	  (0x8000U)
#else
#define CW_USEDEFAULT	  ((int)0x8000)
#endif

/*
  For ExtTextOut
*/
#define ETO_GRAYED     0x0001
#define ETO_OPAQUE     0x0002
#define ETO_CLIPPED    0x0004

#define WF_PMODE	0x0001
#define WF_CPU286	0x0002
#define WF_CPU386	0x0004
#define WF_CPU486	0x0008
#define WF_STANDARD	0x0010
#define WF_WIN286	0x0010
#define WF_ENHANCED	0x0020
#define WF_WIN386	0x0020
#define WF_CPU086	0x0040
#define WF_CPU186	0x0080
#define WF_LARGEFRAME	0x0100
#define WF_SMALLFRAME	0x0200
#define WF_80x87	0x0400
#define WF_PAGING       0x0800
#define WF_WLO          0x8000

#define MAKEINTRESOURCE(i)  ((LPCSTR)MAKELP(0, (i)))

#ifdef VAXC
#pragma standard
#endif


/*
  File handling functions
*/
/* _lopen() flags */
#define READ        0
#define WRITE       1
#define READ_WRITE  2



#ifdef VAXC
#pragma nostandard
#endif

/*
  GDI functions
*/
typedef enum tagOBJECTTYPE
{
  OBJ_PEN=1, OBJ_BRUSH, OBJ_FONT, OBJ_RGN, OBJ_BMP, OBJ_PALETTE
} OBJECTTYPE;

/* Cursor IDs */
#define IDC_ARROW     MAKEINTRESOURCE(32512)
#define IDC_IBEAM     MAKEINTRESOURCE(32513)
#define IDC_WAIT      MAKEINTRESOURCE(32514)
#define IDC_CROSS	    MAKEINTRESOURCE(32515)
#define IDC_UPARROW   MAKEINTRESOURCE(32516)
#define IDC_SIZE      MAKEINTRESOURCE(32640)
#define IDC_ICON      MAKEINTRESOURCE(32641)
#define IDC_SIZENWSE  MAKEINTRESOURCE(32642)
#define IDC_SIZENESW  MAKEINTRESOURCE(32643)
#define IDC_SIZEWE    MAKEINTRESOURCE(32644)
#define IDC_SIZENS    MAKEINTRESOURCE(32645)

#ifdef VAXC
#pragma standard
#endif


/* Standard Icon IDs */
#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)

/*
  SetWindowPos() flags
*/
#define SWP_NOSIZE       0x01
#define SWP_NOMOVE       0x02
#define SWP_NOZORDER     0x04
#define SWP_NOREDRAW     0x08
#define SWP_NOACTIVATE   0x10
#define SWP_DRAWFRAME    0x20
#define SWP_FRAMECHANGED 0x20 
#define SWP_SHOWWINDOW   0x40
#define SWP_HIDEWINDOW   0x80
#define SWP_NOCOPYBITS   0x0100
#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */
#define SWP_NOREPOSITION    SWP_NOOWNERZORDER
#define SWP_NOSENDCHANGING  0x0400
#define SWP_DEFERERASE      0x2000

#define HWND_TOP            ((HWND)0)
#define HWND_BOTTOM         ((HWND)1)
#define HWND_TOPMOST        ((HWND)-1)
#define HWND_NOTOPMOST      ((HWND)-2)

/*
  WIN 3.1 window-placement functions and structure
*/
typedef struct tagWINDOWPLACEMENT
{
  UINT  length;
  UINT  flags;
  UINT  showCmd;
  POINT ptMinPosition;
  POINT ptMaxPosition;
  RECT  rcNormalPosition;
} WINDOWPLACEMENT, *PWINDOWPLACEMENT, FAR* LPWINDOWPLACEMENT;

#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002

BOOL WINAPI GetWindowPlacement(HWND,       WINDOWPLACEMENT FAR*);
BOOL WINAPI SetWindowPlacement(HWND, CONST WINDOWPLACEMENT FAR*);

/*===========================================================================*/
/*                                                                           */
/*                          MESSAGES                                         */
/*                                                                           */
/*===========================================================================*/
/* NOTE: All messages below 0x0400 are RESERVED by Windows */
#define WM_USER		     0x0400

#define WM_ACTIVATE	   0x0006
#define WA_INACTIVE    0
#define WA_ACTIVE      1
#define WA_CLICKACTIVE 2

#define WM_ACTIVATEAPP      0x001C
#define WM_ASKCBFORMATNAME  0x030C
#define WM_CANCELMODE       0x001F
#define WM_CHAR             0x0102
#define WM_CHARTOITEM       0x002F
#define WM_CHANGECBCHAIN    0x030D
#define WM_CHILDACTIVATE    0x0022
#define WM_CLEAR            0x0303
#define WM_CLOSE            0x0010
#define WM_COPY             0x0301
#define WM_COALESCE_LAST    0x039F
#define WM_COMPACTING       0x0041
#define WM_COMPAREITEM      0x0039
#define WM_COALESCE_FIRST   0x0390
#define WM_COMMNOTIFY       0x0044
#define WM_COMMAND          0x0111
#define WM_CREATE           0x0001

#define WM_CTLCOLOR         0x0019
#define CTLCOLOR_MSGBOX      0
#define CTLCOLOR_EDIT        1
#define CTLCOLOR_LISTBOX     2
#define CTLCOLOR_BTN         3
#define CTLCOLOR_DLG         4
#define CTLCOLOR_SCROLLBAR   5
#define CTLCOLOR_STATIC      6

#define WM_CUT              0x0300
#define WM_DESTROYCLIPBOARD 0x0307
#define WM_DESTROY          0x0002
#define WM_DEVMODECHANGE    0x001B
#define WM_DELETEITEM       0x002D
#define WM_DEADCHAR         0x0103
#define WM_DRAWCLIPBOARD    0x0308
#define WM_DRAWITEM         0x002B
#define WM_DROPFILES        0x0233
#define WM_ENDSESSION       0x0016
#define WM_ENABLE           0x000A
#define WM_ENTERIDLE        0x0121
#define WM_ERASEBKGND       0x0014
#define WM_FONTCHANGE       0x001D
#define WM_GETMINMAXINFO    0x0024
#define WM_GETDLGCODE       0x0087
#define WM_GETFONT          0x0031
#define WM_GETTEXTLENGTH    0x000E
#define WM_GETTEXT          0x000D
#define WM_HSCROLL          0x0114
#define WM_HSCROLLCLIPBOARD 0x030E
#define WM_ICONERASEBKGND   0x0027
#define WM_INITMENUPOPUP    0x0117
#define WM_INITDIALOG       0x0110
#define WM_INITMENU         0x0116

#define WM_KEYLAST          0x0108
#define WM_KEYUP            0x0101
#define WM_KEYFIRST         0x0100
#define WM_KEYDOWN          0x0100
/* WM_KEYUP/DOWN/CHAR HIWORD(lParam) flags */
#define KF_EXTENDED        0x0100
#define KF_DLGMODE         0x0800
#define KF_MENUMODE        0x1000
#define KF_ALTDOWN         0x2000
#define KF_REPEAT          0x4000
#define KF_UP              0x8000


#define WM_KILLFOCUS        0x0008
#define WM_MDIRESTORE       0x0223
#define WM_MDICREATE        0x0220
#define WM_MDISETMENU       0x0230
#define WM_MDIGETACTIVE     0x0229
#define WM_MDICASCADE       0x0227
#define WM_MDITILE          0x0226
#define WM_MDIACTIVATE      0x0222
#define WM_MDIDESTROY       0x0221
#define WM_MDIICONARRANGE   0x0228
#define WM_MDINEXT          0x0224
#define WM_MDIMAXIMIZE      0x0225
#define WM_MEASUREITEM      0x002C
#define WM_MENUCHAR         0x0120
#define WM_MENUSELECT       0x011F
#define WM_MOVE             0x0003
#define WM_MOUSEMOVE        0x0200

#define WM_MOUSEACTIVATE    0x0021
/* WM_MOUSEACTIVATE return codes */
#define MA_ACTIVATE          1
#define MA_ACTIVATEANDEAT    2
#define MA_NOACTIVATE        3
#define MA_NOACTIVATEANDEAT  4

#define WM_NCPAINT          0x0085
#define WM_NCDESTROY        0x0082
#define WM_NCCREATE         0x0081
#define WM_NCCALCSIZE       0x0083
#define WM_NCACTIVATE       0x0086

#define WM_NCHITTEST        0x0084
/* WM_NCHITTEST return codes */
#define HTERROR              (-2)
#define HTTRANSPARENT        (-1)
#define HTNOWHERE            0
#define HTCLIENT             1
#define HTCAPTION            2
#define HTSYSMENU            3
#define HTSIZE               4
#define HTMENU               5
#define HTHSCROLL            6
#define HTVSCROLL            7
#define HTMINBUTTON          8
#define HTMAXBUTTON          9
#define HTLEFT               10
#define HTRIGHT              11
#define HTTOP                12
#define HTTOPLEFT            13
#define HTTOPRIGHT           14
#define HTBOTTOM             15
#define HTBOTTOMLEFT         16
#define HTBOTTOMRIGHT        17
#define HTBORDER             18
#define HTGROWBOX            HTSIZE
#define HTREDUCE             HTMINBUTTON
#define HTZOOM               HTMAXBUTTON

#define WM_NEXTDLGCTL        0x0028
#define WM_NULL              0x0000
#define WM_PALETTEISCHANGING 0x0310
#define WM_PAINT             0x000F
#define WM_PAINTICON         0x0153
#define WM_PASTE             0x0302
#define WM_PALETTECHANGED    0x0311
#define WM_PARENTNOTIFY      0x0210
#define WM_PAINTCLIPBOARD    0x0309
#define WM_PENWINFIRST       0x0380
#define WM_PENWINLAST        0x038F
#define WM_POWER             0x0048
#define WM_QUERYENDSESSION   0x0011
#define WM_QUERYNEWPALETTE   0x030F
#define WM_QUIT              0x0012
#define WM_QUERYOPEN         0x0013
#define WM_QUEUESYNC         0x0023
#define WM_QUERYDRAGICON     0x0037
#define WM_RENDERALLFORMATS  0x0306
#define WM_RENDERFORMAT      0x0305
#define WM_SETTEXT           0x000C
#define WM_SETFONT           0x0030
#define WM_SETCURSOR         0x0020
#define WM_SETREDRAW         0x000B
#define WM_SETFOCUS          0x0007
#define WM_SHOWWINDOW        0x0018

#define WM_SIZE              0x0005
/* WM_SIZE message wParam values */
#define SIZE_RESTORED         0
#define SIZE_MINIMIZED        1
#define SIZE_MAXIMIZED        2
#define SIZE_MAXSHOW          3
#define SIZE_MAXHIDE          4
/* Obsolete constant names */
#define SIZENORMAL            SIZE_RESTORED
#define SIZEICONIC            SIZE_MINIMIZED
#define SIZEFULLSCREEN        SIZE_MAXIMIZED
#define SIZEZOOMSHOW          SIZE_MAXSHOW
#define SIZEZOOMHIDE          SIZE_MAXHIDE

#define WM_SIZECLIPBOARD     0x030B
#define WM_SPOOLERSTATUS     0x002A
#define WM_SYSTEMERROR       0x0017
#define WM_SYSDEADCHAR       0x0107
#define WM_SYSKEYUP          0x0105
#define WM_SYSCOMMAND        0x0112
#define WM_SYSKEYDOWN        0x0104
#define WM_SYSCOLORCHANGE    0x0015
#define WM_SYSCHAR           0x0106
#define WM_TIMER             0x0113
#define WM_TIMECHANGE        0x001E
#define WM_UNDO              0x0304
#define WM_VKEYTOITEM        0x002E
#define WM_VSCROLL           0x0115
#define WM_VSCROLLCLIPBOARD  0x030A
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_WININICHANGE      0x001A
#define WM_WINDOWPOSCHANGING 0x0046

/*
  Mouse messages
*/
#define WM_MOUSEFIRST        0x01FF
#define WM_MOUSEREPEAT       0x01FF  /* MEWEL-specific */
#define WM_MOUSEMOVE         0x0200
#define WM_LBUTTONDOWN       0x0201
#define WM_LBUTTONUP         0x0202
#define WM_LBUTTONDBLCLK     0x0203
#define WM_RBUTTONDOWN       0x0204
#define WM_RBUTTONUP         0x0205
#define WM_RBUTTONDBLCLK     0x0206
#define WM_MBUTTONDOWN       0x0207
#define WM_MBUTTONUP         0x0208
#define WM_MBUTTONDBLCLK     0x0209
#define WM_MOUSELAST         0x0209

#define WM_NCMOUSEMOVE       0x00A0
#define WM_NCLBUTTONDOWN     0x00A1
#define WM_NCLBUTTONUP       0x00A2
#define WM_NCLBUTTONDBLCLK   0x00A3
#define WM_NCRBUTTONDOWN     0x00A4
#define WM_NCRBUTTONUP       0x00A5
#define WM_NCRBUTTONDBLCLK   0x00A6
#define WM_NCMBUTTONDOWN     0x00A7
#define WM_NCMBUTTONUP       0x00A8
#define WM_NCMBUTTONDBLCLK   0x00A9

#define WM_MOUSE_TO_NCMOUSE(msg) ((msg - WM_MOUSEMOVE) + WM_NCMOUSEMOVE)
#define WM_NCMOUSE_TO_MOUSE(msg) ((msg - WM_NCMOUSEMOVE) + WM_MOUSEMOVE)
#define IS_WM_NCMOUSE_MSG(msg)   ((msg)>=WM_NCMOUSEMOVE && (msg)<=WM_NCMBUTTONDBLCLK)
#define IS_WM_MOUSE_MSG(msg)     ((msg)>=WM_MOUSEFIRST && (msg)<=WM_MOUSELAST)
#define IS_MOUSE_MSG(msg)        (IS_WM_NCMOUSE_MSG(msg) || IS_WM_MOUSE_MSG(msg))


/* 
  MEWEL-specific messages
*/
#define WM_ALT              0x00F0
#define WM_BORDER           0x00F1
#define WM_GETID            0x00F2
#define WM_GETFLAGS         0x00F3
#define WM_HELP             0x00F4
#define WM_REMOVEDMSG       0xFFFF /* what _PeekMsg() turns removed msg into */
#define WM_SETFLAGS         0x00F5
#define WM_SETID            0x00F6
#define WM_SHADOW           0x00F7
#define WM_VALIDATE         0x00F8
/*
  Windows incompatible data validation messages
*/
#define FN_ERRVALIDATE      0x00F9
#define   VR_NEXTDLGCTRL     ((WORD) -1)
#define   VR_LBUTTONDOWN     ((WORD) -2)
#define   VR_RBUTTONDOWN     ((WORD) -3)
#define WM_SYNCCURSOR       0x00FA


/* Button Control Messages  */
#define BM_GETCHECK      (WM_USER+0)
#define BM_SETCHECK      (WM_USER+1)
#define BM_GETSTATE      (WM_USER+2)
#define BM_SETSTATE      (WM_USER+3)
#define BM_SETSTYLE      (WM_USER+4)

/* User Button Notification Codes */
#define BN_CLICKED       0
#define BN_PAINT         1
#define BN_HILITE        2
#define BN_UNHILITE      3
#define BN_DISABLE       4
#define BN_DOUBLECLICKED 5

#define EM_GETSEL        (WM_USER+0)
#define EM_SETSEL        (WM_USER+1)
#define EM_GETRECT       (WM_USER+2)
#define EM_SETRECT       (WM_USER+3)
#define EM_SETRECTNP     (WM_USER+4)
#define EM_LINESCROLL    (WM_USER+6)
#define EM_GETMODIFY     (WM_USER+8)
#define EM_SETMODIFY     (WM_USER+9)
#define EM_GETLINECOUNT  (WM_USER+10)
#define EM_LINEINDEX     (WM_USER+11)
#define EM_SETHANDLE     (WM_USER+12)
#define EM_GETHANDLE     (WM_USER+13)
#define EM_LINELENGTH    (WM_USER+17)
#define EM_REPLACESEL    (WM_USER+18)
#define EM_SETFONT       (WM_USER+19)    /* NOT IMPLEMENTED: use WM_SETFONT */
#define EM_GETLINE       (WM_USER+20)
#define EM_LIMITTEXT     (WM_USER+21)
#define EM_CANUNDO       (WM_USER+22)
#define EM_UNDO          (WM_USER+23)
#define EM_FMTLINES      (WM_USER+24)
#define EM_LINEFROMCHAR  (WM_USER+25)
#define EM_SETWORDBREAK  (WM_USER+26)    /* NOT IMPLEMENTED: use EM_SETWORDBREAK */
#define EM_SETTABSTOPS   (WM_USER+27)
#define EM_SETPASSWORDCHAR      (WM_USER+28)
#define EM_EMPTYUNDOBUFFER      (WM_USER+29)
#if (WINVER >= 0x030a)
#define EM_GETFIRSTVISIBLELINE  (WM_USER+30)
#define EM_SETREADONLY          (WM_USER+31)
#define EM_SETWORDBREAKPROC     (WM_USER+32)
#define EM_GETWORDBREAKPROC     (WM_USER+33)
#define EM_GETPASSWORDCHAR      (WM_USER+34)
#endif  /* WINVER >= 0x030a */
#define EM_GETSTATE             (WM_USER+35)  /* MEWEL-specific */
#define EM_SETSTATE             (WM_USER+36)  /* MEWEL-specific */

#if (WINVER >= 0x030a)
typedef int (CALLBACK* EDITWORDBREAKPROC)(LPSTR lpch, int ichCurrent, int cch, int code);
/* EDITWORDBREAKPROC code values */
#define WB_LEFT		      0
#define WB_RIGHT	      1
#define WB_ISDELIMITER  2
#endif  /* WINVER >= 0x030a */



/* Edit control notification codes */
#define EN_SETFOCUS         0x0100
#define EN_KILLFOCUS        0x0200
#define EN_CHANGE           0x0300
#define EN_UPDATE           0x0400
#define EN_ERRSPACE         0x0500
#define EN_MAXTEXT          0x0501
#define EN_HSCROLL          0x0601
#define EN_VSCROLL          0x0602

/* Listbox messages */
#define LB_ADDSTRING        (WM_USER+1)
#define LB_INSERTSTRING     (WM_USER+2)
#define LB_DELETESTRING     (WM_USER+3)
#define LB_RESETCONTENT     (WM_USER+5)
#define LB_SETSEL           (WM_USER+6)
#define LB_SETCURSEL        (WM_USER+7)
#define LB_GETSEL           (WM_USER+8)
#define LB_GETCURSEL        (WM_USER+9)
#define LB_GETTEXT          (WM_USER+10)
#define LB_GETTEXTLEN       (WM_USER+11)
#define LB_GETCOUNT         (WM_USER+12)
#define LB_SELECTSTRING     (WM_USER+13)
#define LB_DIR              (WM_USER+14)
#define LB_GETTOPINDEX      (WM_USER+15)
#define LB_FINDSTRING       (WM_USER+16)
#define LB_GETSELCOUNT      (WM_USER+17)
#define LB_GETSELITEMS      (WM_USER+18)
#define LB_SETTABSTOPS      (WM_USER+19)
#define LB_GETHORIZONTALEXTENT (WM_USER+20)
#define LB_SETHORIZONTALEXTENT (WM_USER+21)
#define LB_SETCOLUMNWIDTH      (WM_USER+22)
#define LB_SETTOPINDEX         (WM_USER+24)
#define LB_GETITEMRECT         (WM_USER+25)
#define LB_GETITEMDATA         (WM_USER+26)
#define LB_SETITEMDATA         (WM_USER+27)
#define LB_SELITEMRANGE        (WM_USER+28)
#define LB_SETCARETINDEX       (WM_USER+31)
#define LB_GETCARETINDEX       (WM_USER+32)
#if (WINVER >= 0x030a)
#define LB_SETITEMHEIGHT       (WM_USER+33)
#define LB_GETITEMHEIGHT       (WM_USER+34)
#define LB_FINDSTRINGEXACT     (WM_USER+35)
#endif  /* WINVER >= 0x030a */

/* Listbox notification codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5

/* Combo box messages */
#define CB_GETEDITSEL       (WM_USER+0)
#define CB_LIMITTEXT        (WM_USER+1)
#define CB_SETEDITSEL       (WM_USER+2)
#define CB_ADDSTRING        (WM_USER+3)
#define CB_DELETESTRING     (WM_USER+4)
#define CB_DIR              (WM_USER+5)
#define CB_GETCOUNT         (WM_USER+6)
#define CB_GETCURSEL        (WM_USER+7)
#define CB_GETLBTEXT        (WM_USER+8)
#define CB_GETLBTEXTLEN     (WM_USER+9)
#define CB_INSERTSTRING     (WM_USER+10)
#define CB_RESETCONTENT     (WM_USER+11)
#define CB_FINDSTRING       (WM_USER+12)
#define CB_SELECTSTRING     (WM_USER+13)
#define CB_SETCURSEL        (WM_USER+14)
#define CB_SHOWDROPDOWN     (WM_USER+15)
#define CB_GETITEMDATA      (WM_USER+16)
#define CB_SETITEMDATA      (WM_USER+17)
#if (WINVER >= 0x030a)
#define CB_GETDROPPEDCONTROLRECT (WM_USER+18)
#define CB_SETITEMHEIGHT         (WM_USER+19)
#define CB_GETITEMHEIGHT         (WM_USER+20)
#define CB_SETEXTENDEDUI         (WM_USER+21)
#define CB_GETEXTENDEDUI         (WM_USER+22)
#define CB_GETDROPPEDSTATE       (WM_USER+23)
#define CB_FINDSTRINGEXACT       (WM_USER+24)
#endif  /* WINVER >= 0x030a */

/* Combo box notification codes */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#if (WINVER >= 0x030a)
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10
#endif  /* WINVER >= 0x030a */

/* Static control messages */
#define STM_SETICON         (WM_USER+0)
#define STM_GETICON         (WM_USER+1)


/* Dialog Codes for WM_GETDLGCODE */
#define DLGC_WANTARROWS     0x0001    /* Control wants arrow keys         */
#define DLGC_WANTTAB        0x0002    /* Control wants tab keys           */
#define DLGC_WANTALLKEYS    0x0004    /* Control wants all keys           */
#define DLGC_WANTMESSAGE    0x0004    /* Pass message to control          */
#define DLGC_HASSETSEL      0x0008    /* Understands EM_SETSEL message    */
#define DLGC_DEFPUSHBUTTON  0x0010    /* Default pushbutton               */
#define DLGC_UNDEFPUSHBUTTON 0x0020   /* Non-default pushbutton           */
#define DLGC_RADIOBUTTON    0x0040    /* Radio button                     */
#define DLGC_WANTCHARS      0x0080    /* Want WM_CHAR messages            */
#define DLGC_STATIC         0x0100    /* Static item: don't include       */
#define DLGC_BUTTON         0x2000    /* Button item: can be checked      */


/* Dialog messages */
#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)
/* Returned in HIWORD() of DM_GETDEFID result if msg is supported */
#define DC_HASDEFID         0x534B

/* DlgDirList, DlgDirListComboBox flags values */
#define DDL_READWRITE       0x0000
#define DDL_READONLY        0x0001
#define DDL_HIDDEN          0x0002
#define DDL_SYSTEM          0x0004
#define DDL_DIRECTORY       0x0010
#define DDL_ARCHIVE         0x0020

#define DDL_POSTMSGS        0x2000
#define DDL_DRIVES          0x4000
#define DDL_EXCLUSIVE       0x8000


/*===========================================================================*/
/*                                                                           */
/*                RESOURCE FILE INFORMATION                                  */
/*                                                                           */
/*===========================================================================*/

#ifdef OS2
#undef RT_POINTER
#undef RT_BITMAP
#undef RT_MENU
#undef RT_DIALOG
#undef RT_STRING
#undef RT_ACCELTABLE
#undef RT_RCDATA
#undef RT_MESSAGE
#undef RT_DLGINCLUDE
#undef RT_VKEYTBL
#undef RT_FONT
#undef RT_FONTDIR
#endif

#define RT_CURSOR           MAKEINTRESOURCE(1)
#define RT_BITMAP           MAKEINTRESOURCE(2)
#define RT_ICON             MAKEINTRESOURCE(3)
#define RT_MENU             MAKEINTRESOURCE(4)
#define RT_DIALOG           MAKEINTRESOURCE(5)
#define RT_STRING           MAKEINTRESOURCE(6)
#define RT_FONTDIR          MAKEINTRESOURCE(7)
#define RT_FONT             MAKEINTRESOURCE(8)
#define RT_ACCELERATOR      MAKEINTRESOURCE(9)
#define RT_RCDATA           MAKEINTRESOURCE(10)
#define RT_GROUP_CURSOR     MAKEINTRESOURCE(12)
#define RT_GROUP_ICON       MAKEINTRESOURCE(14)


/*===========================================================================*/
/*                                                                           */
/*                SYSTEM COLOR INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/

extern BOOL bUseSysColors;  /* TRUE if using system colors */

#define SYSTEM_COLOR              ((COLOR)0xFF) /* specified in CreateWindow() */

#ifndef NOCOLOR

#define SYSCLR_FIRST              0
#define SYSCLR_SCROLLBAR          0
#define COLOR_SCROLLBAR           SYSCLR_SCROLLBAR
#define SYSCLR_BACKGROUND         1
#define COLOR_BACKGROUND          SYSCLR_BACKGROUND
#define SYSCLR_ACTIVECAPTION      2
#define COLOR_ACTIVECAPTION       SYSCLR_ACTIVECAPTION
#define SYSCLR_INACTIVECAPTION    3
#define COLOR_INACTIVECAPTION     SYSCLR_INACTIVECAPTION
#define SYSCLR_MENU               4
#define COLOR_MENU                SYSCLR_MENU
#define SYSCLR_WINDOW             5
#define COLOR_WINDOW              SYSCLR_WINDOW
#define SYSCLR_WINDOWFRAME        6
#define COLOR_WINDOWFRAME         SYSCLR_WINDOWFRAME
#define SYSCLR_MENUTEXT           7
#define COLOR_MENUTEXT            SYSCLR_MENUTEXT
#define SYSCLR_WINDOWTEXT         8
#define COLOR_WINDOWTEXT          SYSCLR_WINDOWTEXT
#define SYSCLR_CAPTIONTEXT        9
#define COLOR_CAPTIONTEXT         SYSCLR_CAPTIONTEXT
#define SYSCLR_ACTIVEBORDER       10
#define COLOR_ACTIVEBORDER        SYSCLR_ACTIVEBORDER
#define SYSCLR_INACTIVEBORDER     11
#define COLOR_INACTIVEBORDER      SYSCLR_INACTIVEBORDER
#define SYSCLR_MDICLIENT          12
#define COLOR_APPWORKSPACE        SYSCLR_MDICLIENT
#define SYSCLR_MENUHILITESEL      13
#define COLOR_HIGHLIGHT           SYSCLR_MENUHILITESEL
#define SYSCLR_MENUHILITETEXT     14
#define COLOR_HIGHLIGHTTEXT       SYSCLR_MENUHILITETEXT
#define SYSCLR_BTNFACE            15
#define SYSCLR_BUTTON             SYSCLR_BTNFACE
#define COLOR_BTNFACE             SYSCLR_BTNFACE
#define SYSCLR_BTNSHADOW          16
#define COLOR_BTNSHADOW           SYSCLR_BTNSHADOW
#define SYSCLR_DISABLEDCHECKBOX   17
#define COLOR_GRAYTEXT            SYSCLR_DISABLEDCHECKBOX
#define SYSCLR_BTNTEXT            18
#define COLOR_BTNTEXT             SYSCLR_BTNTEXT
#define SYSCLR_INACTIVECAPTIONTEXT 19
#define COLOR_INACTIVECAPTIONTEXT SYSCLR_INACTIVECAPTIONTEXT
#define SYSCLR_BUTTONHIGHLIGHT    20
#define COLOR_BTNHIGHLIGHT        SYSCLR_BUTTONHIGHLIGHT

/*
  MEWEL-specific extensions
*/
#define SYSCLR_SHADOW             21
#define SYSCLR_MENUGRAYEDTEXT     22
#define SYSCLR_WINDOWSTATICTEXT   23
#define SYSCLR_SCROLLBARARROWS    24
#define SYSCLR_SCROLLBARTHUMB     25
#define SYSCLR_HELPTEXT           26
#define SYSCLR_HELPHILITE         27
#define SYSCLR_HELPBACKGROUND     28
#define SYSCLR_MESSAGEBOX         29
#define SYSCLR_DLGBOX             30
#define SYSCLR_DLGACCEL           31
#define SYSCLR_BUTTONDOWN         32
#define SYSCLR_CHECKBOX           33
#define SYSCLR_LISTBOX            34
#define SYSCLR_EDIT               35
#define SYSCLR_EDITSELECTION      36
#define SYSCLR_SYSMENU            37
#define SYSCLR_DESKTOP            38
#define SYSCLR_DISABLEDBUTTON     39
#define SYSCLR_DISABLEDEDIT       40
#define SYSCLR_WINDOWPREFIXHIGHLIGHT      41
#define SYSCLR_DISABLEDSCROLLBAR          42
#define SYSCLR_DISABLEDSCROLLBARARROWS    43
#define SYSCLR_DISABLEDSCROLLBARTHUMB     44
#define SYSCLR_DISABLEDLISTBOX            45
#define SYSCLR_DISABLEDBORDER             46
#define SYSCLR_MENUGRAYEDHILITESEL        47
#define SYSCLR_HOTKEY                     48

/*
  These additions are by Mike Locke @ CompuServe
*/
#define SYSCLR_MODALCAPTION               49
#define SYSCLR_MODALBORDER                50
#define SYSCLR_PUSHBUTTON                 51
#define SYSCLR_PUSHBUTTONDOWN             52
#define SYSCLR_PUSHBUTTONDISABLED         53
#define SYSCLR_PUSHBUTTONHILITE           54
#define SYSCLR_TITLE                      55
#define SYSCLR_LAST                       SYSCLR_TITLE

#endif  /* NOCOLOR */

/*
  Courtesy of Mark Kalish @ Lan Systems
*/
#ifndef RGB
#define RGB(r,g,b)          ((COLORREF)((((BYTE)(r)|((WORD)(g)<<8)))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(rgb)      ((BYTE) ((rgb) & 0x00FF))
#define GetGValue(rgb)      ((BYTE) ((((WORD)(rgb)) >> 8) & 0x00FF))
#define GetBValue(rgb)      ((BYTE) (((rgb)>>16) & 0x00FF))
#define PALETTERGB(r,g,b)   (0x02000000L | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000L | (DWORD)(WORD)(i)))

#endif


/*===========================================================================*/
/*                                                                           */
/*                SYSTEM CHARACTER INFORMATION                               */
/*                                                                           */
/*===========================================================================*/

#define SYSCHAR_WINDOW_BACKGROUND         0
#define SYSCHAR_ACTIVE_CAPTION            1
#define SYSCHAR_INACTIVE_CAPTION          2
#define SYSCHAR_SYSMENU                   3
#define SYSCHAR_MINICON                   4
#define SYSCHAR_MAXICON                   5
#define SYSCHAR_EDIT_BACKGROUND           6
#define SYSCHAR_EDIT_LBORDER              7
#define SYSCHAR_EDIT_RBORDER              8
#define SYSCHAR_COMBO_ICON                9
#define SYSCHAR_RADIOBUTTONCHECK          10
#define SYSCHAR_RADIOBUTTON_LBORDER       11
#define SYSCHAR_RADIOBUTTON_RBORDER       12
#define SYSCHAR_CHECKBOX_LBORDER          13
#define SYSCHAR_CHECKBOX_RBORDER          14
#define SYSCHAR_PUSHBUTTON_LBORDER        15
#define SYSCHAR_PUSHBUTTON_RBORDER        16
#define SYSCHAR_DEFPUSHBUTTON_LBORDER     17
#define SYSCHAR_DEFPUSHBUTTON_RBORDER     18
#define SYSCHAR_SCROLLBAR_THUMB           19
#define SYSCHAR_SCROLLBAR_LEFT            20
#define SYSCHAR_SCROLLBAR_RIGHT           21
#define SYSCHAR_SCROLLBAR_UP              22
#define SYSCHAR_SCROLLBAR_DOWN            23
#define SYSCHAR_SCROLLBAR_FILL            24
#define SYSCHAR_SUBMENU                   25
#define SYSCHAR_MENUCHECK                 26
#define SYSCHAR_SHADOW                    27
#define SYSCHAR_HILITEPREFIX              28
#define SYSCHAR_CHECKBOXCHECK             29
#define SYSCHAR_RESTOREICON               30
#define SYSCHAR_MULTISELFOCUS             31
#define SYSCHAR_3STATECHECK               32
#define SYSCHAR_MDISYSMENU                33
#define SYSCHAR_RADIOBUTTON_OFF           34
#define SYSCHAR_CHECKBOX_OFF              35
#define SYSCHAR_CHECKBOX_LBORDER_ON       36
#define SYSCHAR_CHECKBOX_RBORDER_ON       37
#define SYSCHAR_BUTTONBOTTOMSHADOW        38
#define SYSCHAR_BUTTONRIGHTSHADOW         39
#define SYSCHAR_MULTISELFOCUS_RIGHT       40
#define SYSCHAR_DESKTOP_BACKGROUND        41
extern BYTE FAR chSysChars[];
#define WinGetSysChar(idx)                (chSysChars[idx])
#define WinSetSysChar(idx, ch)            (chSysChars[idx] = ch)

#if defined(MULTICHAR_SYSICONS)
#define SYSCHARSTR_SYSMENU                0
#define SYSCHARSTR_MINICON                1
#define SYSCHARSTR_MAXICON                2
#define SYSCHARSTR_RESTOREICON            3
extern PSTR pszSysCharStrs[];
#define WinGetSysCharStr(idx)             (pszSysCharStrs[idx])
#define WinSetSysCharStr(idx, s)          (pszSysCharStrs[idx] = s)
#endif

/*===========================================================================*/
/*                                                                           */
/*                       DrawText() Information                              */
/*                                                                           */
/*===========================================================================*/
#define DT_TOP        0x0000
#define DT_LEFT       0x0000
#define DT_CENTER     0x0001
#define DT_RIGHT      0x0002
#define DT_VCENTER    0x0004
#define DT_BOTTOM     0x0008
#define DT_WORDBREAK  0x0010
#define DT_SINGLELINE 0x0020
#define DT_EXPANDTABS 0x0040
#define DT_TABSTOP    0x0080
#define DT_NOCLIP     0x0100
#define DT_EXTERNALLEADING  0x0200
#define DT_CALCRECT   0x0400
#define DT_NOPREFIX   0x0800
#define DT_INTERNAL   0x1000


/*===========================================================================*/
/*                                                                           */
/*                       EVENT INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/

#define MAXEVENTS    32        /* Length of the message queue */

/*
  The message structure
*/
typedef struct tagMSG
{
  HWND   hwnd;                /* the window that the message is meant for */
  UINT   message;             /* the message code */
  WPARAM wParam;              /* word-length argument 1 */
  LPARAM lParam;              /* double-word length arg 2 */
  DWORD  time;                /* time that the msg was generated */
  POINT  pt;                  /* mouse point when msg was generated */
} MSG, *LPMSG;


/* flags for PeekMessage() */
#define PM_NOREMOVE  0
#define PM_REMOVE    1
#define PM_NOYIELD   2


/*
  GetQueueStatus
*/
DWORD WINAPI GetQueueStatus(UINT);
#define QS_KEY          0x0001
#define QS_MOUSEMOVE    0x0002
#define QS_MOUSEBUTTON  0x0004
#define QS_MOUSE        (QS_MOUSEMOVE | QS_MOUSEBUTTON)
#define QS_POSTMESSAGE  0x0008
#define QS_TIMER        0x0010
#define QS_PAINT        0x0020
#define QS_SENDMESSAGE  0x0040
#define QS_ALLINPUT     0x007f


/*===========================================================================*/
/*                                                                           */
/*                       DLGBOX INFORMATION                                  */
/*                                                                           */
/*===========================================================================*/

#define DLGWINDOWEXTRA   16
#define WC_DIALOG        (MAKEINTATOM(0x8002))


/*===========================================================================*/
/*                                                                           */
/*                       EDIT FIELD INFORMATION                              */
/*                                                                           */
/*===========================================================================*/

extern UINT EditBufferSize;  /* = 4096 */

/*
  Status bits returned by EM_GETSTATE
*/
#define STATE_INSERT              0x01


/*===========================================================================*/
/*                                                                           */
/*                       LISTBOX INFORMATION                                 */
/*                                                                           */
/*===========================================================================*/

#define ATEND        ((UINT) -1)
#define ATSTART      (0)

#define LB_OKAY      (0)
#define LB_ERR       (-1)
#define LB_ERRSPACE  (-2)

/*===========================================================================*/
/*                                                                           */
/*                       COMBOBOX INFORMATION                                */
/*                                                                           */
/*===========================================================================*/

/* Combo Box return Values */
#define CB_OKAY      0
#define CB_ERR       (-1)
#define CB_ERRSPACE  (-2)

/*===========================================================================*/
/*                                                                           */
/*                       SCROLLBAR INFORMATION                               */
/*                                                                           */
/*===========================================================================*/

/* ScrollBar Messages */
#define SB_LINEUP      0
#define SB_LINELEFT    0
#define SB_LINEDOWN    1
#define SB_LINERIGHT   1
#define SB_PAGEUP      2
#define SB_PAGELEFT    2
#define SB_PAGEDOWN    3
#define SB_PAGERIGHT   3
#define SB_THUMBPOSITION  4
#define SB_THUMBTRACK  5
#define SB_TOP         6
#define SB_LEFT        6
#define SB_BOTTOM      7
#define SB_RIGHT       7
#define SB_ENDSCROLL   8



/*===========================================================================*/
/*                                                                           */
/*                       MENU  INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/

/* Menu template header */
typedef struct
{
  UINT versionNumber;
  UINT offset;
} MENUITEMTEMPLATEHEADER;

/* Menu template item struct */
typedef struct
{
  UINT mtOption;
  UINT mtID;
  char mtString[1];
} MENUITEMTEMPLATE;


/* ChangeMenu flags */
#define MF_INSERT         0x0000
#define MF_CHANGE         0x0080
#define MF_APPEND         0x0100
#define MF_DELETE         0x0200
#define MF_REMOVE         0x1000

/* Menu flags for Add/Check/EnableMenuItem() */
#define MF_BYCOMMAND      0x0000
#define MF_BYPOSITION     0x0400

#define MF_SEPARATOR      0x0800

#define MF_ENABLED        0x0000
#define MF_GRAYED         0x0001
#define MF_DISABLED       0x0002

#define MF_UNCHECKED      0x0000
#define MF_CHECKED        0x0008
#define MF_USECHECKBITMAPS 0x0200

#define MF_STRING         0x0000
#define MF_BITMAP         0x0004
#define MF_OWNERDRAW      0x0100

#define MF_POPUP          0x0010
#define MF_MENUBARBREAK   0x0020
#define MF_MENUBREAK      0x0040

#define MF_UNHILITE       0x0000
#define MF_HILITE         0x0080

#define MF_SYSMENU        0x2000
#define MF_HELP           0x4000
#define MF_MOUSESELECT    0x8000

#define MF_END            0x0080  /* Only valid in menu resource templates */

/*
  MEWEL-specific flags
*/
#define MF_SHADOW         0x0020  /* same as MF_MENUBARBREAK */
#define MF_RIGHTJUST      0x8000  /* same as MF_MOUSESELECT  */

/*
  Macro to change a menubar to a popup
*/
#define CHANGE_TO_POPUP(hPop)  ChangeMenu(hPop,(UINT)-1,NULL,hPop,MF_POPUP|MF_CHANGE)

/*
  Flags for TrackPopupMenu
*/
#define TPM_LEFTBUTTON  0x0000
#define TPM_RIGHTBUTTON 0x0002
#define TPM_LEFTALIGN   0x0000
#define TPM_CENTERALIGN 0x0004
#define TPM_RIGHTALIGN  0x0008

/*===========================================================================*/
/*                                                                           */
/*                         CLIPBOARD INFORMATION                             */
/*                                                                           */
/*===========================================================================*/
#define CF_TEXT         1    /* The only fmt supported by MEWEL */
#define CF_BITMAP       2
#define CF_METAFILEPICT 3
#define CF_SYLK         4
#define CF_DIF          5
#define CF_TIFF         6
#define CF_OEMTEXT      7
#define CF_DIB          8
#define CF_PALETTE      9
#define CF_OWNERDISPLAY 10

/*===========================================================================*/
/*                                                                           */
/* File    :  WINMDI.H                                                       */
/*                                                                           */
/* Purpose :  Information for the Windows 3.0 compatible MDI interface       */
/*                                                                           */
/*===========================================================================*/
#define MDIS_ALLCHILDSTYLES 0x0001

#define WM_MDICREATE        0x0220
#define WM_MDIDESTROY       0x0221
#define WM_MDIACTIVATE      0x0222
#define WM_MDIRESTORE       0x0223
#define WM_MDINEXT          0x0224
#define WM_MDIMAXIMIZE      0x0225
#define WM_MDITILE          0x0226
#define WM_MDICASCADE       0x0227
#define AWP_CASCADED          0
#define AWP_TILED             1
#define WM_MDIICONARRANGE   0x0228
#define WM_MDIGETACTIVE     0x0229
#define WM_MDISETMENU       0x0230

typedef struct tagMDICREATESTRUCT
{
  LPCSTR  szClass;
  LPCSTR  szTitle;
  HANDLE hOwner;
  int    x, y;
  int    cx, cy;
  LONG   style;
  LONG   lParam;  /* app-defined stuff */
} MDICREATESTRUCT, FAR *LPMDICREATESTRUCT;
/* wParam values for WM_MDITILE and WM_MDICASCADE messages. */
#define MDITILE_VERTICAL      0x0000
#define MDITILE_HORIZONTAL    0x0001
#define MDITILE_SKIPDISABLED  0x0002

typedef struct tagCLIENTCREATESTRUCT
{
  HANDLE hWindowMenu;
  UINT   idFirstChild;
} CLIENTCREATESTRUCT, FAR *LPCLIENTCREATESTRUCT;
#define MDIS_ALLCHILDSTYLES 0x0001


/*===========================================================================*/
/*                                                                           */
/*                   ATOM & PROPERTY LIST INFORMATION                        */
/*                                                                           */
/*===========================================================================*/

#define MAKEINTATOM(i)      ((LPCSTR)MAKELP(0, (i)))
typedef UINT  ATOM;

/*===========================================================================*/
/*                                                                           */
/*                       TEXTMETRIC  INFORMATION                             */
/*                                                                           */
/*===========================================================================*/

typedef struct tagTEXTMETRIC
{
  int tmHeight;
  int tmAscent;
  int tmDescent;
  int tmInternalLeading;
  int tmExternalLeading;
  int tmAveCharWidth;
  int tmMaxCharWidth;
  int tmWeight;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmFirstChar;
  BYTE tmLastChar;
  BYTE tmDefaultChar;
  BYTE tmBreakChar;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
  int tmOverhang;
  int tmDigitizedAspectX;
  int tmDigitizedAspectY;
} TEXTMETRIC;
typedef TEXTMETRIC *PTEXTMETRIC;
typedef TEXTMETRIC NEAR *NPTEXTMETRIC;
typedef TEXTMETRIC FAR *LPTEXTMETRIC;
typedef TEXTMETRIC FAR *LPTEXTMETRICS;

/* tmPitchAndFamily values */
#define TMPF_FIXED_PITCH    0x01
#define TMPF_VECTOR         0x02
#define TMPF_DEVICE         0x08
#define TMPF_TRUETYPE       0x04


extern BOOL FAR PASCAL GetTextMetrics(HDC, LPTEXTMETRIC);

/* Logical Font */
#define LF_FACESIZE      32
typedef struct tagLOGFONT
{
  int       lfHeight;
  int       lfWidth;
  int       lfEscapement;
  int       lfOrientation;
  int       lfWeight;
#define FW_DONTCARE          0
#define FW_THIN              100
#define FW_EXTRALIGHT        200
#define FW_LIGHT             300
#define FW_NORMAL            400
#define FW_MEDIUM            500
#define FW_SEMIBOLD          600
#define FW_BOLD              700
#define FW_EXTRABOLD         800
#define FW_HEAVY             900
#define FW_ULTRALIGHT        FW_EXTRALIGHT
#define FW_REGULAR           FW_NORMAL
#define FW_DEMIBOLD          FW_SEMIBOLD
#define FW_ULTRABOLD         FW_EXTRABOLD
#define FW_BLACK             FW_HEAVY

  BYTE      lfItalic;
  BYTE      lfUnderline;
  BYTE      lfStrikeOut;
  BYTE      lfCharSet;
#define ANSI_CHARSET         0
#define DEFAULT_CHARSET      1
#define SYMBOL_CHARSET       2
#define SHIFTJIS_CHARSET     128
#define HANGEUL_CHARSET      129
#define CHINESEBIG5_CHARSET  136
#define OEM_CHARSET          255
    

  BYTE      lfOutPrecision;
#define OUT_DEFAULT_PRECIS   0
#define OUT_STRING_PRECIS    1
#define OUT_CHARACTER_PRECIS 2
#define OUT_STROKE_PRECIS    3
#define OUT_TT_PRECIS        4
#define OUT_DEVICE_PRECIS    5
#define OUT_RASTER_PRECIS    6
#define OUT_TT_ONLY_PRECIS   7
  
  BYTE      lfClipPrecision;
#define CLIP_DEFAULT_PRECIS     0x00
#define CLIP_CHARACTER_PRECIS   0x01
#define CLIP_STROKE_PRECIS      0x02
#define CLIP_MASK               0x0F
#define CLIP_LH_ANGLES          0x10
#define CLIP_TT_ALWAYS          0x20
#define CLIP_EMBEDDED           0x80
    
  BYTE      lfQuality;
#define DEFAULT_QUALITY       0
#define DRAFT_QUALITY         1
#define PROOF_QUALITY         2

  BYTE      lfPitchAndFamily;
#define DEFAULT_PITCH         0
#define FIXED_PITCH           1
#define VARIABLE_PITCH        2
    /* Font Families */
#define FF_DONTCARE       (0<<4)
#define FF_ROMAN          (1<<4)
#define FF_SWISS          (2<<4)
#define FF_MODERN         (3<<4)
#define FF_SCRIPT         (4<<4)
#define FF_DECORATIVE     (5<<4)

  char      lfFaceName[LF_FACESIZE];
} LOGFONT, *PLOGFONT, FAR *LPLOGFONT;




#if (WINVER >= 0x030a)
typedef struct tagNEWTEXTMETRIC
{
    int     tmHeight;
    int     tmAscent;
    int     tmDescent;
    int     tmInternalLeading;
    int     tmExternalLeading;
    int     tmAveCharWidth;
    int     tmMaxCharWidth;
    int     tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    int     tmOverhang;
    int     tmDigitizedAspectX;
    int     tmDigitizedAspectY;
    DWORD   ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRIC;
typedef NEWTEXTMETRIC*       PNEWTEXTMETRIC;
typedef NEWTEXTMETRIC NEAR* NPNEWTEXTMETRIC;
typedef NEWTEXTMETRIC FAR*  LPNEWTEXTMETRIC;

/* ntmFlags field flags */
#define NTM_REGULAR  0x00000040L
#define NTM_BOLD     0x00000020L
#define NTM_ITALIC   0x00000001L

#define LF_FULLFACESIZE     64

/* Structure passed to FONTENUMPROC */
typedef struct tagENUMLOGFONT
{
    LOGFONT elfLogFont;
    char    elfFullName[LF_FULLFACESIZE];
    char    elfStyle[LF_FACESIZE];
} ENUMLOGFONT, FAR* LPENUMLOGFONT;

#endif /* WINVER >= 0x30A */


/*===========================================================================*/
/*                                                                           */
/*                       SYSMETRICS  INFORMATION                             */
/*                                                                           */
/*===========================================================================*/
#ifndef NOSYSMETRICS

int WINAPI GetSystemMetrics(int);

/* GetSystemMetrics() codes */
#define SM_CXSCREEN          0
#define SM_CYSCREEN          1
#define SM_CXVSCROLL         2
#define SM_CYHSCROLL         3
#define SM_CYCAPTION         4
#define SM_CXBORDER          5
#define SM_CYBORDER          6
#define SM_CXDLGFRAME        7
#define SM_CYDLGFRAME        8
#define SM_CYVTHUMB          9
#define SM_CXHTHUMB          10
#define SM_CXICON            11
#define SM_CYICON            12
#define SM_CXCURSOR          13
#define SM_CYCURSOR          14
#define SM_CYMENU            15
#define SM_CXFULLSCREEN      16
#define SM_CYFULLSCREEN      17
#define SM_CYKANJIWINDOW     18
#define SM_MOUSEPRESENT      19
#define SM_CYVSCROLL         20
#define SM_CXHSCROLL         21
#define SM_DEBUG             22
#define SM_SWAPBUTTON        23
#define SM_RESERVED1         24
#define SM_RESERVED2         25
#define SM_RESERVED3         26
#define SM_RESERVED4         27
#define SM_CXMIN             28
#define SM_CYMIN             29
#define SM_CXSIZE            30
#define SM_CYSIZE            31
#define SM_CXFRAME           32
#define SM_CYFRAME           33
#define SM_CXMINTRACK        34
#define SM_CYMINTRACK        35

#if (WINVER >= 0x030a)
#define SM_CXDOUBLECLK       36
#define SM_CYDOUBLECLK       37
#define SM_CXICONSPACING     38
#define SM_CYICONSPACING     39
#define SM_MENUDROPALIGNMENT 40
#define SM_PENWINDOWS        41
#define SM_DBCSENABLED       42
#endif  /* WINVER >= 0x030a */

#define SM_CMETRICS          45

/* MEWEL-specific extensions */
#define SM_CXCHECKBOX        43
#define SM_CYCHECKBOX        44

#endif  /* NOSYSMETRICS */


/* OpenFile() Flags */
#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002
#define OF_SHARE_COMPAT     0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400
#define OF_CANCEL           0x0800
#define OF_CREATE           0x1000
#define OF_PROMPT           0x2000
#define OF_EXIST            0x4000
#define OF_REOPEN           0x8000

#define HFILE_ERROR ((HFILE)-1)


/* 
  GetDriveType()
*/
extern UINT FAR PASCAL GetDriveType(int);
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED     3
#define DRIVE_REMOTE    4

/*==========================================================================*/
/*                               Structures                                 */
/*==========================================================================*/

/* OpenFile() Structure */
typedef struct tagOFSTRUCT
{
  BYTE cBytes;
  BYTE fFixedDisk;
  UINT nErrCode;
  BYTE reserved[4];
  char szPathName[128];
} OFSTRUCT;
typedef OFSTRUCT        *POFSTRUCT;
typedef OFSTRUCT NEAR   *NPOFSTRUCT;
typedef OFSTRUCT FAR    *LPOFSTRUCT;

/* OpenFile() Flags */
#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002

#define OF_SHARE_COMPAT     0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040

#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400
#define OF_CANCEL           0x0800

#define OF_CREATE           0x1000
#define OF_PROMPT           0x2000
#define OF_EXIST            0x4000
#define OF_REOPEN           0x8000

HFILE WINAPI OpenFile(LPCSTR, OFSTRUCT FAR*, UINT);

/* GetTempFileName() Flags */
#define TF_FORCEDRIVE       ((BYTE) 0x80)
BYTE FAR PASCAL GetTempDrive(BYTE);
int  FAR PASCAL GetTempFileName(BYTE, LPSTR, UINT, LPSTR);

/*===========================================================================*/
/*                                                                           */
/*                       VIDEO INFORMATION                                   */
/*                                                                           */
/*===========================================================================*/
/*
  Color info
*/
#define INTENSE(color)  ( (color) | 0x08 )

#if (!defined(CPPV) && !defined(ZAPP)) || defined(_WPRIVATE_H)
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define YELLOW          6
#define WHITE           7
#define INTENSE_BLACK   INTENSE(BLACK)
#define INTENSE_BLUE    INTENSE(BLUE)
#define INTENSE_GREEN   INTENSE(GREEN)
#define INTENSE_CYAN    INTENSE(CYAN)
#define INTENSE_RED     INTENSE(RED)
#define INTENSE_MAGENTA INTENSE(MAGENTA)
#define INTENSE_YELLOW  INTENSE(YELLOW)
#define INTENSE_WHITE   INTENSE(WHITE)
#endif

#define MAKE_ATTR(fg, bg)       ( (((bg) << 4) | (fg)) )
#ifdef DUMB_CURSES /* normal and inverse video, use standout sparingly */
#define MAKE_HIGHLITE(attr)     MAKE_ATTR(BLACK,WHITE)
#else
#define MAKE_HIGHLITE(attr)     ( (((attr) << 4) | ((attr) >> 4)) & (TEST_PROGRAM_STATE(STATE_EXTENDED_ATTRS) ? 0xFF : 0x7F) )
#endif
#define GET_FOREGROUND(attr)    ( (attr) & 0x0F )
#define GET_BACKGROUND(attr)    ( ((attr) >> 4) & 0x0F )


/*===========================================================================*/
/*                                                                           */
/* File    : SOUND.H                                                         */
/*                                                                           */
/*===========================================================================*/

/* Notes available for music routine */
#define PAUSENOTE -1     /* Programmed Pause */
#define RESTNOTE   0     /* Musical rest (silence) */
#define CNOTE      1     /* C, C sharp, D flat, etc. */
#define CsNOTE     2
#define DbNOTE     2
#define DNOTE      3
#define DsNOTE     4
#define EbNOTE     4
#define ENOTE      5
#define FNOTE      6
#define FsNOTE     7
#define GbNOTE     7
#define GNOTE      8
#define GsNOTE     9
#define AbNOTE     9
#define ANOTE      10
#define AsNOTE     11
#define BbNOTE     11
#define BNOTE      12

/* Octaves range available for music routine */
#define MIN_OCTAVE   -3             /* Lowest avalible octave */
#define MID_OCTAVE    0             /* Octave middle C is in */
#define MAX_OCTAVE    7             /* Highest avalible octave */


/****** Sound support ******************************************************/

#ifndef NOSOUND

/* SetSoundNoise() Sources */
#define S_PERIOD512   0
#define S_PERIOD1024  1
#define S_PERIOD2048  2
#define S_PERIODVOICE 3
#define S_WHITE512    4
#define S_WHITE1024   5
#define S_WHITE2048   6
#define S_WHITEVOICE  7

/* WaitSoundState() constants */
#define S_QUEUEEMPTY        0
#define S_THRESHOLD         1
#define S_ALLTHRESHOLD      2

/* Accent Modes */
#define S_NORMAL      0
#define S_LEGATO      1
#define S_STACCATO    2

/* Error return values */
#define S_SERDVNA     (-1)
#define S_SEROFM      (-2)
#define S_SERMACT     (-3)
#define S_SERQFUL     (-4)
#define S_SERBDNT     (-5)
#define S_SERDLN      (-6)
#define S_SERDCC      (-7)
#define S_SERDTP      (-8)
#define S_SERDVL      (-9)
#define S_SERDMD      (-10)
#define S_SERDSH      (-11)
#define S_SERDPT      (-12)
#define S_SERDFQ      (-13)
#define S_SERDDR      (-14)
#define S_SERDSR      (-15)
#define S_SERDST      (-16)

#endif  /* NOSOUND */


/****************************************************************************/
/*                                                                          */
/*                        OBJECT HANDLING                                   */
/*                                                                          */
/****************************************************************************/

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
{
  UINT  lbStyle;
  DWORD lbColor;
  int   lbHatch;
} LOGBRUSH, *PLOGBRUSH, *LPLOGBRUSH;


/* Logical Pen */
typedef struct tagLOGPEN
{
  UINT  lopnStyle;
  POINT lopnWidth;
  DWORD lopnColor;
} LOGPEN, *PLOGPEN, *LPLOGPEN;

typedef struct tagPALETTEENTRY
{
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
/* Palette entry flags */
#define PC_RESERVED     0x01    /* palette index used for animation */
#define PC_EXPLICIT     0x02    /* palette index is explicit to device */
#define PC_NOCOLLAPSE   0x04    /* do not match color to system palette */
} PALETTEENTRY, FAR  *LPPALETTEENTRY;

/* Logical Palette */
typedef struct tagLOGPALETTE
{
  UINT palVersion;
  UINT palNumEntries;
  PALETTEENTRY    palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, *NPLOGPALETTE, FAR *LPLOGPALETTE;

/* Get/SetSystemPaletteUse() values */
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2


/* Brush Styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3
#define BS_INDEXED          4
#define BS_DIBPATTERN       5

/* Hatch Styles */
#define HS_HORIZONTAL       0            /* ----- */
#define HS_VERTICAL         1            /* ||||| */
#define HS_FDIAGONAL        2            /* \\\\\ */
#define HS_BDIAGONAL        3            /* ///// */
#define HS_CROSS            4            /* +++++ */
#define HS_DIAGCROSS        5            /* xxxxx */

/* Pen Styles */
#define PS_SOLID            0
#define PS_DASH             1            /* -------        */
#define PS_DOT              2            /* .......        */
#define PS_DASHDOT          3            /* _._._._        */
#define PS_DASHDOTDOT       4            /* _.._.._        */
#define PS_NULL             5
#define PS_INSIDEFRAME      6

/* Stock Logical Objects */
#define BADOBJECT           ((HANDLE) -1)
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8

#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16

#define STOCK_OBJECT_FIRST  WHITE_BRUSH
#define STOCK_OBJECT_LAST   SYSTEM_FIXED_FONT



/*--------------------------------------------------------------------*/
#ifndef NOOBJECTS

#ifdef DeleteObject
#undef DeleteObject
#endif
#ifdef SelectObject
#undef SelectObject
#endif
#ifdef CreatePen
#undef CreatePen
#endif
#ifdef GetStockObject
#undef GetStockObject
#endif
#ifdef CreateSolidBrush
#undef CreateSolidBrush
#endif

int    FAR PASCAL GetObject(HGDIOBJ, INT, VOID FAR *);
BOOL   FAR PASCAL DeleteObject(HANDLE);
HANDLE FAR PASCAL SelectObject(HDC, HANDLE);

HANDLE FAR PASCAL GetStockObject(int);
BOOL   FAR PASCAL IsGDIObject(HGDIOBJ);

HPEN   FAR PASCAL CreatePen(int, int, DWORD);
HPEN   FAR PASCAL CreatePenIndirect(LOGPEN *);

HBRUSH FAR PASCAL CreateSolidBrush(DWORD);
HBRUSH FAR PASCAL CreateHatchBrush(int, DWORD);
BOOL   FAR PASCAL UnrealizeObject(HGDIOBJ);
#ifdef OS2
HBRUSH FAR PASCAL CreatePatternBrush(HANDLE);
#else
HBRUSH FAR PASCAL CreatePatternBrush(HBITMAP);
#endif
HBRUSH FAR PASCAL CreateBrushIndirect(LOGBRUSH *);

int  FAR PASCAL SetROP2(HDC, int);
int  FAR PASCAL GetROP2(HDC);
int  FAR PASCAL SetBkMode(HDC, int);
int  FAR PASCAL GetBkMode(HDC);
int  FAR PASCAL SetMapMode(HDC, int);
int  FAR PASCAL GetMapMode(HDC);

/* Binary raster ops */
#define R2_BLACK            1  /*  0   */
#define R2_NOTMERGEPEN      2  /* DPon */
#define R2_MASKNOTPEN       3  /* DPna */
#define R2_NOTCOPYPEN       4  /* PN   */
#define R2_MASKPENNOT       5  /* PDna */
#define R2_NOT              6  /* Dn   */
#define R2_XORPEN           7  /* DPx  */
#define R2_NOTMASKPEN       8  /* DPan */
#define R2_MASKPEN          9  /* DPa  */
#define R2_NOTXORPEN	    10 /* DPxn */
#define R2_NOP		    11 /* D    */
#define R2_MERGENOTPEN	    12 /* DPno */
#define R2_COPYPEN	    13 /* P    */
#define R2_MERGEPENNOT	    14 /* PDno */
#define R2_MERGEPEN	    15 /* DPo  */
#define R2_WHITE	    16 /*  1   */

/* Mapping modes */

/* Background modes */
#define TRANSPARENT     1
#define OPAQUE          2

#define MM_TEXT         1
#define MM_LOMETRIC     2
#define MM_HIMETRIC     3
#define MM_LOENGLISH    4
#define MM_HIENGLISH    5
#define MM_TWIPS        6
#define MM_ISOTROPIC    7
#define MM_ANISOTROPIC  8



DWORD FAR PASCAL SetWindowOrg(HDC, int, int);
DWORD FAR PASCAL OffsetWindowOrg(HDC, int, int);
DWORD FAR PASCAL GetWindowOrg(HDC);
DWORD FAR PASCAL SetWindowExt(HDC, int, int);
DWORD FAR PASCAL GetWindowExt(HDC);
DWORD FAR PASCAL ScaleWindowExt(HDC, int, int, int, int);
DWORD FAR PASCAL SetViewportOrg(HDC, int, int);
DWORD FAR PASCAL OffsetViewportOrg(HDC, int, int);
DWORD FAR PASCAL GetViewportOrg(HDC);
DWORD FAR PASCAL SetViewportExt(HDC, int, int);
DWORD FAR PASCAL GetViewportExt(HDC);
DWORD FAR PASCAL ScaleViewportExt(HDC, int, int, int, int);

#if (WINVER >= 0x030a)
BOOL WINAPI SetWindowOrgEx(HDC, int, int, POINT FAR*);
BOOL WINAPI GetWindowOrgEx(HDC, POINT FAR*);
BOOL WINAPI SetWindowExtEx(HDC, int, int, SIZE FAR*);
BOOL WINAPI GetWindowExtEx(HDC, SIZE FAR*);
BOOL WINAPI OffsetWindowOrgEx(HDC, int, int, POINT FAR*);
BOOL WINAPI ScaleWindowExtEx(HDC, int, int, int, int, SIZE FAR*);
BOOL WINAPI SetViewportExtEx(HDC, int, int, SIZE FAR*);
BOOL WINAPI GetViewportExtEx(HDC, SIZE FAR*);
BOOL WINAPI SetViewportOrgEx(HDC, int, int, POINT FAR*);
BOOL WINAPI GetViewportOrgEx(HDC, POINT FAR*);
BOOL WINAPI OffsetViewportOrgEx(HDC, int, int, POINT FAR*);
BOOL WINAPI ScaleViewportExtEx(HDC, int, int, int, int, SIZE FAR*);
#endif  /* WINVER >= 0x030a */

BOOL  FAR PASCAL LPtoDP(HDC, LPPOINT, int);
BOOL  FAR PASCAL DPtoLP(HDC, LPPOINT, int);

/* PolyFill() Modes */
#define ALTERNATE        1
#define WINDING          2
int   FAR PASCAL SetPolyFillMode(HDC, int);
int   FAR PASCAL GetPolyFillMode(HDC);

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

/*
  Region stuff
*/
HRGN FAR PASCAL CreateRectRgn(int, int, int, int);
int  FAR PASCAL SelectClipRgn(HDC, HRGN);
int  FAR PASCAL GetUpdateRgn(HWND, HRGN, BOOL);
int  FAR PASCAL ExcludeUpdateRgn(HDC, HWND);
void FAR PASCAL InvalidateRgn(HWND, HRGN, BOOL);
void FAR PASCAL ValidateRgn(HWND, HRGN);
#define ERROR          0
#define NULLREGION     1
#define SIMPLEREGION   2
#define COMPLEXREGION  3

/* CombineRgn() Styles */
#define RGN_AND       1
#define RGN_OR        2
#define RGN_XOR       3
#define RGN_DIFF      4
#define RGN_COPY      5


HFONT FAR PASCAL CreateFont(int,int,int,int,int,
                        BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT FAR PASCAL CreateFontIndirect(CONST LOGFONT FAR *);

#ifdef VAXC
#pragma nostandard
#endif

/* Text Alignment Options */
extern UINT FAR PASCAL SetTextAlign(HDC, UINT);
extern UINT FAR PASCAL GetTextAlign(HDC);
#define TA_NOUPDATECP    0
#define TA_UPDATECP      1
#define TA_LEFT          0
#define TA_RIGHT         2
#define TA_CENTER        4
#define TA_TOP           0
#define TA_BOTTOM        8
#define TA_BASELINE      24

#endif /* NOOBJECTS */


/* SetWindowsHook() codes */
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD         2
#define WH_GETMESSAGE       3
#define WH_CALLWNDPROC      4
#define WH_CBT              5
#define WH_SYSMSGFILTER     6
#define WH_WINDOWMGR        7
#define WH_MSGFILTER        8
#define WH_COMMDLG          9  /* MEWEL-specific */
#define WH_LAST             WH_COMMDLG

#ifndef NOWH
#define HCBT_MOVESIZE       0
#define HCBT_MINMAX         1
#define HCBT_QS             2
#define HCBT_CREATEWND      3
#define HCBT_DESTROYWND     4
#define HCBT_ACTIVATE       5
#define HCBT_CLICKSKIPPED   6
#define HCBT_KEYSKIPPED     7
#define HCBT_SYSCOMMAND     8
#define HCBT_SETFOCUS       9

/* HCBT_CREATEWND parameters pointed to by lParam */
typedef struct tagCBT_CREATEWND
{
  CREATESTRUCT FAR* lpcs;
  HWND    hwndInsertAfter;
} CBT_CREATEWND;
typedef CBT_CREATEWND FAR* LPCBT_CREATEWND;

/* HCBT_ACTIVATE structure pointed to by lParam */
typedef struct tagCBTACTIVATESTRUCT
{
  BOOL    fMouse;
  HWND    hWndActive;
} CBTACTIVATESTRUCT;
#endif  /* NOWH */


/* Hook Codes */
#define HC_LPLPFNNEXT       (-2)
#define HC_LPFNNEXT         (-1)
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREM            3
#define HC_NOREMOVE         3
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

/* WH_MSGFILTER Filter Proc Codes */
#define MSGF_DIALOGBOX      0
#define MSGF_MESSAGEBOX     1
#define MSGF_MENU           2
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6

/* Window Manager Hook Codes */
#define WC_INIT             1
#define WC_SWP              2
#define WC_DEFWINDOWPROC    3
#define WC_MINMAX           4
#define WC_MOVE             5
#define WC_SIZE             6
#define WC_DRAWCAPTION      7


/*
  Printing support
*/
typedef struct
{
  int     cbSize;
  LPCSTR  lpszDocName;
  LPCSTR  lpszOutput;
} DOCINFO;
typedef DOCINFO FAR* LPDOCINFO;

int     WINAPI StartDoc(HDC, DOCINFO FAR*);
int     WINAPI StartPage(HDC);
int     WINAPI EndPage(HDC);
int     WINAPI EndDoc(HDC);
int     WINAPI AbortDoc(HDC);

int     WINAPI SetAbortProc(HDC, ABORTPROC);
HANDLE  WINAPI SpoolFile(LPSTR, LPSTR, LPSTR, LPSTR);
extern int  FAR PASCAL Escape(HDC, int, int, LPCSTR, VOID FAR *);


extern HDC  FAR PASCAL CreateDC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *);
extern HDC  FAR PASCAL CreateIC(LPCSTR, LPCSTR, LPCSTR, CONST VOID FAR *);
extern BOOL FAR PASCAL DeleteDC(HDC);

extern int  FAR PASCAL GetDeviceCaps(HDC, int);
/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version      */
#define TECHNOLOGY    2     /* Device classification      */
#define HORZSIZE      4     /* Horizontal size in millimeters    */
#define VERTSIZE      6     /* Vertical size in millimeters    */
#define HORZRES       8     /* Horizontal width in pixels    */
#define VERTRES       10    /* Vertical width in pixels     */
#define BITSPIXEL     12    /* Number of bits per pixel     */
#define PLANES        14    /* Number of planes       */
#define NUMBRUSHES    16    /* Number of brushes the device has   */
#define NUMPENS       18    /* Number of pens the device has    */
#define NUMMARKERS    20    /* Number of markers the device has   */
#define NUMFONTS      22    /* Number of fonts the device has    */
#define NUMCOLORS     24    /* Number of colors the device supports  */
#define PDEVICESIZE   26    /* Size required for device descriptor  */
#define CURVECAPS     28    /* Curve capabilities      */
#define LINECAPS      30    /* Line capabilities      */
#define POLYGONALCAPS 32    /* Polygonal capabilities      */
#define TEXTCAPS      34    /* Text capabilities      */
#define CLIPCAPS      36    /* Clipping capabilities      */
#define RASTERCAPS    38    /* Bitblt capabilities      */
#define ASPECTX       40    /* Length of the X leg      */
#define ASPECTY       42    /* Length of the Y leg      */
#define ASPECTXY      44    /* Length of the hypotenuse     */

#define LOGPIXELSX    88    /* Logical pixels/inch in X     */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y     */

#define SIZEPALETTE  104    /* Number of entries in physical palette  */
#define NUMRESERVED  106    /* Number of reserved entries in palette  */
#define COLORRES     108    /* Actual color resolution       */



#ifndef NOGDI

/*
  Pack structures to byte boundaries
*/
#ifdef VAXC
#pragma standard
#endif

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif

typedef struct tagRGBTRIPLE
{
  BYTE rgbtBlue;
  BYTE rgbtGreen;
  BYTE rgbtRed;
} RGBTRIPLE;

typedef struct tagRGBQUAD
{
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD;


typedef struct tagBITMAPINFOHEADER
{
  DWORD     biSize;
  DWORD     biWidth;
  DWORD     biHeight;
  USHORT    biPlanes;
  USHORT    biBitCount;

  DWORD     biCompression;
  DWORD     biSizeImage;
  DWORD     biXPelsPerMeter;
  DWORD     biYPelsPerMeter;
  DWORD     biClrUsed;
  DWORD     biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER FAR *LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER *PBITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{ 
  BITMAPINFOHEADER  bmiHeader;
  RGBQUAD    bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO FAR *LPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct tagBITMAPFILEHEADER
{
  USHORT  bfType;
  DWORD    bfSize;
  USHORT  bfReserved1;
  USHORT  bfReserved2;
  DWORD    bfOffBits;
} BITMAPFILEHEADER;
typedef BITMAPFILEHEADER FAR *LPBITMAPFILEHEADER;
typedef BITMAPFILEHEADER *PBITMAPFILEHEADER;

typedef struct tagBITMAP
{
  int   bmType;
  int   bmWidth;
  int   bmHeight;
  int   bmWidthBytes;
  BYTE  bmPlanes;
  BYTE  bmBitsPixel;
  LPSTR bmBits;
} BITMAP;
typedef BITMAP NEAR      *NPBITMAP;
typedef BITMAP FAR      *LPBITMAP;


/*
  Restore default structure packing
*/
#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 2
#elif defined(DOS386) || defined(WC386)
#pragma pack
#else
#pragma pack()
#endif
#endif


/*
  ROP codes
*/
#define SRCCOPY             0x00CC0020L
#define SRCPAINT            0x00EE0086L
#define SRCAND              0x008800C6L
#define SRCINVERT           0x00660046L
#define SRCERASE            0x00440328L
#define NOTSRCCOPY          0x00330008L
#define NOTSRCERASE         0x001100A6L
#define MERGECOPY           0x00C000CAL
#define MERGEPAINT          0x00BB0226L
#define PATCOPY             0x00F00021L
#define PATPAINT            0x00FB0A09L
#define PATINVERT           0x005A0049L
#define DSTINVERT           0x00550009L
#define BLACKNESS           0x00000042L
#define WHITENESS           0x00FF0062L

/* StretchBlt() Modes */
#define BLACKONWHITE         1
#define WHITEONBLACK         2
#define COLORONCOLOR         3

HDC     FAR PASCAL CreateCompatibleDC(HDC);
HBITMAP FAR PASCAL CreateBitmap(int, int, UINT, UINT, CONST void FAR *);
BOOL    FAR PASCAL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);


BOOL  FAR PASCAL RoundRect(HDC, int, int, int, int, int, int);
BOOL  FAR PASCAL StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
int   FAR PASCAL SetStretchBltMode(HDC, int);
HBITMAP FAR PASCAL CreateBitmapIndirect(BITMAP FAR *);
LONG  FAR PASCAL GetBitmapBits(HBITMAP, LONG, VOID FAR*);
LONG  FAR PASCAL SetBitmapBits(HBITMAP, DWORD, CONST VOID FAR*);

/*
  DIB functions
*/
int FAR PASCAL SetDIBits(HDC,HBITMAP,UINT,UINT,CONST VOID FAR*,LPBITMAPINFO,UINT);
int FAR PASCAL GetDIBits(HDC,HBITMAP,UINT,UINT,VOID FAR *,LPBITMAPINFO,UINT);
int FAR PASCAL SetDIBitsToDevice(HDC,INT,INT,INT,INT,INT,INT,UINT,UINT,
                                 VOID FAR *,BITMAPINFO FAR *,UINT);
HBITMAP FAR PASCAL CreateDIBitmap(HDC,BITMAPINFOHEADER FAR *,DWORD,
                                  CONST VOID FAR *,BITMAPINFO FAR *,UINT);
INT    FAR PASCAL StretchDIBits(HDC, INT, INT, INT, INT, INT, 
                                INT, INT, INT, CONST VOID FAR*, LPBITMAPINFO, 
                                UINT, DWORD);

#if !defined(BYTE_SWAP)
void swBitmapInfoHeader(LPBITMAPINFOHEADER);
#else
#define swBitmapInfoHeader(x)  (x)
#endif


#ifdef STRICT
typedef void (CALLBACK* LINEDDAPROC)(int, int, LPARAM);
#else
typedef FARPROC LINEDDAPROC;
#endif
VOID FAR PASCAL LineDDA(int, int, int, int, LINEDDAPROC, LPARAM);

BOOL  FAR PASCAL PatBlt(HDC, int, int, int, int, DWORD);
BOOL  FAR PASCAL Chord(HDC, int, int, int, int, int, int, int, int);

#if !defined(MAC)
BOOL FAR PASCAL FillRgn(HDC, HRGN, HBRUSH);
BOOL FAR PASCAL FrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL FAR PASCAL InvertRgn(HDC, HRGN);
BOOL FAR PASCAL PaintRgn(HDC, HRGN);
BOOL FAR PASCAL EqualRgn(HRGN, HRGN);
int  FAR PASCAL OffsetRgn(HRGN, int, int);
void FAR PASCAL SetRectRgn(HRGN, int, int, int, int);
#endif
int  FAR PASCAL CombineRgn(HRGN, HRGN, HRGN, int);
int  FAR PASCAL GetRgnBox(HRGN, LPRECT);
BOOL FAR PASCAL PtInRegion(HRGN, int, int);
BOOL FAR PASCAL RectInRegion(HRGN, CONST RECT FAR *);
HRGN FAR PASCAL CreateEllipticRgnIndirect(CONST RECT FAR *);
HRGN FAR PASCAL CreateEllipticRgn(int, int, int, int);
HRGN FAR PASCAL CreatePolygonRgn(CONST POINT FAR *, int, int);
HRGN FAR PASCAL CreatePolyPolygonRgn(CONST POINT FAR *, CONST INT FAR *, int, int);
HRGN FAR PASCAL CreateRoundRectRgn(int, int, int, int, int, int);
HRGN FAR PASCAL CreateRectRgnIndirect(CONST RECT FAR *);

int     FAR PASCAL GetClipBox(HDC, LPRECT);

#ifdef STRICT
typedef int (CALLBACK* OLDFONTENUMPROC)(CONST LOGFONT FAR*, CONST TEXTMETRIC FAR*, int, LPARAM);
typedef int (CALLBACK* FONTENUMPROC)(CONST ENUMLOGFONT FAR*, CONST NEWTEXTMETRIC FAR*, int, LPARAM);
int     WINAPI EnumFontFamilies(HDC, LPCSTR, FONTENUMPROC, LPARAM);
int     WINAPI EnumFonts(HDC, LPCSTR, OLDFONTENUMPROC, LPARAM);
#else
typedef FARPROC OLDFONTENUMPROC;
typedef FARPROC FONTENUMPROC;
int     WINAPI EnumFonts(HDC, LPCSTR, OLDFONTENUMPROC, LPSTR);
int     WINAPI EnumFontFamilies(HDC, LPCSTR, FONTENUMPROC, LPSTR);
#endif


DWORD     FAR PASCAL SetMapperFlags(HDC, DWORD);
INT       FAR PASCAL GetTextFace(HDC, int, LPSTR);
FARPROC   FAR PASCAL GetProcAddress(HINSTANCE, LPCSTR);
HINSTANCE FAR PASCAL LoadLibrary(LPCSTR);
VOID      FAR PASCAL FreeLibrary(HINSTANCE);
#define HINSTANCE_ERROR ((HINSTANCE) 32)


/* System bitmaps */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738
#define OBM_UPARROWI        32737
#define OBM_DNARROWI        32736
#define OBM_RGARROWI        32735
#define OBM_LFARROWI        32734

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

typedef struct tagBITMAPCOREHEADER
{
  DWORD  bcSize;
  short bcWidth;
  short bcHeight;
  WORD  bcPlanes;
  WORD  bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR *LPBITMAPCOREHEADER;
typedef BITMAPCOREHEADER *PBITMAPCOREHEADER;

typedef struct tagBITMAPCOREINFO
{
  BITMAPCOREHEADER bmciHeader;
  RGBTRIPLE        bmciColors[1];
} BITMAPCOREINFO;
typedef BITMAPCOREINFO*      PBITMAPCOREINFO;
typedef BITMAPCOREINFO FAR* LPBITMAPCOREINFO;

#define  CBM_INIT  0x04L  /* initialize bitmap */

#define  DIB_RGB_COLORS  0  /* color table in RGBTriples */
#define  DIB_PAL_COLORS  1  /* color table in palette indices */

/* constants for the biCompression field */
#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L

HBITMAP FAR PASCAL CreateCompatibleBitmap(HDC, int, int);
void FAR PASCAL FillWindow(HWND, HWND, HDC, HBRUSH);

/****************************************************************************/
/*                                                                          */
/*                           METAFILE STUFF                                 */
/*                                                                          */
/****************************************************************************/
#ifndef NOMETAFILE

DECLARE_HANDLE(HMETAFILE);

/* Clipboard Metafile Picture Structure */
typedef struct tagMETAFILEPICT
{
  int     mm;
  int     xExt;
  int     yExt;
  HMETAFILE hMF;
} METAFILEPICT;
typedef METAFILEPICT FAR* LPMETAFILEPICT;

typedef struct tagMETAHEADER
{
  UINT    mtType;
  UINT    mtHeaderSize;
  UINT    mtVersion;
  DWORD   mtSize;
  UINT    mtNoObjects;
  DWORD   mtMaxRecord;
  UINT    mtNoParameters;
} METAHEADER;

typedef struct tagHANDLETABLE
{
  HGDIOBJ objectHandle[1];
} HANDLETABLE;
typedef HANDLETABLE*      PHANDLETABLE;
typedef HANDLETABLE FAR* LPHANDLETABLE;

typedef struct tagMETARECORD
{
  DWORD   rdSize;
  UINT    rdFunction;
  UINT    rdParm[1];
} METARECORD;
typedef METARECORD*      PMETARECORD;
typedef METARECORD FAR* LPMETARECORD;

/* Metafile Functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0410
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_DRAWTEXT                0x062F

#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0a32
#define META_SETDIBTODEV             0x0d33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139

#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0b41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0f43

#define META_EXTFLOODFILL            0x0548

#define META_RESETDC                 0x014C
#define META_STARTDOC                0x014D
#define META_STARTPAGE               0x004F
#define META_ENDPAGE                 0x0050
#define META_ABORTDOC                0x0052
#define META_ENDDOC                  0x005E

#define META_DELETEOBJECT            0x01f0

#define META_CREATEPALETTE           0x00f7
#define META_CREATEBRUSH             0x00F8
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEBITMAPINDIRECT    0x02FD
#define META_CREATEBITMAP            0x06FE
#define META_CREATEREGION            0x06FF

/* WMETAFIL.C */
HMETAFILE WINAPI CloseMetaFile(HDC);
HMETAFILE WINAPI CopyMetaFile(HMETAFILE, LPCSTR);
HDC       WINAPI CreateMetaFile(LPCSTR);
BOOL      WINAPI DeleteMetaFile(HMETAFILE);
HMETAFILE WINAPI GetMetaFile(LPCSTR);
HGLOBAL   WINAPI GetMetaFileBits(HMETAFILE);
BOOL      WINAPI PlayMetaFile(HDC, HMETAFILE);
HMETAFILE WINAPI SetMetaFileBits(HGLOBAL);
HMETAFILE WINAPI SetMetaFileBitsBetter(HGLOBAL);
void      WINAPI PlayMetaFileRecord(HDC, HANDLETABLE FAR*, METARECORD FAR*, UINT);

#ifdef STRICT
typedef int (CALLBACK* MFENUMPROC)(HDC, HANDLETABLE FAR*, METARECORD FAR*, int, LPARAM);
#else
typedef FARPROC MFENUMPROC;
#endif
BOOL    WINAPI EnumMetaFile(HDC, HMETAFILE, MFENUMPROC, LPARAM);

#endif /*NOMETAFILE*/



/* GDI Escapes */
#define NEWFRAME                     1
#define ABORTDOC                     2
#define NEXTBAND                     3
#define SETCOLORTABLE                4
#define GETCOLORTABLE                5
#define FLUSHOUTPUT                  6
#define DRAFTMODE                    7
#define QUERYESCSUPPORT              8
#define SETABORTPROC                 9
#define STARTDOC                     10
#define ENDDOC                       11
#define GETPHYSPAGESIZE              12
#define GETPRINTINGOFFSET            13
#define GETSCALINGFACTOR             14
#define MFCOMMENT                    15
#define GETPENWIDTH                  16
#define SETCOPYCOUNT                 17
#define SELECTPAPERSOURCE            18
#define DEVICEDATA                   19
#define PASSTHROUGH                  19
#define GETTECHNOLGY                 20
#define GETTECHNOLOGY                20
#define SETLINECAP                   21
#define SETENDCAP                    21
#define SETLINEJOIN                  22
#define SETMITERLIMIT                23
#define BANDINFO                     24
#define DRAWPATTERNRECT              25
#define GETVECTORPENSIZE             26
#define GETVECTORBRUSHSIZE           27
#define ENABLEDUPLEX                 28
#define GETSETPAPERBINS              29
#define GETSETPRINTORIENT            30
#define ENUMPAPERBINS                31
#define SETDIBSCALING                32
#define EPSPRINTING                  33
#define ENUMPAPERMETRICS             34
#define GETSETPAPERMETRICS           35
#define POSTSCRIPT_DATA              37
#define POSTSCRIPT_IGNORE            38
#define GETEXTENDEDTEXTMETRICS       256
#define GETEXTENTTABLE               257
#define GETPAIRKERNTABLE             258
#define GETTRACKKERNTABLE            259
#define EXTTEXTOUT                   512
#define ENABLERELATIVEWIDTHS         768
#define ENABLEPAIRKERNING            769
#define SETKERNTRACK                 770
#define SETALLJUSTVALUES             771
#define SETCHARSET                   772

#define STRETCHBLT                   2048
#define BEGIN_PATH                   4096
#define CLIP_TO_PATH                 4097
#define END_PATH                     4098
#define EXT_DEVICE_CAPS              4099
#define RESTORE_CTM                  4100
#define SAVE_CTM                     4101
#define SET_ARC_DIRECTION            4102
#define SET_BACKGROUND_COLOR         4103
#define SET_POLY_MODE                4104
#define SET_SCREEN_ANGLE             4105
#define SET_SPREAD                   4106
#define TRANSFORM_CTM                4107
#define SET_CLIP_BOX                 4108
#define SET_BOUNDS                   4109
#define SET_MIRROR_MODE              4110

/* Spooler Error Codes */
#define SP_NOTREPORTED               0x4000
#define SP_ERROR                     (-1)
#define SP_APPABORT                  (-2)
#define SP_USERABORT                 (-3)
#define SP_OUTOFDISK                 (-4)
#define SP_OUTOFMEMORY               (-5)

#define PR_JOBSTATUS                 0x0000

#ifndef NOGDICAPMASKS
/* Device Capability Masks: */
/* Device Technologies */
#define DT_PLOTTER         0  /* Vector plotter        */
#define DT_RASDISPLAY      1  /* Raster display        */
#define DT_RASPRINTER      2  /* Raster printer        */
#define DT_RASCAMERA       3  /* Raster camera        */
#define DT_CHARSTREAM      4  /* Character-stream, PLP      */
#define DT_METAFILE        5  /* Metafile, VDM        */
#define DT_DISPFILE        6  /* Display-file         */

/* Curve Capabilities */
#define CC_NONE            0  /* Curves not supported       */
#define CC_CIRCLES         1  /* Can do circles        */
#define CC_PIE             2  /* Can do pie wedges        */
#define CC_CHORD           4  /* Can do chord arcs        */
#define CC_ELLIPSES        8  /* Can do ellipese        */
#define CC_WIDE            16 /* Can do wide lines        */
#define CC_STYLED          32 /* Can do styled lines        */
#define CC_WIDESTYLED      64 /* Can do wide styled lines      */
#define CC_INTERIORS      128 /* Can do interiors        */

/* Line Capabilities */
#define LC_NONE           0   /* Lines not supported        */
#define LC_POLYLINE       2   /* Can do polylines        */
#define LC_MARKER         4   /* Can do markers        */
#define LC_POLYMARKER     8   /* Can do polymarkers        */
#define LC_WIDE           16  /* Can do wide lines        */
#define LC_STYLED         32  /* Can do styled lines        */
#define LC_WIDESTYLED     64  /* Can do wide styled lines      */
#define LC_INTERIORS      128 /* Can do interiors        */

/* Polygonal Capabilities */
#define PC_NONE           0  /* Polygonals not supported      */
#define PC_POLYGON        1  /* Can do polygons        */
#define PC_RECTANGLE      2  /* Can do rectangles        */
#define PC_WINDPOLYGON    4  /* Can do winding polygons      */
#define PC_TRAPEZOID      4  /* Can do trapezoids        */
#define PC_SCANLINE       8  /* Can do scanlines        */
#define PC_WIDE           16  /* Can do wide borders        */
#define PC_STYLED         32  /* Can do styled borders      */
#define PC_WIDESTYLED     64  /* Can do wide styled borders      */
#define PC_INTERIORS      128 /* Can do interiors        */

/* Polygonal Capabilities */
#define CP_NONE           0  /* No clipping of output      */
#define CP_RECTANGLE      1  /* Output clipped to rects      */

/* Text Capabilities */
#define TC_OP_CHARACTER   0x0001  /* Can do OutputPrecision  CHARACTER  */
#define TC_OP_STROKE      0x0002  /* Can do OutputPrecision  STROKE     */
#define TC_CP_STROKE      0x0004  /* Can do ClipPrecision  STROKE       */
#define TC_CR_90          0x0008  /* Can do CharRotAbility  90         */
#define TC_CR_ANY         0x0010  /* Can do CharRotAbility  ANY         */
#define TC_SF_X_YINDEP    0x0020  /* Can do ScaleFreedom  X_YINDEPENDENT */
#define TC_SA_DOUBLE      0x0040  /* Can do ScaleAbility  DOUBLE       */
#define TC_SA_INTEGER     0x0080  /* Can do ScaleAbility  INTEGER      */
#define TC_SA_CONTIN      0x0100  /* Can do ScaleAbility  CONTINUOUS   */
#define TC_EA_DOUBLE      0x0200  /* Can do EmboldenAbility  DOUBLE     */
#define TC_IA_ABLE        0x0400  /* Can do ItalisizeAbility  ABLE     */
#define TC_UA_ABLE        0x0800  /* Can do UnderlineAbility  ABLE     */
#define TC_SO_ABLE        0x1000  /* Can do StrikeOutAbility  ABLE     */
#define TC_RA_ABLE        0x2000  /* Can do RasterFontAble  ABLE       */
#define TC_VA_ABLE        0x4000  /* Can do VectorFontAble  ABLE       */
#define TC_RESERVED       0x8000
#endif /* NOGDICAPMASKS */

/* Raster Capabilities */
#define RC_BITBLT         1       /* Can do standard BLT.    */
#define RC_BANDING        2        /* Device requires banding support  */
#define RC_SCALING        4        /* Device requires scaling support  */
#define RC_BITMAP64       8        /* Device can support >64K bitmap  */
#define  RC_GDI20_OUTPUT  0x0010  /* has 2.0 output calls     */
#define  RC_DI_BITMAP     0x0080  /* supports DIB to memory  */
#define  RC_PALETTE       0x0100  /* supports a palette    */
#define  RC_DIBTODEV      0x0200  /* supports DIBitsToDevice  */
#define  RC_BIGFONT       0x0400  /* supports >64K fonts    */
#define  RC_STRETCHBLT    0x0800  /* supports StretchBlt    */
#define  RC_FLOODFILL     0x1000  /* supports FloodFill    */
#define  RC_STRETCHDIB    0x2000  /* supports StretchDIBits  */

#endif /* NOGDI */

#ifndef NOKERNEL
UINT   FAR PASCAL AllocDStoCSAlias(UINT);
UINT   FAR PASCAL AllocCStoDSAlias(UINT);
UINT   FAR PASCAL AllocSelector(UINT);
UINT   FAR PASCAL ChangeSelector(UINT, UINT);
UINT   FAR PASCAL FreeSelector(UINT);
UINT   FAR PASCAL PrestoChangoSelector(UINT, UINT);
#endif /* NOKERNEL */

/*
  Help Engine stuff (for the MEWEL Toolbox)
*/
#define HELP_CONTEXT      0x0001   /* Display topic in ulTopic */
#define HELP_QUIT         0x0002   /* Terminate help */
#define HELP_INDEX        0x0003   /* Display index */
#define HELP_CONTENTS     0x0003
#define HELP_HELPONHELP   0x0004   /* Display help on using help */
#define HELP_SETINDEX     0x0005   /* Set the current Index for multi index help */
#define HELP_CONTEXTPOPUP 0x0008
#define HELP_FORCEFILE    0x0009
#define HELP_KEY          0x0101   /* Display topic for keyword in offabData */
#define HELP_COMMAND      0x0102
#define HELP_PARTIALKEY   0x0105
#define HELP_MULTIKEY     0x0201
#define HELP_SETWINPOS    0x0203
int FAR PASCAL WinHelp(HWND hwndMain, LPCSTR lpszHelp, UINT usCommand, DWORD ulData);


/****************************************************************************/
/*                                                                          */
/*                     Communications Stuff                                 */
/*                                                                          */
/****************************************************************************/
#ifndef NOCOMM
#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2

#define IGNORE              0       /* Ignore signal    */
#define INFINITE            0xFFFF  /* Infinite timeout */

/* Error Flags */

#define CE_RXOVER           0x0001  /* Receive Queue overflow       */
#define CE_OVERRUN          0x0002  /* Receive Overrun Error        */
#define CE_RXPARITY         0x0004  /* Receive Parity Error         */
#define CE_FRAME            0x0008  /* Receive Framing error        */
#define CE_BREAK            0x0010  /* Break Detected               */
#define CE_CTSTO            0x0020  /* CTS Timeout                  */
#define CE_DSRTO            0x0040  /* DSR Timeout                  */
#define CE_RLSDTO           0x0080  /* RLSD Timeout                 */
#define CE_TXFULL           0x0100  /* TX Queue is full             */
#define CE_PTO              0x0200  /* LPTx Timeout                 */
#define CE_IOE              0x0400  /* LPTx I/O Error               */
#define CE_DNS              0x0800  /* LPTx Device not selected     */
#define CE_OOP              0x1000  /* LPTx Out-Of-Paper            */
#define CE_MODE             0x8000  /* Requested mode unsupported   */

#define IE_BADID            (-1)    /* Invalid or unsupported id    */
#define IE_OPEN             (-2)    /* Device Already Open          */
#define IE_NOPEN            (-3)    /* Device Not Open              */
#define IE_MEMORY           (-4)    /* Unable to allocate queues    */
#define IE_DEFAULT          (-5)    /* Error in default parameters  */
#define IE_HARDWARE         (-10)   /* Hardware Not Present         */
#define IE_BYTESIZE         (-11)   /* Illegal Byte Size            */
#define IE_BAUDRATE         (-12)   /* Unsupported BaudRate         */
#define IE_NOTSUPPORTED     (-13)   /* Function not yet implemented */

/* Events */

#define EV_RXCHAR           0x0001  /* Any Character received       */
#define EV_RXFLAG           0x0002  /* Received certain character   */
#define EV_TXEMPTY          0x0004  /* Transmitt Queue Empty        */
#define EV_CTS              0x0008  /* CTS changed state            */
#define EV_DSR              0x0010  /* DSR changed state            */
#define EV_RLSD             0x0020  /* RLSD changed state           */
#define EV_BREAK            0x0040  /* BREAK received               */
#define EV_ERR              0x0080  /* Line status error occurred   */
#define EV_RING             0x0100  /* Ring signal detected         */
#define EV_PERR             0x0200  /* Printer error occured        */

/* Escape Functions */
#define SETXOFF             1       /* Simulate XOFF received       */
#define SETXON              2       /* Simulate XON received        */
#define SETRTS              3       /* Set RTS high                 */
#define CLRRTS              4       /* Set RTS low                  */
#define SETDTR              5       /* Set DTR high                 */
#define CLRDTR              6       /* Set DTR low                  */
#define RESETDEV            7       /* Reset device if possible     */


#if (WINVER >= 0x030a)

/* new escape functions */
#define GETMAXLPT           8
#define GETMAXCOM           9
#define GETBASEIRQ          10

/* Comm Baud Rate indices */
#define CBR_110      0xFF10
#define CBR_300      0xFF11
#define CBR_600      0xFF12
#define CBR_1200     0xFF13
#define CBR_2400     0xFF14
#define CBR_4800     0xFF15
#define CBR_9600     0xFF16
#define CBR_14400    0xFF17
#define CBR_19200    0xFF18
#define CBR_38400    0xFF1B
#define CBR_56000    0xFF1F
#define CBR_128000   0xFF23
#define CBR_256000   0xFF27

/* notifications passed in low word of lParam on WM_COMMNOTIFY messages */
#define CN_RECEIVE  0x0001
#define CN_TRANSMIT 0x0002
#define CN_EVENT    0x0004

#endif  /* WINVER >= 0x030a */

/*----------------------------------------------------------------*
*
* DCB  - main control interface structure
*
*----------------------------------------------------------------*/
typedef struct tagDCB
{
  BYTE Id;              /* Internal Device ID              */
  UINT BaudRate;        /* Baudrate at which runing        */
  BYTE ByteSize;        /* Number of bits/byte, 4-8        */
  BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
  BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
  UINT RlsTimeout;      /* Timeout for RLSD to be set      */
  UINT CtsTimeout;      /* Timeout for CTS to be set       */
  UINT DsrTimeout;      /* Timeout for DSR to be set       */

  UINT fBinary: 1;      /* Binary Mode (skip EOF check     */
  UINT fRtsDisable:1;   /* Don't assert RTS at init time   */
  UINT fParity: 1;      /* Enable parity checking          */
  UINT fOutxCtsFlow:1;  /* CTS handshaking on output       */
  UINT fOutxDsrFlow:1;  /* DSR handshaking on output       */
  UINT fDummy: 2;       /* Reserved                        */
  UINT fDtrDisable:1;   /* Don't assert DTR at init time   */

  UINT fOutX: 1;        /* Enable output X-ON/X-OFF        */
  UINT fInX: 1;         /* Enable input X-ON/X-OFF         */
  UINT fPeChar: 1;      /* Enable Parity Err Replacement   */
  UINT fNull: 1;        /* Enable Null stripping           */
  UINT fChEvt: 1;       /* Enable Rx character event.      */
  UINT fDtrflow: 1;     /* DTR handshake on input          */
  UINT fRtsflow: 1;     /* RTS handshake on input          */
  UINT fDummy2: 1;

  char XonChar;         /* Tx and Rx X-ON character        */
  char XoffChar;        /* Tx and Rx X-OFF character       */
  UINT XonLim;          /* Transmit X-ON threshold         */
  UINT XoffLim;         /* Transmit X-OFF threshold        */
  char PeChar;          /* Parity error replacement char   */
  char EofChar;         /* End of Input character          */
  char EvtChar;         /* Recieved Event character        */
  UINT TxDelay;         /* Amount of time between chars    */
} DCB;

/*----------------------------------------------------------------*
*
* COMSTAT  - status structure
*
*----------------------------------------------------------------*/
#if (defined(STRICT) || (WINVER >= 0x030a))
typedef struct tagCOMSTAT
{
  BYTE status;
  UINT cbInQue;
  UINT cbOutQue;
} COMSTAT;

#define CSTF_CTSHOLD    0x01
#define CSTF_DSRHOLD    0x02
#define CSTF_RLSDHOLD   0x04
#define CSTF_XOFFHOLD   0x08
#define CSTF_XOFFSENT   0x10
#define CSTF_EOF        0x20
#define CSTF_TXIM       0x40

#else   /* (STRICT || WINVER >= 0x030a) */

typedef struct tagCOMSTAT
{
  BYTE fCtsHold: 1;   /* Transmit is on CTS hold         */
  BYTE fDsrHold: 1;   /* Transmit is on DSR hold         */
  BYTE fRlsdHold: 1;  /* Transmit is on RLSD hold        */
  BYTE fXoffHold: 1;  /* Received handshake              */
  BYTE fXoffSent: 1;  /* Issued handshake                */
  BYTE fEof: 1;       /* End of file character found     */
  BYTE fTxim: 1;      /* Character being transmitted     */
  UINT cbInQue;       /* count of characters in Rx Queue */
  UINT cbOutQue;      /* count of characters in Tx Queue */
} COMSTAT;
#endif

int     WINAPI BuildCommDCB(LPCSTR, DCB FAR*);
int     WINAPI OpenComm(LPCSTR, UINT, UINT);
int     WINAPI CloseComm(int);
int     WINAPI ReadComm(int, void FAR*, int);
int     WINAPI WriteComm(int, const void FAR*, int);
int     WINAPI UngetCommChar(int, char);
int     WINAPI FlushComm(int, int);
int     WINAPI TransmitCommChar(int, char);
int     WINAPI SetCommState(const DCB FAR*);
int     WINAPI GetCommState(int, DCB FAR*);
int     WINAPI GetCommError(int, COMSTAT FAR* );
int     WINAPI SetCommBreak(int);
int     WINAPI ClearCommBreak(int);
UINT FAR* WINAPI SetCommEventMask(int, UINT);
UINT    WINAPI GetCommEventMask(int, int);
LONG    WINAPI EscapeCommFunction(int, int);
BOOL    WINAPI EnableCommNotification(int, HWND, int, int);

#endif /* NOCOMM */


BOOL    FAR PASCAL FillRgn(HDC, HRGN, HBRUSH);
BOOL    FAR PASCAL FrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL    FAR PASCAL InvertRgn(HDC, HRGN);
BOOL    FAR PASCAL PaintRgn(HDC, HRGN);
BOOL    FAR PASCAL PtInRegion(HRGN, int, int);

BOOL    FAR PASCAL CallMsgFilter(LPMSG, int);
VOID    FAR PASCAL FatalExit(int);
VOID    FAR PASCAL GetKeyboardState(BYTE FAR *);
BOOL    FAR PASCAL IsTwoByteCharPrefix(char);
VOID    FAR PASCAL SetKeyboardState(BYTE FAR *);
BOOL    WINAPI IsDBCSLeadByte(BYTE);

DWORD   WINAPI     GetAspectRatioFilter(HDC);
BOOL    WINAPI     GetAspectRatioFilterEx(HDC, SIZE FAR*);

/****************************************************************************/
/*                                                                          */
/*     Network-type stuff not supported in MEWEL....                        */
/*                                                                          */
/****************************************************************************/

UINT WINAPI WNetAddConnection(LPSTR, LPSTR, LPSTR);
UINT WINAPI WNetGetConnection(LPSTR, LPSTR, UINT FAR*);
UINT WINAPI WNetCancelConnection(LPSTR, BOOL);

/* Errors */
#define WN_SUCCESS                      0x0000
#define WN_NOT_SUPPORTED                0x0001
#define WN_NET_ERROR                    0x0002
#define WN_MORE_DATA                    0x0003
#define WN_BAD_POINTER                  0x0004
#define WN_BAD_VALUE                    0x0005
#define WN_BAD_PASSWORD                 0x0006
#define WN_ACCESS_DENIED                0x0007
#define WN_FUNCTION_BUSY                0x0008
#define WN_WINDOWS_ERROR                0x0009
#define WN_BAD_USER                     0x000A
#define WN_OUT_OF_MEMORY                0x000B
#define WN_CANCEL                       0x000C
#define WN_CONTINUE                     0x000D

/* Connection errors */
#define WN_NOT_CONNECTED                0x0030
#define WN_OPEN_FILES                   0x0031
#define WN_BAD_NETNAME                  0x0032
#define WN_BAD_LOCALNAME                0x0033
#define WN_ALREADY_CONNECTED            0x0034
#define WN_DEVICE_ERROR                 0x0035
#define WN_CONNECTION_CLOSED            0x0036


/****************************************************************************/
/*                                                                          */
/*     KERNEL-type stuff not supported in MEWEL....                         */
/*                                                                          */
/****************************************************************************/

typedef struct tagSEGINFO
{
  UINT offSegment;
  UINT cbSegment;
  UINT flags;
  UINT cbAlloc;
  HGLOBAL h;
  UINT alignShift;
  UINT reserved[2];
} SEGINFO;
typedef SEGINFO FAR* LPSEGINFO;
VOID WINAPI GetCodeInfo(FARPROC, SEGINFO FAR *);

/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct tagWINDEBUGINFO
{
  UINT    flags;
  DWORD   dwOptions;
  DWORD   dwFilter;
  char    achAllocModule[8];
  DWORD   dwAllocBreak;
  DWORD   dwAllocCount;
} WINDEBUGINFO;
VOID    FAR PASCAL DebugBreak();

BOOL    FAR PASCAL FreeModule(HANDLE);
HANDLE  FAR PASCAL GetCodeHandle(FARPROC);
UINT    FAR PASCAL GetCurrentPDB(VOID);
VOID    FAR PASCAL LimitEmsPages(DWORD);
HINSTANCE FAR PASCAL LoadModule(LPCSTR, LPVOID);
UINT    FAR PASCAL SetHandleCount(UINT);
LONG    FAR PASCAL SetSwapAreaSize(UINT);
VOID    FAR PASCAL SwapRecording(UINT);
VOID    FAR PASCAL SwitchStackBack();
VOID    FAR PASCAL SwitchStackTo(UINT, UINT, UINT);
VOID    FAR PASCAL ValidateCodeSegments(VOID);
VOID    FAR PASCAL ValidateFreeSpaces(VOID);


/****************************************************************************/
/*                                                                          */
/*     DRIVER-type stuff not supported in MEWEL....                         */
/*                                                                          */
/****************************************************************************/
#ifndef NODRIVERS

DECLARE_HANDLE(HDRVR);

typedef LRESULT (CALLBACK* DRIVERPROC)(DWORD, HDRVR, UINT, LPARAM, LPARAM);

/* Driver messages */
#define DRV_LOAD                0x0001
#define DRV_ENABLE              0x0002
#define DRV_OPEN                0x0003
#define DRV_CLOSE               0x0004
#define DRV_DISABLE             0x0005
#define DRV_FREE                0x0006
#define DRV_CONFIGURE           0x0007
#define DRV_QUERYCONFIGURE      0x0008
#define DRV_INSTALL             0x0009
#define DRV_REMOVE              0x000A
#define DRV_EXITSESSION         0x000B
#define DRV_EXITAPPLICATION     0x000C
#define DRV_POWER               0x000F

#define DRV_RESERVED            0x0800
#define DRV_USER                0x4000

/* LPARAM of DRV_CONFIGURE message */
typedef struct tagDRVCONFIGINFO
{
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
} DRVCONFIGINFO;
typedef DRVCONFIGINFO NEAR* PDRVCONFIGINFO;
typedef DRVCONFIGINFO FAR* LPDRVCONFIGINFO;

/* Supported return values for DRV_CONFIGURE message */
#define DRVCNF_CANCEL           0x0000
#define DRVCNF_OK               0x0001
#define DRVCNF_RESTART          0x0002

/* Supported lParam1 of DRV_EXITAPPLICATION notification */
#define DRVEA_NORMALEXIT            0x0001
#define DRVEA_ABNORMALEXIT          0x0002

LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR driverID, UINT message, LPARAM lParam1, LPARAM lParam2);

HDRVR   WINAPI OpenDriver(LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2);
LRESULT WINAPI CloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2);

LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT message, LPARAM lParam1, LPARAM lParam2);

HINSTANCE WINAPI GetDriverModuleHandle(HDRVR hDriver);

HDRVR   WINAPI GetNextDriver(HDRVR, DWORD);

/* GetNextDriver flags */
#define GND_FIRSTINSTANCEONLY   0x00000001

#define GND_FORWARD             0x00000000
#define GND_REVERSE             0x00000002

typedef struct tagDRIVERINFOSTRUCT
{
    UINT    length;
    HDRVR   hDriver;
    HINSTANCE hModule;
    char    szAliasName[128];
} DRIVERINFOSTRUCT;
typedef DRIVERINFOSTRUCT FAR* LPDRIVERINFOSTRUCT;

BOOL    WINAPI GetDriverInfo(HDRVR, DRIVERINFOSTRUCT FAR*);

#endif  /* !NODRIVERS */


/****************************************************************************/
/*                                                                          */
/*                WINDOWS 3.1 specific definitions                          */
/*                                                                          */
/****************************************************************************/

#if (WINVER >= 0x030a)
BOOL WINAPI EnableScrollBar(HWND hWnd, int nBar, UINT flags);
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003
#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002
#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002
#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT

HDC WINAPI GetDCEx(HWND hwnd, HRGN hrgnClip, DWORD flags);
#define DCX_WINDOW          0x00000001L
#define DCX_CACHE           0x00000002L
#define DCX_CLIPCHILDREN    0x00000008L
#define DCX_CLIPSIBLINGS    0x00000010L
#define DCX_PARENTCLIP      0x00000020L
#define DCX_EXCLUDERGN      0x00000040L
#define DCX_INTERSECTRGN    0x00000080L
#define DCX_LOCKWINDOWUPDATE 0x00000400L
#define DCX_USESTYLE        0x00010000L

BOOL WINAPI LockWindowUpdate(HWND hwndLock);

VOID WINAPI MapWindowPoints(HWND hwndFrom, HWND hwndTo, 
                            POINT FAR* lppt, UINT cpt);

BOOL WINAPI QueryAbort(HDC, int);

BOOL WINAPI RedrawWindow(HWND hwnd, CONST RECT FAR* lprcUpdate, 
                         HRGN hrgnUpdate, UINT flags);
#define RDW_INVALIDATE     0x0001
#define RDW_INTERNALPAINT  0x0002
#define RDW_ERASE          0x0004
#define RDW_VALIDATE       0x0008
#define RDW_NOINTERNALPAINT  0x0010
#define RDW_NOERASE        0x0020
#define RDW_NOCHILDREN     0x0040
#define RDW_ALLCHILDREN    0x0080
#define RDW_UPDATENOW      0x0100
#define RDW_ERASENOW       0x0200
#define RDW_FRAME          0x0400
#define RDW_NOFRAME        0x0800

int WINAPI ScrollWindowEx(HWND hwnd, int dx, int dy,
                          CONST RECT FAR* prcScroll, CONST RECT FAR* prcClip,
                          HRGN hrgnUpdate, RECT FAR* prcUpdate, UINT flags);
#define SW_SCROLLCHILDREN   0x0001
#define SW_INVALIDATE       0x0002
#define SW_ERASE            0x0004


UINT WINAPI SetBoundsRect(HDC hDC, CONST RECT FAR* lprcBounds, UINT flags);
UINT WINAPI GetBoundsRect(HDC hDC, RECT FAR* lprcBounds, UINT flags);

/* WM_WINDOWPOSCHANGING/CHANGED struct pointed to by lParam */
typedef struct tagWINDOWPOS
{
  HWND    hwnd;
  HWND    hwndInsertAfter;
  int     x;
  int     y;
  int     cx;
  int     cy;
  UINT    flags;
} WINDOWPOS, FAR* LPWINDOWPOS;


/* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP       0x0010
#define WVR_ALIGNLEFT      0x0020
#define WVR_ALIGNBOTTOM    0x0040
#define WVR_ALIGNRIGHT     0x0080
#define WVR_HREDRAW        0x0100
#define WVR_VREDRAW        0x0200
#define WVR_REDRAW         (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS     0x0400

/* WM_NCCALCSIZE parameter structure */
typedef struct tagNCCALCSIZE_PARAMS
{
  RECT     rgrc[3];
  WINDOWPOS FAR* lppos;
} NCCALCSIZE_PARAMS, FAR* LPNCCALCSIZE_PARAMS;

/* Struct pointed to by WM_GETMINMAXINFO lParam */
typedef struct tagMINMAXINFO
{
  POINT ptReserved;
  POINT ptMaxSize;
  POINT ptMaxPosition;
  POINT ptMinTrackSize;
  POINT ptMaxTrackSize;
} MINMAXINFO;



#ifndef NOSCALABLEFONT
DWORD WINAPI GetFontData(HDC, DWORD, DWORD, VOID FAR*, DWORD);
BOOL  WINAPI CreateScalableFontResource(UINT, LPCSTR, LPCSTR, LPCSTR);

typedef struct tagGLYPHMETRICS
{
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    int     gmCellIncX;
    int     gmCellIncY;
} GLYPHMETRICS, FAR* LPGLYPHMETRICS;

typedef struct tagFIXED
{
    UINT    fract;
    int     value;
} FIXED, FAR* LPFIXED;

typedef struct tagMAT2
{
    FIXED  eM11;
    FIXED  eM12;
    FIXED  eM21;
    FIXED  eM22;
} MAT2, FAR* LPMAT2;

DWORD WINAPI GetGlyphOutline(HDC hDC, UINT uChar, UINT fuFormat, 
                             GLYPHMETRICS FAR* lpgm, DWORD cbBuffer,
                             VOID FAR *lpBuffer, CONST MAT2 FAR* lpMat2);
/* GetGlyphOutline constants */
#define GGO_METRICS        0
#define GGO_BITMAP         1
#define GGO_NATIVE         2

#define TT_POLYGON_TYPE   24

#define TT_PRIM_LINE       1
#define TT_PRIM_QSPLINE    2

typedef struct tagPOINTFX
{
    FIXED x;
    FIXED y;
} POINTFX, FAR* LPPOINTFX;

typedef struct tagTTPOLYCURVE
{
    UINT    wType;
    UINT    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE, FAR* LPTTPOLYCURVE;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER, FAR* LPTTPOLYGONHEADER;

typedef struct tagABC
{
  int   abcA;
  UINT  abcB;
  int   abcC;
} ABC;
typedef ABC FAR* LPABC;

BOOL WINAPI GetCharABCWidths(HDC hDC, UINT chFirst, UINT chLast, ABC FAR* lpabc);


typedef struct tagKERNINGPAIR
{
  WORD wFirst;
  WORD wSecond;
  int  iKernAmount;
} KERNINGPAIR, FAR* LPKERNINGPAIR;
int WINAPI GetKerningPairs(HDC hDC, int nPairs, KERNINGPAIR FAR* lpkp);

typedef struct tagRASTERIZER_STATUS
{
    int   nSize;
    int   wFlags;
    int   nLanguageID;
} RASTERIZER_STATUS;
typedef RASTERIZER_STATUS FAR* LPRASTERIZER_STATUS;

/* bits defined in wFlags of RASTERIZER_STATUS */
#define TT_AVAILABLE  0x0001
#define TT_ENABLED  0x0002

BOOL WINAPI GetRasterizerCaps(RASTERIZER_STATUS FAR* lprs, int cb);

typedef struct tagPANOSE
{
  BYTE    bFamilyType;
  BYTE    bSerifStyle;
  BYTE    bWeight;
  BYTE    bProportion;
  BYTE    bContrast;
  BYTE    bStrokeVariation;
  BYTE    bArmStyle;
  BYTE    bLetterform;
  BYTE    bMidline;
  BYTE    bXHeight;
} PANOSE, FAR* LPPANOSE;

typedef struct tagOUTLINETEXTMETRIC
{
  UINT    otmSize;
  TEXTMETRIC otmTextMetrics;
  BYTE    otmFiller;
  PANOSE  otmPanoseNumber;
  UINT    otmfsSelection;
  UINT    otmfsType;
  int     otmsCharSlopeRise;
  int     otmsCharSlopeRun;
  int     otmItalicAngle;
  UINT    otmEMSquare;
  int     otmAscent;
  int     otmDescent;
  UINT    otmLineGap;
  UINT    otmsCapEmHeight;
  UINT    otmsXHeight;
  RECT    otmrcFontBox;
  int     otmMacAscent;
  int     otmMacDescent;
  UINT    otmMacLineGap;
  UINT    otmusMinimumPPEM;
  POINT   otmptSubscriptSize;
  POINT   otmptSubscriptOffset;
  POINT   otmptSuperscriptSize;
  POINT   otmptSuperscriptOffset;
  UINT    otmsStrikeoutSize;
  int     otmsStrikeoutPosition;
  int     otmsUnderscorePosition;
  int     otmsUnderscoreSize;
  PSTR    otmpFamilyName;
  PSTR    otmpFaceName;
  PSTR    otmpStyleName;
  PSTR    otmpFullName;
} OUTLINETEXTMETRIC, FAR* LPOUTLINETEXTMETRIC;

UINT WINAPI GetOutlineTextMetrics(HDC hDC, UINT cbData, 
                                  OUTLINETEXTMETRIC FAR* lpotm);
#endif  /* NOSCALABLEFONT */

UINT WINAPI GetFreeSystemResources(UINT fuSysResource);
#define GFSR_SYSTEMRESOURCES   0x0000
#define GFSR_GDIRESOURCES      0x0001
#define GFSR_USERRESOURCES     0x0002

#endif  /* WINVER >= 0x030a */

#if defined(STRICT)
int     PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
int FAR PASCAL LibMain(HINSTANCE, WORD, WORD, LPSTR);
#else
int     PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
int FAR PASCAL LibMain(HANDLE, WORD, WORD, LPSTR);
#endif
int CALLBACK   WEP(int);
#define WEP_SYSTEM_EXIT 1
#define WEP_FREE_DLL    0


/*===========================================================================*/
/*                                                                           */
/* Things which are needed in MEWEL but which are not in Windows' windows.h  */
/*                                                                           */
/*===========================================================================*/

/*
  Windows compatibility flags
*/
VOID FAR PASCAL SetWindowsCompatibility(UINT);
UINT FAR PASCAL TestWindowsCompatibility(UINT);
#define WC_SEND_WMCTLCOLOR   0x0010
#define WC_MAXCOMPATIBILITY  0xFFFF

/*
  Define this if you want the extra code associated with bitmaps
  in menus (and the mdi system menu)
*/
#if 0
#define USE_BITMAPS_IN_MENUS
#endif

/*
  The initial graphics mode. The app can set this before WinInit() in
  order to control the video mode which the GUI starts up in. The
  value -1 means to default to the maximum resolution.
*/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
extern INT MEWELInitialGraphicsMode;
#endif

/*
  This is an undocumented variable which you can set to TRUE (before doing
  your WinInit() call) in order to disable MEWEL from intercepting
  interrupt 9H. The only capability you will lose is the ability to tap
  on the ALT key in order to bring you up to the menu bar.
*/
extern BYTE MEWEL_NoInt9;


/*
  MEWELmain definitions
*/
#if defined(XWINDOWS)

#define MEWELmain(hInstance) \
main(int argc, char **argv) \
{ \
  char achCmdLine[512]; \
  int  i; \
  HINSTANCE _hInst; \
  extern int _Xargc; \
  extern char **_Xargv; \
  _Xargc = argc;  _Xargv = argv; \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  MDIInitialize(); \
  _hInst = OpenResourceFile(argv[0]); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}

#elif defined(UNIX)
#define MEWELmain(hInstance) \
UINT Int24Err = 0;
main(int argc, char **argv) \
{ \
  char achCmdLine[512]; \
  int  i; \
  HINSTANCE _hInst; \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  MDIInitialize(); \
  _hInst = OpenResourceFile(argv[0]); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}

#elif defined(__WATCOMC__)
#define MEWELmain(hInstance) \
extern int main(int argc, char **argv); \
main(int argc, char **argv) \
{ \
  extern char **_argv; \
  extern char *_LpPgmName; \
  char achCmdLine[128]; \
  char szResFile[128]; \
  int  i; \
  HINSTANCE _hInst; \
  (void) argc;  (void) argv; \
  strcpy(szResFile, _LpPgmName); \
  strcpy(strrchr(szResFile, '.'), ".RES"); \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  _hInst = OpenResourceFile(szResFile); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}

#elif defined(PLTNT) && defined(MSC)
#define MEWELmain(hInstance) \
extern int main(int argc, char **argv); \
main(int argc, char **argv) \
{ \
  extern char **__argv; \
  char achCmdLine[128]; \
  char szResFile[128]; \
  int  i; \
  HINSTANCE _hInst; \
  (void) argc;  (void) argv; \
  strcpy(szResFile, __argv[0]); \
  strcpy(strrchr(szResFile, '.'), ".RES"); \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  _hInst = OpenResourceFile(szResFile); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}


#elif defined(NOMDI)
#define MEWELmain(hInstance) \
extern int main(int argc, char **argv); \
main(int argc, char **argv) \
{ \
  char achCmdLine[128]; \
  int  i; \
  HINSTANCE _hInst; \
  (void) argc;  (void) argv; \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  _hInst = OpenResourceFile(NULL); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}

#else
#define MEWELmain(hInstance) \
extern int main(int argc, char **argv); \
main(int argc, char **argv) \
{ \
  char achCmdLine[128]; \
  int  i; \
  HINSTANCE _hInst = 0; \
  (void) argc;  (void) argv; \
  achCmdLine[0] = '\0'; \
  for (i = 1;  i < argc;  i++) \
  { \
    lstrcat(achCmdLine, argv[i]); \
    if (i < argc-1) \
    lstrcat(achCmdLine, " "); \
  } \
  WinInit(); \
  WinUseSysColors(NULL, TRUE); \
  MDIInitialize(); \
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY); \
  _hInst = OpenResourceFile(NULL); \
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL); \
}
#endif


/*--------------------------------------------------------------------------*/
/*
  Include internal header files only if MEWEL itself is being recompiled.
*/
#ifdef _WPRIVATE_H
#include "winternl.h"
#if defined(XWINDOWS)
#include "xmewpriv.h"
#endif
#endif

/*
  Include most of the declarations
*/
#include "windecls.h"

#ifdef __cplusplus
}
#endif

#endif /* __WINDOW_H */
#endif /* RC_INVOKED */

/*
  If MEWELMAIN is defined, then the default main() function is included
  in the module.
*/
#ifdef MEWELMAIN
MEWELmain(0)
#endif


/****************************************************************************/
/*                                                                          */
/* Everything below here are constants meant to be used in RC files         */
/* by including "windows.h".                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
#define WM_USER        0x0400

#ifndef MEWEL
#define MEWEL            1
#endif

/* Basic window types */
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L

/* Clipping styles */
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_CLIP             WS_CLIPCHILDREN | WS_CLIPSIBLINGS

/* Generic window states */
#define WS_VISIBLE          0x10000000L
#define WS_DISABLED         0x08000000L

/* Main window states */
#define WS_MINIMIZE         0x20000000L
#define WS_MAXIMIZE         0x01000000L

/* Main window styles */
#define WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L
#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L

/* Control window styles */
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L

/* MEWEL specific styles */
#define WS_SYSCOLOR         0x00002000L
#define WS_SHADOW           0x00004000L
#define WS_HIDDEN           0x00008000L

/* Common Window Styles */
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW      (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW      (WS_CHILD)

/* Extended Window Styles */
#define WS_EX_DLGMODALFRAME  0x00000001L
#define WS_EX_NOPARENTNOTIFY 0x00000004L

#define WS_EX_TOPMOST        0x00000008L
#define WS_EX_ACCEPTFILES    0x00000010L
#define WS_EX_TRANSPARENT    0x00000020L

/* Obsolete style names */
#define WS_TILED             WS_OVERLAPPED
#define WS_ICONIC            WS_MINIMIZE
#define WS_SIZEBOX           WS_THICKFRAME
#define WS_TILEDWINDOW       WS_OVERLAPPEDWINDOW

/* Static Control Styles */
/* MEWEL-specific */
#define SS_TEXT             0x00000200L
#define SS_FRAME            0x00000400L
#define SS_BOX              0x00000800L

#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L | SS_BOX
#define SS_GRAYRECT         0x00000005L | SS_BOX
#define SS_WHITERECT        0x00000006L | SS_BOX
#define SS_BLACKFRAME       0x00000007L | SS_FRAME
#define SS_GRAYFRAME        0x00000008L | SS_FRAME
#define SS_WHITEFRAME       0x00000009L | SS_FRAME
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_NOPREFIX         0x00000080L


/* Button Control Styles */
#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON      0x00000004L
#define BS_3STATE           0x00000005L
#define BS_AUTO3STATE       0x00000006L
#define BS_GROUPBOX         0x00000007L
#define BS_USERBUTTON       0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW        0x0000000BL
#define BS_LEFTTEXT         0x00000020L

/* MEWEL-specific */
#define BS_INVERTED         0x00000080L
#define BS_HELP             0x00000040L

/* Edit control styles */
#define ES_LEFT             0x00000000L
#define ES_CENTER           0x00000001L
#define ES_RIGHT            0x00000002L
#define ES_MULTILINE        0x00000004L
#define ES_UPPERCASE        0x00000008L
#define ES_LOWERCASE        0x00000010L
#define ES_PASSWORD         0x00000020L
#define ES_AUTOVSCROLL      0x00000040L
#define ES_AUTOHSCROLL      0x00000080L
#define ES_NOHIDESEL        0x00000100L
#define ES_OEMCONVERT       0x00000400L
#define ES_READONLY         0x00000800L
#define ES_WANTRETURN       0x00001000L

/* Scroll bar styles */
#define SBS_HORZ            0x0000L
#define SBS_VERT            0x0001L
#define SBS_TOPALIGN        0x0002L
#define SBS_LEFTALIGN       0x0002L
#define SBS_BOTTOMALIGN     0x0004L
#define SBS_RIGHTALIGN      0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX         0x0008L
/* MEWEL-specific */
#define SBS_CTL             0x0010L

/* Listbox styles */
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_STANDARD        (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Combo box styles */
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L

/* Dialog styles */
#define DS_ABSALIGN           0x01L
#define DS_SYSMODAL           0x02L
#define DS_LOCALEDIT          0x20L
#define DS_SETFONT            0x40L
#define DS_MODALFRAME         0x80L
#define DS_NOIDLEMSG          0x100L

/*
  Some macros to make style testing easier
*/
#define HAS_CAPTION(ulStyle)  (((ulStyle) & WS_CAPTION) == WS_CAPTION)
#define HAS_BORDER(ulStyle)   ((ulStyle) & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME))

/*
  Scrollbar access constants
*/
#define SB_HORZ               0
#define SB_VERT               1
#define SB_CTL                2
#define SB_BOTH               3

/*
   WM_SYSCOMMAND messages
*/
#define SC_SIZE        0xF000
#define SC_MOVE        0xF010
#define SC_MINIMIZE    0xF020
#define SC_MAXIMIZE    0xF030
#define SC_NEXTWINDOW  0xF040
#define SC_PREVWINDOW  0xF050
#define SC_CLOSE       0xF060
#define SC_CLOSEMDI    (SC_CLOSE+1)
#define SC_VSCROLL     0xF070
#define SC_HSCROLL     0xF080
#define SC_MOUSEMENU   0xF090
#define SC_KEYMENU     0xF100
#define SC_ARRANGE     0xF110
#define SC_RESTORE     0xF120
#define SC_TASKLIST    0xF130
#define SC_SCREENSAVE  0xF140
#define SC_HOTKEY      0xF150
#define SC_ICON        SC_MINIMIZE
#define SC_ZOOM        SC_MAXIMIZE


/*===========================================================================*/
/*                                                                           */
/*                       MESSAGE BOX INFORMATION                             */
/*                                                                           */
/*===========================================================================*/
#ifndef RC_INVOKED
extern BOOL bMessageBoxHasShadow;
#endif

#define IDOK      1
#define IDCANCEL  2
#define IDABORT   3
#define IDRETRY   4
#define IDIGNORE  5
#define IDYES     6
#define IDNO      7
#ifndef OWL
#ifndef AFX
#define IDHELP    8
#endif
#endif

#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004
#define MB_RETRYCANCEL      0x0005
#define MB_TYPEMASK         0x000F

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONMASK         0x00F0

#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP         MB_ICONHAND

#define MB_DEFBUTTON1       0x0000
#define MB_DEFBUTTON2       0x0100
#define MB_DEFBUTTON3       0x0200
#define MB_DEFMASK          0x0F00

#define MB_APPLMODAL        0x0000
#define MB_SYSTEMMODAL      0x1000
#define MB_TASKMODAL        0x2000

#define MB_NOFOCUS          0x8000

/*
  Defines for INTERNATIONAL_MEWEL strings in the resource file. The unusual
  numbers will help avoiding conflicts with user-defined stringtable entries
*/
#define INTL_MENU_FIRST               INTL_RESTORE
#define INTL_RESTORE                  0xE510
#define INTL_MOVE                     0xE511
#define INTL_SIZE                     0xE512
#define INTL_MINIMIZE                 0xE513
#define INTL_MAXIMIZE                 0xE514
#define INTL_CLOSE                    0xE515
#define INTL_CLOSEMDI                 0xE516
#define INTL_MENU_COUNT               0x07

#define INTL_BUTTON_FIRST             INTL_OK
#define INTL_OK                       0xE520
#define INTL_CANCEL                   0xE521
#define INTL_ABORT                    0xE522
#define INTL_RETRY                    0xE523
#define INTL_IGNORE                   0xE524
#define INTL_YES                      0xE525
#define INTL_NO                       0xE526
#define INTL_HELP                     0xE527
#define INTL_BUTTON_COUNT             0x08

#define INTL_SYSSTR_FIRST             INTL_OUT_OF_MEMORY
#define INTL_OUT_OF_MEMORY            0xE530
#define INTL_ERROR                    0xE531
#define INTL_SYSSTR_COUNT             0x02

#define INTL_INT24_FIRST              INTL_WRITE_PROTECT_ERR
#define INTL_WRITE_PROTECT_ERR        0xE540
#define INTL_UNKNOWN_UNIT             0xE541
#define INTL_DRIVE_NOT_READY          0xE542
#define INTL_UNKNOWN_CMD              0xE543
#define INTL_BAD_CRC_ERR              0xE544
#define INTL_BAD_REQ_HDR_LENGTH       0xE545
#define INTL_SEEK_ERR                 0xE546
#define INTL_UNKNOWN_MEDIA            0xE547
#define INTL_SECTOR_NOT_FOUND         0xE548
#define INTL_OUT_OF_PAPER             0xE549
#define INTL_WRITE_FAULT              0xE54A
#define INTL_READ_FAULT               0xE54B
#define INTL_GENERAL_FAILURE          0xE54C
#define INTL_SHARING_VIOLATION        0xE54D
#define INTL_LOCK_VIOLATION           0xE54E
#define INTL_INVALID_DISK_CHANGE      0xE54F
#define INTL_FCB_UNAVAILABLE          0xE550
#define INTL_SHARING_BUF_OVERFLOW     0xE551
#define INTL_CODE_PAGE_MISMATCH       0xE552
#define INTL_OUT_OF_INPUT             0xE553
#define INTL_INSUFF_DISK_SPACE        0xE554
#define INTL_ON_DRIVE                 0xE555
#define INTL_ON_DEVICE                0xE556
#define INTL_INT24_COUNT              0x17

#define INTL_EXEC_FIRST               INTL_ENTER_EXIT_TO_RETURN
#define INTL_ENTER_EXIT_TO_RETURN     0xE560
#define INTL_PRESS_ANY_KEY            0xE561
#define INTL_SWAP_FILE_ERR            0xE562
#define INTL_NO_ROOM_FOR_SWAPPING     0xE563
#define INTL_CANT_ALLOC_ENV_SPACE     0xE564
#define INTL_DOS_EXEC_ERROR           0xE565
#define INTL_INTERNAL_EXEC_ERROR      0xE566
#define INTL_EXEC_COUNT               0x07
#define INTL_COMMFILE_FIRST           INTL_CREATEPROMPT
#define INTL_CREATEPROMPT             0xE570
#define INTL_OVERWRITEPROMPT          0xE571
#define INTL_COMMFILE_COUNT           0x02

/****************************************************************************/
/*                Colors (BIOS values 0-16)                                 */
/****************************************************************************/
#ifdef RC_INVOKED
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define YELLOW          6
#define WHITE           7
#define INTENSE_BLACK   8
#define INTENSE_BLUE    9
#define INTENSE_GREEN   10
#define INTENSE_CYAN    11
#define INTENSE_RED     12
#define INTENSE_MAGENTA 13
#define INTENSE_YELLOW  14
#define INTENSE_WHITE   15
#endif /* RC_INVOKED */


/****************************************************************************/
/*                         Keystroke values                                 */
/****************************************************************************/

#ifndef RC_INVOKED
/*
  SHIFT states
*/
#define UNSHIFTED   ((WORD) 0x00)
#define RIGHT_SHIFT ((WORD) 0x01)
#define LEFT_SHIFT  ((WORD) 0x02)
#define SHIFT_SHIFT ((WORD) 0x03)
#define CTL_SHIFT   ((WORD) 0x04)
#define ALT_SHIFT   ((WORD) 0x08)
#define INS_SHIFT   ((WORD) 0x80)
#define CAP_SHIFT   ((WORD) 0x40)
#define NUM_SHIFT   ((WORD) 0x20)
#define SCL_SHIFT   ((WORD) 0x10)
#define TOGGLE_SHIFT_MASK (INS_SHIFT | CAP_SHIFT | SCL_SHIFT | NUM_SHIFT)

#define VK_CTRL_BREAK     256   /* MEWEL-specific code for CTRL-BREAK */

#define MAKEKEY(scan, shift)  ((UINT) ((scan) * 256u + (shift)))
#endif /* RC_INVOKED */


#ifdef MOTIF
#define USE_WINDOWS_COMPAT_KEYS
#endif

#ifdef USE_WINDOWS_COMPAT_KEYS
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_CANCEL           0x03
#define VK_MBUTTON          0x04
#define VK_BACK             0x08
#define VK_TAB              0x09
#define VK_CLEAR            0x0C
#define VK_RETURN           0x0D
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
#define VK_ESCAPE           0x1B
#define VK_SPACE            0x20
#define VK_PRIOR            0x21
#define VK_NEXT             0x22
#define VK_END              0x23
#define VK_HOME             0x24
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_SELECT           0x29
#define VK_PRINT            0x2A
#define VK_EXECUTE          0x2B
#define VK_SNAPSHOT         0x2C
#define VK_INSERT           0x2D
#define VK_DELETE           0x2E
#define VK_HELP             0x2F
#define VK_NUMPAD0          0x60
#define VK_NUMPAD1          0x61
#define VK_NUMPAD2          0x62
#define VK_NUMPAD3          0x63
#define VK_NUMPAD4          0x64
#define VK_NUMPAD5          0x65
#define VK_NUMPAD6          0x66
#define VK_NUMPAD7          0x67
#define VK_NUMPAD8          0x68
#define VK_NUMPAD9          0x69
#define VK_MULTIPLY         0x6A
#define VK_ADD              0x6B
#define VK_SEPARATOR        0x6C
#define VK_SUBTRACT         0x6D
#define VK_DECIMAL          0x6E
#define VK_DIVIDE           0x6F
#define VK_F1               0x70
#define VK_F2               0x71
#define VK_F3               0x72
#define VK_F4               0x73
#define VK_F5               0x74
#define VK_F6               0x75
#define VK_F7               0x76
#define VK_F8               0x77
#define VK_F9               0x78
#define VK_F10              0x79
#define VK_F11              0x7A
#define VK_F12              0x7B
#define VK_F13              0x7C
#define VK_F14              0x7D
#define VK_F15              0x7E
#define VK_F16              0x7F
#define VK_F17              0x80
#define VK_F18              0x81
#define VK_F19              0x82
#define VK_F20              0x83
#define VK_F21              0x84
#define VK_F22              0x85
#define VK_F23              0x86
#define VK_F24              0x87
#define VK_NUMLOCK          0x90
#define VK_SCROLL           0x91

#else

#define VK_HOME            0x4700
#define VK_SH_HOME         0x4703
#define VK_CTRL_HOME       0x7704
#define VK_ALT_HOME        0x9708

#define VK_END             0x4f00
#define VK_SH_END          0x4f03
#define VK_CTRL_END        0x7504
#define VK_ALT_END         0x9f08

#define VK_PGUP            0x4900
#define VK_SH_PGUP         0x4903
#define VK_CTRL_PGUP       0x8404
#define VK_ALT_PGUP        0x9908

#define VK_PGDN            0x5100
#define VK_SH_PGDN         0x5103
#define VK_CTRL_PGDN       0x7604
#define VK_ALT_PGDN        0xa108

#define VK_LEFT            0x4B00
#define VK_SH_LEFT         0x4B03
#define VK_CTRL_LEFT       0x7304
#define VK_ALT_LEFT        0x9b08

#define VK_RIGHT           0x4d00
#define VK_SH_RIGHT        0x4d03
#define VK_CTRL_RIGHT      0x7404
#define VK_ALT_RIGHT       0x9d08

#define VK_UP              0x4800
#define VK_SH_UP           0x4803
#define VK_CTRL_UP         0x8d04
#define VK_ALT_UP          0x9808

#define VK_DOWN            0x5000
#define VK_SH_DOWN         0x5003
#define VK_CTRL_DOWN       0x9104
#define VK_ALT_DOWN        0xa008

#define VK_INS             0x5200
#define VK_INSERT          0x5200
#define VK_SH_INS          0x5203
#define VK_CTRL_INS        0x9204
#define VK_ALT_INS         0xa208

#define VK_DEL             0x5300
#define VK_DELETE          0x5300
#define VK_SH_DEL          0x5303
#define VK_CTRL_DEL        0x9304
#define VK_ALT_DEL         0xa308

#define VK_FIVE            0x4c00
#define VK_SH_FIVE         0x4c03
#define VK_CTRL_FIVE       0x8f04
#define VK_ALT_FIVE        0x4c08

#define VK_BACKSPACE       8
#define VK_SH_BACKSPACE    0x0e03
#define VK_CTRL_BACKSPACE  127
#define VK_ALT_BACKSPACE   0x0e08

#define VK_TAB             0x0009
#define VK_SH_TAB          0x0f03
#define VK_CTRL_TAB        0x9404
#define VK_ALT_TAB         0xa508

#define VK_ENTER           '\r'
#define VK_SH_ENTER        '\r'
#define VK_CTRL_ENTER      '\n'
#define VK_ALT_ENTER       0xa608

#define VK_GREYPLUS        0x4e00
#define VK_SH_GREYPLUS     0x4e03
#define VK_CTRL_GREYPLUS   0x9004
#define VK_ALT_GREYPLUS    0x4e08

#define VK_GREYMINUS       0x4a00
#define VK_SH_GREYMINUS    0x4a03
#define VK_CTRL_GREYMINUS  0x8e04
#define VK_ALT_GREYMINUS   0x4a08

#define VK_GREYSTAR        0x3700
#define VK_SH_GREYSTAR     0x3703
#define VK_CTRL_GREYSTAR   0x9604
#define VK_ALT_GREYSTAR    0x3708

#define VK_GREYDIVIDE      '/'
#define VK_SH_GREYDIVIDE   '/'
#define VK_CTRL_GREYDIVIDE 0x9504
#define VK_ALT_GREYDIVIDE  0xa408

#define VK_ALT_1           0x7808
#define VK_ALT_2           0x7908
#define VK_ALT_3           0x7a08
#define VK_ALT_4           0x7b08
#define VK_ALT_5           0x7c08
#define VK_ALT_6           0x7d08
#define VK_ALT_7           0x7e08
#define VK_ALT_8           0x7f08
#define VK_ALT_9           0x8008
#define VK_ALT_0           0x8108
#define VK_ALT_MINUS       0x8208
#define VK_ALT_PLUS        0x8308

#define VK_ALT_A           0x1e08
#define VK_ALT_B           0x3008
#define VK_ALT_C           0x2e08
#define VK_ALT_D           0x2008
#define VK_ALT_E           0x1208
#define VK_ALT_F           0x2108
#define VK_ALT_G           0x2208
#define VK_ALT_H           0x2308
#define VK_ALT_I           0x1708
#define VK_ALT_J           0x2408
#define VK_ALT_K           0x2508
#define VK_ALT_L           0x2608
#define VK_ALT_M           0x3208
#define VK_ALT_N           0x3108
#define VK_ALT_O           0x1808
#define VK_ALT_P           0x1908
#define VK_ALT_Q           0x1008
#define VK_ALT_R           0x1308
#define VK_ALT_S           0x1f08
#define VK_ALT_T           0x1408
#define VK_ALT_U           0x1608
#define VK_ALT_V           0x2f08
#define VK_ALT_W           0x1108
#define VK_ALT_X           0x2d08
#define VK_ALT_Y           0x1508
#define VK_ALT_Z           0x2c08

#define VK_F1              0x3b00
#define VK_F2              0x3c00
#define VK_F3              0x3d00
#define VK_F4              0x3e00
#define VK_F5              0x3f00
#define VK_F6              0x4000
#define VK_F7              0x4100
#define VK_F8              0x4200
#define VK_F9              0x4300
#define VK_F10             0x4400
#define VK_F11             0x8500
#define VK_F12             0x8600
#define VK_F13             0x8C00
#define VK_F14             0x8D00
#define VK_F15             0x8E00
#define VK_F16             0x8F00
#define VK_SH_F1           0x5403
#define VK_SH_F2           0x5503
#define VK_SH_F3           0x5603
#define VK_SH_F4           0x5703
#define VK_SH_F5           0x5803
#define VK_SH_F6           0x5903
#define VK_SH_F7           0x5a03
#define VK_SH_F8           0x5b03
#define VK_SH_F9           0x5c03
#define VK_SH_F10          0x5d03
#define VK_SH_F11          0x8703
#define VK_SH_F12          0x8803
#define VK_CTRL_F1         0x5e04
#define VK_CTRL_F2         0x5f04
#define VK_CTRL_F3         0x6004
#define VK_CTRL_F4         0x6104
#define VK_CTRL_F5         0x6204
#define VK_CTRL_F6         0x6304
#define VK_CTRL_F7         0x6404
#define VK_CTRL_F8         0x6504
#define VK_CTRL_F9         0x6604
#define VK_CTRL_F10        0x6704
#define VK_CTRL_F11        0x8904
#define VK_CTRL_F12        0x8a04
#define VK_ALT_F1          0x6808
#define VK_ALT_F2          0x6908
#define VK_ALT_F3          0x6a08
#define VK_ALT_F4          0x6b08
#define VK_ALT_F5          0x6c08
#define VK_ALT_F6          0x6d08
#define VK_ALT_F7          0x6e08
#define VK_ALT_F8          0x6f08
#define VK_ALT_F9          0x7008
#define VK_ALT_F10         0x7108
#define VK_ALT_F11         0x8b08
#define VK_ALT_F12         0x8c08

#define  VK_CTRL_A         1
#define  VK_CTRL_B         2
#define  VK_CTRL_C         3
#define  VK_CTRL_D         4
#define  VK_CTRL_E         5
#define  VK_CTRL_F         6
#define  VK_CTRL_G         7
#define  VK_CTRL_H         8
#define  VK_CTRL_I         9
#define  VK_CTRL_J         10
#define  VK_CTRL_K         11
#define  VK_CTRL_L         12
#define  VK_CTRL_M         13
#define  VK_CTRL_N         14
#define  VK_CTRL_O         15
#define  VK_CTRL_P         16
#define  VK_CTRL_Q         17
#define  VK_CTRL_R         18
#define  VK_CTRL_S         19
#define  VK_CTRL_T         20
#define  VK_CTRL_U         21
#define  VK_CTRL_V         22
#define  VK_CTRL_W         23
#define  VK_CTRL_X         24
#define  VK_CTRL_Y         25
#define  VK_CTRL_Z         26

/*
  For MS Windows compatibility
*/
#define VK_CANCEL          0x03
#define VK_ADD             '+'
#define VK_DIVIDE          '/'
#define VK_MULTIPLY        '*'
#define VK_SUBTRACT        '-'
#define VK_PRIOR           VK_PGUP
#define VK_NEXT            VK_PGDN
#define VK_BACK            VK_BACKSPACE
#define VK_ESCAPE          27
#define VK_RETURN          '\r'
#define VK_SPACE           ' '
#define VK_CLEAR           VK_CTRL_L
#define VK_PAUSE           VK_CTRL_S

/*
  The following VK keys are not currently generated by MEWEL
*/
#define VK_SELECT         330
#define VK_PRINT          331
#define VK_EXECUTE        332
#define VK_NUMPAD0        333
#define VK_NUMPAD1        334
#define VK_NUMPAD2        335
#define VK_NUMPAD3        336
#define VK_NUMPAD4        337
#define VK_NUMPAD5        338
#define VK_NUMPAD6        339
#define VK_NUMPAD7        340
#define VK_NUMPAD8        341
#define VK_NUMPAD9        342
#define VK_DECIMAL        343
#define VK_F17            348
#define VK_F18            349
#define VK_F19            350
#define VK_F20            351
#define VK_F21            352
#define VK_F22            353
#define VK_F23            354
#define VK_F24            355
#define VK_HELP           356
#define VK_SNAPSHOT       357
#define VK_SEPARATOR      358

#define VK_SCROLL         0x1010  /* MAKEKEY(SCL_SHIFT, SCL_SHIFT) */
#define VK_NUMLOCK        0x2020  /* MAKEKEY(NUM_SHIFT, NUM_SHIFT) */
#define VK_CAPITAL        0x4040  /* MAKEKEY(CAP_SHIFT, CAP_SHIFT) */

/*
  For GetKeyState()
*/
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_ALT            VK_MENU
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_MBUTTON        0x04

#endif /* USE_WINDOWS_COMPAT_KEYS */


/*
  MK_xxx modifiers for the wParam of mouse messages
*/
#define MK_LBUTTON        0x0001
#define MK_RBUTTON        0x0002
#define MK_SHIFT          0x0004
#define MK_CONTROL        0x0008
#define MK_MBUTTON        0x0010

/*
  For the resource compiler, define MEWEL_TEXT by default
*/
#ifdef RC_INVOKED
#ifndef MEWEL_GUI
#ifndef XWINDOWS
#define MEWEL_TEXT
#endif
#endif
#endif

#endif /* __WINDOWS_H  */
#endif /* _INC_WINDOWS */

