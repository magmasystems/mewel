/*===========================================================================*/
/*                                                                           */
/* File    : WINTERM.C                                                       */
/*                                                                           */
/* Purpose : WinTerminate()                                                  */
/*             Closes the window system                                      */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/
#define FREE_REMAINING_MEMORY
#define INCLUDE_MEMMGR

#include "wprivate.h"
#include "window.h"

extern VOID CDECL MemoryTerminate(void);


#if defined(WC386) && !defined(__HIGHC__)
void pushebx(void);
#pragma aux pushebx = "push ebx";
void popebx(void);
#pragma aux popebx = "pop ebx";
#endif

#if defined(__WATCOMC__)
VOID       WinTerminate()
#else
VOID CDECL WinTerminate()
#endif
{
#if defined(WC386) && !defined(__HIGHC__)
  pushebx();
#endif

  SET_PROGRAM_STATE(STATE_EXITING);

#if defined(FREE_REMAINING_MEMORY)
  FreeAllRemainingMem();
#endif


#ifdef OS2
  SuspendEventThreads();
#endif

  KBDTerminate();

#ifdef DOS
  if (InternalSysParams.fOldBrkFlag) /* set break flag if it was ON initially */
    setbrk();
#endif

  int23res(); 
#ifdef DOS
  Int24Restore();
#endif
  MouseTerminate();
  VidTerminate();

#if !defined(__DPMI16__) && !defined(__DPMI32__)
  MemoryTerminate();
#endif

#if defined(__BORLANDC__) && defined(__DPMI16__) && defined(__DLL__)
  TerminateRTMSwitches();
#endif

#if defined(WC386) && !defined(__HIGHC__)
  popebx();
#endif
}



#if defined(FREE_REMAINING_MEMORY)
#include "wobject.h"
#include "winevent.h"
#include "winrc.h"
extern EXTWNDCLASS *WndClassList;
extern WORD       _ObjectTblSize;
extern LPOBJECT  *_ObjectTable;
extern VIRTUALSCREEN VirtualScreen;
extern QUEUE EventQueue;

#if defined(DOS) && defined(MSC) && !defined(EXTENDED_DOS)
#define TRUENEAR  _near
#else
#define TRUENEAR
#endif

extern WINDOW ** TRUENEAR HwndArray;
extern WINSYSPARAMS InternalSysParams;
extern WORD FAR *WinVisMap;
extern STRINGCACHE StringCache;

char *bigList[] = {
    "NORMAL","BUTTON","EDIT","LISTBOX","SCROLLBAR","STATIC",
    "PUSHBUTTON","CHECKBOX","RADIOBUTTON","TEXT","FRAME","BOX",        
    "ICON","COMBOBOX","DIALOG","MENU",NULL};
char *tinyList[] = {"NORMAL", NULL};

int polingKludge(char *s, char **list)
{
  int i;
  char *ptr;

  for (i = 0;  list[i];  i++)
  {
    ptr = list[i];
    if (strcmp(s, ptr) == 0)
      return TRUE;
  }
  return FALSE;
}



FreeAllRemainingMem()
{
  int i;
  WINDOW      *w, *wNext;
  EXTWNDCLASS *pCl,*pClNext;
    
  if (TEST_PROGRAM_STATE(STATE_SPAWNING))
    return FALSE;

  for (pCl = WndClassList;  pCl;  pCl = pClNext)
  {
    LPCSTR s;
    MyFree(pCl->lpszClassExtra);
    if ((s = pCl->lpszMenuName) != NULL && !ISNUMERICRESOURCE(s))
      MYFREE_FAR((LPSTR) s);
    if (!(ISNUMERICRESOURCE(pCl->lpszClassName))
          && !(polingKludge((LPSTR)pCl->lpszClassName,bigList)))
      MYFREE_FAR((LPSTR) pCl->lpszClassName);
    if (!(ISNUMERICRESOURCE(pCl->lpszBaseClass))
          && !(polingKludge((LPSTR)pCl->lpszBaseClass,bigList)))
      MYFREE_FAR((LPSTR) pCl->lpszBaseClass);
    pClNext = pCl->next;
    MyFree(pCl);
  }
  WndClassList = NULL;

  for (i = 0;  i < _ObjectTblSize;  i++)
  {
    if (_ObjectTable[i] && FP_SEG(_ObjectTable[i]) != 0)
      DeleteObject(i | OBJECT_SIGNATURE);
  }

#if !defined(MEWEL_GUI)
  free(VirtualScreen.pbRowBad);
  free(VirtualScreen.pVirtScreen);
  free(WinVisMap);
#endif
  MyFree(EventQueue.qdata);
  MyFree(HwndArray);

  for (w = InternalSysParams.WindowList;  w;  w = wNext)
  {
    wNext = w->next;
    MYFREE_FAR(w->title);
    MyFree(w->pWinExtra);
    MyFree(w->pPrivate);
    MyFree(w->pMDIInfo);
    free(w);
  }
  if (InternalSysParams.wDesktop)
  {
    MYFREE_FAR(InternalSysParams.wDesktop->title);
    free(InternalSysParams.wDesktop);
  }

#if !defined(__DPMI16__)
  for (i = 0;  i < CACHESIZE;  i++)
  {
    if (StringCache.cache[i].pszString)
      free(StringCache.cache[i].pszString);
  }
#endif

#if !defined(__DPMI16__) && !defined(__DPMI32__)
  for (i = 1;  i < MewelMemoryMgr.nHandlesAlloced;  i++)
  {
    LPMEMHANDLEINFO lpMI;
    if ((lpMI = DerefHandle(i)) != NULL && lpMI->uMem.lpMem)
      GlobalFree(i);
  }
  MYFREE_FAR(MewelMemoryMgr.HandleTable);
#endif

  return TRUE;
}
#endif

