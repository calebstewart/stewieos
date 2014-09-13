[global spin_lock:function]
spin_lock:
	mov edx,1 ; the value we want in the lock
	mov ecx,[esp+4]
	pushf
	sti
.loop0:
	; if( [ecx] == eax ){
	;	[ecx] = edx
	;	ZF = 1
	; } else {
	; 	eax = edx
	; 	ZF = 0
	; }
	mov eax,0 ; the test value
	mov edx,1
	lock cmpxchg dword[ecx],edx
	jnz .loop0 ; If the zero flag is not set, then we didn't aquire the lock
.return:
	popf
	ret

[global spin_unlock:function]
spin_unlock:
	mov edx,[esp+4]
	mov eax,0
	lock xchg dword[edx],eax
	ret