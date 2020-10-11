#ifndef _WIN31_H
#define _WIN31_H

#if (WINVER >= 0x030a)

#ifdef __cplusplus
extern "C" {
#endif

BOOL    WINAPI QueryAbort(HDC, int);

void    WINAPI MapWindowPoints(HWND hwndFrom, HWND hwndTo, POINT FAR* lppt, UINT cpt);

HDC     WINAPI GetDCEx(register HWND hwnd, HRGN hrgnClip, DWORD flags);
#define DCX_WINDOW	    0x00000001L
#define DCX_CACHE	    0x00000002L
#define DCX_CLIPCHILDREN    0x00000008L
#define DCX_CLIPSIBLINGS    0x00000010L
#define DCX_PARENTCLIP	    0x00000020L
#define DCX_EXCLUDERGN	    0x00000040L
#define DCX_INTERSECTRGN    0x00000080L
#define DCX_LOCKWINDOWUPDATE 0x00000400L
#define DCX_USESTYLE	    0x00010000L

int     WINAPI ScrollWindowEx(HWND hwnd, int dx, int dy,
                const RECT FAR* prcScroll, const RECT FAR* prcClip,
                HRGN hrgnUpdate, RECT FAR* prcUpdate, UINT flags);
#define SW_SCROLLCHILDREN   0x0001
#define SW_INVALIDATE       0x0002
#define SW_ERASE            0x0004

BOOL    WINAPI LockWindowUpdate(HWND hwndLock);

/* Drawing bounds accumulation APIs */
UINT    WINAPI SetBoundsRect(HDC hDC, const RECT FAR* lprcBounds, UINT flags);
UINT    WINAPI GetBoundsRect(HDC hDC, RECT FAR* lprcBounds, UINT flags);


typedef FARPROC ABORTPROC;
typedef FARPROC GRAYSTRINGPROC;
typedef FARPROC TIMERPROC;


/* Static Control Mesages */
#define STM_SETICON	    (WM_USER+0)
#define STM_GETICON	    (WM_USER+1)


BOOL    WINAPI RedrawWindow(HWND hwnd, const RECT FAR* lprcUpdate, HRGN hrgnUpdate, UINT flags);
#define RDW_INVALIDATE		0x0001
#define RDW_INTERNALPAINT	0x0002
#define RDW_ERASE		0x0004
#define RDW_VALIDATE		0x0008
#define RDW_NOINTERNALPAINT	0x0010
#define RDW_NOERASE		0x0020
#define RDW_NOCHILDREN		0x0040
#define RDW_ALLCHILDREN 	0x0080
#define RDW_UPDATENOW		0x0100
#define RDW_ERASENOW		0x0200
#define RDW_FRAME               0x0400
#define RDW_NOFRAME             0x0800


/* EnableScrollBar() flags */
BOOL    WINAPI EnableScrollBar(HWND, int, UINT);
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003
#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002
#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002
#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT


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

BOOL    WINAPI GetWindowPlacement(HWND, WINDOWPLACEMENT FAR*);
BOOL    WINAPI SetWindowPlacement(HWND, const WINDOWPLACEMENT FAR*);

/* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP	    0x0010
#define WVR_ALIGNLEFT	    0x0020
#define WVR_ALIGNBOTTOM     0x0040
#define WVR_ALIGNRIGHT	    0x0080
#define WVR_HREDRAW	    0x0100
#define WVR_VREDRAW	    0x0200
#define WVR_REDRAW	    (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS	    0x0400

/* WM_NCCALCSIZE parameter structure */
typedef struct tagNCCALCSIZE_PARAMS
{
    RECT	   rgrc[3];
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


/*
  Printing support
*/
typedef struct
{
    int     cbSize;
    LPCSTR  lpszDocName;
    LPCSTR  lpszOutput;
}   DOCINFO;
typedef DOCINFO FAR* LPDOCINFO;

int     WINAPI StartDoc(HDC, DOCINFO FAR*);
int     WINAPI StartPage(HDC);
int     WINAPI EndPage(HDC);
int     WINAPI EndDoc(HDC);
int     WINAPI AbortDoc(HDC);

int     WINAPI SetAbortProc(HDC, ABORTPROC);
HANDLE  WINAPI SpoolFile(LPSTR, LPSTR, LPSTR, LPSTR);

#ifndef NOSCALABLEFONT
DWORD   WINAPI GetFontData(HDC, DWORD, DWORD, void FAR*, DWORD);
BOOL	WINAPI CreateScalableFontResource(UINT, LPCSTR, LPCSTR, LPCSTR);

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

DWORD   WINAPI GetGlyphOutline(HDC, UINT, UINT, GLYPHMETRICS FAR*, DWORD, void FAR*, const MAT2 FAR*);

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

BOOL    WINAPI GetCharABCWidths(HDC, UINT, UINT, ABC FAR*);

typedef struct tagKERNINGPAIR
{
    WORD wFirst;
    WORD wSecond;
    int  iKernAmount;
} KERNINGPAIR, FAR* LPKERNINGPAIR;

int     WINAPI GetKerningPairs(HDC, int, KERNINGPAIR FAR*);

typedef struct tagRASTERIZER_STATUS
{
    int   nSize;
    int   wFlags;
    int   nLanguageID;
} RASTERIZER_STATUS;
typedef RASTERIZER_STATUS FAR* LPRASTERIZER_STATUS;

/* bits defined in wFlags of RASTERIZER_STATUS */
#define TT_AVAILABLE	0x0001
#define TT_ENABLED	0x0002

BOOL    WINAPI GetRasterizerCaps(RASTERIZER_STATUS FAR*, int);

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

WORD    WINAPI GetOutlineTextMetrics(HDC, UINT, OUTLINETEXTMETRIC FAR*);

#endif  /* NOSCALABLEFONT */

#ifdef __cplusplus
}
#endif

#endif  /* WINVER >= 0x030a */
#endif /* _WIN31_H */
