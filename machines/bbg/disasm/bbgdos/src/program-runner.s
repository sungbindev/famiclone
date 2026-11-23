; 模块：program-runner
; 程序运行：DosRunProgram、DosRunPathProgram 与程序类型判断

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_3574"
; 段 0，$B174-$B3B5
; 判断 0 结尾路径 X:Y（调用者页，X 高）的扩展名：与 program_ext_table 里的 CMD、EXC、GAM 比较（不分大小写）。出口：C=0 时 X 为类型（3 为 CMD，6 为 EXC，9 为 GAM），transfer_command 为扩展名首字符之后的下标；C=1 为不认识的扩展名。
program_type_from_name:
  stx far_ptr_high
  sty far_ptr_low
  ldy #$00
type_length_loop:
  jsr read_far_byte
  tax
  beq type_find_dot
  iny
  bne type_length_loop
type_find_dot:
  dey
  bmi type_not_found
  jsr read_far_byte
  cmp #$5C
  beq type_not_found
  cmp #$2E
  bne type_find_dot
  iny
  sty transfer_command
  ldx #$00
type_ext_loop:
  lda #$03
  sta fat_access_mode
  ldy transfer_command
type_compare_loop:
  jsr read_far_byte
  and #$DF
  cmp program_ext_table,x
  bne type_next_ext
  iny
  inx
  dec fat_access_mode
  bne type_compare_loop
  clc
  rts
type_next_ext:
  inx
  dec fat_access_mode
  bne type_next_ext
  cpx #$09
  bne type_ext_loop
type_not_found:
  sec
  rts

; DOS 功能 15 DosRunPathProgram：运行一个程序，程序名可以带路径。入口：$FF 为程序所在盘号；X:Y（X 高）为以 0 结尾的路径与文件名，文件名必须带扩展名 CMD、EXC 或 GAM；$0100 起是传给程序的参数。出口同功能 5。嵌套最多 4 层。Ember 实测。
dos_run_path_program:
  lda far_ptr_high
  sta fb_drive
  lda current_page
  sta fb_page
  jmp run_program_common

; DOS 功能 5 DosRunProgram：运行一个程序（CMD、EXC、GAM）。入口：$FF 为程序所在盘号（0 默认，1 为 A 盘）；X:Y（X 高）指向 11 字节文件名（主名 8 加扩展名 3，空格补齐），经 sub_B13F 转成“主名.扩展名”放在 $0430 再继续；$0100 起是传给程序的参数。程序类型由扩展名判断（表在 $B136：CMD、EXC、GAM），扩展名不是这三种时返回 C=1（Ember 实测 Y=$0B）。出口：C=0 时 A 为程序的退出码。嵌套最多 4 层；返回后寄存器内容被破坏。Ember 实测。
dos_run_program:
  lda far_ptr_high
  sta fb_drive
  stx far_ptr_high
  sty far_ptr_low
  jsr run_name_format
run_program_common:
  stx fb_buffer_high
  sty fb_buffer_low
  jsr program_type_from_name
  ldy #$0B
  bcs run_done
  cpx #$03
  bne run_check_depth
  ldy transfer_command
  dey
  ldx #$07
run_check_command_cmd:
  dey
  jsr read_far_byte
  and #$DF
  cmp command_cmd_path,x
  bne run_set_cmd_type
  dex
  bne run_check_command_cmd
  lda #$FF
  sta fb_drive
  lda dos_home_page
  ldx #$B1
  ldy #$29
  sta fb_page
  stx fb_buffer_high
  sty fb_buffer_low
run_set_cmd_type:
  ldx #$03
run_check_depth:
  stx far_ptr_high
  lda run_stack_top
  cmp #$28
  ldy #$2A
  bcs run_done
  tay
  lda #$04
  sta far_ptr_low
  tsx
run_push_loop:
  lda stack_slot_1,x
  inx
  sta run_stack,y
  iny
  dec far_ptr_low
  bne run_push_loop
  lda current_page
  sta run_stack,y
  iny
  sty run_stack_top
  lda #$14
  jsr file_call_function
  bcs run_open_failed
  lda far_ptr_high
  cmp #$03
  bne run_load_exe
  jsr cmd_load_and_run
  jmp run_pop_frame
run_load_exe:
  jsr exe_load_and_run
  jmp run_pop_frame
run_open_failed:
  lda #$02
run_pop_frame:
  tay
  lda run_stack_top
  sec
  sbc #$05
  sta run_stack_top
  sec
run_done:
  rts

; Ctrl-C 的默认处理（CTRL_C_ENTRY 的目标）：显示 ^C 并换行，然后以返回码 $FF、Y=0 结束当前程序并回到上一层程序（abort_program）。
ctrl_c_default_handler:
  ldx #$5E
  jsr console_put_char
  ldx #$43
  jsr console_put_char
  ldx #$0D
  jsr console_put_char
  ldx #$0A
  jsr console_put_char
abort_program:
  ldx #$FF
  stx result_code
  inx
  stx result_flags
  jmp program_return_pop
; “DEVICE must CMD or SYS extend”，以 $ 结束
msg_device_extension:
  .byte $44, $45, $56, $49, $43, $45, $20, $6D, $75, $73, $74, $20, $43, $4D, $44, $20
  .byte $6F, $72, $20, $53, $59, $53, $20, $65, $78, $74, $65, $6E, $64, $24
; “Bad or missing ”，以 $ 结束，后接关键字或文件名
msg_bad_or_missing:
  .byte $0A, $42, $61, $64, $20, $6F, $72, $20, $6D, $69, $73, $73, $69, $6E, $67, $20
  .byte $24
config_keyword_table:
  .byte $44
config_keyword_low:
  .byte $45
config_keyword_high:
  .byte $56, $49, $43, $45, $00, $20, $B4, $53, $48, $45, $4C, $4C, $00, $3F, $B4, $00
config_buffer_count:
  .byte $00
config_read_index:
  .byte $00
config_line_number:
  .byte $30, $30, $30, $31
; “Error in CONFIG.SYS line ”，后接 config_error_digits 的行号
msg_config_error:
  .byte $45, $72, $72, $6F, $72, $20, $69, $6E, $20, $43, $4F, $4E, $46, $49, $47, $2E
  .byte $53, $59, $53, $20, $6C, $69, $6E, $65, $20
config_error_digits:
  .byte $58, $58, $58, $58, $0D, $0A, $24
config_value_pos:
  .byte $00
config_buffer:
  .res 128, $00

; CONFIG.SYS 解析：若 text_line_buffer 从 Y 起是“X:”形式的盘符，A 返回盘号（A 盘为 1）并把 Y 前进 2；否则 A=0。
config_parse_drive:
  lda text_line_buffer,y
  cmp #$0D
  beq config_no_drive
  tax
  lda text_line_second,y
  cmp #$3A
  bne config_no_drive
  txa

; config_parse_drive 的后半：盘符字母（A）减 $40 得盘号，Y 前进 2，C=0。
config_drive_number:
  sbc #$40
  iny
  iny
  clc
  rts
config_no_drive:
  lda #$00

; 清 C 返回（config_parse_drive 没有盘符时 A=0 的出口）。
config_drive_done:
  clc
  rts

; CONFIG.SYS 解析：判断 A 是否为分隔符（回车、空格、制表符），是则 Z=1。
; CONFIG.SYS 解析：判断 A 是否为分隔符（回车、空格、制表符），是则 Z=1
config_is_separator:
  cmp #$0D
  beq config_is_separator_return
  cmp #$20
  beq config_is_separator_return
  cmp #$09

; config_is_separator 的返回指令（Z 已按比较结果设好）。
config_is_separator_return:
  rts

; CONFIG.SYS 解析：把等号后的值拆成程序名（放 config_program_name，$6182 起，0 结尾）与参数（跳过程序名后剩下的部分直到回车，放 command_tail 起，开头字节为长度）。出口：X:Y 为程序名地址（$61:$82）。
; CONFIG.SYS 解析：把等号后的值拆成程序名（放 $6182）与参数（放 $0100 起）
config_split_value:
  ldx #$00
split_name_loop:
  lda text_line_buffer,y
  jsr config_is_separator
  beq split_name_end
  sta config_program_name,x
  inx
  iny
  bne split_name_loop
split_name_end:
  lda #$00
  sta config_program_name,x
  ldx #$00
split_args_loop:
  lda text_line_buffer,y
  sta stack_slot_1,x
  iny
  inx
  cmp #$0D
  bne split_args_loop
  dex
  stx command_tail
  ldx #$61
  ldy #$82
  rts
