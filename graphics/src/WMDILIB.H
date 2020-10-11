/*
 * MDILIB.H - Include for MDI Library
 *
 * (C) Copyright 1988
 * Eikon Systems, Inc.
 * 989 E. Hillsdale Blvd, Suite 260
 * Foster City  CA  94404
 *
 */

/*===========================================================================*/
/*                                                                           */
/* History : Taken from Welch's article for Microsoft Systems Journal. Code  */
/*           used by permission of MSJ. Modified by Marc Adler to support    */
/*           MEWEL and to add MS Windows 3.0 compatibility.                  */
/*                                                                           */
/*===========================================================================*/


/**/

/*
 *
 * Public section of file
 *
 */

#include "wprivate.h"

#ifdef OS2
#define HBITMAP  HWND
#endif

static VOID FAR PASCAL MDIDocumentArrange(HWND, UINT, WPARAM);
UINT FAR PASCAL MdiGetProp(HWND, UINT);
UINT FAR PASCAL MdiSetProp(HWND, UINT, UINT);

#define MDIGETACTIVEWINDOW()  (_HwndMDIActiveChild)


/* Programmatic interface */
static HWND PASCAL MdiChildCreateWindow(LPCSTR, LPCSTR, DWORD, int, int, int, int,
         HWND, HMENU, HANDLE, LPSTR);

/* Child System Menu Commands */
/* Do not use these menu choices yourself */
#if 12695
#define  IDM_CLOSE       (SC_CLOSE      | 0x800)
#define  IDM_RESTORE     (SC_RESTORE    | 0x800)
#define  IDM_NEXTWINDOW  (SC_NEXTWINDOW | 0x800)
#define  IDM_PREVWINDOW  (SC_PREVWINDOW | 0x800)
#define  IDM_MOVE        (SC_MOVE       | 0x800)
#define  IDM_SIZE        (SC_SIZE       | 0x800)
#define  IDM_MAXIMIZE    (SC_MAXIMIZE   | 0x800)
#define  IDM_MINIMIZE    (SC_MINIMIZE   | 0x800)
#else
#define  IDM_CLOSE       0x3f00
#define  IDM_RESTORE     0x3f01
#define  IDM_NEXTWINDOW  0x3f02
#define  IDM_PREVWINDOW  0x3f03
#define  IDM_MOVE        0x3f04
#define  IDM_SIZE        0x3f05
#define  IDM_MAXIMIZE    0x3f06
#define  IDM_MINIMIZE    0x3f0e
#endif

/* Window Menu Choices */
#define  IDM_NEWWINDOW   0x3f07
#define  IDM_WINDOWTILE  0x3f08
#define  IDM_ARRANGEALL  0x3f09
#define  IDM_WINDOWCASCADE  0x3f0c
#define  IDM_WINDOWICONS 0x3f0d
#define  IDM_HIDE        0x3f0a
#define  IDM_UNHIDE      0x3f0b
#define  IDM_FIRSTWIN    0x3f10
#define  WINDOW_POS      9

/**/

/*
 *
 * Private section of file
 *
 */

/* Window data for the MDI parent */
#define  PROP_ACTIVE           1
#define  PROP_COUNT            2
#define  PROP_ZOOM             3
#define  PROP_MAINMENU         4
#define  PROP_WINDOWMENU       5
#define  PROP_CTRLACCEL        6
#define  PROP_SYSMENU          7
#define  PROP_TITLE            8
#define  PROP_FIRSTCHILDID     9
#define  PROP_ITEMSBEFORESEP   10

/* Window data for MDI children */
#define  PROP_LEFT             11
#define  PROP_TOP              12
#define  PROP_WIDTH            13
#define  PROP_HEIGHT           14
#define  PROP_ACCEL            15
#define  PROP_MENUID           16
#define  PROP_ISMDI            17
#define  PROP_ICONIZED         18


/* Unhide dialog box */
#define  DLGUNHIDE_LB    0x100

/* Return values for WM_MENUCHAR (missing from windows.h) */
#define  MC_ABORT        1
#define  MC_SELECT      2

/* States for the keyboard interface for the menus */
#define  POP_NONE        -3
#define  POP_MAINSYS      -2
#define  POP_CHILDSYS    -1
#define  POP_MAIN1ST      0

/**/

/*
 *
 * Procedure prototypes
 *
 */

/* mdi2.c */
static VOID PASCAL MdiDestroyChildWindow(HWND);
static VOID PASCAL MdiActivateChild(HWND, BOOL);
static VOID PASCAL MdiActivateNextChild(HWND);
static VOID PASCAL MdiActivatePrevChild(HWND);
static VOID PASCAL MdiDeactivateChild(HWND);
static VOID PASCAL MdiZoomChild(HWND);
static VOID PASCAL MdiRestoreChild(HWND, BOOL);
static BOOL PASCAL MdiSwitchZoom(HWND, HWND);
static HWND PASCAL MdiChooseNewActiveChild(HWND);
static VOID PASCAL MdiMinimizeChild(HWND);

/* mdi3.c */
static VOID PASCAL MdiZoomMenu(HWND);
static VOID PASCAL MdiRestoreMenu(HWND);
static VOID PASCAL MdiAppendWindowToMenu(HWND, BOOL);
static VOID PASCAL MdiRemoveWindowFromMenu(HWND, BOOL);
static HMENU PASCAL MdiGetChildSysMenu(void);
static HBITMAP PASCAL MdiCreateChildSysBitmap(HWND);
static VOID PASCAL MdiSetMenuKeyHook(HANDLE);
static LONG FAR PASCAL MdiMsgHook(int, WPARAM, LPARAM);
static VOID PASCAL MdiFreeMenuKeyHook(void);

