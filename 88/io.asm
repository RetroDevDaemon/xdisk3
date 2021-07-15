; ----------------------------------------------------------------------------
;	xdisk 2.0
;	copyright (c) 2000 cisc.
; ----------------------------------------------------------------------------
;	割り込みとか
; ----------------------------------------------------------------------------
;	$Id: io.asm,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $

; ----------------------------------------------------------------------------
;	割り込み用意
;
InInit:
		xor	a
		ld	(Critical),a
		di
		ld	a,i
		ld	(InOldI),a
		ld	a,HIGH InVector
		ld	i,a
		ld	a,(PORTE6)
		ld	(InOldPortE6),a
		ld	a,2
		ld	(PORTE6),a
		out	(0e6h),a
		ld	a,0ffh
		out	(0e4h),a
		ei
		ret
		
InCleanup:
		di
		ld	a,0
InOldI		equ	$-1
		ld	i,a
		ld	a,0
InOldPortE6	equ	$-1
		ld	(PORTE6),a
		out	(0e6h),a
		ld	a,0ffh
		out	(0e4h),a
		ei
		ret
		
InDummy:
		push	af
		ld	a,0ffh
		out	(0e4h),a
		pop	af
		ei
		ret
		
;	STOP キー強制終了チェック

InVsync:
		push	af
		in	a,(9)
		rrca
		jr	c,InVsync1
		ld	a,(Critical)
		or	a
		jp	z,exit
InVsync1:
		ld	a,0ffh
		out	(0e4h),a
		pop	af
		ei
		ret

SelectN88ROM:
		di
		ld	a,(PORT31)
		and	not 6
		out	(31h),a
		ld	(PORT31),a
		ei
		ret
		
SelectRAM:
		di
		ld	a,(PORT31)
		or	2
		out	(31h),a
		ld	(PORT31),a
		ei
		ret

; -----------------------------------------------------------------------------
;	テキスト画面モード設定
;
InitScreen:
		ld	a,0e8h
		ld	(TXTATTR),a
		ld	b,80
		ld	a,(0ef88h)
		ld	c,a
		ld	a,1
		ld	(0e6b2h),a
		ld	a,25
		ld	(0e6b3h),a
		ld	a,0ffh
		ld	(0e6b9h),a
		call	6f6bh
		ret

; -----------------------------------------------------------------------------
;	USART clock が可変なら CY
;
Is6fSupported:
		in	a,(6fh)
		or	0f0h
		inc	a
		cp	1
		ccf
		ret
		