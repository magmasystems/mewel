/*===========================================================================*/
/*                                                                           */
/* File    : WKBSTACK.C                                                      */
/*                                                                           */
/* Purpose : Implements the keyboard pushback buffer                         */
/*                                                                           */
/* History :  Feb 2, 1992 (maa) - separated from WINKBD.C                    */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1992         Marc Adler/Magma Systems  All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

#define PUSHBACKBUFSIZE  8

static struct
{
  INT      iStackSP;
  VKEYCODE vkeyStack[PUSHBACKBUFSIZE];
} PushBackInfo =
{
  -1
};


/* nungetc - pushes on char onto the input stack */
VOID FAR PASCAL nungetc(c)
  VKEYCODE c;
{
  if (PushBackInfo.iStackSP < PUSHBACKBUFSIZE)
  {
    PushBackInfo.vkeyStack[++PushBackInfo.iStackSP] = c;
    SET_PROGRAM_STATE(STATE_PUSHBACKCHAR_WAITING);
  }
}

/* ngetc - this is the main routine which reads in characters */
VKEYCODE FAR PASCAL ngetc(void)
{
  VKEYCODE c;

  /* See if there are characters in the internal character buffer   */
  if (PushBackInfo.iStackSP >= 0)
  {
    c = PushBackInfo.vkeyStack[PushBackInfo.iStackSP--];
    if (PushBackInfo.iStackSP < 0)
      CLR_PROGRAM_STATE(STATE_PUSHBACKCHAR_WAITING);
  }
  else
  {
#if (defined(sunos) || defined(VAXC))  /* enable timers */
    c = KBDRead();
#else
    while ((c = KBDRead()) == 0) ;   /* wait until a char is ready  */
#endif
  }
  return c;
}

