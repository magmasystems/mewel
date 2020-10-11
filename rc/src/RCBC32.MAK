BCCD32  = bcc32
TLINK32 = TLink32

BCPATH = n:\BC4

#
# Debug or Optimized
#
!ifdef D
DEBUG=-v
!endif

#
# Options
#
IDE_LFLAGSD32 =  -L$(BCPATH)\LIB $(DEBUG)
LLATD32_rcdexe =  -Tpe -ax
LNIEAT_rcdexe = -x
LEAT_rcdexe = $(LLATD32_rcdexe)

.c.obj:
  $(BCCD32) -P- -c -WX -w-rvl $(DEBUG) {$< }

#
# Dependency List
#
Dep_rc = \
   rc32.exe

Dep_rcdexe = \
   idefine.obj\
   intlex.obj\
   intsym.obj\
   rc.obj\
   rccoord.obj\
   rcmenu.obj\
   rcstring.obj\
   ytab.obj\
   yyerror.obj

rc32.exe : $(Dep_rcdexe)
  $(TLINK32) @&&|
 $(IDE_LFLAGSD32) $(LEAT_rcdexe) $(LNIEAT_rcdexe) +
c0x32.obj+
idefine.obj+
intlex.obj+
intsym.obj+
rc.obj+
rccoord.obj+
rcmenu.obj+
rcstring.obj+
ytab.obj+
yyerror.obj
rc32.exe
nul
dpmi32.lib+
cw32.lib
|

