; ------------------------------------------------------------------------
; 名称：fdc.s
; 说明：软驱控制器（uPD765 兼容）的驱动：启动马达与校准、寻道、按逻辑扇区读取 512 字节到 SECBUF。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：只用 1.44MB 的 0 号驱动器，不用 DMA，逐字节轮询主状态；超时或读出错误都经 show_error 显示提示。
; ------------------------------------------------------------------------
.setcpu "6502"
.include "defs.inc"

.export fdc_init, fdc_stop, fdc_read_sector
.import show_error, delay

.segment "CODE"

; 超时或读错误：显示读盘错误，不返回
fdc_fail:
    ldx #ERR_READ
    jmp show_error

; 计时加一，超时则报错
tmo_tick:
    inc TMO
    bne @ok
    inc TMO+1
    bne @ok
    jmp fdc_fail
@ok:
    rts

; 启动马达（约 0.5 秒），规定速率，校准到 0 柱面
fdc_init:
    lda #$00
    sta FDC_DOR
    lda #$1C
    sta FDC_DOR
    lda #$00
    sta FDC_DCR
    lda #$03
    jsr delay
    lda #$03
    jsr fdc_put
    lda #$DF
    jsr fdc_put
    lda #$03
    jsr fdc_put
    lda #$07
    jsr fdc_put
    lda #$00
    jsr fdc_put
    jsr fdc_sense_wait
    and #$C0
    beq @ok
    jmp fdc_fail
@ok:
    lda #$00
    sta CURCYL
    rts

; 停转并清掉控制器的中断
fdc_stop:
    lda #$08
    jsr fdc_put
    jsr fdc_get
    jsr fdc_get
    lda #$04
    sta FDC_DOR
    rts

; 把 A 写进数据寄存器
fdc_put:
    pha
    lda #$00
    sta TMO
    sta TMO+1
@wait:
    lda FDC_MSR
    and #$C0
    cmp #$80
    beq @go
    jsr tmo_tick
    jmp @wait
@go:
    pla
    sta FDC_DATA_W
    rts

; 从数据寄存器读一字节到 A
fdc_get:
    lda #$00
    sta TMO
    sta TMO+1
@wait:
    lda FDC_MSR
    and #$C0
    cmp #$C0
    beq @go
    jsr tmo_tick
    jmp @wait
@go:
    lda FDC_DATA_R
    rts

; 发送 SENSE INTERRUPT 直到有中断（ST0 不为 $80）；A 返回 ST0，T0 返回 PCN
fdc_sense_wait:
    lda #$00
    sta TMO2
    sta TMO2+1
@again:
    lda #$08
    jsr fdc_put
    jsr fdc_get
    pha
    jsr fdc_get
    sta T0
    pla
    cmp #$80
    bne @done
    inc TMO2
    bne @again
    inc TMO2+1
    bne @again
    jmp fdc_fail
@done:
    rts

; 寻道到 A 指定的柱面
fdc_seek:
    sta T1
    lda #$0F
    jsr fdc_put
    lda #$00
    jsr fdc_put
    lda T1
    jsr fdc_put
    jsr fdc_sense_wait
    and #$C0
    beq @ok
    jmp fdc_fail
@ok:
    lda T0
    sta CURCYL
    rts

; 读一个逻辑扇区（LBA，每柱 36 个扇区）到 SECBUF
fdc_read_sector:
    lda LBA
    sta TMP
    lda LBA+1
    sta TMP+1
    lda #$00
    sta CYL
@cyl:
    lda TMP+1
    bne @sub
    lda TMP
    cmp #36
    bcc @cyl_done
@sub:
    lda TMP
    sec
    sbc #36
    sta TMP
    lda TMP+1
    sbc #$00
    sta TMP+1
    inc CYL
    jmp @cyl
@cyl_done:
    lda #$00
    sta HEAD
    lda TMP
    cmp #18
    bcc @sector
    lda #$01
    sta HEAD
    lda TMP
    sec
    sbc #18
    sta TMP
@sector:
    lda TMP
    clc
    adc #$01
    sta SECT
    lda CYL
    cmp CURCYL
    beq @read
    lda CYL
    jsr fdc_seek
@read:
    lda #$46
    jsr fdc_put
    lda HEAD
    asl
    asl
    jsr fdc_put
    lda CYL
    jsr fdc_put
    lda HEAD
    jsr fdc_put
    lda SECT
    jsr fdc_put
    lda #$02
    jsr fdc_put
    lda SECT
    jsr fdc_put
    lda #$1B
    jsr fdc_put
    lda #$FF
    jsr fdc_put
    lda #<SECBUF
    sta ZD
    lda #>SECBUF
    sta ZD+1
    ldx #$02
@page:
    ldy #$00
@byte:
    jsr fdc_get
    sta (ZD),y
    iny
    bne @byte
    inc ZD+1
    dex
    bne @page
    jsr fdc_get
    sta RES_ST0
    jsr fdc_get
    sta RES_ST1
    jsr fdc_get
    sta RES_ST2
    jsr fdc_get                     ; C、H、R、N 不用
    jsr fdc_get
    jsr fdc_get
    jsr fdc_get
    lda RES_ST1
    and #$37
    bne @fail
    lda RES_ST2
    and #$37
    bne @fail
    lda RES_ST0
    and #$08
    bne @fail
    rts
@fail:
    jmp fdc_fail
