// ------------------------------------------------------------------------
// 名称：opcode-table.mjs
// 说明：6502 官方指令表：每个操作码的助记符、字节数与流程类别，非法操作码为 null。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：数据取自 cc65 的 da65 指令表（cc65/src/da65/opc6502.c）；流程类别由助记符推出。
// ------------------------------------------------------------------------

/** 流程类别：next 顺序执行；branch 条件分支；jump 绝对跳转；jump-indirect 间接跳转；call 子程序调用；return 返回；stop 中断指令 */
export const OPCODES = [
  ['brk', 1, 'stop'], // $00
  ['ora', 2, 'next'], // $01
  null, // $02
  null, // $03
  null, // $04
  ['ora', 2, 'next'], // $05
  ['asl', 2, 'next'], // $06
  null, // $07
  ['php', 1, 'next'], // $08
  ['ora', 2, 'next'], // $09
  ['asl', 1, 'next'], // $0a
  null, // $0b
  null, // $0c
  ['ora', 3, 'next'], // $0d
  ['asl', 3, 'next'], // $0e
  null, // $0f
  ['bpl', 2, 'branch'], // $10
  ['ora', 2, 'next'], // $11
  null, // $12
  null, // $13
  null, // $14
  ['ora', 2, 'next'], // $15
  ['asl', 2, 'next'], // $16
  null, // $17
  ['clc', 1, 'next'], // $18
  ['ora', 3, 'next'], // $19
  null, // $1a
  null, // $1b
  null, // $1c
  ['ora', 3, 'next'], // $1d
  ['asl', 3, 'next'], // $1e
  null, // $1f
  ['jsr', 3, 'call'], // $20
  ['and', 2, 'next'], // $21
  null, // $22
  null, // $23
  ['bit', 2, 'next'], // $24
  ['and', 2, 'next'], // $25
  ['rol', 2, 'next'], // $26
  null, // $27
  ['plp', 1, 'next'], // $28
  ['and', 2, 'next'], // $29
  ['rol', 1, 'next'], // $2a
  null, // $2b
  ['bit', 3, 'next'], // $2c
  ['and', 3, 'next'], // $2d
  ['rol', 3, 'next'], // $2e
  null, // $2f
  ['bmi', 2, 'branch'], // $30
  ['and', 2, 'next'], // $31
  null, // $32
  null, // $33
  null, // $34
  ['and', 2, 'next'], // $35
  ['rol', 2, 'next'], // $36
  null, // $37
  ['sec', 1, 'next'], // $38
  ['and', 3, 'next'], // $39
  null, // $3a
  null, // $3b
  null, // $3c
  ['and', 3, 'next'], // $3d
  ['rol', 3, 'next'], // $3e
  null, // $3f
  ['rti', 1, 'return'], // $40
  ['eor', 2, 'next'], // $41
  null, // $42
  null, // $43
  null, // $44
  ['eor', 2, 'next'], // $45
  ['lsr', 2, 'next'], // $46
  null, // $47
  ['pha', 1, 'next'], // $48
  ['eor', 2, 'next'], // $49
  ['lsr', 1, 'next'], // $4a
  null, // $4b
  ['jmp', 3, 'jump'], // $4c
  ['eor', 3, 'next'], // $4d
  ['lsr', 3, 'next'], // $4e
  null, // $4f
  ['bvc', 2, 'branch'], // $50
  ['eor', 2, 'next'], // $51
  null, // $52
  null, // $53
  null, // $54
  ['eor', 2, 'next'], // $55
  ['lsr', 2, 'next'], // $56
  null, // $57
  ['cli', 1, 'next'], // $58
  ['eor', 3, 'next'], // $59
  null, // $5a
  null, // $5b
  null, // $5c
  ['eor', 3, 'next'], // $5d
  ['lsr', 3, 'next'], // $5e
  null, // $5f
  ['rts', 1, 'return'], // $60
  ['adc', 2, 'next'], // $61
  null, // $62
  null, // $63
  null, // $64
  ['adc', 2, 'next'], // $65
  ['ror', 2, 'next'], // $66
  null, // $67
  ['pla', 1, 'next'], // $68
  ['adc', 2, 'next'], // $69
  ['ror', 1, 'next'], // $6a
  null, // $6b
  ['jmp', 3, 'jump-indirect'], // $6c
  ['adc', 3, 'next'], // $6d
  ['ror', 3, 'next'], // $6e
  null, // $6f
  ['bvs', 2, 'branch'], // $70
  ['adc', 2, 'next'], // $71
  null, // $72
  null, // $73
  null, // $74
  ['adc', 2, 'next'], // $75
  ['ror', 2, 'next'], // $76
  null, // $77
  ['sei', 1, 'next'], // $78
  ['adc', 3, 'next'], // $79
  null, // $7a
  null, // $7b
  null, // $7c
  ['adc', 3, 'next'], // $7d
  ['ror', 3, 'next'], // $7e
  null, // $7f
  null, // $80
  ['sta', 2, 'next'], // $81
  null, // $82
  null, // $83
  ['sty', 2, 'next'], // $84
  ['sta', 2, 'next'], // $85
  ['stx', 2, 'next'], // $86
  null, // $87
  ['dey', 1, 'next'], // $88
  null, // $89
  ['txa', 1, 'next'], // $8a
  null, // $8b
  ['sty', 3, 'next'], // $8c
  ['sta', 3, 'next'], // $8d
  ['stx', 3, 'next'], // $8e
  null, // $8f
  ['bcc', 2, 'branch'], // $90
  ['sta', 2, 'next'], // $91
  null, // $92
  null, // $93
  ['sty', 2, 'next'], // $94
  ['sta', 2, 'next'], // $95
  ['stx', 2, 'next'], // $96
  null, // $97
  ['tya', 1, 'next'], // $98
  ['sta', 3, 'next'], // $99
  ['txs', 1, 'next'], // $9a
  null, // $9b
  null, // $9c
  ['sta', 3, 'next'], // $9d
  null, // $9e
  null, // $9f
  ['ldy', 2, 'next'], // $a0
  ['lda', 2, 'next'], // $a1
  ['ldx', 2, 'next'], // $a2
  null, // $a3
  ['ldy', 2, 'next'], // $a4
  ['lda', 2, 'next'], // $a5
  ['ldx', 2, 'next'], // $a6
  null, // $a7
  ['tay', 1, 'next'], // $a8
  ['lda', 2, 'next'], // $a9
  ['tax', 1, 'next'], // $aa
  null, // $ab
  ['ldy', 3, 'next'], // $ac
  ['lda', 3, 'next'], // $ad
  ['ldx', 3, 'next'], // $ae
  null, // $af
  ['bcs', 2, 'branch'], // $b0
  ['lda', 2, 'next'], // $b1
  null, // $b2
  null, // $b3
  ['ldy', 2, 'next'], // $b4
  ['lda', 2, 'next'], // $b5
  ['ldx', 2, 'next'], // $b6
  null, // $b7
  ['clv', 1, 'next'], // $b8
  ['lda', 3, 'next'], // $b9
  ['tsx', 1, 'next'], // $ba
  null, // $bb
  ['ldy', 3, 'next'], // $bc
  ['lda', 3, 'next'], // $bd
  ['ldx', 3, 'next'], // $be
  null, // $bf
  ['cpy', 2, 'next'], // $c0
  ['cmp', 2, 'next'], // $c1
  null, // $c2
  null, // $c3
  ['cpy', 2, 'next'], // $c4
  ['cmp', 2, 'next'], // $c5
  ['dec', 2, 'next'], // $c6
  null, // $c7
  ['iny', 1, 'next'], // $c8
  ['cmp', 2, 'next'], // $c9
  ['dex', 1, 'next'], // $ca
  null, // $cb
  ['cpy', 3, 'next'], // $cc
  ['cmp', 3, 'next'], // $cd
  ['dec', 3, 'next'], // $ce
  null, // $cf
  ['bne', 2, 'branch'], // $d0
  ['cmp', 2, 'next'], // $d1
  null, // $d2
  null, // $d3
  null, // $d4
  ['cmp', 2, 'next'], // $d5
  ['dec', 2, 'next'], // $d6
  null, // $d7
  ['cld', 1, 'next'], // $d8
  ['cmp', 3, 'next'], // $d9
  null, // $da
  null, // $db
  null, // $dc
  ['cmp', 3, 'next'], // $dd
  ['dec', 3, 'next'], // $de
  null, // $df
  ['cpx', 2, 'next'], // $e0
  ['sbc', 2, 'next'], // $e1
  null, // $e2
  null, // $e3
  ['cpx', 2, 'next'], // $e4
  ['sbc', 2, 'next'], // $e5
  ['inc', 2, 'next'], // $e6
  null, // $e7
  ['inx', 1, 'next'], // $e8
  ['sbc', 2, 'next'], // $e9
  ['nop', 1, 'next'], // $ea
  null, // $eb
  ['cpx', 3, 'next'], // $ec
  ['sbc', 3, 'next'], // $ed
  ['inc', 3, 'next'], // $ee
  null, // $ef
  ['beq', 2, 'branch'], // $f0
  ['sbc', 2, 'next'], // $f1
  null, // $f2
  null, // $f3
  null, // $f4
  ['sbc', 2, 'next'], // $f5
  ['inc', 2, 'next'], // $f6
  null, // $f7
  ['sed', 1, 'next'], // $f8
  ['sbc', 3, 'next'], // $f9
  null, // $fa
  null, // $fb
  null, // $fc
  ['sbc', 3, 'next'], // $fd
  ['inc', 3, 'next'], // $fe
  null, // $ff
];
