[global initial_switch_dir]
[section .init.text]		; This function disables paging, so we need the EIP to point to a physical address
;[function initial_switch_dir]


; Disable paging, turn of 4MB pages, set the new page directory and reenable paging!
initial_switch_dir:
	mov eax,[esp+4]		; grab the value for cr3
	
	mov ebx,cr0
	and ebx,0x7FFFFFFF	; Disable paging
	mov cr0,ebx
	
	mov ebx,cr4
	and ebx,0xFFFFFFEF	; Remove the 4mb page flags
	mov cr4,ebx
	
	mov cr3,eax		; Set the new page directory
	
	mov ebx,cr0
	or ebx,0x80000000	; Enable paging
	mov cr0,ebx
	
	lea eax,[.continue]
	jmp eax

[section .text]
	
.continue:
	ret