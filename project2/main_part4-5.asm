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
			bis.b	#BIT0, &P1DIR			; setting the P1.0 as an output ([SCL] shared clock line)
			bis.b	#BIT6, &P6DIR			; setting the P6 as output ([SDL] shared data line)
			bic.w	#LOCKLPM5, &PM5CTL0  	; disabled the GPIO power-on HighZ
			bis.b	#BIT6, &P6OUT			; toggle P6.6 (LED2) (SDL)
			bis.b	#BIT0, &P1OUT			; toggle P1.0 (LED1) (SCLK)


main:
			call	#idle					; pause before transmission


			call 	#I2Cstart				; create start bit
			mov.b	#68h, R6				; moving 01101000b into R6 (this is the RTC address)
			mov.b	#0, R5					; set flag to YES R/W bit (0 = yes R/W bit)
			mov.b	#0h, R8					; R/W bit, [0 is WRITE], [1 is READ]
			call	#I2CsendAddress			; start data transmission
			call	#ACK					; check if receiving an ACK


			mov.b	#00h, R6				; moving 0x00 into R6 (this is the RTC seconds data address)
			mov.b	#1, R5					; set flag to YES R/W bit (0 = yes R/W bit)
			call 	#I2CsendAddress
			call	#ACK					; check if receiving an ACK
			call	#I2Cstop				; create stop bit


			mov.w	#1, R15					; puts number of 1x100ms delays
			call 	#Delay					; call delay subroutine

			;;;;; ---------------------------------------


			call 	#I2Cstart				; create start bit
			mov.b	#68h, R6				; moving 1101000b into R6 (this is the RTC address)
			mov.b	#0, R5					; set flag to YES R/W bit (0 = yes R/W bit)
			mov.b	#1h, R8					; R/W bit, [0 is WRITE], [1 is READ]
			call	#I2CsendAddress			; start data transmission
			call	#ACK					; check if receiving an ACK

			mov.b	#3h, R4					; we want 3 bytes of data (seconds, minutes, hours)

timeLoop:
			dec 	R4
			call	#I2CreceiveData			; get 1 byte of data						; decrement loop var R4
			cmp.b	#0h, R4					; compare if we received full 3 bytes
			jnz		timeLoop				; if not, go back to loop
			; NACK is called inside I2CreceiveData

			call	#I2Cstop				; create stop bit


temperature:
			;;;;; ---------------------------------------
			; Now move the address to the temperature address

			call 	#I2Cstart				; create start bit
			mov.b	#68h, R6				; moving 01101000b into R6 (this is the RTC address)
			mov.b	#0, R5					; set flag to YES R/W bit (0 = yes R/W bit)
			mov.b	#0h, R8					; R/W bit, [0 is WRITE], [1 is READ]
			call	#I2CsendAddress			; start data transmission
			call	#ACK					; check if receiving an ACK


			mov.b	#11h, R6				; moving 0x00 into R6 (this is the RTC seconds data address)
			mov.b	#1, R5					; set flag to NO R/W bit (0 = yes R/W bit)
			call 	#I2CsendAddress
			call	#ACK					; check if receiving an ACK
			call	#I2Cstop				; create stop bit


			mov.w	#1, R15					; puts number of 1x100ms delays
			call 	#Delay					; call delay subroutine

			call 	#I2Cstart				; create start bit
			mov.b	#68h, R6				; moving 1101000b into R6 (this is the RTC address)
			mov.b	#0, R5					; set flag to YES R/W bit (0 = yes R/W bit)
			mov.b	#1h, R8					; R/W bit, [0 is WRITE], [1 is READ]
			call	#I2CsendAddress			; start data transmission
			call	#ACK					; check if receiving an ACK


			mov.b	#3h, R4					; we want 3 bytes of data (seconds, minutes, hours)

timeLoop2:
			dec 	R4
			call	#I2CreceiveData			; get 1 byte of data						; decrement loop var R4
			cmp.b	#0h, R4					; compare if we received full 3 bytes
			jnz		timeLoop2				; if not, go back to loop
			; NACK is called inside I2CreceiveData

			call	#I2Cstop				; create stop bit


			;mov.w	#1, R15					; puts number of 1x100ms delays
			;call 	#Delay					; call delay subroutine




			jmp 	main




;-------------------------------------------------------------------------------
; IDLE Subroutine
; (1.5 second pause before transmission)
;-------------------------------------------------------------------------------
idle:
			mov.w	#15, R15				; puts number of 15x100ms delays 1.5s
			call 	#Delay					; delay just to get it started late
			ret



;-------------------------------------------------------------------------------
; I2Cstart Subroutine
;-------------------------------------------------------------------------------
I2Cstart:
			bic.b	#BIT6, &P6OUT			; toggle P6.6 (LED2) (HIGH TO LOW)

			mov.w	#1, R15					; StabilityDelay
			call 	#StabilityDelay			; StabilityDelay

           	bic.b	#BIT0, &P1OUT			; toggle P1.0 (LED1) (HIGH TO LOW)

			ret


;-------------------------------------------------------------------------------
; I2Cstop Subroutine
;-------------------------------------------------------------------------------
I2Cstop:
			bis.b	#BIT0, &P1OUT			; toggle P1.0 (LED1) (LOW TO HIGH)

			mov.b	#1, R15					; StabilityDelay
			call 	#StabilityDelay			; StabilityDelay

			bis.b	#BIT6, &P6OUT			; toggle P6.6 (LED2) (LOW TO HIGH)

			ret

;-------------------------------------------------------------------------------
; I2Csend Subroutine (start the I2C send address)
;-------------------------------------------------------------------------------
I2CsendAddress:
			mov.w	#1, R15					; puts number of 1x100ms delays
			call 	#Delay

			mov.b	#7h, R7


transmitData:
			call	#I2CtxByte				; start 1 byte transmission

			ret

;-------------------------------------------------------------------------------
; I2Creceive Subroutine receives data (# of bytes based on R7
;-------------------------------------------------------------------------------
I2CreceiveData:

			mov.b	#8h, R7					; we want to receive 1 byte (8 bits)
			call	#I2CrxByte				; start 1 byte transmission

			cmp.b	#0, R4
			jz		sendNack

			; if not 0, keep waiting for ACK
			call 	#FakeACK				; send ACK to slave
			jmp		receiveReturn			; go back to return

sendNack:

			call	#NACK					; send NACK when we receive all the bytes we need

receiveReturn:

			ret





;-------------------------------------------------------------------------------
; I2CtxByte Subroutine
; R6 is where the data we want to send lives
; R7 will determine how many times this will loop
; R8 is the bit to check whether it's a RD/WR
;-------------------------------------------------------------------------------
I2CtxByte:
			clrc						; clear carry bit

			cmp.b	#1, R5				; check whether we are sending a R/W bit. If R5 = 1, we are not, so no need to shift
			jz		carryCode			; skip shift if R5 = 1, so no R/W bit

			rlc.b	R6					; shift initially as we only send 7 bits first

carryCode:
			rlc.b	R6
			jc		yesCarry

			; if no carry, then its a logic LOW, turn off LED2 (Shared Data Line)
			bic.b	#BIT6, &P6OUT			; P6.6 (LED2) (HIGH TO LOW)
			jmp		checkIfEnd


; if a carry, turn on LED2 (Shared Data line)
yesCarry:
			bis.b	#BIT6, &P6OUT			; Turn on P6.6 (LED2) (LOW to HIGH))


checkIfEnd:
			dec		R7						; decrement loop

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bic.b	#BIT6, &P6OUT			; turn off P6.6 (LED2) (HIGH TO LOW)

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay


			cmp.b	#0, R7					; compare the loop number
			jnz 	carryCode				; go back into loop if not done

			;;; -------------------------------------------------------------
			;;; now set the R/W bit
			;;; -------------------------------------------------------------

			cmp.b	#1, R5					; check whether the YES/NO flag for R/W bit is set (1 is no, 0 is yes)
			jz		checkIfEnd2				; skip R/W bit if R5 = 1

			cmp.b	#0h, R8					; check if it's a RD/WR bit
			jz		writeBit				; jmp if its a write (0)

			bis.b	#BIT6, &P6OUT			; it's a 1, so it's a read
			jmp		checkIfEnd2


writeBit:
			bic.b	#BIT6, &P6OUT			; it's a 0, so it's a write


checkIfEnd2:
			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bic.b	#BIT6, &P6OUT			; turn off P6.6 (LED2) (HIGH TO LOW)

			ret





;-------------------------------------------------------------------------------
; I2CrxByte Subroutine
; receives RTC info
; R7 is the number of times this loops (8 times for 8 bits)
; R10 is used to get P1IN input (ACK or not)
;-------------------------------------------------------------------------------
I2CrxByte:

			bic.b	#BIT6, &P6DIR			; setting the P6 as input ([SDL] shared data line)

loopArea:
			dec		R7						; decrement loop

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay


			mov.b	P6IN, R10				; get input from P6In


			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay


			cmp.b	#0, R7					; compare the loop number
			jnz 	loopArea				; go back into loop if not done


			;; ---------------------------------------------------


			bis.b	#BIT6, &P6DIR			; setting the P6 as output ([SDL] shared data line)
			bic.b	#BIT6, &P6OUT			; turn off SDL



			ret







;;; Now send data.
;;; write a counter from 0 to 9 and send those
;-------------------------------------------------------------------------------
; I2CsendData Subroutine (start the I2C send)
; This will send data to the slave device
; This will utilize the I2Csend subroutine.
; We will create the counter here and keep track of it.
; Each time we have a new count variable, we call the I2CsendData subroutine
; R4 is to keep track the count
; R5 is a flag to check whether we need a R/W bit (0 is yes, 1 is no)
;-------------------------------------------------------------------------------
I2CsendCounterData:
			mov.b	#0, R4					; initialize R4 count variable with 0
			jmp		callLoop				; jump to the loop label

incrementCounter:
			inc		R4						; increment counter variable R4


callLoop:
			mov.b	#8, R7					; we want the I2Csend subroutine to loop 8 times this time
			mov.b	R4, R6					; move the count variable to R6 register which sends it in I2Csend
			mov.b	#1, R5					; set flag to NOT R/W bit
			call	#I2CtxByte				; call the I2CtxByte subroutine to send the data in R6

			cmp		#9, R4					; compare if we've counted 9 times

			jnz		incrementCounter		; if not, decrement R4 (counter var) and send the new counter

			;;; if it's 0:
			ret								; return back to main if the counter variable is 0












;-------------------------------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ACK Subroutine
; R10 is used to get P1IN input (ACK or not)
;-------------------------------------------------------------------------------
ACK:
			bic.b	#BIT6, &P6DIR			; setting the P6 as input ([SDL] shared data line)

retreiveRTCdata:
			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			;;;----------------------------------------------------------
			mov.b	P6IN, R10				; read input from RTC
			;;;----------------------------------------------------------

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay


			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock


			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			cmp.b	#0, R10					; check if ACK

			jnz		retreiveRTCdata			; loop until we receive ACK

			bis.b	#BIT6, &P6DIR			; setting the P6 as output ([SDL] shared data line)
			bic.b	#BIT6, &P6OUT			; turn off

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			ret



;-------------------------------------------------------------------------------
; NACK Subroutine
;-------------------------------------------------------------------------------
NACK:
			;;; holding it as HIGH (NACK)
			bis.b	#BIT6, &P6OUT			; turn on LED2 for NACK HIGH

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			ret



;-------------------------------------------------------------------------------
; ACK Subroutine
; sends an ACK from the master to slave
;-------------------------------------------------------------------------------
FakeACK:
			;;; holding it as a LOW (ACK)
			bic.b	#BIT6, &P6OUT			; turn off P6.6 (LED2) (HIGH TO LOW)


			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bis.b	#BIT0, &P1OUT			; turn on P1.0 (LED1) (LOW TO HIGH) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			bic.b	#BIT0, &P1OUT			; turn off P1.0 (LED1) (HIGH TO LOW) clock

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay


			mov.w	#1, R15					; call Delay with 1ms
			call	#Delay					; call delay

			mov.w	#1, R15					; StabilityDelay
			call	#StabilityDelay			; call stability delay

			ret



;-------------------------------------------------------------------------------
; Delay Subroutine
;-------------------------------------------------------------------------------
Delay:
beginning:
			mov.w 	#02162h, R14			; move 0x0FFF to R14 (inner loop variable)

ifInner:	dec  	R14						; decrement inner loop variable
			cmp.w 	#0, R14					; compare if inner loop variable is 0
			jnz		ifInner					; if not zero go back to decrementing it


			dec 	R15						; if equal to 0, decrement outer loop
			cmp.w	#0, R15					; and check if outer loop is 0


			jnz		beginning				; if not 0, go back to the beginning.
			ret


;-------------------------------------------------------------------------------
; StabilityDelay Subroutine
;-------------------------------------------------------------------------------
StabilityDelay:
stabilitybeginning:
			mov.w 	#01662h, R14			; move 0x0FFF to R14 (inner loop variable)

stabilityifInner:
			dec  	R14						; decrement inner loop variable
			cmp.w 	#0, R14					; compare if inner loop variable is 0
			jnz		stabilityifInner		; if not zero go back to decrementing it


			dec 	R15						; if equal to 0, decrement outer loop
			cmp.w	#0, R15					; and check if outer loop is 0


			jnz		stabilitybeginning				; if not 0, go back to the beginning.
			ret

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
            
