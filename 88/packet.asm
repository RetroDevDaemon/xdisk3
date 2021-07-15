; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	パケット入出力
; ----------------------------------------------------------------------------
;	$Id: packet.asm,v 1.7 2000/01/21 01:57:22 cisc Exp $

PD_BASE		equ	40h

; ----------------------------------------------------------------------------
;	1 バイトからなる結果パケットの送信
;	A	result byte
;
PtSendByteResult:
		ld	hl,PacketBuffer
		ld	(hl),a
		ld	bc,1
		ld	a,'#'
	;	jp	PtSend

;-----------------------------------------------------------------------------
;	パケットの送信
;	HL	SRC
;	BC	Length
;
PtSend:
		call	RsSendByte
		call	PtInit
		ld	de,-1
		push	hl
		ld	hl,(PtIndex)
		ld	a,l
		call	PtSendByteCRC
		ld	a,h
		call	PtSendByteCRC
		pop	hl
		ld	a,c
		call	PtSendByteCRC
		ld	a,b
		call	PtSendByteCRC
		ld	a,b
		or	c
		jr	z,PtSend_2
PtSend_1:
		ld	a,(hl)
 		call	PtSendByteCRC
		cpi
		jp	pe,PtSend_1
PtSend_2:
		ld	a,e
		call	PtSendByte
		ld	a,d
		call	PtSendByte
		call	PtSendFlush
		ret
		
; -----------------------------------------------------------------------------
;	パケットを受信する
;	次のような機能をもつ
;	・コマンドパケットヘッダによる中断
;
;	1. ヘッダ文字読み込み
;	2. データ読み込み
;
PtRecv:
		call	PtInit		; データ部の読み込みを始める
		ld	de,-1		; CRC
		
		; Index
		call	PtRecvByteCRC
		ret	c
		db	0ddh
		ld	l,a
		call	PtRecvByteCRC
		ret	c
		db	0ddh
		ld	h,a
		
		; データサイズ
		call	PtRecvByteCRC
		ret	c
		ld	c,a
		call	PtRecvByteCRC
		ret	c
		ld	b,a
		or	c
		jr	z,PtRecv_2
		
		; データ受信	
		push	hl
PtRecv_1:
		call	PtRecvByteCRC
		ret	c
		ld	(hl),a
		
		cpi
		jp	pe,PtRecv_1
		pop	hl
		
PtRecv_2:
		; CRC チェック
		call	PtRecvByte
		ret	c
		
		xor	e
		ld	e,a
		call	PtRecvByte
		ret	c
		
		xor	d
		or	e
		ret	z
		scf
		ret

;-----------------------------------------------------------------------------
;	バッファの中身は 0-6(6-0) bits
;	-xxx----	2
;	----yyyy	3
;	yyyycccc
;	まずデータを 1-7 bits 右シフトする
;	ばっふぁの中身と OR して出力
;	残りのデータは 7-1 bits 左シフト
;	1:	a >>= e		7-e
;	2:	a |= d, out
;	3:	a <<= 7-e	e
;	4:	e++		e--
;	  
PtSendByteCRC:
		push	af
		call	CalcCRC
		pop	af
PtSendByte:
		exx
		ld	c,a
		ld	b,e
		inc	b
PtSendByte_2:
		srl	a
		djnz	PtSendByte_2
		or	d
		and	7fh
		add	a,PD_BASE
		call	RsSendByte
		
		ld	a,6
		sub	e
		jr	z,PtSendByte_4
		ld	b,a
		ld	a,c
PtSendByte_3:
		add	a,a
		djnz	PtSendByte_3
		ld	d,a
		inc	e
		exx
		ret
PtSendByte_4:
		ld	a,c
		and	7fh
		add	a,PD_BASE
		call	RsSendByte
		ld	de,0
		exx
		ret

PtInit:
		exx
		ld	de,0000h
		exx
		ret
		
PtSendFlush:
		exx
		ld	a,d
		and	7fh
		add	a,PD_BASE
		dec	e
		call	p,RsSendByte
		exx
		ret

;-----------------------------------------------------------------------------
;	バッファ内データ e=0-6 bits
;	e=0 bit なら d=7bit よみこみ、d<<=1, e=7	1-7
;	c=7 bit 読み込み
;	a=c
;	a>>=6-0 bit 右シフト
;	a|=d, *p++=a
;	d=c, 
;	d<<=7-1 bit 左シフト

;	d, e
;	if (e==0)
;		d=Read, d<<=1, e=7
;	c=Read
;	a=c
;	a>>=(e-1)
;	a|=d,
;	c=d, d<<=e
;	e--

PtRecvByte:
		exx
		inc	e
		dec	e
		jr	nz,PtRecvByte_1
		call	RsRecvByte
		sub	PD_BASE
		jp	m,PtRecvByte_PH
		add	a,a
		ld	d,a
		ld	e,7
PtRecvByte_1:
		call	RsRecvByte
		sub	PD_BASE
		jp	m,PtRecvByte_PH
		ld	c,a
		add	a,a
		ld	b,e
PtRecvByte_2:
		srl	a
		djnz	PtRecvByte_2
		or	d
		ld	d,c
		push	af
		ld	a,9
		sub	e
		ld	b,a
PtRecvByte_3:
		sla	d
		djnz	PtRecvByte_3
		pop	af
		dec	e
		exx
		ret
		
PtRecvByte_PH:
		exx
		pop	de
		add	a,PD_BASE
		call	RsUngetc
		cp	'!'		; command packet
		jp	z,Main0a
		cp	'+'		; error1
		jp	z,Main0a
		cp	'*'		; error2
		jp	z,Main0a
		scf
		ret
		
PtRecvByteCRC:
		call	PtRecvByte
		ret	c
		push	af
		call	CalcCRC
		pop	af
		ret

;-----------------------------------------------------------------------------
;	データパケットの送信
;	RLE 圧縮を試みる
;
;	HL	SRC
;	BC	Length
;
PtSendData:
		ld	a,(PtBurstMode)
		or	a
		call	nz,RsEnterBurstMode
		call	SelectRAM
	;jr	PtSendData_unc
		
		; 16 bytes 以下だったら圧縮しない
		inc	b
		dec	b
		jr	nz,PtSendData_1
		ld	a,c
		cp	16+1
		jr	c,PtSendData_unc

PtSendData_1:
 if 0
 		push	hl
 		push	bc
 		call	CalcCRCBlock
 		pop	bc
 		pop	hl
 		push	hl
 		add	hl,bc
 		ld	(hl),e
 		inc	hl
 		ld	(hl),d
 		pop	hl
 		inc	bc
 		inc	bc
 endif
 		
		ld	de,COMPBUFFER+2
		push	hl
		push	bc
		call	RLECompress	; hl = 圧縮後のサイズ
		pop	bc
		ld	d,h
		ld	e,l
		or	a
		inc	hl
		inc	hl
		sbc	hl,bc
		pop	hl
		jr	nc,PtSendData_unc
		
		; 圧縮データを送ることにする
		ld	hl,COMPBUFFER
		ld	(hl),c
		inc	hl
		ld	a,b
		or	01000000b	; RLE
		ld	(hl),a
		dec	hl
		ld	b,d
		ld	c,e
		jr	PtSendData_2
		
PtSendData_unc:
		dec	hl
		ld	(hl),b
		dec	hl
		ld	(hl),c
PtSendData_2:
		inc	bc
		inc	bc
		ld	(PtLastPtr),hl
		ld	(PtLastLen),bc
		ld	a,'$'
		call	PtSend
		
		call	SelectN88ROM
		ld	a,(PtBurstMode)
		or	a
		call	nz,RsExitBurstMode
		ret

;-----------------------------------------------------------------------------
;	データパケットの受信
;	HL	DEST
;
PtRecvData:
		call	RsRecvByte
		cp	'!'
		jr	z,PtRecvData_ex
		cp	'*'
		jr	z,PtRecvData_ex
		cp	'+'
		jr	z,PtRecvData_ex
		cp	'$'
		jr	z,PtRecvData_2
		cp	'#'
		jr	nz,PtRecvData
		
		call	PtRecv
PtRecvData_1:
		push	af
		ccf
		sbc	a,a
		ld	hl,PacketBuffer
		ld	(hl),a
		ld	bc,1
		ld	a,'%'
		call	PtSend
		pop	af
		ret

PtRecvData_ex:
		call	RsUngetc
		jp	Main0a
		
PtRecvData_2:
		push	hl
		ld	hl,COMPBUFFER
		call	PtRecv
		call	PtRecvData_1
		pop	de
		ret	c
		
		call	SelectRAM
		ld	hl,COMPBUFFER
		ld	c,(hl)
		inc	hl
		ld	b,(hl)
		inc	hl
		
		ld	a,b
		rlca
		rlca
		and	3
		jr	z,PtRecvData_unc
		cp	2
		jr	z,PtRecvData_lz
		call	SelectN88ROM
		call	putmsg
		db	'$2Unsupported compression type.$7',13,10,0
		scf
		ret
		
PtRecvData_unc:
		ld	a,b
		and	3fh
		ld	b,a
		ldir
		call	SelectN88ROM
		or	a
		ret
		
PtRecvData_lz:
		ld	a,b
		and	3fh
		ld	b,a
		push	de
		push	bc
		call	LZ77Dec
		ld	e,(hl)
		inc	hl
		ld	d,(hl)
		pop	bc
		pop	hl
		
		push	de
		call	CalcCRCBlock
		pop	hl
		call	SelectN88ROM
		or	a
		sbc	hl,de
		ret	z
		call	putmsg
		db	13,10,7,"$2Decompression error (CRC).$7",13,10,0
		scf
		ret
		
