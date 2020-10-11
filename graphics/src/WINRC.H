/*===========================================================================*/
/*                                                                           */
/* File    : WINRC.H                                                         */
/*                                                                           */
/* Purpose : Include file for the resource compiler                          */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#ifdef ZORTECH
#pragma ZTC align 1
#endif /* */

#if defined(MSC) && (_MSC_VER >= 700)
#pragma warning(disable:4103 4118)
#endif

#if !defined(UNIX_STRUCTURE_PACKING)
#pragma pack(1)
#endif

typedef struct resource
{
  BYTE  bResType;   /* 0 if iResType is a string-table offset - always 0xFF here */
#define RES_WINCOMPATDLG   0xFE
  WORD  iResType;
  BYTE  bResID;    /* 0 if iResID is a string-table offset - always 0xFF here */
  WORD  iResID;

  WORD  fResFlags;
#define RES_MOVEABLE     0x0010
#define RES_PRELOAD      0x0040
#define RES_DISCARDABLE  0x1000

  DWORD nResBytes;
  BYTE  szResData[1];
} RESOURCE;


typedef struct rcstring
{
  WORD idString;
  WORD nChars;
  char string[1];
} RCSTRING, FAR *LPRCSTRING;

typedef struct stringtable
{
  WORD nStrings;
  RCSTRING strings[1];
} STRINGTABLE;

typedef struct tagStringTblHeader
{
  DWORD ulSignature;
#define RC_SIGNATURE  0x12340031L   /* last two digits are version number */
  DWORD ulSeekPos;
  DWORD ulBytes;
} STRINGTBLHEADER;

typedef struct accel
{
  BYTE fFlags;
  WORD wEvent;
  WORD wId;
} ACCEL, FAR *LPACCEL;

typedef struct mti
{
  WORD style;
  WORD attr;
  WORD idItem;
#ifdef FOOBAZ
  union
  {
    struct item_string           /* MF_STRING     */
    {
      WORD nChars;
      char   string[1];
    };
    MENUTEMPLATE item_popup;     /* MF_POPUP      */
    void foo;                    /* MF_SEPARATEOR */
  } uItem;
#endif
} MTI;

typedef struct menutemplate
{
/*WORD nBytes;*/
  WORD nItems;
  MTI    items[1];
} MENUTEMPLATE;


typedef struct dlgitem
{
  WORD  idClass;
  WORD  id;
  WORD  x, y, cx, cy;
  WORD  attr;
  WORD  nChars;
  char    szTitle[1];
  DWORD   dwStyle;
} DLGITEM;

typedef struct dlgtemplate
{
  WORD  nItems;
  WORD  id;
  WORD  x, y, cx, cy;
  WORD  attr;
  DWORD   dwStyle;
  WORD  nChars;
  char    szTitle[1];
  DLGITEM items[1];
} DIALOGTEMPLATE;


typedef struct index
{
  WORD minRange,
       maxRange;
  DWORD ulSeekPos;
  DWORD ulSeekPosEnd;
} INDEX, FAR *LPINDEX;



#if !defined(UNIX_STRUCTURE_PACKING) || defined(i386)
#pragma pack()
#endif

#ifdef ZORTECH
#pragma ZTC align
#endif /* */

LPSTR FAR PASCAL _GetResource(HMODULE, LPSTR, LPCSTR, DWORD *, DWORD *);


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


/****************************************************************************/
/*                                                                          */
/*        Structures for the LoadString() function                          */
/*                                                                          */
/****************************************************************************/

#define CACHESIZE  16

/*
  A string entry consists of a numeric ID and a pointer to the string
*/
typedef struct string
{
  WORD    idString;
  LPSTR   pszString;
} STRING;

/*
  We cache CACHESIZE strings at a time. You can reduce the number of
  disk accesses by increasing the cache size, but this will also increase
  memory.
*/
typedef struct stringcache
{
  WORD    rangeMin, rangeMax;  /* the min and max id's of the strings */
  int     hModule;
  BOOL    bStringOpened;
  int     nIndex;
  LPINDEX pIndex;
  STRING  cache[CACHESIZE];
} STRINGCACHE;

extern STRINGCACHE StringCache;

/*
  This is the internal format of a resource table. It is used by
  the various functions in WINRES2.C
*/
typedef struct tagResInfo
{
  DWORD  ulSeekPos;    /* Seek offset where the resource was found */
  DWORD  dwResSize;    /* Size of the resource */
  LPCSTR lpszType;     /* Resource type or RT_xxxx */
  HANDLE hResData;     /* Handle of memory to resource data */
  HANDLE hInst;        /* Instance handle (file handle) */
  WORD   fMemFlags;    /* Memory flags */
  WORD   fLoadFlags;   /* Load flags - LOADONCALL, PRELOAD, etc */
} RESINFO, *PRESINFO, *LPRESINFO;

#ifdef MEWEL_32BITS
#define HRES_MAGIC_BITS  0x90000000
#else
#define HRES_MAGIC_BITS  0x9000
#endif


/*
  WINRES3.C
*/
LPSTR FAR PASCAL LoadResourcePtr(HINSTANCE, LPCSTR, LPCSTR, HANDLE *);
VOID FAR PASCAL UnloadResourcePtr(HINSTANCE, LPSTR, HANDLE);

/*
  This macro tests an hInstance to see if it is a MEWEL hInstance
  (ie - the file handle of the open resource file) or a real
  Windows hInstance, something which a Windows DLL might use when
  loading one of its own resource in.
*/
#define IS_MEWEL_INSTANCE(hInst)  ((((UINT) hInst) & ~0x8000) < 256)

