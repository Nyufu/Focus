public FiberInitializer
public SwitchToFocusFiber

Fiber STRUCT
	stackCloser			qword ?
	stackLimit			qword ?
	stackSize			qword ?
	freeSetPlace		qword ?
	stackPointer		qword ?
Fiber ENDS

TaskCommonPartSize		equ 8h

TaskCommon STRUCT
	refCount			qword ?
    fiber_				Fiber <>
TaskCommon ENDS

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
	.endprolog
	;lea         rdx, offset FiberRunner
	;lea rdx, [rip - 017h] ; 10 = the offsett of FiberRunner; 7 = size of lea rdx, []
	DB 48h, 8Dh, 15h, 0E9h, 0FFh, 0FFh, 0FFh		; This works even debug mode

	mov			rbp, (Fiber ptr[rcx]).stackCloser

	mov			rbx, qword ptr[rsp+8]				; the FiberRunner will use the address of Finish function from rbx
	mov			qword ptr[rsp+8], rdx				; set the FiberRunner as the first function in the callstack

	mov			rcx, qword ptr[rsp+10h]
	movsd		xmm0, mmword ptr[rsp+10h]
	mov			rdx, qword ptr[rsp+18h]
	movsd		xmm1, mmword ptr[rsp+18h]
	mov			r8, qword ptr[rsp+20h]
	movsd		xmm2, mmword ptr[rsp+20h]
	mov			r9, qword ptr[rsp+28h]
	movsd		xmm3, mmword ptr[rsp+28h]

	ret
FiberInitializer endp


ALIGN 16
SwitchToFocusFiber proc frame
	.endprolog
	mov			rax, qword ptr gs:[30h]				; save the current TEB's address to rax

	mov			rdx, (Fiber ptr[rcx]).stackLimit
	mov			rsp, (Fiber ptr[rcx]).stackPointer

	mov			qword ptr [rax+8], rcx				; store the new StackBase
	mov			qword ptr [rax+10h], rdx			; store the new StackLimit
	mov			qword ptr [rax+20h], rcx			; store the new fiber's address into the TIB.FiberData

	ret
SwitchToFocusFiber endp

end