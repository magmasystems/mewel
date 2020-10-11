/*===========================================================================*/
/*                                                                           */
/* File    : WINDECLS.H                                                      */
/*                                                                           */
/* Purpose : Function prototypes for MEWEL.                                  */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#if defined(DOS) || defined(OS2) || defined(sco) || defined(VAXC)
#define PROTOTYPES 1
#endif

#if defined(DOS) && !defined(__TSC__) && !defined(MEWEL_32BITS)
#define HUGEDATA  huge
#else
#define HUGEDATA  FAR
#endif

#if defined(MEWEL_32BITS) 
#undef _huge
#define _huge
#endif


/*
  Kludge for the naming conflicts between the Windows API and some
  BGI functions which are called with the PASCAL calling convention.
*/
#if defined(_WPRIVATE_H) && (defined(__DPMI16__) || defined(__DPMI32__)) && defined(MEWEL_GUI) && !defined(META) && !defined(GX)
#define Arc         InternalArc
#define Ellipse     InternalEllipse
#define FloodFill   InternalFloodFill
#define GetPixel    InternalGetPixel
#define LineTo      InternalLineTo
#define MoveTo      InternalMoveTo
#define Rectangle   InternalRectangle
#define SetBkColor  InternalSetBkColor
#endif



/* WARRICON.C */
extern UINT FAR PASCAL ArrangeIconicWindows(HWND);

/* WCLIPBRD.c */
extern BOOL FAR PASCAL CloseClipboard(void);
extern BOOL FAR PASCAL EmptyClipboard(void);
extern UINT FAR PASCAL EnumClipboardFormats(UINT);
extern HANDLE FAR PASCAL GetClipboardData(UINT);
extern HANDLE FAR PASCAL SetClipboardData(UINT, HANDLE);
extern INT  FAR PASCAL GetClipboardFormatName(UINT, LPSTR, INT);
extern HWND FAR PASCAL GetClipboardOwner(void);
extern HWND FAR PASCAL GetClipboardViewer(void);
extern BOOL FAR PASCAL IsClipboardFormatAvailable(UINT);
extern BOOL FAR PASCAL OpenClipboard(HWND);
extern HWND FAR PASCAL SetClipboardViewer(HWND);
extern UINT FAR PASCAL RegisterClipboardFormat(LPCSTR);
extern BOOL FAR PASCAL ChangeClipboardChain(HWND, HWND);
extern INT  FAR PASCAL CountClipboardFormats(void);
extern INT  FAR PASCAL GetPriorityClipboardFormat(UINT FAR *, INT);
#if (WINVER >= 0x030a)
extern HWND WINAPI GetOpenClipboardWindow(void);
#endif  /* WINVER >= 0x030a */


extern BOOL FAR PASCAL IsWindow(HWND);
extern BOOL FAR PASCAL IsMenu(HMENU);

/* WDESKTOP.C */
extern HWND WINAPI GetDesktopWindow(void);


/* WDLGDIR.c */
extern BOOL FAR PASCAL DlgDirListComboBox(HWND,LPSTR, int, int, UINT);
extern BOOL FAR PASCAL DlgDirList(HWND,LPSTR, int, int, UINT);
extern BOOL FAR PASCAL DlgDirSelectComboBox(HWND,LPSTR, int);
extern BOOL FAR PASCAL DlgDirSelect(HWND,LPSTR, int);
extern BOOL FAR PASCAL DlgDirSelectComboBoxEx(HWND,LPSTR,int,int);
extern BOOL FAR PASCAL DlgDirSelectEx(HWND,LPSTR,int,int);
extern INT  FAR PASCAL SpecHasWildcards(LPSTR);
extern INT  FAR PASCAL CheckCriticalError(INT);

/* WDLGCTID.C */
extern INT WINAPI GetDlgCtrlID(HWND);

/* WDLGOPEN.c */
extern INT FAR PASCAL DlgOpenFile(HWND,LPSTR,LPSTR);

/* WDRAWTXT.C */
extern INT FAR PASCAL DrawText(HDC, LPCSTR, int, LPRECT, UINT);
#define  GetTabDistance(CP, TW)     ((TW) - ((CP) % (TW)) - 1)


/* WDRVTYPE.c */
extern UINT  FAR PASCAL GetDriveType(int);
/* GetDriveType return values */
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED     3
#define DRIVE_REMOTE    4

/* WEMALLOC.c */
extern PSTR  FAR PASCAL emalloc(UINT);
extern PSTR  FAR PASCAL emalloc_noquit(UINT);
extern VOID  FAR PASCAL MyFree(void *);
extern LPSTR FAR PASCAL emalloc_far(DWORD);
extern LPSTR FAR PASCAL emalloc_far_noquit(DWORD);
extern LPSTR FAR PASCAL realloc_far(void FAR *, DWORD);
extern VOID  FAR PASCAL MyFree_far(void FAR *);
#if defined(__TURBOC__)
extern DWORD FAR PASCAL MyFarCoreLeft(void);
#endif
extern PSTR  FAR PASCAL strsave(PSTR);
extern LPSTR FAR PASCAL lstrsave(LPSTR);

/* WGETNEXT.c */
extern HWND FAR PASCAL GetWindow(HWND,UINT);
extern HWND FAR PASCAL GetTopWindow(HWND);
extern HWND FAR PASCAL GetNextWindow(HWND,UINT);
#if !defined(MAC)
extern HWND FAR PASCAL FindWindow(LPCSTR, LPCSTR);
#endif

/* WGDIBRUS.C */
DWORD WINAPI GetBrushOrg(HDC);
BOOL  WINAPI GetBrushOrgEx(HDC, POINT FAR *);
DWORD WINAPI SetBrushOrg(HDC, int, int);

/* WGDIBMP2.C */
HBRUSH  WINAPI CreateDIBPatternBrush(HGLOBAL, UINT);
HBITMAP WINAPI CreateDiscardableBitmap(HDC, int, int);
DWORD   WINAPI GetBitmapDimension(HBITMAP);
BOOL    WINAPI GetBitmapDimensionEx(HBITMAP, SIZE FAR *);
DWORD   WINAPI SetBitmapDimension(HBITMAP, int, int);
BOOL    WINAPI SetBitmapDimensionEx(HBITMAP, int, int, SIZE FAR *);


/* WINACCEL.c */
extern BOOL FAR PASCAL TranslateMessage(LPMSG);
extern BOOL FAR PASCAL TranslateAccelerator(HWND, HACCEL, LPMSG);

/* WINATOM.C */
extern BOOL WINAPI InitAtomTable(int);
extern ATOM WINAPI AddAtom(LPCSTR);
extern ATOM WINAPI DeleteAtom(ATOM);
extern ATOM WINAPI FindAtom(LPCSTR);
extern GLOBALHANDLE WINAPI GetAtomHandle(ATOM);
extern UINT WINAPI GetAtomName(ATOM, LPSTR, INT);
extern UINT WINAPI RegisterWindowMessage(LPSTR);
/* WINATOM2.C */
extern ATOM WINAPI GlobalAddAtom(LPCSTR);
extern ATOM WINAPI GlobalDeleteAtom(ATOM);
extern UINT WINAPI GlobalGetAtomName(ATOM, LPSTR, INT);
extern ATOM WINAPI GlobalFindAtom(LPCSTR);


/* WINBLIT.c */
extern INT FAR PASCAL WinBltRect(HWND,HDC,LPRECT,LPRECT);
extern INT FAR PASCAL WinFillRect(HWND,HDC,LPRECT,INT,COLOR);
extern INT FAR PASCAL WinInvertRect(HWND,HDC,LPRECT);
extern INT FAR PASCAL WinRestoreRect(HWND,LPRECT,HANDLE);
extern HANDLE FAR PASCAL _WinSaveRect(HWND,LPRECT);
#if defined(MEWEL_TEXT)
extern INT  FAR PASCAL WinColorRect(HWND,HDC,LPRECT,INT,COLOR);
#else
#define WinColorRect(hWnd, hDC, lpRect, ch, attr)
#endif

/* WINBORDR.c */
extern VOID FAR PASCAL WinDrawBorder(HWND,int);
extern INT  FAR PASCAL WinDrawFrame(HWND,COLOR,int);

/* WINCLASS.c */
extern INT  FAR PASCAL RegisterClass(WNDCLASS *);
extern INT  FAR PASCAL ExtRegisterClass(EXTWNDCLASS *);
extern INT  FAR PASCAL ClassNameToClassID(LPCSTR);
extern INT  FAR PASCAL WinGetClass(HWND);
extern LPCSTR FAR PASCAL WinGetClassName(HWND);
extern DWORD FAR PASCAL GetClassStyle(HWND);
extern INT  FAR PASCAL GetClassName(HWND,LPCSTR,int);
extern LPCSTR FAR PASCAL ClassIDToClassName(UINT);

#if 0
extern LONG (FAR PASCAL *FAR PASCAL ClassToWinProc(int idClass))();
extern LONG (FAR PASCAL *FAR PASCAL ClassIDToDefProc(UINT id))();
extern LONG (FAR PASCAL *FAR PASCAL ClassIDToWndProc(UINT id))();
#else
extern WINPROC *FAR PASCAL ClassToWinProc(int);
extern WINPROC *FAR PASCAL ClassIDToDefProc(UINT);
extern WINPROC *FAR PASCAL ClassIDToWndProc(UINT);
#endif

extern EXTWNDCLASS *FAR PASCAL ClassIDToClassStruct(UINT);
extern INT  FAR PASCAL ClassNameToBaseClass(LPCSTR);
extern BOOL FAR PASCAL GetClassInfo(HINSTANCE, LPCSTR, WNDCLASS FAR *);
extern BOOL FAR PASCAL ExtGetClassInfo(HANDLE, LPCSTR, LPEXTWNDCLASS);
extern VOID FAR PASCAL _RegisterPredefinedClasses(void);
extern BOOL FAR PASCAL UnregisterClass(LPCSTR, HANDLE);

/* WINCOLOR.c */
extern VOID  FAR PASCAL WinUseSysColors(HWND,BOOL);
extern COLOR FAR PASCAL WinQuerySysColor(HWND,UINT);
extern COLOR FAR PASCAL WinSetSysColor(HWND,UINT,COLOR);
extern COLOR FAR PASCAL WinGetClassBrush(HWND);
extern DWORD FAR PASCAL GetSysColor(int);
extern VOID  FAR PASCAL SetSysColors(int, LPINT, DWORD FAR *);

/* WINCREAT.c */
extern HWND FAR PASCAL _CreateWindow(LPCSTR,LPCSTR,DWORD,int,int,int,int,COLOR,UINT,HWND,HMENU,HINSTANCE,void FAR*);
extern HWND FAR PASCAL CreateWindow(LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void FAR*);
extern HWND FAR PASCAL CreateWindowEx(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void FAR*);

/* WINDATA.C */
extern DWORD WINAPI GetVersion(void);
extern DWORD WINAPI GetWinFlags(void);

/* WINDLG.c */
extern HWND FAR PASCAL _DlgGetParent(HWND);
extern HWND FAR PASCAL DialogCreate(HWND,int,int,int,int,LPSTR,COLOR,DWORD,FARPROC,int,LPSTR,HANDLE);

extern HWND FAR PASCAL CreateDialog(HINSTANCE, LPCSTR , HWND, FARPROC);
extern HWND FAR PASCAL CreateDialogParam(HINSTANCE, LPCSTR , HWND, FARPROC, LONG);
extern HWND FAR PASCAL CreateDialogIndirectParam(HINSTANCE, CONST VOID FAR *, HWND, FARPROC, LONG);
extern INT  FAR PASCAL DialogBoxParam(HINSTANCE, LPCSTR, HWND, FARPROC, LONG);
extern INT  FAR PASCAL DialogBox(HINSTANCE, LPCSTR , HWND, FARPROC);
extern INT  FAR PASCAL DialogBoxIndirect(HINSTANCE, GLOBALHANDLE , HWND, FARPROC);
extern INT  FAR PASCAL DialogBoxIndirectParam(HINSTANCE, GLOBALHANDLE , HWND, FARPROC, LONG);
extern INT  FAR PASCAL _DialogBox(HWND);
extern INT  FAR PASCAL _DialogBoxParam(HWND, LONG);

extern LONG FAR PASCAL DlgBoxWinProc(HWND,UINT,UINT,LONG);
extern LONG FAR PASCAL DefDlgProc(HWND,UINT,UINT,LONG);
extern BOOL FAR PASCAL DlgSetFocus(HWND);
extern BOOL FAR PASCAL _DlgSetFocus(HWND,BOOL);
extern BOOL FAR PASCAL IsDialogMessage(HWND,LPMSG);
extern INT  FAR PASCAL EndDialog(HWND,int);
extern HWND FAR PASCAL DlgGetFirstTabItem(HWND);
extern INT  FAR PASCAL DialogTab(HWND,int,int);
extern HWND FAR PASCAL GetNextDlgTabItem(HWND,HWND,BOOL);
extern HWND FAR PASCAL GetNextDlgGroupItem(HWND,HWND,UINT);
extern INT  FAR PASCAL CheckDlgButton(HWND,int,int);
extern INT  FAR PASCAL CheckRadioButton(HWND,int,int,int);
extern UINT FAR PASCAL GetDlgItemInt(HWND, int, BOOL FAR *, BOOL);
extern HWND FAR PASCAL GetDlgItem(HWND,int);
extern INT  FAR PASCAL GetDlgItemText(HWND,int,LPSTR,int);
extern BOOL FAR PASCAL IsDlgButtonChecked(HWND,int);
extern LRESULT FAR PASCAL SendDlgItemMessage(HWND, int, UINT, WPARAM, LPARAM);
extern BOOL FAR PASCAL SetDlgItemText(HWND,int,LPCSTR);
extern VOID FAR PASCAL SetDlgItemInt(HWND,int,UINT,BOOL);
extern BOOL FAR PASCAL SetDlgItemStyle(HWND,int,DWORD);
extern HWND FAR PASCAL DlgGetDefButton(HWND,int *);
extern VOID FAR PASCAL MapDialogRect(HWND, LPRECT);
extern LONG FAR PASCAL GetDialogBaseUnits(void);

/* WINDC.c */
extern HDC   FAR PASCAL GetDC(HWND);
extern HDC   FAR PASCAL GetWindowDC(HWND);
extern INT   FAR PASCAL ReleaseDC(HWND,HDC);
extern DWORD FAR PASCAL GetDCOrg(HDC);
extern HDC   FAR PASCAL BeginPaint(HWND,LPPAINTSTRUCT);
extern VOID  FAR PASCAL EndPaint(HWND,CONST PAINTSTRUCT FAR*);
extern DWORD FAR PASCAL SetBkColor(HDC,DWORD);
extern DWORD FAR PASCAL GetBkColor(HDC);
extern DWORD FAR PASCAL SetTextColor(HDC,DWORD);
extern DWORD FAR PASCAL GetTextColor(HDC);
extern DWORD FAR PASCAL GetNearestColor(HDC, COLORREF);
extern UINT  FAR PASCAL RGBtoAttr(HDC, DWORD);
extern BOOL  FAR PASCAL _PrepareWMCtlColor(HWND, UINT, HDC);
extern VOID  FAR PASCAL UpdateWindow(HWND);
extern VOID  FAR PASCAL InvalidateRect(HWND,CONST RECT FAR *,BOOL);
extern VOID  FAR PASCAL ValidateRect(HWND, CONST RECT FAR *);
extern INT   FAR PASCAL GetUpdateRect(HWND,LPRECT,BOOL);
extern VOID  FAR PASCAL SetUpdateRect(HWND,LPRECT);
extern BOOL  FAR PASCAL TextOut(HDC,int,int,LPCSTR,int);
extern BOOL  FAR PASCAL ExtTextOut(HDC,INT,INT,UINT,CONST RECT FAR *,LPCSTR,UINT, LPINT);
extern DWORD FAR PASCAL GetTextExtent(HDC,LPCSTR,int);
extern BOOL  FAR PASCAL GetTextExtentPoint(HDC,LPCSTR,int,SIZE FAR *);
extern BOOL  FAR PASCAL GetCharWidth(HDC, UINT, UINT, LPINT);
#if !defined(MAC)
extern INT   FAR PASCAL FillRect(HDC,CONST RECT FAR *,HBRUSH);
extern INT   FAR PASCAL FrameRect(HDC,CONST RECT FAR *,HBRUSH);
extern VOID  FAR PASCAL InvertRect(HDC, CONST RECT FAR *);
#endif
extern BOOL  FAR PASCAL Rectangle(HDC,INT,INT,INT,INT);
#if defined(MEWEL_GUI) || defined(XWINDOWS)
extern VOID  FAR PASCAL DrawFocusRect(HDC, CONST RECT FAR *);
#endif
extern VOID  FAR PASCAL AdjustWindowRect(LPRECT,DWORD,BOOL);
#if (WINVER >= 0x030a)
extern INT   FAR PASCAL SetWindowPos(HWND,HWND,INT,INT,INT,INT,UINT);
#else
extern VOID  FAR PASCAL SetWindowPos(HWND,HWND,INT,INT,INT,INT,UINT);
#endif
extern LONG  FAR PASCAL _WindowizeMouse(HWND, LONG);
extern LONG  FAR PASCAL _UnWindowizeMouse(HWND, LONG);

extern BOOL FAR PASCAL GrayString(HDC,HBRUSH,FARPROC,DWORD,int,int,int,int,int);


/* WINDRAW.c */
extern VOID FAR PASCAL WinDrawAllWindows(void);
extern INT  FAR PASCAL WinClear(HWND);
extern INT  FAR PASCAL WinEraseEOP(HWND,int,int,COLOR);
extern INT  FAR PASCAL WinEraseEOL(HWND,int,int,COLOR);
extern INT  FAR PASCAL WinEraseRect(HWND,LPRECT,COLOR);
extern INT  FAR PASCAL WinPutsCenter(HWND,int,LPSTR,COLOR);
extern INT  FAR PASCAL WinPutsRight(HWND,int,LPSTR,COLOR);
extern INT  FAR PASCAL WinPuts(HWND,int,int,LPSTR,COLOR);
extern INT  FAR PASCAL WinPutc(HWND,int,int,int,COLOR);
extern INT  FAR PASCAL WinGenAncestorClippingRect(HWND,HDC,LPRECT);
extern BOOL FAR PASCAL WinIsPointVisible(HWND,int,int);

extern BOOL FAR PASCAL IsWindowVisible(HWND);
extern HWND FAR PASCAL WindowFromPoint(POINT);
extern HWND FAR PASCAL ChildWindowFromPoint(HWND,POINT);

/* WINDSTRY.c */
extern INT FAR PASCAL DestroyWindow(HWND);

/* WINENABL.c */
extern BOOL FAR PASCAL IsWindowEnabled(HWND);
extern INT  FAR PASCAL EnableWindow(HWND,BOOL);

/* WINENUM.c */
#ifdef STRICT
typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
#else
typedef FARPROC WNDENUMPROC;
#endif
extern BOOL FAR PASCAL EnumWindows(WNDENUMPROC,LONG);
extern BOOL FAR PASCAL EnumChildWindows(HWND,WNDENUMPROC,LONG);
extern BOOL FAR PASCAL EnumTaskWindows(HTASK, WNDENUMPROC, LONG);

/* WINEXEC.c */
#ifdef MODSOFT
extern INT  FAR PASCAL WinExecVars(char *, char *, char *);
extern INT  FAR PASCAL WinExecOpt(char *, char *, BOOL , BOOL);
#endif
extern INT  FAR PASCAL WinExec(char *, UINT);
#define EXEC_SWAP   0x00FF
#define EXEC_PAUSE  0x0100
#define EXEC_80X25  0x0200

/* WINEXTRA.c */
extern BOOL FAR PASCAL SetWindowExtra(HWND,int);
extern WORD FAR PASCAL GetWindowWord(HWND,int);
extern LONG FAR PASCAL GetWindowLong(HWND,int);
extern WORD FAR PASCAL SetWindowWord(HWND,int,WORD);
extern LONG FAR PASCAL SetWindowLong(HWND,int,long);

/* WINICON.c */
extern HICON WINAPI LoadIcon(HINSTANCE, LPCSTR);
extern HICON WINAPI CreateIcon(HINSTANCE,int,int,BYTE,BYTE,CONST VOID FAR*,CONST VOID FAR*);
extern BOOL  FAR PASCAL DestroyIcon(HICON);
extern BOOL  FAR PASCAL DrawIcon(HDC,int,int,HICON);
extern BOOL  FAR PASCAL IsIconic(HWND);
extern BOOL  FAR PASCAL OpenIcon(HWND);

/* WININIT.C */
extern INT  FAR PASCAL WinInit(void);
extern INT  FAR PASCAL GetModuleFileName(HANDLE, LPSTR, int);
extern UINT FAR PASCAL GetSystemDirectory(LPSTR, INT);
extern UINT WINAPI GetWindowsDirectory(LPSTR, int);
extern INT  FAR PASCAL GetKeyState(int);
extern VOID FAR PASCAL XMSReserveMemory(UINT);
extern BOOL FAR PASCAL LocalInit(UINT, UINT, UINT);
#if defined(MOTIF)
VOID FAR PASCAL XMEWELInitMotif(void);
#endif

extern UINT FAR PASCAL SetErrorMode(UINT);
/* SetErrorMode() constants */
#define SEM_FAILCRITICALERRORS  0x0001
#define SEM_NOGPFAULTERRORBOX   0x0002
#define SEM_NOOPENFILEERRORBOX  0x8000

/* WINKBD.C */
INT FAR PASCAL GetKeyboardType(int);

/* WINMDI.C */
extern LONG FAR PASCAL DefFrameProc(HWND,HWND,UINT,WPARAM,LPARAM);
extern LONG FAR PASCAL DefMDIChildProc(HWND,UINT,WPARAM,LPARAM);
extern BOOL FAR PASCAL TranslateMDISysAccel(HWND,LPMSG);
extern int  FAR PASCAL MDIInitialize(VOID);

/* WINMENU.c */
extern BOOL  FAR PASCAL ChangeMenu(HMENU,UINT,LPCSTR,UINT,UINT);
extern BOOL  FAR PASCAL CheckMenuItem(HMENU,UINT,UINT);
extern HMENU FAR PASCAL CreateMenu(void);
extern HMENU FAR PASCAL CreatePopupMenu(void);
extern INT   FAR PASCAL DestroyMenu(HMENU);
extern BOOL  FAR PASCAL EnableMenuItem(HMENU,UINT,UINT);
extern INT   FAR PASCAL GetMenuItemCount(HMENU);
extern UINT  FAR PASCAL GetMenuItemID(HMENU,int);
extern UINT  FAR PASCAL GetMenuState(HMENU,UINT,UINT);
extern INT   FAR PASCAL GetMenuString(HMENU,UINT,LPSTR,int,UINT);
extern HMENU FAR PASCAL GetSubMenu(HMENU,int);
extern HMENU FAR PASCAL LoadPopupMenu(HWND,LPCSTR);
extern BOOL  FAR PASCAL SetMenu(HWND,HMENU);
extern BOOL  FAR PASCAL TrackPopupMenu(HMENU,UINT,int,int,int,HWND,CONST RECT *);
#if !defined(MAC)
extern INT   FAR PASCAL DrawMenuBar(HWND);
extern HMENU FAR PASCAL GetMenu(HWND);
extern BOOL  WINAPI     InsertMenu(HMENU, UINT, UINT, UINT, LPCSTR);
extern BOOL  WINAPI     AppendMenu(HMENU, UINT, UINT, LPCSTR);
extern BOOL  WINAPI     DeleteMenu(HMENU, UINT, UINT);
#endif
extern BOOL  WINAPI     ModifyMenu(HMENU, UINT, UINT, UINT, LPCSTR);
extern BOOL  WINAPI     RemoveMenu(HMENU, UINT, UINT);

/* WINMONO.c */
extern COLOR FAR PASCAL WinMapAttr(COLOR);
extern VOID FAR PASCAL WinSetMonoMapColor(int,int,COLOR);
extern VOID FAR PASCAL WinUseMonoMap(int);

/* WINMOVE.c */
extern INT  FAR PASCAL WinMove(HWND,int,int);
#if !defined(MAC)
#if (WINVER >= 0x030a)
extern BOOL FAR PASCAL MoveWindow(HWND, int, int, int, int, BOOL);
#else
extern VOID FAR PASCAL MoveWindow(HWND, int, int, int, int, BOOL);
#endif
#endif /* MAC */

/* WINPROP.c */
extern BOOL   FAR PASCAL SetProp(HWND, LPCSTR, HANDLE);
extern HANDLE FAR PASCAL GetProp(HWND, LPCSTR);
extern HANDLE FAR PASCAL RemoveProp(HWND, LPCSTR);
#ifdef STRICT
typedef BOOL (CALLBACK* PROPENUMPROC)(HWND, LPCSTR, HANDLE);
#else
typedef FARPROC PROPENUMPROC;
#endif
extern INT  FAR PASCAL EnumProps(HWND, PROPENUMPROC);


/* WINRECT.c */
extern VOID FAR PASCAL RectTile(RECT,RECT,RECT[]);
extern VOID FAR PASCAL CopyRect(LPRECT,CONST RECT FAR *);
#if !defined(MAC)
extern BOOL FAR PASCAL EqualRect(CONST RECT FAR *,CONST RECT FAR *);
extern VOID FAR PASCAL OffsetRect(LPRECT,int,int);
extern BOOL FAR PASCAL PtInRect(CONST RECT FAR *,POINT);
extern VOID FAR PASCAL SetRect(LPRECT,int,int,int,int);
extern BOOL WINAPI     UnionRect(RECT FAR*, CONST RECT FAR*, CONST RECT FAR*);
#endif /* MAC */
extern VOID FAR PASCAL InflateRect(LPRECT,int,int);
extern INT  FAR PASCAL IntersectRect(RECT FAR *,CONST RECT FAR *,CONST RECT FAR *);
extern BOOL FAR PASCAL IsRectEmpty(CONST RECT FAR *);
extern VOID FAR PASCAL SetRectEmpty(LPRECT);
extern VOID FAR PASCAL ClientToScreen(HWND,LPPOINT);
extern VOID FAR PASCAL ScreenToClient(HWND,LPPOINT);
extern VOID FAR PASCAL WinClientRectToScreen(HWND,LPRECT);
extern VOID FAR PASCAL WinScreenRectToClient(HWND,LPRECT);
extern VOID FAR PASCAL GetClientRect(HWND, LPRECT);
extern BOOL WINAPI SubtractRect(RECT FAR*, CONST RECT FAR*, CONST RECT FAR*);

/* WINRES.c */
#if !defined(MAC)
extern LPSTR  FAR PASCAL GetResource(HMODULE,LPSTR,LPCSTR);
#endif
extern HINSTANCE FAR PASCAL OpenResourceFile(PSTR);
extern VOID   FAR PASCAL CloseResourceFile(HINSTANCE);

extern HACCEL FAR PASCAL LoadAccelerators(HINSTANCE,LPCSTR);
extern HMENU  FAR PASCAL LoadMenu(HINSTANCE,LPCSTR);
extern HMENU  FAR PASCAL LoadMenuIndirect(CONST VOID FAR *);
extern HWND   FAR PASCAL LoadDialog(HINSTANCE,LPCSTR,HWND,FARPROC);
extern HWND   FAR PASCAL CreateDialogIndirect(HINSTANCE, CONST VOID FAR*, HWND, FARPROC);
extern HWND   FAR PASCAL _CreateDialogIndirect(HINSTANCE, CONST VOID FAR*, HWND, FARPROC);

extern PSTR   FAR PASCAL _DosSearchPath(PSTR,PSTR,PSTR);
extern INT    FAR PASCAL LoadString(HANDLE,int,LPSTR,UINT);

/* WINRES2.C */
HRSRC   WINAPI FindResource(HINSTANCE, LPCSTR, LPCSTR);
HGLOBAL WINAPI LoadResource(HINSTANCE, HRSRC);
BOOL    WINAPI FreeResource(HGLOBAL);
#ifdef STRICT
void    FAR* WINAPI LockResource(HGLOBAL);
#else
char    FAR* WINAPI LockResource(HGLOBAL);
#endif
#define UnlockResource(hRes)   GlobalUnlock(hRes)
GLOBALHANDLE FAR PASCAL AllocResource(HANDLE, HANDLE, DWORD);
DWORD   FAR PASCAL SizeofResource(HANDLE, HANDLE);
int     FAR PASCAL AccessResource(HANDLE, HANDLE);
#ifdef STRICT
typedef HGLOBAL (CALLBACK* RSRCHDLRPROC)(HGLOBAL, HINSTANCE, HRSRC);
#else
typedef FARPROC RSRCHDLRPROC;
#endif
RSRCHDLRPROC WINAPI SetResourceHandler(HINSTANCE, LPCSTR, RSRCHDLRPROC);


/* WINSCROL.c */
extern INT  FAR PASCAL WinScrollWindow(HWND,int,int,CONST RECT FAR*);
extern VOID WINAPI ScrollWindow(HWND, int, int, CONST RECT FAR*, CONST RECT FAR*);

/* WINSET.c */
extern DWORD FAR PASCAL WinSetFlags(HWND,DWORD);
extern DWORD FAR PASCAL WinSetStyle(HWND,DWORD);
extern DWORD FAR PASCAL WinGetFlags(HWND);
extern DWORD FAR PASCAL WinGetStyle(HWND);
extern COLOR FAR PASCAL WinSetAttr(HWND,COLOR,BOOL);
extern COLOR FAR PASCAL WinGetAttr(HWND);
extern HWND  FAR PASCAL SetCapture(HWND);
extern HWND  FAR PASCAL GetCapture(void);
extern INT   FAR PASCAL ReleaseCapture(void);
#if defined(UNIX) && !defined(sco)
extern LONG (*WinSetWinProc(HWND,long (*)()))();
#else
extern WINPROC *FAR PASCAL WinSetWinProc(HWND,WINPROC *);
#endif
extern INT  FAR PASCAL SetWindowText(HWND,LPCSTR);
extern INT  FAR PASCAL GetWindowText(HWND,LPSTR,int);
extern INT  FAR PASCAL GetWindowTextLength(HWND);
#if !defined(MAC)
extern HCURSOR FAR PASCAL SetCursor(HCURSOR);
extern HCURSOR FAR PASCAL GetCursor(void);
#endif


/* WINSHOW.c */
#if !defined(MAC)
extern BOOL FAR PASCAL ShowWindow(HWND,int);
extern VOID FAR PASCAL CloseWindow(HWND);
#endif

/* WINSIZE.c */
extern INT  FAR PASCAL WinSetSize(HWND,int,int);

/* WINSOUND.c */
extern VOID FAR PASCAL MessageBeep(int);
extern VOID FAR PASCAL SoundNote(int,int,unsigned);
extern VOID FAR PASCAL SoundTone(unsigned,unsigned,unsigned);
extern VOID FAR PASCAL SoundClick(void);

/* WINTASK.C */
HTASK FAR PASCAL GetWindowTask(HWND);
#if defined(__DPMI16__) || defined(__DPMI32__)
HTASK FAR PASCAL GetCurrentTask(void);
#else
#ifdef __cplusplus
inline HTASK FAR PASCAL GetCurrentTask(void)  { return GetWindowTask(NULL); }
#else
#define GetCurrentTask()                               GetWindowTask(NULL)
#endif
#endif
UINT WINAPI GetNumTasks(void);

/* WINTERM.c */
#if defined(__WATCOMC__)
extern VOID           WinTerminate(void);
#else
extern VOID     CDECL WinTerminate(void);
#endif

/* WINTIME.c */
extern INT   FAR PASCAL DosGetTime(int *,int *,int *);
extern DWORD FAR PASCAL BIOSGetTime(void);
extern DWORD FAR PASCAL BIOSSetTime(int);
extern DWORD FAR PASCAL GetCurrentTime(void);
extern DWORD FAR PASCAL GetTickCount(void);

/* WINTIMER.c */
extern UINT  FAR PASCAL SetTimer(HWND,UINT,UINT,TIMERPROC);
extern BOOL  FAR PASCAL KillTimer(HWND,UINT);

/* WINTOTOP.c */
extern INT   FAR PASCAL BringWindowToTop(HWND);
extern INT   FAR PASCAL WinToTop(HWND);

/* WINZOOM.c */
extern BOOL FAR PASCAL WinZoom(HWND);
extern BOOL FAR PASCAL IsZoomed(HWND);
extern BOOL FAR PASCAL WinMinimize(HWND);

#ifndef VAXC
/* WLINETO.c */
#if !defined(MAC)
extern DWORD FAR PASCAL MoveTo(HDC, INT, INT);
extern BOOL  FAR PASCAL LineTo(HDC, INT, INT);
#endif
extern BOOL  FAR PASCAL MoveToEx(HDC, INT, INT, POINT FAR *);
extern DWORD FAR PASCAL GetCurrentPosition(HDC);
extern BOOL  FAR PASCAL GetCurrentPositionEx(HDC, POINT FAR *);
#endif

/*
  Other graphics functions
*/
BOOL  FAR PASCAL Polyline(HDC, CONST POINT FAR *, int);
BOOL  FAR PASCAL Ellipse(HDC, int, int, int, int);
BOOL  FAR PASCAL Arc(HDC,int,int,int,int,int,int,int,int);
BOOL  FAR PASCAL Pie(HDC,int,int,int,int,int,int,int,int);
BOOL  FAR PASCAL Polygon(HDC, CONST POINT FAR *, int);
DWORD FAR PASCAL GetPixel(HDC, int, int);
DWORD FAR PASCAL SetPixel(HDC, int, int, COLORREF);


/* WLOADBMP.c */
extern HBITMAP FAR PASCAL LoadBitmap(HINSTANCE, LPCSTR);


/*
  String functions defined in WLSTRING.C
*/
#if defined(MSC) || defined(__TURBOC__) || defined(__ZTC__)
#ifndef memcpy
#include <string.h>
#endif
#endif
#if !defined(__TURBOC__) && !defined(__HIGHC__) && defined(MSC) && (_MSC_VER < 700) && !defined(UNIX)
#pragma intrinsic(memcpy)
#pragma intrinsic(memset)
#pragma intrinsic(memcmp)
#pragma intrinsic(strlen)
#if !defined(MSC)
#pragma intrinsic(strchr)
#endif
#endif

#if (defined(DOS) && !defined(M_I86LM) && !defined(M_I86HM) && !defined(__LARGE__) && !defined(MEWEL_32BITS))
extern LPSTR FAR PASCAL lmemchr(LPSTR, int, UINT);
extern LPSTR FAR PASCAL lmemcpy(LPSTR,LPSTR,UINT);
extern LPSTR FAR PASCAL lmemset(LPSTR,int,UINT);
extern INT   FAR PASCAL lstrlen(LPCSTR);
extern LPSTR FAR PASCAL lstrchr(LPCSTR, int);
extern LPSTR FAR PASCAL lstrcpy(LPSTR,LPCSTR);
extern LPSTR FAR PASCAL lstrncpy(LPSTR,LPCSTR,UINT);
extern LPSTR FAR PASCAL lstrcpyn(LPSTR,LPCSTR,UINT);
extern LPSTR FAR PASCAL lstrcat(LPSTR,LPCSTR);
extern INT   FAR PASCAL lstrcmp(LPCSTR,LPCSTR);
#else
/*
  Use built-in functions for non-DOS systems, DOS-large, or DOS prot-mode
*/
#include <string.h>
#define lmemchr(s, c, n)   memchr(s, c, n)  
#define lmemcpy(s, t, n)   memcpy(s, t, n)  
#define lmemset(s, c, n)   memset(s, c, n)  
#if defined(sunos) && !defined(OLD_HEADERS)
extern void *memmove(void *, const void *, size_t);
#endif
#define lstrlen(s)         ((int) strlen(s))
#define lstrchr(s, c)      strchr(s, c)
#define lstrcpy(s,t)       strcpy(s,t)
#define lstrncpy(s,t,n)    strncpy(s,t,n)

/*
  The four funcs below are not in the BC4 RTL
*/
#if defined(__BORLANDC__) && defined(__DLL__)
extern LPSTR FAR PASCAL lstrcpyn(LPSTR,LPCSTR,UINT);
extern LPSTR FAR PASCAL lstrcat(LPSTR,LPCSTR);
extern INT   FAR PASCAL lstrcmp(LPCSTR,LPCSTR);
#else
#define lstrcpyn(s,t,n)    strncpy(s,t,n)
#define lstrcat(s,t)       strcat(s,t)
#define lstrcmp(s,t)       strcmp(s,t)
#endif
#endif

extern INT   FAR PASCAL lstricmp(LPCSTR,LPCSTR);
extern INT   FAR PASCAL lstrcmpi(LPCSTR,LPCSTR);
extern INT   FAR PASCAL lstrnicmp(LPCSTR,LPCSTR,UINT);

#if defined(UNIX)
extern char *strlwr(char *);
extern char *strrev(char *);
extern char *strdate(char *);
#endif
extern LPSTR FAR PASCAL lstrupr(LPSTR);
extern INT  FAR PASCAL lmemscan(LPSTR, int, UINT);
extern PSTR FAR PASCAL strcpy_until(PSTR,PSTR,int);
extern PSTR FAR PASCAL next_int_token(PSTR,int *);
extern PSTR FAR PASCAL next_token(PSTR);
extern PSTR FAR PASCAL span_blanks(PSTR);
extern PSTR FAR PASCAL span_chars(PSTR);
extern PSTR FAR PASCAL rtrim(PSTR);
extern VOID FAR PASCAL hmemcpy(void _huge *, CONST VOID _huge *, LONG);

/* WFILEIO.c */
extern HFILE FAR PASCAL _lcreat(LPCSTR, int);
extern HFILE FAR PASCAL _lopen(LPCSTR, int);
extern HFILE FAR PASCAL _lclose(HFILE);
extern UINT  FAR PASCAL _lread(HFILE, VOID _huge *, UINT);
extern UINT  FAR PASCAL _lwrite(HFILE, CONST VOID _huge *, UINT);
extern LONG  FAR PASCAL _hread(HFILE, VOID _huge *, LONG);
extern LONG  FAR PASCAL _hwrite(HFILE, CONST VOID _huge *, LONG);
extern LONG  FAR PASCAL _llseek(HFILE, LONG, int);
#if !defined(SEEK_SET)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* WMENUBMP.C */
BOOL  FAR PASCAL SetMenuItemBitmaps(HMENU,INT,UINT,HBITMAP,HBITMAP);
DWORD FAR PASCAL GetMenuCheckMarkDimensions(void);

/* WMESSAGE.c */
extern UINT  FAR PASCAL _PeekMessage(LPMSG);
extern UINT  FAR PASCAL _WinGetMessage(LPMSG);
extern BOOL    WINAPI PostMessage(HWND, UINT, WPARAM, LPARAM);
extern LRESULT WINAPI SendMessage(HWND, UINT, WPARAM, LPARAM);
extern BOOL  FAR PASCAL InSendMessage(void);
#ifdef STRICT
extern LRESULT WINAPI CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
#else
extern LRESULT WINAPI CallWindowProc(FARPROC, HWND, UINT, WPARAM, LPARAM);
#endif
extern INT     FAR PASCAL DispatchMessage(LPMSG);
extern BOOL    FAR PASCAL GetMessage(LPMSG,HWND,UINT,UINT);
extern LONG    FAR PASCAL GetMessageTime(void);
extern DWORD   FAR PASCAL GetMessagePos(void);
extern BOOL    FAR PASCAL PeekMessage(LPMSG,HWND,UINT,UINT,UINT);
extern VOID    FAR PASCAL WaitMessage(void);
extern INT     FAR PASCAL PostQuitMessage(int);
extern LRESULT FAR PASCAL DefWindowProc(HWND,UINT,WPARAM,LPARAM);

extern BOOL  FAR PASCAL UnhookWindowsHook(int, HOOKPROC);
#ifdef STRICT
HHOOK   WINAPI SetWindowsHook(int, HOOKPROC);
LRESULT WINAPI DefHookProc(int, WPARAM, LPARAM, HHOOK FAR*);
#else
HOOKPROC WINAPI SetWindowsHook(int, HOOKPROC);
LRESULT WINAPI DefHookProc(int, WPARAM, LPARAM, HOOKPROC FAR*);
#endif

#if defined(AFXNT)
HHOOK   WINAPI SetWindowsHookEx(int, HOOKPROC, HINSTANCE, DWORD);
#else
HHOOK   WINAPI SetWindowsHookEx(int, HOOKPROC, HINSTANCE, HTASK);
#endif
BOOL    WINAPI UnhookWindowsHookEx(HHOOK);
LRESULT WINAPI CallNextHookEx(HHOOK, int, WPARAM, LPARAM);

/* WMODULE.C */
extern HMODULE WINAPI GetModuleHandle(LPCSTR);
extern int     WINAPI GetModuleUsage(HINSTANCE);

/* WMOUAPI.c */
extern BOOL FAR PASCAL IsMouseInstalled(void);
#if !defined(MAC)
extern INT  FAR PASCAL ShowCursor(BOOL);
#endif
extern INT  FAR PASCAL MouseShow(void);
extern VOID FAR PASCAL GetCursorPos(LPPOINT);
extern VOID FAR PASCAL GetPixelCursorPos(LPPOINT);
extern VOID FAR PASCAL SetCursorPos(int, int);
extern INT  FAR PASCAL SetDoubleClickTime(int);
extern INT  FAR PASCAL GetDoubleClickTime(void);
extern VOID FAR PASCAL ClipCursor(LPRECT);
extern BOOL FAR PASCAL SwapMouseButton(BOOL);
#ifndef OS2
extern VOID     FAR PASCAL WinUseGraphicsMouse(void);
#endif /* OS2 */


/* WMSGBOX.c */
extern INT  FAR PASCAL MessageBox(HWND, LPCSTR, LPCSTR, UINT);

/* WMULDIV.C */
extern INT  FAR PASCAL MulDiv(int, int, int);

/* WPROCINS.C */
extern FARPROC WINAPI MakeProcInstance(FARPROC, HINSTANCE);
extern VOID    WINAPI FreeProcInstance(FARPROC);

/* WPROFILE.c */
extern INT  FAR PASCAL WriteProfileString(LPCSTR,LPCSTR,LPCSTR);
extern INT  FAR PASCAL WritePrivateProfileString(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
extern INT  FAR PASCAL GetProfileString(LPCSTR,LPCSTR,LPCSTR,LPSTR,int);
extern INT  FAR PASCAL GetPrivateProfileString(LPCSTR,LPCSTR,LPCSTR,LPSTR,int,LPCSTR);
extern UINT FAR PASCAL GetProfileInt(LPCSTR,LPCSTR,int);
extern UINT FAR PASCAL GetPrivateProfileInt(LPCSTR,LPCSTR,int,LPCSTR);
extern LPSTR FAR PASCAL env_parse(LPSTR, LPSTR);
extern VOID FAR PASCAL CloseProfile(void);

/* WREADINI.C */
extern VOID FAR PASCAL WinReadINI(void);

/* WSCRLBAR.c */
extern LONG FAR PASCAL ScrollBarWinProc(HWND,UINT,WPARAM,LPARAM);
extern HWND FAR PASCAL ScrollBarCreate(HWND,int,int,int,int,COLOR,DWORD,UINT);
extern VOID FAR PASCAL ShowScrollBar(HWND,int,BOOL);
extern INT  FAR PASCAL GetScrollPos(HWND,int);
extern INT  FAR PASCAL SetScrollPos(HWND,int,int,BOOL);
extern VOID FAR PASCAL GetScrollRange(HWND,int,int *,int *);
extern VOID FAR PASCAL SetScrollRange(HWND,int,int,int,BOOL);
extern VOID FAR PASCAL WinGetScrollbars(HWND,HWND *,HWND *);

/* WSETPRNT.c */
extern HWND FAR PASCAL SetParent(HWND,HWND);
extern HWND FAR PASCAL GetParent(HWND);
extern BOOL FAR PASCAL IsChild(HWND,HWND);

/* WSETRECT.c */
extern VOID FAR PASCAL WinSetRect(HWND,RECT);
extern INT  FAR PASCAL _WinSetClientRect(HWND);
extern VOID FAR PASCAL WinSetClientRect(HWND, LPRECT);
extern VOID FAR PASCAL GetWindowRect(HWND, LPRECT);
extern VOID FAR PASCAL WinGetClient(HWND, LPRECT);

/* WSHADOW.c */
#if defined(MEWEL_TEXT)
extern INT  FAR PASCAL WinDrawShadow(HWND);
#else
#define WinDrawShadow(hWnd)
#endif

/* WSHOWPOP.C */
extern VOID WINAPI ShowOwnedPopups(HWND, BOOL);

/* WSOUND2.C */
int WINAPI OpenSound(void);
void WINAPI CloseSound(void);
int WINAPI StartSound(void);
int WINAPI StopSound(void);
int WINAPI SetVoiceQueueSize(int, int);
int WINAPI SetVoiceNote(int, int, int, int);
int WINAPI SetVoiceAccent(int, int, int, int, int);
int WINAPI SetVoiceEnvelope(int, int, int);
int WINAPI SetVoiceSound(int, DWORD, int);
int WINAPI SetVoiceThreshold(int, int);
int FAR* WINAPI GetThresholdEvent(void);
int WINAPI GetThresholdStatus(void);
int WINAPI SetSoundNoise(int, int);


/* WSTFOCUS.C */
extern HWND FAR PASCAL GetFocus(void);
extern HWND FAR PASCAL SetFocus(HWND);

/* WSTRDISP.c */
extern VOID FAR PASCAL VidBlastToScreen(HDC,int,int,int,COLOR,LPSTR);

/* WSYSMENU.c */
extern HMENU FAR PASCAL GetSystemMenu(HWND, BOOL);

/* WVGAMAP.c */
extern VOID FAR PASCAL VioMapCharBitmap(UINT, UINT, LPSTR);
#if defined(MEWEL_TEXT) && !defined(EXTENDED_DOS)
extern BOOL FAR PASCAL VidInitVGAFonts(void);
extern VOID CDECL VidRestoreVGAFonts(void);
#else
#define VidInitVGAFonts()      (FALSE)
#define VidRestoreVGAFonts()   (FALSE)
#endif

 /*
  * UNIX gprof(1) shows that window creation/destruction is extremely time
  * consuming. In fact, most of that time is spent within lwmemset(), in
  * calls from the WinUpdateVismap() routine. The following is an attempt to
  * speed up window creation/destruction by inlining the lwmemset statements,
  * so that the optimizer can produce more efficient code.
  */
#if 0
#if !defined(VAXC)
#ifdef PARANOID /* allow checks by the malloc library support routines */
#define lwmemset(d, c, l) { \
			    register LPWORD dest = d;	\
			    register WORD cell = c;	\
			    register UINT len = l;	\
				memset(dest, 0, len * sizeof(WORD));	\
				while (len-- > 0)	\
				    *dest++ = cell;	\
			    }
#else
#define lwmemset(d, c, l) { \
			    register LPWORD dest = d;	\
			    register WORD cell = c;	\
			    register UINT len = l;	\
				while (len-- > 0)	\
				    *dest++ = cell;	\
			    }
#endif
#endif /* VAXC */
#endif


extern LPSTR FAR PASCAL lmemmove(LPSTR, LPSTR, UINT);
#if defined(DOS) || defined(OS2)
extern VOID  FAR PASCAL lmemshr(LPSTR, LPSTR, UINT);
#else
#define lmemshr memmove
#endif

#ifndef OS2
extern VOID  FAR CDECL DosSleep(int);
#endif
extern INT   FAR CDECL keyready(UINT);

#ifdef INTERNATIONAL_MEWEL
extern INT CDECL lang_upper(int);
#else
#define lang_upper(c)   toupper(c)
#endif

extern WORD FAR PASCAL GetClassWord(HWND, INT);
extern WORD FAR PASCAL SetClassWord(HWND, INT, WORD);
extern LONG FAR PASCAL GetClassLong(HWND, INT);
extern LONG FAR PASCAL SetClassLong(HWND, INT, LONG);

extern HANDLE FAR PASCAL GlobalAlloc(UINT, DWORD);
extern DWORD  FAR PASCAL GlobalCompact(DWORD);
extern HANDLE FAR PASCAL GlobalDiscard(HANDLE);
extern UINT   FAR PASCAL GlobalFlags(HANDLE);
extern HANDLE FAR PASCAL GlobalFree(HANDLE);
extern DWORD  FAR PASCAL GlobalHandle(UINT);
extern LPSTR  FAR PASCAL GlobalLock(HANDLE);
extern HANDLE FAR PASCAL GlobalLRUNewest(HANDLE);
extern HANDLE FAR PASCAL GlobalLRUOldest(HANDLE);
extern VOID   FAR PASCAL GlobalNotify(FARPROC);
extern HANDLE FAR PASCAL GlobalReAlloc(HANDLE, DWORD, UINT);
extern DWORD  FAR PASCAL GlobalSize(HANDLE);
extern BOOL   FAR PASCAL GlobalUnlock(HANDLE);
extern BOOL   FAR PASCAL GlobalUnWire(HANDLE);
extern LPSTR  FAR PASCAL GlobalWire(HANDLE);
extern HANDLE FAR PASCAL LocalAlloc(UINT, UINT);
extern UINT   FAR PASCAL LocalCompact(UINT);
extern HANDLE FAR PASCAL LocalDiscard(HANDLE);
extern UINT   FAR PASCAL LocalFlags(HANDLE);
extern HANDLE FAR PASCAL LocalFree(HANDLE);
#ifdef STRICT
extern HANDLE FAR PASCAL LocalHandle(VOID near *);
#else
extern HANDLE FAR PASCAL LocalHandle(UINT);
#endif


#if defined(__DPMI16__)
extern VOID _near* FAR PASCAL LocalLock(HANDLE);
#else
extern PSTR   FAR PASCAL LocalLock(HANDLE);
#endif

extern HANDLE FAR PASCAL LocalReAlloc(HANDLE, UINT, UINT);
extern UINT   FAR PASCAL LocalSize(HANDLE);
extern BOOL   FAR PASCAL LocalUnlock(HANDLE);
extern DWORD  FAR PASCAL GetFreeSpace(UINT);

/*
  Segment and data locking and unlocking functions
*/
HANDLE  FAR PASCAL LockSegment(UINT);
HANDLE  FAR PASCAL UnlockSegment(UINT);
#if defined(__DPMI16__) || defined(__DPMI32__)
#define LockData(dummy)     LockSegment((UINT)-1)
#define UnlockData(dummy)   UnlockSegment((UINT)-1)
#else
#ifdef __cplusplus
inline HGLOBAL FAR PASCAL LockData(UINT seg) { return (HGLOBAL) seg; }
inline VOID FAR PASCAL UnlockData(UINT seg)  { (void) seg; }
#else
#define LockData(seg)                (seg)
#define UnlockData(seg)              (TRUE)
#endif
#endif


/*
  WACTIVE.C
*/
extern HWND   FAR PASCAL GetActiveWindow(void);
extern HWND   FAR PASCAL SetActiveWindow(HWND);

/*
  WADJUST.C
*/
extern VOID   FAR PASCAL AdjustWindowRect(LPRECT, DWORD, BOOL);
extern VOID   FAR PASCAL AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD);

/*
  WINANSI.C
*/
VOID    WINAPI AnsiToOem(CONST char HUGEDATA *, char HUGEDATA *);
VOID    WINAPI OemToAnsi(CONST char HUGEDATA *, char HUGEDATA *);

VOID    WINAPI AnsiToOemBuff(LPCSTR, LPSTR, UINT);
VOID    WINAPI OemToAnsiBuff(LPCSTR, LPSTR, UINT);
LPSTR   WINAPI AnsiNext(LPCSTR);
LPSTR   WINAPI AnsiPrev(LPCSTR, LPCSTR);
LPSTR   WINAPI AnsiUpper(LPSTR);
LPSTR   WINAPI AnsiLower(LPSTR);
UINT    WINAPI AnsiUpperBuff(LPSTR, UINT);
UINT    WINAPI AnsiLowerBuff(LPSTR, UINT);

/*
  WANYPOP.C
*/
extern BOOL   FAR PASCAL AnyPopup(void);
extern HWND   FAR PASCAL GetLastActivePopup(HWND);

/*
  WINCATCH.C
*/
#if !defined(AFX)
extern INT    FAR PASCAL Catch(LPCATCHBUF);
extern VOID   FAR PASCAL Throw(LPCATCHBUF, INT);
#endif

/*
  WENUMOBJ.C
*/
#ifdef STRICT
typedef int (CALLBACK* GOBJENUMPROC)(void FAR*, LPARAM);
int     WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPARAM);
#else
typedef FARPROC GOBJENUMPROC;
int     WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPSTR);
#endif

/*
  WISCHAR.C
*/
extern BOOL   FAR PASCAL IsCharAlpha(UINT);
extern BOOL   FAR PASCAL IsCharAlphaNumeric(UINT);
extern BOOL   FAR PASCAL IsCharLower(UINT);
extern BOOL   FAR PASCAL IsCharUpper(UINT);

/*
  WINCARET.C
*/
extern VOID   FAR PASCAL CreateCaret(HWND, HBITMAP, INT, INT);
extern VOID   FAR PASCAL DestroyCaret(void);
extern VOID   FAR PASCAL HideCaret(HWND);
extern VOID   FAR PASCAL ShowCaret(HWND);
extern VOID   FAR PASCAL GetCaretPos(LPPOINT);
extern VOID   FAR PASCAL SetCaretPos(INT, INT);
extern UINT   FAR PASCAL GetCaretBlinkTime(void);
extern VOID   FAR PASCAL SetCaretBlinkTime(UINT);

/*
  WGDICLIP.C
*/
extern INT    FAR PASCAL ExcludeClipRect(HDC, INT, INT, INT, INT);
extern INT    FAR PASCAL IntersectClipRect(HDC, INT, INT, INT, INT);
extern INT    FAR PASCAL OffsetClipRect(HDC, INT, INT);
extern INT    FAR PASCAL OffsetClipRgn(HDC, INT, INT);
extern BOOL   FAR PASCAL PtVisible(HDC, INT, INT);
extern BOOL   FAR PASCAL RectVisible(HDC, CONST RECT FAR *);

/*
  WINDOS.C
*/
extern LPSTR  FAR PASCAL GetDOSEnvironment(void);
extern DWORD  FAR PASCAL GlobalDosAlloc(DWORD);
extern UINT   FAR PASCAL GlobalDosFree(UINT);

/*
  WINKBD.C
*/
extern BOOL FAR PASCAL EnableHardwareInput(BOOL);

/*
  WINVID.C
*/
extern INT   FAR PASCAL ToggleEGA(void);
extern INT   FAR PASCAL IsEGA(void);
extern INT   FAR PASCAL IsVGA(void);
extern INT   FAR PASCAL VidGetMode(void);
extern INT   FAR PASCAL VidGetCurrVideoMode(void);
extern VOID  FAR PASCAL VidSetVideoMode(int);
extern BOOL  FAR PASCAL VidSetBlinking(BOOL);

/*
  WINEVENT.C
*/
extern BOOL  FAR PASCAL GetInputState(void);

/*
  WINFLASH.C
*/
extern BOOL FAR PASCAL FlashWindow(HWND, BOOL);

/*
  WEXITWIN.C
*/
extern BOOL FAR PASCAL ExitWindows(DWORD, UINT);
extern VOID FAR PASCAL FatalAppExit(UINT, LPCSTR);

/*
  WMENUHI.C
*/
extern BOOL FAR PASCAL HiliteMenuItem(HWND, HMENU, UINT, UINT);

/*
  WSCROLDC.C
*/
extern BOOL   FAR PASCAL ScrollDC(HDC,INT,INT,CONST RECT FAR *,CONST RECT FAR *,HRGN,RECT FAR *);

/*
  WINDEFER.C
*/
DECLARE_HANDLE(HDWP);
HDWP    WINAPI BeginDeferWindowPos(int);
HDWP    WINAPI DeferWindowPos(HDWP, HWND, HWND, int, int, int, int, UINT);
BOOL    WINAPI EndDeferWindowPos(HDWP);

/*
  WINDEBUG.C
*/
#if defined(__DPMI32__)
extern VOID FAR PASCAL OutputDebugStringA(LPCSTR);
#define OutputDebugString  OutputDebugStringA
#else
extern VOID FAR PASCAL OutputDebugString(LPCSTR);
#endif

/*
  WSAVEDC.C
*/
extern BOOL   FAR PASCAL RestoreDC(HDC, INT);
extern INT    FAR PASCAL SaveDC(HDC);

/*
  WSPRINTF.C
  Note : These defs differ from the Windows defs, but we do not have
    anything like a _farsprintf function in the C run-time libraries.
*/
#if defined(M_I86LM) || defined(__LARGE__)
int FAR CDECL wsprintf(LPSTR lpszOut, LPCSTR lpszFmt, ...);
#else
int FAR CDECL wsprintf(PSTR, PCSTR, ...);
#endif
#if defined(M_I86MM) || defined(__MEDIUM__)
#define wvsprintf  vsprintf
#else
int WINAPI wvsprintf(LPSTR lpszOut, LPCSTR lpszFmt, CONST void FAR* lpParams);
#endif

/*
  More stubs...
*/
int WINAPI GetInstanceData(HINSTANCE, BYTE*, int);
HCURSOR WINAPI CopyCursor(HINSTANCE, HCURSOR);
HICON WINAPI CopyIcon(HINSTANCE, HICON);

/****************************************************************************/
/*                                                                          */
/* The following Windows functions are not implemented in MEWEL and         */
/* most are dummied out.                                                    */
/*                                                                          */
/****************************************************************************/
#if defined(MEWEL_GUI) || defined(XWINDOWS)
int     FAR PASCAL AddFontResource(LPCSTR);
HCURSOR FAR PASCAL LoadCursor(HINSTANCE, LPCSTR);
int     FAR PASCAL RemoveFontResource(LPCSTR);
HCURSOR FAR PASCAL CreateCursor(HINSTANCE, INT, INT, INT, INT, CONST VOID FAR *, CONST VOID FAR *);
BOOL    FAR PASCAL DestroyCursor(HCURSOR);
#else
#ifdef __cplusplus
inline int FAR PASCAL AddFontResource(LPCSTR lpFileName)
  { (void) lpFileName;  return 1; }
inline HCURSOR FAR PASCAL CreateCursor(HINSTANCE hInst,INT xHot, INT yHot, INT nWidth, INT nHeight, CONST VOID FAR *lpAND, CONST VOID FAR * lpXOR)
  {
    (void)hInst; (void)xHot; (void)yHot; (void)nWidth; (void)nHeight;
    (void)lpAND; (void)lpXOR;
    return 0;
  }
inline BOOL FAR PASCAL DestroyCursor(HCURSOR hCursor)
  {  (void) hCursor;    return TRUE; }
inline BOOL FAR PASCAL DestroyIcon(HICON hIcon)
  {  (void) hIcon;      return TRUE; }
inline VOID FAR PASCAL DrawFocusRect(HDC hDC, CONST RECT FAR *lpRect)
  { (void) hDC;   (void) lpRect;} 
inline HCURSOR  FAR PASCAL LoadCursor(HANDLE h, LPCSTR lpsz)
  { (void) h;   (void) lpsz;  return 0; } 
inline BOOL FAR PASCAL RemoveFontResource(LPCSTR lpszFont)
  { (void) lpszFont;    return TRUE; }
#else
#define AddFontResource(lpFileName)  (TRUE)
#define CreateCursor(hInst, xHot, yHot, nWidth, nHeight, lpAND, lpXOR)  (0)
#define DestroyCursor(hCursor)       (TRUE)
#define DestroyIcon(hIcon)           (TRUE)
#define DrawFocusRect(hDC,lpRect)
#define LoadCursor(hCursor, psz)     (0)
#define RemoveFontResource(lpszFont) (TRUE)
#endif
#endif

/*
  Palette Management stuff
*/
#if defined(XWINDOWS)
#define USE_PALETTES
#endif

#if (!defined(MEWEL_GUI) && !defined(XWINDOWS)) || !defined(USE_PALETTES)
#ifdef __cplusplus
inline VOID FAR PASCAL AnimatePalette(HPALETTE hPal, UINT i1, UINT i2, CONST PALETTEENTRY FAR *lpPE)
  { (void) hPal;  (void) i1;  (void) i2;  (void) lpPE; }
inline HPALETTE FAR PASCAL CreatePalette(CONST LOGPALETTE FAR *lp)
  { (void) lp;  return 0; }
inline UINT FAR PASCAL GetNearestPaletteIndex(HPALETTE hPal, DWORD rgb)
  { (void) hPal;  (void) rgb;  return 0; }
inline UINT FAR PASCAL GetPaletteEntries(HPALETTE hPal,UINT i1,UINT i2,LPPALETTEENTRY lppe)
  { (void) hPal;  (void) i1;  (void) i2;  (void) lppe;  return 0; } 
inline UINT FAR PASCAL GetSystemPaletteEntries(HDC hDC, UINT i1, UINT i2, LPPALETTEENTRY lpPE)
  { (void) hDC;  (void) i1;  (void) i2;  (void) lpPE;  return 0; }
inline UINT FAR PASCAL GetSystemPaletteUse(HDC hDC)
  { (void) hDC;  return 0; }
inline UINT FAR PASCAL RealizePalette(HDC hDC)
  { (void) hDC;  return TRUE; }
inline BOOL FAR PASCAL ResizePalette(HPALETTE hPal, UINT uSize)
  { (void) hPal;  (void) uSize;  return 0; }
inline HPALETTE FAR PASCAL SelectPalette(HDC hDC, HPALETTE hP, BOOL bErase)
  { (void) hDC;  (void) hP;  (void) bErase;  return 0; } 
inline UINT FAR PASCAL SetPaletteEntries(HPALETTE hPal,UINT i1,UINT i2,CONST PALETTEENTRY FAR *lppe)
  { (void) hPal;  (void) i1;   (void) i2;   (void) lppe;   return 0; } 
inline UINT FAR PASCAL SetSystemPaletteUse(HDC hDC, UINT f)
  { (void) hDC;   (void) f;  return 0; }
inline int FAR PASCAL UpdateColors(HDC hDC)
  { (void) hDC;   return TRUE; }
#else
#define AnimatePalette(hPal, i1, i2, lpPal)
#define CreatePalette(lp)                       (0)
#define GetNearestPaletteIndex(hPal, rgb)       (0)
#define GetPaletteEntries(hPal, i1, i2, lpPal)  (0)
#define GetSystemPaletteEntries(hDC,i1,i2,lppe) (0)
#define GetSystemPaletteUse(hDC)                (0)
#define RealizePalette(hDC)                     (1)
#define ResizePalette(hPal, size)               (0)
#define SelectPalette(hDC,hP,bErase)            (0)
#define SetPaletteEntries(hPal, i1, i2, lpPal)  (0)
#define SetSystemPaletteUse(hDC, f)             (0)
#define UpdateColors(hDC)                       (1)
#endif

#else
VOID     FAR PASCAL AnimatePalette(HPALETTE, UINT, UINT, CONST PALETTEENTRY FAR *);
HPALETTE FAR PASCAL CreatePalette(CONST LOGPALETTE FAR *);
UINT     FAR PASCAL GetNearestPaletteIndex(HPALETTE, DWORD);
UINT     FAR PASCAL GetPaletteEntries(HPALETTE,UINT,UINT,LPPALETTEENTRY);
UINT     FAR PASCAL GetSystemPaletteEntries(HDC,UINT,UINT,LPPALETTEENTRY);
UINT     FAR PASCAL GetSystemPaletteUse(HDC);
UINT     FAR PASCAL RealizePalette(HDC);
BOOL     FAR PASCAL ResizePalette(HPALETTE, UINT);
HPALETTE FAR PASCAL SelectPalette(HDC, HPALETTE, BOOL);
UINT     FAR PASCAL SetPaletteEntries(HPALETTE,UINT,UINT,CONST PALETTEENTRY FAR *);
UINT     FAR PASCAL SetSystemPaletteUse(HDC, UINT);
INT      FAR PASCAL UpdateColors(HDC);
#endif

BOOL   WINAPI IsDBCSLeadByte(BYTE);
LPARAM WINAPI GetMessageExtraInfo(void);
DWORD  WINAPI GetQueueStatus(UINT);
BOOL   WINAPI PostAppMessage(HTASK, UINT, WPARAM, LPARAM);
BOOL   WINAPI QuerySendMessage(HANDLE, HANDLE, HANDLE, LPMSG);
void   WINAPI ReplyMessage(LRESULT);
BOOL   WINAPI SetMessageQueue(int);



int   FAR PASCAL GetStretchBltMode(HDC);

DWORD FAR PASCAL ScaleViewportExt(HDC, int, int, int, int);
DWORD FAR PASCAL ScaleWindowExt(HDC, int, int, int, int);

HWND  FAR PASCAL GetSysModalWindow(void);
HWND  FAR PASCAL SetSysModalWindow(HWND);

int   FAR PASCAL SetTextJustification(HDC, int, int);
DWORD FAR PASCAL GetTabbedTextExtent(HDC, LPCSTR, int, int, LPINT);
int   FAR PASCAL SetTextCharacterExtra(HDC, int);
int   FAR PASCAL GetTextCharacterExtra(HDC);
LONG  FAR PASCAL TabbedTextOut(HDC, int, int, LPCSTR, int, int, LPINT, int);
BOOL  FAR PASCAL FloodFill( HDC, int, int, COLORREF);
BOOL  FAR PASCAL ExtFloodFill(HDC, int, int, COLORREF, UINT);
BOOL  FAR PASCAL PolyPolygon(HDC, CONST POINT FAR *, LPINT, int);

/*
  WGUIPCX.C
*/
extern INT FAR PASCAL DrawPCXFile(HDC, char *, INT, INT);

/*
  For printer support
*/
void FAR PASCAL InstallPrinterSupport(void);


#if defined(AFXNT)

/*
  VC32++/NT defs
*/

/*
  Messages
*/
#define WM_CTLCOLORMSGBOX               0x0132
#define WM_CTLCOLOREDIT                 0x0133
#define WM_CTLCOLORLISTBOX              0x0134
#define WM_CTLCOLORBTN                  0x0135
#define WM_CTLCOLORDLG                  0x0136
#define WM_CTLCOLORSCROLLBAR            0x0137
#define WM_CTLCOLORSTATIC               0x0138
#define WM_MDIREFRESHMENU               0x0234
#define EM_SCROLLCARET                  (WM_USER+40)


#ifdef __cplusplus
inline DWORD FAR PASCAL GetWindowThreadProcessId(HWND hWnd, LPDWORD lpID)
{
  if (lpID)
    *lpID = 0;
  return 0;
}
inline DWORD FAR PASCAL GetCurrentThreadId(void)
{
  return 0;
}
#else
#define GetWindowThreadProcessId(h,p)  GetWindowTask(h)
#define GetCurrentThreadId()           GetCurrentTask()
#endif


#define CharToOemA                CharToOem
#define LoadStringA               LoadString
#define OemToCharA                OemToChar


/*
  For DEVMODE...
*/
#include <drivinit.h>

HDC WINAPI ResetDC(HDC, CONST DEVMODE FAR *);


/*
  Structures from winbase.h
*/
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

BOOL WINAPI FileTimeToSystemTime(CONST FILETIME *lpFileTime,
                                 LPSYSTEMTIME lpSystemTime);


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

typedef DLGTEMPLATE *LPDLGTEMPLATEA;
typedef DLGTEMPLATE *LPDLGTEMPLATEW;
#ifdef UNICODE
typedef LPDLGTEMPLATEW LPDLGTEMPLATE;
#else
typedef LPDLGTEMPLATEA LPDLGTEMPLATE;
#endif /* UNICODE */
typedef CONST DLGTEMPLATE *LPCDLGTEMPLATEA;
typedef CONST DLGTEMPLATE *LPCDLGTEMPLATEW;
#ifdef UNICODE
typedef LPCDLGTEMPLATEW LPCDLGTEMPLATE;
#else
typedef LPCDLGTEMPLATEA LPCDLGTEMPLATE;
#endif /* UNICODE */


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

typedef DLGITEMTEMPLATE *PDLGITEMTEMPLATEA;
typedef DLGITEMTEMPLATE *PDLGITEMTEMPLATEW;
#ifdef UNICODE
typedef PDLGITEMTEMPLATEW PDLGITEMTEMPLATE;
#else
typedef PDLGITEMTEMPLATEA PDLGITEMTEMPLATE;
#endif /* UNICODE */
typedef DLGITEMTEMPLATE *LPDLGITEMTEMPLATEA;
typedef DLGITEMTEMPLATE *LPDLGITEMTEMPLATEW;
#ifdef UNICODE
typedef LPDLGITEMTEMPLATEW LPDLGITEMTEMPLATE;
#else
typedef LPDLGITEMTEMPLATEA LPDLGITEMTEMPLATE;
#endif /* UNICODE */


/*
  GetObjectType stuff
*/
DWORD FAR PASCAL GetObjectType(HGDIOBJ hObj);
#define OBJ_BITMAP  5

/*
  VirtualProtect (used by MFC/NT)
*/
#ifdef __cplusplus
inline BOOL WINAPI VirtualProtect(LPVOID lpAddress, DWORD dwSize,
                                  DWORD flNewProtect, LPDWORD lpflOldProtect)
{ return TRUE; }
#else
#define VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect)  (1)
#endif
#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     
#define PAGE_GUARD            0x100     
#define PAGE_NOCACHE          0x200     
#define MEM_COMMIT           0x1000     
#define MEM_RESERVE          0x2000     
#define MEM_DECOMMIT         0x4000     
#define MEM_RELEASE          0x8000     
#define MEM_FREE            0x10000     
#define MEM_PRIVATE         0x20000     
#define MEM_MAPPED          0x40000     
#define MEM_TOP_DOWN       0x100000     
#define SEC_FILE           0x800000     
#define SEC_IMAGE         0x1000000     
#define SEC_RESERVE       0x4000000     
#define SEC_COMMIT        0x8000000     
#define SEC_NOCACHE      0x10000000     
#define MEM_IMAGE         SEC_IMAGE     


/*
  Shell API stuff needed for MFC/NT ... stubbed out in WMFCSTUB.C
*/
DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;
#define HKEY_CLASSES_ROOT           (( HKEY ) 0x80000000 )
#define REG_SZ                      (1)
#define ERROR_SUCCESS               (0)
LONG APIENTRY RegSetValue(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, 
                          LPCSTR lpData, DWORD cbData);
LONG APIENTRY RegQueryValue(HKEY hKey, LPCSTR lpSubKey, LPSTR lpValue,
                            LONG *lpcbValue);
/*
  The NT version of windows.h includes some files which contain
  defs required by MFC/NT
*/
#include <stdarg.h>

#endif /* AFXNT */

#if defined(UNIX)
/* ---------------------------------------------------------------------------
 * DOS file i/o functions used by MFC. (WOWLSTUB.C)
 * -------------------------------------------------------------------------*/
#include <dirent.h>

#if defined(OLD_HEADERS)
extern int fgetpos(FILE *, fpos_t *);
extern unsigned long int strtoul(const char *, char **, int);
#endif

typedef struct _find_t {
    char reserved[21];
    int attrib;
    UINT wr_time;
    UINT wr_date;
    long size;
    char name[256];
    /* Specific to UNIX implementation */
    char pattern[256];
    int mask;
    DIR* dirp;
    } FINDT, FAR *LPFINDT;

typedef struct _diskfree_t {
    UINT total_clusters;
    UINT avail_clusters;
    UINT sectors_per_cluster;
    UINT bytes_per_sector;
    } DISKFREE, FAR *LPDISKFREET;

#define	_A_NORMAL  0x01
#define	_A_RDONLY  0x02
#define	_A_HIDDEN  0x04
#define	_A_SYSTEM  0x08
#define	_A_VOLID   0x10
#define	_A_SUBDIR  0x20
#define	_A_ARCH    0x40

UINT _dos_creat(LPCSTR, UINT, INT*);
UINT _dos_open(LPCSTR, UINT, INT*);
UINT _dos_read(INT, VOID*, UINT, UINT*);
UINT _dos_write(INT, const void*, UINT, UINT*);
UINT _dos_commit(INT);
UINT _dos_close(INT);
UINT _dos_findfirst(LPCSTR, UINT, LPFINDT);
UINT _dos_findnext(LPFINDT);
VOID _dos_findclose(LPFINDT);
UINT _dos_getfileattr(LPCSTR, UINT*);
UINT _dos_setfileattr(LPCSTR, UINT);
UINT _dos_getftime(INT, UINT*, UINT*);
UINT _dos_setftime(INT, UINT, UINT);
/* void _dos_getdrive(UINT*); */
/* UINT _dos_getdiskfree(UINT, LPDISKFREET); */

/* ---------------------------------------------------------------------------
 * Functions used by OWL. (WOWLSTUB.C)
 * -------------------------------------------------------------------------*/
int WINAPI GetHwndObject(HWND);
int WINAPI SetHwndObject(HWND, int);
#endif /* UNIX */
