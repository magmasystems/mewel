/*===========================================================================*/
/*                                                                           */
/* File    : WINHELP.H                                                       */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989,1990 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#include <help.h>

#define HELP_RES_FILE       "HELPCOMP"

#define HELPERR_CONTEXTNOTFOUND   (-1)
#define HELPERR_BAD_HELPFILE      (-1)
#define HELPERR_BAD_RCFILE        (-2)

/*
  Help Color Attributes
*/
#define HELPCLR_FIRST             0
#define HELPCLR_NORMAL            0
#define HELPCLR_BOLD              1
#define HELPCLR_ITALICS           2
#define HELPCLR_BOLDITALICS       3
#define HELPCLR_UNDERLINE         4
#define HELPCLR_BOLDUNDERLINE     5
#define HELPCLR_ITALICSUNDERLINE  6
#define HELPCLR_BOLDITALICSUNDERLINE  7
#define HELPCLR_HOTSPOT           8
#define HELPCLR_LAST              8


/*===========================================================================*/
/*                                                                           */
/* File    :                                                                 */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989,1990 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/

#define ID_EXIT             999
#define ID_OPEN             110
#define ID_CLOSE            111
#define ID_SEARCH           112
#define ID_TOPIC            230

#define ID_NEXT             300
#define ID_PREV             301
#define ID_BACK             302

extern int PASCAL HelpQueryColor(UINT iSysColor);
extern int PASCAL HelpSetColor(UINT iSysColor, UINT attr);

#define MAX_CONTEXT_LEN     100 /* Max help context string length */
extern int FAR PASCAL WinHelp(HWND hwndMain, LPCSTR lpszHelp, UINT usCommand, DWORD ulData);
