public FiberInitializer
public SwitchToFocusFiber

Fiber STRUCT
	stackLimit			qword ?
	stackSize			qword ?
	freeSetPlace		qword ?
	stackPointer		qword ?
Fiber ENDS

FiberInitData STRUCT
	finish				qword ?
	taskFunction		qword ?
	basePointer			qword ?
	nextInstruction		qword ?
FiberInitData ENDS

.code

ALIGN 16
FiberRunner proc frame
	.setframe	rbp, 0								; the rbp points to the bottom of the stack
	.endprolog

	mov			rcx, rsp							; set the taskCallerDomain as this
	mov			rdx, rax							; forward the POD result to the Finish function as 1. parameter
	push		0									; set the Finish as first function in the callstack
	jmp			rbx									; now switch to the Finish
FiberRunner endp


ALIGN 16
FiberInitializer proc frame
	;lea rbx, [rip - 017h] ; 10 = the offsett of FiberRunner; 7 = size of lea rbx, []
	DB 48h, 8Dh, 1Dh, 0E9h, 0FFh, 0FFh, 0FFh
	mov			qword ptr[rsp], rbx					; set the FiberRunner as the first function in the callstack

	mov			rbp, qword ptr[rsp-8]				; now the rbp points to the bottom of the stack
	.endprolog

	mov			rbx, qword ptr[rsp-18h]				; the FiberRunner will use the address of Finish function from rbx
	mov			rax, qword ptr[rsp-10h]				; load the taskFunction

	mov			rcx, qword ptr[rsp+8]
	movsd		xmm0, mmword ptr[rsp+8]
	mov			rdx, qword ptr[rsp+10h]
	movsd		xmm1, mmword ptr[rsp+10h]
	mov			r8, qword ptr[rsp+18h]
	movsd		xmm2, mmword ptr[rsp+18h]
	mov			r9, qword ptr[rsp+20h]
	movsd		xmm3, mmword ptr[rsp+20h]

	jmp			rax									; the taskFunction
FiberInitializer endp


ALIGN 16
SwitchToFocusFiber proc frame
	mov			rsp, (Fiber ptr[rcx]).stackPointer
	.endprolog

	mov			rax, qword ptr gs:[30h]				; save the current TEB's address to rax

	mov			rdx, (Fiber ptr[rcx]).stackLimit

	mov			r8, qword ptr[rsp]					; loads the nextInstruction

	mov			qword ptr [rax+8], rcx				; store the new StackBase
	mov			qword ptr [rax+10h], rdx			; store the new StackLimit
	mov			qword ptr [rax+20h], rcx			; store the new fiber's address into the TIB.FiberData

	jmp			r8									; jumps to the nextInstruction which the FiberInitializer or the interrupted task
SwitchToFocusFiber endp

end