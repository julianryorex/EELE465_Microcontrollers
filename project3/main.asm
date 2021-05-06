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
; R13 is the flag on whether the passcode was entered or not. R13 = 0 = YES ACCESS
;-------------------------------------------------------------------------------
init:
			bic.w	#LOCKLPM5, &PM5CTL0  	; disabled the GPIO power-on HighZ
			mov.b	#1h, R13				; R14 = 1, so no initial access

main:
			cmp.b	#0, R13
			jnz		enterKeyPass


			call	#CheckKeypad

			cmp.b	#81h, R6				; Click on 'A'
			jz		callLED1

			cmp.b	#41h, R6
			jz		callLED2

			jmp		endMain


enterKeyPass:
			call	#KeyPass
			jmp		endMain




callLED1:
			call	#LED1
			jmp		endMain

callLED2:
			call	#LED2
			jmp		endMain

endMain:
			jmp 	main


;-------------------------------------------------------------------------------
; KeyPass Subroutine (checks if the key code was entered
; R13 is used to set the flag of whether a password is needed in later iterations
;-------------------------------------------------------------------------------
; Key code: 4351
KeyPass:
Num1:
			call	#CheckKeypad			; check the pressed button
			cmp.b 	#48h, R10				; compare if button '4' was pressed
			jnz		Num1					; if wrong button, wait for next press

Num2:
			mov.b	#1, R15					; delay of 1ms
			call	#Delay					; call delay subroutine

			call	#CheckKeypad			; check the pressed button
			cmp.b 	#82h, R10				; compare if button '3' was pressed
			jnz		Num2

Num3:
			mov.b	#1, R15					; delay of 1ms
			call	#Delay					; call delay subroutine

			call	#CheckKeypad			; check the pressed button
			cmp.b 	#44h, R10				; compare if button '5' was pressed
			jnz		Num3

Num4:
			mov.b	#1, R15					; delay of 1ms
			call	#Delay					; call delay subroutine

			call	#CheckKeypad			; check the pressed button
			cmp.b 	#88h, R10				; compare if button '1' was pressed
			jnz		Num4



endKeyPass:
			mov.b	#0, R13
			ret


;-------------------------------------------------------------------------------
; LED1 Subroutine (Pattern 1 - XOXOXOXO)
;-------------------------------------------------------------------------------
LED1:
			; setup ports for LED bar as output
			; ports are not all P6 but they're in a straight line
			bis.b	#BIT0, &P6DIR			; setting P6.0 as output
			bis.b	#BIT1, &P6DIR			; setting P6.1 as output
			bis.b	#BIT2, &P6DIR			; setting P6.2 as output
			bis.b	#BIT3, &P6DIR			; setting P6.3 as output
			bis.b	#BIT4, &P6DIR			; setting P6.4 as output
			bis.b	#BIT7, &P3DIR			; setting P3.7 as output
			bis.b	#BIT5, &P2DIR			; setting P2.4 as output
			bis.b	#BIT6, &P6DIR			; setting P6.6 as output

			; initialize the pattern
			bic.b	#BIT0, &P6OUT			; OFF
			bis.b	#BIT1, &P6OUT			; ON
			bic.b	#BIT2, &P6OUT			; OFF
			bis.b	#BIT3, &P6OUT			; ON
			bic.b	#BIT4, &P6OUT			; OFF
			bis.b	#BIT5, &P2OUT			; ON
			bic.b	#BIT6, &P6OUT			; OFF
			bis.b	#BIT7, &P3OUT			; ON

			ret

;-------------------------------------------------------------------------------
; LED2 Subroutine (Pattern 2 - counts up to 255)
; may need interrrupts later
; R5 is the counter register for this subroutine
;-------------------------------------------------------------------------------
LED2:
			; setup ISR flags
			bic.b	#BIT0, &P1IFG			; clear P1.0 interrupt flag
			bis.b	#BIT0, &P1IE			; local enable P1.0
			bis.w	#GIE, SR				; global enable


			; setup ports
			bis.b	#BIT0, &P6DIR			; setting P6.0 as output
			bis.b	#BIT1, &P6DIR			; setting P6.1 as output
			bis.b	#BIT2, &P6DIR			; setting P6.2 as output
			bis.b	#BIT3, &P6DIR			; setting P6.3 as output
			bis.b	#BIT4, &P6DIR			; setting P6.4 as output
			bis.b	#BIT5, &P2DIR			; setting P2.5 as output <- 5 here for individual bits
			bis.b	#BIT6, &P6DIR			; setting P6.6 as output
			bis.b	#BIT7, &P3DIR			; setting P3.7 as output

			; initialize all as OFF
			bic.b	#BIT0, &P6OUT
			bic.b	#BIT1, &P6OUT
			bic.b	#BIT2, &P6OUT
			bic.b	#BIT3, &P6OUT
			bic.b	#BIT4, &P6OUT
			bic.b	#BIT5, &P2OUT
			bic.b	#BIT6, &P6OUT
			bic.b	#BIT7, &P3OUT




			; start
			mov.b	#0, R5					; initialize R4 count variable with 0
			jmp		callLoop				; jump to the loop label

incrementCounter:
			inc		R5						; increment counter variable R4


callLoop:
			; perform lights here
			; move R5 to each port
			mov.b	R5, &P6OUT
			mov.b	R5, &P2OUT
			mov.b	R5, &P3OUT

			mov.b	#10, R15				; 1s outer loop value in R15 for delay subroutine
			call	#Delay

			call 	#CheckKeypad				; delay subroutine

			cmp		#255, R5				; compare if we've counted 255 times

			jnz		incrementCounter		; if not, increment R4 (counter var)

			;;; if it's 0:

			bic.b	#BIT0, &P1IE			; local enable P1.0
			ret								; return back to main if the counter variable is 0


;-------------------------------------------------------------------------------
; CheckKeypad Subroutine
; R4 is used as storing P1IN
; R6 is used as the current status register
; R7 is used as the actual number pressed
;-------------------------------------------------------------------------------
CheckKeypad:
			; resetting registers R6 and R7
			mov.b	#0,	R6
			mov.b	#0, R7

			; set inputs
			bic.b	#BIT0, &P1DIR			; setting P1.0 as input
			bic.b	#BIT1, &P1DIR			; setting P1.1 as input
			bic.b	#BIT2, &P1DIR			; setting P1.2 as input
			bic.b	#BIT3, &P1DIR			; setting P1.3 as input

			; setting outputs
			bis.b	#BIT4, &P1DIR			; setting P1.4 as output
			bis.b	#BIT5, &P1DIR			; setting P1.5 as output
			bis.b	#BIT6, &P1DIR			; setting P1.6 as output
			bis.b	#BIT7, &P1DIR			; setting P1.7 as output

			; turn on outputs
			bis.b	#BIT4, &P1OUT			; setting P1.4 as input
			bis.b	#BIT5, &P1OUT			; setting P1.5 as input
			bis.b	#BIT6, &P1OUT			; setting P1.6 as input
			bis.b	#BIT7, &P1OUT			; setting P1.7 as input

			; set pullup/down resistors
			bis.b	#BIT0, &P1REN			; enable pull up/down resistor on P1.0
			bis.b	#BIT1, &P1REN			; enable pull up/down resistor on P1.1
			bis.b	#BIT2, &P1REN			; enable pull up/down resistor on P1.2
			bis.b	#BIT3, &P1REN			; enable pull up/down resistor on P1.3

			; configure all resistors as pull up
			bic.b	#BIT0, &P1OUT			; configure pull up on P1.0
			bic.b	#BIT1, &P1OUT			; configure pull up on P1.1
			bic.b	#BIT2, &P1OUT			; configure pull up on P1.2
			bic.b	#BIT3, &P1OUT			; configure pull up on P1.3


			;---------------------------------------------------------------------------
			; check what the current state of the keypad is with the configuration above
			;---------------------------------------------------------------------------
			mov.b	P1IN, R4
			cmp.b	#0F8h, R4 				; if 0, column #1 was selected
			jz		set8h

			cmp.b	#0F4h, R4 				; if 0, column #2 was selected
			jz		set4h

			cmp.b	#0F2h, R4 				; if 0, column #3 was selected
			jz		set2h

			cmp.b	#0F1h, R4 				; if 0, column #4 was selected
			jz		set1h

			jmp		end


; setting the selected bits into R6
set8h:
			bis.b	#08h, R6					; setting 0x1000 to R6
			jmp		continue

set4h:
			bis.b	#04h, R6					; setting 0x0100 to R6
			jmp		continue

set2h:
			bis.b	#02h, R6					; setting 0x0010 to R6
			jmp		continue

set1h:
			bis.b	#01h, R6					; setting 0x0001 to R6
			jmp		continue


continue:
			;---------------------------------------------------------------------------
			; perform same check but with different configurations
			;---------------------------------------------------------------------------

			; setting inputs
			bic.b	#BIT4, &P1DIR			; setting P1.4 as input
			bic.b	#BIT5, &P1DIR			; setting P1.5 as input
			bic.b	#BIT6, &P1DIR			; setting P1.6 as input
			bic.b	#BIT7, &P1DIR			; setting P1.7 as input

			; set outputs
			bis.b	#BIT0, &P1DIR			; setting P1.0 as output
			bis.b	#BIT1, &P1DIR			; setting P1.1 as output
			bis.b	#BIT2, &P1DIR			; setting P1.2 as output
			bis.b	#BIT3, &P1DIR			; setting P1.3 as output

			; turn on outputs
			bis.b	#BIT0, &P1OUT			; setting P1.0 as input
			bis.b	#BIT1, &P1OUT			; setting P1.1 as input
			bis.b	#BIT2, &P1OUT			; setting P1.2 as input
			bis.b	#BIT3, &P1OUT			; setting P1.3 as input

			; set pullup/down resistors
			bis.b	#BIT4, &P1REN			; enable pull up/down resistor on P1.4
			bis.b	#BIT5, &P1REN			; enable pull up/down resistor on P1.5
			bis.b	#BIT6, &P1REN			; enable pull up/down resistor on P1.6
			bis.b	#BIT7, &P1REN			; enable pull up/down resistor on P1.7

			; configure all resistors as pull up
			bic.b	#BIT4, &P1OUT			; configure pull up on P1.4
			bic.b	#BIT5, &P1OUT			; configure pull up on P1.5
			bic.b	#BIT6, &P1OUT			; configure pull up on P1.6
			bic.b	#BIT7, &P1OUT			; configure pull up on P1.7


			;---------------------------------------------------------------------------
			; check what the current state of the keypad is with the configuration above
			;---------------------------------------------------------------------------
			mov.b	P1IN, R4
			cmp.b	#8Fh, R4 				; if 0, row #1 was selected
			jz		set80h

			cmp.b	#4Fh, R4 				; if 0, row #2 was selected
			jz		set40h

			cmp.b	#2Fh, R4 				; if 0, row #3 was selected
			jz		set20h

			cmp.b	#1Fh, R4 				; if 0, row #4 was selected
			jz		set10h

			jmp		end



; setting the selected bits into R6
set80h:
			xor.b	#80h, R6					; setting 0x1000 to R6
			jmp		end

set40h:
			xor.b	#40h, R6					; setting 0x0100 to R6
			jmp		end

set20h:
			xor.b	#20h, R6					; setting 0x0010 to R6
			jmp		end

set10h:
			xor.b	#10h, R6					; setting 0x0001 to R6
			jmp		end


end:
			mov.b	R6, R10
			ret







;-------------------------------------------------------------------------------
; Delay Subroutine (1ms)
; uses R15 for outer loop
; Uses R14 for inner loop
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
; ISR - P1
; R5 is the counter in the LED subroutine
;-------------------------------------------------------------------------------
ISR_P1:
			cmp.b	#0F2h, R4				; if button B is pressed
			jz		move0R5

			cmp.b	#0F1h, R4				; if button A is pressed
			jz		move255R5

			jmp		endISRP1


move0R5:
			mov.b	#0, R5					; reset counter to 0
			jmp		endISRP1

move255R5:
			mov.b	#255, R5				; put counter to 255 to fininsh the counter
			jmp		endISRP1


endISRP1:
			bic.b	#BIT0, &P1IFG
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
            
            .sect ".int25"					; port 1
            .short ISR_P1

