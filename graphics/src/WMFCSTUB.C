/*===========================================================================*/
/*                                                                           */
/* File    : WMFCSTUB.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#if defined(__TURBOC__)
#pragma warn -par
#endif

int FAR PASCAL WinHelp(HWND hwndMain, LPCSTR lpszHelp, UINT usCommand, 
                       DWORD ulData)
{
  return 0;
}


#if 0
#include <shellapi.h>
#else
#define ERROR_SUCCESS   0L
#define HDROP           HANDLE
typedef DWORD           HKEY;
typedef HKEY FAR *      PHKEY;
#endif

LONG WINAPI RegSetValue(HKEY hKey, LPCSTR lpszSubKey, DWORD fdwType,
                        LPCSTR lpszValue, DWORD cb)
{
  return ERROR_SUCCESS;
}

LONG WINAPI RegQueryValue(HKEY hKey, LPCSTR lpszSubKey, LPSTR lpszValue,
                          LONG FAR* lpcb)
{
  return ERROR_SUCCESS;
}

UINT WINAPI DragQueryFile(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cb)
{
  return 0;
}

UINT WINAPI DragQueryFileA(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cb)
{
  return 0;
}

void WINAPI DragFinish(HDROP hDrop)
{
}

void WINAPI DragAcceptFiles(HWND hWnd, BOOL fAccept)
{
  (void) hWnd;
  (void) fAccept;
}

BOOL WINAPI DragQueryPoint(HDROP hDrop, POINT FAR* lppt)
{
  (void) hDrop;
  lppt->x = lppt->y = 0;
  return TRUE;
}


#if 0
#include <print.h>      /* for ResetDC */
#else
#define CCHDEVICENAME 32
#define CCHPAPERNAME  64

typedef struct tagDEVMODE
{
    char  dmDeviceName[CCHDEVICENAME];
    UINT  dmSpecVersion;
    UINT  dmDriverVersion;
    UINT  dmSize;
    UINT  dmDriverExtra;
    DWORD dmFields;
    int   dmOrientation;
    int   dmPaperSize;
    int   dmPaperLength;
    int   dmPaperWidth;
    int   dmScale;
    int   dmCopies;
    int   dmDefaultSource;
    int   dmPrintQuality;
    int   dmColor;
    int   dmDuplex;
    int   dmYResolution;
    int   dmTTOption;
} DEVMODE;
typedef DEVMODE* PDEVMODE, NEAR* NPDEVMODE, FAR* LPDEVMODE;
#endif

HDC WINAPI ResetDC(HDC hDC, CONST DEVMODE FAR *lpdm)
{
  (void) lpdm;
  return hDC;
}

VOID FAR PASCAL FatalAppExit(UINT nCode, LPCSTR s)
{
  MessageBox(HWND_DESKTOP, s, NULL, MB_OK | MB_ICONEXCLAMATION);
  exit(nCode);
}


BOOL FAR PASCAL FastWindowFrame(HDC hDC, LPRECT lpRect, 
                                int xWidth, int yWidth, long rop)
{
  int  width  = (lpRect->right - lpRect->left) - xWidth;
  int  height = (lpRect->bottom - lpRect->top) - yWidth;

  (void) rop;

  PatBlt(hDC, lpRect->left, lpRect->top, xWidth, height, PATCOPY);
  PatBlt(hDC, xWidth, lpRect->top, width, yWidth, PATCOPY);
  PatBlt(hDC, lpRect->left, height, width, yWidth, PATCOPY);
  PatBlt(hDC, width, yWidth, xWidth, height, PATCOPY);
  return TRUE;
}


/*
  Set/GetKeyboardState is used by OWL 2.0 in TINYCAP.CPP
*/
VOID FAR PASCAL GetKeyboardState(BYTE FAR *lpbKeyState)
{
  /*
    A key is 0x80 if it is down, and it is 0x00 if it is up.
    For a toggle key, the value is 0x01 if it in on and 0x00
    if it is off.
  */
  lmemset(lpbKeyState, 0, 256);
}

VOID FAR PASCAL SetKeyboardState(BYTE FAR *lpbKeyState)
{
  /*
    This function is supposed to set the LEDs and BIOS state of the lock
    key (scroll, num, and caps). Look at the VK_SCROLL, VK_CAPITAL, and
    VK_NUMLOCK virtual-key codes.
  */
}

#if defined(UNIX)
/* ---------------------------------------------------------------------------
 * DOS file i/o functions used by MFC.
 * -------------------------------------------------------------------------*/
#define	CURRDIR	 "."

static BOOL _wildmatch(LPCSTR, LPCSTR);

/* ---------------------------------------------------------------------------
 * _dos_creat - Xreates a new file or overwrites an existing one.
 * -------------------------------------------------------------------------*/
UINT _dos_creat(LPCSTR pathname, UINT attrib, INT* handlep)
{
  INT iAttrib = 0; /* Normal file */
  HFILE fd;

  /*
     Map DOS attribute to Windows attribute
     NOTE: DOS allows OR'ing, Windows needs 0, 1, 2 or 3
  */
  if (attrib & _A_RDONLY)
    iAttrib = 1; /* Read-only file */
  else if (attrib & _A_HIDDEN)
    iAttrib = 2; /* Hidden file */
  else if (attrib & _A_SYSTEM)
    iAttrib = 3; /* System file */

  if ( (fd = _lcreat(pathname, iAttrib)) == HFILE_ERROR )
    return 1;

  *handlep = (INT)fd;
  return 0;
    return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_open - Opens a file for reading or writing.
 * -------------------------------------------------------------------------*/
UINT _dos_open(LPCSTR pathname, UINT oflags, INT* handlep)
{
  INT iAttrib;
  HFILE fd;

  /* Map DOS flags to Windows equivalents */
  switch (oflags & 0x000F) {
    case O_RDONLY: iAttrib = READ; break;
    case O_WRONLY: iAttrib = WRITE; break;
    default: iAttrib = READ_WRITE; break; }
  
  if ( (fd = _lopen(pathname, iAttrib)) == HFILE_ERROR )
    return 1;

  *handlep = (INT)fd;
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_read - Reads a number of bytes from a file handle.
 * -------------------------------------------------------------------------*/
UINT _dos_read(INT handle, VOID* bufp, UINT count, UINT* nreadp)
{
  UINT n;

  if ( (n = _lread(handle, bufp, count)) == HFILE_ERROR )
    return 1;

  *nreadp = n;
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_write - Writes a number of bytes to a file handle.
 * -------------------------------------------------------------------------*/
UINT _dos_write(INT handle, const void* bufp, UINT count, UINT* nwritep)
{
  UINT n;

  if ( (n = _lwrite(handle, bufp, count)) == HFILE_ERROR )
    return 1;

  *nwritep = n;
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_commit - Flushes file output to disk.
 * -------------------------------------------------------------------------*/
UINT _dos_commit(INT handle)
{
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_close - Closes the file associated with a given handle.
 * -------------------------------------------------------------------------*/
UINT _dos_close(INT handle)
{
  return _lclose((HFILE)handle);
}

/* ---------------------------------------------------------------------------
 * _dos_getfileattr - Retrieves the file attributes of a given path.
 * -------------------------------------------------------------------------*/
UINT _dos_getfileattr(LPCSTR pathname, UINT* attribp)
{
  struct stat st;
  UINT ret;

  if ( (ret = stat(pathname, &st)) == 0)
  {
    /* Map UNIX file attributes to their DOS counterparts */
    if ((st.st_mode & S_IFMT) == S_IFDIR)
      *attribp = _A_SUBDIR;
    else if ((st.st_mode & S_IFMT) == S_IFREG)
      *attribp = _A_NORMAL;
    if ((st.st_mode & S_IREAD) && (st.st_mode & S_IWRITE) == 0)
      *attribp |= _A_RDONLY;
  }
  return ret;
}

/* ---------------------------------------------------------------------------
 * _dos_setfileattr - Sets the file attributes of a given path.
 * -------------------------------------------------------------------------*/
UINT _dos_setfileattr(LPCSTR lpPath, UINT attr)
{
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_getftime - Gets the file date and time.
 * -------------------------------------------------------------------------*/
UINT _dos_getftime(INT handle, UINT* datep, UINT* timep)
{
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_setftime - Sets the file date and time.
 * -------------------------------------------------------------------------*/
UINT _dos_setftime(INT handle, UINT date, UINT time)
{
  return 0;
}

/* ---------------------------------------------------------------------------
 * _dos_findfirst - MSC directory access routines.
 * _dos_findnext    Contributed by W.Z. Venema (The Netherlands)
 * _dos_findclose
 * -------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------
 * _dos_findfirst - Find first file that matches pattern
 * -------------------------------------------------------------------------*/
UINT _dos_findfirst(LPCSTR pathname, UINT attrib, LPFINDT ffblk)
{
  /* Initialize the directory search. */
  if ((ffblk->dirp = opendir(CURRDIR)) == NULL)
     return 2; /* ENOENT */

  ffblk->mask = attrib;
  strcpy(ffblk->pattern, pathname);

  return (_dos_findnext(ffblk));
}

/* ---------------------------------------------------------------------------
 * _dos_findnext - Find next file that matches pattern
 * -------------------------------------------------------------------------*/
UINT _dos_findnext(LPFINDT ffblk)
{
  struct dirent *de;
  struct stat st;
  UINT   attrib;

  /* Skip files whose name does not match or whose status does not exist. */

  while (de = readdir(ffblk->dirp))
  {
    if (_wildmatch(de->d_name, ffblk->pattern) &&
        _dos_getfileattr(de->d_name, &attrib) == 0 &&
        ffblk->mask & attrib)
    {
      strcpy(ffblk->name, de->d_name);
      stat(de->d_name, &st);
      ffblk->size = st.st_size;
      ffblk->attrib = attrib;
      return 0;
    }
  }
  return 1;
}

/* ---------------------------------------------------------------------------
 * _dos_findclose - Free directory search resources
 * -------------------------------------------------------------------------*/

VOID _dos_findclose(LPFINDT ffblk)
{
  if (ffblk->dirp)
  {
    closedir(ffblk->dirp);
    ffblk->dirp = NULL;
  }
}

/* ---------------------------------------------------------------------------
 * _wildmatch - Wildcard matching routine by Karl Heuer.
 * Test whether string s is matched by pattern p.
 * Supports "?", "*", "[", each of which may be escaped with "\";
 * -------------------------------------------------------------------------*/

BOOL _wildmatch(LPCSTR s, LPCSTR p)
{
  char c;

  while ((c = *p++) != '\0')
  {
    switch (c)
    {
      case '?':
        if (*s++ == '\0')
          return FALSE;
        break;

      case '[':
        {
        BOOL wantit = TRUE;
        BOOL seenit = FALSE;

        if (*p == '!')
        {
          wantit = FALSE;
          ++p;
        }
        c = *p++;

        do {
          if (c == '\0')
            return FALSE;
          if (*p == '-' && p[1] != '\0')
          {
            if (*s >= c && *s <= p[1])
              seenit = TRUE;
            p += 2;
          } else
          {
            if (c == *s)
              seenit = TRUE;
          }
        } while ((c = *p++) != ']');

        if (wantit != seenit)
          return FALSE;
        ++s;
        }
        break;

      case '*':
        if (*p == '\0')
          return TRUE;  /* optimize common case */

        do {
          if (_wildmatch(s, p))
            return TRUE;
        } while (*s++ != '\0');

        return FALSE;

      case '\\':
        if (*p == '\0' || *p++ != *s++)
          return FALSE;
        break;

      default:
        if (c != *s++)
          return FALSE;
    }
  }
  return (*s == '\0');
}
#endif /* UNIX */
