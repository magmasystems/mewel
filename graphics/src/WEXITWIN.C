/*===========================================================================*/
/*                                                                           */
/* File    : WEXITWIN.C                                                      */
/*                                                                           */
/* Purpose : Implements the ExitWindows() function                           */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

BOOL FAR PASCAL ExitWindows(dwReserved, wReturnCode)
  DWORD dwReserved;
  UINT  wReturnCode;
{
  WINDOW *w;
  WINDOW *wLastSent = (WINDOW *) NULL;
  int    rc = TRUE;

  (void) dwReserved;

  /*
    Send WM_QUERYENDSESSION messages to the top-level windows. The SDK says
    that if a zero is returned by any window, then we must send
    WM_ENDSESSION(0) to all of the windows who we previously sent 
    WM_QUERYENDSESION messages to.
  */
  for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
    if ((rc = (int) SendMessage(w->win_id,WM_QUERYENDSESSION,0,0L)) == FALSE)
    {
      wLastSent = w;
      break;
    }

  for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
  {
    SendMessage(w->win_id, WM_ENDSESSION, rc, 0L);
    if (w == wLastSent)
      break;
  }

  if (rc)
  {
    /* Must call VidTerminate() in the UNIX version prior to shutdown. Really,
     *   this means that all curses apps must call endwin() prior to closedown.
     *   Just calling system("stty sane") isn't good enough- some UNIX's will
     *   kill the login session (ie. MICROPORT).
     */
#ifdef UNIX
    VidTerminate();
#endif
    exit(wReturnCode);
  }
  return FALSE;
}

