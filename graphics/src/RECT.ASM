;  rect.asm
;
;

if TC
	include tc8086.mac
MSC  equ 0
else
	include lm8086.mac
endif

if TC
ALIGNMNT  EQU     0    ; TC starts the rect structure at bp+6
else
ALIGNMNT  EQU     2    ; MSC starts the rect structure at bp+8
endif


	DATASEGM
	ENDDS

	PROGSEG  rect

_rect  struc
left    dw ?
top     dw ?
right   dw ?
bottom  dw ?
_rect  ends


	public INTERSECTRECT


if LMODEL

INTERSECTRECT proc far

	push  bp
	mov   bp,sp
	push  si
	push  di
	push  ds

	lds si, [bp+6]
	les di, [bp+6+4]
	mov ax, [si].left
	mov bx, es:[di].left
	cmp ax, bx
	jge ir_1
	mov ax, bx
ir_1:
	mov bx, [si].right
	mov cx, es:[di].right
	cmp bx, cx
	jle ir_2
	mov bx, cx
ir_2:
	cmp ax, bx
	jge ir_e

	mov cx, [si].top
	mov dx, es:[di].top
	cmp cx, dx
	jge ir_3
	mov cx, dx
ir_3:
	mov dx, [si].bottom
	mov si, es:[di].bottom
	cmp dx, si
	jle ir_4
	mov dx, si
ir_4:
	cmp cx, dx
	jge ir_e
	mov di, 1

ir_exit:
	lds si, [bp+6+4+4]      ; ds:si is the destination rect
	mov [si].left, ax
	mov [si].right, bx
	mov [si].top, cx
	mov [si].bottom, dx
	mov ax, di

	pop ds
	pop di
	pop si
	pop	bp
	ret 12

ir_e:
	xor ax, ax
	xor bx, bx
	xor cx, cx
	xor dx, dx
	xor di, di
	jmp ir_exit

INTERSECTRECT endp

else

INTERSECTRECT proc far

	push  bp
	mov   bp,sp
	push  si
	push  di

	mov si, [bp+6]
	mov di, [bp+6+2]
	mov ax, [si].left
	mov bx, [di].left
	cmp ax, bx
	jge ir_1
	mov ax, bx
ir_1:
	mov bx, [si].right
	mov cx, [di].right
	cmp bx, cx
	jle ir_2
	mov bx, cx
ir_2:
	cmp ax, bx
	jge ir_e

	mov cx, [si].top
	mov dx, [di].top
	cmp cx, dx
	jge ir_3
	mov cx, dx
ir_3:
	mov dx, [si].bottom
	mov si, [di].bottom
	cmp dx, si
	jle ir_4
	mov dx, si
ir_4:
	cmp cx, dx
	jge ir_e
	mov di, 1

ir_exit:
	mov si, [bp+6+2+2]
	mov [si].left, ax
	mov [si].right, bx
	mov [si].top, cx
	mov [si].bottom, dx
	mov ax, di

	pop di
	pop si
	pop	bp
	ret 6

ir_e:
	xor ax, ax
	xor bx, bx
	xor cx, cx
	xor dx, dx
	xor di, di
	jmp ir_exit

INTERSECTRECT endp

endif


if TC
	endps rect
else
	endps
endif
	end

