	name	languppr
	IFDEF	LMODEL
	IF	LMODEL
	.model	large,c
	%out	Large Model
	ELSE
	.model	small,c
	%out	Small Model
	ENDIF
	ELSE
	.model	small,c
	%out	Small Model
	ENDIF
;
; (almost) language-independent case mapping routine for MEWEL
;	written by
;		Thomas Wagner
;		Ferrari electronic GmbH
;		Beusselstrasse 27
;		D-1000 Berlin 21, Germany
;
;
	public	lang_upper
;
;
;	country information (DOS 3.0 and later)
;
cinfo		struc
;
datefmt		dw	?
currency	db	5 dup(?)
sep_thou	db	2 dup(?)
sep_decimal	db	2 dup(?)
sep_date	db	2 dup(?)
sep_time	db	2 dup(?)
pos_currency	db	?
places_currency	db	?
time_format	db	?
mapproc		dd	?
sep_list	db	2 dup(?)
reserved	db	10 dup(?)
;
cinfo		ends
;
	.data
;
jumpad	dw	offset determine_method
;
	.data?
;
countrybuf	cinfo	<>
;
	.code
lang_upper	proc	inchar: byte
	mov	al,inchar
	xor	ah,ah
	cmp	al,'a'
	jb	no_change
	cmp	al,'z'
	jbe	do_trans
	cmp	al,80h
	jb	no_change
	jmp	jumpad
;
do_trans:
	sub	al,'a'-'A'
no_change:
	ret
;
;
determine_method:
	mov	jumpad,offset dirtrans
	mov	ax,3000h	; get DOS version
	int	21h
	cmp	al,3
	jb	method_ok	; use internal table for DOS versions < 3
;
;	DOS 3.0 and above provide a character map call address.
;
	mov	ax,3800h	; get current country
	mov	dx,offset countrybuf
	int	21h
	jc	method_ok	; fall back to 2.x if this fails
	mov	ax,word ptr countrybuf.mapproc
	or	ax,word ptr countrybuf.mapproc + 2
	jz	method_ok	; fall back to 2.x if no routine
	mov	jumpad,offset callproc
;
method_ok:
	mov	al,inchar
	xor	ah,ah
	jmp	jumpad
;
;------------------------------------------------------------------------
;
;	For pre-3.0 versions of DOS, no character translation
;	routine is provided by DOS, so we have to use our own.
;	We can safely assume codepage 437, since no other codepages
;	existed - assuming we're running on an IBM compatible at all.
;
uppertab	label	byte
;
	db	080h,09Ah,045h,041h,08Eh,041h,08Fh,080h,045h,045h,045h,049h,049h,049h,08Eh,08Fh
	db	090h,092h,092h,04Fh,099h,04Fh,055h,055h,059h,099h,09Ah,09Bh,09Ch,09Dh,09Eh,09Fh
	db	041h,049h,04Fh,055h,0A5h,0A5h,0A6h,0A7h,0A8h,0A9h,0AAh,0ABh,0ACh,0ADh,0AEh,0AFh
	db	0B0h,0B1h,0B2h,0B3h,0B4h,0B5h,0B6h,0B7h,0B8h,0B9h,0BAh,0BBh,0BCh,0BDh,0BEh,0BFh
	db	0C0h,0C1h,0C2h,0C3h,0C4h,0C5h,0C6h,0C7h,0C8h,0C9h,0CAh,0CBh,0CCh,0CDh,0CEh,0CFh
	db	0D0h,0D1h,0D2h,0D3h,0D4h,0D5h,0D6h,0D7h,0D8h,0D9h,0DAh,0DBh,0DCh,0DDh,0DEh,0DFh
	db	0E0h,0E1h,0E2h,0E3h,0E4h,0E5h,0E6h,0E7h,0E8h,0E9h,0EAh,0EBh,0ECh,0EDh,0EEh,0EFh
	db	0F0h,0F1h,0F2h,0F3h,0F4h,0F5h,0F6h,0F7h,0F8h,0F9h,0FAh,0FBh,0FCh,0FDh,0FEh,0FFh
;
dirtrans:
	mov	bx,ax
	mov	al,uppertab-80h[bx]
	ret
;
;------------------------------------------------------------------------
;
;	DOS-versions 3.3 and up provide a case mapping call address.
;
callproc:
	call	countrybuf.mapproc
	ret
;
lang_upper	endp
;
;------------------------------------------------------------------------
;------------------------------------------------------------------------
;
alttab_usa	label	byte
	db	0,'1234567890','QWERTYUIOP'

	end

