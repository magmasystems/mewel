;  vbtshlp.asm
;
;  void far pascal vbtshlp(void far *dest, void far *src, int attr, int cnt)
;

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

	DATASEGM
	ENDDS

	PROGSEG vbtshlp

vbtsstr  struc
        dw      3 dup (?)    ; bp and far return
bSnow   dw      ?
len     dw      ?
attr    dw      ?
src     dd      ?
dest    dd      ?
vbtsstr  ends

	public VBTSHLP
VBTSHLP PROC FAR
	push	bp
	mov     bp,sp

	push	di
	push	si
	push	ds

	lds	si, [bp].src
	les	di, [bp].dest
        mov     bx, [bp].bSnow
        mov     dx, 03DAH
	mov     ax, [bp].attr
	mov     ah, al
	mov	cx, [bp].len

        ; if (len == 0)
        ;   return;
	jcxz    done

	cld

        ; while (len-- > 0)
        ;   if ((al = *src++) != 0xFF)
        ;     *dest++ = al;
        ;
_loop:
	lodsb
	cmp     al,0FFH
	je      skip
        test    bl,bl
        jnz short DoSnow
	stosw
	loop    _loop
	jmp short done

skip:
	add     di, 2
	loop    _loop
        jmp     done

DoSnow:
        mov     bh, al    ; save the character in BH
        cli
l2:     in      al, dx    ; wait for the horiz retrace
        test    al, 1
        jnz     l2
l3:     in      al, dx
        test    al, 1
        jz      l3
        mov     al,bh     ; restore the character. AX is now attr/char
        stosw             ; blast the char to video mem
        sti
	loop    _loop


done:
	pop	ds
	pop	si
	pop	di
	pop	bp
	ret     14

VBTSHLP endp


if TC
	endps vbtshlp
else
        endps
endif
        end

