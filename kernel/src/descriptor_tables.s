[global flush_gdt]

flush_gdt:
	mov eax,[esp+4]		; Grab the pointer
	lgdt [eax]		; Load the new gdt
	
	; Reload segment registers
	mov ax,0x10
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	; do a far jump to completely flush the buffer
	jmp 0x08:.flush
; return
.flush:
	ret