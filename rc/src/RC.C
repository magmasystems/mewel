/*===========================================================================*/
/*                                                                           */
/* File    : RC.C                                                            */
/*                                                                           */
/* Purpose : Main file for the resource compiler.                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989 Marc Adler/Magma Systems     All Rights Reserved       */
/*===========================================================================*/
#include "int.h"
#include "rccomp.h"
#include "ytab.h"

#if defined(UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(MSC) || defined(__TURBOC__)
#include <sys\types.h>
#include <sys\stat.h>
#endif

#ifdef UNIX
#define is_option(s) (*s == '-')
#define CH_SLASH '/'
#else
#define is_option(s) ((*s == '-') || (*s == '/'))
#define CH_SLASH '\\'
#endif


PROG *Prog;
int  Prog_sp = 0;
int  ProgMaxSpace;

FILE *ifd;
extern int  resFD;

extern PROGRAM *Programs;
extern int     LastProg;

extern int yynerrs;
extern int yydebug;
int    dumpsyms;
int    dumptext = 0;



main(argc, argv)
  int  argc;
  char **argv;
{
  int  i = 1;
  char fname[256];
  char *szExt;
  char *pExp, *pDef;
#ifdef RC_TRANS
  int  bDumpResources = 1;
#else
  int  bDumpResources = 0;
#endif

  char *lpNewResName = NULL;
  char *lpNewExeName = NULL;


  printf("MEWEL Resource Compiler v 4.0. (C) Copyright 1989-1993 Magma Systems\n");

  /* interpret any arguments on the command line */
  while ((i < argc) && is_option(argv[i]))
  {
    switch (argv[i][1])
    {
      case '3' :
        if (argv[i][2] == '\0')    /* just /3, not /30 /31 */
          bScreenRelativeCoords++;
        break;

      case 'c' :
        switch (argv[i][2])
        {
          case 'x' :
            cxTranslated = atoi(argv[i]+3);
            break;
          case 'y' :
            cyTranslated = atoi(argv[i]+3);
            break;
        }
        break;
      case 'd':  
        if (argv[i][2] == '\0')
        {
          yydebug++;
          break;
        }
        /* fall through ... so that -d<def> is the same as -D<def> */
      case 'D':
        pDef = &argv[i][2];
        if ((pExp = strchr(pDef, '=')) != NULL)
          pExp++;
        else
          pExp = "1";
        define_install(pDef, pExp);
        break;
      case 'e':
        bEchoTranslation = 1;
        break;

      case 'f' :    /* -fe = rename EXE file */
      case 'F' :    /* -fo = rename RES file */
        switch (argv[i][2])
        {
          case 'e' :
          case 'o' :
            pDef = &argv[i][3];
            while (isspace(*pDef))
              pDef++;
            if (argv[i][2] == 'o')
              lpNewResName = pDef;
            else
              lpNewExeName = pDef;
            break;
        }
        break;

      case 'i' :   /* -i = include directories */
      case 'I' :   /* -I = include directories */
#ifdef WZV /* Windows-compatible -I option */
        search_install(&argv[i][2]);
#else
        AddIncludePath(&argv[i][2]);
#endif
        break;
      case 'k' :  /* Windows compat */
        break;
      case 'n' :  /* nologo */
      case 'N' :
        break;
      case 'p' :
        if ((nPushButtonHeight = atoi(&argv[i][2])) == 0)
          nPushButtonHeight = 1;
        break;
      case 'r' :   /* rc -r does nothing */
        break;
      case 's':
        dumpsyms++;
        break;
      case 't':
        dumptext++;
        break;
      case 'w' :
        /*
          Windows compatibility flag
        */
        switch (argv[i][2])
        {
          case 'b' :
            bNoBorders = 1;
            break;
          case 'c' :
            bNoClipping = 1;
            break;
          case 'd' :
            bWindowsCompatDlg = 1;
            break;
          case 'h' :
            bNoHeuristics = 1;
            break;
          case 'r' :        /* -wr rounds up */
            iRounding = 0;
            break;
          case 't' :
            bUseCTMASK = 1;
            break;
          case '\0' :
            xTranslated = cxTranslated = 4;
            yTranslated = cyTranslated = 8;
            break;
        }
        break;
      case 'x':
        xTranslated = atoi(argv[i]+2);
        break;
      case 'y':
        yTranslated = atoi(argv[i]+2);
        break;
      case 'z' :
        bDumpResources++;
        break;
      default :
        usage();
        exit(1);
        break;
    }
    i++;
  }

  if (i >= argc)
  {
    usage();
    exit(0);
  }
  else
    strcpy(fname, argv[i]);

  /*
    If there is no extension in the filename, append ".rc".
    If there is a ".res" extension, then we want to process the
    binary res file and append it onto the end of the EXE file.
  */
  if ((szExt = strrchr(fname, '.')) == NULL || szExt[1] == CH_SLASH)
    strcat(fname, ".rc");
  else if (stricmp(szExt+1, "RES") == 0)
  {
    AppendResToExe(lpNewExeName ? lpNewExeName : fname);
    exit(0);
  }

  if (access(fname, 0) != 0)
  {
    drop_extension(fname);
    strcat(fname, ".rc");
    if (access(fname, 0) != 0)
    {
      execerror("Error. Can't open the RC file.", NULL);
    }
  }

  if (!include_file(fname))
    exit(1);

  /*
    Form the name of the RES file. It is either the root name of the RC
    file or the name specified by the -fo parameter.
  */
  if (lpNewResName)
  {
    strcpy(fname, lpNewResName);
  }
  else
  {
    drop_extension(fname);
    strcat(fname, ".res");
  }

  /*
    Open the new RES file for writing
  */
#if defined(UNIX) || defined(VAXC)
  if ((resFD=open(fname,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,0666)) < 0)
#elif defined(MSC) || defined(__TURBOC__)
  if ((resFD=open(fname,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE)) < 0)
#else
  if ((resFD=open(fname,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,0)) < 0)
#endif
    execerror("Can't open output file", NULL);

  init_getbuf();
  init_prog();

  if (!yyparse() && !yynerrs)     /* compiled without fail */
  {
    write_prog(resFD);
    printf("The compiled resource is in file %s\n", fname);
#ifdef RC_TRANS
    if (bDumpResources)
      DumpResources(fname);
#endif
    exit(0);
  }
  else
  {
    close(resFD);
    unlink(fname);
  }

  exit(1);
  return 0;
}

/* execerror - prints a fatal message, then dies */
execerror(msg, s)
  char *msg, *s;
{
  (void) s;
  fprintf(stderr, "%s\n", msg);
  exit(1);
  return 0;
}

/* write_prog - writes out the executable intermediate code to an .exm file */
write_prog(resFD)
  int resFD;
{
  ProcessStringTables();
  WriteIDTable();
  close(resFD);
  return 0;
}

char *drop_extension(fname)
  char *fname;
{
  register char *s;
#if !defined(__TURBOC__) && !defined(__ZTC__)
  char *strchr();
#endif

  /*
    Search backwards for the dot. Make sure we don't have a filename like
    ..\dir\fname
  */
  if ((s = strrchr(fname, '.')) != NULL && s[1] != CH_SLASH)
    *s = '\0';

  return(fname);
}


typedef struct tagCVInfo
{
  char szSignature[4];
  unsigned long ulOffsetFromEOF;
} CVINFO;


AppendResToExe(resName)
  char *resName;
{
  char *exeName;
  int  exeFD;
  int  n;
  unsigned char *buf;
  unsigned long total = 0;
  int  fCodeview = 0;
  int  bufSize   = 0x7FFF;
  long ulEndOfEXEPos;
  long ulResSig;
  CVINFO cvinfo;

  while ((buf = malloc(bufSize)) == NULL && bufSize > 512)
    bufSize >>= 1;
  if (!buf)
    execerror("Can't allocate scratch buffer", NULL);

  /*
    First try to open the RES file for reading
  */
#ifdef VAXC
  if ((resFD = open(resName, O_BINARY | O_RDONLY, 0)) < 0)
#else
  if ((resFD = open(resName, O_BINARY | O_RDONLY)) < 0)
#endif
    execerror("Can't open the .RES file", NULL);
  
  /*
    Try to open the EXE file for writing
  */
  exeName = strcat(drop_extension(resName), ".exe");
#ifdef VAXC
  if ((exeFD = open(exeName, O_BINARY | O_RDWR | O_APPEND, 0)) < 0)
#else
  if ((exeFD = open(exeName, O_BINARY | O_RDWR | O_APPEND)) < 0)
#endif
    execerror("Can't open the .EXE file", NULL);
  fprintf(stderr, "Writing to %s\n", exeName);

  ulEndOfEXEPos = lseek(exeFD, 0L, 2);

  /*
    See if the EXE file contains CodeView information at the end of it.
  */
  lseek(exeFD, -8L, 1);
  n = read(exeFD, (char *) &cvinfo, sizeof(cvinfo));
  if (n < 0)
    execerror("Can't read file's tail", NULL);
  fCodeview = (cvinfo.szSignature[0] == 'N' && cvinfo.szSignature[1] == 'B');
  /*
    Now, this is funny.... some of my Borland-compiled programs actually
    end up with the string "WINBLIT.C" at the end of the file, especially
    if WINBLIT.OBJ was added to the Borland MEWEL library as the last file.
    So, check for this...
  */
  if (cvinfo.szSignature[2] == 'L' && cvinfo.szSignature[3] == 'I')
    fCodeview = 0;

  if (fCodeview)
  {
    unsigned long ulCVStartPos;
    int exe2FD;
    char tmpFile[80];

    strcpy(tmpFile, "~RCTMP!!.EXE");

#ifdef MSC
    if ((exe2FD = open(tmpFile, O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE)) < 0)
#else
    if ((exe2FD = open(tmpFile, O_BINARY|O_CREAT|O_TRUNC|O_WRONLY, 0)) < 0)
#endif
      execerror("Can't open the TEMP file", NULL);

    fprintf(stderr, "The file has CodeView info in it!\n");

    ulCVStartPos = lseek(exeFD, -(cvinfo.ulOffsetFromEOF), 2);
    lseek(exeFD, 0L, 0);

    /*
      1) Copy the old EXE data to the new EXE, stopping before we
         reach the codeview info.
    */
    for (total = 0L;  total < ulCVStartPos;  total += n)
    {
      n = read(exeFD, buf, bufSize);
      if (nwrite(exe2FD, buf, n) != n)
        execerror("Error in writing file", NULL);
      fprintf(stderr, ".");
    }

    /*
      2) Rewind both EXE's to the Codeview start point.
    */
    lseek(exeFD,  ulCVStartPos, 0);
    lseek(exe2FD, ulCVStartPos, 0);

    /*
      3) Copy the resource file data into the new exe
    */
    while ((n = read(resFD, buf, bufSize)) > 0)
    {
      if (nwrite(exe2FD, buf, n) != n)
        execerror("Error in writing file", NULL);
      total += n;
      fprintf(stderr, ".");
    }

    /*
      4) Now copy the codeview info from the original exe into the new exe
    */
    fprintf(stderr, "\nCopying the Codeview info\n");
    while ((n = read(exeFD, buf, bufSize)) > 0)
    {
      if (nwrite(exe2FD, buf, n) != n)
        execerror("Error in writing file", NULL);
      total += n;
      fprintf(stderr, ".");
    }

    /*
      Close the two exe files. Get rid of the old exe and rename the
      temp file to be the same as the old exe.
    */
    close(exeFD);
    close(exe2FD);
    free(buf);

    unlink(exeName);
    rename(tmpFile, exeName);
  }

  else   /* no codeview info */
  {
    /*
      Transfer the data from the RES file to the end of the EXE file
    */
    while ((n = read(resFD, buf, bufSize)) > 0)
    {
      if (nwrite(exeFD, buf, n) != n)
        execerror("Error in writing file", NULL);
      total += n;
      fprintf(stderr, ".");
    }

    /*
      Write a little trailer. The signature, then the RES seek pos.
    */
    ulResSig = RC_SIGNATURE;
    nwrite(exeFD, (char *) &ulResSig, sizeof(ulResSig));
    nwrite(exeFD, (char *) &ulEndOfEXEPos, sizeof(ulEndOfEXEPos));


    close(exeFD);
  }

  fprintf(stderr, "\n(Wrote %ld bytes)\n", total);
  close(resFD);
  return 0;
}


usage()
{
  static char *szUsage[] =
  {
    "rc [options] filename",
    "  Options are :",
    "  -Dname[=value]   Defines name to be equal to value (or 1)",
    "  -e               Echo translation of Windows to char coordinates",
    "  -fe <filename>   Rename the EXE file to <filename>",
    "  -fo <filename>   Rename the RES file to <filename>",
    "  -I[path]         Use the path to search for include files",
    "  -p<num>          Used with -w to specify a push button height",
    "  -r               No-op. Included for Windows compatibility",
    "  -w               Translates Windows dialog coords to MEWEL's text mode",
    "  -wb              Disable the automatic placing of borders on pushbuttons",
    "  -wd              Outputs Windows compatible dialog templates",
    "  -wh              Do not use any heuristics when converting",  
    "  -wr              Rounds coordinates up (default is rounding down)",
    "  -wt              Output classname as a CT_xxx value",
    "  -x<num>            x axis scaling factor for dialog boxes",
    "  -y<num>            y axis scaling factor for dialog boxes",
    "  -cx<num>           width scaling factor for dialog boxes",
    "  -cy<num>           height scaling factor for dialog boxes",
    " The filename can be an RC file if you want to create a binary RES file.",
    " If the extension is RES, then the RES file will be appended onto the",
    " EXE file of the same name.",
    NULL
  };

  char **s;

  for (s = szUsage;  *s != NULL;  s++)
    printf("%s\n", *s);
  return 0;
}


int nwrite(fd, pBuf, nBytes)
  int  fd;
  void *pBuf;
#if defined(UNIX)
  int nBytes;
#else
  unsigned int nBytes;
#endif
{

#if defined(UNIX)
  int n;
  if (nBytes < 0)
  {
    execerror("nwrite with n < 0\n",  NULL);
  }

#else
  unsigned int n;
#endif

  if ((n = write(fd, pBuf, nBytes)) != nBytes)
  {
    execerror("Error writing output file (Disk full?). No RES file generated.",
               NULL);
  }

  return n;
}


#if defined(UNIX) || defined(VAXC)

char *strupr(s)
  char *s;
{
  char *orig_s = s;

  while (*s)
  {
    *s = toupper(*s);
    s++;
  }
  return orig_s;
}

int stricmp(s, t)
  char *s, *t;
{
  while (*s && *t && tolower(*s) == tolower(*t))
    s++, t++;
  return (*s - *t);
}

#endif


#define	ALIGN		sizeof(WORD)
#define	MISALIGN	(ALIGN - 1)

int word_pad(len)
  int len;
{
#ifdef WORD_ALIGNED /* Keep resource data word-aligned */
  if (len & MISALIGN)
    len = (len & ~MISALIGN) + ALIGN;
#endif
  return len;
}

