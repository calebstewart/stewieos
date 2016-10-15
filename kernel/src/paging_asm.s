[global copy_physical_frame:function]
[section .physcopy.text]

copy_physical_frame:		; void copy_physical_frame(u32 dest, u32 src) ATTR((cdecl));
	mov eax,0			; initialize counter
	mov esi,[esp+8]	; source pointer in esi
	mov edi,[esp+4]		; destination pointer in edi
	pushf				; save flags to restore interrupt state
	cli					; turn off interrupts
	; turn off paging
	mov ecx,cr0
	and ecx,0x7fffffff
	mov cr0,ecx

; Loop through all 256 DWORDS in the page
.loop0:
	; Copy source to ecx
	mov ecx,dword [esi+eax*4]
	; Copy ecx to destination
	mov dword[edi+eax*4],ecx
	; Increment index
	inc eax
	; compare index to 1024
	cmp eax,0x400
	; if index is less than 1024, continue copying
	jl .loop0

; After we copy 1024 dwords...
.done:
	; turn on paging
	mov ecx,cr0
	or ecx,0x80000000
	mov cr0,ecx
	popf				; restore flags (reverses our cli)
	ret