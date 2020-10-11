;  lmemcpy.asm
;
;  char far pascal *lmemcpy(char far *dest, char far *src, unsigned len)
;

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

	DATASEGM
	ENDDS

	PROGSEG    lmemcpy

cpystr  struc
        dw      3 dup (?)    ; bp and far return
len     dw      ?
src     dd      ?
dest    dd      ?
cpystr  ends

	public LMEMCPY
LMEMCPY PROC FAR

	push	bp
	mov	  bp,sp

	push	di
	push	si
	push	ds

	lds	si, [bp].src
	les	di, [bp].dest
	mov	cx, [bp].len

; set up return values
	mov dx, es
	mov ax, di

	cld
	shr	cx,1
	rep	movsw
; MOVE THE (possible) REMAINING BYTE
	adc	cx,cx
	rep	movsb

	pop	ds
	pop	si
	pop	di

	pop	bp
	ret 10

LMEMCPY endp
if TC
	endps lmemcpy
else
  endps
endif
  end

