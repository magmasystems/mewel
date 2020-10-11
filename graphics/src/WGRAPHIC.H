/*===========================================================================*/
/*                                                                           */
/* File    : WGRAPHICS.H                                                     */
/*                                                                           */
/* Purpose : Header for the MEWEL graphics interface                         */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#if !defined(MEWEL_TEXT)

#define _WGRAPHIC_H

/*
  Use the BGI library for a Borland C++ program which is not linked in
  with Genus' GX, or if we are using FlashTek's BGI emulation for
  Zortech's DOSX.
*/
#if (defined(__TURBOC__) && !defined(GX) && !defined(GURU) && !defined(META)) || defined(DOS386)
#define BGI
/*
  Are we using Metagraphics' special 32-bit version of BGI?
*/
#if defined(__DPMI32__)
#define METABGI
#endif
#endif

/*
  The default TopSpeed C graphics engine is BGI compatible
*/
#if (defined(__TSC__) && !defined(GX) && !defined(GURU) && !defined(META))
#define BGI
#define _BGI_COLORS
#undef  MSC
#endif

/*
  DJGPP and the BCC2GRX library
*/
#if defined(__GNUC__) && defined(MEWEL_GUI)
#define USE_BCC2GRX
#if defined(USE_BCC2GRX)
#define GNUGRX
#define BGI
#undef BLACK
#undef BLUE
#undef GREEN
#undef CYAN
#undef RED
#undef MAGENTA
#undef YELLOW
#undef WHITE
#elif !defined(META)
#define GNUGRX
#endif
#endif


#if defined(XWINDOWS)
#undef MSC

/*
  These constants are used for communication between the Arc, Pie,
  and Chord functions.
*/
#define MODE_ARC    0
#define MODE_PIE    1
#define MODE_CHORD  2

#endif

#if defined(MEWEL_32BITS)
#undef huge
#undef _huge
#undef __huge
#undef far
#undef _far
#undef __far
#define huge
#define _huge
#define __huge
#define far
#define _far
#define __far
#endif

typedef char HUGE *HPSTR;


#if (defined(META) || defined(BGI) || defined(MSC) || defined(GX) || defined(GURU)) && !defined(MOTIF)
#define USE_REGIONS
#endif


/*
  EXTENT_OFFSET is used as a compensating factor between graphics libs which
  treat the coordinates pf the lower-right corner of a line/rectangle in the
  "Windows" way (the lower-right is not included in the drawing), and
  graphics libs which include the lower-right in the drawing.
*/
#if defined(META)
#define EXTENT_OFFSET  0
#else
#define EXTENT_OFFSET  1
#endif


#if !defined(WAGNER_GRAPHICS)

#if defined(XWINDOWS)
#define _setcolor(i)
#define _moveto(x, y)
#define _setviewport(x1,y1,x2,y2)

#define _GPSET         GXcopy
#define _GXOR          GXor
#define _GPRESET       GXcopyInverted
#define _GAND          GXand
#define _GOR           GXnoop

/*
  This structure is allocated in the DC's lpDCExtra field.
*/
typedef struct tagXDCInfo
{
  Pixmap pixmap;
} XDCINFO, *PXDCINFO, FAR *LPXDCINFO;


#elif defined(META)
#include "metawndo.h"
#define FONT_EXTENSION  ".fnt"
#define _moveto        mwMoveTo
#define _lineto        mwLineTo
#define getpixel       mwGetPixel
#define outtextxy(x,y,s)  mwMoveTo(x, y), mwDrawString(s)
#define _putimage(x, y, s, mode)
#define _setcolor(i)   mwPenColor(((i) == 15) ? -1 : (i))
VOID FAR PASCAL _setviewport(int, int, int, int);
#define textheight(s)  VideoInfo.yFontHeight
#define textwidth(s)   mwStringWidth(s)
#define _GPSET         zREPz
#define _GXOR          zXORz
#define _GPRESET       zINVERTz
#define _GAND          zANDz
#define _GOR           zORz

typedef struct tagMetaInfo
{
  grafPort *thePort, *pOrigPort;
  HANDLE   hDefaultFontBuf;
  grafPort MyPort;
} METAINFO;

extern METAINFO _MetaInfo;

/*
  This structure is allocated in the DC's lpDCExtra field.
*/
typedef struct tagMetaDCInfo
{
  grafPort FAR *pGrafPort;
  grafMap  FAR *pGrafMap;
  region   FAR *pClipRegion;
  fontRcd  FAR *lpFontBuf;
  HANDLE        hFontBuf;
} METADCINFO, *PMETADCINFO, FAR *LPMETADCINFO;


#elif defined(GX)
#ifdef DOS286X
#define DOSX286
#endif
#include <gxlib.h>
#include <grlib.h>
#if defined(USE_GX_TEXT)
#include <txlib.h>
#endif

/*
  Are we using version 3.x of Genus GX?
*/
#if defined(gxMAJOR) && (gxMAJOR >= 3)
#define GX3
#if defined(__TURBOC__)
#pragma warn -sus
#endif
#endif

#define GX_DEFAULT_FONT   grTXT8X14
#define FONT_EXTENSION  ".fnt"
#define _moveto        grMoveTo
#define _lineto        grLineTo
#define _floodfill     grFloodFill
#define getpixel       grGetPixel
#define _putimage(x, y, s, mode)
#define _setcolor      grSetColor
#define _setviewport(x1,y1,x2,y2) \
{\
  grSetViewPort(x1,y1,x2,y2);\
  grSetClipRegion(x1,y1,x2,y2);\
  grSetClipping(grCLIP);\
}
#if defined(USE_GX_TEXT)
#define textheight(s)  txChrHeight()
#define textwidth(s)   txStrWidth(s)
#if defined(GX3)
#define outtextxy(x,y,s)  txOutTextXY(s,x,y)
#else
#define outtextxy(x,y,s)  txPutString(s,x,y)
#endif
#else
#define textheight(s)  SysGDIInfo.tmHeight
#define textwidth(s)   (SysGDIInfo.tmAveCharWidth * lstrlen(s))
#define outtextxy(x,y,s)  grMoveTo(x, y), grOutText(s)
#endif
#define _GPSET         gxSET
#define _GXOR          gxXOR
#define _GPRESET       gxXOR
#define _GAND          gxAND
#define _GOR           gxOR

/*
  This structure is allocated in the DC's lpDCExtra field.
*/
typedef struct tagGXDCInfo
{
  GXHEADER *gxHeader;
  HANDLE   hTXHeader;  /* handle of TXHEADER structure */
} GXDCINFO, *PGXDCINFO, FAR *LPGXDCINFO;

extern VOID FAR PASCAL GXVirtualScreen(BOOL);


#elif defined(GURU)
#undef YELLOW
#undef WHITE

#include <engine.h>

#define GURU_DEFAULT_FONT   ROM8x14
#define FONT_EXTENSION  ".fnt"

#define _getimage      GetImage
#define getpixel       GetPixelX
#define _imagesize     ImageSize
#define _moveto(x,y)
#define outtextxy      ADrawText
#define _putimage(x,y,s,i)  RPutImage(x,y,s)
#define _setcolor      SetColor
#define _setfillmask   SetFill
#define _setviewport(x1,y1,x2,y2) \
{\
  SetClip(x1,y1,x2,y2);\
  Clipping(1);\
}
#define textheight(s)  SysGDIInfo.tmHeight
#define textwidth(s)   (SysGDIInfo.tmAveCharWidth * lstrlen(s))
#define _GPSET         0
#define _GXOR          1
#define _GPRESET       2
#define _GAND          3
#define _GOR           4



#elif defined(MSC) && !defined(DOS386) && !defined(UNIX) && !defined(__ZTC__)
#include <graph.h>
#define FONT_EXTENSION  ".fon"
#define getpixel       _getpixel
#define outtextxy(x,y,s)  _moveto(x, y), _outgtext(s)
#define textheight(s)  VideoInfo.yFontHeight
#define textwidth(s)   _getgtextextent(s)
#if defined(__HIGHC__)
#define _MAXRESMODE    _VRES16COLOR
/*
  The default HIGH-C engine is the GFX graphics library
*/
#define GFX
#endif /* HIGHC */


#elif defined(BGI)
#define __COLORS
#undef  CGA
#undef  EGAMONO

#if defined(USE_BCC2GRX)
#include <libbcc.h>
#define NOT_PUT 10   /* not implemented in BCC2GRX */
/*
  This structure is allocated in the DC's lpDCExtra field.
*/
typedef struct tagGRXDCInfo
{
  GrContext *grContext;
} GRXDCINFO, *PGRXDCINFO, FAR *LPGRXDCINFO;
#else
#include <graphics.h>
#endif

#define FONT_EXTENSION  ".chr"
#define _moveto        moveto
#define _lineto        lineto
#define _floodfill     floodfill
#define _imagesize     imagesize
#define _getimage      getimage
#define _putimage      putimage
#define _setcolor      setcolor
#define _setviewport(x1,y1,x2,y2)  setviewport(x1,y1,x2,y2,TRUE)
#define _setwritemode  setwritemode
#define _GPSET         COPY_PUT
#define _GXOR          XOR_PUT
#define _GPRESET       NOT_PUT
#define _GAND          AND_PUT
#define _GOR           OR_PUT

#if defined(DOS386)
#define halloc(n,s)    malloc(n)
#define hfree(s)       free(s)
#else
#define halloc(n,s)    farmalloc(n)
#define hfree(s)       farfree(s)
#endif

#endif
#endif

#if defined(XWINDOWS)
#define SETPIXEL(hDC,x,y,attr)  SetPixel(hDC,x,y,attr)
#elif defined(META)
#define SETPIXEL(hDC,x,y,attr)  mwPenColor((attr==15) ? -1 : attr), mwSetPixel(x, y)
#elif defined(GX)
#define SETPIXEL(hDC,x,y,attr)  grPutPixel(x, y, attr)
#elif defined(GURU)
#define SETPIXEL(hDC,x,y,attr)  SetColor(attr), RPlot(x, y)
#elif defined(MSC)
#define SETPIXEL(hDC,x,y,attr)  _setcolor(attr), _setpixel(x,y)
#elif defined(BGI)
#define SETPIXEL(hDC,x,y,attr)  putpixel(x, y, attr)
#endif



/*
  PCX stuff...
*/

#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 1
#else
#pragma pack(1)
#endif
#endif

typedef struct tagPCXHeader
{
  BYTE bManufacturer;
  BYTE bVersion;
  BYTE bEncoding;
  BYTE bBitsPerPixel;
  RECT rDimensions;
  USHORT wHorizResolution;
  USHORT wVertResolution;
  BYTE rgbPalette[16*3];  /* red, green, blue */
  BYTE bReserved1;
  BYTE bNPlanes;
  USHORT wBytesPerLine;
  USHORT wPaletteInfo;
  USHORT wHorizScreenSize;
  USHORT wVertScreenSize;
  BYTE bReserved[54];
} PCXHEADER;

extern INT FAR PASCAL DrawPCXFile(HDC, char *, INT, INT);


/*
  Icon stuff...
*/
typedef struct tagIconHeader
{
  USHORT icoReserved;
  USHORT icoResourceType;  /* must be 1 */
  USHORT icoResourceCount;
/*
  ICONDIRECTORY icoDir[icoResourceCount];
*/
} ICONHEADER, *PICONHEADER, FAR *LPICONHEADER;

typedef struct tagIconDirectory
{
  BYTE Width;
  BYTE Height;
  BYTE ColorCount;
  BYTE bReserved1;
  USHORT bReserved2;
  USHORT bReserved3;
  DWORD icoDIBSize;    /* number of bytes */
  DWORD icoDIBOffset;  /* byte offset from start of file to image */
/*
  BITMAPINFOHEADER bmih;
  BYTE XORimage[];  color bitmap image
  BYTE ANDimage[];  monochrome AND bitmap
*/
} ICONDIRECTORY, *PICONDIRECTORY, FAR *LPICONDIRECTORY;


/*
  This format is only valid in a Windows EXE or DLL
  see SDK vol 4, page 86
*/
typedef struct tagCursorDirectory
{
  USHORT Width;
  USHORT Height;
  USHORT bReserved2;
  USHORT bReserved3;
  DWORD curDIBSize;    /* number of bytes */
  WORD  curDIBOffset;  /* byte offset from start of file to image */
} CURSORDIRECTORY, *PCURSORDIRECTORY, FAR *LPCURSORDIRECTORY;


/*
  BMPHEADER structure
*/
typedef struct tagBmpHeader
{
  UINT wSignature;
#define SIG_DDB    0x6666    /* device dependent bitmap */
#define SIG_DIB    0x7777    /* contains palette info   */
  UINT nPaletteEntries;
  /*
  RGBQUAD rgbQuad[nPaletteEntries];
  */
  /*
  IMGHEADER imgHeader;
  */
} BMPHEADER, *PBMPHEADER, FAR *LPBMPHEADER;

/*
  The IMGHEADER is a engine-dependent structure which is really the
  information header used by the various _putimage() and _getimage()
  functions. This structure is referenced by WGUIBMP.C and WGUIPCX.C.
*/
#if defined(META)
#define IMGHEADER  imageHeader
#elif defined(GX)
#define IMGHEADER  GXHEADER
#elif defined(GNUGRX)
#define IMGHEADER  GrContext
#else
typedef struct tagImgHeader
{
  USHORT  wWidth;
  USHORT  wHeight;
#if defined(WC386)
  USHORT  filler;
#elif defined(METABGI)
  short    imWidth;        /* Pixel width (X)              */
  short    imHeight;       /* Pixel height (Y)             */
  char     imAlign;        /* Image alignment              */
  char     imFlags;        /* Image flags                  */
  short    imRowBytes;     /* Bytes per row                */
  short    imBits;         /* Bits per pixel               */
  short    imPlanes;       /* Planes per pixel             */
/*image    imData[]; */    /* (image data, variable length)*/
#endif
} IMGHEADER;
#endif

#if !defined(BYTE_SWAP)
void swIconHeader(LPICONHEADER);
void swIconDirectory(LPICONDIRECTORY);
#else
#define swIconHeader(x)        (x)
#define swIconDirectory(x)     (x)
#endif


/*
  Offset for first two words putimage
*/
#if defined(BGI) && !defined(METABGI)
#define PUTIMAGE_OFFSET  1
#else
#define PUTIMAGE_OFFSET  0
#endif

/*
  These two arrays map a DIB pixel to planes of a 4-color bitmap
*/
extern int iDIBtoDDBPlaneOrder[];
extern int iDDBtoDIBPlaneOrder[];


#if !defined(UNIX) && !defined(VAXC)
#if defined(__ZTC__)
#pragma ZTC align 2
#elif defined(DOS386) || defined(WC386)
#pragma pack
#else
#pragma pack(2)
#endif
#endif



int PASCAL DrawIconFile(HDC hDC, char *pszFile, int x, int y);
int PASCAL DrawBitmapFile(HDC hDC, char *pszFile, int x, int y);
BOOL PASCAL DrawBitmap(HDC hDC, LPBITMAPINFOHEADER lpbmi, 
                                 LPSTR lpBitmapBytes, int x, int y);
VOID FAR PASCAL MEWELDrawDIBitmap(HDC hDestDC, int x,int y,int nWidth,int nHeight, 
  HDC hSrcDC, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, DWORD dwRop);
VOID FAR PASCAL DrawBitmapToDC(HDC hDC, int x, int y, HBITMAP hBitmap, DWORD dwRop);

#define SM_CYEDITBORDER        2
#define SM_CXEDITBORDER        2
#define SM_CYSCROLLBARBORDER   1
#define SM_CXSCROLLBARBORDER   1
#define SM_CYTHUMBBORDER       1
#define SM_CXTHUMBBORDER       1
#define SM_CYCHECKBOXBORDER    2
#define SM_CXCHECKBOXBORDER    2
#define SM_CYRADIOBUTTONBORDER 2
#define SM_CXRADIOBUTTONBORDER 2
#define SM_CYPUSHBUTTONBORDER  2
#define SM_CXPUSHBUTTONBORDER  2
#ifdef ZAPP
#define SM_CXFOCUSSHIFT        1  /* For depressed pushbuttons */
#define SM_CYFOCUSSHIFT        1
#else
#define SM_CXFOCUSSHIFT        2  /* For depressed pushbuttons */
#define SM_CYFOCUSSHIFT        2
#endif

/*
  Definitions of the system bitmaps. These are really indicies into the
  system bitmap array.
*/
#define SYSBMP_CHECKBOXOFF  0
#define SYSBMP_CHECKBOXON   1
#define SYSBMP_3STATEON     2
#define SYSBMP_RADIOOFF     3
#define SYSBMP_RADIOON      4

#define SYSBMPT_MAXIMIZE    0
#define SYSBMPT_MAXIMIZEDEP 1
#define SYSBMPT_MINIMIZE    2
#define SYSBMPT_MINIMIZEDEP 3
#define SYSBMPT_RESTORE     4
#define SYSBMPT_RESTOREDEP  5
#define SYSBMPT_SYSMENU     6
#define SYSBMPT_MDISYSMENU  7
#define SYSBMPT_COMBO       8


/*
  Definitions of the system transparent bitmaps. These are really indicies into
  the system transparent bitmap array.
*/

extern VOID FAR PASCAL WindowRectToPixels(HWND hWnd, LPRECT rc);
extern LONG FAR PASCAL GetClientPixelCursorPos(HWND hWnd, LONG lParam);
extern BOOL FAR PASCAL _GraphicsSetViewport(HDC hDC);

#if defined(MEWEL_GUI) || defined(XWINDOWS)
extern VOID FAR PASCAL DrawSysBitmap(HDC hDC, INT idxBitmap, INT x, INT y);
extern VOID FAR PASCAL DrawSysTransparentBitmap(HWND hWnd, HDC hOrigDC, INT idxIcon);
extern VOID PASCAL DrawComboBoxIcon(HWND, HDC);
extern VOID PASCAL Draw3DBox(HDC hDC, LPRECT lpRect, BOOL bSelected);
extern VOID PASCAL DrawBeveledBox(HWND, HDC, LPRECT, int, int, BOOL, HBRUSH);
extern VOID PASCAL GUIDrawBorder(HWND hWnd, UINT bActive);
extern VOID PASCAL GUIDrawCaption(HWND hWnd, HDC hDC, BOOL bActive);
#endif


#if defined(MEWEL_GUI) || defined(XWINDOWS)
extern VOID      PASCAL GUIEraseBackground(HWND hWnd, HDC hDC);
extern VOID  FAR PASCAL RealizeROP2(HDC hDC);
extern BOOL  FAR PASCAL RealizeBrush(HDC hDC);
extern BOOL  FAR PASCAL RealizePen(HDC hDC);
extern BOOL  FAR PASCAL RealizeFont(HDC hDC);
extern VOID  FAR PASCAL RealizeBitmap(HDC, HBITMAP);
extern VOID  FAR PASCAL MetaSetDefaultFont(void);
extern VOID  FAR PASCAL GXSetDefaultFont(void);
#else
#define RealizeROP2(hDC)
#define RealizeBrush(hDC)
#define RealizePen(hDC)
#define RealizeFont(hDC)
#define RealizeBitmap(hDC, hBmp)
#endif

#if defined(XWINDOWS)
extern VOID  PASCAL _XLogToPhys(LPHDC, MWCOORD *, MWCOORD *);
extern VOID  PASCAL _XNLogToPhys(LPHDC, INT, LPPOINT);
#else
#define _XLogToPhys(lphDC,  px, py)
#define _XNLogToPhys(lphDC, n,  lppt)
#endif

extern BOOL  FAR PASCAL GrLPtoSP(HDC hDC, LPPOINT lpPt, int nCount);
extern INT   FAR PASCAL BitmapBitsToLineWidth(UINT, UINT, INT *);
extern VOID  FAR PASCAL EngineDeleteBitmap(HANDLE hObj);
extern LPSTR FAR PASCAL HObjectToImageBits(HANDLE, HANDLE *, LPBITMAP *, INT *);

extern VOID  FAR PASCAL MOUSE_ConditionalOffDC(LPHDC, int, int, int, int);
extern VOID  FAR PASCAL MOUSE_ShowCursorDC(void);
extern INT   FAR PASCAL MOUSE_ShowCursor(void);
extern INT   FAR PASCAL MOUSE_HideCursor(void);
extern VOID  FAR PASCAL MOUSE_ConditionalOff(int,int,int,int);
extern VOID  FAR PASCAL MEWELSortPoints(MWCOORD *, MWCOORD *, MWCOORD *, MWCOORD *);
extern LPHDC FAR PASCAL GDISetup(HDC);
extern VOID  FAR PASCAL GDIScrollWindow(HDC, LPRECT, LPRECT);
extern LPSTR FAR PASCAL _GExpandTabs(LPSTR, INT *, INT *, INT *);
extern DWORD FAR PASCAL _GetTextExtent(LPCSTR);


#ifdef INCLUDE_SYSBITMAPS
typedef struct tagSysBitmapInfo
{
  HPSTR    lpBits;
  INT      biWidth,
           biHeight,
           biBitCount;
  HBITMAP  hBitmap;
} SYSBITMAPINFO, HUGEDATA *PSYSBITMAPINFO;


typedef struct tagSysCaptionBitmapInfo
{
  HPSTR    lpBits;
  INT      biWidth,
           biHeight,
           biBitCount;
  INT      cxLeftColOffset, cxRightColOffset;
  INT      cxPixelOffset, cyPixelOffset;
  HBITMAP  hBitmap;
} SYSCAPTIONBITMAPINFO, HUGEDATA *PSYSCAPTIONBITMAPINFO;
#endif

#ifdef INCLUDE_SYSICONS
typedef struct tagIconInfo
{
  HBITMAP hBitmap;
  HBITMAP hAndBitmap;
  UINT    fFlags;
#define ICON_SYSTEMICON  0x0001
} ICONINFO, *PICONINFO, HUGEDATA *LPICONINFO;
#endif

/*
  Windows ROP-to-engine code translation table
*/
extern int ROPtoEngineCode[];
int FAR PASCAL MapROPCode(DWORD dwRop);



#endif /* !defined(MEWEL_TEXT) */
