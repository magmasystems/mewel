############################################################################
#                                                                          #
#  MAKEFILE for the ME macro compiler,                                     #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = M
# Model is M for Medium, L for Large, S for Small
MODEL = L

# Microsoft C macros
CC = cl
DEBUG = /Zi /Od /DYYDEBUG=1
OPT = /Oit /Gs
CFLAGS = /c /A$(MODEL) /J /DMSC /Zp $(OPT)
LINK = link
LFLAGS = 

OBJS=rcname.obj rcdump.obj

.c.obj :
	$(CC) $(CFLAGS) $*.c

all : rcname.exe

rcname.exe:  $(OBJS)
  $(LINK) $(LFLAGS) $**,rcname.exe;


