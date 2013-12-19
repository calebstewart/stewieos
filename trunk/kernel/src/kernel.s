;
; Function: disablei
; Parameters:
;	None
; Purpose:
;	Save the eflags, disable interrupts and return the old eflags
;	This is a safe way to disable interrupts if you do not know
;	the state of the interrupt flag, just make sure to re-enable
;	them using the restore function, providing the eflags returned
;	by this function
; Returns:
;	u32 -- The eflags from before the call to CLI
[global disablei]
disablei:
	pushf
	pop eax
	cli
	ret

;
; Function: restore
; Parameters:
;	u32 eflags -- the value of eflags you wish to set (usually returned by disablei)
; Purpose:
;	Mainly just to re-enable interrupts after a call to disablei
;	It's a way to safely disable interrupts when you do not know
;	if they are already disabled or not.
;
[global restore]
restore:
	push dword [esp+4]
	popf
	ret
	
;
; Function: read_eip
; Parameters:
;	none.
; Purpose:
;	Reads the instruction pointer following the call to this
;	function.
;
[global read_eip]
read_eip:
	pop eax
	push eax
	ret