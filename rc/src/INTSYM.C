/****************************************************************************\
 * INTSYM.C - symbol table routines for the macro compiler                  *
 *                                                                          *
 * (C) Copyright 1986   Marc Adler    All Rights Reserved                   *
 *                                                                          *
 *                                                                          *
\****************************************************************************/

#include "int.h"
#include "ytab.h"

SYMBOL *Symtab;
unsigned int SymMaxSpace = NSYMBOLS;
unsigned int Sym_sp = 1;
unsigned int Sym_start = 1; /* start of the symbols for the current program */

extern int hash(char *);


/* init_prog - called once to allocate memory for the Prog and Symtable */
init_prog()
{
  Symtab = (SYMBOL *) emalloc(NSYMBOLS * sizeof(SYMBOL));
  SymMaxSpace = NSYMBOLS;
  Sym_start = Sym_sp = 1;
  compiling = TRUE;
  define_install("RC_INVOKED", "1");
  define_install("MEWEL", "1");
#ifdef sun
  define_install("sun", "1");
#endif
#ifdef VAXC
  define_install("VAXC", "1");
#endif
  return 0;
}


/* ylookup - tries to find string s in the symbol table. We search through */
/*   the portion of the symbol table belonging to the currently defined    */
/*   program. If the symbol isn't found there, search through the other    */
/*   part of the symbol table to see if it's a global variable.            */
ylookup(s)
  register char *s;
{
  register int  sp;
  register char *nam;

  int iHash = hash(s);

  for (sp = 1;  sp < Sym_sp;  sp++)
  {
    nam = Symtab[sp].name;
    if (nam && Symtab[sp].iHashVal == iHash && !stricmp(nam, s))
      return(sp);
  }
  return(0);
}


/* litlookup - looks up a literal in the symbol table. If found, returns a  */
/* pointer to the symbol table entry. This allows us to store literals once.*/
litlookup(type, val)
  int type;             /* NUMBER or STRING */
  void *val;
{
  register int  sp;
  SYMBOL *sym;

  for (sp = 1;  sp < Sym_sp;  sp++)     /* look through the entire table */
  {
    sym = &Symtab[sp];
    if (sym->type == type)
      if (sym->u.sval && !strcmp(sym->u.sval, (char *) val))
        return(sp);
  }
  return(0);
}

/* install - installs a symbol in the symbol table */
install(s, type, val)
  char   *s;
  int    type;
  void  *val;
{
  register SYMBOL *sp;

  if (Sym_sp >= SymMaxSpace)
  {
    unsigned oldsize = SymMaxSpace * sizeof(SYMBOL);
    unsigned newsize;

#if !defined(UNIX) && !defined(__WATCOMC__) && !defined(__HIGHC__)
    /*
      If allocations are limited to 64K, prevent overflow.
    */
    if (SymMaxSpace * sizeof(SYMBOL) > 32000)
    {
      newsize = 64000;
      SymMaxSpace = newsize / sizeof(SYMBOL);
    }
    else
#endif
      newsize = (SymMaxSpace <<= 1) * sizeof(SYMBOL);

    if (newsize == oldsize)  /* we were already at the maximum */
      execerror("Fatal error - Too many symbols", 0);

    if ((Symtab = (SYMBOL *) realloc((char *) Symtab, newsize)) == NULL)
      execerror("Fatal error - Too many symbols", 0);
    memset(&Symtab[Sym_sp], 0, newsize - oldsize);
  }

  sp = &Symtab[Sym_sp++];
  if (s)
  {
    sp->name = strsave(s);
    sp->iHashVal = hash(s);
  }
  sp->type  = type;

  switch (type)
  {
    case ID    :
    case STRING:
      sp->u.sval = (char *) val;
      break;
    default:
      break;
  }

  if (yydebug)
    printf("INSTALL - put %s at location %d\n", s, Sym_sp-1);
  return(Sym_sp - 1);
}


char *emalloc(size)
  int size;
{
  char *s;

  if ((s = calloc(size, 1)) == NULL)
  {
    fprintf(stderr, "OUT OF MEMORY!!!\n");
    exit(1);
  }
  return s;
}

char *strsave(str)
  char *str;
{
#if !defined(__TURBOC__) && !defined(__ZTC__)
  char *strcpy();
#endif
  char *s = emalloc(strlen(str) + 1);
  return strcpy(s, str);
}


/****************************************************************************/
/*                                                                          */
/* Function : _Hash()                                                       */
/*                                                                          */
/* Purpose  : Given a string, computes the string's hash value.             */
/*                                                                          */
/* Returns  : The hash value. (A bucket number from 0 to nSize-1)           */
/*                                                                          */
/****************************************************************************/
int hash(lpsz)
  char *lpsz;
{
  int h;

  for (h = 0;  *lpsz;  h += *lpsz++)
    ;
  return (h % 59);
}

