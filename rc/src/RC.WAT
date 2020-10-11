############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL resourcecompiler,                                #
#                                                                          #
#  This is for the Watcom/386 compiler                                     #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = Z
# Model is M for Medium, L for Large, S for Small
MODEL = x

CC = wcc386
#OPT = 
DEBUG = /od /d2 -DYYDEBUG=1
CFLAGS = -mf -zq -3r -DMSC -DDOS $(DEBUG) $(OPT)
ASM = masm 
##LMODEL should be 1 if you want large data
ASMDEBUG = /Zi
ASMFLAGS = /DLMODEL=1 /DTC=0 /mx $(ASMDEBUG) /DMODL=large
LIB = wlib
LINK = wlink
LFLAGS = DEBUG A OP C OP ST=8192
#LFLAGS = OP C

OBJS=rc.obj idefine.obj intlex.obj intsym.obj ytab.obj yyerror.obj rcstring.obj rccoord.obj rcmenu.obj


.c.obj :
	$(CC) $(CFLAGS) $*.c


rcwat.exe:  $(OBJS)
	$(LINK) $(LFLAGS) N rcwat F rc,idefine,intlex,intsym,ytab,yyerror,rcstring,rccoord,rcmenu

ytab.c:         rc.y
	yacc -ld rc.y

idefine.obj:    idefine.c
	$(CC) $(CFLAGS) $*.c

intlex.obj:     intlex.c
	$(CC) $(CFLAGS) $*.c

rc.obj:         rc.c
	$(CC) $(CFLAGS) $*.c

rcstring.obj:   rcstring.c
	$(CC) $(CFLAGS) $*.c

intsym.obj:     intsym.c
	$(CC) $(CFLAGS) $*.c

ytab.obj:       ytab.c
	$(CC) $(CFLAGS) $*.c

yyerror.obj:    yyerror.c
	$(CC) $(CFLAGS) $*.c

rccoord.obj:    rccoord.c
	$(CC) $(CFLAGS) $*.c

rcmenu.obj:    rcmenu.c
	$(CC) $(CFLAGS) $*.c
