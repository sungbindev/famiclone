; 模块：dos-console
; 控制台输入输出与 DOS 调用分发：键盘、字符与字符串显示、命令行编辑

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_29BC"
; 段 0，$A5BC-$AAA8
; 判断按键是否为 Ctrl-C：A=3，或 A=0 且 X=0，Z=1 表示是。
key_is_ctrl_c:
  cmp #$03
  beq key_check_done
  cmp #$00
  bne key_check_done
  cpx #$00
key_check_done:
  rts

; 调用 BIOS 功能 $34（X=0），开始等待键盘输入（具体作用未确认）。
key_wait_begin:
  lda #$34
  ldx #$00
  jmp BIOS_ENTRY

; 调用 BIOS 功能 $18（外设调用）子功能 0：取出已按下的键，A 为字符码，X 为功能键码。
key_fetch:
  lda #$18
  ldx #$00
  jmp BIOS_ENTRY

; 输出字符 X：调用 BIOS 功能 $1B（Y=0）。
console_put_char:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

; 查询键盘并处理控制键：有 Ctrl-S（$13）则暂停到再按键；按 Ctrl-C 经 CTRL_C_ENTRY 通知应用。出口：Z=1 无键；Z=0 有键，A 为键码。
key_poll_control:
  jsr dos_check_keyboard
  beq poll_done
  cmp #$13
  bne poll_other_key
  jsr key_fetch
  jsr key_fetch
  jsr key_is_ctrl_c
  beq poll_call_ctrl_c
  rts
poll_other_key:
  jsr key_is_ctrl_c
  bne poll_done
  jsr key_fetch
poll_call_ctrl_c:
  jsr CTRL_C_ENTRY
poll_done:
  rts

; 读调用者页 current_page 中地址 far_ptr 加 Y 处的一个字节（BIOS 功能 $20）。出口：A 为该字节，X、far_ptr_high 与 saved_x 保持不变。
read_far_byte:
  stx saved_x
  ldx current_page
  lda far_ptr_high
  pha
  lda #$20
  jsr BIOS_ENTRY
  tax
  pla
  sta far_ptr_high
  txa
  ldx saved_x
  rts

; 把 transfer_command 里的字节写到调用者页 current_page 中地址 far_ptr 加 Y 处（BIOS 功能 $21），far_ptr_high 保持不变。
write_far_byte:
  lda far_ptr_high
  pha
  lda #$21
  ldx current_page
  jsr BIOS_ENTRY
  pla
  sta far_ptr_high
  rts

; 检查文件名字符 A：控制字符、空格与 双引号 * + , . / : ; < = > [ 反斜杠 ] | $7F 无效（? 与 - 有效）；小写字母转成大写。出口：C=0 有效，A 为结果字符；C=1 无效，并把 result_code 置 2。
name_char_check:
  cmp #$21
  bcc char_invalid
  beq char_valid
  cmp #$23
  bcc char_invalid
  cmp #$2A
  bcc char_valid
  cmp #$2D
  beq char_valid
  cmp #$30
  bcc char_invalid
  cmp #$3A
  bcc char_valid
  cmp #$3F
  bcc char_invalid
  cmp #$5B
  bcc char_valid
  cmp #$5E
  bcc char_invalid
  cmp #$61
  bcc char_valid
  cmp #$7B
  bcc char_uppercase
  cmp #$7C
  beq char_invalid
  cmp #$7F
  bne char_valid
char_invalid:
  lda #$02
  sta result_code
  sec
  rts
char_uppercase:
  and #$DF
char_valid:
  clc
  rts

; DOS 调用分发表：17 项（功能号 0-16），每项 2 字节，内容是处理例程地址减 1（用 RTS 跳转）。
dos_call_table:
dos_call_table_high = * + 1
  .word dos_program_return - 1, dos_read_key_echo - 1
  .word dos_display_char - 1, dos_inquire_pages - 1
  .word dos_display_string - 1, dos_run_program - 1
  .word dos_get_kb_string - 1, dos_read_key_no_echo - 1
  .word dos_read_kb_clear - 1, dos_initial_screen - 1
  .word dos_release_pages - 1, dos_get_set_disp_mode - 1
  .word dos_file_name_access - 1, dos_get_set_init_screen - 1
  .word dos_check_keyboard - 1, dos_run_path_program - 1
  .word dos_get_version - 1

; DOS 调用分发（入口 $580C 的桩代码把页切到 DOS 页后调用本例程）。
; 入口：A 为功能号 0-16，X、Y 为各功能自己的参数。
; 出口：功能号大于 16 时 A=1（ErrorCommand）且 C=1；否则转到 dos_call_table 里的处理例程，由它设置 C 与 A。
dos_call_dispatch:
  asl a
  cmp #$22
  bcs dos_dispatch_bad_function
  stx saved_x
  tax
  lda dos_call_table_high,x
  pha
  lda dos_call_table,x
  pha
  ldx saved_x
  clc
dos_dispatch_bad_function:
  lda #$01
  rts

; DOS 功能 16 DosGetDOSVersion（手册未列）：取 DOS 版本。
; 出口：A=3，X=0，C=0，对应屏幕上显示的 BBGDOS Version 3.0。
dos_get_version:
  lda #$03
  ldx #$00
  clc
  rts

; DOS 功能 0 DosProgramReturn：程序结束返回。
; 入口：X 为返回码；Y 位 0=1 先初始化显示屏幕（DosInitialScreen），位 1=1 先初始化系统资源（BIOS 功能 4）。
; 副作用：把堆栈置空，恢复调用者的程序状态（弹出运行程序时压入的 4 字节与页号），随后返回上一层程序。
dos_program_return:
  stx result_code
  sty result_flags
  ldx #$FF
  txs
  lda #$22
  jsr BIOS_ENTRY
  lda result_flags
  and #$02
  beq return_check_screen
  lda #$18
  ldx #$06
  ldy #$00
  jsr BIOS_ENTRY
  lda #$17
  jsr BIOS_ENTRY
  txa
  pha
  tya
  pha
  lda #$04
  jsr BIOS_ENTRY
  pla
  tay
  pla
  tax
  lda #$16
  jsr BIOS_ENTRY
return_check_screen:
  lda result_flags
  lsr a
  bcc program_return_pop
  jsr dos_initial_screen
program_return_pop:
  ldx run_stack_top
  dex
  lda run_stack,x
  sta current_page
  ldy #$04
return_pop_loop:
  dex
  lda run_stack,x
  pha
  dey
  bne return_pop_loop
  stx run_stack_top
  lda result_code
  ldy result_flags
  clc
  rts

; 判断读到的键是否为功能键：A=0，或 A=$E0 且 X 不是 $E2、$E3 时 C=1；否则 C=0（普通字符）。
key_is_function_key:
  cmp #$00
  beq fkey_done
  cpx #$E2
  beq fkey_clear_carry
  cpx #$E3
  beq fkey_clear_carry
  cmp #$E0
  beq fkey_done
fkey_clear_carry:
  clc
fkey_done:
  rts

; DOS 功能 7 DosReadKeyNoEcho：读键盘，不回显。
; 出口：A 为字符码；0 表示功能键，下一次调用取功能键码。
dos_read_key_no_echo:
  ldx #$00
  lda pending_function_key
  bne read_key_store_pending
  jsr key_wait_begin
read_key_wait:
  jsr key_poll_control
  beq read_key_wait
  jsr key_fetch
  jsr key_is_function_key
  bcc read_key_done
  lda #$00
read_key_store_pending:
  stx pending_function_key
read_key_done:
  rts

; DOS 功能 1 DosReadKeyEcho：读键盘并回显。
; 出口：A 为字符码；0 表示功能键，下一次调用取功能键码。
dos_read_key_echo:
  jsr dos_read_key_no_echo
  pha
  tax
  jsr dos_display_char
  pla
  rts

; DOS 功能 14 DosCheckKeyBoard：测试键盘是否有键。
; 出口：Z=1 无键；Z=0 有键，A 为键码（经外设调用 X=1 取得，有键时再读一次键盘）。
dos_check_keyboard:
  ldx #$01
  jsr DEVICE_ENTRY
  tya
  beq check_kb_done
  lda #$18
  ldx #$01
  jsr BIOS_ENTRY
  cpy #$00
check_kb_done:
  rts

; DOS 功能 8 DosReadKBClear：先清空键盘缓冲，再执行 $FF 里指定的键盘功能（1、6、7）。
; 入口：$FF 为要执行的 DOS 功能号。
dos_read_kb_clear:
  lda far_ptr_high

; 先清空键盘缓冲（把已按下的键都读掉，并清除待取的功能键），再按 A 给出的 DOS 功能号调用 dos_call_dispatch；X、Y 保持。
kb_clear_and_call:
  pha
  lda #$00
  sta pending_function_key
  txa
  pha
  tya
  pha
  jsr key_wait_begin
clear_kb_loop:
  jsr key_poll_control
  beq clear_kb_dispatch
  jsr key_fetch
  jmp clear_kb_loop
clear_kb_dispatch:
  pla
  tay
  pla
  tax
  pla
  jmp dos_call_dispatch

; 弹出两层返回地址后返回（直接回到上上层调用者）。未找到调用处，用途未确认。
pop_two_return:
  pla
  pla
  rts
pending_char_byte:
  .byte $00

; 如果有等待配对的汉字首字节（pending_char_byte），把它单独输出（BIOS 功能 $1B，Y=0）并清除。
display_flush_pending:
  ldx pending_char_byte
  beq flush_pending_done
  lda #$1B
  ldy #$00
  jsr BIOS_ENTRY
  lda #$00
  sta pending_char_byte
flush_pending_done:
  rts

; DOS 功能 2 DosDisplayChar：显示字符。
; 入口：X 为字符码；$0D 回车，$0A 换行，$09 按 8 列制表位补空格；中文方式下两个字节组成一个汉字。
dos_display_char:
  cpx #$09
  bne char_mode_check
  jsr display_flush_pending
  lda #$1E
  jsr BIOS_ENTRY
  tya
  sta saved_x
  ora #$07
  sec
  sbc saved_x
  beq char_after_tab
tab_fill_loop:
  pha
  ldx #$20
  jsr console_put_char
  pla
  sec
  sbc #$01
  bne tab_fill_loop
char_after_tab:
  ldx #$20
char_mode_check:
  bit display_mode
  bpl char_single_byte
  cpx #$80
  bcs char_high_byte
  txa
  pha
  jsr display_flush_pending
  pla
  tax
char_single_byte:
  ldy #$00
  beq char_output
char_high_byte:
  ldy pending_char_byte
  beq char_hold_lead
  lda #$00
  sta pending_char_byte
char_output:
  lda #$1B
  jsr BIOS_ENTRY
  lda ctrl_check_enabled
  beq char_done
  jmp key_poll_control
char_hold_lead:
  stx pending_char_byte
char_done:
  rts
free_page:
  .byte $00

; DOS 功能 3 DosInquirePages：申请内存页（16KB 一页）。
; 入口：X 为申请页数；0 表示只取当前空闲页号。
; 出口：C=0 时 A 为申请到的起始页号；C=1 时页数不够，A 为最大可申请页数。
dos_inquire_pages:
  txa
  clc
  adc free_page
  bcs pages_insufficient
  cmp #$FF
  beq pages_insufficient
  ldx free_page
  sta free_page
  txa
  clc
  rts
pages_insufficient:
  lda #$00
  sbc free_page
  sec
  rts
bios_version_high:
  .byte $00
bios_version_low:
  .byte $00

; 没有被引用的例程：按 BIOS 版本（bios_version_high、bios_version_low，即 $40 与 $62 以上）选择不同方式调用 BIOS 功能 $3C，并经 BIOS 功能 1 切回原页（用途未确认）。
; 没有被引用的例程：按 BIOS 版本判断后调用 BIOS 功能 $3C（未确认用途）
dos_unused_bios_3c:
  pla
  tax
  lda bios_version_high
  cmp #$40
  bne bios3c_call_old
  lda bios_version_low
  cmp #$62
  bcs bios3c_call_old
  cpx #$00
  beq bios3c_call_new
  sec
  bcs bios3c_finish
bios3c_call_new:
  lda #$3C
  jsr BIOS_ENTRY
  clc
  bcc bios3c_finish
bios3c_call_old:
  lda #$3C
  jsr BIOS_ENTRY
  cmp #$01
bios3c_finish:
  pla
  tax
  lda #$01
  jmp BIOS_ENTRY
init_screen_page:
  .byte $00
init_screen_vector_low:
  .byte $F6
init_screen_vector_high:
  .byte $A7

; DOS 功能 9 DosInitialScreen：初始化显示屏幕（字模、调色板、汉字提示行、光标），清屏。
; 处理分两步：先 BIOS 功能 $22 初始化显示，再链式调用 DosGetSetInitScreen 设置的向量。
dos_initial_screen:
  lda #$22
  jsr BIOS_ENTRY
  lda #$00
  sta pending_char_byte
  sta pending_function_key
  lda dos_home_page
  pha
  lda display_mode
  pha
  lda init_screen_vector_high
  pha
  lda init_screen_vector_low
  pha
  lda #$01
  ldx init_screen_page
  jmp BIOS_ENTRY

; DOS 功能 10 DosReleasePages：释放内存页。
; 入口：X 为要释放的起始页号，从该页起的所有页都被释放。
dos_release_pages:
  stx free_page
  clc
  rts

; DOS 功能 4 DosDisplayString：显示字符串。
; 入口：X:Y 为字符串地址（X 高字节，Y 低字节，当前页），字符串以 '$' 结束；逐字符调用 dos_display_char。
dos_display_string:
  lda far_ptr_high
  pha
  lda far_ptr_low
  pha
  stx far_ptr_high
  sty far_ptr_low
  jsr display_flush_pending
  ldy #$00
display_string_loop:
  jsr read_far_byte
  cmp #$24
  beq display_string_end
  tax
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne display_string_loop
  inc far_ptr_high
  jmp display_string_loop
display_string_end:
  jsr display_flush_pending
  pla
  sta far_ptr_low
  pla
  sta far_ptr_high
  rts

; 英文方式下在光标处画实心块（字符 $DB，BIOS 功能 $1D）作光标；中文方式不处理。
cursor_show_block:
  bit display_mode
  bmi cursor_done
  ldx #$DB
  bne cursor_draw

; 英文方式下擦掉光标（画空格）；中文方式不处理。
cursor_hide_english:
  bit display_mode
  bmi cursor_done

; 在光标处画空格擦掉光标（BIOS 功能 $1D，Y=1，X=$20）。
cursor_hide:
  ldx #$20
cursor_draw:
  lda #$1D
  ldy #$01
  jsr BIOS_ENTRY
cursor_done:
  rts
edit_max_length:
  .byte $00
edit_length:
  .byte $00
edit_recall_index:
  .byte $00
edit_insert_mode:
  .byte $00
edit_start_column:
  .byte $00

; 把字符 A 追加到正在输入的行：写入用户缓冲区的 edit_length 位置并回显（控制字符显示为 ^ 加字母）；缓冲区满（edit_length 加 1 达到 edit_max_length）时 C=1。
edit_insert_char:
  ldx edit_length
  inx
  cpx edit_max_length
  bcs insert_done
  sta transfer_command
  ldy edit_length
  jsr write_far_byte
  inc edit_length
  ldx transfer_command
  cpx #$20
  bcs insert_echo
  ldy #$5E
  jsr dos_display_char
  lda transfer_command
  ora #$40
  tax
insert_echo:
  jsr dos_display_char
  clc
insert_done:
  rts

; F1 与右键：把上一次输入行（line_buffer）的下一个字符（edit_recall_index 处，不是回车时）复制到当前行并前进一位；edit_insert_mode 清零。Z=1 表示已到行尾。
edit_copy_char:
  lda #$00
  sta edit_insert_mode
  ldx edit_recall_index
  lda line_buffer,x
  cmp #$0D
  beq copy_char_done
  jsr edit_insert_char
  inc edit_recall_index
copy_char_done:
  rts

; 退格与左键：删掉当前行最后一个字符并擦除屏幕上的显示（控制字符占两格），同时上一行的回显位置 edit_recall_index 退一位（不小于 0）。
edit_backspace:
  lda #$00
  sta edit_insert_mode
  ldy edit_length
  beq backspace_recall
  dey
  sty edit_length
  jsr read_far_byte
  cmp #$20
  bcs backspace_erase
  jsr cursor_hide_english
  ldx #$08
  jsr dos_display_char
  jsr cursor_hide
backspace_erase:
  jsr cursor_hide_english
  ldx #$08
  jsr dos_display_char
  jsr cursor_hide
backspace_recall:
  dec edit_recall_index
  bpl backspace_done
  inc edit_recall_index
backspace_done:
  rts

; 显示光标并等待按键：等到有键后取出。出口：A 为字符码，X 为功能键码；光标留在屏幕上，由调用者擦除。
edit_read_key:
  jsr cursor_show_block
  jsr key_wait_begin
edit_read_key_loop:
  jsr key_poll_control
  beq edit_read_key_loop
  jsr key_fetch
  rts

; F2 键：等待按下一个字符，把上一行从当前回显位置起直到该字符之前的内容复制到当前行；上一行里没有该字符时不复制。
; 命令行编辑 F2 键：从上一行复制到指定字符之前
edit_copy_up_to_char:
  lda #$00
  sta edit_insert_mode
  jsr edit_read_key
  beq copy_up_to_done
  ldx edit_recall_index
  cpx #$3E
  beq copy_up_to_done
copy_up_to_scan:
  inx
  cpx #$3E
  beq copy_up_to_done
  cmp line_buffer,x
  bne copy_up_to_scan
  ldx edit_recall_index
copy_up_to_copy_loop:
  pha
  lda line_buffer,x
  jsr edit_insert_char
  pla
  inc edit_recall_index
  ldx edit_recall_index
  cmp line_buffer,x
  bne copy_up_to_copy_loop
copy_up_to_done:
  rts

; F3 键：把上一行剩下的部分全部复制到当前行。
; 命令行编辑 F3 键：复制上一行的剩余部分
edit_copy_rest:
  lda #$00
  sta edit_insert_mode
copy_rest_loop:
  jsr edit_copy_char
  bne copy_rest_loop
  rts

; F4 键：等待按下一个字符，把上一行的回显位置跳到该字符处（跳过之前的内容，不复制）。
; 命令行编辑 F4 键：跳过上一行到指定字符为止的内容
edit_skip_up_to_char:
  lda #$00
  sta edit_insert_mode
  jsr edit_read_key
  cmp #$0D
  beq skip_up_to_done
  ldx edit_recall_index
skip_up_to_scan:
  cmp line_buffer,x
  beq skip_up_to_found
  ldy line_buffer,x
  inx
  cpy #$0D
  bne skip_up_to_scan
  rts
skip_up_to_found:
  stx edit_recall_index
skip_up_to_done:
  rts

; Ins 键：切换插入方式（edit_insert_mode 加 1，位 0 为 1 时每输入一个字符不推进上一行的回显位置）。
; 命令行编辑 Ins 键：切换插入方式
edit_toggle_insert:
  inc edit_insert_mode
  rts

; Del 键：跳过上一行的一个字符（不是回车时回显位置加 1）。
; 命令行编辑 Del 键：跳过上一行的一个字符
edit_skip_char:
  ldx edit_recall_index
  lda line_buffer,x
  cmp #$0D
  beq skip_char_done
  inc edit_recall_index
skip_char_done:
  rts
edit_key_codes:
  .byte $3B, $3C, $3D, $3E, $4B, $4D, $52, $53
edit_key_handler_high:
  .byte $A8, $A9, $A9, $A9, $A8, $A8, $A9, $A9
edit_key_handler_low:
  .byte $C6, $1F, $51, $5C, $DC, $C6, $7D, $81

; 按功能键在 edit_key_codes 里的下标 X，用 RTS 跳转到命令行编辑处理例程（edit_key_handler_high、edit_key_handler_low，地址减 1）。
edit_dispatch_key:
  lda edit_key_handler_high,x
  pha
  lda edit_key_handler_low,x
  pha
  rts

; DOS 功能 6 DosGetKBString：输入一行字符并回显，支持 F1-F4 命令行编辑。
; 入口：X:Y 为缓冲区地址；缓冲区第 0 字节是最大字符个数（含回车）。
; 出口：缓冲区第 1 字节为实际输入的字符个数（不含回车），第 2 字节起是输入内容，以回车结束。
dos_get_kb_string:
  lda far_ptr_high
  pha
  lda far_ptr_low
  pha
  lda transfer_command
  pha
  stx far_ptr_high
  sty far_ptr_low
  ldy #$00
  jsr read_far_byte
  sta edit_max_length
  lda far_ptr_low
  clc
  adc #$02
  sta far_ptr_low
  bcc getstr_init
  inc far_ptr_high
getstr_init:
  lda #$00
  sta edit_length
  sta edit_recall_index
  sta edit_insert_mode
  lda #$1E
  jsr BIOS_ENTRY
  sty edit_start_column
getstr_loop:
  jsr edit_read_key
  cmp #$08
  bne getstr_check_special
  jsr edit_backspace
  jmp getstr_loop
getstr_check_special:
  jsr key_is_function_key
  bcs getstr_function_key
  cmp #$1B
  bne getstr_normal_key
  ldx #$5C
  jsr dos_display_char
  jsr startup_print_crlf
  lda edit_start_column
  beq getstr_reset_line
  sta transfer_command
getstr_erase_loop:
  ldx #$20
  jsr dos_display_char
  dec transfer_command
  bne getstr_erase_loop
getstr_reset_line:
  lda #$00
  sta edit_length
  sta edit_recall_index
  sta edit_insert_mode
  beq getstr_loop
getstr_function_key:
  txa
  ldx #$08
getstr_find_key_loop:
  dex
  bmi getstr_loop
  cmp edit_key_codes,x
  bne getstr_find_key_loop
  jsr edit_dispatch_key
  jmp getstr_loop
getstr_normal_key:
  cmp #$0D
  beq getstr_enter
  cmp #$09
  bne getstr_check_ctrl_c
  lda #$20
getstr_check_ctrl_c:
  cmp #$03
  beq getstr_cancel
  jsr edit_insert_char
  bcs getstr_loop
  lda edit_insert_mode
  lsr a
  bcs getstr_loop
  ldx edit_recall_index
  lda line_buffer,x
  cmp #$0D
  beq getstr_loop
  inc edit_recall_index
  bne getstr_loop
getstr_cancel:
  ldy #$00
  sty edit_length
  sec
  bcs getstr_finish
getstr_enter:
  pha
  jsr cursor_hide_english
  pla
  tax
  jsr dos_display_char
  clc
getstr_finish:
  php
  lda #$0D
  sta transfer_command
  ldy edit_length
  jsr write_far_byte
  cpy #$3E
  bcc getstr_save_line_loop
  lda #$0D
  sta line_buffer_end
  ldy #$3C
getstr_save_line_loop:
  jsr read_far_byte
  sta line_buffer,y
  dey
  bpl getstr_save_line_loop
  lda far_ptr_low
  sec
  sbc #$02
  sta far_ptr_low
  bcs getstr_store_length
  dec far_ptr_high
getstr_store_length:
  ldx edit_length
  stx transfer_command
  ldy #$01
  jsr write_far_byte
  plp
  pla
  sta transfer_command
  pla
  sta far_ptr_low
  pla
  sta far_ptr_high
  rts
