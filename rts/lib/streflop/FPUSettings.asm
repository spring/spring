PUBLIC __streflop_fstcw, __streflop_fldcw, __streflop_stmxcsr, __streflop_ldmxcsr

.code

_tmp$ = -8
__streflop_fstcw proc
	push RCX
	fstcw WORD PTR _tmp$[RSP+8]
	mov AX, WORD PTR _tmp$[RSP+8]
	pop RCX
	ret
__streflop_fstcw endp

_tmp$ = -8
__streflop_fldcw proc
	push RCX
	mov WORD PTR _tmp$[RSP+8], CX
	fclex
	fldcw WORD PTR _tmp$[RSP+8]
	pop RCX
	ret
__streflop_fldcw endp

_tmp$ = -8
__streflop_stmxcsr proc
	push RCX
	stmxcsr DWORD PTR _tmp$[RSP+8]
	mov EAX, DWORD PTR _tmp$[RSP+8]
	pop RCX
	ret
__streflop_stmxcsr endp

_tmp$ = -8
__streflop_ldmxcsr proc
	push RCX
	mov DWORD PTR _tmp$[RSP+8], ECX
	ldmxcsr DWORD PTR _tmp$[RSP+8]
	pop RCX
	ret
__streflop_ldmxcsr endp

end
