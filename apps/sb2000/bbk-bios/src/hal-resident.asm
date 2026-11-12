;---------------------------------------;
; 名称：hal-resident.asm
; 说明：SB2000 硬件适配层中必须常驻 RAM 的部分：页切换、$C000-$FFFF 切换、$2000 写口。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-12
; 备注：本文件被包含在 bios.asm 的“临时部分”里（MoveRAMEnding 之前），随它一起复制到 $5800 运行，
;       因此只用相对跳转，不用绝对跳转，各过程只用栈保存寄存器，可以被 NMI 重入。
;       $C000-$FFFF 切到 DRAM 时 ROM 里的代码看不见，所以这些过程在 RAM 里也要有一份。
;       RAM 里的调用写 HalXxx-$C000+$5800，ROM 里的调用写 HalXxx。
;---------------------------------------;
;    设置 $8000-$BFFF 的 16K 页		;
;	A（HalPageX 为 X）=步步高页号，保存 A,X,Y 和标志。	;
;	页号 $80-$FF 是 DRAM，共 16 页，页号只取低 4 位（真机对高位也不译码）；	;
;	页号 <$80 是 ROM 映像页，取低 3 位。	;
;---------------------------------------;
HalPageX:				; 页号在 X
	php				;
	pha				;
	txa				;
	pha				;
	txa				;
	clv				;
	bvc	hpg04			;
HalPageA:				; 页号在 A
	php				;
	pha				;
	txa				;
	pha				;
	tsx				;
	lda	$0102,x			; 原 A
hpg04:	cmp	#$80			;
	bcs	hpg08			;
	and	#7			; ROM 映像页：bank = $E0+页*4
	asl				;
	asl				;
	ora	#$e0			;
	bne	hpg12			;
hpg08:	and	#$0f			; DRAM：bank = $80+页*4
	asl				;
	asl				;
	ora	#$80			;
hpg12:	tax				;
	stx	SBWindow0		;
	inx				;
	stx	SBWindow0+1		;
	inx				;
	stx	SBWindow0+2		;
	inx				;
	stx	SBWindow0+3		;
	pla				;
	tax				;
	pla				;
	plp				;
	rts				;
;---------------------------------------;
;    视频控制口 $FF01			;
;	A=值。位 3：0 时 $C000-$FFFF 是 ROM 映像最后一页，1 时是 DRAM 最后一页；	;
;	位 1-0 是名称表镜像。保存 A,X,Y 和标志。	;
;---------------------------------------;
HalVideoCtrl:				;
	php				;
	pha				;
	txa				;
	pha				;
	tsx				;
	lda	$0102,x			;
	and	#3			;
	sta	vMir			;
	lda	$0102,x			;
	and	#8			;
	beq	hvc04			;
	ldx	#$bc			; DRAM 第 15 页
	bne	hvc08			;
hvc04:	ldx	#$fc			; ROM 映像第 7 页
hvc08:	stx	SBWindow0+4		;
	inx				;
	stx	SBWindow0+5		;
	inx				;
	stx	SBWindow0+6		;
	inx				;
	stx	SBWindow0+7		;
	pla				;
	tax				;
	pla				;
	plp				;
	rts				;
;---------------------------------------;
;    虚拟 $2000 / 原 $2008		;
;	A=值，保存 A,X,Y 和标志。	;
;	HalW2000 记下虚拟值；HalW2000T 原来写 $2008（它是 $2000 的镜像，用来暂时关 NMI），只写真口。	;
;	真 $2000 的位 2（地址增量）固定为 0，增量由虚拟 PPU 按虚拟值处理。	;
;---------------------------------------;
HalW2000:				;
	php				;
	pha				;
	sta	vReg2000		;
	and	#$fb			;
	sta	$2000			;
	pla				;
	plp				;
	rts				;
HalW2000T:				;
	php				;
	pha				;
	and	#$fb			;
	sta	$2000			;
	pla				;
	plp				;
	rts				;
;---------------------------------------;
;    键盘中断				;
;	SB2000 的键盘经 $4020 送来 XT 扩展码（按下为通码，松开为通码加 $80，前缀 $E0），由 IRQ 通知。	;
;	进中断时 BRKINTEntry 先调用本过程：是键盘中断就把扩展码放进队列并应答，返回 C=1；	;
;	不是键盘中断返回 C=0。改 A（调用者已存），保存 X,Y。	;
;	应答按 SB2000 BIOS 的做法：读前 $4022 写 0，读后写 $80；再用 $4032 清掉中断请求，	;
;	只留键盘中断不屏蔽（$DF），其余中断源（定时器、鼠标、软驱）都屏蔽。	;
;---------------------------------------;
HalKbdIrq:				;
	lda	$4033			;
	and	#$20			;
	beq	hki20			;
	txa				;
	pha				;
	lda	#0			;
	sta	$4022			;
	lda	$4020			;
	ldx	vKbTail			;
	sta	vKbBuf,x		;
	inx				;
	txa				;
	and	#$0f			;
	cmp	vKbHead			; 队列满则丢弃这一个
	beq	hki08			;
	sta	vKbTail			;
hki08:	lda	#$80			;
	sta	$4022			;
	lda	#$ff			;
	sta	$4032			;
	lda	#$df			;
	sta	$4032			;
	pla				;
	tax				;
	sec				;
	rts				;
hki20:	clc				;
	rts				;
