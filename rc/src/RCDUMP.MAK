############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL Rsource Dumper                                   #
#                                                                          #
#  Microsoft C version.                                                    #
#                                                                          #
#  nmk -f rcdump [D=1] [PL=1]                                              #
#     if D is defined, then a debugging version will be built.             #
#     if PL is defined, a version for Pharlap 286 will be built.           #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = M
# Model is M for Medium, L for Large, S for Small
MODEL = L

# Microsoft C macros
CC = cl
!ifdef D
DEBUG = /Zi /Od /DYYDEBUG=1
!else
OPT = /Oit /Gs
!endif

CFLAGS = /c /A$(MODEL) /J /DMSC /Zp $(OPT) $(DEBUG)

#
# For Pharlap, throw the /Lp switch
#
!ifdef PL
CFLAGS=$(CFLAGS) /Lp
!endif


LINK = link
!ifdef D
LFLAGS = /co /st:10000
!else
LFLAGS = /st:10000 
!endif


OBJS=rcdump.obj rcname.obj

.c.obj :
	$(CC) $(CFLAGS) $*.c


all : rcdump.exe

rcdump.obj:    rcdump.c

rcname.obj:    rcname.c


!ifdef PL
rcdump.exe:  $(OBJS)
  $(LINK) $(LFLAGS) /noe $**,rcdump,nul,/nod:llibce llibcep llibpe phapi,protmode.def
!else
rcdump.exe:  $(OBJS)
  $(LINK) $(LFLAGS) $**,rcdump.exe;
!endif

