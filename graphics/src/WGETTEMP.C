/*===========================================================================*/
/*                                                                           */
/* File    : WGETTEMP.C                                                      */
/*                                                                           */
/* Purpose :                                                                 */
/*                                                                           */
/* History : Created by Wolfgang Lorenz                                      */
/*                                                                           */
/* (C) Copyright 1993-1993 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"


int FAR PASCAL GetTempFileName(bDrive, pcPrefix, wNumber, pcBuffer)
  BYTE  bDrive;
  LPSTR pcPrefix; 
  UINT  wNumber;
  LPSTR pcBuffer;
{ 
  PSTR  pc;


  (void) bDrive;

  /*
    Get a pointer to a temp directory. Try the TEMP and TMP env vars.
  */
  if ((pc = getenv("TEMP")) == NULL &&
      (pc = getenv("TMP"))  == NULL)
    pc = "C:"; 

  /*
    Create a full DOS path specifier.
  */
  _fullpath(pcBuffer, pc, 80); 

  /*
    Make sure that the last char of the directory is a slash
  */
  pc = pcBuffer + strlen(pcBuffer); 
  if (pc[-1] != CH_SLASH)
    *pc++ = CH_SLASH;

  /*
    If the name of the temp file has a prefix, use it
  */
  if (pcPrefix)
  {
    strcpy(pc, pcPrefix); 
    pc += strlen(pcPrefix); 
  } 

  /*
    Get the rest of the temp filename from the system time if the user
    did not specify a number
  */
  if (!wNumber)
  {
#if defined(DOS) && !defined(EXTENDED_DOS)
    UINT wNumber = * (UINT *) 0x0000046CL;
#else
    UINT wNumber = 10000;
#endif
    do
    { 
      wsprintf(pc, "%05u.TMP", ++wNumber); 
    } while (!access(pcBuffer, 0)); 
  }
  else
    wsprintf(pc, "%05u.TMP", wNumber); 

  /*
    Translate the string to upper case and return the file number.
  */
  AnsiUpper(pcBuffer); 
  return (int) wNumber; 
} 


BYTE WINAPI GetTempDrive(chDrive)
  char chDrive;
{
  char *pEnv;

  (void) chDrive;  /* ignored in Windows */

  /*
    Get a pointer to a temp directory. Try the TEMP and TMP env vars.
  */
  if (((pEnv = getenv("TEMP")) != NULL || (pEnv = getenv("TMP")) != NULL) &&
      pEnv[1] == ':')
    return toupper(pEnv[0]);

  return 'C';
}

