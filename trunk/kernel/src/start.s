;
; File: start.s
; Author: Caleb Stewart
; Date: 15Sep2013
; Purpose: Initial startup code for StewieOS, sets up stack and multiboot information
;
[global KERNEL_VIRTUAL_BASE]

MBOOT_PAGE_ALIGN	equ 1<<0						; We want the kernel to be loaded page aligned
MBOOT_MEM_INFO		equ 1<<1						; We want memory info from GRUB
MBOOT_HEADER_MAGIC	equ 0x1BADB002						; The magic number that grub looks for
MBOOT_HEADER_FLAGS	equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO			; Combine the flags
MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)		; Create a checksum value
KERNEL_VIRTUAL_BASE	equ 0xC0000000
KERNEL_PAGE_NUMBER	equ (KERNEL_VIRTUAL_BASE >> 22)

[bits 32]
[global multiboot_header]
[extern code]
[extern bss]
[extern end]
	
[extern main]
[global start]

;[global start]
;start equ (_start - KERNEL_VIRTUAL_BASE)

[section .init.data]

; This page directory will identity map the first 4MB and map the first 4MB to 0xC0000000
boot_page_directory:
	; 4MB Page, Read/Write, Present
	dd 0x00000083
	; Unmapped pages before kernel space
	times (KERNEL_PAGE_NUMBER-1) dd 0
	; 4MB page, Read/Write, Present
	dd 0x00000083
	; Unmapped pages after 4MB of kernel space
	times (1024-KERNEL_PAGE_NUMBER-1) dd 0

[section .init.text]

; Encode the multiboot header into the beginning of the executable
multiboot_header:
	dd MBOOT_HEADER_MAGIC
	dd MBOOT_HEADER_FLAGS
	dd MBOOT_CHECKSUM

; Entry point
start:
	; Set the CR3 register to point to our page directory
	mov ecx,(boot_page_directory)
	mov cr3,ecx
	
	; Enable 4MB pages
	mov ecx,cr4
	or ecx,0x00000010
	mov cr4,ecx	
	
	; Enable paging
	mov ecx,cr0
	or ecx,0x80000000
	mov cr0,ecx
	
	lea ecx,[higher_start]
	jmp ecx
	
[section .text]
	
higher_start:
	;  We leave it mapped so we can do some silly things later (see paging.s)
	;mov dword[boot_page_directory],0
	;invlpg [0]
	
	mov esp,stack+8192
	push eax
	
	push ebx
	cli
	call main
	jmp $

[section .bss]
stack:
	resb 8192