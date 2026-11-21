; ------------------------------------------------------------------------
; 名称：fat.s
; 说明：在 1.44MB 的 FAT12 软盘上找文件、按簇链读入；检查 iNES 头，把 PRG 写进程序 RAM 第 1 页，把 CHR 暂存到 WRAM。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：先读完整个文件（CHR 只暂存在 WRAM），之后才写字符 RAM，所以读盘出错时字体还在，提示仍能显示。
;       只支持映射器 0、PRG 16K 或 32K、CHR 0 或 8K、没有 trainer、非 NES 2.0 头。
; ------------------------------------------------------------------------
.setcpu "6502"
.include "defs.inc"

.export load_game
.import fdc_read_sector, copy_bytes, ram_copy

.segment "CODE"

; 检查引导扇区：512 字节、1 个扇区一簇、9 个扇区的 FAT
check_boot:
    lda #$00
    sta LBA
    sta LBA+1
    jsr fdc_read_sector
    lda SECBUF+11
    bne @bad
    lda SECBUF+12
    cmp #$02
    bne @bad
    lda SECBUF+13
    cmp #$01
    bne @bad
    lda SECBUF+22
    cmp #$09
    bne @bad
    lda #$00
    rts
@bad:
    lda #ERR_DISK
    rts

; 在根目录（第 19 个扇区起 14 个扇区）找 NAME83；找到时 A 为 1，FCL、FSIZE 为首簇与大小
find_entry:
    lda #$13
    sta LBA
    lda #$00
    sta LBA+1
    lda #14
    sta K
@sector:
    jsr fdc_read_sector
    lda #<SECBUF
    sta ZD
    lda #>SECBUF
    sta ZD+1
    ldx #16
@entry:
    ldy #$00
    lda (ZD),y
    beq @notfound
    cmp #$E5
    beq @next
    ldy #11
    lda (ZD),y
    and #$18
    bne @next
    ldy #10
@cmp:
    lda (ZD),y
    cmp NAME83,y
    bne @next
    dey
    bpl @cmp
    ldy #26
    lda (ZD),y
    sta FCL
    iny
    lda (ZD),y
    sta FCL+1
    ldy #28
    lda (ZD),y
    sta FSIZE
    iny
    lda (ZD),y
    sta FSIZE+1
    ldy #30
    lda (ZD),y
    beq @found
    lda #$FF
    sta FSIZE
    sta FSIZE+1
@found:
    lda #$01
    rts
@next:
    lda ZD
    clc
    adc #32
    sta ZD
    bcc @nc
    inc ZD+1
@nc:
    dex
    bne @entry
    inc LBA
    dec K
    bne @sector
@notfound:
    lda #$00
    rts

; 检查 SECBUF 里的 iNES 头，设置 PRG、CHR、PAY（PRG 加 CHR）与 MIRROR_VAL；成功时 A 为 0
check_header:
    lda SECBUF+0
    cmp #'N'
    beq @h1
    jmp @hdr
@h1:
    lda SECBUF+1
    cmp #'E'
    beq @h2
    jmp @hdr
@h2:
    lda SECBUF+2
    cmp #'S'
    beq @h3
    jmp @hdr
@h3:
    lda SECBUF+3
    cmp #$1A
    beq @h4
    jmp @hdr
@h4:
    lda SECBUF+6
    and #$04
    beq @h5
    jmp @size
@h5:
    lda SECBUF+7
    and #$0C
    cmp #$08
    bne @h6
    jmp @size
@h6:
    lda SECBUF+6
    and #$F0
    sta TMP
    lda SECBUF+7
    and #$F0
    ora TMP
    beq @h7
    jmp @mapper
@h7:
    lda SECBUF+4
    cmp #$01
    beq @prg16
    cmp #$02
    beq @prg32
    jmp @size
@prg32:
    lda #$00
    sta PRG
    lda #$80
    sta PRG+1
    jmp @chr
@prg16:
    lda #$00
    sta PRG
    lda #$40
    sta PRG+1
@chr:
    lda SECBUF+5
    beq @chr0
    cmp #$01
    beq @chr8
    jmp @size
@chr8:
    lda #$00
    sta CHR
    lda #$20
    sta CHR+1
    jmp @pay
@chr0:
    lda #$00
    sta CHR
    sta CHR+1
@pay:
    clc
    lda PRG
    adc CHR
    sta PAY
    lda PRG+1
    adc CHR+1
    sta PAY+1
    lda SECBUF+6
    and #$01
    eor #$01
    ora #$04
    sta MIRROR_VAL
    lda #$00
    rts
@hdr:
    lda #ERR_HEADER
    rts
@size:
    lda #ERR_SIZE
    rts
@mapper:
    lda #ERR_MAPPER
    rts

; 按簇号 CL 查 FAT12 的下一簇，结果写回 CL
fat_next:
    lda CL+1
    lsr
    sta TMP+1
    lda CL
    ror
    sta TMP
    lda CL
    clc
    adc TMP
    sta TMP
    lda CL+1
    adc TMP+1
    sta TMP+1
    lda TMP
    clc
    adc #<FAT_BASE
    sta ZD
    lda TMP+1
    adc #>FAT_BASE
    sta ZD+1
    ldy #$00
    lda (ZD),y
    sta T0
    iny
    lda (ZD),y
    sta T1
    lda CL
    and #$01
    bne @odd
    lda T0
    sta CL
    lda T1
    and #$0F
    sta CL+1
    rts
@odd:
    lda T0
    lsr
    lsr
    lsr
    lsr
    sta TMP
    lda T1
    asl
    asl
    asl
    asl
    ora TMP
    sta CL
    lda T1
    lsr
    lsr
    lsr
    lsr
    sta CL+1
    rts

; 把 SECBUF 里的一个扇区的负载交给 PRG（程序 RAM）与 CHR（WRAM）；CNT 为字节数，P 为负载位置，结束时 P 前进 CNT
emit_payload:
    lda P+1
    cmp PRG+1
    bcc @prg
    bne @chr
    lda P
    cmp PRG
    bcs @chr
@prg:
    sec
    lda PRG
    sbc P
    sta DIFF
    lda PRG+1
    sbc P+1
    sta DIFF+1
    lda CNT
    cmp DIFF
    lda CNT+1
    sbc DIFF+1
    bcc @use_cnt
    lda DIFF
    sta N1
    lda DIFF+1
    sta N1+1
    jmp @copy
@use_cnt:
    lda CNT
    sta N1
    lda CNT+1
    sta N1+1
@copy:
    lda P
    sta ZD
    lda P+1
    ora #$80
    sta ZD+1
    lda N1
    sta ZN
    lda N1+1
    sta ZN+1
    lda #$81
    sta ZPAGE
    jsr ram_copy
    clc
    lda P
    adc N1
    sta P
    lda P+1
    adc N1+1
    sta P+1
    sec
    lda CNT
    sbc N1
    sta REM
    lda CNT+1
    sbc N1+1
    sta REM+1
    jmp @chr_part
@chr:
    lda CNT
    sta REM
    lda CNT+1
    sta REM+1
@chr_part:
    lda REM
    ora REM+1
    beq @finish
    sec
    lda P
    sbc PRG
    sta ZD
    lda P+1
    sbc PRG+1
    clc
    adc #$60
    sta ZD+1
    lda REM
    sta ZN
    lda REM+1
    sta ZN+1
    jsr copy_bytes
@finish:
    clc
    lda P
    adc REM
    sta P
    lda P+1
    adc REM+1
    sta P+1
    rts

; 加载游戏：成功时 A 为 0，PRG、CHR 与 MIRROR_VAL 已设好；失败时 A 为错误码
load_game:
    jsr check_boot
    beq @boot_ok
    rts
@boot_ok:
    jsr find_entry
    bne @found
    lda #ERR_OPEN
    rts
@found:
    lda FCL
    clc
    adc #31
    sta LBA
    lda FCL+1
    adc #$00
    sta LBA+1
    jsr fdc_read_sector
    jsr check_header
    beq @header_ok
    rts
@header_ok:
    lda PAY
    clc
    adc #16
    sta TMP
    lda PAY+1
    adc #$00
    sta TMP+1
    lda FSIZE
    cmp TMP
    lda FSIZE+1
    sbc TMP+1
    bcs @size_ok
    lda #ERR_TRUNC
    rts
@size_ok:
    lda TMP
    clc
    adc #$FF
    lda TMP+1
    adc #$01
    lsr
    sta NSEC_ALL
    sta NSEC
    lda #$01
    sta LBA
    lda #$00
    sta LBA+1
    lda #$09
    sta K
    lda #<FAT_BASE
    sta ZD
    lda #>FAT_BASE
    sta ZD+1
@fat:
    jsr fdc_read_sector
    lda #<SECBUF
    sta ZS
    lda #>SECBUF
    sta ZS+1
    lda #$00
    sta ZN
    lda #$02
    sta ZN+1
    jsr copy_bytes
    inc LBA
    dec K
    bne @fat
    lda FCL
    sta CL
    lda FCL+1
    sta CL+1
    lda #$00
    sta IDX
@cluster:
    ldy IDX
    lda CL
    sta CLUSTERS,y
    lda CL+1
    sta CLUSTERS+1,y
    inc IDX
    inc IDX
    dec NSEC
    beq @links_ok
    jsr fat_next
    lda CL+1
    cmp #$0C
    bcs @links_bad
    lda CL+1
    bne @cluster
    lda CL
    cmp #$02
    bcs @cluster
@links_bad:
    lda #ERR_TRUNC
    rts
@links_ok:
    lda NSEC_ALL
    sta NSEC
    lda #$00
    sta P
    sta P+1
    sta K
@sector:
    lda K
    asl
    tay
    lda CLUSTERS,y
    sta CL
    lda CLUSTERS+1,y
    sta CL+1
    clc
    lda CL
    adc #31
    sta LBA
    lda CL+1
    adc #$00
    sta LBA+1
    jsr fdc_read_sector
    lda K
    bne @rest
    lda #<(SECBUF+16)
    sta ZS
    lda #>(SECBUF+16)
    sta ZS+1
    lda #<496
    sta CNT
    lda #>496
    sta CNT+1
    jmp @emit
@rest:
    lda #<SECBUF
    sta ZS
    lda #>SECBUF
    sta ZS+1
    sec
    lda PAY
    sbc P
    sta CNT
    lda PAY+1
    sbc P+1
    sta CNT+1
    lda CNT+1
    cmp #$02
    bcc @emit
    lda #$00
    sta CNT
    lda #$02
    sta CNT+1
@emit:
    jsr emit_payload
    inc K
    lda K
    cmp NSEC
    bne @sector
    lda #$00
    rts
