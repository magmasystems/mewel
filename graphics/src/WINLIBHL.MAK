############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  Metaware HighC version                                                  #
#                                                                          #
#  NOTE::: This makefile is equivalent to WINLIB, except that it uses      #
#          the "combined" files in order to reduce the number of segments  #
#          needed. (ie - CHECKBOX.C combines all of the CBxxx files).      #
#          Of course, you can overcome this by using the "named segment"   #
#          feature of some compilers.                                      #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C, H for HighC
COMPILER = H
# Model is M for Medium, L for Large, S for Small
MODEL = f

# HIGH C macros
CC = hc386
CFLAGS = @highc.opt
ASM = masm 
##LMODEL should be 1 if you want large data
#ASMDEBUG = /Zi
ASMFLAGS = /DLMODEL=1 /DTC=0 /mx $(ASMDEBUG) /DMODL=large

# We tack the compiler and model onto the end of the library name, so
# the library for Borland's Turbo C, medium model would be called WINLIBTM.LIB


# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.

.c.obj :
  $(CC) $(CFLAGS) $*.c
# $(LIB) winlib$(COMPILER)$(MODEL)-+$*.obj,nul;

.asm.obj :
  $(ASM) $(ASMFLAGS) $*;
# $(LIB) winlib$(COMPILER)$(MODEL)-+$*.obj,nul;


OBJS1 =      WACTIVE.OBJ  WADJUST.OBJ  WANYPOP.OBJ  WBITMAP.OBJ  WCHECKBX.OBJ WCLIPBRD.OBJ \
WCREATE.OBJ  WDLGDIR.OBJ  WDLGOPEN.OBJ WDRAWTXT.OBJ WDRVTYPE.OBJ WEDITPRO.OBJ \
WEDTDRAW.OBJ WEMALLOC.OBJ WENUMFNT.OBJ WENUMOBJ.OBJ WEXITWIN.OBJ WEXTTEXT.OBJ \
WFILEIO.OBJ  WFILLWIN.OBJ WFINDWIN.OBJ 

OBJS2 =      WGDICHAR.OBJ WGDICLIP.OBJ WGDIFONT.OBJ WGDIRECT.OBJ \
WGDIRGB.OBJ  WGDIRGN.OBJ  WGDISET.OBJ  WGDISHAP.OBJ WGDISTUB.OBJ WGETCLAS.OBJ WGRAYSTR.OBJ \
WINACCEL.OBJ WINANSI.OBJ  WINATOM.OBJ  WINBLIT.OBJ  WINBORDR.OBJ \
WINCARET.OBJ WINCATCH.OBJ WINCLASS.OBJ WINCLICK.OBJ WINCOLOR.OBJ \
WINCOMBO.OBJ WINCREAT.OBJ WINDOS.OBJ   WINDLG.OBJ   WINDC.OBJ    WINDEBUG.OBJ 

OBJS3 =      WINDEFER.OBJ WINDRAW.OBJ  WINDSTRY.OBJ WINEDIT.OBJ  WINENABL.OBJ \
WINENUM.OBJ  WINEVENT.OBJ WINEXEC.OBJ  WINEXTRA.OBJ WINFLASH.OBJ WINHOOK.OBJ  \
WINICON.OBJ  WININIT.OBJ  WINKBD.OBJ   WINLBDIR.OBJ WINMDI.OBJ   \
WINMENU.OBJ  WINMENU2.OBJ WINMODAL.OBJ WINMONO.OBJ  WINMOUSE.OBJ WINMOVE.OBJ  \
WINPOINT.OBJ WINPROP.OBJ  WINQUEUE.OBJ WINRECT.OBJ  WINRES.OBJ   WINRES2.OBJ

OBJS4 =      WINSCROL.OBJ WINSET.OBJ   WINSHOW.OBJ  WINSIZE.OBJ  WINSOUND.OBJ \
WINTASK.OBJ  WINTERM.OBJ  WINTIME.OBJ  WINTIMER.OBJ WINVID.OBJ   \
WINVIRT.OBJ  WINZOOM.OBJ  WISCHAR.OBJ  WKBSTACK.OBJ WLASTPOP.OBJ WLIBRARY.OBJ \
WLINETO.OBJ  WLISTBOX.OBJ WLNKLIST.OBJ WLOADACC.OBJ WLOADBMP.OBJ WLOADDLG.OBJ \
WLOADMNU.OBJ WLOADSTR.OBJ WLPTODP.OBJ  WLSTRING.OBJ WMEMGLOB.OBJ WMEMLOCL.OBJ \
WMENUHI.OBJ  WMESSAGE.OBJ WMNUCHNG.OBJ WMNUDRAW.OBJ WMNUINFO.OBJ WMOUAPI.OBJ  \
WMSGBOX.OBJ  WNOTIFY.OBJ

OBJS5 =      WOBJECT.OBJ  WOPENFIL.OBJ WPATBLT.OBJ  WPROFILE.OBJ WPUSHBUT.OBJ \
WREFORM.OBJ  WREFRESH.OBJ WRUBBER.OBJ  WSAVEDC.OBJ  WSCROLDC.OBJ WSCRLBAR.OBJ \
WSELECTR.OBJ WSETRECT.OBJ WSETWPOS.OBJ WSHADOW.OBJ  WSPRINTF.OBJ WSRCHPTH.OBJ WSTATIC.OBJ  \
WSTDPROC.OBJ WSTFOCUS.OBJ WSTRDISP.OBJ WSYSMENU.OBJ WTEXTMET.OBJ WTEXTOUT.OBJ \
WVISMAP.OBJ  WZORDER.OBJ

OBJS6 =      WCOMFILE.OBJ WCOMFIND.OBJ WDOS386.OBJ \
WKEYSTAT.OBJ WWINORG.OBJ  WWINEXT.OBJ  WVIEWORG.OBJ WVIEWEXT.OBJ \
WTABTEXT.OBJ WBITBLT.OBJ  WFILETIT.OBJ WUNREGCL.OBJ WGETPOS.OBJ

OBJS7 =      WINBREAK.OBJ WININT24.OBJ

all          :  $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)
	echo "Everything compiled, now building the library..."
	del winlibhl.lib
	386lib winlibhl -twocase -create @winlibhl.lnk
	echo "Library built. We are done!"

# The library WINLIBxx
winlib$(COMPILER)$(MODEL): $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5)
#  $(LIB) @winlib.lnk

WACTIVE.OBJ  :  WACTIVE.C
  $(CC) $(CFLAGS) $*.c

WADJUST.OBJ  :  WADJUST.C
  $(CC) $(CFLAGS) $*.c

WANYPOP.OBJ  :  WANYPOP.C
  $(CC) $(CFLAGS) $*.c

WBITBLT.OBJ  :  WBITBLT.C
  $(CC) $(CFLAGS) $*.c

WBITMAP.OBJ  :  WBITMAP.C
  $(CC) $(CFLAGS) $*.c

WCHECKBX.OBJ :  WCHECKBX.C
  $(CC) $(CFLAGS) $*.c

WCLIPBRD.OBJ :  WCLIPBRD.C
  $(CC) $(CFLAGS) $*.c

WCOMFILE.OBJ :  WCOMFILE.C
  $(CC) $(CFLAGS) $*.c

WCOMFIND.OBJ :  WCOMFIND.C
  $(CC) $(CFLAGS) $*.c

WCREATE.OBJ  :  WCREATE.C
  $(CC) $(CFLAGS) $*.c

WDLGDIR.OBJ  :  WDLGDIR.C
  $(CC) $(CFLAGS) $*.c

WDLGOPEN.OBJ :  WDLGOPEN.C
  $(CC) $(CFLAGS) $*.c

WDOS386.OBJ  :  WDOS386.C
  $(CC) $(CFLAGS) $*.c

WDRAWTXT.OBJ :  WDRAWTXT.C
  $(CC) $(CFLAGS) $*.c

WDRVTYPE.OBJ :  WDRVTYPE.C
  $(CC) $(CFLAGS) $*.c

WEDITPRO.OBJ :  WEDITPRO.C
  $(CC) $(CFLAGS) $*.c

WEDTDRAW.OBJ :  WEDTDRAW.C
  $(CC) $(CFLAGS) $*.c

WEMALLOC.OBJ :  WEMALLOC.C
  $(CC) $(CFLAGS) $*.c

WENUMFNT.OBJ :  WENUMFNT.C
  $(CC) $(CFLAGS) $*.c

WENUMOBJ.OBJ :  WENUMOBJ.C
  $(CC) $(CFLAGS) $*.c

WEXITWIN.OBJ :  WEXITWIN.C
  $(CC) $(CFLAGS) $*.c

WEXTTEXT.OBJ :  WEXTTEXT.C
  $(CC) $(CFLAGS) $*.c

WFILEIO.OBJ  :  WFILEIO.C
  $(CC) $(CFLAGS) $*.c

WFILETIT.OBJ :  WFILETIT.C
  $(CC) $(CFLAGS) $*.c

WFILLWIN.OBJ :  WFILLWIN.C
  $(CC) $(CFLAGS) $*.c

WFINDWIN.OBJ :  WFINDWIN.C
  $(CC) $(CFLAGS) $*.c

WGDICHAR.OBJ :  WGDICHAR.C
  $(CC) $(CFLAGS) $*.c

WGDICLIP.OBJ :  WGDICLIP.C
  $(CC) $(CFLAGS) $*.c

WGDIFONT.OBJ :  WGDIFONT.C
  $(CC) $(CFLAGS) $*.c

WGDIRECT.OBJ :  WGDIRECT.C
  $(CC) $(CFLAGS) $*.c

WGDIRGB.OBJ  :  WGDIRGB.C
  $(CC) $(CFLAGS) $*.c

WGDIRGN.OBJ  :  WGDIRGN.C
  $(CC) $(CFLAGS) $*.c

WGDISET.OBJ  :  WGDISET.C
  $(CC) $(CFLAGS) $*.c

WGDISHAP.OBJ :  WGDISHAP.C
  $(CC) $(CFLAGS) $*.c

WGDISTUB.OBJ :  WGDISTUB.C
  $(CC) $(CFLAGS) $*.c

WGETCLAS.OBJ :  WGETCLAS.C
  $(CC) $(CFLAGS) $*.c

WGETPOS.OBJ  :  WGETPOS.C
  $(CC) $(CFLAGS) $*.c

WGRAYSTR.OBJ :  WGRAYSTR.C
  $(CC) $(CFLAGS) $*.c

WINACCEL.OBJ :  WINACCEL.C
  $(CC) $(CFLAGS) $*.c

WINALT.OBJ   :  WINALT.C
  $(CC) $(CFLAGS) $*.c

WINANSI.OBJ  :  WINANSI.C
  $(CC) $(CFLAGS) $*.c

WINATOM.OBJ  :  WINATOM.C
  $(CC) $(CFLAGS) $*.c

WINBLIT.OBJ  :  WINBLIT.C
  $(CC) $(CFLAGS) $*.c

WINBORDR.OBJ :  WINBORDR.C
  $(CC) $(CFLAGS) $*.c

WINBREAK.OBJ :  WINBREAK.C
  $(CC) $(CFLAGS) $*.c

WINCARET.OBJ :  WINCARET.C
  $(CC) $(CFLAGS) $*.c

WINCATCH.OBJ :  WINCATCH.C
  $(CC) $(CFLAGS) $*.c

WINCLASS.OBJ :  WINCLASS.C
  $(CC) $(CFLAGS) $*.c

WINCLICK.OBJ :  WINCLICK.C
  $(CC) $(CFLAGS) $*.c

WINCOLOR.OBJ :  WINCOLOR.C
  $(CC) $(CFLAGS) $*.c

WINCOMBO.OBJ :  WINCOMBO.C
  $(CC) $(CFLAGS) $*.c

WINCREAT.OBJ :  WINCREAT.C
  $(CC) $(CFLAGS) $*.c

WINDLG.OBJ   :  WINDLG.C
  $(CC) $(CFLAGS) $*.c

WINDC.OBJ    :  WINDC.C
  $(CC) $(CFLAGS) $*.c

WINDEBUG.OBJ :  WINDEBUG.C
  $(CC) $(CFLAGS) $*.c

WINDEFER.OBJ :  WINDEFER.C
  $(CC) $(CFLAGS) $*.c

WINDOS.OBJ   :  WINDOS.C
  $(CC) $(CFLAGS) $*.c

WINDRAW.OBJ  :  WINDRAW.C
  $(CC) $(CFLAGS) $*.c

WINDSTRY.OBJ :  WINDSTRY.C
  $(CC) $(CFLAGS) $*.c

WINEDIT.OBJ  :  WINEDIT.C
  $(CC) $(CFLAGS) $*.c

WINENABL.OBJ :  WINENABL.C
  $(CC) $(CFLAGS) $*.c

WINENUM.OBJ  :  WINENUM.C
  $(CC) $(CFLAGS) $*.c

WINEVENT.OBJ :  WINEVENT.C
  $(CC) $(CFLAGS) $*.c

WINEXEC.OBJ  :  WINEXEC.C
  $(CC) $(CFLAGS) $*.c

WINEXTRA.OBJ :  WINEXTRA.C
  $(CC) $(CFLAGS) $*.c

WINFLASH.OBJ :  WINFLASH.C
  $(CC) $(CFLAGS) $*.c

WINHOOK.OBJ  :  WINHOOK.C
  $(CC) $(CFLAGS) $*.c

WINICON.OBJ  :  WINICON.C
  $(CC) $(CFLAGS) $*.c

WININIT.OBJ  :  WININIT.C
  $(CC) $(CFLAGS) $*.c

WININT24.OBJ :  WININT24.C
  $(CC) $(CFLAGS) $*.c

WINKBD.OBJ   :  WINKBD.C
  $(CC) $(CFLAGS) $*.c

WINLBDIR.OBJ :  WINLBDIR.C
  $(CC) $(CFLAGS) $*.c

WINMDI.OBJ   :  WINMDI.C
  $(CC) $(CFLAGS) $*.c

WINMENU.OBJ  :  WINMENU.C
  $(CC) $(CFLAGS) $*.c

WINMENU2.OBJ :  WINMENU2.C
  $(CC) $(CFLAGS) $*.c

WINMODAL.OBJ :  WINMODAL.C
  $(CC) $(CFLAGS) $*.c

WINMONO.OBJ  :  WINMONO.C
  $(CC) $(CFLAGS) $*.c

WINMOUSE.OBJ :  WINMOUSE.C
  $(CC) $(CFLAGS) $*.c

WINMOVE.OBJ  :  WINMOVE.C
  $(CC) $(CFLAGS) $*.c

WINQUEUE.OBJ :  WINQUEUE.C
  $(CC) $(CFLAGS) $*.c

WINPOINT.OBJ :  WINPOINT.C
  $(CC) $(CFLAGS) $*.c

WINPROP.OBJ  :  WINPROP.C
  $(CC) $(CFLAGS) $*.c

WINRECT.OBJ  :  WINRECT.C
  $(CC) $(CFLAGS) $*.c

WINRES.OBJ   :  WINRES.C
  $(CC) $(CFLAGS) $*.c

WINRES2.OBJ  :  WINRES2.C
  $(CC) $(CFLAGS) $*.c

WINSCROL.OBJ :  WINSCROL.C
  $(CC) $(CFLAGS) $*.c

WINSET.OBJ   :  WINSET.C
  $(CC) $(CFLAGS) $*.c

WINSHOW.OBJ  :  WINSHOW.C
  $(CC) $(CFLAGS) $*.c

WINSIZE.OBJ  :  WINSIZE.C
  $(CC) $(CFLAGS) $*.c

WINSOUND.OBJ :  WINSOUND.C
  $(CC) $(CFLAGS) $*.c

WINSPAWN.OBJ :  WINSPAWN.ASM
  $(CC) $(CFLAGS) $*.c

WINTASK.OBJ  :  WINTASK.C
  $(CC) $(CFLAGS) $*.c

WINTERM.OBJ  :  WINTERM.C
  $(CC) $(CFLAGS) $*.c

WINTIME.OBJ  :  WINTIME.C
  $(CC) $(CFLAGS) $*.c

WINTIMER.OBJ :  WINTIMER.C
  $(CC) $(CFLAGS) $*.c

WINVID.OBJ   :  WINVID.C
  $(CC) $(CFLAGS) $*.c

WINVIRT.OBJ  :  WINVIRT.C
  $(CC) $(CFLAGS) $*.c

WINZOOM.OBJ  :  WINZOOM.C
  $(CC) $(CFLAGS) $*.c

WISCHAR.OBJ  :  WISCHAR.C
  $(CC) $(CFLAGS) $*.c

WKBSTACK.OBJ :  WKBSTACK.C
  $(CC) $(CFLAGS) $*.c

WKEYSTAT.OBJ :  WKEYSTAT.C
  $(CC) $(CFLAGS) $*.c

WLASTPOP.OBJ :  WLASTPOP.C
  $(CC) $(CFLAGS) $*.c

WLIBRARY.OBJ :  WLIBRARY.C
  $(CC) $(CFLAGS) $*.c

WLINETO.OBJ  :  WLINETO.C
  $(CC) $(CFLAGS) $*.c

WLISTBOX.OBJ :  WLISTBOX.C
  $(CC) $(CFLAGS) $*.c

WLNKLIST.OBJ :  WLNKLIST.C
  $(CC) $(CFLAGS) $*.c

WLOADACC.OBJ :  WLOADACC.C
  $(CC) $(CFLAGS) $*.c

WLOADBMP.OBJ :  WLOADBMP.C
  $(CC) $(CFLAGS) $*.c

WLOADDLG.OBJ :  WLOADDLG.C
  $(CC) $(CFLAGS) $*.c

WLOADMNU.OBJ :  WLOADMNU.C
  $(CC) $(CFLAGS) $*.c

WLOADSTR.OBJ :  WLOADSTR.C
  $(CC) $(CFLAGS) $*.c

WLPTODP.OBJ  :  WLPTODP.C
  $(CC) $(CFLAGS) $*.c

WLSTRING.OBJ :  WLSTRING.C
  $(CC) $(CFLAGS) $*.c

WMEMGLOB.OBJ :  WMEMGLOB.C
  $(CC) $(CFLAGS) $*.c

WMEMLOCL.OBJ :  WMEMLOCL.C
  $(CC) $(CFLAGS) $*.c

WMENUHI.OBJ  :  WMENUHI.C
  $(CC) $(CFLAGS) $*.c

WMESSAGE.OBJ :  WMESSAGE.C
  $(CC) $(CFLAGS) $*.c

WMNUCHNG.OBJ :  WMNUCHNG.C
  $(CC) $(CFLAGS) $*.c

WMNUDRAW.OBJ :  WMNUDRAW.C
  $(CC) $(CFLAGS) $*.c

WMNUINFO.OBJ :  WMNUINFO.C
  $(CC) $(CFLAGS) $*.c

WMOUAPI.OBJ  :  WMOUAPI.C
  $(CC) $(CFLAGS) $*.c

WMSGBOX.OBJ  :  WMSGBOX.C
  $(CC) $(CFLAGS) $*.c

WNOTIFY.OBJ  :  WNOTIFY.C
  $(CC) $(CFLAGS) $*.c

WOBJECT.OBJ  :  WOBJECT.C
  $(CC) $(CFLAGS) $*.c

WOPENFIL.OBJ :  WOPENFIL.C
  $(CC) $(CFLAGS) $*.c

WPATBLT.OBJ  :  WPATBLT.C
  $(CC) $(CFLAGS) $*.c

WPROFILE.OBJ :  WPROFILE.C
  $(CC) $(CFLAGS) $*.c

WPUSHBUT.OBJ :  WPUSHBUT.C
  $(CC) $(CFLAGS) $*.c

WREFORM.OBJ  :  WREFORM.C
  $(CC) $(CFLAGS) $*.c

WREFRESH.OBJ :  WREFRESH.C
  $(CC) $(CFLAGS) $*.c

WRUBBER.OBJ  :  WRUBBER.C
  $(CC) $(CFLAGS) $*.c

WSAVEDC.OBJ  :  WSAVEDC.C
  $(CC) $(CFLAGS) $*.c

WSCROLDC.OBJ :  WSCROLDC.C
  $(CC) $(CFLAGS) $*.c

WSCRLBAR.OBJ :  WSCRLBAR.C
  $(CC) $(CFLAGS) $*.c

WSELECTR.OBJ :  WSELECTR.C
  $(CC) $(CFLAGS) $*.c

WSETRECT.OBJ :  WSETRECT.C
  $(CC) $(CFLAGS) $*.c

WSETWPOS.OBJ :  WSETWPOS.C
  $(CC) $(CFLAGS) $*.c

WSHADOW.OBJ  :  WSHADOW.C
  $(CC) $(CFLAGS) $*.c

WSPRINTF.OBJ :  WSPRINTF.C
  $(CC) $(CFLAGS) $*.c

WSRCHPTH.OBJ :  WSRCHPTH.C
  $(CC) $(CFLAGS) $*.c

WSTATIC.OBJ  :  WSTATIC.C
  $(CC) $(CFLAGS) $*.c

WSTDPROC.OBJ :  WSTDPROC.C
  $(CC) $(CFLAGS) $*.c

WSTFOCUS.OBJ :  WSTFOCUS.C
  $(CC) $(CFLAGS) $*.c

WSTRDISP.OBJ :  WSTRDISP.C
  $(CC) $(CFLAGS) $*.c

WSYSMENU.OBJ :  WSYSMENU.C
  $(CC) $(CFLAGS) $*.c

WTABTEXT.OBJ :  WTABTEXT.C
  $(CC) $(CFLAGS) $*.c

WTEXTMET.OBJ :  WTEXTMET.C
  $(CC) $(CFLAGS) $*.c

WTEXTOUT.OBJ :  WTEXTOUT.C
  $(CC) $(CFLAGS) $*.c

WUNREGCL.OBJ :  WUNREGCL.C
  $(CC) $(CFLAGS) $*.c

WVISMAP.OBJ  :  WVISMAP.C
  $(CC) $(CFLAGS) $*.c

WVIEWEXT.OBJ :  WVIEWEXT.C
  $(CC) $(CFLAGS) $*.c

WVIEWORG.OBJ :  WVIEWORG.C
  $(CC) $(CFLAGS) $*.c

WWINEXT.OBJ  :  WWINEXT.C
  $(CC) $(CFLAGS) $*.c

WWINORG.OBJ  :  WWINORG.C
  $(CC) $(CFLAGS) $*.c

WZORDER.OBJ  :  WZORDER.C
  $(CC) $(CFLAGS) $*.c

