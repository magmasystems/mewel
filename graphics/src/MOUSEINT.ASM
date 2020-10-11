.MODEL LARGE

        ASSUME  DS: DGROUP, SS: DGROUP
extrn   __MouseInt:word

.CODE

_MouseHandler PROC
              public  _MouseHandler

  push ds
  pushf

; Switch over to the C data segment
  mov  ax,word ptr DGROUP
  mov  ds,ax
  mov  word ptr DGROUP:__MouseInt,1

  popf
  pop  ds
  retf

_MouseHandler ENDP
END

