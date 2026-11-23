; 模块：critical-error
; 严重错误处理：磁盘错误提示与 Abort、Ignore、Retry、Fail 选择

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_1C00"
; 段 0，$9800-$9D76
  .byte $0D, $0A, $49, $6E, $73, $65, $72, $74, $20, $64, $69, $73, $6B, $65, $74, $74
  .byte $65, $20, $66, $6F, $72, $20, $64, $72, $69, $76, $65, $20
data_981C:
  .byte $3F, $3A, $0D, $0A, $61, $6E, $64, $20, $70, $72, $65, $73, $73, $20, $61, $6E
  .byte $79, $20, $6B, $65, $79, $20, $77, $68, $65, $6E, $20, $72, $65, $61, $64, $79
  .byte $0D, $0A, $0A, $24, $0D, $0A, $B0, $D1
data_9844:
  .byte $3F, $C5, $CC, $B2, $E5, $C8, $EB, $C7, $FD, $B6, $AF, $C6, $F7, $D6, $D0, $2C
  .byte $B0, $B4, $C8, $CE, $D2, $E2, $BC, $FC, $C8, $B7, $C8, $CF, $0D, $0A, $24

; 严重错误：显示“Insert diskette for drive ?: and press any key when ready”（中文方式显示中文提示），盘符由 Y 加 $41 填入消息，然后等待用户更换磁盘：用 BIOS 功能 24（外设调用，子功能 1）查询磁盘是否已更换，直到子功能返回的 Y 为 0。
critical_disk_prompt:
  tya
  clc
  adc #$41
  sta data_981C
  sta data_9844
  ldx #$98
  ldy #$00
  lda data_A544
  beq loc_987A
  ldx #$98
  ldy #$40
loc_987A:
  lda dos_page
  sta data_A53D
  jsr dos_display_string
loc_9883:
  lda #$18
  ldx #$01
  jsr BIOS_ENTRY
  tya
  beq loc_9897
  lda #$18
  ldx #$00
  jsr BIOS_ENTRY
  jmp loc_9883
loc_9897:
  lda #$18
  ldx #$00
  jmp BIOS_ENTRY

; 文件调用 0 FileReset：复位驱动器。清除当前驱动器保存的状态字节（$9C83 起的状态表），若它是已选定的驱动器（mounted_drive）则标成未选定，再重新读取驱动器信息（drive_info_read）。Ember 实测返回成功。
file_call_reset:
  jsr drive_state_offset
  lda #$00
  sta data_9C83,x
  lda current_drive
  cmp mounted_drive
  bne drive_info_read
  lda #$FF
  sta mounted_drive

; 读驱动器信息：调用 BIOS 功能 20（BiosGetDriveMessage），结果存入 drive_info_a（A 低 2 位）、drive_info_x、drive_info_y（Y 低 7 位），并由 X 低 2 位按位计数算出 logical_drive_count（至少 2）。出口：A=0。
drive_info_read:
  lda #$14
  jsr BIOS_ENTRY
  and #$03
  sta drive_info_a
  stx drive_info_x
  tya
  and #$7F
  sta drive_info_y
  txa
  and #$03
  ldy #$02
loc_98CB:
  lsr a
  bcc loc_98CF
  iny
loc_98CF:
  bne loc_98CB
  lda drive_info_y
  beq loc_98D7
  iny
loc_98D7:
  sty logical_drive_count
  lda #$00
  rts
data_98DD:
  .byte $CC, $80, $40, $10, $08, $06, $04, $03
data_98E5:
  .byte $0A, $02, $06, $04, $04, $02, $08, $00, $0C

; 把 BIOS 磁盘错误码（A）经 data_98DD 表换成严重错误的原因编号（data_98E5 表），然后调用严重错误入口 CRITICAL_ERROR_ENTRY（$5818）；Y 是读写标志与驱动器相关的参数（未确认）。
critical_error_report:
  ldx disk_command_temp
  cpx #$03
  bne loc_98F6
  iny
loc_98F6:
  ldx #$00
loc_98F8:
  cmp data_98DD,x
  beq loc_9902
  inx
  cpx #$08
  bne loc_98F8
loc_9902:
  lda data_98E5,x
  tax
  jmp CRITICAL_ERROR_ENTRY
critical_messages_english:
  .byte $57, $72, $69, $74, $65, $20, $70, $72, $6F, $74, $65, $63, $74, $00, $49, $6E
  .byte $76, $61, $6C, $69, $64, $20, $75, $6E, $69, $74, $00, $4E, $6F, $74, $20, $72
  .byte $65, $61, $64, $79, $00, $49, $6E, $76, $61, $6C, $69, $64, $20, $64, $65, $76
  .byte $69, $63, $65, $00, $44, $61, $74, $61, $20, $65, $72, $72, $6F, $72, $00, $49
  .byte $6E, $76, $61, $6C, $69, $64, $20, $70, $61, $72, $61, $6D, $65, $74, $65, $72
  .byte $73, $00, $53, $65, $65, $6B, $20, $65, $72, $72, $6F, $72, $00, $49, $6E, $76
  .byte $61, $6C, $69, $64, $20, $6D, $65, $64, $69, $61, $20, $74, $79, $70, $65, $00
  .byte $53, $65, $63, $74, $6F, $72, $20, $6E, $6F, $74, $20, $66, $6F, $75, $6E, $64
  .byte $00, $50, $72, $69, $6E, $74, $65, $72, $20, $6F, $75, $74, $20, $6F, $66, $20
  .byte $70, $61, $70, $65, $72, $00, $57, $72, $69, $74, $65, $20, $66, $61, $75, $6C
  .byte $74, $20, $65, $72, $72, $6F, $72, $00, $52, $65, $61, $64, $20, $66, $61, $75
  .byte $6C, $74, $20, $65, $72, $72, $6F, $72, $00, $47, $65, $6E, $65, $72, $61, $6C
  .byte $20, $66, $61, $69, $6C, $75, $72, $65, $00, $20, $72, $65, $61, $64, $00, $20
  .byte $77, $72, $69, $74, $65, $00, $20, $64, $72, $69, $76, $65, $20, $00, $0D, $0A
  .byte $41, $62, $6F, $72, $74, $00, $2C, $20, $49, $67, $6E, $6F, $72, $65, $00, $2C
  .byte $20, $52, $65, $74, $72, $79, $00, $2C, $20, $46, $61, $69, $6C, $00
data_9A07:
  .byte $00, $0E, $1B, $25, $34, $3F, $52, $5D, $70, $81, $96, $A8, $B9
critical_messages_chinese:
  .byte $D0, $B4, $B1, $A3, $BB, $A4, $00, $CE, $B4, $D6, $AA, $B5, $A5, $D4, $AA, $00
  .byte $CE, $B4, $BE, $CD, $D0, $F7, $00, $CE, $DE, $D0, $A7, $C9, $E8, $B1, $B8, $00
  .byte $CA, $FD, $BE, $DD, $B3, $F6, $B4, $ED, $00, $B2, $CE, $CA, $FD, $CE, $DE, $D0
  .byte $A7, $00, $B2, $E9, $D5, $D2, $B3, $F6, $B4, $ED, $00, $B4, $ED, $CE, $F3, $BD
  .byte $E9, $D6, $CA, $C0, $E0, $D0, $CD, $00, $CE, $B4, $D5, $D2, $B5, $BD, $C9, $C8
  .byte $C7, $F8, $00, $B4, $F2, $D3, $A1, $BB, $FA, $C8, $B1, $D6, $BD, $00, $D0, $B4
  .byte $B4, $ED, $CE, $F3, $00, $B6, $C1, $B4, $ED, $CE, $F3, $00, $D2, $BB, $B0, $E3
  .byte $B4, $ED, $CE, $F3, $00, $B6, $C1, $00, $D0, $B4, $00, $B4, $C5, $C5, $CC, $20
  .byte $00, $0D, $0A, $CD, $CB, $B3, $F6, $41, $00, $2C, $BA, $F6, $C2, $D4, $49, $00
  .byte $2C, $D6, $D8, $CA, $D4, $52, $00, $2C, $CA, $A7, $B0, $DC, $46, $00, $20, $CA
  .byte $B1, $B7, $A2, $CF, $D6, $00, $0D, $0A, $00
data_9ABD:
  .byte $00, $07, $10, $17, $20, $29, $32, $3B, $48, $53, $5E, $65, $6C
data_9ACA:
  .byte $00

; 显示中文消息：从 critical_messages_chinese 的偏移 Y 起逐字节显示，到 0 为止。
critical_print_chinese:
  ldx critical_messages_chinese,y
  beq loc_9ADA
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne critical_print_chinese
loc_9ADA:
  rts

; 显示英文消息：从 critical_messages_english 的偏移 Y 起逐字节显示，到 0 为止。
critical_print_english:
  ldx critical_messages_english,y
  beq loc_9AEA
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne critical_print_english
loc_9AEA:
  rts

; 显示中文方式下的前缀消息（中文表偏移 $A6，即 CRLF），保留 Y。
critical_print_reason:
  tya
  pha
  ldy #$A6
  jsr critical_print_chinese
  pla
  tay
  rts

; 询问用户怎么处理：显示“?”，等待按键（大写化）：A（Abort）返回 Y=2，I（Ignore）Y=0，R（Retry）Y=1，F（Fail）Y=3；只接受消息里列出的选项，其他按键 C=1 要求重新输入。
critical_ask_action:
  ldx #$3F
  jsr dos_display_char
  lda #$01
  jsr sub_A74E
  and #$DF
  tax
  ldy #$02
  cpx #$41
  beq loc_9B24
  ldy #$00
  lda #$20
  cpx #$49
  beq loc_9B1D
  iny
  lsr a
  cpx #$52
  beq loc_9B1D
  ldy #$03
  lsr a
  cpx #$46
  bne loc_9B22
loc_9B1D:
  and data_9ACA
  bne loc_9B24
loc_9B22:
  sec
  rts
loc_9B24:
  jsr critical_print_reason
  clc
  rts

; 严重错误处理（CRITICAL_ERROR_ENTRY 的目标，启动时装入）。入口：X 为原因编号，Y 为读写与可用选项标志。显示“原因 read/write error on drive X”与“Abort, Ignore, Retry, Fail”中允许的选项，等待用户选择。出口：Y 为选择结果（见 critical_ask_action）。显示方式为中文时用中文消息表。
critical_error_handler:
  sty data_9ACA
  cpx #$0D
  bcc loc_9B32
  ldx #$0C
loc_9B32:
  lda data_A544
  bpl loc_9B96
  ldy data_9ABD,x
  tya
  pha
  jsr critical_print_reason
  lda data_9ACA
  ldy #$75
  lsr a
  bcc loc_9B49
  ldy #$78
loc_9B49:
  jsr critical_print_chinese
  lda data_9ACA
  bpl loc_9B51
loc_9B51:
  ldy #$7B
  jsr critical_print_chinese
  ldy #$01
  lda (var_00EE),y
  clc
  adc #$40
  tax
  jsr dos_display_char
  ldy #$9E
  jsr critical_print_chinese
  pla
  tay
  jsr critical_print_chinese
loc_9B6B:
  ldy #$81
  jsr critical_print_chinese
  lda data_9ACA
  asl a
  asl a
  bpl loc_9B7E
  pha
  ldy #$89
  jsr critical_print_chinese
  pla
loc_9B7E:
  asl a
  bpl loc_9B88
  pha
  ldy #$90
  jsr critical_print_chinese
  pla
loc_9B88:
  asl a
  bpl loc_9B90
  ldy #$97
  jsr critical_print_chinese
loc_9B90:
  jsr critical_ask_action
  bcs loc_9B6B
  rts
loc_9B96:
  sty data_9ACA
  cpx #$0D
  bcc loc_9B9F
  ldx #$0C
loc_9B9F:
  ldy data_9A07,x
  jsr critical_print_reason
  jsr critical_print_english
  lda data_9ACA
  ldy #$C9
  lsr a
  bcc loc_9BB2
  ldy #$CF
loc_9BB2:
  jsr critical_print_english
  lda data_9ACA
  bpl loc_9BBA
loc_9BBA:
  ldy #$D6
  jsr critical_print_english
  ldy #$01
  lda (var_00EE),y
  clc
  adc #$40
  tax
  jsr dos_display_char
loc_9BCA:
  ldy #$DE
  jsr critical_print_english
  lda data_9ACA
  asl a
  asl a
  bpl loc_9BDD
  pha
  ldy #$E6
  jsr critical_print_english
  pla
loc_9BDD:
  asl a
  bpl loc_9BE7
  pha
  ldy #$EF
  jsr critical_print_english
  pla
loc_9BE7:
  asl a
  bpl loc_9BEF
  ldy #$F7
  jsr critical_print_english
loc_9BEF:
  jsr critical_ask_action
  bcs loc_9BCA
  rts
data_9BF5:
  .res 62, $00
data_9C33:
  .res 80, $00
data_9C83:
  .res 244, $00
