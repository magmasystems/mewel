############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  NOTE::: This makefile is equivalent to WINLIB, except that it uses      #
#          the "combined" files in order to reduce the number of segments  #
#          needed. (ie - CHECKBOX.C combines all of the CBxxx files).      #
#          Of course, you can overcome this by using the "named segment"   #
#          feature of some compilers.                                      #
#                                                                          #
############################################################################

# Put the application name here
LIB=libmewprint.a

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = M
# Model is M for Medium, L for Large, S for Small
MODEL = L

# Microsoft C macros
CC = cc
OPT = -Gs -Oit -K
DEBUG = -g -Od
UNIXDEFS= -Dpascal= -Dcdecl= -Dfar= -Dnear=
CFLAGS = -W2 -c -DSYSV -DM_TERMINFO -DUNIX -DXENIX -DMSC -UDOS -Me -J $(UNIXDEFS) -Zp $(OPT)

.c.obj :
	$(CC) $(CFLAGS) $*.c


# OBJECT FILES

OS = prdriver.o\
wprntdc.o\
wprntout.o\
wprntdrv.o\
wprnusr.o


$(LIB): $(OS)
	ar -ru $(LIB) $(OS)
	cp $(LIB) /lib/$(LIB)

