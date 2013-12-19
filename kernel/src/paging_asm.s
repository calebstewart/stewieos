[global copy_physical_frame:function]

copy_physical_frame:		; void copy_physical_frame(u32*parm1,u32*parm2)
	mov eax,256		; u32 counter = 256;
	mov esi,[esp]		; u32* src = parm2;
	mov edi,[esp+4]		; u32* dst = parm1;
	; no good C explanation: save eflags, disable interrupts and paging
	pushf
	cli
	mov ecx,cr0
	and ecx,0x7fffffff
	mov cr0,ecx
.loop0:				; while( 1 ) {
	mov ecx,dword [esi]	; 	*dst = *src;
	mov dword[edi],ecx	;
	add edi,4		; 	dst = dst + 4;
	add esi,4		; 	src = src + 4;
	dec eax			; 	--counter;
	cmp eax,0		; 	if(counter==0) break;
	je .loop0		; 	else continue;
.done:				; }
	; Enable interrupts and restore eflags->and interrupt state
	mov ecx,cr0
	and ecx,0x80000000
	mov cr0,ecx
	popf
	
	ret			; return;