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
;----- setup LED
			bis.b	#BIT0, &P1DIR			; set P1.0 to output
			bic.b	#BIT0, &P1OUT			; turn LED1 OFF
			bic.b	#LOCKLPM5, &PM5CTL0		; enable digital I/O

;----- setup Timer B0
			bis.w	#TBCLR, &TB0CTL				; clear TB0
			bis.w	#TBSSEL__ACLK, &TB0CTL		; choose ACLK
			bis.w	#MC__CONTINUOUS, &TB0CTL 	; put it to continuous mode
			bis.w	#ID__8, &TB0CTL				; divide by 8
			bis.w	#CNTL_1, &TB0CTL			; binary count to 2^12


;----- setup overflow IRQ
			bis.w 	#TBIE, &TB0CTL			; local enable for TB0 overflow
			eint							; enable global for maskables
			bic.w	#TBIFG, &TB0CTL			; clear flag for first use


main:
			jmp		main



;-------------------------------------------------------------------------------
; Interrupt service routing
;-------------------------------------------------------------------------------

ISR_TB0_Overflow:
			xor.b 	#BIT0, &P1OUT			; toggle LED1
			bic.w	#TBIFG, &TB0CTL			; clear flag for first use
			reti


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
            
            .sect	".int42"				; init vector table for TB0
            .short 	ISR_TB0_Overflow

