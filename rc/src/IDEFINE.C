/****************************************************************************\
 * IDEFINE.C - processes #define and #include for the macro language        *
 *                                                                          *
 * (C) Copyright 1986   Marc Adler    All Rights Reserved                   *
 *                                                                          *
 *                                                                          *
\****************************************************************************/

#include "int.h"
#include "ytab.h"

extern char *DosSearchPath(char *szEnvVar, char *szFile, char *szRetName);
extern char yylinebuf[];
extern char *yytext;
extern int hash(char *);


DEFSYM *DefSymtab = NULL;               /* Symbol table for the defines */
int    bNewFile = 1;
char   szCustomIncludePath[256] = { '\0' };

#if defined(UNIX) || defined(VAXC)
char *strlwr(s)
  char *s;
{
  char *origS = s;

  while (*s)
    *s++ = tolower(*s);

  return origS;
}
#endif


/* check_define - checks if a token has been #defined */
DEFSYM *check_define(word)
  char *word;
{
  register DEFSYM *d;
  register int    iHash = hash(word);

  for (d = DefSymtab;  d;  d = d->next)
    if (d->iDefHash == iHash && !strcmp(word, d->definition))
      return d;
  return NULL;
}

/* define_install - adds a macro def to the define symbol table */
void define_install(definition, expansion)
  char *definition,
       *expansion;
{
  register DEFSYM *d;
  
  d = (DEFSYM *) emalloc(sizeof(DEFSYM));
  d->definition = strsave(definition);
  d->iDefHash   = hash(definition);
  d->expansion  = strsave(expansion);
  d->next = DefSymtab;
  DefSymtab = d;

  if (yydebug)
    printf("#defined %s\n", definition);
}


typedef struct fileinfo         /* nested file info for the compiler */
{
  struct fileinfo *prevfile;
  char   *filename;
  FILE   *fp;
  int    linectr;
  char   *pszOldYYlinebuf;
  char   hash_lines_only;
} FILEINFO;

FILEINFO *CurrFileInfo = NULL;

include_file(filename)
  char *filename;
{
  FILE *f;
#if !defined(__TURBOC__) && !defined(__ZTC__)
  char *strchr();
#endif

#if (defined(UNIX) || defined(VAXC)) /* long path names */
  char buf[BUFSIZ];
#else
  char buf[80];
#endif
  
  if (strchr("<\"", *filename))
  {
    char *s;
    filename++;
    if ((s = strchr(filename, '>')) != NULL ||
	(s = strchr(filename, '"')) != NULL)
      *s = '\0';
  }

  strlwr(filename);

#ifdef WZV /* Support Windows-compatible -I option */
  if ((!DosSearchPath("INCLUDE", filename, buf)  &&
	!search_include(filename, buf))          ||
#else
  if ((!DosSearchPath(NULL, filename, buf)       &&
       !DosSearchPath("INCLUDE", filename, buf)) ||
#endif
       (f = fopen(buf, "r")) == NULL)
  {
    printf("Error: Can't open the include file %s\n", filename);
    return(NO);
  }

  push_file(buf, f);
  bNewFile = 1;
  return(YES);
}

void push_file(filename, fp)
  char *filename;
  FILE *fp;
{
  FILEINFO *fi;
  
  fi = (FILEINFO *) emalloc(sizeof(FILEINFO));
  fi->filename = strsave(filename);
  fi->fp = fp;
  fi->prevfile = CurrFileInfo;
  if (CurrFileInfo)                   /* If not at startup,       */
    CurrFileInfo->linectr = linectr;  /* save prev file's linectr */
  fi->linectr = linectr = 1;
  fi->pszOldYYlinebuf = strsave(yylinebuf);

  fi->hash_lines_only = hash_lines_only =
           (strcmp(".h", filename + strlen(filename) - 2) == 0) ||
           (strcmp(".H", filename + strlen(filename) - 2) == 0);

  CurrFileInfo = fi;
  ifd = fp;
}

pop_file()
{
  FILEINFO *oldfi;

 /*
  * If the rc file ends in an include statement the rc file info is popped
  * more than once. It would be better to rewrite the scanner, but the
  * following workaround will do for now.
  */
  if (CurrFileInfo == 0)
    return(NO);


  oldfi = CurrFileInfo;
  CurrFileInfo = CurrFileInfo->prevfile;
  fclose(oldfi->fp);

  strcpy(yylinebuf, oldfi->pszOldYYlinebuf);
  yytext = yylinebuf;

  free((char *) oldfi->pszOldYYlinebuf);
  free((char *) oldfi->filename);
  free((char *) oldfi);

  if (CurrFileInfo != NULL)
  {
    ifd = CurrFileInfo->fp;
    linectr = CurrFileInfo->linectr;
    hash_lines_only = CurrFileInfo->hash_lines_only;
    return(YES);
  }
  else
    return(NO);
}


char *DosSearchPath(char *szEnvVar, char *szFile, char *szRetName)
{
  char szPath[256], *pDir, *pSemi, *pEnd;
  char szName[80];

  /*
    First, search the current directory for the file
  */
  if (access(szFile, 0) == 0)
    return strcpy(szRetName, szFile);

  /*
    Get the path specified in the environment var
  */
  if (szEnvVar == NULL)
  {
    pDir = szCustomIncludePath;  /* use the custom path */
    if (pDir[0] == '\0')
      return NULL;
  }
  else
  {
#ifdef MSDOS /* WZV - avoid access violations */
    strupr(szEnvVar);
#endif
    if ((pDir = getenv(szEnvVar)) == NULL)
      return NULL;
  }

  strcpy(szPath, pDir);
  pDir = szPath;

  for (;;)
  {
    /*
      Is there a semi-colon. If so, cut it off before copying.
    */
    if ((pSemi = strchr(pDir, ';')) != NULL)
      *pSemi = '\0';
    /*
      Copy the path, a backslash, and the filename
    */
    strcpy(szName, pDir);
    pEnd = szName + strlen(szName);
    if (pEnd[-1] != CH_SLASH)  /* append the final backslash */
      *pEnd++ = CH_SLASH;
    strcpy(pEnd, szFile);

    /*
      Check for existence
    */
    if (access(szName, 0) == 0)
      return strcpy(szRetName, szName);

    /*
      Not there. Try to move on to the next path.
    */
    if (pSemi == NULL)
      return NULL;
    else
      pDir = pSemi + 1;   /* go past the semicolon */
  }
}


/*
  AddIncludePath()

  This is called when the -I argument is encountered in the RC command
  line. You can have a mixture of the following two kinds of arguments :

  -I<path1>;<path2>;<path3>

  -I<path1> -I<path2> -I<path3>

*/
void AddIncludePath(pszNewPath)
  char *pszNewPath;
{
  /*
    Make sure that the existing include path ends with a separator
  */
  int iLen = strlen(szCustomIncludePath);
  if (iLen > 0 && szCustomIncludePath[iLen-1] != ';')
    strcat(szCustomIncludePath, ";");

  /*
    Concat the new include path onto the end of the existing
    include path
  */
  strcat(szCustomIncludePath, pszNewPath);
}


/*****************************************************************************/
/*                                                                           */
/*            IFDEF PROCESSING                                               */
/*                                                                           */
/*****************************************************************************/

char    IfStack[MAXIFNEST];     /* #if information              */
char    *ifptr = IfStack;       /* -> current ifstack item      */

ProcessConditional(token)
  int token;
{
  /*
    If we are parsing a section of code which has been ifdef'ed out,
    then we must keep track of the nesting levels. If we see another
    #if..., then we must bump the nesting level by one.
  */

  if (!compiling)
  {			/* Not compiling now    */
    switch (token)
    {
      case IF     :	/* These can't turn     */
      case IFDEF  :	/* compilation on, but */
      case IFNDEF :	/* we must nest #if's */
        if (++ifptr >= &IfStack[MAXIFNEST])
          goto if_nest_err;
        *ifptr = 0;		/* !WAS_COMPILING       */
dump_line:
        eat_line();       /* Ignore rest of line  */
	return 0;
    }
  }


  switch (token)
  {
    case ELSE  :
      if (ifptr == &IfStack[0])   /* no #if with this else? */
        goto nest_err;
      if (*ifptr & ELSE_SEEN)     /* two #else's in a row? */
        goto else_seen_err;
      *ifptr |= ELSE_SEEN;
      if (*ifptr & WAS_COMPILING)
      {
        if (compiling || (*ifptr & TRUE_SEEN))
          compiling = FALSE;
	else
          compiling = TRUE;
      }
      break;

    case ELIF   :
      if (ifptr == &IfStack[0])       /* no #if with this else? */
	goto nest_err;
      if ((*ifptr & ELSE_SEEN) != 0)  /* #elif followed an #else? */
      {
else_seen_err:
	yyerror("#else or #elif may not follow #else", NULL);
	goto dump_line;
      }
      if ((*ifptr & (WAS_COMPILING | TRUE_SEEN)) != WAS_COMPILING)
      {
	compiling = FALSE;		/* Done compiling stuff */
	eat_line();    		        /* Skip this clause     */
	return 0;
      }
      EvalIfdef(IF);
      break;

    case IF     :
    case IFDEF  :
    case IFNDEF :
      if (++ifptr >= &IfStack[MAXIFNEST])
if_nest_err:
        yyerror("Too many nested #ifxxx statements", NULL);
      *ifptr = WAS_COMPILING;
      EvalIfdef(token);
      break;

    case ENDIF  :
      if (ifptr == &IfStack[0])
      {
nest_err:
        yyerror("#endif must be in an #if", NULL);
        goto dump_line;
      }
#if 0
      if (!compiling && (*ifptr & WAS_COMPILING) != 0)
	wrongline = TRUE;
#endif
      compiling = ((*ifptr & WAS_COMPILING) != 0);
      --ifptr;
      break;
  }
  return 0;
}



/*
 * Process an #if, #ifdef, or #ifndef.  The latter two are straightforward,
 * while #if needs a subroutine of its own to evaluate the expression.
 *
 * EvalIfdef() is called only if compiling is TRUE.  If false, compilation
 * is always supressed, so we don't need to evaluate anything.  This
 * supresses unnecessary warnings.
*/
EvalIfdef(token)
  int token;
{
  register int c;
  register int found;
  char buf[256];

  /*
    Skip over the whitespace between the #ifxxx and the first token
  */
  if ((c = eat_whitespace()) == '\n' || c == EOF)
  {
    nungetc(c);
    goto badif;
  }

  if (token == IF)
  {
    nungetc(c);
    get_token(buf);  /* get all characters up until the next space */

    if (buf[0] == '!' || !strncmp(buf, "defined", 7))
    {
      yyerror(
"#if defined(xxx) is not yet supported by the MEWEL RC.\nChange it to #ifdef or #ifndef.",
                NULL);
      return 1;
    }

    if (isdigit(buf[0]))
      found = (atoi(buf) != 0);
    else
      found = (check_define(buf) != NULL);
    token = IFDEF;		/* #if is now like #ifdef       */
  }
  else
  {
    if (!iscsym(c))		/* Next non-blank isn't letter  */
      goto badif;		/* ... is an error              */
    nungetc(c);
    get_alpha_token(buf);       /* get the macro token */
    found = (check_define(buf) != NULL);
  }

  if (found == (token == IFDEF))
  {
    compiling = TRUE;
    *ifptr |= TRUE_SEEN;
  }
  else
  {
    compiling = FALSE;
  }
  return 1;

badif:
  yyerror("#if, #ifdef, or #ifndef without an argument", NULL);
  return 1;
}


void UnDef(szSym)
  char *szSym;
{
  DEFSYM *d;

  if ((d = check_define(szSym)) != NULL)
  {
    d->definition[0] = '\0';
    if (yydebug)
      printf("#undef'ed %s\n", szSym);
  }
}

