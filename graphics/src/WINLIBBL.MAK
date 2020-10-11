############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  Borland C++ Text version                                                #
#                                                                          #
#  Usage :                                                                 #
#                                                                          #
#     make [-DPL] [-DD] -f winlibbl.mak                                    #
#                                                                          #
#   where  -DD builds a debugging version                                  #
#          -DPL builds a version for Pharlap 286 Extender                  #
#          -DDP16 builds a version for DPMI16                              #
#          -DDP32 builds a version for DPMI32                              #
#                                                                          #
#   The default is for an optimized, real-mode version.                    #
#                                                                          #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = T
# Model is M for Medium, L for Large, S for Small
MODEL = -ml
!ifdef DP32
MODEL =
!endif

#
# These are two hard-coded directory names which you need to change
#
# Root directory for MEWEL
# Root directory of the Borland compiler
#
MEWELPATH=\mewel
BCPATH=\bc4

#
# Specify the compiler to use
#
CC=bcc
!ifdef DP32
CC=bcc32
!endif

#
# Optimization : -G favors speed over size, -O improves jumps & loops
#
!ifdef D
DEBUG=-v
LIBFILE=winlibbl
!else
OPT=-G -O
LIBFILE=winlibbl
!endif

DEFINES=-DDOS

#
# Specify your include path for the Pharlap 286 H files
#
!ifdef PL
INCPATH=$(INCPATH) -Ic:\run286\bc3\include
!endif

!ifdef ZAPP
DEFINES=$(DEFINES) -DZAPP
!endif

#
# Specify overlay flags
#
OVL=-Yo
PREPOVL=-Y
!ifdef DP32
OVL=
PREPOVL=-WX
!endif
!ifdef DP16
OVL=
PREPOVL=-WX
!endif
!ifdef PL
OVL=
PREPOVL=
!endif

CFLAGS=$(OPT) $(DEBUG) -c -d -f- -k -N -K  $(PREPOVL) $(MODEL) $(DEFINES) -I$(MEWELPATH) -I$(BCPATH)\include $(INCPATH)

#
# Add -DDOS286 and -2 for Pharlap
#
!ifdef PL
CFLAGS=$(CFLAGS) -DDOS286X -2
!endif

!ifdef ZAPP
CFLAGS=$(CFLAGS) -DZAPP
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


# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.

.c.obj :
  $(CC) $(CFLAGS) $*.c

.asm.obj :
  $(ASM) $(ASMFLAGS) $*;


OBJS1=\
WACTIVE.OBJ  \
WADJUST.OBJ  \
WANYPOP.OBJ  \
WARRICON.OBJ \
WBITBLT.OBJ  \
WBITMAP.OBJ  \
WCHECKBX.OBJ \
WCLIPBRD.OBJ \
WCOMFILE.OBJ \
WCOMFIND.OBJ \
WCREATDC.OBJ \
WCREATE.OBJ  \
WDESKTOP.OBJ \
WDLGCTID.OBJ \
WDLGDIR.OBJ  \
WDLGOPEN.OBJ \
WDRAWTXT.OBJ \
WDRVTYPE.OBJ \
WEDITPRO.OBJ \
WEDTDRAW.OBJ \
WEMALLOC.OBJ \
WENUMFNT.OBJ \
WENUMOBJ.OBJ \
WEXITWIN.OBJ \
WEXTTEXT.OBJ \
WFILEIO.OBJ  \
WFILETIT.OBJ \
WFILLWIN.OBJ \
WFINDWIN.OBJ \
WGDICHAR.OBJ \
WGDICLIP.OBJ \
WGDIFONT.OBJ \
WGDIRECT.OBJ \
WGDIRGB.OBJ  \
WGDIRGN.OBJ  \
WGDISET.OBJ  \
WGDISHAP.OBJ \
WGDISTUB.OBJ \
WGETCLAS.OBJ \
WGETPOS.OBJ  \
WGRAYSTR.OBJ \
WHFILEIO.OBJ \
WINACCEL.OBJ \
WINANSI.OBJ  \
WINATOM.OBJ  \
WINATOM2.OBJ \
WINBLIT.OBJ  \
WINBORDR.OBJ \
WINCARET.OBJ \
WINCLASS.OBJ \
WINCLICK.OBJ \
WINCOLOR.OBJ \
WINCOMBO.OBJ \
WINCREAT.OBJ \
WINDATA.OBJ  \
WINDC.OBJ    \
WINDLG.OBJ   \
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
WINICON.OBJ  \
WININIT.OBJ  \
WINKBD.OBJ   \
WINLBDIR.OBJ \
WINMDI.OBJ   \
WINMENU.OBJ  \
WINMENU2.OBJ \
WINMODAL.OBJ \
WINMONO.OBJ  \
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
WINVIRT.OBJ  \
WINZOOM.OBJ  \
WISCHAR.OBJ  \
WKBSTACK.OBJ \
WKEYSTAT.OBJ \
WLASTPOP.OBJ \
WLINETO.OBJ  \
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
WMENUBMP.OBJ \
WMENUHI.OBJ  \
WMESSAGE.OBJ \
WMFCSTUB.OBJ \
WMNUCHNG.OBJ \
WMNUDRAW.OBJ \
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
WPROCINS.OBJ \
WPROFILE.OBJ \
WPUSHBUT.OBJ \
WREFORM.OBJ  \
WREFRESH.OBJ \
WRGNFILL.OBJ \
WRGNFRAM.OBJ \
WRGNINV.OBJ  \
WRGNPAIN.OBJ \
WRGNVAL.OBJ  \
WRUBBER.OBJ  \
WSAVEDC.OBJ  \
WSCROLDC.OBJ \
WSCRLBAR.OBJ \
WSETRECT.OBJ \
WSETWPOS.OBJ \
WSHADOW.OBJ  \
WSHOWPOP.OBJ \
WSPRINTF.OBJ \
WSPY.OBJ     \
WSRCHPTH.OBJ \
WSTATIC.OBJ  \
WSTDPROC.OBJ \
WSTFOCUS.OBJ \
WSTRDISP.OBJ \
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
WVISMAP.OBJ  \
WVIEWEXT.OBJ \
WVIEWORG.OBJ \
WVIEWSCL.OBJ \
WWINEXT.OBJ  \
WWINORG.OBJ  \
WWINSCL.OBJ  \
WZORDER.OBJ

# OBJS7 has files which are not used for the protected-mode build
OBJS7 =      WVGAMAP.OBJ
!ifdef PL
OBJS7 =
!endif
!ifdef DP16
OBJS7 =
!endif
!ifdef DP32
OBJS7 =
!endif

#
# Some real-mode-only files. When building for protected mode, you'll
# probably get benign warnings (not errors) from the linker.
#
OBJSMOUSE = WINMOUSE.OBJ
!ifdef DP32
OBJS32BIT = WDOS386.OBJ  WDPMI.OBJ
!else
#
# Real mode or 16-bit protected mode
#
OBJSASM   = DELAY.OBJ    KEY.OBJ      LCGACPY.OBJ  LMEMCHR.OBJ  LMEMMOVE.OBJ \
            LWMEMSET.OBJ LMEMCPY.OBJ  LSTRING.OBJ  MOUSESUB.OBJ RECT.OBJ     \
            VBTSHLP.OBJ
OBJSEXEC  = WINSPAWN.OBJ CHECKPAT.OBJ EXEC.OBJ
OBJSHARD  = WINSOUND.OBJ WVGAWRT.OBJ
OBJSINTR  = WINALT.OBJ   WINBREAK.OBJ WININT24.OBJ
!ifndef DP16
#
# Real mode only
#
OBJSMEM   = WMEMGLOB.OBJ WMEMLOCL.OBJ
OBJSREAL  = WINDOS.OBJ
OBJSXMS   = XMSLIB.OBJ
!endif
!endif

#
# DP16 and DP32 have DLL capabilities, so we don't need MEWEL's
# stubs for Load/FreeLibrary and GetProcAddress
#
!if !$d(DP16) && !$d(DP32)
OBJSNDLL  = WINCATCH.OBJ WINDEBUG.OBJ WLIBRARY.OBJ WSELECTR.OBJ 
!endif


all :  $(OBJS1) $(OBJS7) \
$(OBJSREAL) $(OBJSMEM) $(OBJS32BIT) $(OBJSASM) $(OBJSEXEC)  \
$(OBJSHARD) $(OBJSINTR) $(OBJSMOUSE) $(OBJSXMS) $(OBJSNDLL)
	del $(LIBFILE).lib
	tlib /C /P32 $(LIBFILE) @winlib.lnk
!ifdef DP32
  $(LIB) /C /P32 $(LIBFILE) +wdos386 +wdpmi
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

RECT.OBJ     :  RECT.ASM

VBTSHLP.OBJ  :  VBTSHLP.ASM

WACTIVE.OBJ  :  WACTIVE.C

WADJUST.OBJ  :  WADJUST.C

WANYPOP.OBJ  :  WANYPOP.C

WARRICON.OBJ :  WARRICON.C

WBITMAP.OBJ  :  WBITMAP.C

WCHECKBX.OBJ :  WCHECKBX.C

WCLIPBRD.OBJ :  WCLIPBRD.C

WCREATDC.OBJ :  WCREATDC.C

WCREATE.OBJ  :  WCREATE.C

WDESKTOP.OBJ :  WDESKTOP.C

WDLGDIR.OBJ  :  WDLGDIR.C

WDLGOPEN.OBJ :  WDLGOPEN.C

WDRAWTXT.OBJ :  WDRAWTXT.C

WDRVTYPE.OBJ :  WDRVTYPE.C

WEDITPRO.OBJ :  WEDITPRO.C

WEDTDRAW.OBJ :  WEDTDRAW.C

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

WGDIMAP.OBJ  :  WGDIMAP.C

WGDIRECT.OBJ :  WGDIRECT.C

WGDIRGB.OBJ  :  WGDIRGB.C

WGDIRGN.OBJ  :  WGDIRGN.C

WGDISET.OBJ  :  WGDISET.C

WGDISHAP.OBJ :  WGDISHAP.C

WGDISTUB.OBJ :  WGDISTUB.C

WGETCLAS.OBJ :  WGETCLAS.C

WGETPOS.OBJ  :  WGETPOS.C

WGRAYSTR.OBJ :  WGRAYSTR.C

WHFILEIO.OBJ :  WHFILEIO.C

WINACCEL.OBJ :  WINACCEL.C

WINALT.OBJ   :  WINALT.C

WINANSI.OBJ  :  WINANSI.C

WINATOM.OBJ  :  WINATOM.C

WINBLIT.OBJ  :  WINBLIT.C

WINBORDR.OBJ :  WINBORDR.C

WINBREAK.OBJ :  WINBREAK.C

WINCARET.OBJ :  WINCARET.C

WINCATCH.OBJ :  WINCATCH.C

WINCLASS.OBJ :  WINCLASS.C

WINCLICK.OBJ :  WINCLICK.C

WINCOLOR.OBJ :  WINCOLOR.C

WINCOMBO.OBJ :  WINCOMBO.C

WINCREAT.OBJ :  WINCREAT.C

WINDATA.OBJ  :  WINDATA.C

WINDC.OBJ    :  WINDC.C

WINDEBUG.OBJ :  WINDEBUG.C

WINDEFER.OBJ :  WINDEFER.C

WINDLG.OBJ   :  WINDLG.C

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

WINRES3.OBJ  :  WINRES3.C

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

WINVIRT.OBJ  :  WINVIRT.C

WINZOOM.OBJ  :  WINZOOM.C

WISCHAR.OBJ  :  WISCHAR.C

WKBSTACK.OBJ :  WKBSTACK.C

WKEYSTAT.OBJ :  WKEYSTAT.C

WLIBRARY.OBJ :  WLIBRARY.C

WLINETO.OBJ  :  WLINETO.C

WLISTBOX.OBJ :  WLISTBOX.C

WLNKLIST.OBJ :  WLNKLIST.C

WLOADACC.OBJ :  WLOADACC.C

WLOADBMP.OBJ :  WLOADBMP.C

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

WMODULE.OBJ  :  WMODULE.C

WMOUAPI.OBJ  :  WMOUAPI.C

WMSGBOX.OBJ  :  WMSGBOX.C

WOBJECT.OBJ  :  WOBJECT.C

WOPENFIL.OBJ :  WOPENFIL.C

WPATBLT.OBJ  :  WPATBLT.C

WPROCINS.OBJ :  WPROCINS.C

WPROFILE.OBJ :  WPROFILE.C

WPUSHBUT.OBJ :  WPUSHBUT.C

WREFORM.OBJ  :  WREFORM.C

WRUBBER.OBJ  :  WRUBBER.C

WSAVEDC.OBJ  :  WSAVEDC.C

WSCROLDC.OBJ :  WSCROLDC.C

WSCRLBAR.OBJ :  WSCRLBAR.C

WSELECTR.OBJ :  WSELECTR.C

WSETRECT.OBJ :  WSETRECT.C

WSETWPOS.OBJ :  WSETWPOS.C

WSHADOW.OBJ  :  WSHADOW.C

WSPRINTF.OBJ :  WSPRINTF.C

WSTATIC.OBJ  :  WSTATIC.C

WSTDPROC.OBJ :  WSTDPROC.C

WSTFOCUS.OBJ :  WSTFOCUS.C

WSTRDISP.OBJ :  WSTRDISP.C

WSYSMENU.OBJ :  WSYSMENU.C

WTEXTMET.OBJ :  WTEXTMET.C

WTEXTOUT.OBJ :  WTEXTOUT.C

WUNREGCL.OBJ :  WUNREGCL.C

WVGAMAP.OBJ  :  WVGAMAP.C

WVGAWRT.OBJ  :  WVGAWRT.ASM

WVISMAP.OBJ  :  WVISMAP.C

WZORDER.OBJ  :  WZORDER.C


clean :
	del *.pdb
        del *.pch
        del *.err
        del *.obj
        del winlibbl.lib

