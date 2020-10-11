/*===========================================================================*/
/*                                                                           */
/* File    : WGDISTUB.H                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

 
DWORD   PASCAL GlobalSize(GLOBALHANDLE);
DWORD   PASCAL GlobalCompact(DWORD dwMemFree);
HDC     PASCAL CreateCompatibleDC(HDC);
HBITMAP PASCAL CreateBitmap(int, int, BYTE, BYTE, LPSTR);
BOOL    PASCAL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);

#undef GetWindowTask
HANDLE PASCAL GetWindowTask(HWND);
void PASCAL WaitMessage(void);

BOOL  FAR PASCAL RoundRect(HDC, int, int, int, int, int, int);
BOOL  FAR PASCAL StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
int   FAR PASCAL SetStretchBltMode(HDC, int);
DWORD FAR PASCAL GetCurrentPosition(HDC);
HBITMAP FAR PASCAL CreateBitmapIndirect(BITMAP FAR *);
LONG  FAR PASCAL GetBitmapBits(HBITMAP, LONG, LPSTR);


VOID FAR PASCAL LineDDA(int, int, int, int, FARPROC, LPSTR);

BOOL  FAR PASCAL PatBlt(HDC, int, int, int, int, DWORD);
BOOL  FAR PASCAL Chord(HDC, int, int, int, int, int, int, int, int);

BOOL FAR PASCAL FillRgn(HDC, HRGN, HBRUSH);
BOOL FAR PASCAL FrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL FAR PASCAL InvertRgn(HDC, HRGN);
BOOL FAR PASCAL PaintRgn(HDC, HRGN);

int  FAR PASCAL CombineRgn(HRGN, HRGN, HRGN, int);
BOOL FAR PASCAL EqualRgn(HRGN, HRGN);
int  FAR PASCAL OffsetRgn(HRGN, int, int);
int  FAR PASCAL GetRgnBox(HRGN, LPRECT);
BOOL FAR PASCAL PtInRegion(HRGN, int, int);
BOOL FAR PASCAL RectInRegion(HRGN, LPRECT);
HRGN FAR PASCAL CreateEllipticRgnIndirect(LPRECT);
HRGN FAR PASCAL CreateEllipticRgn(int, int, int, int);
HRGN FAR PASCAL CreatePolygonRgn(LPPOINT, int, int);
HRGN FAR PASCAL CreatePolyPolygonRgn(LPPOINT, LPINT, int, int);
HRGN FAR PASCAL CreateRoundRectRgn(int, int, int, int, int, int);
HRGN FAR PASCAL CreateRectRgnIndirect(LPRECT);
void FAR PASCAL SetRectRgn(HRGN, int, int, int, int);

int     FAR PASCAL GetClipBox(HDC, LPRECT);
int     FAR PASCAL EnumFonts(HDC, LPSTR, FARPROC, LPSTR);
DWORD   FAR PASCAL SetMapperFlags(HDC, DWORD);
INT     FAR PASCAL GetTextFace(HDC, int, LPSTR);
FARPROC FAR PASCAL GetProcAddress(HANDLE, LPSTR);
HANDLE	FAR PASCAL LoadLibrary(LPSTR);
void	FAR PASCAL FreeLibrary(HANDLE);


/* System bitmaps */
#define OBM_CLOSE	    32754
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

typedef struct tagBITMAPCOREHEADER {
	DWORD	bcSize;			/* used to get to color table */
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR *LPBITMAPCOREHEADER;
typedef BITMAPCOREHEADER *PBITMAPCOREHEADER;

#define	CBM_INIT	0x04L	/* initialize bitmap */

#define	DIB_RGB_COLORS	0	/* color table in RGBTriples */
#define	DIB_PAL_COLORS	1	/* color table in palette indices */

/* constants for the biCompression field */
#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L

HBITMAP FAR PASCAL CreateCompatibleBitmap(HDC, int, int);
HBITMAP FAR PASCAL CreateDIBitmap(HDC,LPBITMAPINFOHEADER,DWORD,LPSTR,
						LPBITMAPINFO,WORD);

void PASCAL OutputDebugString(LPSTR);
void PASCAL FillWindow(HWND hParent, HWND hWnd, HDC hDC, HBRUSH hBrush);

#undef GetCurrentTime
DWORD FAR PASCAL GetCurrentTime(void);

