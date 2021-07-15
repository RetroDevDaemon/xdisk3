; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	雑用
; ----------------------------------------------------------------------------
;	$Id: misc.asm,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $


; -----------------------------------------------------------------------------
;	書式付き文字列の表示
;	書式
;		$b	HEX-byte
;		$w	HEX-word
;		$d	10進-byte (2ｹﾀ)
;		$n	10進-word
;		$s	文字列
;	USES:	AFHLDE
;		
putmsg:
		pop	hl
putmsg_1:
		ld	a,(hl)
		inc	hl
		or	a
		jr	z,putmsg_2
		cp	'$'
		jr	z,putmsg_3
		call	putchr
		jr	putmsg_1
putmsg_2:	
		jp	(hl)

putmsg_3:
		ld	a,(hl)
		inc	hl
		cp	'b'
		jr	z,putmsg_b
		cp	'w'
		jr	z,putmsg_w
		cp	'd'
		jr	z,putmsg_d
		cp	'n'
		jr	z,putmsg_n
		cp	't'
		jr	z,putmsg_t
		cp	's'
		jr	z,putmsg_s
		cp	'c'
		jr	z,putmsg_c
		cp	'0'
		jr	c,putmsg_4
		cp	'7'+1
		jr	c,putmsg_col
putmsg_4:
		call	putchr
		jr	putmsg_1
		
putmsg_c:
		pop	af
		call	putchr
		jr	putmsg_1

putmsg_s:
		pop	de
putmsg_s1:
		ld	a,(de)
		or	a
		jr	z,putmsg_1
		inc	de
		call	putchr
		jr	putmsg_s1
		
putmsg_b:
		pop	af
		call	puthex
		jr	putmsg_1

putmsg_w:
		pop	de
		ld	a,d
		call	puthex
		ld	a,e
		call	puthex
		jr	putmsg_1
		
putmsg_d:
		pop	af
		call	putnum1
		jr	putmsg_1

putmsg_n:
		ex	de,hl
		pop	hl
		push	de
		push	bc
		ld	b,5-1
		call	putnum2
		pop	bc
		pop	hl
		jr	putmsg_1

putmsg_t:
		ex	de,hl
		pop	hl
		push	de
		push	bc
		ld	b,5-3
		call	putnum2
		pop	bc
		pop	hl
		jr	putmsg_1

putmsg_col:
		sub	'0'
		rrca
		rrca
		rrca
		or	00001000b
		ld	(TXTATTR),a
		jp	putmsg_1

puthex:
		push	af
		rlca
		rlca
		rlca
		rlca
		call	puthex_1
		pop	af
puthex_1:
		and	0fh
		cp	10
		jr	c,puthex_2
		add	a,7
puthex_2:
		add	a,'0'
		jp	putchr
		
putnum2:
		xor	a
		ld	de,10000
		call	putnum2_1
		ld	de,1000
		call	putnum2_1
		ld	de,100
		call	putnum2_1
		ld	de,10
		call	putnum2_1
		ld	a,l
		add	a,'0'
		jp	putchr
putnum2_1:
		push	af
		ld	a,-1
		or	a
putnum2_2:
		inc	a
		sbc	hl,de
		jr	nc,putnum2_2
		add	hl,de
		pop	de
		or	d
		jr	z,putnum2_3
		or	'0'
		call	putchr
		ld	a,'0'
		ret
putnum2_3:
		dec	b
		ret	p
		ld	a,' '
		call	putchr
		xor	a
		ret

putnum1:
		ld	d,0
putnum1_1:
		inc	d
		sub	10
		jr	nc,putnum1_1
		add	a,10 + '0'
		ld	e,a
		dec	d
		ld	a,' '
		jr	z,putnum1_2
		ld	a,'0'
		add	a,d
putnum1_2:
		call	putchr
		ld	a,e
		jp	putchr
		
	