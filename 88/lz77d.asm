; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: lz77d.asm,v 1.3 2000/01/15 00:31:20 cisc Exp $

;	符号の作成にあたって Teddy Matsumoto 氏の DIETTECH.DOC を参考に
;	させてもらいました．感謝！

 if 0
putadr:
		push	af
		push	de
		ld	de,0f3c8h+76
		exx
		ld	a,d
		exx
		call	putadr_1
		exx
		ld	a,e
		exx
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

;-----------------------------------------------------------------------------
;	hl:	圧縮されたデータ
;	de:	圧縮データの展開先
;

LZ77Dec:
		push	hl		; de' = 展開先
		exx
		pop	hl
		
		ld	d,(hl)
		ld	e,8
		inc	hl
LZ77Dec_1:
		sla	d		; get a bit
		dec	e		; -
		jp	nz,$+7		; -
		ld	d,(hl)		; -
		ld	e,8		; -
		inc	hl		; -
		
		ld	a,(hl)
		inc	hl
		exx
		jr	nc,LZ77Dec_c
		
		ld	(de),a		; 1 [xx]
		inc	de
		exx
		jp	LZ77Dec_1
		
LZ77Dec_c:
		ld	l,a		; 0 [xx]
		exx
		
		sla	d		; get a bit
		dec	e		; -
		jp	nz,$+7		; -
		ld	d,(hl)		; -
		ld	e,8		; -
		inc	hl		; -
		jr	c,LZ77Dec_f
		
		or	a		; 0 [xx] 0
		ret	z		; 0 [00] 0
		
	; 3 bytes, short range
		exx
		ld	h,0ffh
		add	hl,de
		ldi
		ldi
		ldi
		exx
		jp	LZ77Dec_1
	
LZ77Dec_f:
		ld	a,0ffh
		call	LZ77Dec_b
		rla
		call	LZ77Dec_b	; 1111111l
		jr	c,LZ77Dec_f2	; 0 [xx] 1a?
		and	11111101b
		
		call	LZ77Dec_b	; 1111110l ?
		jr	c,LZ77Dec_f2	; 0 [xx] 1a0?
		call	LZ77Dec_b
		rla
		
		call	LZ77Dec_b	; 111110
		jr	c,LZ77Dec_f2	; 0 [xx] 1l00m?
		call	LZ77Dec_b
		rla
		
		call	LZ77Dec_b	; 11110lmn
		jr	c,LZ77Dec_f2	; 0 [xx] 1l00m0n?
		call	LZ77Dec_b	; 1110lmn?
		rla
LZ77Dec_f2:
		exx
		ld	h,a
		exx
		
		ld	b,4
		ld	a,3
LZ77Dec_f3:
		sla	d		; get a bit
		dec	e		; -
		jp	nz,$+7		; -
		ld	d,(hl)		; -
		ld	e,8		; -
		inc	hl		; -
		jr	c,LZ77Dec_f5	; 01	4
		inc	a		; 001	5
		djnz	LZ77Dec_f3	; 0001	6 bytes
		
		call	LZ77Dec_b	; 0000?
		jr	nc,LZ77Dec_f4
		
		call	LZ77Dec_b	; 00001x
		adc	a,0
		jp	LZ77Dec_f5
		
		; 9 bytes 以上
LZ77Dec_f4:
		call	LZ77Dec_b	; 00000?
		jr	c,LZ77Dec_f6
		
		xor	a
		call	LZ77Dec_b
		rla
		call	LZ77Dec_b
		rla
		call	LZ77Dec_b
		rla
		add	a,9
LZ77Dec_f5:
		exx
		ld	b,0
		ld	c,a
		add	hl,de
		ldir
		exx
		jp	LZ77Dec_1

		; 17 bytes 以上
LZ77Dec_f6:
		ld	a,(hl)
		inc	hl
		add	a,17
		exx
		ld	c,a
		ld	b,0
		jr	nc,LZ77Dec_f7
		inc	b
LZ77Dec_f7:
		add	hl,de
		ldir
		exx
		jp	LZ77Dec_1
;
;	１ビット読み込み cy に返す
;
LZ77Dec_b:
		sla	d		; get a bit
		dec	e
		ret	nz
		ld	d,(hl)		; read bit buffer
		ld	e,8
		inc	hl
		ret


