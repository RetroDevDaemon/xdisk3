; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: writetrk.asm,v 1.1 2000/01/05 13:25:31 cisc Exp $

; ----------------------------------------------------------------------------
;	èëçûÇ›ÇçsÇ§
;	arg:	TRACK LENGTH
;	
CmdWriteTrack:
		ld	hl,DATA
		call	PtRecvData
		ld	a,(CmdPacket+1)
		ld	l,a
		ld	h,0
		push	hl
		call	putmsg
		db	"$6Writing track $7$n$6.",13,0
		
		
		ld	hl,DATA
		ld	bc,(CmdPacket+2)
		ld	a,(CmdPacket+1)
		ld	e,a
		
		call	SelectRAM
		call	DiWriteTrack
		call	SelectN88ROM
		ret
		
