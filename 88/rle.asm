; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: rle.asm,v 1.2 2000/01/14 13:24:34 cisc Exp $

 if 0
putadr:
		push	af
		push	de
		ld	de,0f3c8h+76
		ld	a,b
		call	putadr_1
		ld	a,c
		call	putadr_1
		pop	de
		pop	af
		ret
		
putadr_1:
		push	af
		rrca
		rrca
		rrca
		rrca
		call	putadr_2
		pop	af
putadr_2:
		and	0fh
		cp	10
		jr	c,$+4
		add	a,7
		add	a,'0'
		ld	(de),a
		inc	de
		ret
 endif

; ----------------------------------------------------------------------------
;	RLE 圧縮
;	in:	hl	圧縮するデータ
;		de	圧縮先
;		bc	データサイズ
;	out:	hl	圧縮後のデータサイズ
;		nc
;
RLECompress:	
		push	de
		push	de
		exx
		pop	hl
RLECompress_1:
		ld	d,h	; DE' = mark
		ld	e,l
		inc	hl
		exx
		ld	e,0
RLECompress_2:
;	call	putadr
		ld	a,(hl)
		inc	hl
		cpi
		jp	po,RLECompress_3
		jr	z,RLECompress_4		; (HL-1) == (HL)
RLECompress_6:
		dec	hl
		exx
		ld	(hl),a
		inc	hl
		exx
		ld	a,e
		inc	e
		cp	7fh
		jr	c,RLECompress_2
		
		; 書込み
		exx
		ld	(de),a
		jp	RLECompress_1
		
RLECompress_3:	; 1 バイト読んだ時点で時間切れ
		exx
		ld	(hl),a
		inc	hl
		exx
		ld	a,e
		exx
		ld	(de),a
		pop	de
		or	a
		sbc	hl,de
		ret

RLECompress_5:
		dec	hl
		inc	bc
		jp	RLECompress_6

RLECompress_4:
;	call	putadr
		cpi				; HL=ORG+3
		jp	po,RLECompress_5
		jr	nz,RLECompress_5
		; 3 バイト一致したみたい
		ld	d,a
		ld	a,e
		exx
		sub	1
		ld	(de),a
		jr	nc,RLECompress_11
		; 直前も RLE だった場合
		dec	hl
RLECompress_11:
		exx
		; 後何バイト一致しているの？
		ld	a,d
		ld	e,81h
RLECompress_7:
		cpi			; HL=ORG+4
		jp	po,RLECompress_9
		jr	nz,RLECompress_8
		inc	e
		jr	nz,RLECompress_7
		dec	bc
		jr	RLECompress_10
RLECompress_8:				; HL = 不一致点の次
		dec	hl
RLECompress_10:
		exx
		ld	e,a		; 記号
		exx
		dec	e
		ld	a,b
		or	c
		ld	a,e		; 一致長
		exx
		ld	(hl),a
		inc	hl
		ld	(hl),e
		inc	hl
		jp	nz,RLECompress_1
		jp	RLECompress_12

RLECompress_9:
		exx			; HL = 最終一致点, 最低一致長 = 4
		ld	e,a
		exx
		ld	a,e
		exx
		ld	(hl),a
		inc	hl
		ld	(hl),e
		inc	hl
RLECompress_12:
		pop	de
		or	a
		sbc	hl,de
		ret
		
