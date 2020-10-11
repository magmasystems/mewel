############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  Microsoft C GUI version for MSC graphics.lib (or GX or MetaWindows)     #
#                                                                          #
#  Usage :                                                                 #
#                                                                          #
#     nmake -f winlibmg.mak [MW=1] [GX=1] [PL=1] [D=1]                     #
#                                                                          #
#   where  D=1 builds a debugging version                                  #
#          GX=1 builds a version for Genus GX                              #
#          MW=1 builds a version for MetaWindows                           #
#          PL=1 builds a version for Pharlap 286 Extender                  #
#          RAT=1 builds a version for Rational DOS/16M Extender            #
#                                                                          #
#   The default is for an optimized, real-mode version for MSC graphics.   #
#                                                                          #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = M
# Model is M for Medium, L for Large, S for Small
MODEL = L

#
# MEWELPATH is the directory where the MEWEL include files can be found
#
MEWELPATH=\mewel

#
# For the Pharlap 286 DOS extender, RUN286PATH is the directory where
# the Pharlap include files can be found (like PHAPI.H).
#
RUN286PATH=\run286\inc

#
# For the Rational DOS/16M extender, DOS16PATH is the directory where
# the Rational include files can be found.
#
DOS16MPATH=f:\dos16m\source

#
# MSCPATH should be the path where the compiler's .H files can be found
#
MSCPATH=\msvc\include

#
# This specifies the complete INCLUDE path for the compiler
#
INC1=/I$(MEWELPATH) /I$(MSCPATH)


#
# Microsoft C macros
#
!ifdef PL
CC = cl
!else
CC = cl
!endif

# QUICK - MSC 6 stuff for quick compiles
# /qc = quick compile   /Gi = padding    /Zr = check NULL ptrs
QUICK=/qc /Gi

#
# Optimization and debugging flags
#
!ifdef D
DEBUG = /Od /Zi /YX
!else
OPT = /Oit /Gs /YX
!endif

#
# LIBFILE is the name of the resulting library. It has the root of 'mewel',
# the two-letter abbreviation of the graphics engine, and the letter 'm'
# to signify the Microsoft compiler.
# The abbreviations for the graphics engines are :
#   mg - Microsoft Graphics.lib
#   gx - Genus GX
#   mw - MetaGraphics' MetaWindows
#
LIBFILE=mewelmgm.lib

DEF1=/DMSC /DDOS /DMEWEL_GUI
!ifdef ZAPP
DEF1=$(DEF1) /DZAPP
!endif

!ifdef GX
DEF2=/DGX
LIBFILE=mewelgxm.lib
INC2= /Ie:\genus3
!ifdef GXTEXT
DEF2=$(DEF2) /DUSE_GX_TEXT
!endif
!endif

!ifdef MW
DEF2=/DMETA
LIBFILE=mewelmwm.lib
INC2= /I\metawin
!endif

!ifdef PL
DEF3=/DDOS286X /Lp
INC3= /I$(RUN286PATH)
!else
!ifdef RAT
DEF3=/DDOS16M
INC3= /I$(DOS16MPATH)
!endif
!endif

CFLAGS = $(DEBUG)$(OPT) /A$(MODEL) /W3 /c /J $(DEF1) $(DEF2) $(DEF3) $(INC1) $(INC2) $(INC3)

#
# Assembler stuff
#
ASM = masm 
# LMODEL should be 1 if you want large data
ASMFLAGS = /DLMODEL=1 /DTC=0 /mx /DMODL=large
!ifdef D
ASMFLAGS=$(ASMFLAGS) /Zi
!endif
!ifdef PL
ASMFLAGS=$(ASMFLAGS) /DDOS286X
!endif
!ifdef RAT
ASMFLAGS=$(ASMFLAGS) /DDOS16M
!endif

LIB = lib

#
# Inference rules for C and ASM files
#
.c.obj :
  $(CC) $(CFLAGS) $*.c

.asm.obj :
  $(ASM) $(ASMFLAGS) $*;


OBJS = \
CHECKPAT.OBJ \
DELAY.OBJ    \
EXEC.OBJ     \
KEY.OBJ      \
LCGACPY.OBJ  \
LMEMCHR.OBJ  \
LMEMMOVE.OBJ \
LWMEMSET.OBJ \
LMEMCPY.OBJ  \
LSTRING.OBJ  \
MOUSESUB.OBJ \
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
WGDIPAL.OBJ  \
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
WINALT.OBJ   \
WINANSI.OBJ  \
WINATOM.OBJ  \
WINATOM2.OBJ \
WINBLIT.OBJ  \
WINBREAK.OBJ \
WINCARET.OBJ \
WINCATCH.OBJ \
WINCLASS.OBJ \
WINCLICK.OBJ \
WINCOLOR.OBJ \
WINCOMBO.OBJ \
WINCREAT.OBJ \
WINDATA.OBJ  \
WINDC.OBJ    \
WINDEBUG.OBJ \
WINDLG.OBJ   \
WINDOS.OBJ   \
WINDEFER.OBJ \
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
WININT24.OBJ \
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
WINSOUND.OBJ \
WINSPAWN.OBJ \
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
WLIBRARY.OBJ \
WLISTBOX.OBJ \
WLNKLIST.OBJ \
WLOADACC.OBJ \
WLOADBMP.OBJ \
WLOADDLG.OBJ \
WLOADMNU.OBJ \
WLOADSTR.OBJ \
WLPTODP.OBJ  \
WLSTRING.OBJ \
WMAPPOIN.OBJ \
WMEMGLOB.OBJ \
WMEMLOCL.OBJ \
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
WSELECTR.OBJ \
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
WZORDER.OBJ  \
XMSLIB.OBJ


!ifdef RAT
OBJSMISC =      MOUSEINT.OBJ
!endif

!ifdef MW
OBJSMISC =      WGUIMOUS.OBJ
!endif

!ifdef GX
OBJSMISC =      WGUIMOUS.OBJ
!ifdef GXPRINT
OBJSMISC = $(OBJSMISC) WGXPRINT.OBJ
!endif
!endif


all :  $(OBJS) $(OBJSMISC)
	del $(LIBFILE)
	$(LIB) $(LIBFILE) /p:64 @winlibgu.lnk
!ifdef RAT
	$(LIB) $(LIBFILE) +mouseint;
	del *.bak
!endif
!ifdef MW
	$(LIB) $(LIBFILE) -winmouse;
	del *.bak
	$(LIB) $(LIBFILE) +wguimous;
	del *.bak
!endif
!ifdef GX
	$(LIB) $(LIBFILE) -winmouse;
	del *.bak
	$(LIB) $(LIBFILE) +wguimous;
	del *.bak
!ifdef GXPRINT
	$(LIB) $(LIBFILE) +wgxprint;
	del *.bak
!endif
!endif


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

MOUSEINT.OBJ :  MOUSEINT.ASM

WACTIVE.OBJ  :  WACTIVE.C

WADJUST.OBJ  :  WADJUST.C

WANYPOP.OBJ  :  WANYPOP.C

WARRICON.OBJ :  WARRICON.C

WBITMAP.OBJ  :  WBITMAP.C

WCHECKBX.OBJ :  WCHECKBX.C

WCLIPBRD.OBJ :  WCLIPBRD.C

WCOMFILE.OBJ :  WCOMFILE.C

WCOMFIND.OBJ :  WCOMFIND.C

WCREATE.OBJ  :  WCREATE.C

WCRECURS.OBJ :  WCRECURS.C

WCREICON.OBJ :  WCREICON.C

WDLGDIR.OBJ  :  WDLGDIR.C

WDLGOPEN.OBJ :  WDLGOPEN.C

WDRAWTXT.OBJ :  WDRAWTXT.C

WDRVTYPE.OBJ :  WDRVTYPE.C

WEDITPRO.OBJ :  WEDITPRO.C

WEMALLOC.OBJ :  WEMALLOC.C

WENUMFNT.OBJ :  WENUMFNT.C

WENUMOBJ.OBJ :  WENUMOBJ.C

WEXITWIN.OBJ :  WEXITWIN.C

WEXTTEXT.OBJ :  WEXTTEXT.C

WFILEIO.OBJ  :  WFILEIO.C

WFILETIT.OBJ :  WFILETIT.C

WFILLWIN.OBJ :  WFILLWIN.C

WFINDWIN.OBJ :  WFINDWIN.C

WGDICHAR.OBJ :  WGDICHAR.C

WGDICLIP.OBJ :  WGDICLIP.C

WGDIFONT.OBJ :  WGDIFONT.C

WGDIINV.OBJ  :  WGDIINV.C

WGDIMAP.OBJ  :  WGDIMAP.C

WGDIRGB.OBJ  :  WGDIRGB.C

WGDIRGN.OBJ  :  WGDIRGN.C

WGDISET.OBJ  :  WGDISET.C

WGDIDDA.OBJ  :  WGDIDDA.C

WGDISCRO.OBJ :  WGDISCRO.C

WGDISTUB.OBJ :  WGDISTUB.C

WGDISTRD.OBJ :  WGDISTRD.C

WGETCLAS.OBJ :  WGETCLAS.C

WGETPOS.OBJ  :  WGETPOS.C

WGRAYSTR.OBJ :  WGRAYSTR.C

WGXPRINT.OBJ :  WGXPRINT.C

WHFILEIO.OBJ :  WHFILEIO.C

WINACCEL.OBJ :  WINACCEL.C

WINALT.OBJ   :  WINALT.C

WINANSI.OBJ  :  WINANSI.C

WINATOM.OBJ  :  WINATOM.C

WINBLIT.OBJ  :  WINBLIT.C

WINBREAK.OBJ :  WINBREAK.C

WINCARET.OBJ :  WINCARET.C

WINCATCH.OBJ :  WINCATCH.C

WINCLASS.OBJ :  WINCLASS.C

WINCLICK.OBJ :  WINCLICK.C

WINCOLOR.OBJ :  WINCOLOR.C

WINCOMBO.OBJ :  WINCOMBO.C

WINCREAT.OBJ :  WINCREAT.C

WINDLG.OBJ   :  WINDLG.C

WINDC.OBJ    :  WINDC.C

WINDEBUG.OBJ :  WINDEBUG.C

WINDEFER.OBJ :  WINDEFER.C

WINDOS.OBJ   :  WINDOS.C

WINDRAW.OBJ  :  WINDRAW.C

WINDSTRY.OBJ :  WINDSTRY.C

WINEDIT.OBJ  :  WINEDIT.C

WINENABL.OBJ :  WINENABL.C

WINENUM.OBJ  :  WINENUM.C

WINEVENT.OBJ :  WINEVENT.C

WINEXEC.OBJ  :  WINEXEC.C

WINEXTRA.OBJ :  WINEXTRA.C

WINFLASH.OBJ :  WINFLASH.C

WINHOOK.OBJ  :  WINHOOK.C

WINICON.OBJ  :  WINICON.C

WININIT.OBJ  :  WININIT.C

WININT24.OBJ :  WININT24.C

WINKBD.OBJ   :  WINKBD.C

WINLBDIR.OBJ :  WINLBDIR.C

WINMDI.OBJ   :  WINMDI.C

WINMENU.OBJ  :  WINMENU.C

WINMENU2.OBJ :  WINMENU2.C

WINMODAL.OBJ :  WINMODAL.C

WINMONO.OBJ  :  WINMONO.C

WINMOUSE.OBJ :  WINMOUSE.C

WINMOVE.OBJ  :  WINMOVE.C

WINQUEUE.OBJ :  WINQUEUE.C

WINPOINT.OBJ :  WINPOINT.C

WINPRINT.OBJ :  WINPRINT.C

WINPROP.OBJ  :  WINPROP.C

WINRECT.OBJ  :  WINRECT.C

WINRES.OBJ   :  WINRES.C

WINRES2.OBJ  :  WINRES2.C

WINSCROL.OBJ :  WINSCROL.C

WINSET.OBJ   :  WINSET.C

WINSHOW.OBJ  :  WINSHOW.C

WINSIZE.OBJ  :  WINSIZE.C

WINSOUND.OBJ :  WINSOUND.C

WINSPAWN.OBJ :  WINSPAWN.ASM

WINTASK.OBJ  :  WINTASK.C

WINTERM.OBJ  :  WINTERM.C

WINTIME.OBJ  :  WINTIME.C

WINTIMER.OBJ :  WINTIMER.C

WINVID.OBJ   :  WINVID.C

WINZOOM.OBJ  :  WINZOOM.C

WISCHAR.OBJ  :  WISCHAR.C

WKBSTACK.OBJ :  WKBSTACK.C

WLIBRARY.OBJ :  WLIBRARY.C

WLISTBOX.OBJ :  WLISTBOX.C

WLNKLIST.OBJ :  WLNKLIST.C

WLOADACC.OBJ :  WLOADACC.C

WLOADDLG.OBJ :  WLOADDLG.C

WLOADMNU.OBJ :  WLOADMNU.C

WLOADSTR.OBJ :  WLOADSTR.C

WLPTODP.OBJ  :  WLPTODP.C

WLSTRING.OBJ :  WLSTRING.C

WMEMGLOB.OBJ :  WMEMGLOB.C

WMEMLOCL.OBJ :  WMEMLOCL.C

WMENUHI.OBJ  :  WMENUHI.C

WMESSAGE.OBJ :  WMESSAGE.C

WMFCSTUB.OBJ :  WMFCSTUB.C

WMNUCHNG.OBJ :  WMNUCHNG.C

WMNUINFO.OBJ :  WMNUINFO.C

WMOUAPI.OBJ  :  WMOUAPI.C

WMSGBOX.OBJ  :  WMSGBOX.C

WOBJECT.OBJ  :  WOBJECT.C

WOPENFIL.OBJ :  WOPENFIL.C

WPATBLT.OBJ  :  WPATBLT.C

WPLACEMT.OBJ :  WPLACEMT.C

WPRNTDLG.OBJ :  WPRNTDLG.C

WPROFILE.OBJ :  WPROFILE.C

WPUSHBUT.OBJ :  WPUSHBUT.C

WREADINI.OBJ :  WREADINI.C

WREFORM.OBJ  :  WREFORM.C

WREGION.OBJ  :  WREGION.C

WRGNFILL.OBJ :  WRGNFILL.C

WRGNFRAM.OBJ :  WRGNFRAM.C

WRGNINV.OBJ  :  WRGNINV.C

WRGNPAIN.OBJ :  WRGNPAIN.C

WRGNVAL.OBJ  :  WRGNVAL.C

WRUBBER.OBJ  :  WRUBBER.C

WSAVEDC.OBJ  :  WSAVEDC.C

WSCROLDC.OBJ :  WSCROLDC.C

WSCRLBAR.OBJ :  WSCRLBAR.C

WSELECTR.OBJ :  WSELECTR.C

WSETRECT.OBJ :  WSETRECT.C

WSETWPOS.OBJ :  WSETWPOS.C

WSPRINTF.OBJ :  WSPRINTF.C

WSRCHPTH.OBJ :  WSRCHPTH.C

WSTATIC.OBJ  :  WSTATIC.C

WSTDPROC.OBJ :  WSTDPROC.C

WSTFOCUS.OBJ :  WSTFOCUS.C

WSYSMENU.OBJ :  WSYSMENU.C

WTABTEXT.OBJ :  WTABTEXT.C

WTEXTMET.OBJ :  WTEXTMET.C

WTEXTOUT.OBJ :  WTEXTOUT.C

WUNREGCL.OBJ :  WUNREGCL.C

WZORDER.OBJ  :  WZORDER.C

XMSLIB.OBJ   :  XMSLIB.C


clean :
	del *.pdb
        del *.pch
        del *.err
        del *.obj
        del mewel??m.lib

