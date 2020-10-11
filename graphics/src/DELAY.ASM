;--------------------------------------------------------------------------
;
;  delay(int milliseconds)
;
;--------------------------------------------------------------------------

ifdef TC
        include tc8086.mac
else
        include lm8086.mac
endif

  DATASEGM
  ENDDS

        PROGSEG DosSleep

public _DosSleep
 
args    struc
          dw      3 dup (?)    ; bp and far return
millisecs dw      ?            ; millisecsonds to delay
args     ends


_DosSleep  proc  far
        push  bp
        mov    bp,sp
 
        push  si  ;save SI
        push  es  ;ES

        mov   cx,[bp].millisecs

;Find out which piece of hardware we are running in?
 
ifndef DOS286X
        mov   dx,275                    ;about 1 millisecond delay PC or (8088)
        les   si,cs:Machine_Id
        cmp   byte ptr es:[si],0FCH     ;is it the PC-AT (286)
        jnz   delay1
endif
        mov   dx,660                    ;about 1 millisecond delay AT or (286)
 
delay1: push  cx  ;save counter
        mov   cx,dx
 
delay2: loop  delay2  ;wait 1 millisecond
        pop   cx
        loop  delay1  ;do another millisecond
 
        pop   es      ;recover ES
        pop   si      ;recover SI
        pop   bp
        ret

Machine_Id  label  dword
        dw  0FFFEH
        dw  0F000H

_DosSleep  endp
ifdef TC
        endps DosSleep
else
        endps
endif
        end

