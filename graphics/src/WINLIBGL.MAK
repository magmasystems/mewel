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
#                                                                          #
#   The default is for an optimized, real-mode version.                    #
#                                                                          #
#                                                                          #
############################################################################

#
# Specify the compiler to use
#
CC=gcc

#
# Optimization : -G favors speed over size, -O improves jumps & loops
#
#DEBUG=-g -O0
OPT=-O2
LIBFILE=libmewel.a

DEFINES=-DDOS -DMEWEL

#
# Specify your include path
#
INCPATH=-Ic:/mewel

CFLAGS=$(OPT) $(DEBUG) -c $(DEFINES) $(INCPATH) -xc -ansi -funsigned-char -fwritable-strings

#
# Linker and librarian flags
#
LIB = ar


# Inference rules for C and ASM files
#   Note - if you want to build the library as you go, uncomment the
#          second line. Or else, you can use the WINLIB.LNK file to create
#          the library in a single shot.

.c.obj :
	$(CC) $(CFLAGS) $*.c


OBJSASM =    DELAY.o    KEY.o      LCGACPY.o  LMEMCHR.o  LMEMMOVE.o \
LWMEMSET.o LMEMCPY.o  LSTRING.o

OBJSINTR =   WINALT.o   WINBREAK.o WININT24.o

OBJSEXEC =   WINSPAWN.o CHECKPAT.o EXEC.o

OBJSREAL =   WINDOS.o   XMSLIB.o



objs1 =    wactive.o  wadjust.o  wanypop.o  wbitblt.o  wbitmap.o  \
wcheckbx.o wclipbrd.o wcreate.o  wdlgdir.o  wdlgopen.o wdrawtxt.o \
wdrvtype.o weditpro.o wedtdraw.o wemalloc.o wenumfnt.o wenumobj.o \
wexitwin.o wexttext.o wfileio.o  wfillwin.o wfindwin.o 

objs2 =    wgdichar.o wgdiclip.o wgdifont.o wgdirect.o wgdirgb.o  \
wgdirgn.o  wgdiset.o  wgdishap.o wgdistub.o wgetclas.o wgraystr.o \
winaccel.o winansi.o  winatom.o  winblit.o  winbordr.o wincaret.o \
wincatch.o winclass.o winclick.o wincolor.o wincombo.o wincreat.o \
windlg.o   windc.o    windebug.o 

objs3 =    windefer.o windraw.o  windstry.o winedit.o  winenabl.o \
winenum.o  winevent.o winexec.o  winextra.o winflash.o winhook.o  \
winicon.o  wininit.o  winkbd.o   winlbdir.o winmdi.o   winmenu.o  \
winmenu2.o winmodal.o winmono.o  winmouse.o winmove.o  winpoint.o \
winprop.o  winqueue.o winrect.o  winres.o   

objs4 =    winscrol.o winset.o   winshow.o  winsize.o  winsound.o \
wintask.o  winterm.o  wintime.o  wintimer.o winvid.o   \
winvirt.o  winzoom.o  wischar.o  wkbstack.o wlastpop.o wlibrary.o \
wlineto.o  wlistbox.o wlnklist.o wloadacc.o wloadbmp.o wloaddlg.o \
wloadmnu.o wloadstr.o wlptodp.o  wlstring.o wmemglob.o wmemlocl.o \
wmenuhi.o  wmessage.o wmnuchng.o wmnudraw.o wmnuinfo.o wmouapi.o  \
wmsgbox.o  

objs5 =    wobject.o  wopenfil.o wpatblt.o  wprofile.o wpushbut.o \
wreform.o  winres2.o  wrubber.o  wsavedc.o  wscroldc.o wscrlbar.o \
wselectr.o wsetrect.o wsetwpos.o wshadow.o  wsprintf.o wsrchpth.o \
wstatic.o  wstdproc.o wstfocus.o wstrdisp.o wsysmenu.o wtabtext.o \
wtextmet.o wtextout.o wvismap.o  wzorder.o  wviewext.o wvieworg.o \
wviewscl.o wwinext.o  wwinorg.o  wwinscl.o

objs6 =    wcomfile.o wcomfind.o wspy.o     wrefresh.o \
wnotify.o  wkeystat.o wcreatdc.o winprint.o wfiletit.o warricon.o \
wunregcl.o wgetpos.o

# objs7 has files which are not used for the protected-mode build
objs7 = wdos386.o


all          :  $(objs1) $(objs2) $(objs3) $(objs4) $(objs5) $(objs6) $(objs7)
	del d:\djgpp\$(LIBFILE)
	$(LIB) -ru $(LIBFILE) $(objs1) $(objs2) $(objs3) $(objs4) $(objs5) $(objs6) $(objs7)
#copy libmewel.a d:\djgpp\lib

wactive.o  :  wactive.c

wadjust.o  :  wadjust.c

wanypop.o  :  wanypop.c

warricon.o :  warricon.c

wbitblt.o  :  wbitblt.c

wbitmap.o  :  wbitmap.c

wcheckbx.o :  wcheckbx.c

wclipbrd.o :  wclipbrd.c

wcreatdc.o :  wcreatdc.c

wcreate.o  :  wcreate.c

wdlgdir.o  :  wdlgdir.c

wdlgopen.o :  wdlgopen.c

wdrawtxt.o :  wdrawtxt.c

wdrvtype.o :  wdrvtype.c

weditpro.o :  weditpro.c

wedtdraw.o :  wedtdraw.c

wemalloc.o :  wemalloc.c

wenumfnt.o :  wenumfnt.c

wenumobj.o :  wenumobj.c

wexitwin.o :  wexitwin.c

wexttext.o :  wexttext.c

wfileio.o  :  wfileio.c

wfiletit.o :  wfiletit.c

wfillwin.o :  wfillwin.c

wfindwin.o :  wfindwin.c

wgdichar.o :  wgdichar.c

wgdiclip.o :  wgdiclip.c

wgdifont.o :  wgdifont.c

wgdimap.o  :  wgdimap.c

wgdirect.o :  wgdirect.c

wgdirgb.o  :  wgdirgb.c

wgdirgn.o  :  wgdirgn.c

wgdiset.o  :  wgdiset.c

wgdishap.o :  wgdishap.c

wgdistub.o :  wgdistub.c

wgetclas.o :  wgetclas.c

wgetpos.o  :  wgetpos.c

wgraystr.o :  wgraystr.c

winaccel.o :  winaccel.c

winalt.o   :  winalt.c

winansi.o  :  winansi.c

winatom.o  :  winatom.c

winblit.o  :  winblit.c

winbordr.o :  winbordr.c

winbreak.o :  winbreak.c

wincaret.o :  wincaret.c

wincatch.o :  wincatch.c

winclass.o :  winclass.c

winclick.o :  winclick.c

wincolor.o :  wincolor.c

wincombo.o :  wincombo.c

wincreat.o :  wincreat.c

windlg.o   :  windlg.c

windc.o    :  windc.c

windebug.o :  windebug.c

windefer.o :  windefer.c

windos.o   :  windos.c

windraw.o  :  windraw.c

windstry.o :  windstry.c

winedit.o  :  winedit.c

winenabl.o :  winenabl.c

winenum.o  :  winenum.c

winevent.o :  winevent.c

winexec.o  :  winexec.c

winextra.o :  winextra.c

winflash.o :  winflash.c

winhook.o  :  winhook.c

winicon.o  :  winicon.c

wininit.o  :  wininit.c

winint24.o :  winint24.c

winkbd.o   :  winkbd.c

winlbdir.o :  winlbdir.c

winmdi.o   :  winmdi.c

winmenu.o  :  winmenu.c

winmenu2.o :  winmenu2.c

winmodal.o :  winmodal.c

winmono.o  :  winmono.c

winmouse.o :  winmouse.c

winmove.o  :  winmove.c

winqueue.o :  winqueue.c

winpoint.o :  winpoint.c

winprint.o :  winprint.c

winprop.o  :  winprop.c

winrect.o  :  winrect.c

winres.o   :  winres.c

winres2.o  :  winres2.c

winscrol.o :  winscrol.c

winset.o   :  winset.c

winshow.o  :  winshow.c

winsize.o  :  winsize.c

winsound.o :  winsound.c

winspawn.o :  winspawn.asm

wintask.o  :  wintask.c

winterm.o  :  winterm.c

wintime.o  :  wintime.c

wintimer.o :  wintimer.c

winvid.o   :  winvid.c

winvirt.o  :  winvirt.c

winzoom.o  :  winzoom.c

wischar.o  :  wischar.c

wkbstack.o :  wkbstack.c

wkeystat.o :  wkeystat.c

wlastpop.o :  wlastpop.c

wlibrary.o :  wlibrary.c

wlineto.o  :  wlineto.c

wlistbox.o :  wlistbox.c

wlnklist.o :  wlnklist.c

wloadacc.o :  wloadacc.c

wloadbmp.o :  wloadbmp.c

wloaddlg.o :  wloaddlg.c

wloadmnu.o :  wloadmnu.c

wloadstr.o :  wloadstr.c

wlptodp.o  :  wlptodp.c

wlstring.o :  wlstring.c

wmemglob.o :  wmemglob.c

wmemlocl.o :  wmemlocl.c

wmenuhi.o  :  wmenuhi.c

wmessage.o :  wmessage.c

wmnuchng.o :  wmnuchng.c

wmouapi.o  :  wmouapi.c

wmsgbox.o  :  wmsgbox.c

wobject.o  :  wobject.c

wopenfil.o :  wopenfil.c

wpatblt.o  :  wpatblt.c

wprofile.o :  wprofile.c

wpushbut.o :  wpushbut.c

wreform.o  :  wreform.c

wrubber.o  :  wrubber.c

wsavedc.o  :  wsavedc.c

wscroldc.o :  wscroldc.c

wscrlbar.o :  wscrlbar.c

wselectr.o :  wselectr.c

wsetrect.o :  wsetrect.c

wsetwpos.o :  wsetwpos.c

wshadow.o  :  wshadow.c

wsprintf.o :  wsprintf.c

wstatic.o  :  wstatic.c

wstdproc.o :  wstdproc.c

wstfocus.o :  wstfocus.c

wstrdisp.o :  wstrdisp.c

wsysmenu.o :  wsysmenu.c

wtextmet.o :  wtextmet.c

wtextout.o :  wtextout.c

wunregcl.o :  wunregcl.c

wvgamap.o  :  wvgamap.c

wvgawrt.o  :  wvgawrt.asm

wvismap.o  :  wvismap.c

wzorder.o  :  wzorder.c

