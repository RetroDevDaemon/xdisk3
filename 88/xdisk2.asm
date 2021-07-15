; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: xdisk2.asm,v 1.7 2000/02/01 13:08:57 cisc Exp $

VERSION_MAJOR	equ	02h
VERSION_MINOR	equ	07h

DEBUG		equ	0

putchr		equ	03e0dh

PORT30		equ	0e6c0h
PORT31		equ	0e6c2h
PORTE6		equ	0ef0eh
TXTATTR		equ	0e6b4h

DATA		equ	04000h
COMPBUFFER	equ	00800h
RsBuffer	equ	0b400h

		org	0b500h
 if 1	; bloadheader
 		org	$-4
		dw	$+4
		dw	endofcode
 endif

CodeBegin	equ	$

; -----------------------------------------------------------------------------

InVector	jr	Start		; 割り込みベクタ
		dw	InVsync, InDummy, InDummy
		dw	InDummy, InDummy, InDummy, InDummy
		
; -----------------------------------------------------------------------------
;	たちあげ
;
Start:
		in	a,(71h)
		push	af
		ld	a,0ffh
		out	(71h),a
		
		ld	(ExitStack),sp
		
		; stack check
		or	a
		ld	hl,RsBuffer
		sbc	hl,sp
		jp	c,start_e2
		
		ld	hl,RsIntr
		ld	(InVector),hl
		
		xor	a
		ld	(RsIsOpen),a
		
		; new
		ld	hl,0
		ld	(1),hl
		
		call	InitScreen
		ld	a,VERSION_MINOR
		push	af
		ld	hl,VERSION_MAJOR
		push	hl
		call	putmsg
		db	'$7',12
		db	'TransDisk/88 $n.$b',13,10
		db	'Copyright (C) 2000 cisc.',13,10,10,0
		
		call	BuildCRCTable
		call	DiSetup
		jr	c,start_e1
		
		call	putmsg
		db	"Push [$6STOP$7] key to terminate.",13,10,0
		
		call	InInit
		call	Is6fSupported
		ld	a,8
		jr	nc,Start_1
		in	a,(6fh)
Start_1:
		call	RsOpen
		call	Main
exit:
		di
		ld	sp,0
ExitStack	equ	$-2
		call	SelectN88ROM
		call	RsClose
		call	DiCleanup
		call	InCleanup
		ei
exit_1:		
		pop	af
		out	(71h),a
		ld	hl,0e18h
		ld	(InVector),hl
		ret
start_e1:
		call	putmsg
		db	7,"$2Couldn't make comminucation with sub system!$7"
		db	13,10,0
		jr	exit
		
start_e2:
		ld	hl,RsBuffer
		dec	hl
		push	hl
		call	putmsg
		db	"Bad stack pointer!",13,10
		db	"Be sure to execute 'CLEAR, &H$w'.",13,10,0
		jp	exit_1

;-----------------------------------------------------------------------------

		incl	"siodrv.asm"
		incl	"io.asm"
		incl	"packet.asm"
		incl	"crc.asm"
		incl	"rle.asm"
		incl	"lz77d.asm"
		incl	"getrom.asm"
		incl	"disk.asm"
		incl	"main.asm"
		incl	"readtrk.asm"
		incl	"writetrk.asm"
		incl	"sysinfo.asm"
		incl	"setdrive.asm"
		incl	"misc.asm"
		incl	"diskcode.asm"

;-----------------------------------------------------------------------------
;	ワーク
;
IDTABLE_0	db	0, 0, 0, 0, 0, 0, 0, 1, 40h, 0, 0, 0
endofcode	equ	$
		ds	4
		
IDTABLE		ds	400h

Critical	db	0

DiDrive		db	0
DiTrack		db	0
DiType		db	0
DiNumTracks	db	0
DiSectors	db	0
DiMFM		db	0
DiDataLength	dw	0

TransmittedLen	dw	0

PtIndex		dw	0
PtLastPtr	dw	0
PtLastLen	dw	0
PtBurstMode	db	0

RsIsOpen	db	0
RsOld6f		db	0
RsCommand	db	0
RsError		db	0
RsBufW		dw	0		; 読バッファの書き込みポインタ
RsBufR		dw	0		; 読バッファの読み込みポインタ
RsFree		db	0		; 読バッファの空きバイト数
RsOverflow	db	0		; オーバーフローした？


CmdPacket	ds	32		; コマンドパケットバッファ
PacketBuffer	ds	64		; 小規模パケットのためのバッファ

		org	(($+255) AND (NOT 255))
CRCTABLE_H	ds	100h
CRCTABLE_L	ds	100h

		end
		