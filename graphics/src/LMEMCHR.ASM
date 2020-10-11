;  lmemchr.asm
;
;  int pascal far lstrnlen(char far *s, int c, unsigned n)
;

if TC
  include tc8086.mac
else
  include lm8086.mac
endif

  DATASEGM
  ENDDS

  PROGSEG    lmemchr

strlen  struc
        dw      3 dup (?)    ; bp and far return
n       dw      ?
chr     dw      ?
s       dd      ?
strlen  ends

  public LMEMSCAN
LMEMSCAN PROC FAR
  push  bp
  mov   bp,sp
  mov   bx,di

  les di, [bp].s
  mov ax, [bp].chr
  mov cx, [bp].n
  jcxz notfound1

  cld

  repne scasb
  mov ax, [bp].n
  jne _l1

  sub ax,cx
  dec ax
  jmp short _l1

notfound1:
  mov  ax, 0

_l1:
  mov  di,bx
  pop  bp
  ret  8
LMEMSCAN endp


  public LMEMCHR
LMEMCHR PROC FAR
  push  bp
  mov   bp,sp
  mov   bx,di

  les  di,[bp].s
  mov  ax,[bp].chr
  mov  cx,[bp].n
  jcxz notfound2

  cld
  repne scasb
  jne  short notfound2

  ; Found the char
  dec  di
  mov  ax,di
  mov  dx,es
  jmp  short bye

notfound2:
  xor  ax,ax
  xor  dx,dx

bye:
  mov  di,bx
  pop  bp
  ret  8
LMEMCHR endp



if TC
  endps lmemchr
else
  endps
endif
  end
