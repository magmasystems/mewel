
Memo #56464
Return-path: <wildfire!dpelland@uu3.psi.com>
Received: from delphi.com by bix.com (CoSy3.31.1.45) id
 <9401071257.memo.56464@BIX.com>; Fri,  7 Jan 1994 12:57:11 -0500 (EST)
Received: from uu3.psi.com by delphi.com (PMDF V4.2-11 #4520) id
 <01H7EH0OITG08WWQUC@delphi.com>; Fri, 7 Jan 1994 12:49:44 EDT
Received: from wildfire.UUCP by uu3.psi.com (5.65b/4.0.071791-PSI/PSINet) via
 UUCP; id AA04986 for ; Fri, 7 Jan 94 12:35:37 -0500
Received: from dpelland by wildfire.wildfire.com; Fri,  7 Jan 94 12:33 EST
From: "David M. Pelland" <dpelland@wildfire.com>
Date: Fri, 07 Jan 1994 12:33:22 -0500 (EST)
To: magma@bix.com
Message-id: <9401071735.AA04986@uu3.psi.com>
Subject: makefile
MIME-version: 1.0
Content-type: text/plain; charset=US-ASCII
Content-transfer-encoding: 7bit
X-Envelope-to: bix.com!magma
X-Mailer: ELM [version 2.4 PL21]
Content-Length: 2922


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
CC = gcc
#OPT = -Gs -Oit -K
DEBUG = -g 
UNIXDEFS= -Dpascal= -Dcdecl= -Dfar= -Dnear= -DPASCAL= -DCDECL= -DFAR= -DNEAR=
CFLAGS = -c -funsigned-char -fwritable-strings -DSYSV -DM_TERMINFO -DUNIX -UDOS 
$(UNIXDEFS) $(OPT) $(DEBUG)

# OBJECT FILES

OS = wactive.o\
pell.o\
wadjust.o\
wanypop.o\
wbitmap.o\
wcheckbx.o\
wclipbrd.o\
wcomfile.o\
wcreate.o\
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
wgraystr.o\
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
windlg.o\
windc.o\
windebug.o\
windefer.o\
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
winqueue.o\
winpoint.o\
winprop.o\
winrect.o\
winres.o\
winres2.o\
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
wlastpop.o\
wlibrary.o\
wlineto.o\
wlistbox.o\
wlnklist.o\
wloadacc.o\
wloaddlg.o\
wloadmnu.o\
wloadstr.o\
wlptodp.o\
wlstring.o\
wmemglob.o\
wmemlocl.o\
wmenuhi.o\
wmessage.o\
wmnuchng.o\
wmnudraw.o\
wmnuinfo.o\
wmsgbox.o\
wnotify.o\
wobject.o\
wopenfil.o\
wpatblt.o\
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
wvismap.o\
wzorder.o\
wunixdir.o\
wunixkbd.o\
wunixmis.o\
wunixscr.o


$(LIB): $(OS)
        ar -ru $(LIB) $(OS)

read/action: