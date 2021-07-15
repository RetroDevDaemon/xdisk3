; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	$Id: readtrk.asm,v 1.3 2000/01/21 01:57:22 cisc Exp $
		
; ----------------------------------------------------------------------------
;	ReadTrack
;	指定されたトラックを読み込む
;	arg:	[03] TRACK
;	res:	成否
;	
CmdReadTrack:
		ld	a,(CmdPacket+1)
		ld	e,a
		call	DiReadTrack
		xor	a
		jp	PtSendByteResult
		
; ----------------------------------------------------------------------------
;	SendTrack
;	最後に読み込んだトラックを転送する
;	
;	送信内容(圧縮アリ)
;		BYTE	ns (セクタ数) / エラー
;		16*ns	ID (圧縮済み)
;		????	DATA
;
CmdSendTrack:
		call	MakeTrackPacket
		push	hl
		push	bc
		call	PutTrackInfo
		pop	bc
		pop	hl
		ld	a,(CmdPacket+1)
		ld	(PtBurstMode),a
		jp	PtSendData

; ----------------------------------------------------------------------------
;	SendAndReadTrack
;	前回読み込んだトラックデータを転送し，それと並列して指定された
;	トラックを読み込む
;
;	arg:	[05] TRACK MODE
;	res:	TRACKDATA
;
CmdSendAndReadTrack:
		call	MakeTrackPacket
		push	hl
		push	bc
		call	PutTrackInfo
		ld	de,(CmdPacket+1)
		ld	a,d
		ld	(PtBurstMode),a
		call	DiReadTrack
		pop	bc
		pop	hl
		jp	PtSendData
		


MakeTrackPacket:
		call	DiGetTrackInfo
		jr	c,MakeTrackPacket_1
		ld	a,e
		ld	(DATA),a
		or	a
		jr	z,MakeTrackPacket_1
		
		; ID を受け取る
		push	bc
		add	a,a
		add	a,a
		ld	c,a
		ld	b,0
		rl	b
		ld	hl,IDTABLE
		ld	de,d_idtable
		call	DiRapidRecv
		ld	de,DATA+1
		call	IDCompress
		pop	bc
		
		; DATA を受け取る
		ex	de,hl
		ld	de,d_data
		inc	bc
		inc	bc
		inc	bc
		srl	b
		rr	c
		srl	b
		rr	c
		call	DiRapidRecv
 if 0		
	push hl
	ld a,(DiTrack)
	push af
	call putmsg
	db ' $6$b$5:$7',0
	pop hl
	push hl
	ld de,-8
	add hl,de
	ld b,8
mtp_1:
	call SelectRAM
	ld a,(hl)
	inc hl
	push hl
	push af
	call SelectN88ROM
	call putmsg
	db '$b',0
	pop hl
	djnz mtp_1
	pop hl
 endif
		ld	de,-DATA
		add	hl,de
		ld	b,h
		ld	c,l
		ld	hl,DATA
		ret
MakeTrackPacket_1:
		ld	hl,DATA
		ld	(hl),a
		ld	bc,1
		ret

		
; ----------------------------------------------------------------------------
;	ID TABLE を RLE 圧縮に適した形に加工
;	IDTABLE->DE
;
IDCompress:
		ld	a,(DiTrack)
		ld	b,a
		srl	a
		ld	(IDTABLE-10h+idr_c),a
		ld	a,b
		and	1
		ld	(IDTABLE-10h+idr_h),a
		
		ld	ix,IDTABLE
		call	IDCalcSectorSize
		ld	a,l
		cpl
		ld	l,a
		ld	a,h
		cpl
		ld	h,a
		inc	hl
		ld	(IDTABLE-10h+idr_data),hl
		
		ld	a,(DiSectors)
		or	a
		ret	z
		
IDCompress_1:
		push	af
		
		; R, RP + 1 - R
		ld	a,(ix-10h+idr_r)
		sub	(ix+idr_r)
		inc	a
		ld	(de),a
		inc	de
		
		; LENGTH = D - SECTSIZE
		call	IDCalcSectorSize
		push	hl
		ld	a,(ix+idr_length)
		sub	l
		ld	(de),a
		inc	de
		ld	a,(ix+idr_length+1)
		sbc	a,h
		ld	(de),a
		inc	de
		
		; DATA = DP + SECTSIZE - D
		pop	hl
		ld	c,(ix-10h+idr_data)
		ld	b,(ix-10h+idr_data+1)
		add	hl,bc
		ld	c,(ix+idr_data)
		ld	b,(ix+idr_data+1)
		or	a
		sbc	hl,bc
		ld	a,l
		ld	(de),a
		inc	de
		ld	a,h
		ld	(de),a
		inc	de
		
		; C, H, N, density, st0, st1, st2 = CURRENT - PREV
		ld	b,7
IDCompress_2:
		ld	a,(ix+idr_c)
		sub	(ix-10h+idr_c)
		inc	ix
		ld	(de),a
		inc	de
		djnz	IDCompress_2
		ld	bc,16 - 7
		add	ix,bc
		pop	af
		dec	a
		jr	nz,IDCompress_1
		ret
		
; ----------------------------------------------------------------------------
;	N の値からセクタサイズを求める
;
IDCalcSectorSize:
		ld	a,(ix+idr_n)
		cp	7
		jr	c,IDCalcSectorSize_1
		ld	hl,100h
		ret
		
IDCalcSectorSize_1:
		bit	6,(ix+idr_density)
		jr	nc,IDCalcSectorSize_2
		inc	a
IDCalcSectorSize_2:
		inc	a
		
		ld	b,a
		ld	hl,20h
IDCalcSectorSize_3:
		add	hl,hl
		djnz	IDCalcSectorSize_3
		ret

; ----------------------------------------------------------------------------
;	"Track ??: MFM xx sectors xxxxh bytes" という情報を表示
;
PutTrackInfo:
		ld	a,(DiTrack)
		ld	e,a
		ld	d,0
		push	de
		call	putmsg
		db	"$7Track $5$t$7: ",0
		
		ld	a,(DiSectors)
		or	a
		jr	z,PutTrackInfo_unf
		
		ld	hl,(DiDataLength)
		push	hl
		push	af
		
		call	DiGetDensity
		dec	a
		jr	z,PutTrackInfo_1
		dec	a
		jr	z,PutTrackInfo_2
		call	putmsg
		db	"$5MIX",0
		jr	PutTrackInfo_3
PutTrackInfo_1:
		call	putmsg
		db	"$4MFM",0
		jr	PutTrackInfo_3
PutTrackInfo_2:
		call	putmsg
		db	"$3FM ",0
PutTrackInfo_3:
		call	putmsg
		db	" $6$d $7sectors",0
		call	DiCountErrSectors
		or	a
		jr	z,PutTrackInfo_4
		push	af
		call	putmsg
		db	"($6$d $2errors$7)",0
		
PutTrackInfo_4:
		call	putmsg
		db	", $6$w$7h bytes.",13,10,0
		ret

PutTrackInfo_unf:
		call	putmsg
		db	"$1Unformatted$7",13,10,0
		ret
		