;  lmemcpy.asm
;
;  void far pascal lcgacpy(char far *dest, char far *src, unsigned len)
;

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

	DATASEGM
	ENDDS

	PROGSEG    lcgacpy

cpystr  struc
        dw      3 dup (?)    ; bp and far return
len     dw      ?
src     dd      ?
dest    dd      ?
cpystr  ends

TIMEOUT EQU     6
VBCount EQU     250


	public LCGACPY
LCGACPY PROC FAR

	push	bp
	mov	bp,sp

	push	di
	push	si
	push	ds

	mov	cx, [bp].len
	shr	cx,1            ; CX has the number of words to move
	jcxz	L10
	lds	si, [bp].src    ; DS:SI is the address of the cell buffer
	les	di, [bp].dest   ; ES:DI is the video ram address
	mov	dx, 03DAH       ; DX is the address of the CGA port
	cld

        ; We will always be moving words (char/attr pairs), so we don't have
        ; to worry about the remaining byte to copy.

        ; This stuff is right out of pages 71-72 of the Wilton book. Sure 
        ; hope it works!

; Write during the remaining vertical blanking interval
L01:    mov     bx, cx
        mov     cx, TIMEOUT
        cli

L02:    in      al,dx
        test    al,1
        loopnz  L02
        jz      L03

        movsw
        sti
        mov     cx,bx
        loop    L01

        jmp     short  L10


; Write during the horizontal blanking interval
L03:    sti
        mov     cx,bx

L04:    lodsw
        mov     bx,ax
        push    cx
        mov     cx,TIMEOUT
        cli

L05:    in      al,dx
        test    al,1
        loopnz  L05
        jnz     L07

L06:    in      al,dx
        test    al,1
        jz      L06

        xchg    ax,bx
        stosw
        sti
        pop     cx
        loop    L04

        jmp     short L10

; Write during the entire vertical blanking interval
L07:    pop     bx
        dec     si
        dec     si

        mov     cx,VBCount
        cmp     bx,cx
        jnb     L08

        mov     cx,bx
        xor     bx,bx
        jmp     short  L09

L08:    sub     bx,cx

L09:    rep     movsw
        mov     cx,bx
        test    cx,cx
        jnz     L01



L10:
	pop	ds
	pop	si
	pop	di
	pop	bp
	ret	10

LCGACPY endp
if TC
	endps lcgacpy
else
  endps
endif
  end

