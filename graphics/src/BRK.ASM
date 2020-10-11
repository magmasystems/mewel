;----------------------------------------------------------------------------
;  BRK.ASM
;    Control-Break handler for MEWEL
;
; (C) Copyright 1989  Marc Adler/Magma Systems
;----------------------------------------------------------------------------

if TC
  include tc8086.mac
  PROGSEG brk
else
  include lm8086.mac
  PROGSEG
endif

oldint23     dd  ?         ; address of the old int 23H handler
oldint1B     dd  ?         ; address of the old int 1BH handler
BrkFoundAddr dd  ?         ; far byte ptr to the CTRL-BREAK semaphore


public  _int23ini, _int23res, _int23rtn, _int1Brtn

;--------------------------------------------------------------------------
; void INT23INI(char far *pBrkSemaphore)
;   Sets our own handlers for int 23H and int 1BH
;--------------------------------------------------------------------------
_int23ini proc    far
        push    bp
        mov     bp,sp
        push    ds
        push    es
        push    si
        push    di

        mov     ax,3523H        ; get the old interrupt 23H into es:bx
        int     21H
        mov     si,offset cs:oldint23
        mov     cs:[si],bx
        mov     cs:[si+2],es
        mov     ax,351BH        ; get the old interrupt 1BH into es:bx
        int     21H
        mov     si,offset cs:oldint1B
        mov     cs:[si],bx
        mov     cs:[si+2],es

        push    cs
        pop     ds
        mov     dx,offset _int23rtn
        mov     ax,2523H        ; make int 23H point to new handler
        int     21H
        mov     dx,offset _int1Brtn
        mov     ax,251BH        ; make int 1BH point to new handler
        int     21H

; Store the address of the CTRL-BREAK semaphore into BrkFoundAddr
        lea     bx,BrkFoundAddr
        mov     ax,word ptr [bp+6]      ; offset of break indicator
        mov     cs:[bx+0],ax
        mov     ax,word ptr [bp+8]      ; segment of break indicator
        mov     cs:[bx+2],ax

        pop     di
        pop     si
        pop     es
        pop     ds
        mov     sp,bp
        pop     bp
        ret
_int23ini endp

;--------------------------------------------------------------------------
; void INT23RES(void)
;   Restores the old int 23H and int 1BH handlers
;--------------------------------------------------------------------------
_int23res proc   far
        push    bp
        mov     bp,sp
        push    ds
        push    si

        mov     si,offset cs:oldint23
        mov     dx,cs:[si+0]
        mov     ds,cs:[si+2]
        mov     ax,2523H        ; make int 23H point to new handler
        int     21H

        lea     si,cs:oldint1B
        mov     dx,cs:[si+0]
        mov     ds,cs:[si+2]
        mov     ax,251BH        ; make int 23H point to new handler
        int     21H

        pop     si
        pop     ds
        mov     sp,bp
        pop     bp
        ret
_int23res endp


;--------------------------------------------------------------------------
; void INT23RTN(void)
;   This is the int 23H handler  
;--------------------------------------------------------------------------
_int23rtn proc   far
         iret
;        jmp    short _int1Brtn
;        jmp    dword ptr cs:oldint23
_int23rtn endp


;--------------------------------------------------------------------------
; void INT1BRTN(void)
;   This is the int 1BH handler  
;--------------------------------------------------------------------------
_int1Brtn proc   far
         push   es
         push   bx

; Set the CTRL-BREAK semaphore so the app knows that we got here
         les    bx,dword ptr cs:BrkFoundAddr
         mov    byte ptr es:[bx],1

;        mov    bx,40H
;        mov    es,bx
;        mov    byte ptr es:[71H],0

         pop    bx
         pop    es
         iret
;        jmp    dword ptr cs:oldint1B
_int1Brtn endp


if TC
  endps brk
else
  endps
endif
        end
