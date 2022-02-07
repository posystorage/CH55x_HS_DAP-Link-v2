$NOMOD51
NAME	SW_DP_ASM

SWC_PIN	BIT	090H.7	
SWD_PIN	BIT	090H.5
SWD_PIN_DIR	BIT	0B0H.2
SWD_PMOD_OC	DATA 092H	
SWD_PDIR_PU	DATA 093H	
P2	DATA	0A0H
;Pn_MOD_OC Pn_DIR_PU
;	0			0		Input
;	0			1		PP_Output
;	1			0		OC_Output
;	1			1		OC_PU_Output
	
?PR?SWD_Write_Byte?SW_DP_ASM                SEGMENT CODE 
?PR?_SWJ_Sequence?SW_DP_ASM                 SEGMENT CODE 
?PR?_SWD_IN_Sequence?SW_DP_ASM              SEGMENT CODE 
?PR?_SWD_Transfer?SW_DP_ASM					SEGMENT CODE 

	EXTRN	IDATA (data_phase)
	EXTRN	IDATA (idle_cycles)
	;EXTRN	IDATA (turnaround)
	PUBLIC	_SWJ_Sequence
	PUBLIC	_SWD_IN_Sequence
	PUBLIC	_SWD_Transfer

;ACC = data
;R0  = Bits
	RSEG  ?PR?SWD_Write_Byte?SW_DP_ASM
SWD_Write_Byte:
	MOV		R0,#8
SWD_Write_Bits:	
	SETB	SWC_PIN
	RRC		A 
	MOV		SWD_PIN,C
	CLR		SWC_PIN
	DJNZ	R0,SWD_Write_Bits
	SETB	SWC_PIN
	RET
	
SWD_Read_Byte:
	MOV		R0,#8
SWD_Read_Bits:	
	CLR		SWC_PIN
	MOV		C,SWD_PIN
	RRC		A 
	SETB	SWC_PIN
	DJNZ	R0,SWD_Read_Bits
	RET	
		
;R1~R3 = datas Addr	
;R7 = Count
	RSEG  ?PR?_SWJ_Sequence?SW_DP_ASM
_SWJ_Sequence:
	MOV		P2,R2
SWJ_Next_Byte:
	MOV		A,R7
	CLR		C
	SUBB	A,#9
	JC		SWJ_Last_Bits
	INC		A
	MOV		R7,A
	MOVX	A,@R1
	Call	SWD_Write_Byte
	INC		R1
	SJMP	SWJ_Next_Byte
SWJ_Last_Bits:	
	MOV		A,R7
	MOV		R0,A
	MOVX	A,@R1
	Call	SWD_Write_Bits
	RET
	
	
;R1~R3 = datas Addr	
;R7 = Count
	RSEG  ?PR?_SWD_IN_Sequence?SW_DP_ASM
_SWD_IN_Sequence:
	SETB	SWD_PIN				
	ORL  	SWD_PMOD_OC,#20H  ;PP_Output->OC_PU_Output->PU_Input ;P1.5
	CLR		SWD_PIN_DIR
	MOV		P2,R2
SWD_IN_Next_Byte:	
	MOV		A,R7
	CLR		C
	SUBB	A,#9
	JC		SWD_IN_Last_Bits
	INC		A
	MOV		R7,A
	Call	SWD_Read_Byte
	MOVX	@R1,A
	INC		R1	
	SJMP	SWD_IN_Next_Byte
SWD_IN_Last_Bits:	
	MOV		A,R7
	MOV		R0,A
	Call	SWD_Read_Bits
	MOVX	@R1,A
	SETB	SWD_PIN_DIR
	ANL  	SWD_PMOD_OC,#0DFH ;PU_Input->PP_Output ;P1.5
	RET	

;R5 = datas Addr(Idata)	
;R7 = Req
	RSEG  ?PR?_SWD_Transfer?SW_DP_ASM
_SWD_Transfer:
	MOV		A,R5
	MOV		R1,A
SWD_T1:	
	MOV		A,R7
	ANL  	ACC,#0FH
	RL		A
	JNB		PSW.0,SWD_T2
	SETB	ACC.5
SWD_T2:	
	ORL  	ACC,#81H
	call	SWD_Write_Byte ;Start->Park
	SETB	SWD_PIN				
	ORL  	SWD_PMOD_OC,#20H  ;PP_Output->OC_PU_Output->PU_Input ;P1.5
	;ANL		SWD_PDIR_PU,#0DFH ;PP_Output->Input	
	CLR		SWD_PIN_DIR
	MOV		R0,#4
	call	SWD_Read_Bits ;Trn+3bit_ASK
	SWAP	A
	RR		A
	ANL  	ACC,#07H 
	XCH		A,R7;R7=ACK[2:0](for return) A=request
	CJNE	R7,#01H,SWD_T3;ACK=001b	WAIT=010b  FAULT=100b (LSB)
	JB		ACC.1,SWD_TR1
	;write
	CLR		SWC_PIN	;Trn	
	NOP
	NOP
	SETB	SWC_PIN	
	MOV		R5,#0	;PARITY
	SETB	SWD_PIN_DIR
	ANL  	SWD_PMOD_OC,#0DFH ;PU_Input->PP_Output ;P1.5
	;ORL  	SWD_PDIR_PU,#20H 
	;write 33bits
	MOV		A,@R1
	JNB 	PSW.0,SWD_TW1
	INC		R5
SWD_TW1:		
	CALL	SWD_Write_Byte
	INC		R1
	MOV		A,@R1
	JNB 	PSW.0,SWD_TW2
	INC		R5
SWD_TW2:		
	CALL	SWD_Write_Byte
	INC		R1
	MOV		A,@R1
	JNB 	PSW.0,SWD_TW3
	INC		R5
SWD_TW3:		
	CALL	SWD_Write_Byte
	INC		R1
	MOV		A,@R1
	JNB 	PSW.0,SWD_TW4
	INC		R5
SWD_TW4:		
	CALL	SWD_Write_Byte
	MOV		A,R5
	RRC		A 
	MOV		SWD_PIN,C;PARITY bit
	SJMP	SWD_TR7
SWD_TR1:	;read	
	;read 33bits
	MOV		R5,#0	;PARITY
	CALL	SWD_Read_Byte
	JNB 	PSW.0,SWD_TR2
	INC		R5
SWD_TR2:
	MOV		@R1,A	
	INC		R1	
	CALL	SWD_Read_Byte
	JNB 	PSW.0,SWD_TR3
	INC		R5
SWD_TR3:
	MOV		@R1,A	
	INC		R1	
	CALL	SWD_Read_Byte
	JNB 	PSW.0,SWD_TR4
	INC		R5
SWD_TR4:
	MOV		@R1,A	
	INC		R1	
	CALL	SWD_Read_Byte
	JNB 	PSW.0,SWD_TR5
	INC		R5
SWD_TR5:
	MOV		@R1,A	
	CLR		SWC_PIN
	CLR		A
	MOV		C,SWD_PIN
	RLC		A 
	SETB	SWC_PIN	
	XRL  	A,R5
	JNB  	ACC.0,SWD_TR6
	MOV		R7,#08H
SWD_TR6:	
	SETB	SWD_PIN_DIR
	ANL  	SWD_PMOD_OC,#0DFH ;PU_Input->PP_Output ;P1.5
	;ORL  	SWD_PDIR_PU,#20H 
SWD_TR7:
	;MOV  	R0,#LOW (idle_cycles)
	;INC		R0
	;MOV  	R0,#01H
;SWD_TR8:	
	CLR		SWC_PIN
	NOP
	NOP
	SETB	SWC_PIN	
	;DJNZ	R0,SWD_TR8	
	RET
	
SWD_T3:	; wait/fault/error
	;Dummy write/read	
	CJNE	R7,#02H,SWD_TE1	;010b wait
	SJMP	SWD_TE3
SWD_TE1:	
	CJNE	R7,#02H,SWD_TE2	;100b fault
	SJMP	SWD_TE3
SWD_TE2:	
	MOV		R7,#010H		;others:MISMATCH
	MOV  	R0,#022H
	SJMP	SWD_TE4
SWD_TE3:	
;	MOV  	R0,#022H
;	MOV  	A,#LOW (data_phase)
;	JNZ		SWD_TE4
	MOV  	R0,#01H
SWD_TE4:	
	CLR		SWC_PIN
	NOP
	NOP
	SETB	SWC_PIN	
	DJNZ	R0,SWD_TE4
	SETB	SWD_PIN_DIR
	ANL  	SWD_PMOD_OC,#0DFH ;PU_Input->PP_Output ;P1.5
	;ORL  	SWD_PDIR_PU,#20H 
	RET
	
END