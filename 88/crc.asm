; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	CRC
; ----------------------------------------------------------------------------
;	$Id: crc.asm,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $


; -----------------------------------------------------------------------------
;	CRC 計算
;	in:	DE	これまでの CRC
;		A	次のデータ
;	out:	DE
;	USES:	AF
;
CalcCRC:
		push	hl
		xor	d
		ld	l,a
		ld	a,e
		ld	h,high CRCTABLE_L
		ld	e,(hl)
		ld	h,high CRCTABLE_H
		xor	(hl)
		ld	d,a
		pop	hl
		ret

; -----------------------------------------------------------------------------
;	CRC 計算 (ブロック)
;	in:	HL	ブロックのアドレス
;		BC	長さ
;	out:	DE	CRC
;	uses:	AFHLD'E'
;
CalcCRCBlock:
		exx
		ld	de,0ffffh
		exx
CalcCRCBlock_1:
		ld	a,(hl)
		exx
		xor	d
		ld	l,a
		ld	a,e
		ld	h,high CRCTABLE_L
		ld	e,(hl)
		ld	h,high CRCTABLE_H
		xor	(hl)
		ld	d,a
		exx
		cpi
		jp	pe,CalcCRCBlock_1
		exx
		push	de
		exx
		pop	de
		ret

; -----------------------------------------------------------------------------
;	CRC TABLE 作成
;
BuildCRCTable:
		ld	c,0
buildcrc_0:
		ld	h,c
		ld	l,0
		ld	b,8
buildcrc_1:
		add	hl,hl
		jr	nc,buildcrc_2
		ld	a,h
		xor	10h
		ld	h,a
		ld	a,l
		xor	21h
		ld	l,a
buildcrc_2:
		djnz	buildcrc_1
		ld	b,high CRCTABLE_H
		ld	a,h
		ld	(bc),a
		ld	b,high CRCTABLE_L
		ld	a,l
		ld	(bc),a
		inc	c
		jr	nz,buildcrc_0
		ret
		
