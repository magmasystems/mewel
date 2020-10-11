;  lmemmove.asm
;
;  LPSTR *PASCAL lmemmove(LPSTR dest, LPSTR src, WORD len)
;  Works like lmemcpy, but insures that overlapping areas are moved correctly
; 
;  IMPORTANT NOTE -
;    This is a special version which only deals with screen cells. It cannot
;  be used as a general purpose lmemmove.
;

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

  DATASEGM
  ENDDS

  PROGSEG    lmemmove

cpystr  struc
        dw      3 dup (?)    ; bp and far return
len     dw      ?
src     dd      ?
dest    dd      ?
cpystr  ends

  public  LMEMMOVE
LMEMMOVE PROC FAR

  push  bp
  mov   bp,sp

  push  di
  push  si
  push  ds
  push  es

  lds   si, [bp].src
  les   di, [bp].dest
  mov   cx, [bp].len

  mov   ax, di        ; get the distance between the dest and source
  sub   ax, si
  test  ax, 8000H
  jnz   minus

plus:               ; the dest is to the right of the source
  add si, cx        ; add the distances to the source & dest
  sub si, 2
  add di, cx
  sub di, 2

; MOVE THE PROPER AMOUNT OF WORDS
  std
  shr  cx,1
  rep  movsw
; MOVE THE (possible) REMAINING BYTE
  adc  cx,cx
  rep  movsb
  jmp bye

minus:
; We are moving leftwards
; MOVE THE PROPER AMOUNT OF WORDS
  cld
  shr  cx,1
  rep  movsw
; MOVE THE (possible) REMAINING BYTE
  adc  cx,cx
  rep  movsb

bye:
  cld
  pop  es
  pop  ds
  pop  si
  pop  di

  mov  sp,bp
  pop  bp
  ret  10

LMEMMOVE endp


  public  LMEMSHR
LMEMSHR PROC FAR

  push  bp
  mov   bp,sp

  push  di
  push  si
  push  ds
  push  es

  lds   si, [bp].src
  les   di, [bp].dest
  mov   cx, [bp].len

  ; the dest is to the right of the source
  add si, cx        ; add the distances to the source & dest
  add di, cx
  sub si, 2
  sub di, 2

; SHR THE PROPER AMOUNT OF WORDS
  std

  cmp  cx,1
  je   moveone

  shr  cx,1
  rep  movsw
; SHR THE (possible) REMAINING BYTE
  adc  cx,cx

moveone:
  cmp  cx,1
  jne  bye2
  inc  di
  inc  si
  rep  movsb

bye2:
  cld
  pop  es
  pop  ds
  pop  si
  pop  di

  mov  sp,bp
  pop  bp
  ret  10

LMEMSHR endp


if TC
  endps lmemmove
else
  endps
endif
  end

