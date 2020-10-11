/*===========================================================================*/
/*                                                                           */
/* File    : WANYPOP.C                                                       */
/*                                                                           */
/* Purpose : Implements the AnyPopup() function                              */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1991 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include "wprivate.h"
#include "window.h"

BOOL FAR PASCAL AnyPopup(void)
{
  WINDOW *w;

  for (w = InternalSysParams.wDesktop->children;  w;  w = w->sibling)
    if (w->flags & WS_POPUP)
      return TRUE;
  return FALSE;
}

