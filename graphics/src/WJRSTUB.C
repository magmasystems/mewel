#include "wprivate.h"
#include "window.h"

#ifdef NOALT
int nAltKeyPresses = 0;
char MEWEL_NoInt9 = 0;
void Int9Init(void)
{
}
void Int9Terminate(void)
{
}
#endif


#ifdef NOCOMBOBOXES
LONG FAR PASCAL ComboBoxWndProc(hWnd, message, wParam, lParam)
  HWND hWnd;
  WORD message;
  WORD wParam;
  LONG lParam;
{
  return 0L;
}
HWND FAR PASCAL ComboBoxCreate(hParent, row1,col1,row2,col2, title,attr,flags,id)
  HWND  hParent;
  int   row1, row2, col1, col2;
  PSTR  title;
  COLOR attr;
  DWORD flags;
  WORD  id;
{
  return NULLHWND;
}
#endif


#ifdef NODC
LPHDC FAR PASCAL _GetDC(HDC hDC)
{
  return (LPHDC) NULL;
}
COLOR FAR PASCAL RGBtoAttr(DWORD clrText)
{
  return 0;
}
#endif


#ifdef NOGRAPHICS
VOID FAR PASCAL ATIPageSelect(WORD iPage)
{
}
VOID FAR PASCAL FAR VGAWrtGraphicsChar(WORD ch, int x, int y, WORD attr)
{
}
VOID FAR PASCAL FAR _VGAVioWrtCellStr(LPWORD lpCell, int n, int x, int y, int iRes)
{
}
#endif


#ifdef NOICONS
HICON FAR PASCAL LoadIcon(hModule, idIcon)
  WORD hModule;
  LPSTR idIcon;
{
  return 0;
}
BOOL FAR PASCAL DrawIcon(hDC, x, y, hIcon)
  HDC   hDC;
  int   x, y;
  HICON hIcon;
{
  return FALSE;
}
BOOL FAR PASCAL IsIconic(hWnd)
  HWND hWnd;
{
  return FALSE;
}
BOOL FAR PASCAL OpenIcon(hWnd)
  HWND hWnd;
{
  return FALSE;
}
#endif


#ifdef NOMDI
int FAR PASCAL MDIInitialize()
{
  return FALSE;
}
#endif


#ifdef NOMINMAX
BOOL bIsaWindowZoomed = FALSE;    /* TRUE if there's a zoomed window */
BOOL FAR PASCAL WinZoom(hWnd)
  HWND hWnd;
{
  return FALSE;
}
BOOL FAR PASCAL IsZoomed(hWnd)
  HWND hWnd;
{
  return FALSE;
}
BOOL FAR PASCAL WinMinimize(hWnd)
  HWND hWnd;
{
  return FALSE;
}
#endif


#ifdef NORUBBERBANDING
int FAR PASCAL WinRubberband(hWnd, message, origMouseRow, origMouseCol, iSizeMode)
  HWND hWnd;
  WORD message;   /* WM_MOVE or WM_SIZE */
  int  origMouseRow;
  int  origMouseCol;
  int  iSizeMode;
{
  return 0;
}
#endif



#ifdef NOTIMERS
BOOL FAR PASCAL SetTimer(hWnd, id, interval, timerFunc)
  HWND hWnd;
  WORD id;
  WORD interval;
  FARPROC timerFunc;
{
  return FALSE;
}
FAR PASCAL KillTimer(hWnd, id)
  HWND hWnd;
  WORD id;
{
  return FALSE;
}
VOID FAR PASCAL KillWindowTimers(hWnd)
  HWND hWnd;
{
}
FAR PASCAL TimerCheck(event)
  LPMSG event;
{
  return FALSE;
}
#endif


#ifdef NOOBJECTS
HANDLE FAR PASCAL GetStockObject(iObject)
  int iObject;
{
  return 0;
}
BOOL FAR PASCAL LPtoDP(HDC hDC, LPPOINT lpPt, int nCount)
{
  return TRUE;
}
int FAR PASCAL GetObject(hObject, nCount, lpObject)
  HANDLE hObject;
  int    nCount;
  LPSTR  lpObject;
{
  return 0;
}
#endif

#ifdef NORESOURCES
HMENU FAR PASCAL LoadMenu(hModule, idMenu)
  WORD hModule;
  LPSTR idMenu;
{
  return NULLHWND;
}
HWND FAR PASCAL LoadDialog(hModule, idDialog, hParent, dlgfunc)
  WORD hModule;
  LPSTR idDialog;
  HWND hParent;
  DLGPROC *dlgfunc;
{
  return NULLHWND;
}
HWND FAR PASCAL CreateDialogIndirect(hInst, lpDlg, hParent, dlgfunc)
  HANDLE hInst;
  LPSTR  lpDlg;
  HWND   hParent;
  FARPROC dlgfunc;
{
  return NULLHWND;
}
WORD FAR PASCAL OpenResourceFile(szFileName)
  BYTE *szFileName;
{
  return 0;
}
void FAR PASCAL CloseResourceFile(hModule)
  WORD hModule;
{
}
#endif
