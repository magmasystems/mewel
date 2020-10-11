############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  Borland C++ GUI version for BGI (or GX or MetaWindows)                  #
#                                                                          #
#  Usage :                                                                 #
#                                                                          #
# make [-DMW] [-DGX] [-DPL] [-DD] [-DRTL] [-DDP16] [-DDLL] -f winlibbg.mak #
#                                                                          #
#   where  -DD builds a debugging version                                  #
#          -DGX builds a version for Genus GX                              #
#          -DMW builds a version for MetaWindows                           #
#          -DPL builds a version for Pharlap 286 Extender                  #
#          -DDP16 builds a version for DPMI16                              #
#          -DDP32 builds a version for DPMI32                              #
#          -DDLL  builds a DLL version                                     #
#                                                                          #
#   The default is for an optimized, real-mode version for BGI.            #
#                                                                          #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = T

#
# These are two hard-coded directory names which you need to change
#
# Root directory for MEWEL
# Root directory of the Borland compiler
#
MEWELPATH=.
BCPATH=\bc4

# Model is M for Medium, L for Large, S for Small
MODEL = -ml
!if $d(DP32) || $d(TNT)
MODEL =
!endif

#
# Specify the compiler to use
#
CC=bcc
!if $d(DP32) || $d(TNT)
CC=bcc32
!endif

#
# Optimization : -G favors speed over size, -O improves jumps & loops
#
!ifdef D
DEBUG=-v
!else
OPT=-G -O
!endif

#
# Include path
#
INC=-I$(MEWELPATH) -I$(BCPATH)\include

!ifdef TNT
INC=$(INC) -Ic:\tnt\include
!endif

# 
# If you define either MW or GX, then a library will be built for either
# MetaWindows (by MetaGraphics) or GX (by Genus) rather than for BGI.
#   ie :  make -DGX -f winlibbg.mak
#
LIBFILE = mewelbgi
GUILIB=bgi16

!ifdef MW
ENGINE=-DMETA
METAPATH=n:\metawin
INC=$(INC) -I$(METAPATH)
LIBFILE=mewelmwb
!if $d(DP32)
GUILIB=$(METAPATH)\met_xb3o
!else
GUILIB=$(METAPATH)\met_xd2o
!endif
!endif

!ifdef GX
ENGINE=-DGX
GXPATH=e:\genus3
INC=$(INC) -I$(GXPATH)
GUILIB=$(GXPATH)\gr2_clp.lib $(GXPATH)\gx2_clp.lib
!ifdef GXTEXT
ENGINE=$(ENGINE) -DUSE_GX_TEXT
!endif
LIBFILE=mewelgxb
!endif

#
# Specify your include path for the Pharlap 286 H files
#
!ifdef PL
ENGINE=$(ENGINE)
INC=$(INC) -Id:\run286\bc4\include
!endif

#
# Specify overlay or DPMI-extender flags
#
OVL=-Yo
PREPOVL=-Y
!if $d(DP32) || $d(DP16)
OVL=
!ifdef DLL
!ifdef DP16
PREPOVL=-WXD -2
!else
PREPOVL=-WXDE
!endif
!else
PREPOVL=-WX
!ifdef DP16
PREPOVL=$(PREPOVL) -2
!endif
!endif
!endif

# Flags for Pharlap 286 and TNT
!if $d(PL) || $d(TNT)
OVL=
!if $d(PL)
PREPOVL=-2
!else
PREPOVL=-3 -WX
!endif
!endif

!ifndef DLL
STKCHK=-N
!endif

DEFINES=-DDOS -DMEWEL_GUI
!ifdef TNT
DEFINES=$(DEFINES) -DPLTNT -I\tnt\include
!endif

CFLAGS= +turboc.cfg $(PREPOVL) $(ENGINE) $(OPT) $(DEBUG) -c -d -f- -k -K $(STKCHK) $(MODEL) $(DEFINES) -H


#
# Add -DDOS286 and -2 for Pharlap
#
!ifdef PL
CFLAGS=$(CFLAGS) -DDOS286X -2
!endif

!ifdef ZAPP
CFLAGS=$(CFLAGS) -DZAPP
!endif

!ifdef ACAD
CFLAGS=$(CFLAGS) -DACAD
!endif

#
# Assembler flags
#
ASM = tasm
ASMFLAGS = /dTC=1 /dLMODEL=1 /dMODL=large /mx
!ifdef D
ASMFLAGS=$(ASMFLAGS) /zi
!endif
!ifdef PL
ASMFLAGS=$(ASMFLAGS) /dDOS286X=1
!endif
!ifdef DP16
ASMFLAGS=$(ASMFLAGS) /dDOS286X=1
!endif

#
# Linker and librarian flags
#
LIB = tlib
LINK = tlink
LFLAGS = /v

#
# For a 16-bit DLL version, pick a DEF file
#
!ifdef DP16
!ifdef DLL
!if $d(MW)
DEFFILE16=mewelmet.def
!elif $d(GX)
DEFFILE16=mewelgx.def
!else
DEFFILE16=mewelg16.def
!endif
!endif
!endif


#
# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.
#
.c.obj :
  $(CC) $(CFLAGS) $*.c
!ifdef ADD
  $(LIB) /C $(LIBFILE) -+$*.obj
!endif

.asm.obj :
	$(ASM) $(ASMFLAGS) $*;


OBJS1 = \
WACTIVE.OBJ  \
WADJUST.OBJ  \
WANYPOP.OBJ  \
WARRICON.OBJ \
WBITBLT.OBJ  \
WBITMAP.OBJ  \
WCHECKBX.OBJ \
WCHOCOLR.OBJ \
WCHOFONT.OBJ \
WCLIPBRD.OBJ \
WCOMFILE.OBJ \
WCOMFIND.OBJ \
WCREATDC.OBJ \
WCREATE.OBJ  \
WCRECURS.OBJ \
WCREICON.OBJ \
WDESKTOP.OBJ \
WDLGCTID.OBJ \
WDLGDIR.OBJ  \
WDLGOPEN.OBJ \
WDRAWTXT.OBJ \
WDRVTYPE.OBJ \
WEDITPRO.OBJ \
WEDTDRAW.OBJ \
WEMALLOC.OBJ \
WENUMOBJ.OBJ \
WEXITWIN.OBJ \
WEXTTEXT.OBJ \
WFILEIO.OBJ  \
WFILETIT.OBJ \
WFILLWIN.OBJ \
WFINDWIN.OBJ \
WGDIARC.OBJ  \
WGDIBMP2.OBJ \
WGDIBRUS.OBJ \
WGDICHAR.OBJ \
WGDICLIP.OBJ \
WGDIDDA.OBJ  \
WGDIDDIB.OBJ \
WGDIELLI.OBJ \
WGDIFILL.OBJ \
WGDIFLOD.OBJ \
WGDIFONT.OBJ \
WGDIFRAM.OBJ \
WGDIGDIB.OBJ \
WGDIGPIX.OBJ \
WGDIINV.OBJ  \
WGDILINE.OBJ \
WGDIPIE.OBJ  \
WGDIPLIN.OBJ \
WGDIPOLY.OBJ \
WGDIRGB.OBJ  \
WGDIRGN.OBJ  \
WGDISBIT.OBJ \
WGDISCRO.OBJ \
WGDISDIB.OBJ \
WGDISET.OBJ  \
WGDISPIX.OBJ \
WGDISTRD.OBJ \
WGDISTUB.OBJ \
WGDITEXT.OBJ \
WGETCLAS.OBJ \
WGETPOS.OBJ  \
WGETTEMP.OBJ \
WGRAPHIC.OBJ \
WGRAYSTR.OBJ \
WGUI.OBJ     \
WGUIBMP.OBJ  \
WGUICURS.OBJ \
WGUIDATA.OBJ \
WGUIENUM.OBJ \
WGUIFONT.OBJ \
WGUIICON.OBJ \
WGUIMENU.OBJ \
WGUIPCX.OBJ  \
WHFILEIO.OBJ \
WINACCEL.OBJ \
WINANSI.OBJ  \
WINATOM.OBJ  \
WINATOM2.OBJ \
WINBLIT.OBJ  \
WINBWCC.OBJ  \
WINCARET.OBJ \
WINCLASS.OBJ \
WINCLICK.OBJ \
WINCOLOR.OBJ \
WINCOMBO.OBJ \
WINCREAT.OBJ \
WINDATA.OBJ  \
WINDC.OBJ    \
WINDEFER.OBJ \
WINDLG.OBJ   \
WINDRAW.OBJ  \
WINDSTRY.OBJ \
WINEDIT.OBJ  \
WINENABL.OBJ \
WINENUM.OBJ  \
WINEVENT.OBJ \
WINEXEC.OBJ  \
WINEXTRA.OBJ \
WINFLASH.OBJ \
WINHOOK.OBJ  \
WININIT.OBJ  \
WINKBD.OBJ   \
WINLBDIR.OBJ \
WINMDI.OBJ   \
WINMENU.OBJ  \
WINMENU2.OBJ \
WINMODAL.OBJ \
WINMONO.OBJ  \
WINMOUSE.OBJ \
WINMOVE.OBJ  \
WINPOINT.OBJ \
WINPRINT.OBJ \
WINPROP.OBJ  \
WINQUEUE.OBJ \
WINRECT.OBJ  \
WINRES.OBJ   \
WINRES2.OBJ  \
WINRES3.OBJ  \
WINSCROL.OBJ \
WINSET.OBJ   \
WINSHOW.OBJ  \
WINSIZE.OBJ  \
WINTASK.OBJ  \
WINTERM.OBJ  \
WINTIME.OBJ  \
WINTIMER.OBJ \
WINVID.OBJ   \
WINZOOM.OBJ  \
WISCHAR.OBJ  \
WKBSTACK.OBJ \
WKEYSTAT.OBJ \
WLASTPOP.OBJ \
WLISTBOX.OBJ \
WLNKLIST.OBJ \
WLOADACC.OBJ \
WLOADBMP.OBJ \
WLOADBMP.OBJ \
WLOADDLG.OBJ \
WLOADMNU.OBJ \
WLOADSTR.OBJ \
WLPTODP.OBJ  \
WLSTRING.OBJ \
WMAPPOIN.OBJ \
WMENUBMP.OBJ \
WMENUHI.OBJ  \
WMESSAGE.OBJ \
WMETAFIL.OBJ \
WMFCSTUB.OBJ \
WMNUCHNG.OBJ \
WMNUINFO.OBJ \
WMODULE.OBJ  \
WMOUAPI.OBJ  \
WMSGBOX.OBJ  \
WMULDIV.OBJ  \
WNOTIFY.OBJ  \
WOBJECT.OBJ  \
WOPENFIL.OBJ \
WPATBLT.OBJ  \
WPLACEMT.OBJ \
WPRNTDLG.OBJ \
WPROCINS.OBJ \
WPROFILE.OBJ \
WPUSHBUT.OBJ \
WREADINI.OBJ \
WREFORM.OBJ  \
WREFRESH.OBJ \
WREGION.OBJ  \
WRGNFILL.OBJ \
WRGNFRAM.OBJ \
WRGNINV.OBJ  \
WRGNPAIN.OBJ \
WRGNVAL.OBJ  \
WRUBBER.OBJ  \
WSAVEDC.OBJ  \
WSCRLBAR.OBJ \
WSCROLDC.OBJ \
WSETRECT.OBJ \
WSETWPOS.OBJ \
WSHOWPOP.OBJ \
WSOUND2.OBJ  \
WSPRINTF.OBJ \
WSPY.OBJ     \
WSRCHPTH.OBJ \
WSTATIC.OBJ  \
WSTDPROC.OBJ \
WSTFOCUS.OBJ \
WSTRETCH.OBJ \
WSTUBSB1.OBJ \
WSTUBSG1.OBJ \
WSTUBSG2.OBJ \
WSTUBSG3.OBJ \
WSTUBSK1.OBJ \
WSTUBSS1.OBJ \
WSTUBSU1.OBJ \
WSTUBSU2.OBJ \
WSTUBSU3.OBJ \
WSTUBSU4.OBJ \
WSTUBSU5.OBJ \
WSTUBSU6.OBJ \
WSTUBSU7.OBJ \
WSYSMENU.OBJ \
WTABTEXT.OBJ \
WTEXTMET.OBJ \
WTEXTOUT.OBJ \
WUNREGCL.OBJ \
WVIEWEXT.OBJ \
WVIEWORG.OBJ \
WVIEWSCL.OBJ \
WWINEXT.OBJ  \
WWINORG.OBJ  \
WWINSCL.OBJ  \
WZORDER.OBJ

!if $d(MW) || $d(GX)
OBJSMOUSE = WGUIMOUS.OBJ
!if $d(DP16) && !$d(GX)
OBJSMOUSE = $(OBJSMOUSE) MWGRAF.OBJ
!endif
!endif

#
# Some real-mode-only files. When building for protected mode, you'll
# probably get benign warnings (not errors) from the linker.
#
!if $d(DP32)
OBJS32BIT = WDOS386.OBJ WDPMI.OBJ
!elif $d(TNT)
OBJS32BIT = WDOS386.OBJ
OBJSMEM   = WMEMGLOB.OBJ WMEMLOCL.OBJ
OBJSHARD  = WINSOUND.OBJ WGDIPAL.OBJ
!else
#
# Real mode or 16-bit protected mode
#
OBJSASM   = DELAY.OBJ    KEY.OBJ      LCGACPY.OBJ  LMEMCHR.OBJ  LMEMMOVE.OBJ \
            LWMEMSET.OBJ LMEMCPY.OBJ  LSTRING.OBJ  MOUSESUB.OBJ RECT.OBJ
OBJSHARD  = WINSOUND.OBJ WGDIPAL.OBJ
OBJSINTR  = WINALT.OBJ   WINBREAK.OBJ WININT24.OBJ
!ifndef DP16
#
# Real mode only
#
OBJSMEM   = WMEMGLOB.OBJ WMEMLOCL.OBJ
OBJSREAL  = WINDOS.OBJ
OBJSXMS   = XMSLIB.OBJ
OBJSEXEC  = WINSPAWN.OBJ CHECKPAT.OBJ EXEC.OBJ
!endif
!endif

#
# DP16 and DP32 have DLL capabilities, so we don't need MEWEL's
# stubs for Load/FreeLibrary and GetProcAddress
#
!if !$d(DP16) && !$d(DP32) && !$d(TNT)
OBJSNDLL  = WINCATCH.OBJ WINDEBUG.OBJ WLIBRARY.OBJ WSELECTR.OBJ
!endif

#
# DP16 needs the RTM patching functions
#
!if $d(DP16)
OBJSINTR = $(OBJSINTR) WBORRTM.OBJ
!endif


all :  turboc.cfg $(OBJS1) \
$(OBJSREAL) $(OBJSMEM) $(OBJS32BIT) $(OBJSASM) $(OBJSEXEC) $(OBJSHARD) \
$(OBJSINTR) $(OBJSMOUSE) $(OBJSXMS) $(OBJSNDLL)
!ifdef DLL
!ifdef DP16
#
# If RTL is defined, we reference BC40RTL.DLL. If it's not defined, then
# we get a more 'static' version, one which is 20K larger.
#
!ifdef RTL
  tlink      -L$(BCPATH)\lib -Twd $(DEBUG) @mewelg16.lnk,dpmi16+crtldll+$(GUILIB),$(DEFFILE16)
!else
  tlink /c/C -L$(BCPATH)\lib -Twd $(DEBUG) @mewelg16.lnk,dpmi16+cwl+mathwl+$(GUILIB),$(DEFFILE16)
!endif

!else
# DPMI32
  tlink32 -L$(BCPATH)\lib -Tpd -v @mewelg32.lnk
!endif
  implib $(LIBFILE).lib mewel.dll
!else
  del $(LIBFILE).lib
  $(LIB) /C /P64 $(LIBFILE) @winlibgu.lnk
!if $d(DP32) || $d(TNT)
  $(LIB) /C /P64 $(LIBFILE) +wdos386 +wdpmi
!endif
!ifdef MW
  $(LIB) /C /P64 $(LIBFILE) -winmouse
  $(LIB) /C /P64 $(LIBFILE) +wguimous
!endif
!endif

turboc.cfg :
	echo $(INC) > turboc.cfg


CHECKPAT.OBJ :  CHECKPAT.ASM

DELAY.OBJ    :  DELAY.ASM

EXEC.OBJ     :  EXEC.C

KEY.OBJ      :  KEY.ASM

LCGACPY.OBJ  :  LCGACPY.ASM

LMEMCHR.OBJ  :  LMEMCHR.ASM

LMEMCPY.OBJ  :  LMEMCPY.ASM

LMEMMOVE.OBJ :  LMEMMOVE.ASM

LSTRING.OBJ  :  LSTRING.ASM

LWMEMSET.OBJ :  LWMEMSET.ASM

RECT.OBJ     :  RECT.ASM

VBTSHLP.OBJ  :  VBTSHLP.ASM

WACTIVE.OBJ  :  WACTIVE.C

WADJUST.OBJ  :  WADJUST.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WANYPOP.OBJ  :  WANYPOP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WARRICOB.OBJ :  WARRICON.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WBITMAP.OBJ  :  WBITMAP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCHECKBX.OBJ :  WCHECKBX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCHOCOLR.OBJ :  WCHOCOLR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCHOFONT.OBJ :  WCHOFONT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCLIPBRD.OBJ :  WCLIPBRD.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCOMFILE.OBJ :  WCOMFILE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCOMFIND.OBJ :  WCOMFIND.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCREATDC.OBJ :  WCREATDC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCREATE.OBJ  :  WCREATE.C

WCRECURS.OBJ :  WCRECURS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WCREICON.OBJ :  WCREICON.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WDESKTOP.OBJ :  WDESKTOP.C

WDLGCTID.OBJ :  WDLGCTID.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WDLGDIR.OBJ  :  WDLGDIR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WDLGOPEN.OBJ :  WDLGOPEN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WDRAWTXT.OBJ :  WDRAWTXT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WDRVTYPE.OBJ :  WDRVTYPE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WEDITPRO.OBJ :  WEDITPRO.C

WEDTDRAW.OBJ :  WEDTDRAW.C

WEMALLOC.OBJ :  WEMALLOC.C

WENUMFNT.OBJ :  WENUMFNT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WENUMOBJ.OBJ :  WENUMOBJ.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WEXITWIN.OBJ :  WEXITWIN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WEXTTEXT.OBJ :  WEXTTEXT.C

WFILEIO.OBJ  :  WFILEIO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WFILETIT.OBJ :  WFILETIT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WFILLWIN.OBJ :  WFILLWIN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WFINDWIN.OBJ :  WFINDWIN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIARC.OBJ  :  WGDIARC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIBMP2.OBJ :  WGDIBMP2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIBRUS.OBJ :  WGDIBRUS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDICHAR.OBJ :  WGDICHAR.C

WGDICLIP.OBJ :  WGDICLIP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIELLI.OBJ :  WGDIELLI.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIFLOD.OBJ :  WGDIFLOD.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIFONT.OBJ :  WGDIFONT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIFRAM.OBJ :  WGDIFRAM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIGDIB.OBJ :  WGDIGDIB.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIGPIX.OBJ :  WGDIGPIX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIINV.OBJ  :  WGDIINV.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIMAP.OBJ  :  WGDIMAP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIPAL.OBJ  :  WGDIPAL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIPIE.OBJ  :  WGDIPIE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIPLIN.OBJ :  WGDIPLIN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIPOLY.OBJ :  WGDIPOLY.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIRGB.OBJ  :  WGDIRGB.C

WGDIRGN.OBJ  :  WGDIRGN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISBIT.OBJ :  WGDISBIT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISCRO.OBJ :  WGDISCRO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISDIB.OBJ :  WGDISDIB.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISET.OBJ  :  WGDISET.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDIDDA.OBJ  :  WGDIDDA.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISPIX.OBJ :  WGDISPIX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISTRD.OBJ :  WGDISTRD.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGDISTUB.OBJ :  WGDISTUB.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGETCLAS.OBJ :  WGETCLAS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGETPOS.OBJ  :  WGETPOS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGETTEMP.OBJ :  WGETTEMP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGRAPHIC.OBJ :  WGRAPHIC.C

WGRAYSTR.OBJ :  WGRAYSTR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGUI.OBJ     :  WGUI.C

WGUIBMP.OBJ  :  WGUIBMP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGUICURS.OBJ :  WGUICURS.C

WGUIENUM.OBJ :  WGUIENUM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGUIFONT.OBJ :  WGUIFONT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGUIPCX.OBJ  :  WGUIPCX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WHFILEIO.OBJ :  WHFILEIO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINACCEL.OBJ :  WINACCEL.C

WINALT.OBJ   :  WINALT.C

WINANSI.OBJ  :  WINANSI.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINATOM.OBJ  :  WINATOM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINATOM2.OBJ :  WINATOM2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINBLIT.OBJ  :  WINBLIT.C

WINBREAK.OBJ :  WINBREAK.C

WINBWCC.OBJ  :  WINBWCC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINCARET.OBJ :  WINCARET.C

WINCATCH.OBJ :  WINCATCH.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINCLASS.OBJ :  WINCLASS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINCLICK.OBJ :  WINCLICK.C

WINCOLOR.OBJ :  WINCOLOR.C

WINCOMBO.OBJ :  WINCOMBO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINCREAT.OBJ :  WINCREAT.C

WINDATA.OBJ  :  WINDATA.C

WINDC.OBJ    :  WINDC.C

WINDEBUG.OBJ :  WINDEBUG.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINDEFER.OBJ :  WINDEFER.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINDLG.OBJ   :  WINDLG.C

WINDOS.OBJ   :  WINDOS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINDRAW.OBJ  :  WINDRAW.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINDSTRY.OBJ :  WINDSTRY.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINEDIT.OBJ  :  WINEDIT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINENABL.OBJ :  WINENABL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINENUM.OBJ  :  WINENUM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINEVENT.OBJ :  WINEVENT.C

WINEXEC.OBJ  :  WINEXEC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINEXTRA.OBJ :  WINEXTRA.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINFLASH.OBJ :  WINFLASH.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINHOOK.OBJ  :  WINHOOK.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WGUIICON.OBJ :  WGUIICON.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WININIT.OBJ  :  WININIT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WININT24.OBJ :  WININT24.C

WINKBD.OBJ   :  WINKBD.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINLBDIR.OBJ :  WINLBDIR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMDI.OBJ   :  WINMDI.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMENU.OBJ  :  WINMENU.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMENU2.OBJ :  WINMENU2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMODAL.OBJ :  WINMODAL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMONO.OBJ  :  WINMONO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINMOUSE.OBJ :  WINMOUSE.C

WINMOVE.OBJ  :  WINMOVE.C

WINQUEUE.OBJ :  WINQUEUE.C

WINPOINT.OBJ :  WINPOINT.C

WINPRINT.OBJ :  WINPRINT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINPROP.OBJ  :  WINPROP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINRECT.OBJ  :  WINRECT.C

WINRES.OBJ   :  WINRES.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINRES2.OBJ  :  WINRES2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINRES3.OBJ  :  WINRES3.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSCROL.OBJ :  WINSCROL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSET.OBJ   :  WINSET.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSHOW.OBJ  :  WINSHOW.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSIZE.OBJ  :  WINSIZE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSOUND.OBJ :  WINSOUND.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINSPAWN.OBJ :  WINSPAWN.ASM

WINTASK.OBJ  :  WINTASK.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINTERM.OBJ  :  WINTERM.C

WINTIME.OBJ  :  WINTIME.C

WINTIMER.OBJ :  WINTIMER.C

WINVID.OBJ   :  WINVID.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WINZOOM.OBJ  :  WINZOOM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WISCHAR.OBJ  :  WISCHAR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WKBSTACK.OBJ :  WKBSTACK.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLASTPOP.OBJ :  WLASTPOP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLIBRARY.OBJ :  WLIBRARY.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLISTBOX.OBJ :  WLISTBOX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLNKLIST.OBJ :  WLNKLIST.C

WLOADACC.OBJ :  WLOADACC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLOADBMP.OBJ :  WLOADBMP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLOADDLG.OBJ :  WLOADDLG.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLOADMNU.OBJ :  WLOADMNU.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLOADSTR.OBJ :  WLOADSTR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLPTODP.OBJ  :  WLPTODP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WLSTRING.OBJ :  WLSTRING.C

WMEMGLOB.OBJ :  WMEMGLOB.C

WMEMLOCL.OBJ :  WMEMLOCL.C

WMENUBMP.OBJ  :  WMENUBMP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMENUHI.OBJ  :  WMENUHI.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMESSAGE.OBJ :  WMESSAGE.C

WMETAFIL.OBJ :  WMETAFIL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMFCSTUB.OBJ :  WMFCSTUB.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMNUCHNG.OBJ :  WMNUCHNG.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMNUINFO.OBJ :  WMNUINFO.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMODULE.OBJ  :  WMODULE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMOUAPI.OBJ  :  WMOUAPI.C

WMSGBOX.OBJ  :  WMSGBOX.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WMULDIV.OBJ  :  WMULDIV.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WOBJECT.OBJ  :  WOBJECT.C

WOPENFIL.OBJ :  WOPENFIL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPATBLT.OBJ  :  WPATBLT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPLACEMT.OBJ :  WPLACEMT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPROCINS.OBJ :  WPROCINS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPROFILE.OBJ :  WPROFILE.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPRNTDLG.OBJ :  WPRNTDLG.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WPUSHBUT.OBJ :  WPUSHBUT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WREADINI.OBJ :  WREADINI.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WREFORM.OBJ  :  WREFORM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WREGION.OBJ  :  WREGION.C

WRGNFILL.OBJ :  WRGNFILL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WRGNFRAM.OBJ :  WRGNFRAM.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WRGNINV.OBJ  :  WRGNINV.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WRGNPAIN.OBJ :  WRGNPAIN.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WRGNVAL.OBJ  :  WRGNVAL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WRUBBER.OBJ  :  WRUBBER.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSAVEDC.OBJ  :  WSAVEDC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSCROLDC.OBJ :  WSCROLDC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSCRLBAR.OBJ :  WSCRLBAR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSELECTR.OBJ :  WSELECTR.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSETRECT.OBJ :  WSETRECT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSETWPOS.OBJ :  WSETWPOS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSHOWPOP.OBJ :  WSHOWPOP.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSOUND2.OBJ  :  WSOUND2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSPRINTF.OBJ :  WSPRINTF.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSPY.OBJ     :  WSPY.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTATIC.OBJ  :  WSTATIC.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTDPROC.OBJ :  WSTDPROC.C

WSTFOCUS.OBJ :  WSTFOCUS.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTRETCH.OBJ :  WSTRETCH.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSB1.OBJ :  WSTUBSB1.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSG1.OBJ :  WSTUBSG1.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSG2.OBJ :  WSTUBSG2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSG3.OBJ :  WSTUBSG3.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSK1.OBJ :  WSTUBSK1.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSS1.OBJ :  WSTUBSS1.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU1.OBJ :  WSTUBSU1.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU2.OBJ :  WSTUBSU2.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU3.OBJ :  WSTUBSU3.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU4.OBJ :  WSTUBSU4.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU5.OBJ :  WSTUBSU5.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU6.OBJ :  WSTUBSU6.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSTUBSU7.OBJ :  WSTUBSU7.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WSYSMENU.OBJ :  WSYSMENU.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WTABTEXT.OBJ :  WTABTEXT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WTEXTMET.OBJ :  WTEXTMET.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WTEXTOUT.OBJ :  WTEXTOUT.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WUNREGCL.OBJ :  WUNREGCL.C
  $(CC) $(CFLAGS) $(OVL) $*.c

WZORDER.OBJ  :  WZORDER.C


clean :
	del *.pdb
        del *.pch
        del *.err
        del *.obj
        del $(LIBFILE).lib



