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
LIB=libmewel.a

# Compiler is M for Microsoft, T for Borland Turbo C
COMPILER = M
# Model is M for Medium, L for Large, S for Small
MODEL = L

# Microsoft C macros
CC = cc
#DEBUG = -g -Od
OPT = -Gs -Oit -K
UNIXDEFS= -Dpascal= -Dcdecl= -Dfar= -Dnear=
CFLAGS = -W2 -c -DSYSV -DM_TERMINFO -DUNIX -Dsco -DMSC -UDOS -Me -J $(UNIXDEFS) -Zp $(OPT) $(DEBUG) -H64

.c.obj :
	$(CC) $(CFLAGS) $*.c
	sync


# OBJECT FILES

OS = wactive.o\
wadjust.o\
wanypop.o\
warricon.o\
wbitblt.o\
wbitmap.o\
wcheckbx.o\
wclipbrd.o\
wcomfile.o\
wcomfind.o\
wcreatdc.o\
wcreate.o\
wdesktop.o\
wdlgdir.o\
wdlgopen.o\
wdrawtxt.o\
wdrvtype.o\
weditpro.o\
wedtdraw.o\
wemalloc.o\
wenumfnt.o\
wenumobj.o\
wexitwin.o\
wexttext.o\
wfileio.o\
wfiletit.o\
wfillwin.o\
wfindwin.o\
wgdichar.o\
wgdiclip.o\
wgdifont.o\
wgdirect.o\
wgdirgb.o\
wgdirgn.o\
wgdiset.o\
wgdishap.o\
wgdistub.o\
wgetclas.o\
wgetpos.o\
wgraystr.o\
whfileio.o\
winaccel.o\
winansi.o\
winatom.o\
winblit.o\
winbordr.o\
winbreak.o\
wincaret.o\
wincatch.o\
winclass.o\
winclick.o\
wincolor.o\
wincombo.o\
wincreat.o\
windata.o\
windc.o\
windebug.o\
windefer.o\
windlg.o\
windos.o\
windraw.o\
windstry.o\
winedit.o\
winenabl.o\
winenum.o\
winevent.o\
winextra.o\
winflash.o\
winhook.o\
winicon.o\
wininit.o\
winmdi.o\
winmenu.o\
winmenu2.o\
winmodal.o\
winmono.o\
winmove.o\
winpoint.o\
winprop.o\
winqueue.o\
winrect.o\
winres.o\
winres2.o\
winres3.o\
winscrol.o\
winset.o\
winshow.o\
winsize.o\
winsound.o\
wintask.o\
winterm.o\
wintime.o\
wintimer.o\
winvirt.o\
winzoom.o\
wischar.o\
wkbstack.o\
wkeystat.o\
wlastpop.o\
wlibrary.o\
wlineto.o\
wlistbox.o\
wlnklist.o\
wloadacc.o\
wloadbmp.o\
wloaddlg.o\
wloadmnu.o\
wloadstr.o\
wlptodp.o\
wlstring.o\
wmemglob.o\
wmemlocl.o\
wmenuhi.o\
wmessage.o\
wmfcstub.o\
wmnuchng.o\
wmnudraw.o\
wmnuinfo.o\
wmodule.o\
wmsgbox.o\
wnotify.o\
wobject.o\
wopenfil.o\
wpatblt.o\
wplacemt.o\
wprocins.o\
wprofile.o\
wpushbut.o\
wreform.o\
wrefresh.o\
wrubber.o\
wsavedc.o\
wscroldc.o\
wscrlbar.o\
wselectr.o\
wsetrect.o\
wsetwpos.o\
wshadow.o\
wsprintf.o\
wsrchpth.o\
wstatic.o\
wstdproc.o\
wstfocus.o\
wsysmenu.o\
wtabtext.o\
wtextmet.o\
wtextout.o\
wunregcl.o\
wvismap.o\
wviewext.o\
wvieworg.o\
wviewscl.o\
wwinext.o\
wwinorg.o\
wwinscl.o\
wzorder.o\
wunixdir.o\
wunixkbd.o\
wunixmis.o\
wunixscr.o


$(LIB): $(OS)
	ar -ru $(LIB) $(OS)
	cp $(LIB) /lib/$(LIB)

