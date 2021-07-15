; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	サブシステム側
; ----------------------------------------------------------------------------
;	$Id: diskcode.asm,v 1.4 2000/01/21 01:57:21 cisc Exp $

idr_r		equ	0
idr_length	equ	1
idr_data	equ	3
idr_c		equ	5
idr_h		equ	6
idr_n		equ	7
idr_density	equ	8
idr_st0		equ	9
idr_st1		equ	10
idr_st2		equ	11

DISKMODULE	equ	$
		org	4000h
DISKCODEBEGIN	equ	$

; ----------------------------------------------------------------------------
;	初期化
;	ID テーブルのクリア
;
DsInit:
		ld	hl,d_idtable
		ld	de,d_idtable+1
		ld	bc,3ffh
		ld	(hl),0
		ldir
		jp	0c1h

; ----------------------------------------------------------------------------
;	トラックの書込み
;	arg:	track
;
DsWrite:
		rst	10h
		ld	(d_track),a
		
		call	DsSetFDCClock1
		call	DsSeek
		ld	a,E_SEEK		; seek_error
		jr	c,DsWrite_e
		
		ld	hl,d_data
DsWrite_1:
		ld	a,(hl)
		inc	hl
		ld	d,a
		and	3fh
		jr	z,DsWrite_e
		sub	2
		jr	c,DsWrite_ID
		jr	z,DsWrite_Data
		ld	a,E_SEQUENCE
		jr	DsWrite_e
		
DsWrite_2:
		ld	a,(d_result_st0)
		and	0c0h
		ld	a,E_WRITE
		jr	z,DsWrite_1
DsWrite_e:
		ld	(d_error),a
		jp	0c1h

;
;	WriteData (bit 7 = DELETED)
;	+1	ID
;	+5	DATA...
;
DsWrite_Data:
		ld	a,d
		and	40h
		bit	7,d
		jr	z,DsWrite_Data1
		or	00001001b	; WRITE DELETED DATA
		db	01h
DsWrite_Data1:
		or	00000101b	; WRITE DATA
		call	DsFDCOut
		call	DsSendHus	; hdus1us0
		ld	b,4
DsWrite_Data2:
		ld	a,(hl)		; CHRN
		inc	hl
		call	DsFDCOut
		djnz	DsWrite_Data2
		ld	a,0ffh		; EOT
		call	DsFDCOut
		ld	a,2		; GPL
		call	DsFDCOut
		ld	a,0ffh		; DTL
		call	DsFDCOut
		
		ld	c,(hl)
		inc	hl
		ld	b,(hl)
		inc	hl
		call	DsExecWritePhase
		push	hl
		jr	c,DsWrite_Data4
		
		in	a,(0f8h)	; tc
DsWrite_Data3:
		in	a,(0fah)
		and	20h
		jr	nz,DsWrite_Data3
		call	DsResultPhase6
		jr	DsWrite_Data5

DsWrite_Data4:
		in	a,(0fah)
		and	20h
		jr	nz,DsWrite_Data4
		
		call	DsResultPhase
DsWrite_Data5:
		pop	hl
		jp	DsWrite_2

;
;	WriteID
;	+1	length
;	+2	numsectors
;	+3	gap3
;	+4	D
;	+5...	ID
;		
DsWrite_ID:
		ld	a,d
		and	40h
		or	00001101b	; WRITE ID
		call	DsFDCOut
		call	DsSendHus	; hdus1us0
		ld	a,(hl)		; N
		inc	hl
		call	DsFDCOut
		ld	a,(hl)		; SC
		ld	e,a
		inc	hl
		call	DsFDCOut
		ld	a,(hl)		; GPL
		inc	hl
		call	DsFDCOut
		ld	a,(hl)		; D
		inc	hl
		call	DsFDCOut
		ld	c,0fbh
DsWrite_ID1:
		ld	d,4
DsWrite_ID2:
		ei
		halt
		in	a,(0fah)
		outi
		and	20h
		jr	z,DsWrite_IDe
		dec	d
		jr	nz,DsWrite_ID2
		
		dec	e
		jr	nz,DsWrite_ID1
		push	hl
		call	DsResultPhase
		pop	hl
		jp	DsWrite_2
		
DsWrite_IDe:
		call	DsResultPhase
		ld	a,E_WRITE
		jp	DsWrite_e
		
; ----------------------------------------------------------------------------
;	read track で読み込んだトラックに関する情報を取得
;	ret:	error nsect dlenh dlenl
;
DsGetTrackInfo:
		ld	a,(d_error)
		rst	18h
		ld	a,(d_ismfm)
		add	a,a
		ld	b,a
		ld	a,(d_nsectors)
		or	b
		rst	18h
		ld	hl,(d_datasize)
		ld	a,h
		rst	18h
		ld	a,l
		rst	18h
		jp	0c1h

; ----------------------------------------------------------------------------
;	2d/2hd 機種判定
;
DsIs2HDSupported:
		ld	hl,(1)
		ld	de,-7eh
		add	hl,de
		ret
		
; ----------------------------------------------------------------------------
;	ドライブの設定
;	arg:	drive type	(0=2d, 1=2dd, 3=2hd)
;	ret:	error?
;
DsSetDrive:
		rst	10h		; drive
		ld	(d_drive),a
		rst	10h		; type
		
		call	DsIs2HDSupported
		jr	nz,DsSetDrive_1	; 2hd

		; 2d, type エラーチェック
		or	a
		ld	a,E_TYPE
		jr	nz,DsSetDrive_e
		jr	DsSetDrive_2D
DsSetDrive_1:
		call	DsSetType1
		ld	a,E_FDC
		jr	c,DsSetDrive_e
		ld	a,(d_type)
		jr	DsSetDrive_e
DsSetDrive_2D:
		xor	a
		ld 	(d_type),a
DsSetDrive_e:
		rst	18h
		jp	0c1h

; ----------------------------------------------------------------------------
;	メディアタイプの設定
;
DsSetType_tbl	db	01h,11h,21h,21h

	if 0
DsSetType:
		ld	(d_type),a
		call	DsIs2HDSupported
		ret	z
		
		and	3
		ld	e,a
		ld	d,0
		ld	hl,DsSetType_tbl
		add	hl,de
		
		ld	a,(d_drive)
		ld	c,a
		ld	a,(d_track)
		ld	d,a
		ld	a,(hl)
		call	82ah
		ret
	endif

DsSetType1:
		ld	(d_type),a
		
		ld	e,a
		ld	a,(7f4ah)
		or	00010000b
		out	(0f4h),a
		ld	b,a
		
		ld	a,(d_drive)
		inc	a		; 1 or 2
		rlca
		rlca
		ld	d,a

		cpl
		and	b
		bit	0,e
		jr	z,DsSetType_2
		or	d
DsSetType_2:
		out	(0f4h),a
		ld	b,a
		
		ld	a,d
		rrca
		rrca
		ld	d,a
		cpl
		and	b
		and	00011111b
		bit	1,e
		jr	z,DsSetType_1
		or	d
		or	100000b
DsSetType_1:
		out	(0f4h),a
		and	00101111b
		ld	(7f4ah),a
		out	(0f4h),a
		
		call	DsRecalibrate
		call	DsRecalibrate
		or	a
		ret

; ----------------------------------------------------------------------------
;	FDC の動作クロック等を設定
;
	if	0
DsSetFDCClock:
		call	DsIs2HDSupported
		ret	z
		ld	a,(d_drive)
		ld	c,a
		ld	a,(d_track)
		ld	d,a
		call	833h
		ret
	endif
	
DsSetFDCClock1:
		ld	a,(d_type)
		and	2
		push	af
		rlca
		rlca
		rlca
		rlca
		ld	hl,7f4ah
		ld	b,a
		ld	a,(hl)
		and	00011111b
		or	b
		out	(0f4h),a
		ld	(hl),a
		pop	af
		ld	hl,0da19h
		jr	z,DsSetFDCClock_1
		ld	hl,0df31h
DsSetFDCClock_1:
		ld	a,3
		call	DsFDCOut
		ld	a,h
		call	DsFDCOut
		ld	a,l
		call	DsFDCOut
		
		ret

; ----------------------------------------------------------------------------
;	ディスクの判別
;	arg:	drive
;	ret:	d_type
;
DsIdentifyType:
		rst	10h
		ld	(d_drive),a
		call	DsIs2HDSupported
		jr	z,DsIdentifyType_2D
		
		ld	a,1		; 2DD チェック
		call	DsSetType1

		ld	a,4		; TR4 と TR2 と TR0 の C が全て
		call	DsIdentifyType_s; 異なることを確認できれば 2DD とする
		
		jr	z,DsIdentifyType_2
		ld	a,(d_result_c)	; C=2 ?
		db	0fdh
		ld	h,a
		
		ld	a,2
		call	DsIdentifyType_s
		jr	z,DsIdentifyType_1
		ld	a,(d_result_c)
		db	0fdh
		ld	l,a
		db	0fdh
		cp	h
		jr	z,DsIdentifyType_1
		
		xor	a
		call	DsIdentifyType_s
		jr	z,DsIdentifyType_2
		ld	a,(d_result_c)
		db	0fdh
		cp	h
		jr	z,DsIdentifyType_2D
		db	0fdh
		cp	l
		jr	z,DsIdentifyType_2D
		jr	DsIdentifyType_e
		
DsIdentifyType_1:
		xor	a		; tr0 が読めるか？
		call	DsIdentifyType_s
		jr	nz,DsIdentifyType_2D

DsIdentifyType_2:
		ld	a,3		; 2HD
		call	DsSetType1
		call	DsSetFDCClock1
		
		xor	a
		call	DsIdentifyType_s
		jr	nz,DsIdentifyType_e
		
		ld	a,E_TYPE	; どの方式で読んでも unformatted だった
		jr	DsIdentifyType_err
		
DsIdentifyType_2D:
		xor	a
		call	DsSetType1
DsIdentifyType_e:
		ld	a,(d_type)
DsIdentifyType_err:
		rst	18h
		jp	0c1h

DsIdentifyType_s:
		ld	(d_track),a
		call	DsSeek
		jr	DsIdentifyDensity

; ----------------------------------------------------------------------------
; 	ヘッドをトラックの先頭に位置付ける
;	
;	ret:	z	unformatted
;		A	d_density	(bit0: MFM  bit1:FM)
;
DsIdentifyDensity:
		ld	a,(d_density)
		cp	1
		ld	de,0200h
		jr	nz,DsIdentifyDensity_1
		ld	d,41h
DsIdentifyDensity_1:
		ld	a,d						;   2
		call	DsIdentifyDensity_s	; 前回と同じ密度で	; ->2
		ld	a,d
		xor	43h
		ld	d,a
		call	DsIdentifyDensity_s	; 前回と異なる密度で	; ->1
		ld	a,e			; 今回見つかった場合
		xor	d
		and	3
		call	z,DsIdentifyDensity_2	; ディスクの先頭を探す
		ld	a,e
		and	3
		ld	(d_density),a
		ret

DsIdentifyDensity_2:
		ld	a,d
		xor	40h
		and	40h
		jp	DsReadIDSingle

DsIdentifyDensity_s:
		and	40h
		call	DsReadIDSingle
		ld	a,(d_result_st1)
		rrca
		ret	c			; NO ID
		ld	a,e
		or	d
		ld	e,a
		ret


; ----------------------------------------------------------------------------
;	トラックの読み込み
;	arg:	track
;
DsReadTrack:
		rst	10h
		ld	(d_track),a
		
		call	DsSetFDCClock1
		
		call	DsSeek
		ld	a,E_SEEK		; seek_error
		jr	c,DsReadTrack_e
		
		call	DsReadID
		or	a
		jr	z,DsReadTrack_e
		
		call	DsAssignArea
		
		ld	ix,d_idtable+10h	; 第 2 セクタ目から
		ld	a,(d_nsectors)
		dec	a
		jr	z,DsReadTrack_3
		ld	b,a
DsReadTrack_2:
		push	bc
		call	DsReadData
		ld	bc,10h
		add	ix,bc
		pop	bc
		djnz	DsReadTrack_2
DsReadTrack_3:
		ld	ix,d_idtable
		call	DsReadData
		xor	a
DsReadTrack_e:
		ld	(d_error),a
		jp	0c1h

; ----------------------------------------------------------------------------
;	データエリアの割り当て
;	n<6  - 80h<<n
;	n>=6 - 100h
;
DsAssignArea:
		ld	ix,d_idtable
		ld	a,(d_nsectors)
		ld	c,a
		ld	de,0;	d_data
DsAssignArea_1:
		ld	a,(ix+idr_n)
		cp	7
		jr	nc,DsAssignArea_3	; n>6, 異常?
		
		ld	b,a
		inc	b
		bit	6,(ix+idr_density)
		jr	nc,DsAssignArea_6
		inc	b
DsAssignArea_6:
		ld	hl,20h
DsAssignArea_2:
		add	hl,hl
		djnz	DsAssignArea_2
		jr	DsAssignArea_4
		
DsAssignArea_3:
		ld	hl,100h
DsAssignArea_4:
		push	hl
		add	hl,de
		ld	a,h
		pop	hl
		cp	high (d_datatop-d_data)
		jr	c,DsAssignArea_5
		ld	hl,(d_datatop-d_data)
		or	a
		sbc	hl,de
DsAssignArea_5:
		ld	(ix+idr_data+0),e
		ld	(ix+idr_data+1),d
		ld	(ix+idr_length+0),l
		ld	(ix+idr_length+1),h
		add	hl,de
		ld	de,10h
		add	ix,de
		ex	de,hl
		dec	c
		jr	nz,DsAssignArea_1
		
;		ld	a,d
;		sub	high d_data
;		ld	d,a
		ld	(d_datasize),de
		ret

; ----------------------------------------------------------------------------
;	read data
;	arg:	ix	idr
;
DsReadData:
		ld	l,(ix+idr_data+0)
		ld	h,(ix+idr_data+1)
		ld	de,d_data
		add	hl,de
		ld	c,(ix+idr_length+0)
		ld	b,(ix+idr_length+1)
		
		ld	a,6		; read data
		call	DsRWCmd
		call	DsExecReadPhase
		jr	c,DsReadData_1
		in	a,(0f8h)	; tc
DsReadData_0:
		in	a,(0fah)
		and	20h
		jr	nz,DsReadData_1
		call	DsResultPhase6
		jr	DsReadData_3
		
DsReadData_1:
		in	a,(0fah)
		and	20h
		jr	nz,DsReadData_1
DsReadData_2:
		call	DsResultPhase
DsReadData_3:
		ld	hl,d_result_st0
		ld	a,(hl)
		and	11011000b
		ld	(ix+idr_st0),a
		inc	hl
		ld	a,(hl)
		ld	(ix+idr_st1),a
		inc	hl
		ld	a,(hl)
		ld	(ix+idr_st2),a
		ret

; ----------------------------------------------------------------------------
;	seek
;	d_track	にシーク
;	out:	cy	エラーなら cy
;	uses:	afbc
;
DsSeek:
		ld	a,(06d0h)
		ld	b,a
		ld	a,(d_type)
		and	1		; bit0 = double density
		ld	a,(d_track)
		jr	z,DsSeek_0
		srl	a
DsSeek_0:
		cp	84		; トラック限界チェック
		ccf
		ret	c
		cp	b
		ld	a,0fh		; pre-compensation control
		jr	nc,DsSeek_1
		ld	a,07h
DsSeek_1:
		out	(0f8h),a
		ld	b,3		; リトライ回数
DsSeek_2:
		push	bc
		call	DsSeek_m
		pop	bc
		ret	nc
		call	DsRecalibrate
		djnz	DsSeek_2
		scf
		ret

DsSeek_m:	
		ld	a,0fh			; seek
		call	DsFDCOut
		call	DsSendHus
		ld	a,(d_track)
		srl	a
		ld	c,a
		call	DsFDCOut
DsSeek_sence:
		ei
		halt
		ld	a,08h			; senceintstatus
		call	DsFDCOut
		call	DsFDCIn
		and	11011000b		; 問題となりそうなビット
		ld	b,a
		call	DsFDCIn
		sub	c
		or	b
		ret	z
		scf
		ret

; ----------------------------------------------------------------------------
;	Recalibrate
;
DsRecalibrate:
		ld	a,7		; recalibrate
		call	DsFDCOut
		ld	a,(d_drive)
		call	DsFDCOut
		jp	DsSeek_sence
		
; ----------------------------------------------------------------------------
;	readid
;	トラック中にあるセクタをとりあえず列挙
;	読んだセクタの情報は d_idtable に
;	arg:	none
;	ret:	a	d_nsectors
;	uses:	AFBCDEHLIXIY
;
DsReadID:
		call	DsIdentifyDensity
		ld	ix,d_idtable
		ld	c,0		; c - nsectors
		
		rrca
		push	af
		ld	a,01000000b	; MFM
		call	c,DsReadID_s
		pop	af
		
		rrca
		ld	a,0		; FM
		call	c,DsReadID_s
		
		ld	a,c
		ld	(d_nsectors),a
		ret

		; とりあえずの方針
		; 最初の id と同じ id が出てくるまで読むことにする
DsReadID_s:
		ld	(d_ismfm),a
		ld	a,c
		cp	040h		; 最大 64 セクタ
		ret	nc
		
		ld	e,1
		push	ix
		pop	iy		; iy=1st sector
DsReadID_s_1:
		ld	a,(d_ismfm)
		call	DsReadIDSingle
		ret	nz
		
		ld	hl,d_result_c
		ld	a,(hl)
		ld	(ix+idr_c),a
		sub	(iy+idr_c)
		inc	hl
		ld	d,a
		ld	a,(hl)
		ld	(ix+idr_h),a
		sub	(iy+idr_h)
		inc	hl
		or	d
		ld	d,a
		ld	a,(hl)
		ld	(ix+idr_r),a
		sub	(iy+idr_r)
		inc	hl
		or	d
		ld	d,a
		ld	a,(hl)
		ld	(ix+idr_n),a
		sub	(iy+idr_n)
		or	d
		or	e		; b=1st
		ret	z
		
		ld	a,(d_ismfm)
		ld	(ix+idr_density),a
		
		ld	de,010h
		add	ix,de
		ld	e,0
		inc	c
		ld	a,c
		cp	64
		jr	c,DsReadID_s_1
		ret

; ----------------------------------------------------------------------------

DsReadIDSingle:
		or	00001010b		; readid/mfm
		call	DsFDCOut
		call	DsSendHus
		ei
		halt
		jp	DsResultPhase

; ----------------------------------------------------------------------------
;	e-phase (read)
;	arg:	hl	書きこみ先
;		bc	サイズ
;	uses:	afhlbcde
;
DsExecReadPhase:
		ld	d,b
		dec	d
		ld	b,c
		ld	c,0fbh
		ld	e,20h
DsExecReadPhase_1:
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		ini			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		ini			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		ini			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	36 45
		halt			; 4
		ini			; 16
		jr	nz,DsExecReadPhase_1	; 12/7
		dec	d			; 4
		jp	p,DsExecReadPhase_1	; 10
		in	a,(0f8h)
		scf
		ret
		
; ----------------------------------------------------------------------------
;	e-phase (read)
;	arg:	hl	書きこみ先
;		bc	サイズ
;	uses:	afhlbcde
;
DsExecWritePhase:
		ld	d,b
		dec	d
		ld	b,c
		ld	c,0fbh
		ld	e,20h
DsExecWritePhase_1:
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		outi			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		outi			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	45
		halt			; 4
		in	a,(0fah)	; 11
		outi			; 16
		and	e		; 4
		ret	z		; 5
		
		ei			; 4	36 45
		halt			; 4
		outi			; 16
		jr	nz,DsExecWritePhase_1	; 12/7
		dec	d			; 4
		jp	p,DsExecWritePhase_1	; 10
		in	a,(0f8h)
		scf
		ret
		

; ----------------------------------------------------------------------------
;	read/write 系コマンドを送る
;	arg:	a	command
;		ix	idr
;	uses:	a
;
DsRWCmd:
		or	(ix+idr_density)
		call	DsFDCOut
		call	DsSendHus	; hdus1us0
		ld	a,(ix+idr_c)	; c
		call	DsFDCOut
		ld	a,(ix+idr_h)	; h
		call	DsFDCOut
		ld	a,(ix+idr_r)	; r
		call	DsFDCOut
		ld	a,(ix+idr_n)	; n
		call	DsFDCOut
		ld	a,01h		; eot (てきと～)
		call	DsFDCOut
		ld	a,08h		; gpl (てきと～)
		call	DsFDCOut
		ld	a,0ffh		; dtl
		jr	DsFDCOut

; ----------------------------------------------------------------------------
;	hd us0 us1 を出力
;	uses:	af
;
DsSendHus:
		ld	a,(d_drive)
		push	bc
		ld	b,a
		ld	a,(d_track)
		and	1
		add	a,a
		add	a,a
		add	a,b
		pop	bc
		jr	DsFDCOut

; ----------------------------------------------------------------------------
;	fdc にデータ出力
;	in:	a
;	uses:	none
;
DsFDCOut:
		push	af
DsFDCOut_1:
		in	a,(0fah)
		xor	10000000b
		and	11000000b
		jr	nz,DsFDCOut_1
		pop	af
		out	(0fbh),a
		ret

; ----------------------------------------------------------------------------
;	fdc からデータ入力
;	out:	a	入力したデータ
;	uses:	f
;
DsFDCIn:
		in	a,(0fah)
		cpl
		and	11000000b
		jr	nz,DsFDCIn
		in	a,(0fbh)
		ret

; ----------------------------------------------------------------------------
;	resultphase
;	7 byte の result を取得
;
;	out:	z	コマンド正常終了
;		nz	コマンド異常終了
;	uses:	a f h l
;
DsResultPhase6:
		push	bc
		dec	hl
		ld	a,(hl)
		ld	hl,d_resultcode
		ld	(hl),a
		inc	hl
		ld	b,6
		jr	DsResultPhase_1
DsResultPhase:
		push	bc
		ld	b,7
		ld	hl,d_resultcode
DsResultPhase_1:
		call	DsFDCIn
		ld	(hl),a
		inc	hl
		djnz	DsResultPhase_1
		ld	a,(d_result_st0)
		and	01000000b
		pop	bc
		ret


; ----------------------------------------------------------------------------
;	高速受信
;
DsRapidRecv:
		call	DsRecvBlockParams
		call	DsRecvFast
		jp	0c1h

; ----------------------------------------------------------------------------
;	高速送信
;
DsRapidSend:
		call	DsRecvBlockParams
		call	DsSendFast
		jp	0c1h

; ----------------------------------------------------------------------------
;	高速ブロック受信
;	in	hl:	データ格納先
;		bc:	データサイズ/4
;
DsRecvFast:
		ld	a,93h
		out	(0ffh),a
DsRecvFast_1:
		in	a,(0feh)
		rrca
		jp	nc,DsRecvFast_1
		in	a,(0fch)
		ld	e,a
		in	a,(0fdh)
		ld	d,a
		ld	a,9
		out	(0ffh),a
		ld	(hl),d
		inc	hl
		ld	(hl),e
		inc	hl
DsRecvFast_2:
		in	a,(0feh)
		rrca
		jp	c,DsRecvFast_2
		in	a,(0fch)
		ld	e,a
		in	a,(0fdh)
		ld	d,a
		ld	a,8
		out	(0ffh),a
		ld	(hl),d
		inc	hl
		ld	(hl),e
		cpi
		jp	pe,DsRecvFast_1
		ld	a,91h
		out	(0ffh),a
		ret

; ----------------------------------------------------------------------------
;	高速ブロック送信
;	in	hl:	データ格納先
;		bc:	データサイズ/4
;
DsSendFast:
		ld	a,81h
		out	(0ffh),a
		ld	d,(hl)
		inc	hl
		ld	e,(hl)
		inc	hl
		jr	DsSendFast_2
		
DsSendFast_1:
		in	a,(0feh)
		rrca
		jp	c,DsSendFast_1
		
DsSendFast_2:
		ld	a,e
		out	(0fch),a
		ld	a,d
		out	(0fdh),a
		ld	a,9
		out	(0ffh),a
		ld	d,(hl)
		inc	hl
		ld	e,(hl)
		inc	hl
		
DsSendFast_3:
		in	a,(0feh)
		rrca
		jp	nc,DsSendFast_3
		
		ld	a,e
		out	(0fch),a
		ld	a,d
		out	(0fdh),a
		ld	a,8
		out	(0ffh),a
		ld	d,(hl)
		inc	hl
		ld	e,(hl)
		cpi
		jp	pe,DsSendFast_1
		
DsSendFast_4:
		in	a,(0feh)
		rrca
		jp	c,DsSendFast_4
		
		ld	a,91h
		out	(0ffh),a
		ret
		
; ----------------------------------------------------------------------------
;	ブロック転送のパラメータ
;
DsRecvBlockParams:
		rst	10h
		ld	h,a
		rst	10h
		ld	l,a
		rst	10h
		ld	b,a
		rst	10h
		ld	c,a
		ret

; ----------------------------------------------------------------------------
;	ワークエリア
;
d_drive		db	0
d_track		db	0
d_nsectors	db	0

d_ismfm		db	0
d_type		db	0
d_error		db	0
d_density	db	0

d_datasize	dw	0
d_resultcode	equ	$
d_result_st0	db	0
d_result_st1	db	0
d_result_st2	db	0
d_result_c	db	0
d_result_h	db	0
d_result_r	db	0
d_result_n	db	0

DISKCODELEN	equ	$-DISKCODEBEGIN

d_idtable	ds	400h

d_data		equ	($+0ffh) and 0ff00h
d_datatop	equ	07f00h

diskblockend	equ	$
		org	DISKMODULE+DISKCODELEN

