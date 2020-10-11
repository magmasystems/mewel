/*===========================================================================*/
/*                                                                           */
/* File    : WCREATE.C                                                       */
/*                                                                           */
/* Purpose : WinCreate() - creates a window structure                        */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

static WINDOW *PASCAL WinAlloc(void);

HWND FAR PASCAL
WinCreate(hParent,row1,col1,row2,col2,title,attr,flags,idClass,idCtrl)
  HWND     hParent;
  int      row1, col1, row2, col2;
  LPSTR    title;
  COLOR    attr;
  DWORD    flags;
  int      idClass;
  int      idCtrl;
{
  WINDOW *w;
  HWND   hWnd;
  LPEXTWNDCLASS pClass;

  /*
    Make sure that the window has a valid class
  */
  if ((pClass = ClassIDToClassStruct(idClass)) == NULL)
    return NULLHWND;

  /*
    Allocate an empty window structure
  */
  if ((w = WinAlloc()) == NULL)
    return NULLHWND;
    
  /*
    Fill in the elements
  */
  hWnd           = w->win_id;
  w->rect.left   = (MWCOORD) col1;
  w->rect.top    = (MWCOORD) row1;
  w->rect.right  = (MWCOORD) col2;
  w->rect.bottom = (MWCOORD) row2;
  w->rClient     = w->rect;
  w->attr        = attr;
  w->iMonitor    = 0;
  w->fillchar    = WinGetSysChar(SYSCHAR_WINDOW_BACKGROUND);
  w->flags       = flags;
  w->title       = lstrsave(title);
  w->idClass     = idClass;
  w->hMenu       = 0;
  w->idCtrl      = idCtrl;
  w->plistProp   = (PLIST) NULL;

  /*
    We need to set the winproc here cause _WinSetClientRect can issue
    a WM_NCCALCSIZE message, and we need the winproc set to process it.
  */
  if ((w->winproc = pClass->lpfnWndProc) == NULL)
    w->winproc = (WINPROC *) StdWindowWinProc;

  /*
    From the class defn, we set the window and class extra bytes, the
    default window proc, and parts of the window style.
  */
  if (pClass->cbWndExtra)
    SetWindowExtra(hWnd, pClass->cbWndExtra);
  w->ulStyle |= pClass->style;

  /*
    For text mode, if we have a thickframe, then we also have a border
  */
#if !defined(MEWEL_GUI) && !defined(XWINDOWS)
  if (flags & WS_THICKFRAME)
    flags = (w->flags |= WS_BORDER);
#endif

  /*
    Calculate the window's client area.
  */
  _WinSetClientRect(hWnd);

  /*
    We must prepare the window for its initial drawing. This means 
    invalidating its entire client area and setting the "erase-it" bit.
  */
  InternalInvalidateWindow(hWnd, TRUE);

  /*
    Link the child onto its parent's child list.
  */
  if (hParent)
    _WinAddChild(hParent, hWnd);

#if defined(XWINDOWS) && !defined(MOTIF)
  XMEWEL_CreateWindow(hWnd);
#endif

  /*
    Create the scrollbars associated with the normal window
  */
  if ((flags & (WS_VSCROLL | WS_HSCROLL)) &&
      (idClass == NORMAL_CLASS  || idClass == DIALOG_CLASS
       || idClass == LISTBOX_CLASS
#if !defined(MOTIF)
       || idClass == EDIT_CLASS
#endif
     ))
  {
    if (flags & WS_VSCROLL)
      w->hSB[SB_VERT] = ScrollBarCreate(hWnd,
                      w->rClient.top,
                      w->rClient.right,
                      w->rClient.bottom,
                      w->rClient.right + IGetSystemMetrics(SM_CXVSCROLL),
                      SYSTEM_COLOR,
                      (DWORD) SBS_VERT | WS_VISIBLE, 0);

    if (flags & WS_HSCROLL)
      w->hSB[SB_HORZ] = ScrollBarCreate(hWnd,
                      w->rClient.bottom,
                      w->rClient.left, 
                      w->rClient.bottom + IGetSystemMetrics(SM_CYHSCROLL),
                      w->rClient.right,
                      SYSTEM_COLOR,
                      (DWORD) SBS_HORZ | WS_VISIBLE, 0);

    /*
      Recalc the client area of the window since we just increased the size
      of the non-client area;
    */
    _WinSetClientRect(hWnd);
  }

  /*
    Create the system menu
  */
#if !defined(MOTIF)
  if (flags & WS_SYSMENU)
    WinCreateSysMenu(hWnd);
#endif

  return hWnd;
}



/****************************************************************************/
/*                                                                          */
/* Function : WinAlloc()                                                    */
/*                                                                          */
/* Purpose  : Allocates the memory for a window structure. Also allocates   */
/*            the master array of window pointers, HwndArray[].             */
/*                                                                          */
/* Returns  : A pointer to the window structure.                            */
/*                                                                          */
/****************************************************************************/

/*
  Televoice's change to speed up searching HwndArray[] for a free
  entry. The free entries are now chained together.
  There is a bug lurking in medium model, since window pointers
  are only 16 bits.

  Note from televoice :

  Description:  The WinAlloc routines looks for an empty slot in the
    window table with a linear search.  After 150-200 windows
    become active, the search for empty slots takes time, especially
    with opening a new dialog box with 40-50 windows.  These changes
    to wcreate.c and windstry.c keep a single linked list of empty
    windows by using the HwndArray to store unsigned integers.
    This change requires pointers to have the upper 16 bits 
    non-zero (in order for WID_TO_WIN to identify the linked list
    entries as NULL windows). Marc-- these changes make a noteable
    speed improvement in our Applications on the slower machines.
    I hope they are portable enough for you to consider including 
    in standard MEWEL.
*/
#if (MODEL_NAME == MODEL_LARGE) || defined(MEWEL_32BITS)
#define NEW_SEARCHING_METHOD
#endif

WINDOW ** TRUENEAR HwndArray; /* Master array of window pointers */
static INT  nHwndArray = 0;   /* Number of elements allocated for HwndArray */
#ifdef NEW_SEARCHING_METHOD
static INT  nHwndEmpty = -1;  /* Next empty window */
#endif


static WINDOW *PASCAL WinAlloc(void)
{
  INT    i;
  WINDOW *w;
  
  /*
    Allocate the array of window pointers if we have not done so.
  */
  if (HwndArray == NULL)
  {
    if ((HwndArray = (WINDOW **)emalloc(MAXWINDOWS * sizeof(WINDOW*))) == NULL)
      return (WINDOW *) NULL;
    nHwndArray = MAXWINDOWS;
#ifdef NEW_SEARCHING_METHOD
    nHwndEmpty = 0;
#endif
  }

  /*
    Allocate a window structure and hook it onto the front of the WindowList.
  */
  if ((w = (WINDOW *) emalloc(sizeof(WINDOW))) == (WINDOW *) NULL)
    return (WINDOW *) NULL;
  w->next = InternalSysParams.WindowList;
  InternalSysParams.WindowList = w;

  /*
    Search for a free handle within HwndArray.
  */
#ifdef NEW_SEARCHING_METHOD
  i = nHwndEmpty;
  if (i != nHwndArray)
  {												/* 6/22/93 */
    if (HwndArray[i] == NULL)
      nHwndEmpty += 1;
    else										/* 6/22/93 */
      nHwndEmpty = (UINT) (DWORD) HwndArray[i];
  }												/* 6/22/93 */
#else
  for (i = 0;  i < nHwndArray;  i++)
    if (HwndArray[i] == (WINDOW *) NULL)
      break;
#endif

  /*
    If there are no free entries left, then reallocate HwndArray.
    Don't use realloc() cause some compilers have broken implementations
  */
  if (i == nHwndArray)
  {
    WINDOW **newHwndArray;

    newHwndArray = (WINDOW **) emalloc(nHwndArray * 2 * sizeof(WINDOW *));
    if (newHwndArray == NULL)
      return (WINDOW *) NULL;
    memcpy((char *)newHwndArray, (char *)HwndArray, nHwndArray*sizeof(WINDOW*));
    MyFree((char *) HwndArray);
    nHwndArray *= 2;
    HwndArray = newHwndArray;
#ifdef NEW_SEARCHING_METHOD
    nHwndEmpty = i + 1;
#endif
  }

  /*
    Return the window pointer
  */
  HwndArray[i] = w;
  w->win_id = i+1;
  return w;
}


/*
 * Called from windstry.c
 */
void PASCAL WinFree(HWND hWnd)							/* 6/22/93 */
{														/* 6/22/93 */
#ifdef NEW_SEARCHING_METHOD
  HwndArray[(UINT) hWnd-1] = (WINDOW *)(DWORD) nHwndEmpty;
  nHwndEmpty = hWnd - 1;								/* 6/22/93 */
#else
  HwndArray[(UINT) hWnd-1] = NULL;
#endif
}														/* 6/22/93 */

BOOL FAR PASCAL IsWindow(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (BOOL) (w != (WINDOW *) NULL);
}

WINDOW *FAR PASCAL WID_TO_WIN(hWnd)
  HWND hWnd;
{
#ifdef NEW_SEARCHING_METHOD
  WINDOW *w;
  return (!hWnd || hWnd > nHwndArray 
                || (((DWORD) (w = HwndArray[(UINT) hWnd-1])) & 0xFFFF0000L) == 0)
             ? (WINDOW *) NULL : w;
#else
  return (!hWnd || hWnd > nHwndArray) ? (WINDOW *) NULL : HwndArray[(UINT) hWnd-1];
#endif
}

