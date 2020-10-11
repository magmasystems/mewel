;------------------------------------------------------------------------------
;
;  VOID PASCAL lwmemset(LPWORD lpDest, WORD wCell, WORD nCount);
;
;    Sets the memory location at lpDest to nCount words of wCell. Note, this
;  function was designed to blast char/attr pairs to video memory, and
;  hence is not a general purpose function. It flips the contents of wCell
;  to get the char/attr pairs in the right order.
;
;------------------------------------------------------------------------------

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

	DATASEGM
	ENDDS

        PROGSEG    lwmemset

        public  LWMEMSET
        public  LIMEMSET

setstr struc
        dw      3 dup (?)    ; bp and far return
len     dw      ?
cell    dw      ?
dest    dd      ?
setstr ends

LWMEMSET proc   far
        push    bp
        mov     bp,sp
        push    di
        cld

        mov     cx,[bp].len             ; number of words to replicate in cx
        mov     ax,[bp].cell            ; the word to replicate in ax
        xchg    al,ah                   ; cause stosw stores in reverse
        les     di,dword ptr [bp].dest  ; get the destination pointer
rep     stosw                           ; copy the source to the destination

        pop     di
        mov     sp,bp
        pop     bp
        ret     8

LWMEMSET endp


LIMEMSET proc   far
        push    bp
        mov     bp,sp
        push    di
        cld

        mov     cx,[bp].len             ; number of words to replicate in cx
        mov     ax,[bp].cell            ; the word to replicate in ax
        les     di,dword ptr [bp].dest  ; get the destination pointer
rep     stosw                           ; copy the source to the destination

        pop     di
        mov     sp,bp
        pop     bp
        ret     8
LIMEMSET endp


if TC
        endps lwmemset
else
        endps
endif
        end
