.MODEL LARGE

ifdef DOS16M
        extrn   __dos_getvect:far
endif

ifdef DOS286X
        extrn   DOSFREESEG:far
        extrn   DOSMAPREALSEG:far
endif

ifdef DOS16M
        extrn   _D16ProtectedPtr:far
        extrn   _D16SegCancel:far
endif

        ASSUME  DS: DGROUP, SS: DGROUP
ifdef DOS286X
        extrn   _selA000Seg:word
        extrn   _selZeroSeg:word
        extrn   _selBiosSeg:word
        extrn   _selCharGenerator:word
endif

.DATA
sel     dw      0

.286

.CODE

;----------------------------------------------------------------------------;
;                                                                            ;
; VOID PASCAL _VGAVioWrtCellStr(LPWORD lpBuf, WORD iWidth,                   ;
;                               WORD row, WORD col, WORD iReserved)          ;
;                                                                            ;
;    Uses the VGA BIOS to write out a string starting at <row,col>. The      ;
; string contains char/attr pairs. The cursor will not be moved.             ;
;                                                                            ;
; BUG - The PC Bios has a bug where if you write in the lower-right corner   ;
;       of the screen, the screen will scroll up one line.                   ;
;                                                                            ;
;----------------------------------------------------------------------------;

argstr  struc
        dw      3 dup (?)    ; bp and far return
iReserv dw      ?
col     dw      ?
row     dw      ?
iWidth  dw      ?
lpBuf   dd      ?
argstr  ends

	public  _VGAVIOWRTCELLSTR
_VGAVIOWRTCELLSTR PROC FAR
	push	bp
	mov     bp,sp

	xor	bh,bh                 ; BH = Page 0
        mov     cx,[bp].iWidth        ; CX = num chars to write
        mov     ax,[bp].row           ; DH = row, DL = col
        mov     dh,al
        mov     ax,[bp].col
        mov     dl,al
	mov	ax, 1302H             ; AX = service 13H, subservice 2
        push    bp
        les     bp,[bp].lpBuf         ; ES:BP = pointer to string
        int     10H
        pop     bp

	mov	sp,bp
	pop	bp
	ret	12

_VGAVIOWRTCELLSTR endp

;-----------------------------------------------------------------------------


setstr struc
        dw      3 dup (?)    ; bp and far return
attr    dw      ?
y       dw      ?
x       dw      ?
char    dw      ?
setstr ends

VARshift EQU [bp-2]

BytesPerLine = 80
RMWbits      = 18h
OriginOffset = 0
VideoBufferSeg = 0A000h


	PUBLIC  VGAWRTGRAPHICSCHAR

VGAWRTGRAPHICSCHAR PROC FAR

	push    bp
        mov     bp,sp
        sub	sp,2
        push	di
        push	si
        push	ds
        push    es

; Calculate the address of the upper-left pixel. Returned in ES:BX
        mov	ax,[bp].y
        mov	bx,[bp].x

	call    PixelAddr10
        inc	cx
        and 	cl,7
        mov	ch,0FFh
        shl	ch,cl
        mov	VARshift,cx

; Get a pointer to the character bitmap in the video ram
	push	es
        mov	si,bx

ifdef DOS286X
        mov	ax,DGROUP:_selBiosSeg
else
        mov	ax,40h        ; 40:85 is the number of pixel rows in a char
endif
        mov	ds,ax
        mov	cx,ds:[85h]

ifdef DOS286X
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,DGROUP:_selZeroSeg
else
        xor	ax,ax
endif
        mov	ds,ax         ; ds is 0

        mov	ax,[bp].char
        mov	bx,43h*4

ifdef DOS286X
; In protected mode. we need to get a selector to the char generator's
; bitmap. The real-mode address is in es:di.
; DS is the 0 segment
        push    ax
        mov     di,ds:[bx]      ; ax:di has the address of the char gen bitmap
        mov     ax,ds:[bx+2]

; Make DS point to the DGROUP again
        push    ds
        push    si
        push    ax
        mov     ax,DGROUP
        mov     ds,ax
        pop     ax

; Get a protected mode selector to the char generator bitmap
        mov     ax,DGROUP:_selCharGenerator
        mov     es,ax

        pop     si
        pop     ds    ; restore the Zero seg
        pop     ax
else
ifdef DOS16M
; Get a pointer to the char generator bitmap into es:di
        push    cx

        ; ds:bx is 0:43*4H
        ; DOS16M uses selector 38H as a transparent selector to real mode
        ; segment 0. However, if we want to be compatible with DPMI,
        ; then we must use D16ProtectedPtr
        push    4
        push    ds
        push    bx
        call    far ptr _D16ProtectedPtr
        add     sp,6
        mov     bx,ax

        ; dx:ax has a protected mode ptr for int 43H
        mov     ds,dx
        mov     di,ds:[bx]      ; ax:di has the address of the char gen bitmap
        mov     ax,ds:[bx+2]

;       Free the selector to segment 0
        push    ax
        push    ds
        push    bx
        call    far ptr _D16SegCancel
        add     sp,4
        pop     ax

        ; ax:di has the real mode address of the bitmap. Xlate to prot mode
        push    0
        push    ax
        push    di
        call    far ptr _D16ProtectedPtr
        add     sp,6
        mov     es,dx
        mov     di,ax

        mov	ax,[bp].char  ; reload the character value which was passed in
        pop     cx
else
        les	di,ds:[bx]    ; ES:DI is char generator's bitmaps
endif
endif

        mul	cl            ; AX is char value * height of char
        add	di,ax         ; DI is the offset into the char generator

        pop	ds            ; DS:SI is the video buffer address
                              ; ES:DI is pointer to bitmap for the char

ifdef DOS16M
; Save the bitmap segment so we can free the selector later
        push    es
        push    di
endif

; Set up the graphics controller regs
	mov	dx,3CEh
        mov	ax,0A05h      ; Write mode 2, read mode 1  0x1010
        out	dx,ax
;       mov	ah,RMWbits    ; Read-Modify-Write bits = 0x18
        mov	ah,0          ; Read-Modify-Write bits = 0x18
       	mov	al,3
        out	dx,ax
        mov	ax,0007       ; don't care about color
        out	dx,ax

; Get the attribute info into BX. BL has the foreground and BH has the
; background.
        mov	bx,[bp].attr
        mov	bh,bl
        and	bl,0Fh
        shr     bh,1
        shr     bh,1
        shr     bh,1
        shr     bh,1

	cmp	byte ptr VARshift,0
        jne	L20

	mov	al,8                   ; al = bit-mask register number
L10:
; Update the background pixels
; Tell the bit-mask register that we want to update only the pixels which

		push cx
		push si
		mov   ah, 0FFh
		out   dx,ax
N10:
		and [si],bh
		add si, BytesPerLine
		loop N10

		pop si
		pop cx

; Update the foreground pixels
; Tell the bit-mask register that we want to update only the pixels which
; are 1 in the character bitmap. Then move the foreground color in there.
N30:
    	mov	ah,byte ptr es:[di]    ; ah is the bit pattern to update
		or ah, 0
		jz SKIP
		out dx,ax
		and ds:[si],bl

; Go on to the next row of pixels
SKIP:
        inc	di
        add	si,BytesPerLine
		  loop   N30
        jmp	short Lexit

L20:	push	cx
	mov	cx,VARshift

	mov	al,es:[di]
        xor	ah,ah
        shl	ax,cl
        push	ax
        mov	al,8
        out	dx,ax
	and	[si],bl
        not	ch
        xor	ah,ch
        out	dx,ax
        and	[si],bh

	pop	ax
        mov	ah,al
        mov	al,8
        out	dx,ax
        inc	si
        and	[si],bl
        not	ch
        xor	ah,ch
        out	dx,ax
        and	[si],bh

	inc	di
        dec	si
        add	si,BytesPerLine
        pop	cx
        loop	L20

Lexit:	mov	ax,0FF08h
	out	dx,ax
        mov	ax,0005
        out	dx,ax
        mov	ax,0003
        out	dx,ax
        mov	ax,0F07h
        out	dx,ax

ifdef DOS286X
        mov     ax,sel
endif

ifdef DOS16M
; Free the selector to the bitmap. ES and DI have already been pushed onto
; the stack above, so they are on the stack properly for the call to
; D16SegCancel! We don't need to pop them and push them again!
        call    far ptr _D16SegCancel
        add     sp,4
endif

        pop     es
	pop	ds
        pop	si
        pop	di
        mov	sp,bp
        pop	bp
        ret     8

VGAWRTGRAPHICSCHAR  ENDP


;------------------------------------------------------------------------
; PixelAddr10()
;   Returns the absolute address on a pixel in the video display in ES:BX
;   Also return a shift factor in CL
;     AX = y  BX = x
;------------------------------------------------------------------------

PixelAddr10 PROC near
	mov	cl,bl             
        push	dx
        mov	dx,BytesPerLine   
        mul	dx                
        pop	dx

	shr	bx,1              
        shr	bx,1
        shr	bx,1
        add	bx,ax             
        add	bx,OriginOffset

; Get the real-mode segment for seg A000 into ES

ifdef DOS286X
; It would be *so* easy if Pharlap had a selector to A000 already set up.
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov	ax,DGROUP:_selA000Seg
        pop     ds
else
        mov	ax,VideoBufferSeg
endif
        mov	es,ax

        and	cl,7
        xor	cl,7
        mov	ah,1
        ret

PixelAddr10 ENDP


;------------------------------------------------------------------------
; PixelAddr256()
;   Returns the absolute address on a pixel in the video display in ES:BX
;   Works in 256-color mode
;     AX = y  BX = x
;------------------------------------------------------------------------

PixelAddr256 PROC near
        push	dx
        mov	dx,640
        mul	dx                
        pop	dx
        add	bx,ax             ; AX = (640 * y)
        add	bx,OriginOffset   ; BX = (640 * y) + x

; Get the real-mode segment for seg A000 into ES

ifdef DOS286X
; It would be *so* easy if Pharlap had a selector to A000 already set up.
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov	ax,DGROUP:_selA000Seg
        pop     ds
else
        mov	ax,VideoBufferSeg
endif
        mov	es,ax
        xor     ah,ah
        xor     cl,cl
        ret

PixelAddr256 ENDP

;------------------------------------------------------------------------------
;
;VOID PASCAL VioMapCharBitmap(WORD chToMap, WORD nBytesPerChar, LPSTR lpBitmap)
;
;------------------------------------------------------------------------------

mapargs struc
         dw      3 dup (?)    ; bp and far return
lpBitmap dd      ?
nBytesPerChar dw ?
chToMap  dw      ?
mapargs ends

	public  VIOMAPCHARBITMAP
VIOMAPCHARBITMAP PROC FAR
	push	bp
	mov     bp,sp

	mov     ax,1100H  ; function 11 service 0 loads a user-defined bitmap 
        mov     bx,[bp].nBytesPerChar
        mov     bh,bl     ; BH is the number of bytes for the bitmap
        mov     bl,0      ; BL is the RAM table to manipulate
        mov     cx,1      ; CX is the number of characters we are defining 
        mov     dx,[bp].chToMap  ; DX is the character to remap
        push    bp
        les     bp,[bp].lpBitmap ; ES:BP has the address of the bitmap
        int     10H
        pop     bp

	mov	sp,bp
	pop	bp
	ret	8

VIOMAPCHARBITMAP endp

;---------------------------------------------------------------------

PAGE_SELECT EQU 0B2H
PAGE_MASK   EQU 0E1H

pageargs struc
         dw      3 dup (?)    ; bp and far return
iPage    dw      ?
pageargs ends

	public  ATIPAGESELECT
ATIPAGESELECT PROC FAR
    push    bp
    mov     bp,sp

    push    di
    mov     ax,0C000H
    mov     es,ax
    mov     di,0010H
    mov     dx, word ptr es:[di]
    mov     cx,[bp].iPage
    mov     ch,cl
    cli
    mov     al,PAGE_SELECT
    out     dx,al
    inc     dl
    in      al,dx
    mov     ah,al
    and     ah,PAGE_MASK
    shl     ch,1
    or      ah,ch
    mov     al,PAGE_SELECT
    dec     dl
    out     dx,ax
    sti
    pop     di

    mov     sp,bp
    pop     bp
    ret     2
ATIPAGESELECT endp

;---------------------------------------------------------------------

;
; Name:		PixelAddr06
;
; Function:	Determine buffer address of pixel in 640x200 2-color mode
;
; Caller:	AX = y-coordinate (0-199)
;		BX = x-coordinate (0-639)
;
; Returns:	AH = bit mask
;		BX = byte offset in buffer
;		CL = number of bits to shift left
;		ES = video buffer segment
;


OriginOffset	EQU	0		; byte offset of (0,0)

		PUBLIC	PixelAddr06
PixelAddr06	PROC	near

		mov	cl,bl		; CL := low-order byte of x

		xchg	ah,al		; AX := 100h * y
		shr	bx,1		; BX := x/2
		shr	ax,1		; AL := 80h*(y&1)
		add	bh,al	  	; BX := x/2 + 8000h*(y&1)
		xor	al,al		; AX := 100h*(y/2)
		add	bx,ax	  	; BX := x/2 + 8000h*(y&1) + 100h*(y/2)
		shr	ax,1
		shr	ax,1		; AX := 40h*(y/2)
		add	bx,ax	  	; BX := x/2 + 8000h*(y&1) + 140h*(y/2)
		shr	bx,1
		shr	bx,1	   	; BX := x/8 + 2000h*(y&1) + 50h*(y/2)
		add	bx,OriginOffset	; BX := byte offset in video buffer

		mov	ax,VideoBufferSeg
		mov	es,ax		; ES:BX := byte address of pixel

		and	cl,7		; CL := x & 7
		xor	cl,7		; CL := number of bits to shift left
		mov	ah,1		; AH := unshifted bit mask

		ret

PixelAddr06	ENDP



VARmask		EQU	[bp-2]
VARtoggle	EQU	[bp-4]

	PUBLIC  WRTGRAPHICSCHAR6

WRTGRAPHICSCHAR6 PROC FAR

	push    bp
        mov     bp,sp
        sub	sp,4
        push	di
        push	si
        push	ds
        push    es

; set up foreground pixel toggle mask
;
; ah = ((attr & 0x0F) != 0) ? 1 : 0;
;       mov	ax,[bp].attr
        xor     ah,ah
;       test    al,0FH
;       jz      wgc6_1
        inc     ah
        
wgc6_1:	        		; AH := 0 or 1 (foreground pixel value)
	ror	ah,1		; high-order bit of AH := 0 or 1
	cwd			; propagate high-order bit through DX
	not	dx		; DX :=     0 if foreground = 1
				;    or FFFFh if foreground = 0
	mov	VARtoggle,dx

; Calculate the address of the upper-left pixel. Returned in ES:BX
        mov	ax,[bp].y
        mov	bx,[bp].x
	call	PixelAddr06	; ES:BX -> buffer
				; CL := # bits to shift left

	xor	cl,7		; CL := # bits to rotate right

	mov	ax,0FF00h
	ror	ax,cl		; AX := bit mask in proper position
	mov	VARmask,ax


; set up video buffer addressing
	mov	dx,2000h	; increment for video buffer interleave
	mov	di,80-2000h	; increment from last to first interleave

	test	bx,2000h	; set zero flag if BX in 1st interleave
	jz	wgc6_2

	xchg	di,dx		; exchange increment values if 1st pixel
				;  lies in 1st interleave

; set up character definition table addressing
wgc6_2:
	push	bx		; preserve buffer address

	mov	ax,40h
	mov	ds,ax		; DS := segment of BIOS Video
				;  Display Data area
	mov	ch,ds:[85h]	; CH := POINTS (pixel rows in character)

	xor	ax,ax
	mov	ds,ax		; DS := absolute zero

	mov	ax,[bp].char	; AL := character code
	cmp	al,80h
	jae	wgc6_3

	mov	bx,43h*4	; DS:BX -> int 43h vector if char < 80h
	jmp	short wgc6_4

wgc6_3:
	mov	bx,1Fh*4	; DS:BX -> int 1Fh vector if char >= 80h
	sub	al,80h		; put character code in range of table 

wgc6_4:
	lds	si,ds:[bx]	; DS:SI -> start of character table
	mul	ch		; AX := offset into char def table
				;  (POINTS * char code)
	add	si,ax		; SI := addr of char def

	pop	bx		; restore buffer address

	test	cl,cl		; test # bits to rotate
	jnz	wgc6_6		; jump if character is not byte-aligned


; routine for byte-aligned characters
	mov	ah,VARtoggle	; AH := foreground toggle mask
	xchg	ch,cl		; CX := POINTS
		
wgc6_5:
	lodsb			; AL := bit pattern for next pixel row
	xor	al,ah		; toggle pixels if foreground = 0
	mov	es:[bx],al	; store pixels in buffer

	add	bx,dx		; BX := next row in buffer
	xchg	di,dx		; swap buffer increments
	loop	wgc6_5
	jmp	short wgc6_exit


; routine for non-byte-aligned characters

wgc6_6:
	mov	ax,VARmask
	and	es:[bx],ax	; mask character pixels in buffer

	xor	ah,ah
	lodsb			; AX := bit pattern for next pixel row
	xor	al,VARtoggle	; toggle pixels if foreground = 0

	ror	ax,cl		; rotate pixels into position
	or	es:[bx],ax	; store pixels in buffer

	add	bx,dx		; BX := next row in buffer
	xchg	di,dx		; swap buffer increments
	dec	ch
	jnz	wgc6_6


wgc6_exit:
        pop     es
	pop	ds
        pop	si
        pop	di
        mov	sp,bp
        pop	bp
        ret     8

WRTGRAPHICSCHAR6  ENDP






;---------------------------------------------------------------------


        public  _dosioctl

iostr struc
        dw      3 dup (?)    ; bp and far return
handle  dw      ?
subfunc dw      ?
rcx     dw      ?
rdx     dw      ?
rds     dw      ?
oax     dd      ?
odx     dd      ?
iostr ends

_dosioctl proc	far
	push	bp
	mov     bp,sp
	push	ds
	mov	ax,[bp].subfunc
	mov	ah,44h
	mov	bx,[bp].handle
	mov	cx,[bp].rcx
	mov	dx,[bp].rdx
	mov	ds,[bp].rds
	int	21h
	jc	ioctl_err

;       if (oax)  *oax = ax;
	lds	bx,DWORD PTR [bp].oax
	mov	cx,ds
	or	cx,bx
	jz	no_oax
	mov	[bx],ax

no_oax:
;       if (odx)  *odx = dx;
	lds	bx,DWORD PTR [bp].odx
	mov	cx,ds
	or	cx,bx
	jz	no_odx
	mov	[bx],dx

no_odx:
	xor	ax,ax

ioctl_err:
	pop	ds
	pop	bp
        ret
_dosioctl endp


	END

