/****************************************************************************\
 * INT.H - header file for the macro compiler                               *
 *                                                                          *
 * (C) Copyright 1986   Marc Adler    All Rights Reserved                   *
 *                                                                          *
 *                                                                          *
\****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#if defined(__HIGHC__)
#define MSC
#endif

#ifdef VAXC 		/* no fcntl.h */
#define	nothing		/* argh */
#include <file.h>
#include <unixio.h>
#define tell(fd) lseek((fd), 0L, SEEK_CUR)
#else
#include <fcntl.h>
#endif  /* VAXC */

#if defined(UNIX) || defined(VAXC)
#define O_BINARY   0
#else
#include <io.h>
#endif

#include <stddef.h>   /* for the offsetof() macro */

#if defined(__TURBOC__)
#include <alloc.h>
#elif defined(VAXC)
#include <stdlib.h>
#elif !defined(__ZTC__)
#include <malloc.h>
#endif

#if defined(__ZTC__)
#define tell(fd) lseek((fd), 0L, SEEK_CUR)
#endif

#ifdef UNIX
extern int  stricmp();
extern char *strupr();
#endif

#ifndef iscsym
#define iscsym(c)  ( (isalnum(c)) || ((c) == '_') )
#endif

#if defined(UNIX)
extern int nwrite(int, void *, int);
#else
extern int nwrite(int, void *, unsigned int);
#endif

#ifndef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

#if defined(UNIX)
#define CH_SLASH   '/'
#define STR_SLASH  "/"
#elif defined(VAXC)
#define CH_SLASH   '/'
#define STR_SLASH  "/"
#else  /* !UNIX */
#define CH_SLASH   '\\'
#define STR_SLASH  "\\"
#endif



#define NO        0
#define YES       1
#define FALSE     0
#define TRUE      1
#define NPROGS    16
#define MAXCHARS  1024
#define NSYMBOLS  100
#define PROGSIZE  1000
#define MAXDIM    8

#define VOID      void
typedef unsigned char  BYTE;
typedef unsigned int   WORD;
typedef unsigned int   BOOL;
typedef unsigned long  DWORD;

#ifndef PASCAL
#if (defined(DOS) || defined(OS2))
#define PASCAL pascal
#else
#define PASCAL
#endif
#endif


typedef int PROG;

/* This structure contains header information for each defined function. */
/* This is used by the compiler and read in by the interpreter.          */
typedef struct _program
{
  char name[22];     /* the name of the function                         */
  int  firststmt;    /* index in Prog[] where the first statement begins */
  int  firstsym;     /* index in Symtab[] where first symbol begins      */
  int  numparams;    /* the number of parameters in the function         */
} PROGRAM;

/* PROGDEF is used by the interpreter to store info about the various    */
/* macros which are loaded. Since the macros may come from different     */
/* files, the PROGDEF structure contains some per-file information, such */
/* as the pointer to the enclosing macro file's symbol table and the     */
/* file's p-code array.                                                  */
typedef struct _proglist
{
  struct _proglist *next;       /* link to the next macro function   */
  PROG   *first_stmt;           /* ptr to the first stmt of the func */
  struct _symbol *symtab;       /* ptr to the symbol table           */
  char   *progname;             /* the name of the macro             */
  int    stmt_offset;           /* offset from Prog2 of first statement */
  int    symtab_offset;         /* offset from Symtab2 of first symbol  */
  int    numparams;             /* number of params to be called with   */
} PROGDEF;


/* This structure defined the symbol table information */
typedef struct _symbol
{
  char *name;       /* The name of the entry */
  int  iHashVal;
  int  type;        /* The type (NUMBER, NVAR, STRING, SVAR) */
  int  scope;       /* The scope of the variable */
#define LOCAL  0
#define GLOBAL 1
#define PARAM  2
  int  array_width;     /* if this is an array, this is the # of elements */
  int  nDims;
  int  *pDims;
  union             /* The value of the variable or literal */
  {
    int  nval;
    unsigned unval;
    long lval;
    unsigned long ulval;
    char *sval;

    char *bytearrayval; /* for byte array       */
    int  *narrayval;    /* for integer array    */
    unsigned *unarrayval;
    long *larrayval;
    unsigned long *ularrayval;
    char **sarrayval;   /* for string  array    */

    int  record_end;    /* for STRUCT           */
  } u;
} SYMBOL;

typedef struct defines
{
  struct defines *next;
  char   *definition;
  char   *expansion;
  int    iDefHash;
} DEFSYM;


extern SYMBOL *Symtab;
extern PROG   *Prog;
#ifndef NEWINT
extern PROG   yylval;
#endif

extern int    pc;
extern int    Prog_sp;
extern unsigned int Sym_sp;
extern int    linectr;
extern int    yydebug;
extern int    hash_lines_only;

extern FILE   *ifd;

extern char   *emalloc(int);
extern char   *strsave(char *);

extern PROG    *Prog2;
extern SYMBOL  *Symtab2;

extern PROGDEF *Hooks[];

#define STOP  0

/* This structure is for this lexical analyzer to identify keywords */
typedef struct _keyword
{
  char *name;
  int  val;
} KEYWORD;


extern KEYWORD ed_functions[];
extern KEYWORD Reserved[];

typedef struct list
{
  struct list *next;
  BYTE *data;
} LIST;

extern  struct list *PASCAL ListCreate(unsigned char *data);
extern  void PASCAL ListAdd(struct list * *headptr,struct list *element);
extern  void PASCAL ListInsert(struct list * *headptr,struct list *element,struct list *elBefore);
extern  struct list *PASCAL ListFindData(struct list *head,unsigned char *data);
extern  void PASCAL ListDelete(struct list * *head,struct list *element);
extern  struct list *PASCAL ListGetNth(struct list *headptr,int n);
extern  int PASCAL ListGetLength(struct list *headptr);
extern  int PASCAL ListFree(struct list * *headptr,int bFreeData);


/*
 * These bits are set in IfStack[]
 */
#define WAS_COMPILING   1               /* TRUE if compile set at entry */
#define ELSE_SEEN       2               /* TRUE when #else processed    */
#define TRUE_SEEN       4               /* TRUE when #if TRUE processed */

#define MAXIFNEST       32
extern char    IfStack[MAXIFNEST];     /* #if information              */
#define compiling   (IfStack[0])
extern char    *ifptr;                 /* -> current ifstack item      */

/*
  Message box flags
*/
#define MB_OK		    0x0000
#define MB_OKCANCEL	    0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL	    0x0003
#define MB_YESNO	    0x0004
#define MB_RETRYCANCEL	    0x0005
#define MB_TYPEMASK	    0x000F
#define MB_ICONHAND	    0x0010
#define MB_ICONQUESTION	    0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONMASK	    0x00F0
#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP         MB_ICONHAND
#define MB_DEFBUTTON1	    0x0000
#define MB_DEFBUTTON2	    0x0100
#define MB_DEFBUTTON3	    0x0200
#define MB_DEFMASK	    0x0F00
#define MB_APPLMODAL	    0x0000
#define MB_SYSTEMMODAL	    0x1000
#define MB_TASKMODAL	    0x2000
#define MB_NOFOCUS	    0x8000

/*
  Stuff for the RC translator ...
*/
extern VOID  AddDialogBox();
extern VOID  AddDialogControl();
extern VOID  AddAcceleratorTable();
extern VOID  AddAcceleratorKey();
extern VOID  AddStringTable();
extern VOID  AddString();
extern VOID  AddMenu();
extern VOID  AddMenuItem();
extern VOID  EndPopup();

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg rc.c */
int  main(int ,char **);
int  execerror(char  *,char *);
int  write_prog(int);
char  *drop_extension(char *);
int  AppendResToExe(char  *);
int  usage(void);
#ifdef UNIX
int  nwrite(int,void  *,int);
#else
int  nwrite(int,void  *,unsigned int);
#endif
int  word_pad(int);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg idefine.c */
struct  defines *check_define(char  *);
void define_install(char  *,char  *);
int  include_file(char  *);
void push_file(char *,FILE *);
int  pop_file(void);
char  *DosSearchPath(char  *,char  *,char  *);
void AddIncludePath(char *);
int  ProcessConditional(int);
int  EvalIfdef(int);
void UnDef(char *);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg intlex.c */
int  yylex(void);
int  get_token(char  *);
int  get_alpha_token(char *);
int  eat_whitespace(void);
void MoveToStartOfToken();
unsigned long  xtol(char  *);
int  strlower(char  *);
int  eat_comment(void);
int  eat_line(void);
int  esc(int);
int  follow(int,int,int);
int  check_reserved_word(char  *,struct  _keyword *,int);
int  init_getbuf(void);
int  nungetc(int);
int  nungets(char  *);
int  ngetc(void);
int  IsNextCharNewline(void);
char  *RE_ALLOC(char  *,int  *);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg intsym.c */
int  init_prog(void);
int  ylookup(char  *);
int  litlookup(int,void  *);
int  install(char  *,int,void *);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg ytab.c */
void  push(long);
long  pop(void);
unsigned long  WriteResourceHeader(int,int);
void  ReadUserResource(char  *);
int  ReadBinaryResourceFromFile(int,int,int);
int  WriteIDTable(void);
int  StringToClassID(char  *,unsigned long  *, int *);
int  CtrlTypeToCT(int);
int  WindowizeString(char  *);
int  AddShiftToKey(unsigned int,unsigned int);
void  MEWELizeCoords(unsigned int  *,unsigned int *,unsigned int *,unsigned int  *,unsigned int,char  *);
int  yyparse(void);
extern int  ProcessStringTables(void);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg yyerror.c */
int  yyerror(char  *,char  *);
char  *yydisplay(int);
int  yywhere(void);

/* cl /c /AL /J /DMSC /Zp /Zi /Od /DYYDEBUG=1 /Zg rcstring.c */
int  StringInfoInit(void);
int  StringInfoAddString(int,int);
int  StringInfoDumpStrings(void);
int  StringInfoComp(const void *, const void *);

/*
  RCCOORD.C
*/
void MEWELizeCoords(WORD *, WORD *, WORD *, WORD *, WORD, char *);

/*
  RCMENU.C
*/
int  CreateMenuNode(WORD, WORD, char *);
int  LinkMenuNodeToSibling(int, int);
int  LinkMenuNodeToParent(int, int);
void WriteMenuTree(int);
void FreeMenuTree(void);

