;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------

init:
			bic.w	#0001h, &PM5CTL0  		; disabled the GPIO power-on HighZ
			bis.b	#01h, &P1DIR			; setting the P1.0 as an output

main:
			call	#FlashRED
			jmp 	main					; repeat main loop forever





;-------------------------------------------------------------------------------
; FlashRED Subroutine
;-------------------------------------------------------------------------------
FlashRED:
			xor.b	#01h, &P1OUT			; toggle P1.0 (LED1)
			mov.w	#10, R15				; puts number of 100ms delays I need (since 1s, I want ten 100ms delays (started with 0xFFFF)
			call 	#Delay
			ret

;-------------------------------------------------------------------------------
; Delay Subroutine
;-------------------------------------------------------------------------------
Delay:
beginning:
			mov.w 	#06662h, R14			; move 0x0FFF to R14 (inner loop variable)

ifInner:	dec  	R14						; decrement inner loop variable
			cmp.w 	#0, R14					; compare if inner loop variable is 0
			jnz		ifInner					; if not zero go back to decrementing it


			dec 	R15						; if equal to 0, decrement outer loop
			cmp.w	#0, R15					; and check if outer loop is 0


			jnz		beginning				; if not 0, go back to the beginning.
			ret								; if 0, return from subroutine
                                            

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
