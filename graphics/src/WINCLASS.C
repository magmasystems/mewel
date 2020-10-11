/*===========================================================================*/
/*                                                                           */
/* File    : WINCLASS.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History : 6/6/89 (maa)   Created                                          */
/*           3/8/90 (maa)   Added ClassIndex[] to try to speed up searching  */
/*                                                                           */
/* (C) Copyright 1989-1994 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define INCLUDE_COMBOBOX
#define INCLUDE_LISTBOX
#define INCLUDE_SCROLLBAR

#include "wprivate.h"
#include "window.h"


EXTWNDCLASS *WndClassList = (EXTWNDCLASS *) NULL;
EXTWNDCLASS *ClassIndex[MAXCLASSES] = { 0 };
static int  nClasses = 0;
static BOOL bRegisteredPredefined = FALSE;
static BOOL bInRegisterPredefinedClasses = FALSE;
static int  iLastClassAllocated = 0;

struct tagDefClassInfo 
{
  WINPROC *lpfnDefClassProc;
  int     idClass;
} _DefClassInfo[6] =
{
  ButtonWinProc,   BUTTON_CLASS,
  EditWinProc,     EDIT_CLASS,
  ListBoxWinProc,  LISTBOX_CLASS,
  ScrollBarWinProc,SCROLLBAR_CLASS,
  StaticWinProc,   STATIC_CLASS,
  ComboBoxWndProc, COMBO_CLASS
};

/*
  Table which maps CT_xxx to a MEWEL class name
*/
static char *aCTtoClassName[] =
{
  "BUTTON",
  "EDIT",
  "STATIC",
  "LISTBOX",
  "SCROLLBAR",
  "COMBOBOX",
};


extern VOID FAR PASCAL _ClassInitBaseClass(EXTWNDCLASS *);
static INT      PASCAL _RegisterClass(UINT,UINT,UINT,LPCSTR,WINPROC *);


/****************************************************************************/
/*                                                                          */
/* Function : RegisterClass()                                               */
/*                                                                          */
/* Purpose  : Takes a pointer to a class structure and installs it in the   */
/*            linked list of classes.                                       */
/*                                                                          */
/* Returns  : TRUE if the class was registered, FALSE if not.               */
/*                                                                          */
/****************************************************************************/

int FAR PASCAL RegisterClass(pWndClass)
  WNDCLASS *pWndClass;
{
  EXTWNDCLASS *pCl;
  int         i;

  /*
    We need the MEWEL pre-defined classes to start at index 0 in the
    class table. For the DLL version of MEWEL, another DLL (like a
    custom control DLL) might have its LibMain called before MEWEL,
    and its LibMain might try to register a class. This would put
    entries in the ClassIndex table before MEWEL registered any of
    its pre-defined classes. So, guard against this... if we have
    not registered the pre-defined classes yet, and we aren't being
    called from _registerPredfinedClasses, the register MEWEL's classes.
  */
  if (!bRegisteredPredefined && !bInRegisterPredefinedClasses)
  {
    _RegisterPredefinedClasses();
  }


  /*
    NULL check...
  */
  if (!pWndClass)
    return FALSE;

  /*
    See if we registered the class already...
  */
  if (ClassNameToClassID(pWndClass->lpszClassName) >= 0)
    return TRUE;

  /*
    Allocate a class structure
  */
  if ((pCl = (EXTWNDCLASS *) emalloc(sizeof(EXTWNDCLASS))) == NULL)
    return FALSE;

  /*
    Copy over the info and link the structure onto the head of the class list
  */
  memcpy((char *) pCl, (char *) pWndClass, sizeof(WNDCLASS));

  /*
    Save the class name. (Allocate space for the name if we are not 
    registering a pre-defined class.)
  */
  if (bRegisteredPredefined && !ISNUMERICRESOURCE(pWndClass->lpszClassName))
    pCl->lpszClassName = (LPCSTR) lstrsave((LPSTR) pWndClass->lpszClassName); 
  else
    pCl->lpszClassName = (LPCSTR) pWndClass->lpszClassName;


  if (pWndClass->lpszMenuName)
  {
    /*
      The menu name can be a string or a numeric id
    */
    if (ISNUMERICRESOURCE(pWndClass->lpszMenuName))
      pCl->lpszMenuName= (LPCSTR) pWndClass->lpszMenuName;
    else
      pCl->lpszMenuName= (LPCSTR) lstrsave((LPSTR) pWndClass->lpszMenuName);
  }


  /*
  */
  for (i = 0;  i < MAXCLASSES && ClassIndex[i] != NULL;  i++)
    ;
  if (i < MAXCLASSES && ClassIndex[i] != NULL)
    pCl->idClass = i;
  else
    pCl->idClass = nClasses++;

  /*
    Hook the class onto the head of the class list
  */
  pCl->next = WndClassList;
  WndClassList = pCl;

  /*
    Allocate the class-extra bytes
  */
  if (pCl->cbClsExtra)
    pCl->lpszClassExtra = (PSTR) emalloc(pCl->cbClsExtra);

  /*
    Add the class to the index
  */
  if (pCl->idClass < MAXCLASSES)
    ClassIndex[iLastClassAllocated = pCl->idClass] = pCl;

  _ClassInitBaseClass(pCl);

  /*
    Give the class its own DC
  */
  if (pCl->style & CS_CLASSDC)
  {
    pCl->hDC = GetDC(NULLHWND);
  }

  return TRUE;
}


int FAR PASCAL ExtRegisterClass(pWndClass)
  EXTWNDCLASS *pWndClass;
{
  EXTWNDCLASS *pCl;

  /*
    NULL check...
  */
  if (!pWndClass)
    return FALSE;

  /*
    See if we registered the class already...
  */
  if (ClassNameToClassID(pWndClass->lpszClassName) >= 0)
    return TRUE;

  /*
    Take care of the WNDCLASS part
  */
  if (!RegisterClass((WNDCLASS *) pWndClass))
    return FALSE;

  /*
    Take care of the base class part, if there is one...
  */
  if (pWndClass->lpszBaseClass != NULL)
  {
    pCl = ClassIndex[iLastClassAllocated];
    pCl->lpszBaseClass = pWndClass->lpszBaseClass;
    _ClassInitBaseClass(pCl);
  }

  return TRUE;
}


VOID FAR PASCAL _ClassInitBaseClass(pCl)
  EXTWNDCLASS *pCl;
{
  EXTWNDCLASS *pCl2;
  int         iClass;

  /*
    This little dance we do here is to see if the app is trying to
    superclass a standard control. If the WNDCLASS structure passed
    has the address of a standard MEWEL control winproc as its
    lpfnWndProc member, then the app is trying to superclass. So,
    we need to set the lpszBaseClass field in order for this to work.
    Of course, everything is fine if the app uses ExtRegisterClass,
    but we want to be Windows compatible here!
  */
  if (bRegisteredPredefined && pCl->lpszBaseClass == NULL)
  {
    for (iClass = 0;  iClass < 6;  iClass++)
      if (pCl->lpfnWndProc == _DefClassInfo[iClass].lpfnDefClassProc)
      {
        pCl->lpszBaseClass = (LPSTR)
                           ClassIDToClassName(_DefClassInfo[iClass].idClass);
        break;
      }
  }


  if (pCl->lpszBaseClass == NULL || *pCl->lpszBaseClass == '\0')
    pCl->lpszBaseClass = (LPSTR) "NORMAL";
  pCl->lpszBaseClass = (LPSTR) 
  (bRegisteredPredefined ? lstrupr(lstrsave(pCl->lpszBaseClass))
                         : pCl->lpszBaseClass);


  /*
    Fill in the base class ID, and take an initial stab at the lowest class ID
  */
  if ((pCl->idBaseClass = ClassNameToClassID(pCl->lpszBaseClass)) == 0xFFFF)
    pCl->idBaseClass = pCl->idClass;
  pCl->idLowestClass = pCl->idBaseClass;

  /*
    Fill in the default proc for the class. It's the window proc for the
    base class, or if no base class, it StdWindowWndProc().
  */
  if ((iClass = ClassNameToClassID(pCl->lpszBaseClass)) >= 0)
    pCl->lpfnDefWndProc = ClassToWinProc(iClass);
  else
    pCl->lpfnDefWndProc = StdWindowWinProc;

  /*
    Get the lowest class id
  */
  if (bRegisteredPredefined)
  {
    pCl2 = pCl;
    for (iClass=pCl2->idClass;  iClass >= USER_CLASS;  iClass=pCl2->idBaseClass)
      pCl2 = ClassIDToClassStruct(iClass);
    pCl->idLowestClass = iClass;
  }
}


/****************************************************************************/
/*                                                                          */
/* Function : ClassNameToClassID()                                          */
/*                                                                          */
/* Purpose  : Internal use. Maps a class name to a class id.                */
/*                                                                          */
/* Returns  : The class id (>= 0) if found, -1 if not.                      */
/*                                                                          */
/****************************************************************************/
int  FAR PASCAL ClassNameToClassID(lpszClass)
  LPCSTR lpszClass;
{
  register EXTWNDCLASS *pCl;

  if (lpszClass)
  {
    if (ISNUMERICRESOURCE(lpszClass))
    {
      /*
        Windows can use MAKEINTRESOURCE for the class name
      */
      for (pCl = WndClassList;  pCl;  pCl = pCl->next)
        if (lpszClass == pCl->lpszClassName)
          return pCl->idClass;
    }
    else
    {
      /*
        Check the first character of the class name. If it has the high-bit
        set, then this is one of the CT_xxx constants which is put out by
        the resource compiler. Map the CT_xxx class id into a class name.
      */
      BYTE ch = *lpszClass;
      if (ch & CT_MASK)
        lpszClass = aCTtoClassName[ch & 0x07];

      for (pCl = WndClassList;  pCl;  pCl = pCl->next)
        if (!lstricmp((LPSTR) lpszClass, (LPSTR) pCl->lpszClassName))
          return pCl->idClass;
    }
  }
  return -1;
}

/****************************************************************************/
/*                                                                          */
/* Function : WinGetClass()                                                 */
/*                                                                          */
/* Purpose  : Returns the 'class' field of a window structure, given a      */
/*            window handle.                                                */
/*                                                                          */
/* Returns  : The class field, or 0 if a bad window handle was passed.      */
/*                                                                          */
/****************************************************************************/
INT FAR PASCAL WinGetClass(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? w->idClass : 0;
}

LPCSTR FAR PASCAL WinGetClassName(hWnd)
  HWND hWnd;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  return (w) ? ClassIDToClassName(w->idClass) : NULL;
}


INT FAR PASCAL GetClassName(hWnd, buf, n)
  HWND  hWnd;
  LPCSTR buf;
  int   n;
{
  LPCSTR pClass = WinGetClassName(hWnd);
  int    iLen;

  if (pClass && !ISNUMERICRESOURCE(pClass))
  {
    lstrncpy((LPSTR) buf, (LPSTR) pClass, n);
    iLen = lstrlen((LPSTR) pClass);
    return min(iLen, n);
  }
  else
    return 0;
}


DWORD FAR PASCAL GetClassStyle(hWnd)
  HWND hWnd;
{
  LPWINDOW w = WID_TO_WIN(hWnd);

  if (w)
    return ClassIDToClassStruct(w->idClass)->style;
  else
    return 0L;
}


LPCSTR FAR PASCAL ClassIDToClassName(id)
  UINT id;
{
  register EXTWNDCLASS *pCl;
  return ((pCl = ClassIDToClassStruct(id)) != (EXTWNDCLASS *) NULL)
                                              ? pCl->lpszClassName : NULL;
}

WINPROC *FAR PASCAL ClassToWinProc(class)
  int class;
{
  register EXTWNDCLASS *pCl;
  return ((pCl = ClassIDToClassStruct(class)) != (EXTWNDCLASS *) NULL)
                                          ? pCl->lpfnWndProc : NULL;
}

WINPROC *FAR PASCAL ClassIDToDefProc(id)
  UINT id;
{                              
  register EXTWNDCLASS *pCl;
  return ((pCl = ClassIDToClassStruct(id)) != (EXTWNDCLASS *) NULL)
                                          ? pCl->lpfnDefWndProc : NULL;
}

#ifdef NOTUSED
long (FAR PASCAL *FAR PASCAL ClassIDToWndProc(id))()
  UINT id;
{                              
  register EXTWNDCLASS *pCl;
  return ((pCl = ClassIDToClassStruct(id)) != (EXTWNDCLASS *) NULL)
                                          ? pCl->lpfnWndProc : NULL;
}
#endif

EXTWNDCLASS *FAR PASCAL ClassIDToClassStruct(id)
  UINT id;
{
  register EXTWNDCLASS *pCl;

  if (id < MAXCLASSES)
    return ClassIndex[id];

  for (pCl = WndClassList;  pCl;  pCl = pCl->next)
    if (id == pCl->idClass)
      return pCl;

#if 1
  /*
    Instead of returning NULL, return the NORMAL class structure
  */
  return ClassIndex[0];
#else
  return (EXTWNDCLASS *) NULL;
#endif
}

#ifdef NOTUSED
int FAR PASCAL ClassNameToBaseClass(lpszClass)
  LPCSTR lpszClass;
{
  int iClass = ClassNameToClassID(lpszClass);

  if (iClass >= 0)
    return ClassIDToClassStruct(iClass)->idBaseClass;
  else
    return -1;
}
#endif

/****************************************************************************/
/*                                                                          */
/* Function : GetClassInfo()                                                */
/*                                                                          */
/* Purpose  : Windows 3.0 function which copies a class structure into a    */
/*            user-passed buffer.                                           */
/*                                                                          */
/* Returns  : TRUE if the class structure was returned, FALSE if not.       */
/*                                                                          */
/****************************************************************************/

BOOL FAR PASCAL GetClassInfo(hInstance, lpClassName, lpWndClass)
  HANDLE     hInstance;
  LPCSTR     lpClassName;
  LPWNDCLASS lpWndClass;
{
  LPEXTWNDCLASS pClass;
  int        iClass;

  (void) hInstance;

  if ((iClass = ClassNameToClassID(lpClassName)) >= 0 &&
			(pClass = ClassIDToClassStruct(iClass)) != NULL)
  {
    lmemcpy((LPSTR) lpWndClass, (LPSTR) pClass, sizeof(WNDCLASS));
    return TRUE;
  }
  else
    return FALSE;
}


BOOL FAR PASCAL ExtGetClassInfo(hInstance, lpClassName, lpWndClass)
  HANDLE     hInstance;
  LPCSTR     lpClassName;
  LPEXTWNDCLASS lpWndClass;
{
  LPEXTWNDCLASS pClass;
  int        iClass;

  (void) hInstance;

  if ((iClass = ClassNameToClassID(lpClassName)) >= 0 &&
			(pClass = ClassIDToClassStruct(iClass)) != NULL)
  {
    lmemcpy((LPSTR) lpWndClass, (LPSTR) pClass, sizeof(EXTWNDCLASS));
    return TRUE;
  }
  else
    return FALSE;
}


/****************************************************************************/
/*                                                                          */
/* Function : _RegisterPredefinedClasses()                                  */
/*                                                                          */
/* Purpose  : Called at MEWEL start-up time to register all of the          */
/*            predefined control classes.                                   */
/*                                                                          */
/* Returns  : Zilch.                                                        */
/*                                                                          */
/****************************************************************************/

#if defined(MOTIF)
#define ButtonWinProc      _XButtonWndProc
#define EditWinProc        _XEditWndProc
#define ListBoxWinProc     _XListBoxWndProc
#define StaticWinProc      _XTextWndProc
#endif

void FAR PASCAL _RegisterPredefinedClasses(void)
{
  if (bRegisteredPredefined)
    return;

  bInRegisterPredefinedClasses++;

  _RegisterClass(0, 0, 0,                     (PCSTR) "NORMAL",     StdWindowWinProc);
  _RegisterClass(0, 0, sizeof(CHECKBOXINFO),  (PCSTR) "BUTTON",     ButtonWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "EDIT",       EditWinProc);
  _RegisterClass(0, 0, sizeof(LISTBOX),       (PCSTR) "LISTBOX",    ListBoxWinProc);
  _RegisterClass(0, 0, sizeof(SCROLLBARINFO), (PCSTR) "SCROLLBAR",  ScrollBarWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "STATIC",     StaticWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "PUSHBUTTON", ButtonWinProc);
  _RegisterClass(0, 0, sizeof(CHECKBOXINFO),  (PCSTR) "CHECKBOX",   ButtonWinProc);
  _RegisterClass(0, 0, sizeof(CHECKBOXINFO),  (PCSTR) "RADIOBUTTON",ButtonWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "TEXT",       StaticWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "FRAME",      StaticWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "BOX",        StaticWinProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "ICON",       StaticWinProc);
  _RegisterClass(0, 0, sizeof(COMBOBOX),      (PCSTR) "COMBOBOX",   ComboBoxWndProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "DIALOG",     DefDlgProc);
  _RegisterClass(0, 0, 0,                     (PCSTR) "MENU",       MenuWndProc);

  bRegisteredPredefined = TRUE;
  bInRegisterPredefinedClasses--;
}


/****************************************************************************/
/*                                                                          */
/* Function : _RegisterClass()                                              */
/*                                                                          */
/* Purpose  : Fills a WNDCLASS data structure with the passed params and    */
/*            calls RegisterClass()                                         */
/*                                                                          */
/* Returns  : Whatever RegisterClass() returns.                             */
/*                                                                          */
/****************************************************************************/

static INT PASCAL _RegisterClass(dwStyle, iWndExtra, iClsExtra, lpszName, proc)
  UINT  dwStyle;
  UINT  iClsExtra;
  UINT  iWndExtra;
  LPCSTR  lpszName;
  WINPROC *proc;
{
  EXTWNDCLASS cl;

  memset((char *) &cl, 0, sizeof(cl));

  cl.style          = dwStyle;
  cl.cbClsExtra     = iClsExtra;
  cl.cbWndExtra     = iWndExtra;
  cl.lpszClassName  = lpszName;
  cl.lpfnWndProc    = proc;
  cl.lpfnDefWndProc = proc;
  if (lpszName[0] == 'D' && lpszName[1] == 'I')
    cl.lpszBaseClass  = "NORMAL";
  else
    cl.lpszBaseClass  = (LPSTR) lpszName;

#if defined(MOTIF)
  cl.hbrBackground  = (HBRUSH) (COLOR_WINDOW+1);
#else
  cl.hbrBackground  = BADOBJECT;
#endif

  return ExtRegisterClass(&cl);
}

