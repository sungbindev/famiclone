; 模块：critical-error
; 严重错误处理：磁盘错误提示与 Abort、Ignore、Retry、Fail 选择

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_1C00"
; 段 0，$9800-$9D76
; 英文提示“Insert diskette for drive ”，后接盘符与后半句
msg_insert_english:
  .byte $0D, $0A, $49, $6E, $73, $65, $72, $74, $20, $64, $69, $73, $6B, $65, $74, $74
  .byte $65, $20, $66, $6F, $72, $20, $64, $72, $69, $76, $65, $20
prompt_drive_letter_english:
  .byte $3F, $3A, $0D, $0A, $61, $6E, $64, $20, $70, $72, $65, $73, $73, $20, $61, $6E
  .byte $79, $20, $6B, $65, $79, $20, $77, $68, $65, $6E, $20, $72, $65, $61, $64, $79
  .byte $0D, $0A, $0A, $24
; 中文方式下的换盘提示（GB2312），后接盘符
msg_insert_chinese:
  .byte $0D, $0A, $B0, $D1
prompt_drive_letter_chinese:
  .byte $3F, $C5, $CC, $B2, $E5, $C8, $EB, $C7, $FD, $B6, $AF, $C6, $F7, $D6, $D0, $2C
  .byte $B0, $B4, $C8, $CE, $D2, $E2, $BC, $FC, $C8, $B7, $C8, $CF, $0D, $0A, $24

; 严重错误：显示“Insert diskette for drive ?: and press any key when ready”（中文方式显示中文提示），盘符由 Y 加 $41 填入消息，然后等待用户更换磁盘：用 BIOS 功能 24（外设调用，子功能 1）查询磁盘是否已更换，直到子功能返回的 Y 为 0。
critical_disk_prompt:
  tya
  clc
  adc #$41
  sta prompt_drive_letter_english
  sta prompt_drive_letter_chinese
  ldx #$98
  ldy #$00
  lda display_mode
  beq prompt_show
  ldx #$98
  ldy #$40
prompt_show:
  lda dos_page
  sta current_page
  jsr dos_display_string
prompt_wait_loop:
  lda #$18
  ldx #$01
  jsr BIOS_ENTRY
  tya
  beq prompt_finish
  lda #$18
  ldx #$00
  jsr BIOS_ENTRY
  jmp prompt_wait_loop
prompt_finish:
  lda #$18
  ldx #$00
  jmp BIOS_ENTRY

; 文件调用 0 FileReset：复位驱动器。清除当前驱动器保存的状态字节（$9C83 起的状态表），若它是已选定的驱动器（mounted_drive）则标成未选定，再重新读取驱动器信息（drive_info_read）。Ember 实测返回成功。
file_call_reset:
  jsr drive_state_offset
  lda #$00
  sta drive_state_marker,x
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
drive_count_loop:
  lsr a
  bcc drive_count_next
  iny
drive_count_next:
  bne drive_count_loop
  lda drive_info_y
  beq drive_count_store
  iny
drive_count_store:
  sty logical_drive_count
  lda #$00
  rts
disk_error_code_table:
  .byte $CC, $80, $40, $10, $08, $06, $04, $03
critical_reason_table:
  .byte $0A, $02, $06, $04, $04, $02, $08, $00, $0C

; 把 BIOS 磁盘错误码（A）经 data_98DD 表换成严重错误的原因编号（data_98E5 表），然后调用严重错误入口 CRITICAL_ERROR_ENTRY（$5818）；Y 是读写标志与驱动器相关的参数（未确认）。
critical_error_report:
  ldx disk_command_temp
  cpx #$03
  bne report_find_code
  iny
report_find_code:
  ldx #$00
report_search_loop:
  cmp disk_error_code_table,x
  beq report_call_handler
  inx
  cpx #$08
  bne report_search_loop
report_call_handler:
  lda critical_reason_table,x
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
english_message_offsets:
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
chinese_message_offsets:
  .byte $00, $07, $10, $17, $20, $29, $32, $3B, $48, $53, $5E, $65, $6C
critical_flags:
  .byte $00

; 显示中文消息：从 critical_messages_chinese 的偏移 Y 起逐字节显示，到 0 为止。
critical_print_chinese:
  ldx critical_messages_chinese,y
  beq print_chinese_done
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne critical_print_chinese
print_chinese_done:
  rts

; 显示英文消息：从 critical_messages_english 的偏移 Y 起逐字节显示，到 0 为止。
critical_print_english:
  ldx critical_messages_english,y
  beq print_english_done
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne critical_print_english
print_english_done:
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
  jsr kb_clear_and_call
  and #$DF
  tax
  ldy #$02
  cpx #$41
  beq ask_accepted
  ldy #$00
  lda #$20
  cpx #$49
  beq ask_check_option
  iny
  lsr a
  cpx #$52
  beq ask_check_option
  ldy #$03
  lsr a
  cpx #$46
  bne ask_invalid
ask_check_option:
  and critical_flags
  bne ask_accepted
ask_invalid:
  sec
  rts
ask_accepted:
  jsr critical_print_reason
  clc
  rts

; 严重错误处理（CRITICAL_ERROR_ENTRY 的目标，启动时装入）。入口：X 为原因编号，Y 为读写与可用选项标志。显示“原因 read/write error on drive X”与“Abort, Ignore, Retry, Fail”中允许的选项，等待用户选择。出口：Y 为选择结果（见 critical_ask_action）。显示方式为中文时用中文消息表。
critical_error_handler:
  sty critical_flags
  cpx #$0D
  bcc handler_clamp_reason
  ldx #$0C
handler_clamp_reason:
  lda display_mode
  bpl handler_english
  ldy chinese_message_offsets,x
  tya
  pha
  jsr critical_print_reason
  lda critical_flags
  ldy #$75
  lsr a
  bcc handler_chinese_rw
  ldy #$78
handler_chinese_rw:
  jsr critical_print_chinese
  lda critical_flags
  bpl handler_chinese_drive
handler_chinese_drive:
  ldy #$7B
  jsr critical_print_chinese
  ldy #$01
  lda (block_ptr_low),y
  clc
  adc #$40
  tax
  jsr dos_display_char
  ldy #$9E
  jsr critical_print_chinese
  pla
  tay
  jsr critical_print_chinese
handler_chinese_options:
  ldy #$81
  jsr critical_print_chinese
  lda critical_flags
  asl a
  asl a
  bpl handler_chinese_ignore
  pha
  ldy #$89
  jsr critical_print_chinese
  pla
handler_chinese_ignore:
  asl a
  bpl handler_chinese_retry
  pha
  ldy #$90
  jsr critical_print_chinese
  pla
handler_chinese_retry:
  asl a
  bpl handler_chinese_ask
  ldy #$97
  jsr critical_print_chinese
handler_chinese_ask:
  jsr critical_ask_action
  bcs handler_chinese_options
  rts
handler_english:
  sty critical_flags
  cpx #$0D
  bcc handler_english_clamp
  ldx #$0C
handler_english_clamp:
  ldy english_message_offsets,x
  jsr critical_print_reason
  jsr critical_print_english
  lda critical_flags
  ldy #$C9
  lsr a
  bcc handler_english_rw
  ldy #$CF
handler_english_rw:
  jsr critical_print_english
  lda critical_flags
  bpl handler_english_drive
handler_english_drive:
  ldy #$D6
  jsr critical_print_english
  ldy #$01
  lda (block_ptr_low),y
  clc
  adc #$40
  tax
  jsr dos_display_char
handler_english_options:
  ldy #$DE
  jsr critical_print_english
  lda critical_flags
  asl a
  asl a
  bpl handler_english_ignore
  pha
  ldy #$E6
  jsr critical_print_english
  pla
handler_english_ignore:
  asl a
  bpl handler_english_retry
  pha
  ldy #$EF
  jsr critical_print_english
  pla
handler_english_retry:
  asl a
  bpl handler_english_ask
  ldy #$F7
  jsr critical_print_english
handler_english_ask:
  jsr critical_ask_action
  bcs handler_english_options
  rts
work_dir_path:
  .res 62, $00
drive_state_table:
  .res 80, $00
drive_state_marker:
  .res 244, $00
