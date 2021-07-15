; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: sysinfo.asm,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $

; ----------------------------------------------------------------------------
;	TransDisk Ç‚ 8801 Ç…ä÷Ç∑ÇÈèÓïÒÇëóÇÈ
;	char	version.major
;	char	version.minor
;	char	fddtype;
;	char	rom;
;	char	port 6f support
;
CmdSendSystemInfo:
		call	putmsg
 if DEBUG
		db	"$4Send System Info",13,10,0
 else
 		db	13,10,"$4Connected to host system.$7",13,10,0
 endif
		ld	hl,PacketBuffer
		push	hl
		ld	(hl),VERSION_MAJOR		; ver.major
		inc	hl
		ld	(hl),VERSION_MINOR		; ver.minor
		inc	hl
		push	hl
		call	DiSystemType
		ld	a,h
		or	l
		pop	hl
		ld	(hl),a				; fdd type
		inc	hl
		push	hl
		call	CheckROM
		pop	hl
		ld	(hl),b
		inc	hl
		call	Is6fSupported
		sbc	a,a
		ld	(hl),a
		pop	hl
		ld	bc,5
		ld	a,'%'
		call	PtSend
		ret
		
