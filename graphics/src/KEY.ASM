if TC
  include tc8086.mac
else
  include lm8086.mac
endif

	DATASEGM
	ENDDS

  PROGSEG    key

        public  _keyready

_keyready  proc    far
        push    bp
        mov     bp,sp

        mov     ax,0100H                ; ah=01 is standard kbd poller svc
        mov     bx,word ptr [bp+6]      ; offset of enhanced indicator
        cmp     bx,0
        jz      doit
        mov     ax,1100H                ; ah=11 is enhanced kbd poller svc
doit:
	int     16H
	;
	;If Ctrl-Break was hit, we see a case where the zero flag comes back
	;clear, and ax is equal to zero.  If we don't set ax to 1, then in that
	;case, the null character will stay in the buffer, and we will always
	;return "no character ready" - this is Not A Good Thing.
	;
	;This case arose from the assumption that after this call, zero flag set
	;implied ax != 0.  In fact, there is at least one case in which the zero
	;flag can be set, and ax == 0.
	;
	;Note that this new logic breaks one behavior of keyread() - in the old
	;version, the value that keyready() returned was equal to zero, or to
	;the value of the key which is at the head of the buffer.  Now, the
	;return value is 0 if no character is available, or 1 if a character
	;is in the buffer
	;
	mov     ax, 1
	jnz     k1
        xor     ax,ax
k1:
        mov     sp,bp
        pop     bp
        ret
_keyready endp

;-------------------------------------------------------------------------
;    We will put this two extremely short DOS interface calls here
;-------------------------------------------------------------------------

        public DOS3CALL
DOS3CALL proc  far
         int    21H
         ret
DOS3CALL endp

        public NETBIOSCALL
NETBIOSCALL proc  far
         int    5CH
         ret
NETBIOSCALL endp


if TC
  endps key
else
  endps
endif
        end
