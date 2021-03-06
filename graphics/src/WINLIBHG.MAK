############################################################################
#                                                                          #
#  MAKEFILE for the MEWEL windowing system.                                #
#                                                                          #
#  This is for the High-C 386 DOS GUI version                              #
#                                                                          #
############################################################################

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = H
# Model is M for Medium, L for Large, S for Small
MODEL = f

MEWELPATH=\mewel

CC = hc386

DBG = -g
CFLAGS = -DMEWEL_GUI $(DBG) @highc.opt

# We tack the compiler and model onto the end of the library name, so
# the library for Borland's Turbo C, medium model would be called WINLIBTM.LIB


# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.

.c.obj :
	$(CC) $(CFLAGS) $*.c
# $(LIB) winlib$(COMPILER)$(MODEL)-+$*.obj,nul

.asm.obj :
  $(ASM) $(ASMFLAGS) $*;
# $(LIB) winlib$(COMPILER)$(MODEL)-+$*.obj,nul;


OBJS1 =      WACTIVE.OBJ  WADJUST.OBJ  WANYPOP.OBJ  WBITBLT.OBJ  WBITMAP.OBJ  \
WCHECKBX.OBJ WCLIPBRD.OBJ WCREATE.OBJ  WDESKTOP.OBJ WDLGDIR.OBJ  WDLGOPEN.OBJ \
WDRAWTXT.OBJ WDRVTYPE.OBJ WEDITPRO.OBJ WEDTDRAW.OBJ WEMALLOC.OBJ WENUMFNT.OBJ \
WENUMOBJ.OBJ WEXITWIN.OBJ WEXTTEXT.OBJ WFILEIO.OBJ  WFILLWIN.OBJ WFINDWIN.OBJ 

OBJS2 =      WGDICHAR.OBJ WGDICLIP.OBJ WGDIFONT.OBJ WGDIRGB.OBJ  WGDIRGN.OBJ  \
WGDISET.OBJ  WGDISHAP.OBJ WGDISTUB.OBJ WGETCLAS.OBJ WGRAYSTR.OBJ WINACCEL.OBJ \
WINANSI.OBJ  WINATOM.OBJ  WINBLIT.OBJ  WINCARET.OBJ WINCATCH.OBJ WINCLASS.OBJ \
WINCLICK.OBJ WINCOLOR.OBJ WINCOMBO.OBJ WINCREAT.OBJ WINDATA.OBJ  WINDOS.OBJ   \
WINDLG.OBJ   WINDC.OBJ    WINDEBUG.OBJ 

OBJS3 =      WINDEFER.OBJ WINDRAW.OBJ  WINDSTRY.OBJ WINEDIT.OBJ  WINENABL.OBJ \
WINENUM.OBJ  WINEVENT.OBJ WINEXEC.OBJ  WINEXTRA.OBJ WINFLASH.OBJ WINHOOK.OBJ  \
WGUIICON.OBJ WININIT.OBJ  WINKBD.OBJ   WINLBDIR.OBJ WINMDI.OBJ   \
WINMENU.OBJ  WINMENU2.OBJ WINMODAL.OBJ WINMONO.OBJ  WINMOUSE.OBJ WINMOVE.OBJ  \
WINPOINT.OBJ WINPROP.OBJ  WINQUEUE.OBJ WINRECT.OBJ  WINRES.OBJ   

OBJS4 =      WINSCROL.OBJ WINSET.OBJ   WINSHOW.OBJ  WINSIZE.OBJ  WINSOUND.OBJ \
WINTASK.OBJ  WINTERM.OBJ  WINTIME.OBJ  WINTIMER.OBJ WINVID.OBJ   \
WINZOOM.OBJ  WISCHAR.OBJ  WKBSTACK.OBJ WLIBRARY.OBJ \
WLISTBOX.OBJ WLNKLIST.OBJ WLOADACC.OBJ WLOADBMP.OBJ WLOADDLG.OBJ WLOADMNU.OBJ \
WLOADSTR.OBJ WLPTODP.OBJ  WLSTRING.OBJ WMEMGLOB.OBJ WMEMLOCL.OBJ WMENUHI.OBJ  \
WMESSAGE.OBJ WMNUCHNG.OBJ WMNUINFO.OBJ WMOUAPI.OBJ  WMSGBOX.OBJ  WMULDIV.OBJ

OBJS5 =      WOBJECT.OBJ  WOPENFIL.OBJ WPATBLT.OBJ  WPROFILE.OBJ WPUSHBUT.OBJ \
WREFORM.OBJ  WINRES2.OBJ  WRUBBER.OBJ  WSAVEDC.OBJ  WSCROLDC.OBJ WSCRLBAR.OBJ \
WSELECTR.OBJ WSETRECT.OBJ WSETWPOS.OBJ WSPRINTF.OBJ WSRCHPTH.OBJ \
WSTATIC.OBJ  WSTDPROC.OBJ WSTFOCUS.OBJ WSYSMENU.OBJ WTABTEXT.OBJ \
WTEXTMET.OBJ WTEXTOUT.OBJ WZORDER.OBJ  \
WVIEWEXT.OBJ WVIEWORG.OBJ WVIEWSCL.OBJ WWINEXT.OBJ  WWINORG.OBJ  WWINSCL.OBJ

OBJS6 =      WCOMFILE.OBJ WCOMFIND.OBJ WREFRESH.OBJ WLOADBMP.OBJ \
WNOTIFY.OBJ  WGRAPHIC.OBJ WGUI.OBJ     WGUICURS.OBJ WGUIPCX.OBJ  \
WLASTPOP.OBJ WGUIBMP.OBJ  WGUIFONT.OBJ WGDIFLOD.OBJ WCRECURS.OBJ WGDIPOLY.OBJ \
WGDIPLIN.OBJ WGDIGPIX.OBJ WGDISPIX.OBJ WGDIELLI.OBJ WGDIPIE.OBJ  WREGION.OBJ  \
WREADINI.OBJ WGDIARC.OBJ  WSTRETCH.OBJ WKEYSTAT.OBJ WGDIFILL.OBJ WGDILINE.OBJ \
WGDITEXT.OBJ WGUIMENU.OBJ WCREATDC.OBJ WGDIPAL.OBJ  WGDIFRAM.OBJ \
WINPRINT.OBJ WGDIGDIB.OBJ WGDISDIB.OBJ WGUIDATA.OBJ WFILETIT.OBJ WCREICON.OBJ \
WARRICON.OBJ WGDISTRD.OBJ WPLACEMT.OBJ WMFCSTUB.OBJ WUNREGCL.OBJ WGDISBIT.OBJ \
WGDIINV.OBJ  WGETPOS.OBJ  WPROCINS.OBJ WGDISCRO.OBJ WINRES3.OBJ  WMODULE.OBJ  \
WCHOFONT.OBJ WGDIBRUS.OBJ WRGBFILL.OBJ


OBJS7 =      WDOS386.OBJ  WGUIMOUS.OBJ

all          :  $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS7)
	echo "Everything compiled, now building the library..."
	del winlibhg.lib
	386lib winlibhg -twocase -create @winlibhg.lnk
	echo "Library built. We are done!"

# The library WINLIBxx
winlib$(COMPILER)$(MODEL): $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)
#  $(LIB) @winlib.lnk

WACTIVE.OBJ  :  WACTIVE.C
  $(CC) $(CFLAGS) $*.c

WADJUST.OBJ  :  WADJUST.C
  $(CC) $(CFLAGS) $*.c

WANYPOP.OBJ  :  WANYPOP.C
  $(CC) $(CFLAGS) $*.c

WARRICON.OBJ :  WARRICON.C
  $(CC) $(CFLAGS) $*.c

WBITBLT.OBJ  :  WBITBLT.C
  $(CC) $(CFLAGS) $*.c

WBITMAP.OBJ  :  WBITMAP.C
  $(CC) $(CFLAGS) $*.c

WCHECKBX.OBJ :  WCHECKBX.C
  $(CC) $(CFLAGS) $*.c

WCHOFONT.OBJ :  WCHOFONT.C
  $(CC) $(CFLAGS) $*.c

WCLIPBRD.OBJ :  WCLIPBRD.C
  $(CC) $(CFLAGS) $*.c

WCOMFILE.OBJ :  WCOMFILE.C
  $(CC) $(CFLAGS) $*.c

WCOMFIND.OBJ :  WCOMFIND.C
  $(CC) $(CFLAGS) $*.c

WCREATDC.OBJ :  WCREATDC.C
  $(CC) $(CFLAGS) $*.c

WCREATE.OBJ  :  WCREATE.C
  $(CC) $(CFLAGS) $*.c

WCRECURS.OBJ :  WCRECURS.C
  $(CC) $(CFLAGS) $*.c

WCREICON.OBJ :  WCREICON.C
  $(CC) $(CFLAGS) $*.c

WDESKTOP.OBJ :  WDESKTOP.C
  $(CC) $(CFLAGS) $*.c

WDLGDIR.OBJ :  WDLGDIR.C
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

WGDIARC.OBJ  :  WGDIARC.C
  $(CC) $(CFLAGS) $*.c

WGDIBRUS.OBJ :  WGDIBRUS.C
  $(CC) $(CFLAGS) $*.c

WGDICHAR.OBJ :  WGDICHAR.C
  $(CC) $(CFLAGS) $*.c

WGDICLIP.OBJ :  WGDICLIP.C
  $(CC) $(CFLAGS) $*.c

WGDIELLI.OBJ :  WGDIELLI.C
  $(CC) $(CFLAGS) $*.c

WGDIFILL.OBJ :  WGDIFILL.C
  $(CC) $(CFLAGS) $*.c

WGDIFLOD.OBJ :  WGDIFLOD.C
  $(CC) $(CFLAGS) $*.c

WGDIFONT.OBJ :  WGDIFONT.C
  $(CC) $(CFLAGS) $*.c

WGDIFRAM.OBJ :  WGDIFRAM.C
  $(CC) $(CFLAGS) $*.c

WGDIGDIB.OBJ :  WGDIGDIB.C
  $(CC) $(CFLAGS) $*.c

WGDIGPIX.OBJ :  WGDIGPIX.C
  $(CC) $(CFLAGS) $*.c

WGDIINV.OBJ :   WGDIINV.C
  $(CC) $(CFLAGS) $*.c

WGDILINE.OBJ :  WGDILINE.C
  $(CC) $(CFLAGS) $*.c

WGDIPAL.OBJ  :  WGDIPAL.C
  $(CC) $(CFLAGS) $*.c

WGDIPIE.OBJ  :  WGDIPIE.C
  $(CC) $(CFLAGS) $*.c

WGDIPLIN.OBJ :  WGDIPLIN.C
  $(CC) $(CFLAGS) $*.c

WGDIPOLY.OBJ :  WGDIPOLY.C
  $(CC) $(CFLAGS) $*.c

WGDIRGB.OBJ  :  WGDIRGB.C
  $(CC) $(CFLAGS) $*.c

WGDIRGN.OBJ  :  WGDIRGN.C
  $(CC) $(CFLAGS) $*.c

WGDISBIT.OBJ :  WGDISBIT.C
  $(CC) $(CFLAGS) $*.c

WGDISCRO.OBJ :  WGDISCRO.C
  $(CC) $(CFLAGS) $*.c

WGDISDIB.OBJ :  WGDISDIB.C
  $(CC) $(CFLAGS) $*.c

WGDISET.OBJ  :  WGDISET.C
  $(CC) $(CFLAGS) $*.c

WGDISHAP.OBJ :  WGDISHAP.C
  $(CC) $(CFLAGS) $*.c

WGDISPIX.OBJ :  WGDISPIX.C
  $(CC) $(CFLAGS) $*.c

WGDISTRD.OBJ :  WGDISTRD.C
  $(CC) $(CFLAGS) $*.c

WGDISTUB.OBJ :  WGDISTUB.C
  $(CC) $(CFLAGS) $*.c

WGDITEXT.OBJ :  WGDITEXT.C
  $(CC) $(CFLAGS) $*.c

WGETCLAS.OBJ :  WGETCLAS.C
  $(CC) $(CFLAGS) $*.c

WGETPOS.OBJ  :  WGETPOS.C
  $(CC) $(CFLAGS) $*.c

WGRAPHIC.OBJ :  WGRAPHIC.C
  $(CC) $(CFLAGS) $*.c

WGRAYSTR.OBJ :  WGRAYSTR.C
  $(CC) $(CFLAGS) $*.c

WGUI.OBJ :  WGUI.C
  $(CC) $(CFLAGS) $*.c

WGUIBMP.OBJ :  WGUIBMP.C
  $(CC) $(CFLAGS) $*.c

WGUICURS.OBJ :  WGUICURS.C
  $(CC) $(CFLAGS) $*.c

WGUIDATA.OBJ :  WGUIDATA.C
  $(CC) $(CFLAGS) $*.c

WGUIFONT.OBJ :  WGUIFONT.C
  $(CC) $(CFLAGS) $*.c

WGUIICON.OBJ :  WGUIICON.C
  $(CC) $(CFLAGS) $*.c

WGUIMENU.OBJ :  WGUIMENU.C
  $(CC) $(CFLAGS) $*.c

WGUIMOUS.OBJ :  WGUIMOUS.C
  $(CC) $(CFLAGS) $*.c

WGUIPCX.OBJ  :  WGUIPCX.C
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

WINDATA.OBJ   :  WINDATA.C
  $(CC) $(CFLAGS) $*.c

WINDC.OBJ    :  WINDC.C
  $(CC) $(CFLAGS) $*.c

WINDEBUG.OBJ :  WINDEBUG.C
  $(CC) $(CFLAGS) $*.c

WINDEFER.OBJ :  WINDEFER.C
  $(CC) $(CFLAGS) $*.c

WINDLG.OBJ   :  WINDLG.C
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

WGUIICON.OBJ :  WGUIICON.C
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

WINPRINT.OBJ :  WINPRINT.C
  $(CC) $(CFLAGS) $*.c

WINPROP.OBJ  :  WINPROP.C
  $(CC) $(CFLAGS) $*.c

WINRECT.OBJ  :  WINRECT.C
  $(CC) $(CFLAGS) $*.c

WINRES.OBJ   :  WINRES.C
  $(CC) $(CFLAGS) $*.c

WINRES2.OBJ  :  WINRES2.C
  $(CC) $(CFLAGS) $*.c

WINRES3.OBJ  :  WINRES3.C
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

WMFCSTUB.OBJ :  WMFCSTUB.C
  $(CC) $(CFLAGS) $*.c

WMNUCHNG.OBJ :  WMNUCHNG.C
  $(CC) $(CFLAGS) $*.c

WMNUDRAW.OBJ :  WMNUDRAW.C
  $(CC) $(CFLAGS) $*.c

WMNUINFO.OBJ :  WMNUINFO.C
  $(CC) $(CFLAGS) $*.c

WMODULE.OBJ  :  WMODULE.C
  $(CC) $(CFLAGS) $*.c

WMOUAPI.OBJ  :  WMOUAPI.C
  $(CC) $(CFLAGS) $*.c

WMSGBOX.OBJ  :  WMSGBOX.C
  $(CC) $(CFLAGS) $*.c

WMULDIV.OBJ  :  WMULDIV.C
  $(CC) $(CFLAGS) $*.c

WNOTIFY.OBJ  :  WNOTIFY.C
  $(CC) $(CFLAGS) $*.c

WOBJECT.OBJ  :  WOBJECT.C
  $(CC) $(CFLAGS) $*.c

WOPENFIL.OBJ :  WOPENFIL.C
  $(CC) $(CFLAGS) $*.c

WPATBLT.OBJ  :  WPATBLT.C
  $(CC) $(CFLAGS) $*.c

WPLACEMT.OBJ :  WPLACEMT.C
  $(CC) $(CFLAGS) $*.c

WPROCINS.OBJ :  WPROCINS.C
  $(CC) $(CFLAGS) $*.c

WPROFILE.OBJ :  WPROFILE.C
  $(CC) $(CFLAGS) $*.c

WPUSHBUT.OBJ :  WPUSHBUT.C
  $(CC) $(CFLAGS) $*.c

WREADINI.OBJ :  WREADINI.C
  $(CC) $(CFLAGS) $*.c

WREFORM.OBJ  :  WREFORM.C
  $(CC) $(CFLAGS) $*.c

WREFRESH.OBJ :  WREFRESH.C
  $(CC) $(CFLAGS) $*.c

WREGION.OBJ  :  WREGION.C
  $(CC) $(CFLAGS) $*.c

WRGNFILL.OBJ :  WRGNFILL.C
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

WSTRETCH.OBJ :  WSTRETCH.C
  $(CC) $(CFLAGS) $*.c

WSYSMENU.OBJ :  WSYSMENU.C
  $(CC) $(CFLAGS) $*.c

WTABTEXT.OBJ :  WTABTEXT.C
  $(CC) $(CFLAGS) $*.c

WTEXTMET.OBJ :  WTEXTMET.C
  $(CC) $(CFLAGS) $*.c

WTEXTOUT.OBJ :  WTEXTOUT.C
  $(CC) $(CFLAGS) $*.c

WVIEWEXT.OBJ :  WVIEWEXT.C
  $(CC) $(CFLAGS) $*.c

WUNREGCL.OBJ :  WUNREGCL.C
  $(CC) $(CFLAGS) $*.c

WVIEWORG.OBJ :  WVIEWORG.C
  $(CC) $(CFLAGS) $*.c

WVIEWSCL.OBJ :  WVIEWSCL.C
  $(CC) $(CFLAGS) $*.c

WWINEXT.OBJ  :  WWINEXT.C
  $(CC) $(CFLAGS) $*.c

WWINORG.OBJ  :  WWINORG.C
  $(CC) $(CFLAGS) $*.c

WWINSCL.OBJ  :  WWINSCL.C
  $(CC) $(CFLAGS) $*.c

WZORDER.OBJ  :  WZORDER.C
  $(CC) $(CFLAGS) $*.c

