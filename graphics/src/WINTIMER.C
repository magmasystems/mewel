/*===========================================================================*/
/*                                                                           */
/* File    : WINTIMER.C                                                      */
/*                                                                           */
/* Purpose : Routines to implement the single software timer                 */
/*                                                                           */
/* History :                                                                 */
/*           09/10/90 (maa) added hWnd arg to KillTimer()                    */
/*           05/13/93 (maa) Windows 3.1 compatibility                        */
/*                                                                           */
/* (C) Copyright 1989-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/* Maximum BIOS Time of Day Tick Value before rollover to 0 */
#define BIOSTODMAX  1573039L
#define BIGDIFF     786520L

typedef struct timer
{
  HWND  hWnd;
  UINT  id;
  WINPROC *func;
  UINT  interval;
  DWORD finaltime;
} TIMER, *PTIMER, FAR *LPTIMER;

/*
  The list of currently active timers.
*/
static LIST *TimerList = NULL;


#if defined(UNIX) || defined(VAXC)
#define	DELTA_T(i)	(i)
#else
#define	DELTA_T(i)	(((i) * 18L + 500) / 1000);
#endif

/*
  SetTimer(hWnd, id, interval, timerFunc)
    sets a single timer
*/
UINT FAR PASCAL SetTimer(hWnd, id, interval, timerFunc)
  HWND hWnd;
  UINT id;            /* ignored if hWnd==NULL */
  UINT interval;
  TIMERPROC timerFunc;
{
  LPTIMER t;
  static  UINT nTimers = 0;

  /*
    Make sure that any timers with the same id are killed.
  */
  KillTimer(hWnd, id);
    
  /*
    Allocate a timer structure and fill the fields.
  */
  if ((t = (LPTIMER) EMALLOC_FAR(sizeof(TIMER))) == NULL)
    return FALSE;
  nTimers++;
  t->hWnd      = hWnd;
  t->id        = (hWnd == NULL || id == 0) ? nTimers : id;
  t->interval  = interval;
  t->func      = (WINPROC *) timerFunc;
  t->finaltime = BIOSGetTime() + DELTA_T(interval);

#if defined(DOS)
  /* Account for BIOS TOD rollover */
  if (t->finaltime > BIOSTODMAX)
    t->finaltime -= BIOSTODMAX;
#endif

  /*
    Append the new timer to the timer list.
  */
  ListAdd(&TimerList, ListCreate((LPSTR) t));

#ifdef OS2
  StartTimerThread();
#endif

  return t->id;
}


BOOL FAR PASCAL KillTimer(hWnd, id)
  HWND hWnd;
  UINT id;
{
  LIST *p;

  /*
    Go through the list, searching for a timer whose identifier is id.
    When we find it, delete it from the timer list.
  */
  for (p = TimerList;  p;  p = p->next)
  {
    if (((LPTIMER) p->data)->id == id && ((LPTIMER) p->data)->hWnd == hWnd)
    {
      ListDelete(&TimerList, p);
      break;
    }
  }

#ifdef OS2
  if (!TimerList)
    KillTimerThread();
#endif
  return TRUE;
}


/*
  KillWindowTimers()
    Kills all of the timer which are associated with window hWnd. This
    is called when we destroy a window.
*/
VOID FAR PASCAL KillWindowTimers(hWnd)
  HWND hWnd;
{
  LIST *p, *nextp;

  /*
    Go through the list, searching for a timer whose window is hWnd.
    When we find it, delete it from the timer list.
  */
  for (p = TimerList;  p;  p = nextp)
  {
    nextp = p->next;   /* save next ptr in case node is deleted */
    if (((LPTIMER) p->data)->hWnd == hWnd)
      ListDelete(&TimerList, p);
  }


#ifdef OS2
  if (!TimerList)
    KillTimerThread();
#endif
}


/*
  TimerCheck()
    This is called from GetEvent() in order to see if a timer event is
    pending. Returns TRUE if a timer expired, FALSE if not.
*/
INT FAR PASCAL TimerCheck(
  LPMSG  event,
  BOOL   bPostIt   /* should we post events, or are we checking the queue */
)
{
  DWORD   currtime;
  LIST    *p;
  LPTIMER t;
  int     rc = FALSE;

  (void) event;

  /*
    Quick check to see if there are any outstanding timers at all.
  */
  if (!TimerList)
    return FALSE;
    
  currtime = BIOSGetTime();


  /*
    Go through the list, searching for a timer which has expired.
  */
  for (p = TimerList;  p;  p = p->next)
  {
    t = (LPTIMER) p->data;

#if defined(DOS)
    if (currtime >= t->finaltime && currtime - t->finaltime < BIGDIFF)
#else
    if (currtime >= t->finaltime)
#endif
    {
      /* re-init the next final time */
      t->finaltime = currtime + DELTA_T(t->interval);
#if defined(DOS)
      if (t->finaltime > BIOSTODMAX) /* Account for BIOS TOD rollover */
        t->finaltime -= BIOSTODMAX;
#endif

      if (bPostIt)
      {
        if (t->func)
          (*t->func)(t->hWnd, WM_TIMER, t->id, currtime);
        else
          PostEvent(t->hWnd, WM_TIMER, t->id, currtime, currtime);
      }
      rc = TRUE;
    }
  }

  return rc;
}

