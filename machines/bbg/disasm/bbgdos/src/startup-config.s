; 模块：startup-config
; 启动流程：解析 CONFIG.SYS、装入 DEVICE 与 SHELL、启动命令解释器与错误提示

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_37B6"
; 段 0，$B3B6-$B5E5
; 显示以 $ 结尾的字符串（地址 X:Y，X 高，当前页），逐字符调用 startup_put_char。
startup_print_string:
  stx transfer_command
  sty fat_access_mode
  lda #$00
print_string_loop:
  tay
  lda (fat_access_mode),y
  cmp #$24
  beq print_string_done
  tax
  iny
  tya
  pha
  jsr startup_put_char
  pla
  bne print_string_loop
print_string_done:
  rts

; 显示字符串后换行：先 startup_print_string，再落入 startup_print_crlf。
startup_print_string_crlf:
  jsr startup_print_string

; 输出回车换行（先 CR，再落入 startup_put_char 输出 LF）。
startup_print_crlf:
  ldx #$0D
  lda #$0A
  clc
  pha
  jsr startup_put_char
  pla
  tax

; 输出字符 X：调用 BIOS 功能 $1B（Y=0）。
startup_put_char:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

; 把 A 的高、低 4 位各转成一个字符（加 $30，只适用于 0-9）写入 config_error_digits 从 X 起的两个位置，X 前进 2。
store_digit_pair:
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  jsr store_digit
  pla
  and #$0F

; 把 A 加 $30 存入 config_error_digits[X]，X 加 1。
store_digit:
  clc
  adc #$30
  sta config_error_digits,x
  inx
  rts
config_report_error_line:
  ldx #$03
config_copy_digits_loop:
  lda config_line_number,x
  sta config_error_digits,x
  dex
  bpl config_copy_digits_loop
  ldx #$B2
  ldy #$C3
  jmp startup_print_string_crlf

; 运行 DEVICE 或 SHELL 指定的程序：盘号取 fb_drive，程序名在 config_program_name（$6182），调用 dos_run_path_program；失败时先显示“Error in CONFIG.SYS line 行号”，再显示“Bad or missing ”。
config_run_program:
  lda fb_drive
  sta far_ptr_high
  ldx #$61
  ldy #$82
  jsr dos_run_path_program
  bcc config_run_done
  jsr config_report_error_line
  ldx #$B2
  ldy #$9A
  jmp startup_print_string_crlf
config_run_done:
  rts

; CONFIG.SYS 的 DEVICE 命令：解析盘符与程序名；扩展名必须是 CMD（program_type_from_name 返回 X=3）才运行（config_run_program），否则显示“DEVICE must CMD or SYS extend”并报告行号。
; CONFIG.SYS 的 DEVICE 命令处理例程
config_handler_device:
  jsr config_parse_drive
  sta fb_drive
  jsr config_split_value
  jsr program_type_from_name
  bcs device_bad_extension
  cpx #$03
  bne device_bad_extension
  jmp config_run_program
device_bad_extension:
  ldx #$B2
  ldy #$7C
  jsr startup_print_string_crlf
  jmp config_report_error_line

; CONFIG.SYS 的 SHELL 命令：解析盘符与程序名后直接运行（config_run_program），不检查扩展名。
; CONFIG.SYS 的 SHELL 命令处理例程
config_handler_shell:
  jsr config_parse_drive
  sta fb_drive
  jsr config_split_value
  jmp config_run_program

; 跳过 text_line_buffer 里从 Y 起的空格与制表符；A 为第一个其他字符，Y 指向它。
config_skip_blanks:
  lda text_line_buffer,y
  cmp #$20
  beq skip_blank_next
  cmp #$09
  bne skip_blank_done
skip_blank_next:
  iny
  bne config_skip_blanks
skip_blank_done:
  rts

; 解析一行 CONFIG.SYS（在 text_line_buffer）：跳过空白，空行直接返回；逐个与 config_keyword_table 里的关键字比较（不分大小写），后面必须有等号，匹配后跳过空白并把值的起始下标存入 config_value_pos，再压入该关键字的处理例程地址并 RTS 进入；没有匹配的关键字时报告行号错误。
config_parse_line:
  ldy #$00
  jsr config_skip_blanks
  cmp #$0D
  beq config_line_done
  ldx #$00
config_match_keyword:
  tya
  pha
config_match_char_loop:
  lda text_line_buffer,y
  and #$DF
  cmp config_keyword_table,x
  bne config_skip_keyword
  iny
  inx
  lda config_keyword_table,x
  bne config_match_char_loop
  jsr config_skip_blanks
  cmp #$3D
  bne config_next_keyword
  iny
  pla
  jsr config_skip_blanks
  sty config_value_pos
  lda config_keyword_high,x
  pha
  lda config_keyword_low,x
  pha
config_line_done:
  rts
config_skip_keyword:
  inx
  lda config_keyword_table,x
  bne config_skip_keyword
config_next_keyword:
  pla
  tay
  inx
  inx
  inx
  lda config_keyword_table,x
  bne config_match_keyword
  jmp config_report_error_line

; 设置 $0400 文件调用块的缓冲区地址 X:Y（fb_buffer_high:fb_buffer_low），然后调用 file_call_function。
file_call_with_buffer:
  stx fb_buffer_high
  sty fb_buffer_low

; 以 A 为功能号调用 FILE_ENTRY，通道块在 $0400（X=4，Y=0）。
file_call_function:
  sta fb_function
  ldx #$04
  ldy #$00
  jmp FILE_ENTRY

; 保证 config_buffer 里还有未处理的数据：X（读取下标）达到 config_buffer_count 时，用文件调用 2 再读最多 128 字节到 config_buffer，X 重置为 0；读到 0 字节（文件结束）或出错时 C=1。Y 保持。
config_ensure_buffer:
  cpx config_buffer_count
  bcc ensure_return
  tya
  pha
  lda dos_home_page
  sta fb_page
  ldx #$00
  ldy #$80
  stx fb_bytes_high
  sty fb_bytes_low
  lda #$02
  ldx #$B2
  ldy #$E4
  jsr file_call_with_buffer
  bcs ensure_done
  lda fb_bytes_low
  sta config_buffer_count
  bne ensure_done
  sec
ensure_done:
  pla
  tay
  ldx #$00
ensure_return:
  rts
; 启动失败提示：缺少命令解释器 COMMAND.CMD，系统停机
msg_missing_shell:
  .byte $07, $0A, $4D, $69, $73, $73, $69, $6E, $67, $20, $44, $4F, $53, $20, $73, $68
  .byte $65, $6C, $6C, $20, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D, $44, $0D
  .byte $0A, $42, $42, $47, $44, $4F, $53, $20, $62, $6F, $6F, $74, $20, $66, $61, $69
  .byte $6C, $75, $72, $65, $21, $20, $48, $61, $6C, $74, $65, $64, $20, $6E, $6F, $77
  .byte $0D, $0A, $24
; 命令解释器的 11 字节文件名“COMMAND CMD”
name_command_cmd:
  .byte $43, $4F, $4D, $4D, $41, $4E, $44, $20, $43, $4D, $44
shell_arguments:
  .byte $02, $2F, $50, $0D
config_read_line:
  ldx config_read_index
  ldy #$00
config_line_loop:
  jsr config_ensure_buffer
  bcs config_finish
  lda config_buffer,x
  cmp #$1A
  beq config_finish
  inx
  cmp #$0A
  beq config_line_end
  cmp #$0D
  beq config_after_cr
  sta text_line_buffer,y
  iny
  cpy #$4F
  bcc config_line_loop
config_skip_overlong:
  jsr config_ensure_buffer
  bcs config_finish
  lda config_buffer,x
  cmp #$1A
  beq config_finish
  inx
  cmp #$0A
  beq config_line_end
  cmp #$0D
  bne config_skip_overlong
config_after_cr:
  jsr config_ensure_buffer
  bcs config_finish
  lda config_buffer,x
  cmp #$0A
  bne config_line_end
  inx
config_line_end:
  lda #$0D
  sta text_line_buffer,y
  stx config_read_index
  jsr config_parse_line
  ldx #$03
  sec
config_increment_line:
  lda config_line_number,x
  adc #$00
  sta config_line_number,x
  cmp #$39
  bcc config_read_line
  lda #$30
  sta config_line_number,x
  dex
  bpl config_increment_line
  bmi config_read_line
config_finish:
  tya
  beq config_close_file
  lda #$0D
  sta text_line_buffer,y
  jsr config_parse_line
config_close_file:
  lda #$05
  jsr file_call_function
shell_install_vectors:
  lda #$4C
  ldx #$9B
  ldy #$29
  sta CRITICAL_ERROR_ENTRY
  stx critical_target_high
  sty critical_target_low
  sta CTRL_C_ENTRY
  ldx #$03
shell_copy_args_loop:
  lda shell_arguments,x
  sta command_tail,x
  dex
  bpl shell_copy_args_loop
  lda #$40
  sta critical_error_flags
  lda boot_disk
  sta far_ptr_high
  ldx #$B5
  ldy #$27
  jsr dos_run_program
  ldx #$B4
  ldy #$E4
startup_print_halt:
  jsr startup_print_string_crlf
startup_halt:
  jmp startup_halt
