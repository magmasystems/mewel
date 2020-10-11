/****************************************************************************\
 * INTLEX.C - lexical analyzer for the macro language                       *
 *                                                                          *
 * (C) Copyright 1986   Marc Adler    All Rights Reserved                   *
 *                                                                          *
 *                                                                          *
\****************************************************************************/

#include "int.h"
#include "rccomp.h"
#include "ytab.h"
#define YYLVAL   yylval

int linectr = 1;
int hash_lines_only = 0;

KEYWORD Reserved[] =
{
  "accelerators", ACCELERATORS,
  "alt",          ALT,            /* ACCEL */
  "ascii",        ASCII,          /* ACCEL */
  "begin",        BEGIN,
  "bitmap",       BITMAP,
  "block",        BLOCK,
  "box",          BOX,
  "caption",      CAPTION,
  "checkbox",     CHECKBOX,
  "checked",      CHECKED,
  "class",        CLASS,
  "combobox",     COMBOBOX,
  "control",      CONTROL,
  "ctext",        CTEXT,
  "cursor",       CURSOR,
  "defpushbutton",DEFPUSHBUTTON,
  "dialog",       DIALOG,
  "discardable",  DISCARDABLE,
  "edit",         EDIT,
  "edittext",     EDIT,
  "end",          END,
  "fileflags",    FILEFLAGS,
  "fileflagsmask", FILEFLAGSMASK,
  "fileos",       FILEOS,
  "filesubtype",  FILESUBTYPE,
  "filetype",     FILETYPE,
  "fileversion",  FILEVERSION,
  "fixed",        FIXED,
  "font",         FONT,
  "frame",        FRAME,
  "grayed",       GRAYED,
  "groupbox",     GROUPBOX,
  "help",         HELP,
  "icon",         ICON,
  "impure",       IMPURE,
  "inactive",     INACTIVE,
  "listbox",      LISTBOX,
  "loadoncall",   LOADONCALL,
  "ltext",        TEXT,
  "menu",         MENU,
  "menubarbreak", MENUBARBREAK,
  "menubreak",    MENUBREAK,
  "menuitem",     MENUITEM,
  "moveable",     MOVEABLE,
  "msgbox",       MSGBOX,
  "noinvert",     NOINVERT,       /* ACCEL */
  "not",          NOT,
  "ok",           OK,
  "okcancel",     OKCANCEL,
  "popup",        POPUP,
  "preload",      PRELOAD,
  "productversion", PRODUCTVERSION,
  "pure",         PURE,
  "pushbutton",   PUSHBUTTON,
  "radiobutton",  RADIOBUTTON,
  "rcdata",       RCDATA,
  "rcinclude",    RCINCLUDE,
  "rtext",        RTEXT,
  "scrollbar",    SCROLLBAR,
  "separator",    SEPARATOR,
  "shadow",       SHADOW,
  "shift",        SHIFT,          /* ACCEL */
  "static",       STATIC,
  "stringtable",  STRINGTABLE,
  "style",        STYLE,
  "text",         TEXT,
  "value",        VALUE,
  "versioninfo",  VERSIONINFO,
  "virtkey",      VIRTKEY,        /* ACCEL */
  "yesno",        YESNO,
  "yesnocancel",  YESNOCANCEL,
};



#define RESSIZE         (sizeof(Reserved) / sizeof(KEYWORD))

char PreprocChar = '#';
char *BadEOFMsg  = "Fatal Error - Unexpected EOF encountered";
long CurrLong;


/* yylex - lexical analyzer used by yyparse(). */
yylex()
{
  int  c;
  int  num;
  long lnum;
  int  type;
  void *pVal;
  char buf[1024];
  char *p;
  char expansion[MAXCHARS];
  int  sym;

  extern WORD bNextSymbolIsLiteral;
  extern WORD bExpectingRESID;
  extern WORD bReadingRawDataList;



start:
  /* span whitespace && non ascii characters */
  if ((c = eat_whitespace()) == EOF)
    return (0);

  if (c == PreprocChar)
  {
    get_alpha_token(buf);
    if (!strcmp(buf, "define") && compiling)
    {
      /*
        Be careful of "#define X". See if we hit a newline
      */
      c = get_alpha_token(buf);     /* get the macro token */
      if (c != '\n' && c != EOF)
        while (isspace(c) && c != EOF && c != '\n')
          c = ngetc();
      if (c == EOF || c == '\n')
      {
        expansion[0] = '\0';
      }
      else
      {
        int lastc = c;
        nungetc(c);
        /*
          Allow escaped '\n' in the definition
        */
        for (p = expansion;
             (c = ngetc()) != '\n' || (c == '\n' && lastc == '\\');
             lastc = c)
        {
          if (c == '\n' && lastc == '\\')
          {
            p[-1] = '\n';
            continue;
          }
          if (c == '/')    /* see if we have a // */
          {
            int c2 = ngetc();
            if (c2 == '/')
            {
              while ((c = ngetc()) != '\n')
                ;
              break;
            }
            else
              nungetc(c2);
          }
          *p++ = c;
        }
        *p = '\0';
      }
      define_install(buf, expansion);    /* install it */
    } /* endif #define */

    else if (!strcmp(buf, "undef") && compiling)
    {
      /*
        Be careful of "#define X". See if we hit a newline
      */
      c = get_alpha_token(buf);     /* get the macro token */
      if (c != '\n' && c != EOF)
        while (isspace(c) && c != EOF && c != '\n')
          c = ngetc();
      nungetc(c);
      UnDef(buf);                  /* uninstall it */
    }
    else if (!strcmp(buf, "pragma") && compiling)
    {
      c = get_alpha_token(buf);     /* get the macro token */
      if (c != '\n')
        eat_line();
      switch (buf[0])
      {
        case 'c' :
          if (buf[1] == 'x')
            cxTranslated = atoi(buf+2);
          else if (buf[1] == 'y')
            cyTranslated = atoi(buf+2);
          break;
        case 'x' :
          xTranslated = atoi(buf+1);
          break;
        case 'y' :
          yTranslated = atoi(buf+1);
          break;
        case 'p' :
          if ((nPushButtonHeight = atoi(buf+1)) == 0)
            nPushButtonHeight = 1;
          break;
        case 'w' :
          if (buf[1] == '0')
          {
            xTranslated = cxTranslated = 1;
            yTranslated = cyTranslated = 1;
          }
          else
          {
            xTranslated = cxTranslated = 4;
            yTranslated = cyTranslated = 8;
          }
          break;
        default :
          break;
      }
    }
    else if (!strcmp(buf, "include") && compiling)
    {
      /*
        Eat up all of the whitespace after the included file, or else
        we might hang here.
      */
      c = get_token(buf);          /* get the macro token */
      if (c != '\n')
        eat_line();
      include_file(buf);
    }
    else if (!strcmp(buf, "echo") && compiling)
    {
      for (p = buf;  (c = ngetc()) != '\n';  *p++ = c) ;
      *p = '\0';
      nungetc(c);
      fprintf(stderr, "%s\n", buf);
    }
    else if (!strcmp(buf, "exit") && compiling)
    {
      printf("Found #exit at line %d\n", linectr);
      exit(1);
    }
    else
    {
      c = 0;
      if (!strcmp(buf, "ifdef"))        c = IFDEF;
      else if (!strcmp(buf, "if"))      c = IF;
      else if (!strcmp(buf, "ifndef"))  c = IFNDEF;
      else if (!strcmp(buf, "else"))    c = ELSE;
      else if (!strcmp(buf, "elif"))    c = ELIF;
      else if (!strcmp(buf, "endif"))   c = ENDIF;
      if (c != 0)
        ProcessConditional(c);
    }
    goto start;
  }

  if (!compiling || hash_lines_only)
  {
    eat_line();
    goto start;
  }

  /* check for a numerical constant */
  if (isdigit(c) || c == '-')
  {
    int ishex = NO;
    int isNeg = NO;

    if (c == '-')
      isNeg = YES;
    if (c == '0')
      if ((c = ngetc()) == 'x' || c == 'X')
      {
        ishex = YES;
        goto getnum;
      }

    if (c != '-')
      nungetc(c);
getnum:
    for (p = buf;  p < buf + MAXCHARS;  *p++ = c)
    {
      c = ngetc();
      if (ishex && !isxdigit(c) || !ishex && !isdigit(c))
        break;
    }
    *p = '\0';
    if (toupper(c) != 'L')
    {
      nungetc(c);
    }

    if (ishex)
    {
      lnum = xtol(buf);
      num  = (isNeg) ? -lnum : lnum;
      type = NUMBER;
      pVal = &num;
      if (toupper(c) == 'L')
      {
        type = LNUMBER;
        CurrLong = lnum;
      }
    }
    else
    {
      lnum = atol(buf);
      type = NUMBER;
      num  = (isNeg) ? -lnum : lnum;
      pVal = &num;
      if (toupper(c) == 'L')
      {
        type = LNUMBER;
        CurrLong = lnum;
      }
    }
n:
    YYLVAL = num;
    if (bExpectingRESID)
      bExpectingRESID = 0;
    return(type);
  }

  if (c == '\'')
  {
    /* ignore single quotes in raw data list */
    if (bReadingRawDataList)
      return c;

    num = ngetc();    /* get the character     */
    if (num == '\\')  /* escaped character     */
      num = esc(ngetc());
    c = ngetc();      /* get the closing quote */
    type = NUMBER;
    pVal = &num;
    goto n;           /* install it as a number*/
  }

  /*
    Check for a string constant
  */
  if (c == '"')
  {
    for (p = buf;  p < buf+sizeof(buf);  *p++ = c)
    {
      if ((c = ngetc()) == '"')
      {
        /*
          See if we had two double-quotes in a row. If we do, this means
          that we take a double-quote literally.
          IE - "He said ""Hello World"" to me"
        */
        int chNext;

        /*
          Break if we got the closing double-quote. This next bit of nonsense
          checks to see if we are at the end of a line, and therefore, avoids
          all of the unpleasantries associated with peeking at the next char
          when the next char is a newline (...then, we have to bump down the
          line counter, etc).
        */
        if (IsNextCharNewline())
          break;
        if ((chNext = ngetc()) != '"')
        {
          nungetc(chNext);
          break;  /* get outta the loop! */
        }
      }

      if (c == '\n' || c == EOF)
      {
        printf("missing quote in line %d\n", linectr);
        return(NO);
      }
      if (c == '\\')            /* an escaped character? */
      {
        c = ngetc();            /* fetch the escaped char */
        if (isdigit(c))         /* we have '\nnn' - an octal number */
        {
          int sum = c - '0';
          c = ngetc();
          if (isdigit(c))
          {
            sum = sum * 8 + (unsigned) c - (unsigned) '0';
            c = ngetc();
            if (isdigit(c))
              sum = sum * 8 + (unsigned) c - (unsigned) '0';
            else
              nungetc(c);        /* put back the non-digit */
          }
          else
            nungetc(c);          /* put back the non-digit */

          /*
            Temporarily translate '\0' to -1 for the benefit of
            using '\0' in the RCDATA data list.
          */
          if (sum == 0)
            sum = -1;
          c = sum;
        }
        else if (c == 'x' || c == 'X') /* we have \x<hex number> */
        {
          char szHex[8];

          memset(szHex, 0, sizeof(szHex));

          c = ngetc();
          if (isdigit(c))
          {
            szHex[0] = c;
            c = ngetc();
            if (isdigit(c))
            {
              szHex[1] = c;
              c = ngetc();
              if (isdigit(c))
                szHex[2] = c;
              else
                nungetc(c);      /* put back the non-digit */
            }
            else
              nungetc(c);        /* put back the non-digit */
            c = (int) xtol(szHex);
          }
          else
          {
            /* we have just \x */
            nungetc(c);
            c = 'x';
          }
        }
        else               /* may have '\b', '\n', or '\t' */
        {
          c = esc(c);
          if (c == 'a')   /* '\a' for menus must remain as \a */
          {
            c = '\\';
            nungetc('a');
          }
        }
      }
    } /* for */
    *p = '\0';
    if ((sym = litlookup(STRING, buf)) == 0)
      YYLVAL = install(NULL, STRING, strsave(buf));
    else
      YYLVAL = sym;
    return (STRING);
  }

  /* check for a variable name or reserved word */
  if (iscsym(c))
  {
    DEFSYM *defsym;

    nungetc(c);
    /*
      If the bNextSymbolIsLiteral var is TRUE, then we may have a
      pathname of a user-define resource or an icon file. This means
      that we have to read the next token, which might include colons,
      slashes, and periods.
    */
    if (bNextSymbolIsLiteral)
    {
      c = get_token(buf);
    }
    else
      c = get_alpha_token(buf);
    nungetc(c);

    if ((defsym = check_define(buf)) != NULL)
    {
      nungets(defsym->expansion);
      goto start;
    }

    /*
      Resource names should be translated to all lower case, since the
      MS resource compiler seems to be case-insensitive.
    */
    strlower(buf);

    if (!bNextSymbolIsLiteral && !bExpectingRESID)
      if ((c = check_reserved_word(buf, Reserved, RESSIZE)) >= 0)
        return(Reserved[c].val);

    /*
      The variable bExpectingRESID is used so that the resID of a resource
      can be a reserved word (like a dialog named EDIT). There are only
      3 "first" words which cannot be a resID. These are END, STRINGTABLE, and
      RCINCLUDE.
    */
    if (bExpectingRESID)
    {
      bExpectingRESID = 0;
      if (!stricmp(buf, "stringtable"))
        return STRINGTABLE;
      if (!stricmp(buf, "end"))
        return END;
      if (!stricmp(buf, "rcinclude"))
        return RCINCLUDE;
    }

    if ((sym = ylookup(buf)) == 0)        /* assume we have a variable */
    {
      num = 0;
      YYLVAL = install(buf, ID, &num);  /* declare it as an NVAR */
    }
    else
      YYLVAL = sym;          /* already declared - return ptr to symtab */

    return (Symtab[YYLVAL].type);
  }

  switch (c)
  {
    case '/' :
      if ((c = ngetc()) == '*')
      {
        eat_comment();
        goto start;
      }
      if (c == '/')
      {
        eat_line();
        goto start;
      }
      break;

    case ';' :
      /*
        a semi-colon in a line means that the rest of the line is a comment
      */
      eat_line();
      goto start;

    case '{' :
      return BEGIN;
    case '}' :
      return END;
  }

  return (c);
}


get_token(buf)
  char *buf;
{
  register char *s, *limit;
  register int  c;

  nungetc(eat_whitespace());

  limit = buf + MAXCHARS;
  for (s = buf;  s < limit && (c=ngetc()) != EOF && !isspace(c);  *s++ = c) ;
  *s = '\0';

  if (c == EOF)
    yyerror(BadEOFMsg, 0);
  return(c);
}

get_alpha_token(buf)
  char *buf;
{
  register char *s, *limit;
  register int  c;

  nungetc(eat_whitespace());

  limit = buf + MAXCHARS;
  for (s = buf;
       s < limit && (c=ngetc()) != EOF && (iscsym(c) || strchr("\\/.$:", c));
       *s++ = c) ;
  *s = '\0';

  if (c == EOF)
    yyerror(BadEOFMsg, 0);
  return(c);
}

/* eat_whitespace - reads the input file while blanks, tabs & newlines */
/*                  Returns the next non-white character.              */
int eat_whitespace()
{
  register int c;

  /* span whitespace && non ascii characters */
  while (isspace(c = ngetc()) && c != EOF)  ;
  return(c);
}

/* xtol - converts a hex string to an unsigned long */
unsigned long xtol(xstr)
  char *xstr;
{
  unsigned long sum = 0L;
  register c;
  
  while ((c = *xstr++) != '\0' && isxdigit(c))
  {
    c = (isalpha(c)) ? (toupper(c) - 'A' + 10) : (c - '0');
    sum = (sum << 4) + c;
  }
  return sum;
}

/* strlower - makes a string all lower case */
strlower(str)
  char *str;
{
  register char *s;

  for (s = str;  *s;  s++)
    *s = tolower(*s);
}

/* eat_comment - gobbles up a C style comment */
eat_comment()
{
  register int c;

  while ((c = ngetc()) != EOF)
    if (c == '*')
      if ((c = ngetc()) == '/')   /* we got the end of comment */
        break;
      else
        nungetc(c);
}

/* eat_line - gobbles up a Microsoft C style comment */
eat_line()
{
  register int c;
  while ((c = ngetc()) != EOF && c != '\n')
    ;
}

/* esc - returns the actual value of an escaped character */
esc(c)
  register int c;
{
  switch (c) {
    case 'b' : return('\b');
    case 'n' : return('\n');
    case 'r' : return('\r');
    case 't' : return('\t');
    case 'f' : return('\f');
    case 'E' : return(27);
    default  : return(c);
  }
}

/* follow - returns YES if the next character on the input stream is 'yesc' */
follow(c, yesc, noc)
  int c, yesc, noc;
{
  int c2 = ngetc();

  if (c2 == c)
    return(yesc);
  nungetc(c2);      /* put the 'bad' character back on the input stream */
  return(noc);
}


/* check_reserved_word - does a fast binary search on a KEYWORD table */
check_reserved_word(word, tab, tablesize)
  char    *word;
  KEYWORD tab[];
  int     tablesize;
{
  int low, mid, high, cond;

  low = 0;                     /* set the limits */
  high = tablesize - 1;

  while (low <= high)
  {
    mid = (low+high) / 2;
    if ((cond = strcmp(word, tab[mid].name)) < 0)
      high = mid - 1;
    else  if (cond > 0)
      low = mid + 1;
    else
      return(mid);
  }

  return(-1);                 /* FAILURE */
}

/**************************************************************************/

#define GETBUFSIZE 100
char *getbuf;    /* for putting back chars on the input stack */
int  MAXBP;      /* max memory allocated to getbuf */
int  bp = -1;    /* current position in getbuf     */

char yylinebuf[1024];
char *yytext;

/* init_getbuf - alloc's an area to receive push-backed characters */
init_getbuf()
{
  getbuf = emalloc(MAXBP = GETBUFSIZE);
}

/* nungetc - pushes on char onto the input stack */
nungetc(c)
  int  c;
{
  if (++bp >= MAXBP)                    /* out of stack space  */
    getbuf = RE_ALLOC(getbuf, &MAXBP);  /* get some more space */
  getbuf[bp] = c;                       /* and push the char   */
}


/* nungets - pushes a string back onto the input stack */
nungets(s)
  register char *s;
{
  register char *t;

  for (t = s + strlen(s) - 1;  t >= s;  nungetc(*t--))  ;
}

/* ngetc - this is the main routine which reads in characters */
ngetc()
{
  register c;
  static   int firsttime = 1;
  static   int last_c = 0;
  extern   int dumptext;
  extern   int bNewFile;

  if (firsttime || bNewFile)
  {
    if (!fgets(yytext = yylinebuf, sizeof(yylinebuf), ifd))
      goto got_eof;
    firsttime = bNewFile = 0;
    if (dumptext)
    {
      yywhere();
      printf("%s", yylinebuf);
    }
  }

  if (bp >= 0)
    c = getbuf[bp--];
  else if (*yytext == 0)  /* do not advance beyond end of buffer */
    c = EOF;
  else if ((c = *yytext++) == '\n')
  {
    if (!fgets(yytext = yylinebuf, sizeof(yylinebuf), ifd))
      goto got_eof;
    linectr++;
    if (dumptext)
    {
      yywhere();
      printf("%s", yylinebuf);
    }
  }

  if (c == EOF)
  {
got_eof:
/*  firsttime = 1; should need need cause popfile restores old yytext */
    /*
      If the file ended without a terminating linefeed, then put one there!
    */
    if (last_c != '\n')
    {
      c = '\n';
      linectr++;
      pop_file();
    }
    else
      c = pop_file() ? ngetc() : EOF;
  }
  return last_c = c;
}


IsNextCharNewline(void)
{
  return (bp < 0 && *yytext == '\n');
}


/*
  MoveToStartOfToken()
    This is called when the bNextSymbolIsLiteral is set to TRUE. The
  current text pointer is moved back to the start of the literla token.
  We need to do this so that in this case :

       f:\foo\baz\cursor.ico

  the 'f:' is not discarded.
*/
void MoveToStartOfToken()
{
  bp = -1;  /* flush the pushback stack */

  /*
    Move the text pointer backwards to the start of the token.
  */
  while (yytext >= yylinebuf && !isspace(*yytext))
    yytext--;
  yytext++;
}


/* RE_ALLOC - allocates more space for a character array. The  */
/*            new max size of the array, which is old max * 2. */
char *RE_ALLOC(oldbuf, newmax)
  char *oldbuf;
  int  *newmax;
{
  char *newbuf;
  int newsize;

  newbuf = emalloc(newsize = (*newmax) << 1);
  strncpy(newbuf, oldbuf, *newmax);
  free(oldbuf);
  *newmax = newsize;
  return (newbuf);
}
