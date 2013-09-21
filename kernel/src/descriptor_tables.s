;
; Author: Caleb Stewart
; Module: descriptor_tables.s
; Purpose:
;	Defines assembly routines related to the interrupt and global
;	descriptor tables.
;


;
; Function: flush_gdt
; Parameters:
;	void* ptr -- Pointer to the GDT pointer structure
; Return value: None
;
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

;
; Function: flush_idt
; Parameters:
;	void* ptr -- Pointer to the IDT pointer structure
; Return value: None
;
[global flush_idt]
flush_idt:
	mov eax,[esp+4]		; Grab the pointer
	lidt [eax]		; load the new idt
	ret

; This is a nice litte macro to define an ISR handler that does not need to
; provide a dumby error code.
%macro ISR_WITHERR 1
	[global isr%1]
	isr%1:
		cli
		push byte %1
		jmp isr_stub
%endmacro

; Same as above, but with a dumby error code
%macro ISR_NOERR 1
	[global isr%1]
	isr%1:
		cli
		push byte 0
		push byte %1
		jmp isr_stub
%endmacro

; Definitions of ISR handlers
ISR_NOERR	0
ISR_NOERR	1
ISR_NOERR	2
ISR_NOERR	3
ISR_NOERR	4
ISR_NOERR	5
ISR_NOERR	6
ISR_NOERR	7
ISR_WITHERR	8
ISR_NOERR	9
ISR_WITHERR	10
ISR_WITHERR	11
ISR_WITHERR	12
ISR_WITHERR	13
ISR_WITHERR	14
ISR_NOERR	15
ISR_NOERR	16
ISR_NOERR	17
ISR_NOERR	18
ISR_NOERR	19
ISR_NOERR	20
ISR_NOERR	21
ISR_NOERR	22
ISR_NOERR	23
ISR_NOERR	24
ISR_NOERR	25
ISR_NOERR	26
ISR_NOERR	27
ISR_NOERR	28
ISR_NOERR	29
ISR_NOERR	30
ISR_NOERR	31
ISR_NOERR	32

;
; Function: isr_stub
; Parameters: None
; Purpose:
;	Save registers, switch to kernel segments, and call the C isr handler
;
[global isr_stub]
[extern isr_handler]	; This is the C handler for interrupt service routines
isr_stub:
	pusha		; Push all common registers
	
	mov ax,ds	; we save the data segment through eax
	push eax
	
	; Load kernel segment selectors
	mov ax,0x10
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	
	call isr_handler
	
	; Restore old segment selectors
	pop eax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	
	; Restore common registers
	popa
	; Remove ISR # and error code from the stack
	add esp,8
	; Re-enable interrupts
	sti
	iret		; Pops CS, EIP, EFLGS, SS, ESP