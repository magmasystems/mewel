if TC
  include tc8086.mac
else
  include lm8086.mac
endif

  DATASEGM
  ENDDS

        PROGSEG lstring

strlenarg  struc
           dw      3 dup (?)    ; bp and far return
string1    dd      ?
strlenarg  ends

	public  LSTRLEN
LSTRLEN proc    far
	push	bp
	mov	bp,sp

	mov	dx,di         ; Save DI without using a 'push di'
	les	di,[bp].string1 ; Get a ptr to the string
	xor	ax,ax         ; AX has the value to scan for (0)
	mov	cx,0FFFFh     ; Scan 64K bytes at max
	repne	scasb         ; Search for the 0 terminator
	not	cx            ; cx has how many bytes left - negate it
	dec	cx            ;  to get the number of chars scanned
	xchg	ax,cx         ;  and move the result into ax
	mov	di,dx         ; Restore DI

	pop	bp
	ret     4
LSTRLEN endp

;---------------------------------------------------------------------------

strchrarg  struc
           dw      3 dup (?)    ; bp and far return
ch1        dw      ?
string2    dd      ?
strchrarg  ends

	public	LSTRCHR
LSTRCHR proc far
	push	bp
	mov	bp,sp

	push	di
	les	di,[bp].string2    ; ES:DI has a ptr to the string
	mov	bx,di              ; Save the starting offset

	xor	ax,ax              ; First do a quick strlen() so we know
	mov	cx,0FFFFh          ;  how many bytes to scan
	repne	scasb
	inc	cx
	neg	cx

	mov	ax,[bp].ch1        ; Scan for the desired char starting from
	mov	di,bx              ;  the original offset.
	repne	scasb

	dec	di
	cmp	es:[di],al         ; Was the char found?
	je	found1

	xor	dx,dx              ; The char wasn't found! 
        xor     ax,ax
        jmp short bye1

found1: mov	ax,di
	mov	dx,es

bye1:   pop     di
	mov	sp,bp
	pop	bp
	ret     6
LSTRCHR endp

;---------------------------------------------------------------------------

strcpyarg  struc
           dw      3 dup (?)    ; bp and far return
src2       dd      ?
dest2      dd      ?
strcpyarg  ends

	public	LSTRCPY
LSTRCPY proc    far
	push	bp
	mov	bp,sp

	mov	dx,di           ; Save SI and DI without using a push
	mov	bx,si
	push	ds

	lds	si,[bp].src2    ; Get the length of the source string
	mov	di,si
	mov	ax,ds
	mov	es,ax
	xor	ax,ax
	mov	cx,0FFFFh
	repne	scasb
	not	cx

	les	di,[bp].dest2    ; Get a ptr to the destination
	mov	ax,di            ; Starting on an odd boundary?
	test	al,1             ; If so, move 1 byte so we can do the
	je	moveem           ;  fast copy from an even boundary
	movsb
	dec	cx

moveem: shr	cx,1             ; Move two bytes at a time
	rep	movsw
	adc	cx,cx
	rep	movsb            ; Move the last byte

	mov	si,bx
	mov	di,dx
	pop	ds
	mov	dx,es
	pop	bp
	ret     8
LSTRCPY endp

;------------------------------------------------------------------------

strncpyarg  struc
           dw      3 dup (?)    ; bp and far return
n          dw      ?
src3       dd      ?
dest3      dd      ?
strncpyarg  ends

	public	LSTRNCPY
LSTRNCPY proc far
	push	bp
	mov	bp,sp
	push	di
	push	si
	push	ds

	les	di,[bp].dest3
	lds	si,[bp].src3
	mov	bx,di              ; Save the starting offset
	mov	cx,[bp].n
	jcxz	bye3               ; Moving no chars? Exit!

cpy1:   lodsb                      ; while ((c = *src3++) != '\0')
	or	al,al
	je	done3
	stosb                      ;   *dest3++ = c;
	loop	cpy1

done3:	xor	al,al              ; *dest3++ = '\0';
	stosb

bye3:   mov	ax,bx
	mov	dx,es
	pop	ds
	pop	si
	pop	di
	mov	sp,bp
	pop	bp
	ret     10
LSTRNCPY endp


if TC
  endps lstring
else
  endps
endif
  end


