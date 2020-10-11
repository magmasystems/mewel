/*===========================================================================*/
/*                                                                           */
/* File    : WINRES.C                                                        */
/*                                                                           */
/* Purpose : Functions to handle the loading of resources.                   */
/*                                                                           */
/* History : 07-Nov-90 DavidHollifield                                       */
/*              Added the ability to find the resources within an EXE        */
/*              no mater where they are located past the end of the EXE      */
/*                                                                           */
/* Enhancements to go :                                                      */
/*            Be able to parse NEWEXE header for OS/2.                       */
/*                                                                           */
/*                                                                           */
/* (C) Copyright 1989-1995 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#define NOGDI
#define NOOBJECT
#define MEWEL_RESOURCES

#include "wprivate.h"
#include "window.h"

#if (defined(UNIX) && !defined(VAXC)) || defined(__GNUC__)
#include <stddef.h>   /* for offsetof() macro */
#define sopen(f, m1, m2, m3)  open(f, m1)
#elif defined(VAXC)
#include <stddef.h>   /* for offsetof() macro */
#define sopen(f, m1, m2, m3)  open(f, m1, 0)
#define O_BINARY 0
#include <file.h>
#include <unixlib.h>
#include <unixio.h>
#else
#include <share.h>
#endif

#if !defined(offsetof)
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/*
  So far, only DOS can use EXE files.
*/
#if defined(UNIX) || defined(VAXC) || defined(OS2) || defined(MEWEL_32BITS)
#define CANT_READ_EXE
#endif


/*
  Header of an EXE file. Used by the DOS version in order to find out
  where the "normal" EXE bytes end and the MEWEL resources begin.
*/
typedef struct exehdr
{
  WORD signature;
  WORD nBytesOnLastPage;
  WORD nPages;
} EXEHDR;

typedef struct tagResTrailer
{
  DWORD ulResSig;
  DWORD ulResSeekPos;
} RESTRAILER;

/*
  Global vars
*/
static STRINGTBLHEADER _StringTblHeader;
static LPSTR    pszIDStrings = NULL;/* strings which are used as resource IDs */
extern BOOL bWinCompatDlg;

/*
  Prototypes
*/
static INT  PASCAL _MapStringIDToNumber(LPCSTR);
static LONG PASCAL FindExeResPos(HMODULE);


/****************************************************************************/
/*                                                                          */
/* Function : FindExeResPos(hModule)                                        */
/*                                                                          */
/* Purpose  : Examines a DOS exe file in order to find where the MEWEL      */
/*            resources begin. This is complicated by the fact that         */
/*            various compilers' debug info can appear at the end of the    */
/*            exe file.                                                     */
/*                                                                          */
/* Returns  : The seek position in the EXE file where the resources start.  */
/*            Returns 0 if the resources are not found.                     */
/*                                                                          */
/* OpSys    : For DOS only.                                                 */
/*                                                                          */
/****************************************************************************/
#ifndef CANT_READ_EXE
static LONG PASCAL FindExeResPos(hModule)
  HMODULE hModule;
{
  char    chBuf[512];
  union   SIGNATURE
  {
    char  ch[4];
    long  l;
  } Signature;
  EXEHDR  EXEhdr;
  RESTRAILER resTrailer;
  int     i;
  int     nCount;
  int     nMatch;
  DWORD   ulPos;

  static  LONG lResPos = -1L;  /* stores the resource's seek position */


  if (lResPos == -1L)
  {
    /*
      Check the end of the EXE file to see if we have the RES trailer.
      If we do, then seek to the resources and return the position.
    */
    ulPos = lseek(hModule, 0L,  2);
    ulPos = lseek(hModule, -8L, 1);
    (void) ulPos;
    read(hModule, (char *) &resTrailer, sizeof(resTrailer));
    if (resTrailer.ulResSig == RC_SIGNATURE)
    {
      return (lResPos = lseek(hModule, resTrailer.ulResSeekPos, 0));
    }

    /*
      We need to examine the EXE header to find the end of the natural
      EXE image. This is calculated by 
      (EXEHDR.nPages - 1) * 512 bytes per page + EXEHDR.bytes_on_last_page
    */
    lseek(hModule, 0L, 0);
    read(hModule, (char *) &EXEhdr, sizeof(EXEhdr));
    lResPos = (LONG) (EXEhdr.nPages-1) * 512 + EXEhdr.nBytesOnLastPage;
    lResPos = lseek(hModule, lResPos, 0);

    /*
      Try to find the resource signature past the end of the natural
      EXE image.
    */
    Signature.l = RC_SIGNATURE;
    nMatch = 0;

    for (;;)
    {
      /*
        Grab 512 bytes
      */
      nCount = read(hModule, chBuf, sizeof(chBuf));

      if (nCount > 0)
      {
        /*
          Faster method of scanning for signature.  Use memchr() to scan for
          the first byte.  The method used below *does* handle cases where the
          pattern crosses the buffer boundary.
        */
        i = 0;
        while (i < nCount)
        {
          if (nMatch == 0)
          {
            PSTR idx = memchr(chBuf+i, Signature.ch[0], nCount-i);

            if (idx != NULL)
            {
              nMatch = 1;
              lResPos += idx - (chBuf+i) + 1;
              i = idx - chBuf + 1;
            }
            else
            {
              lResPos += nCount - i;
              i = nCount;
            }
          }
          if (nMatch != 0)
          {
            while (i < nCount)
            {
              lResPos++;
              if (chBuf[i++] == Signature.ch[nMatch])
              {
                nMatch++;
                if (nMatch == sizeof(LONG))
                {
                  lResPos = lseek(hModule, lResPos - nMatch, 0);
                  return lResPos;
                }
              }
              else
              {
                nMatch = 0;
                lResPos--;
                i--;
                break;
              }
            }
          }
        }
      }
      else /* (nCount <= 0) */
      {
        /*
          At end of file!  Cannot find resource signature.
        */
        return (lResPos = 0);
      }
    }
  }
  else
  {
    /*
      lResPos is not -1L, so we already know where the end of the EXE
      file is.
    */
    lResPos = lseek(hModule, lResPos, 0);
    return lResPos;
  }
}
#endif /* CANT_READ_EXE */


/****************************************************************************/
/*                                                                          */
/* Function : OpenResourceFile(szFilename)                                  */
/*                                                                          */
/* Purpose  : Opens a MEWEL resource file, either a RES file or an EXE file.*/
/*            If the resources are found, the ID-string table is read and   */
/*            stored in memory until the file is closed.                    */
/*                                                                          */
/* Returns  : The file descriptor of the open resource file if it succeeds. */
/*            If it fails, 0 is returned.                                   */
/*            If the resource file is in a DOS exe, then a special bit is   */
/*            turned on in the file descriptor.                             */
/*                                                                          */
/****************************************************************************/
HINSTANCE FAR PASCAL OpenResourceFile(szFileName)
  PSTR  szFileName;
{
  char  fname[MAXPATH];  /* pathname of res file */
  int   fd;              /* file descriptor of res file */
  LONG  lResPos = 0L;    /* seek position */

  /*
    If the user sends down a NULL resource name, then look at the end of
    this app's EXE file. The complete pathname of the app is stored in
    _pgmptr (but only for DOS 3.x and higher).
  */
  if (szFileName == NULL)
  {
    InternalGetModuleFileName(fname, sizeof(fname));
  }
  else
  {
    char *pDot, *pSlash;

    strcpy(fname, szFileName);
#if defined(VAXC)
    { 
      /*
       WZV & RENS,
       We suppose that the xxxx.res file is in the current directory
       while the exe file can be somewhere else.
       In VMS, the name in szFileName might contain the full path,
       Therefor we strip off the path part as well.
      */
      char *cp;
      if ((cp = strrchr((char *) fname, ']')) != NULL)
        strcpy(fname, cp+1);
      if ((cp = strrchr((char *) fname, '.')) != NULL)
        *cp = '\0';     /* zap extension */
    }
#else
    pDot   = strrchr(fname, '.');
    pSlash = strrchr(fname, CH_SLASH);
    if (pDot == NULL || (pSlash != NULL && pDot < pSlash))
      /* We have a dot, but not in a directory name */
#endif
      strcat(fname, ".res");
  }

  /*
    If an open resource file already exists, close it. But do so only
    if it's located in the EXE file.
  */
  if (MewelCurrOpenResourceFile != -1 &&
      MewelCurrOpenResourceFile < 0)
    CloseResourceFile(MewelCurrOpenResourceFile);


  /*
    Search for the RES file. If we find it, try to open it for reading.
  */
  if (!_DosSearchPath("PATH", fname, fname) || 
#if defined(PLTNT) && defined(MSC)
      (fd =  open(fname, O_RDONLY | O_BINARY                    )) < 0)
#elif defined(DOS) && defined(MSC)
      (fd = sopen(fname, O_RDONLY | O_BINARY, 0,         S_IREAD)) < 0)
#else
      (fd = sopen(fname, O_RDONLY | O_BINARY, SH_DENYNO, S_IREAD)) < 0)
#endif
    return 0;


#ifndef CANT_READ_EXE
  /*
    If we are reading from the EXE file, try to locate the start of the
    MEWEL resources within the EXE file.
  */
  if (szFileName == NULL)
  {
    lResPos = FindExeResPos(fd);
    if (lResPos == 0L)      /* can't find the resource table */
    {
      close(fd);
      return 0;
    }
  }
#endif


  /*
    Read the ID string-table header and adjust the offset where you find
    the actual name table.
  */
  read(fd, (char *) &_StringTblHeader, sizeof(_StringTblHeader));
  _StringTblHeader.ulSeekPos += lResPos;
  if (pszIDStrings)
  {
    MyFree_far(pszIDStrings);
    pszIDStrings = NULL;
  }

  /*
    If we have string-id's defined in this resource file, allocate
    memory for the strings and read the ID table into memory.
  */
  if (_StringTblHeader.ulBytes > 0)
  {
    if ((pszIDStrings = emalloc_far((DWORD) _StringTblHeader.ulBytes)) == NULL)
    {
      close(fd);
      return 0;
    }
    lseek(fd, _StringTblHeader.ulSeekPos, 0);
    _lread(fd, pszIDStrings, (UINT) _StringTblHeader.ulBytes);
  }

  /*
    We signify that we are looking at the app's EXE file by turning on
    the high bit of the module file descriptor.
  */
#ifndef CANT_READ_EXE
  if (szFileName == NULL)   /* reading from EXE file? */
    fd |= 0x8000;
#endif

  MewelCurrOpenResourceFile = fd;
  return fd;
}


/****************************************************************************/
/*                                                                          */
/* Function : CloseResourceFile(hModule)                                    */
/*                                                                          */
/* Purpose  : Closes an open resource file. Resets the StringCache fields.  */
/*                                                                          */
/* Returns  : Nada.                                                         */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL CloseResourceFile(hModule)
  HANDLE hModule;
{
  extern VOID FAR PASCAL _StringCacheClose(void);

  if (hModule != 0)
    close((UINT)hModule & 0x00FF);
  _StringCacheClose();
  MewelCurrOpenResourceFile = -1;
}


/****************************************************************************/
/*                                                                          */
/* Function : GetResource() and _GetResource().                             */
/*                                                                          */
/* Purpose  : Tries to load a resource of type lpszType and name lpszID.    */
/*            _GetResource() is called by FindResource().                   */
/*                                                                          */
/* Returns  : A far pointer to the memory block where the resource is       */
/*            stored. This memory block must be freed by any app which      */
/*            calls GetResource(). If the resource was not found, NULL is   */
/*            returned.                                                     */
/*                                                                          */
/****************************************************************************/
LPSTR FAR PASCAL GetResource(hModule, lpszID, lpszType)
  HMODULE hModule;   /* RES file handle  */
  LPSTR   lpszID;    /* Name of resource */
  LPCSTR  lpszType; /* Type of resource, usually RT_xxx */
{
  return _GetResource(hModule, lpszID, lpszType, NULL, NULL);
}

LPSTR FAR PASCAL _GetResource(hModule, lpszID, lpszType, pulSeekPos, pwSize)
  HMODULE hModule;     /* RES file handle  */
  LPSTR lpszID;      /* Name of resource */
  LPCSTR lpszType;   /* Type of resource, usually RT_xxx */
  DWORD *pulSeekPos; /* Seek position where the resource was found */
  DWORD *pwSize;     /* Size of the resource in bytes */
{
  RESOURCE res;
  BOOL     bOpenedEXE = FALSE;
  LONG     lResPos;
  INT      idName;
  INT      idType;
  BOOL     bReadIt = (pulSeekPos == NULL);


  /*
    Sanity check to make sure we're not using stdin, stdout, or stderr.
  */
#ifdef CANT_READ_EXE
  if (hModule <= 2)
    return NULL;
#endif

#ifndef CANT_READ_EXE
  /*
    If the user specified NULL as the module handle for the LoadXXXX()
    functions, then try to open the EXE file. But open it only if it
    is not open already.
  */
  if ((!hModule || hModule == (HMODULE) -1) && MewelCurrOpenResourceFile == -1)
  {
    if ((hModule = OpenResourceFile(NULL)) == 0)
      return NULL;
    bOpenedEXE = TRUE;
  }
  /*
    If we are dealing with the EXE file and it was already open, then use the
    file handle located in MewelCurrOpenResourceFile
  */
  if (!hModule)
    hModule = MewelCurrOpenResourceFile;

  if (hModule & 0x8000)
  {
    /*
      If the high bit of the module handle is turned on, then the resource
      is found at the end of the app's EXE file.
    */
    hModule &= 0x00FF;
    lResPos = FindExeResPos(hModule);
    if (lResPos == 0L)
      return NULL;
  }
  else
#endif
  {
    /*
      If we are using an external RES file, then start seeking from the
      start of the file.
    */
    lseek(hModule, 0L, 0);
    lResPos = 0L;
  }


  /*
    The first thing which we find in the resource section is the String
    ID header. We have already read this in, so just bypass it.
  */
  lseek(hModule, (DWORD) sizeof(STRINGTBLHEADER), 1);

  /*
    If the resource id is a far pointer to a string (the segment isn't 0),
    then translate the string id to a numerical id.
  */
#if defined(MEWEL_32BITS)
  idType = (INT) lpszType;
  if (lpszType == RT_STRING)
  {
    idName = (INT) (DWORD) lpszID;
  }
  else
  {
    /*
      Decode the type
    */
    if (idType > 0x8000L)
    {
      if (_StringTblHeader.ulBytes == 0 || 
                  (idType = _MapStringIDToNumber(lpszType)) == 0)
        goto bye;
    }
  }

  /* 
    If lpszID is a small number (less than 0x8000L), it is a resource ID
    This is the only way to distinguish a 32-bit integer resource id from
    a 32-bit string pointer.
  */
  if ((((long) lpszID) & 0x0000FFFF) == ((long) lpszID))
    idName = (INT) (DWORD) lpszID;
  else if (lpszID[0] == '#')
    idName = atoi(lpszID+1);
  else if (_StringTblHeader.ulBytes == 0 || 
              (idName = _MapStringIDToNumber(lpszID)) == 0)
    goto bye;

#else /* DOS or OS/2 */
  /*
    Decode the type
  */
  if (FP_SEG(lpszType) != 0x0000)
  {
    if (_StringTblHeader.ulBytes == 0 || 
                (idType = _MapStringIDToNumber(lpszType)) == 0)
      goto bye;
  }
  else
    idType = (INT) (DWORD) lpszType;

  /*
    Decode the resource name
  */
  if (FP_SEG(lpszID) != 0x0000)
  {
    /*
      A resource id of "#nnn" where nnn is a number refers to a resource
      with numeric ID nnn.
    */
    if (lpszID[0] == '#')
    {
      char szBuf[32];
      lstrcpy(szBuf, lpszID+1);  /* near/far mismatch nonsense */
      idName = atoi(szBuf);
    }
    else if (_StringTblHeader.ulBytes == 0 || 
             (idName = _MapStringIDToNumber(lpszID)) == 0)
      goto bye;
  }
  else
    idName = (INT) (DWORD) lpszID;

#endif



  for (;;)
  {
    /*
      If we encountered the string id items, then we have finished
      examining the normal resources. Since we haven't found the
      item, exit.
    */
    if (_StringTblHeader.ulSeekPos > 0 &&
                         (DWORD) tell(hModule) >= _StringTblHeader.ulSeekPos)
      break;

    /*
      Read the header for the resource
    */
    if (read(hModule, (char *) &res, sizeof(res)) <= 0)
      break;

    /*
      See if we have the proper type and the proper id
    */
    if ((INT) res.iResType == idType && (INT) res.iResID == idName)
    {
      DWORD cb;
      LPSTR pData = NULL;
      UINT  nIndex;

      /*
        Determine the size of the resource
      */
      if (idType == (UINT) RT_STRING)
      {
        /* 
          For stringtables, only read the index blocks in. The first WORD
          is the number of index blocks for the string table.
        */
        read(hModule, (char *) &nIndex, sizeof(nIndex));
        cb = (DWORD) (sizeof(nIndex) + (nIndex * sizeof(INDEX)));
      }
      else
      {
        cb = res.nResBytes;
        if (idType == (UINT) RT_DIALOG)
          bWinCompatDlg = (res.bResType == RES_WINCOMPATDLG);
      }

      /*
        Record the resource size
      */
      if (pwSize)
        *pwSize = cb;

      /*
        Allocate memory to hold the resource
      */
      if (bReadIt && (pData = emalloc_far(cb)) == NULL)
        goto bye;

      if (idType == (UINT) RT_STRING && pData != NULL)
      {
        /*
          Read the index blocks into the memory area.
        */
        * (LPUINT) pData = nIndex;  /* number of indices */
        _lread(hModule, pData + sizeof(nIndex), (UINT) (cb - sizeof(nIndex)));
        
        /*
          If we read the stringtable in from the EXE file, then we must
          adjust the seek positions which are specified in each index
          block.
        */
        if (lResPos)
        {
          LPINDEX pIndex;
          for (pIndex = (LPINDEX)(pData+sizeof(nIndex));  nIndex-- > 0;  pIndex++)
          {
            pIndex->ulSeekPos    += lResPos;
            pIndex->ulSeekPosEnd += lResPos;
          }
        }
      }
      else
      {
        /*
          Read a non-string resource in. This little loop allows us
          to read in > 64K resources.
        */
        if (pData)
        {
          UINT  wBytes;
          LPSTR pD = pData;

          while (cb > 0)
          {
            wBytes = (UINT) (min(cb, 0x7FFF));
            _lread(hModule, pD, wBytes);
            pD += wBytes;
            cb -= wBytes;
          }
        }
      }

      /*
        Record the seek position
      */
      if (pulSeekPos)
        *pulSeekPos = tell(hModule);

      if (bOpenedEXE)
        CloseResourceFile(hModule);

      return (bReadIt) ? pData : (LPSTR) 1L;
    }

    /*
      The resource wasn't found yet. Skip to the next resource.
    */
    if (lseek(hModule, (long) res.nResBytes, 1) == -1L)
      break;
  }


bye:
  /*
    The resource was not found. Close the resource file if we opened
    it in this routine, and return NULL to signify failure.
  */
  if (bOpenedEXE)
    CloseResourceFile(hModule);
  return NULL;
}


/****************************************************************************/
/*                                                                          */
/* Function : _MapStirngIDToNumber(lpszID)                                  */
/*                                                                          */
/* Purpose  : MEWEL maintains a list of strings which are used as resource  */
/*            identifiers. For example, in the RC definition :              */
/*                                                                          */
/*               Foo DIALOG 3, 10, 70, 10                                   */
/*               BEGIN                                                      */
/*                ....                                                      */
/*               END                                                        */
/*                                                                          */
/*            the string 'Foo' gets mapped to a numeric ID. This function   */
/*            searches the ID-string table for a string identifier and      */
/*            returns its associated numeric identifier.                    */
/*                                                                          */
/* Returns  : The numeric id associated with the string, if found. 0 if not.*/
/*                                                                          */
/****************************************************************************/
static INT PASCAL _MapStringIDToNumber(lpszID)
  LPCSTR   lpszID;
{
  DWORD    nSoFar = 0L;
  LPSTR    lpStrings;
  LPRCSTRING lpRCS;

  lpStrings = (LPSTR) pszIDStrings;
  while (nSoFar < _StringTblHeader.ulBytes)
  {
    lpRCS = (LPRCSTRING) lpStrings;
    /*
      Compare an entry in the string table against the string we're looking for
    */
    if (!lstricmp(lpszID, lpRCS->string))
      return lpRCS->idString;
    else
    {
      /*
        No match. Move on to the next string
      */
#if defined(UNIX) || defined(VAXC) || defined(__GNUC__)
      /*
        Be careful of the padding of the RCSTRING structure
      */
      int nChars = lpRCS->nChars;
      int nBytes = 2*sizeof(WORD) + nChars;
#if defined(WORD_ALIGNED)
      if (nChars & MISALIGN)
       nChars += (nChars & ~MISALIGN) + ALIGN;
#endif

#else
      int nBytes = (sizeof(RCSTRING) - 1) + lpRCS->nChars;
#endif
      lpStrings += nBytes;  /* Move to the next entry */
      nSoFar    += nBytes;  /* Decrease the number of bytes remaining */
    }
  }

  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function : LoadMsgBox()                                                  */
/*                                                                          */
/* Purpose  : Greylock Software's extension to load in a message box from   */
/*            the resource file and execute it.                             */
/*                                                                          */
/* Returns  : The result from MessageBox() if successful, -1 if not.        */
/*                                                                          */
/****************************************************************************/
#ifdef GREYLOCK
typedef struct msgbox
{
  UINT wStyle;
  UINT wFmtStringTblId;
  UINT wCaptionStringTblId;
  UINT nCharsFmt;
  UINT nCharsCaption;
} MSGBOX, FAR *LPMSGBOX;


#ifdef WHEN_I_GET_AROUND_TO_IT
/*
This fellow will use the "body string" of the message box as the format string
to sprintf or something like it (in my case, something like it, since I don't
have the MSC code.)
*/
UINT LoadMsgBoxPrintF(hModule, idMsgBox, hParent, ... )
  HMODULE hModule;
  LPSTR idMsgBox;
  HWND hParent;
#endif

UINT FAR PASCAL LoadMsgBox(hModule, idMsgBox, hParent)
  HMODULE hModule;
  LPSTR idMsgBox;
  HWND hParent;
{
  LPSTR  pData = GetResource(hModule, idMsgBox, RT_MSGBOX);
  LPSTR  pOrigData;
  HWND   hDialog;
  UINT   nItems;
  UINT   nChars;
  char   buf[MAXBUFSIZE], szUserClass[65];
  LPSTR  pFmt;
  DWORD  dwStyle;
  LPMSGBOX msgbox;
  UINT   Rslt;
                                       /* Hey, we have no resource!  Barf! */
                                       /* Probably not all that well!      */
  if ( NULL == ( pOrigData = pData ) )
    return -1;

  msgbox = (LPMSGBOX)pData;
  pFmt = ( pData += sizeof(MSGBOX) );  /* Set up the format pointer        */
  pData += msgbox->nCharsFmt;          /* Set the data pointer to head of  */
                                       /* caption, and if we have a caption,*/
                                       /* copy it into buf.                */
  if (( nChars = msgbox->nCharsCaption  ) )
    lmemcpy( buf, pData, nChars );
  *pData = '\0';                       /* Then zap the head of the caption */
                                       /* (making the format string a "proper" */
                                       /* szString) */
  buf[nChars] = '\0';                  /* and do the same for the copy of the */
                                       /* caption in buf.  Then go to it */
  Rslt = MessageBox( hParent, (LPSTR)pFmt, (LPSTR)buf, msgbox->wStyle );
  MyFree_far( pOrigData );             /* Free the resource */
  return Rslt;                         /* And Boogie */
}
#endif

