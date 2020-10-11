# define YYERRCODE 256

/*
switch (yyerrflag)
{
  case 0:
    if ((yyn = yypact[yystate]) > YYFLAG && yyn < YYLAST)
    {
      register int x;
      for (x = (yyn > 0) ? yyn : 0;  x < YYLAST;  x++)
        if (yychk[yyact[x]] == x - yyn && x - yyn != YYERRCODE)
          yyerror(NULL, yydisplay(x - yyn));
    }
    yyerror(NULL, NULL);
*/
    
/******************************/

#include "int.h"

#if 0	/* initialization does not work with VMS, yyerfp is not used anyway */
FILE *yyerfp = stdout;
#endif

yyerror(s, t)
  char *s, *t;
{
  extern int yynerrs;
  static int list = 0;
  
  if (s || !list)       /* header necessary? */
  {
    yynerrs++;
    yywhere();
    if (s)              /* simple message? */
    {
      printf("%s\n", s);
      return 0;
    }
    if (t)              /* first token? */
    {
#ifdef NOTDEF
      printf("expecting %s", t);
#endif
      list = 1;
      return 0;
    }
    return 0;
  }
  
  if (t)     /* print an item in the list of tokens to expect */
  {
#ifdef NOTDEF
    printf(" %s", t);
#endif
    return 0;
  }
  
  printf("\n");
  list = 0;
  return 0;
}

char *yydisplay(c)
  int c;
{
  static char buf[15];
  static char *token[] =
  {
#include "ytoken.h"
    NULL
  };
  
  switch (c)
  {
    case 0 :            return "[end of file]";
    case YYERRCODE :    return "[error]";
    case '\b' :         return "'\\b'";
    case '\f' :         return "'\\f'";
    case '\n' :         return "'\\n'";
    case '\r' :         return "'\\r'";
    case '\t' :         return "'\\t'";
  }
  
  if (c > 256 && c < 256 + (sizeof(token)/sizeof(token[0])))
    return token[c - 257];
    
  if (isascii(c) && isprint(c))
    sprintf(buf, "'%c'", c);
  else if (c < 256)
    sprintf(buf, "char %04.3o", c);
  else
    sprintf(buf, "token %d", c);
  return buf;
}


typedef struct fileinfo         /* nested file info for the compiler */
{
  struct fileinfo *prevfile;
  char   *filename;
  FILE   *fp;
  int    linectr;
} FILEINFO;

extern FILEINFO *CurrFileInfo;
extern int      linectr;
extern char     *yytext;
extern char     yylinebuf[256];

/* 
   A header line should look like this :
     source.c, line 10 near "badsymbol":
*/

yywhere()
{
  int colon = 0;
  char *fname;
  
  fname = (CurrFileInfo) ? CurrFileInfo->filename : NULL;

  /* 1) Print the current file name */
  if (fname && *fname && strcmp(fname, "\"\""))   /* we have a file name */
  {
    char *cp = fname;
    int  len = strlen(fname);
    
    if (*cp == '*')
      cp++, len -= 2;
    if (strncmp(cp, ".\\", 2) == 0)
      cp += 2, len -= 2;
    printf("%.*s", len, cp);
    colon = 1;
  }
  
  /* 2) Print the line number */
  if (linectr > 0)
  {
    printf("(%d) : ", linectr);
    colon = 1;
  }
  
  /* 3) Print the proximity */
  if (yytext && *yytext)
  {
    int i;
    int col;
    
    for (i = 0;  i < 20;  i++)          /* scan for the end-of-line */
      if (!yytext[i] || yytext[i] == '\n')
        break;
    if (i)
    {
#ifdef NOTDEF
      if (colon)
        printf(" ");
      printf("near \"%.*s\"", 1, yytext);
#endif
      col = yytext - yylinebuf + 1;
      printf("syntax error near column %d\n", col);
      printf("%s", yylinebuf);
      for (i = 1;  i < col;  i++)
        putchar(' ');
      putchar('^');
      putchar('\n');

      colon = 1;
    }
  }
  
  /* 4) Print the colon */
#ifdef NOTDEF
  if (colon)
    printf(": ");
#endif
}


#ifdef NOTDEF
yymark()
{
  char *malloc();

  if (CurrFileInfo->filename)
    free(CurrFileInfo->filename);
  CurrFileInfo->filename = malloc(yyleng);
  if (CurrFileInfo->filename)
    sscanf(yytext, "# %d %s", &linectr, CurrFileInfo->filename);
}
#endif

