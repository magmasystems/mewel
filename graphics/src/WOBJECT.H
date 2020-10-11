/*===========================================================================*/
/*                                                                           */
/* File    : WOBJECT.H                                                       */
/*                                                                           */
/* Purpose : Header for GDI object manipulation routines.                    */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

typedef struct tagObject
{
  OBJECTTYPE iObjectType;
  UINT       fFlags;  
  INT        nRefCount;
#define OBJ_LPEXTRA_IS_HANDLE   0x0001
  LPVOID     lpExtra;    /* for any graphics-engine specific stuff */
  union
  {
    LOGPEN   uLogPen;
    LOGBRUSH uLogBrush;
    LOGFONT  uLogFont;
    RECT     uRgn;
    HBITMAP  uhBitmap;
    HPALETTE uhPalette;
  } uObject;
} OBJECT, *POBJECT, FAR *LPOBJECT;


/*
  PRIVATE STUFF FOR OBJECT HANDLING ...
*/
extern HANDLE FAR PASCAL _ObjectAlloc(UINT);
extern VOID _ObjectsInit(void);
#define MAXOBJECTS     64

/*
  Define a special signature which gets ORed with the index of the
  object in the object table to create an object handle.
*/
#define OBJECT_SIGNATURE    0xA000

#define _ObjectDeref(hObj) \
  (((((UINT)hObj) & ~OBJECT_SIGNATURE)>=(UINT)_ObjectTblSize) ? ((LPOBJECT) 0) : _ObjectTable[(((UINT)hObj)&~OBJECT_SIGNATURE)])

extern UINT       _ObjectTblSize;
extern LPOBJECT  *_ObjectTable;

extern LPRECT FAR PASCAL _RgnToRect(HRGN);

