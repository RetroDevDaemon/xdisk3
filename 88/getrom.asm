; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	ROM éÊìæ
; ----------------------------------------------------------------------------
;	$Id: getrom.asm,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $

;-----------------------------------------------------------------------------
;	ägí£ ROM åüèo
;	ret:	B	b0 - äøéöëÊÇP
;			b1 - äøéöëÊÇQ
;			b2 - é´èë
;			b3 - ÇbÇcÇaÇhÇnÇr
;	USES:	AFHLDEC
;
CheckROM:
		ld	b,0
		; äøéöëÊÇP
		ld	c,0e8h
		call	CheckKANJI
		jr	z,CheckROM_1 
		set	0,b
CheckROM_1:
		; äøéöëÊÇQ
		ld	c,0ech
		call	CheckKANJI
		jr	z,CheckROM_2
		set	1,b
CheckROM_2:
		; é´èë
		xor	a
		out	(0f0h),a
		out	(0f1h),a
		ld	hl,0ffffh
		ld	(hl),a
		cp	(hl)
		jr	nz,CheckROM_5
		ld	a,0ffh
		ld	(hl),a
		cp	(hl)
		jr	z,CheckROM_3
CheckROM_5:
		set	2,b
CheckROM_3:
		ld	a,1
		out	(0f1h),a
		
		; CDBIOS
		push	bc
		ld	hl,0
		ld	bc,100h
		call	CalcCRCBlock
		push	de
		ld	a,10h
		out	(99h),a
		ld	hl,0
		ld	bc,100h
		call	CalcCRCBlock
		pop	hl
		xor	a
		out	(99h),a
		sbc	hl,de
		pop	bc
		jr	z,CheckROM_4
		set	3,b
CheckROM_4:
		ret
		
CheckKANJI:
		xor	a
		out	(c),a
		inc	c
		out	(c),a
		inc	c
		out	(c),a
		dec	c
		in	a,(c)
		inc	c
		in	e,(c)
		inc	c
		inc	c
		out	(c),a
		and	e
		inc	a
		ret


;-----------------------------------------------------------------------------
;	éwíËÇ≥ÇÍÇΩ ROM ÇÃÉCÉÅÅ[ÉWÇì]ëóÇ∑ÇÈ
;	arg:	A	ROMTYPE
;			00-1F	KANJI ROM (00-07 ... f8-ff)
;			20-3F	KANJI ROM 2
;			40-47	N88-BASIC ROM
;			48-4f	N-BASIC ROM
;			50-57	EROM0-3
;			58-59	DISK ROM
;			60-6f	CDBIOS ROM
;			70-EF	é´èë ROM
;
;		HL	destination
;
GetROM:
		cp	20h
		jr	c,GetROM_Kanji
		cp	40h
		jr	c,GetROM_Kanji2
		
		cp	48h
		jr	c,GetROM_N88
		cp	50h
		jr	c,GetROM_N
		cp	58h
		jr	c,GetROM_erom
		cp	60h
		jr	c,GetROM_disk
		cp	70h
		jp	c,GetROM_cdbios
		sub	70h
		jp	GetROM_Jisyo
		
;-----------------------------------------------------------------------------
;	ÉfÉBÉXÉNÉçÉÄÇÃì]ëó
;
GetROM_disk:
		and	1
		add	a,a
		add	a,a
		add	a,a
		add	a,a
		ld	d,a
		ld	e,0
		ld	bc,400h
		jp	DiRapidRecv
		
;-----------------------------------------------------------------------------
;	e-rom ÇÃì]ëó
;
GetROM_erom:
		ld	b,a
		rrca
		and	3
		ld	c,a
		di
		in	a,(32h)
		push	af
		and	not 3
		or	c
		out	(32h),a
		ld	a,not 1
		out	(71h),a
		ld	a,b
		and	1
		add	a,6
		call	GetROM_xfer
		ld	a,0ffh
		out	(71h),a
		pop	af
		out	(32h),a
		ei
		ret

;-----------------------------------------------------------------------------
;	n-basic rom ÇÃì]ëó
;
GetROM_N:
		ld	b,a
		ld	a,(PORT31)
		push	af
		and	not 2
		or	4
		ld	(PORT31),a
		out	(31h),a
		
		ld	a,b
		call	GetROM_xfer
		
		pop	af
		out	(31h),a
		ld	(PORT31),a
		ret

;-----------------------------------------------------------------------------
;	n88-rom ÇÃì]ëó
;
GetROM_N88:
GetROM_xfer:
		and	7
		add	a,a
		add	a,a
		add	a,a
		add	a,a
		ex	de,hl
		ld	h,a
		ld	l,0
		ld	bc,01000h
		ldir
		ret

;-----------------------------------------------------------------------------
;	äøéö rom ÇÃì]ëó
;
GetROM_Kanji:
		ld	c,0e8h
		db	11h
GetROM_Kanji2:
		ld	c,0ech
		and	1fh
		rlca		; x2
		rlca		; x4
		rlca		; x8
		ld	d,a
		ld	e,0
		ld	b,8
GetROM_Kanji_1:
		push	bc
		ld	b,0
GetROM_Kanji_2:
		ld	a,e
		out	(c),a		; e8
		ld	a,d
		inc	de
		inc	c		; e9
		out	(c),a
		inc	c		; ea
		out	(c),a
		dec	c		; e9
		in	a,(c)
		ld	(hl),a
		inc	hl
		jr	$+2
		dec	c		; e8
		in	a,(c)
		ld	(hl),a
		inc	hl
		ld	a,c
		inc	c		; e9
		inc	c		; ea
		inc	c		; eb
		out	(c),a
		ld	c,a		; e8
		djnz	GetROM_Kanji_2
		pop	bc
		djnz	GetROM_Kanji_1
		ret
		
;-----------------------------------------------------------------------------
;	cd bios ÇÃì]ëó
;
GetROM_cdbios:
		bit	3,a
		ld	c,4
		jr	nz,GetROM_cdbios_1
		ld	c,0
GetROM_cdbios_1:
		and	7
		add	a,a
		add	a,a
		add	a,a
		add	a,a
		ld	d,a
		ex	de,hl
		ld	l,0
		
		ld	b,4
GetROM_cdbios_2:
		push	bc
		
		ld	a,10h
		out	(99h),a
		
		ld	a,(PORT31)
		push	af
		and	not 6
		or	c
		ld	(PORT31),a
		out	(31h),a
		
		push	de
		ld	de,IDTABLE
		ld	bc,400h
		ldir
		pop	de
		
		pop	af
		ld	(PORT31),a
		out	(31h),a
		xor	a
		out	(99h),a
		
		push	hl
		ld	hl,IDTABLE
		ld	bc,400h
		ldir
		pop	hl
		pop	bc
		djnz	GetROM_cdbios_2
		
		ret
		
;-----------------------------------------------------------------------------
;	é´èë rom
;
GetROM_Jisyo:
		ld	b,a
		srl	a
		srl	a
		out	(0f0h),a
		ld	a,b
		and	3
		add	a,a
		add	a,a
		add	a,a
		add	a,a
		or	0c0h
		ex	de,hl
		ld	h,a
		ld	l,0
		ld	bc,1000h
		di
		xor	a
		out	(0f1h),a
		ldir
		ld	a,1
		out	(0f1h),a
		ei
		ret

; ----------------------------------------------------------------------------
;	ROM ì]ëó
;	arg:	[07] PAGE MODE
;
CmdSendROM:
		ld	hl,(CmdPacket+1)
		ld	a,h
		ld	(PtBurstMode),a
		
		push	hl
		ld	h,0
		inc	hl
		push	hl
		call	putmsg
		db	'Sending ROM #$5$n$7.',13,0
		pop	hl
		
		ld	a,l
		ld	hl,DATA
		push	hl
		call	GetROM
		pop	hl
		ld	bc,1000h
		call	PtSendData
		ret


