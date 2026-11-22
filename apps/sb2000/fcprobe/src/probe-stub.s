; ------------------------------------------------------------------------
; 名称：probe-stub.s
; 说明：FC 兼容模式探测桩：复制到扩展 RAM 的 $6100 运行，在标准 PPU 模式下测试内存写入、PPU、软驱与键盘，结果写到 $7F00 起。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-12
; 备注：桩在 FC 模式下由 CPU 复位后从 $6101 开始执行，所以标号要换算成 $6100 起的运行地址（RUNAT）；
;       $8000 以上的写入只用偶数地址并写 0，对 MMC3 只是选择寄存器、镜像、IRQ 锁存与关 IRQ，不会改变 PRG 映射。
;       结果区：+0 启动标记 $01，+1 心跳，+2 全部测完为 $FF；+$10 起每个 $8000 以上写入测试 2 字节（写前、写后）；
;       +$20 起 RAM 写入读回；+$30 名表读回，+$31 图案表写后读回，+$32 DMA 复制到显存后读回；+$40 起软驱；+$50 起键盘等寄存器的最近一次读数。
; ------------------------------------------------------------------------

        .p02
        .export _stub_start, _stub_end

        .segment "RODATA"

STUB_BASE = $6100
RES       = $7F00

; 标号在 ERAM 里的运行地址（不能整体加括号，否则会被当成间接寻址）
.define RUNAT(target) STUB_BASE + ((target) - _stub_start)

; 写入测试：读原值、写 0、再读，写前写后各存一个字节
.macro WRITE_ROM addr, slot
        lda addr
        sta RES+slot
        lda #$00
        sta addr
        lda addr
        sta RES+slot+1
.endmacro

; RAM 测试：写 $5A，读回
.macro WRITE_RAM addr, slot
        lda #$5A
        sta addr
        lda addr
        sta RES+slot
.endmacro

_stub_start:
vec_rti:
        rti                             ; NMI 与 IRQ 向量指向这里（$6100）

reset:                                  ; RESET 向量指向这里（$6101）
        sei
        cld
        ldx #$FF
        txs
        lda #$00
        sta $2000
        sta $2001
        lda #$01
        sta RES+0

        ; T1：$8000 以上的写入是否生效（预置内容为 $AA，写 0 后读回仍是 $AA 就说明没有写进存储器）
        WRITE_ROM $9100, $10
        WRITE_ROM $A100, $12
        WRITE_ROM $C100, $14
        WRITE_ROM $E100, $16

        ; T2：可写区域
        WRITE_RAM $0300, $20
        WRITE_RAM $5100, $21
        WRITE_RAM $6800, $22
        WRITE_RAM $7000, $23

        ; T3：标准 PPU：名表写后读回（读两次，第一次是缓冲）
        lda $2002
        lda #$20
        sta $2006
        lda #$00
        sta $2006
        lda #$5A
        sta $2007
        lda #$20
        sta $2006
        lda #$00
        sta $2006
        lda $2007
        lda $2007
        sta RES+$30
        ; 图案表 $0000 写 $A5 后读回
        lda #$00
        sta $2006
        sta $2006
        lda #$A5
        sta $2007
        lda #$00
        sta $2006
        sta $2006
        lda $2007
        lda $2007
        sta RES+$31

        ; T3b：DMA 把扩展 RAM 页 $E0 起的 $AA 复制到扩展显存（显存 bank 12 的开头，即 FC 模式下 PPU 的图案表 $0000），再用 $2007 读回
        lda #12
        sta $4300
        lda #$1C
        sta $4049
        lda #$00
        sta $404A
        sta $404B
        sta $404C
        lda #$80
        sta $404D
        lda #$0F                ; 长度寄存器写“字节数 - 1”：复制 16 字节
        sta $404E
        lda #$00
        sta $404F
        lda #$C0
        sta $4048
        lda #$00
        sta $2006
        sta $2006
        lda $2007
        lda $2007
        sta RES+$32

        ; T4：软驱控制器：复位后读主状态，再发读中断状态命令
        lda #$00
        sta $4101
        jsr RUNAT(delay)
        lda #$0C
        sta $4101
        jsr RUNAT(delay)
        jsr RUNAT(wait_rqm)
        sta RES+$40
        lda #$08
        sta $4205
        jsr RUNAT(wait_rqm)
        sta RES+$41
        lda $4205
        sta RES+$42
        jsr RUNAT(wait_rqm)
        sta RES+$43
        lda $4205
        sta RES+$44
        jsr RUNAT(wait_rqm)
        sta RES+$45

        lda #$FF
        sta RES+2

        ; T5：持续记录键盘等寄存器与心跳
mainloop:
        inc RES+1
        lda $4020
        sta RES+$50
        lda $4022
        sta RES+$51
        lda $4033
        sta RES+$52
        lda $4026
        sta RES+$53
        jmp RUNAT(mainloop)

; 等软驱控制器就绪（主状态位 7），超时也返回，A 为最后一次读到的主状态
wait_rqm:
        ldy #$00
        ldx #$00
@poll:  lda $4204
        bmi @done
        dex
        bne @poll
        dey
        bne @poll
@done:  rts

; 短延时
delay:
        ldx #$00
        ldy #$20
@spin:  dex
        bne @spin
        dey
        bne @spin
        rts
_stub_end:
