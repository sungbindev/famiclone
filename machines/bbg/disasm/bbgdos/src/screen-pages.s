; 模块：screen-pages
; 显示方式、初始化屏幕向量、文件名转换，以及程序运行用的显存与页保存恢复

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2EA9"
; 段 0，$AAA9-$B173
; DOS 功能 11 DosGetSetDispMode：取或设置显示方式。
; 入口：X 位 0=0 取方式，A 返回 0（英文）或 $80（中文）；X 位 0=1 设置，Y 为 0（英文）或 $80（中文）；X 位 1=1 时以后才改变屏幕显示。
dos_get_set_disp_mode:
  txa
  lsr a
  bcs dispmode_set
  lda display_mode
  rts
dispmode_set:
  lsr a
  bcc display_mode_apply
  sty display_mode
  clc
  rts

; 应用 Y 给出的显示方式：display_mode 暂设为 Y，调用 dos_initial_screen 重新初始化屏幕；BIOS 返回 C=1 时恢复原显示方式；最后清除 display_mode 位 0 与 pending_char_byte。
display_mode_apply:
  lda display_mode
  pha
  sty display_mode
  jsr dos_initial_screen
  pla
  bcc dispmode_finish
  sta display_mode
dispmode_finish:
  lda display_mode
  and #$FE
  sta display_mode
  lda #$00
  sta pending_char_byte
  rts

; DOS 功能 13 DosGetSetInitScrn：取并设置初始化显示屏幕处理向量。
; 入口：$FF 为新向量所在页，X:Y 为新的处理程序地址减 1（X 高字节）。
; 出口：A 为原向量所在页，X:Y 为原处理程序地址减 1。
dos_get_set_init_screen:
  lda init_screen_vector_high
  stx init_screen_vector_high
  tax
  lda init_screen_vector_low
  sty init_screen_vector_low
  tay
  lda init_screen_page
  pha
  lda far_ptr_high
  sta init_screen_page
  pla
  rts

; DOS 功能 12 DosFileNameAccs：把 "D:FFFFFFFF.XXX" 形式的文件名转换成 11 字节的内部格式（主名 8 字节加扩展名 3 字节，空格补齐，'*' 展开成 '?'）。
; 入口：X:Y 为以 0 结尾的文件名地址；转换结果写回原地址。
; 出口：A 为盘号（0 默认盘，1 为 A 盘），盘符非法时 A=$0F。
dos_file_name_access:
  lda far_ptr_high
  pha
  lda far_ptr_low
  pha
  lda transfer_command
  pha
  stx far_ptr_high
  sty far_ptr_low
  ldy #$00
  sty result_code
fname_skip_blank:
  jsr read_far_byte
  tax
  beq fname_main_start
  iny
  cmp #$20
  beq fname_skip_blank
  cmp #$09
  beq fname_skip_blank
  jsr read_far_byte
  dey
  cmp #$3A
  bne fname_main_start
  jsr read_far_byte
  and #$DF
  cmp #$5B
  bcs fname_bad_drive
  sbc #$40
  bcs fname_store_drive
  sec
fname_bad_drive:
  lda #$0F
  sta result_code
  jmp fname_finish
fname_store_drive:
  adc #$00
  sta result_code
  iny
  iny
fname_main_start:
  ldx #$00
fname_main_loop:
  jsr read_far_byte
  cmp #$00
  beq fname_main_pad_space
  cmp #$2E
  beq fname_main_pad_space
  iny
  cmp #$2A
  beq fname_main_star
  cpx #$08
  bcs fname_main_loop
  jsr name_char_check
  bcs fname_finish
  sta name_buffer,x
  inx
  bne fname_main_loop
fname_main_pad_space:
  lda #$20
  bne fname_main_fill
fname_main_star:
  lda #$3F
fname_main_fill:
  cpx #$08
  bcs fname_skip_to_ext
  sta name_buffer,x
  inx
  bne fname_main_fill
fname_skip_to_ext:
  jsr read_far_byte
  cmp #$00
  beq fname_ext_pad_space
  iny
  cmp #$2E
  bne fname_skip_to_ext
fname_ext_loop:
  jsr read_far_byte
  iny
  cmp #$00
  beq fname_ext_pad_space
  cmp #$2A
  beq fname_ext_star
  cpx #$0B
  bcs fname_ext_loop
  jsr name_char_check
  bcs fname_finish
  sta name_buffer,x
  inx
  bne fname_ext_loop
fname_ext_pad_space:
  lda #$20
  bne fname_ext_fill
fname_ext_star:
  lda #$3F
fname_ext_fill:
  cpx #$0B
  bcs fname_write_back
  sta name_buffer,x
  inx
  bne fname_ext_fill
fname_write_back:
  ldy #$0A
fname_write_loop:
  lda name_buffer,y
  sta transfer_command
  jsr write_far_byte
  dey
  bpl fname_write_loop
  clc
fname_finish:
  pla
  sta transfer_command
  pla
  sta far_ptr_low
  pla
  sta far_ptr_high
  lda result_code
  rts

; 把 DOS 自己所在页（dos_home_page）的 $8000-$97FF（6KB 常驻代码）经 BIOS 功能 $0C 复制到页 0 的同一地址，并把 dos_page 改为 0。
dos_copy_to_page_0:
  lda dos_home_page
  sta move_src_page
  ldx #$80
  ldy #$00
  stx move_src_high
  sty move_src_low
  lda #$00
  sta move_dst_page
  sta dos_page
  stx move_dst_high
  sty move_dst_low
  ldx #$18
  ldy #$00
  stx move_length_high
  sty move_length_low
  lda #$0C
  ldx #$80
  jmp BIOS_ENTRY

; 用 $0400 的文件调用块读程序文件：记下请求的字节数，调用 file_call_function（功能 2），再核对实际读到的字节数。出口：C=0 读满；读失败 A=$1E、C=1；没有读满 A=$0B、C=1。
exe_read_checked:
  ldx fb_bytes_high
  ldy fb_bytes_low
  stx result_flags
  sty result_code
  lda #$02
  jsr file_call_function
  lda #$1E
  bcs read_checked_done
  ldx result_flags
  cpx fb_bytes_high
  bne read_checked_short
  ldy result_code
  cpy fb_bytes_low
  clc
  beq read_checked_done
read_checked_short:
  lda #$0B
  sec
read_checked_done:
  rts

; 把段表第 Y/4 项的 3 字节文件偏移写进 fb_position_0-2，并把 fb_bytes_high 清零，作为下一次读的位置。
exe_set_position:
  lda #$00
  sta fb_bytes_high
  lda exe_seg_offset_2,y
  sta fb_position_2
  lda exe_seg_offset_1,y
  sta fb_position_1
  lda exe_seg_offset_0,y
  sta fb_position_0
  rts

; 读段表项 Y/4 指向的段记录：A 为要读的字节数，读到 $0457 起（seg_rec_1 起），位置由 exe_set_position 设置。出口同 exe_read_checked。
exe_read_segment_record:
  sta fb_bytes_low
  jsr exe_set_position
  ldx #$04
  ldy #$57
  stx fb_buffer_high
  sty fb_buffer_low
  jmp exe_read_checked

; 按 6 字节段记录设置读写块：缓冲区地址取 seg_rec_2、seg_rec_3（低、高），字节数取 seg_rec_4、seg_rec_5。出口：X:Y 为缓冲区地址（X 高）。
exe_set_buffer_from_record:
  ldx seg_rec_5
  ldy seg_rec_4
  stx fb_bytes_high
  sty fb_bytes_low
  ldx seg_rec_3
  ldy seg_rec_2
  stx fb_buffer_high
  sty fb_buffer_low
  rts

; 把 fb_bytes 设为 $C000 减缓冲区地址与 5 字节段记录里的长度（seg_rec_3、seg_rec_4）中较小者，不让数据超过 $C000。
exe_limit_to_page_end:
  lda #$00
  sec
  sbc fb_buffer_low
  tay
  lda #$C0
  sbc fb_buffer_high
  tax
  cpy seg_rec_3
  sbc seg_rec_4
  bcc limit_store
  ldx seg_rec_4
  ldy seg_rec_3
limit_store:
  stx fb_bytes_high
  sty fb_bytes_low
  rts

; 第 4 类段（5 字节记录：地址、长度）：把数据读进 free_page 页的 $8000 起，再经 BIOS 功能 $25（参数块在 $80，Y=$40，目标页为 free_page；记录地址高字节为 $C0 时为页 $FF）处理（BIOS $25 的含义未确认）。
exe_load_type_04:
  lda #$05
  jsr exe_read_segment_record
  bcs type04_done
  lda free_page
  sta fb_page
  sta move_src_page
  ldx #$80
  ldy #$00
  stx fb_buffer_high
  sty fb_buffer_low
  stx move_src_high
  sty move_src_low
  jsr exe_limit_to_page_end
  jsr exe_read_checked
  bcs type04_done
  ldx free_page
  lda seg_rec_2
  cmp #$C0
  bne type04_store_page
  ldx #$FF
type04_store_page:
  stx move_dst_page
  lda #$25
  ldx #$80
  ldy #$40
  jsr BIOS_ENTRY
  clc
type04_done:
  rts

; 第 2 类段（5 字节记录）：把数据读进 free_page 页的记录地址（seg_rec_1:seg_rec_2）处；超过 $C000 的部分分两次读，后一部分读进页 $FF（地址去掉位 6）。
exe_load_type_02:
  lda #$05
  jsr exe_read_segment_record
  bcs type02_done
  ldx seg_rec_2
  ldy seg_rec_1
  cpx #$C0
  bcs type02_spill
  lda free_page
  sta fb_page
  stx fb_buffer_high
  sty fb_buffer_low
  jsr exe_limit_to_page_end
  lda seg_rec_3
  sec
  sbc fb_bytes_low
  sta seg_rec_3
  lda seg_rec_4
  sbc fb_bytes_high
  sta seg_rec_4
  jsr exe_read_checked
  bcs type02_done
  ldx #$C0
  ldy #$00
type02_spill:
  lda #$FF
  sta fb_page
  txa
  and #$BF
  sta fb_buffer_high
  sty fb_buffer_low
  jsr exe_limit_to_page_end
  jsr exe_read_checked
type02_done:
  rts

; 装入需要整页覆盖时的屏幕准备（仅 X=$FF 时执行，其他值直接返回）：exe_type_flag 位 7 为 1 时先用 BIOS 功能 4（参数 exe_flags）初始化系统资源并把显示方式切成英文；中文方式下清 ppu_reg_2009；最后把 ppu_control_shadow 去掉位 7 写入 ppu_control（关 NMI）并 SEI。
exe_screen_off:
  cpx #$FF
  bne screen_off_done
  bit exe_type_flag
  bpl screen_off_check_mode
  inc screen_initialized
  lda #$04
  ldx exe_flags
  jsr BIOS_ENTRY
  bit display_mode
  bpl screen_off_ppu
  ldx #$01
  ldy #$00
  jsr dos_get_set_disp_mode
screen_off_check_mode:
  bit display_mode
  bpl screen_off_ppu
  lda #$00
  sta ppu_reg_2009
screen_off_ppu:
  lda ppu_control_shadow
  and #$7F
  sta ppu_control
  sei
screen_off_done:
  rts

; 第 0D 类段（6 字节记录：页、地址、长度）：先把数据读进 free_page 页的缓冲区，再经 BIOS 功能 $25 复制到记录页 seg_rec_1（复制前先 exe_screen_off，BIOS $25 的具体含义未确认），最后落入 type0b_check_full。
exe_load_type_0d:
  lda #$06
  jsr exe_read_segment_record
  bcs type_load_done
  lda free_page
  sta fb_page
  sta move_src_page
  jsr exe_set_buffer_from_record
  stx move_src_high
  sty move_src_low
  jsr exe_read_checked
  bcs type_load_done
  ldx seg_rec_1
  stx move_dst_page
  jsr exe_screen_off
  lda #$25
  ldx #$80
  ldy #$40
  jsr BIOS_ENTRY
  jmp type0b_check_full

; 第 0B、0C 类段（6 字节记录）：把数据直接读进记录页 seg_rec_1 的记录地址处（页号小于 free_page 时 screen_initialized 加 1），读前先 exe_screen_off；页号为 $FF 时读完调用 BIOS 功能 9（未确认用途）。
exe_load_type_0b_0c:
  lda #$06
  jsr exe_read_segment_record
  bcs type_load_done
  ldx seg_rec_1
  stx fb_page
  cpx free_page
  bcs type0b_read
  inc screen_initialized
type0b_read:
  jsr exe_screen_off
  jsr exe_set_buffer_from_record
  jsr exe_read_checked
  bcs type_load_done
type0b_check_full:
  lda seg_rec_1
  cmp #$FF
  bcc type_load_done
  lda #$09
  jsr BIOS_ENTRY
  clc
type_load_done:
  rts

; 第 0E 类段（6 字节记录）：目标页为 free_page 加 seg_rec_1 加 1（相对页号），页号用完（达到 $FF）时 A=8、C=1；否则把数据读进该页。
exe_load_type_0e:
  lda #$06
  jsr exe_read_segment_record
  bcs type_load_done
  lda free_page
  sec
  adc seg_rec_1
  bcs type0e_no_pages
  cmp #$FF
  beq type0e_no_pages
  sta fb_page
  jsr exe_set_buffer_from_record
  jmp exe_read_checked
type0e_no_pages:
  lda #$08
  rts

; 读 3 字节段记录得到数据长度（seg_rec_1、seg_rec_2），并落入 exe_read_data_at_8000 把这么多字节读进 free_page 页的 $8000。
exe_read_data_length:
  lda #$03
  jsr exe_read_segment_record
  bcs type_load_done
  ldx seg_rec_2
  ldy seg_rec_1

; 把 X:Y（X 高）个字节读进 free_page 页的 $8000 起（设置 fb_* 后调用 exe_read_checked）。
exe_read_data_at_8000:
  stx fb_bytes_high
  sty fb_bytes_low
  ldx free_page
  stx fb_page
  ldx #$80
  ldy #$00
  stx fb_buffer_high
  sty fb_buffer_low
  jmp exe_read_checked

; 为装入图形数据准备屏幕：中文方式先切成英文显示，清 ppu_mask；screen_initialized 为 0 时置 1 并用 BIOS 功能 4（参数 exe_flags）初始化系统资源。
exe_screen_prepare:
  bit display_mode
  bpl screen_prepare_check_init
  ldx #$01
  ldy #$00
  jsr dos_get_set_disp_mode
screen_prepare_check_init:
  lda #$00
  sta ppu_mask
  lda screen_initialized
  bne screen_prepare_done
  inc screen_initialized
  lda #$04
  ldx exe_flags
  jsr BIOS_ENTRY
screen_prepare_done:
  rts

; 调用 BIOS 功能 $15（参数块在零页 $80）：源页为 free_page、源地址 $8000，Y 由调用者给出；块里 $83:$84 是显示存储器目标地址，$85:$86 是长度（含义为由用法推断，未确认）。
exe_bios_15:
  lda free_page
  sta move_src_page
  lda #$80
  sta move_src_high
  lda #$00
  sta move_src_low
  lda #$15
  ldx #$80
  jmp BIOS_ENTRY

; 第 5 类段：读入数据（长度取自 3 字节记录），把目标地址清零、长度写入 $85:$86，然后经 exe_screen_prepare 与 exe_bios_15（Y=1）送入显示存储器。
exe_load_type_05:
  jsr exe_read_data_length
  bcs type05_06_done
  lda #$00
  sta move_dst_low
  sta move_dst_page
  ldx seg_rec_2
  ldy seg_rec_1
  stx move_length_low
  sty move_dst_high
  lda #$01
  bne type05_06_common

; 第 6 类段：读入数据后以 Y=0 调用 exe_bios_15 送入显示存储器（目标地址沿用上一次设置，未确认）。
exe_load_type_06:
  jsr exe_read_data_length
  bcs type05_06_done
  lda #$00
type05_06_common:
  pha
  jsr exe_screen_prepare
  pla
  tay
  jsr exe_bios_15
  clc
type05_06_done:
  rts

; 第 10 类段（图形数据）：本段长度取下一段表项偏移减本项偏移（copy_left_0-2，24 位）；每 $2000 字节为一块：先用 BIOS 功能 $13 对 4 项做设置（Y 为递增序号，含义未确认），再读入 $2000 字节到 free_page 的 $8000，用 exe_bios_15（Y=1）送入显示存储器，直到数据用完。
exe_load_type_10:
  sec
  lda exe_seg_next_offset_0,y
  sbc exe_seg_offset_0,y
  sta copy_left_0
  lda exe_seg_next_offset_1,y
  sbc exe_seg_offset_1,y
  sta copy_left_1
  lda exe_seg_next_offset_2,y
  sbc exe_seg_offset_2,y
  sta copy_left_2
  jsr exe_set_position
  jsr exe_screen_prepare
  lda #$00
  sta copy_left_0
type10_block_loop:
  ldx #$00
type10_bios13_loop:
  txa
  pha
  lda #$13
  ldy copy_left_0
  jsr BIOS_ENTRY
  pla
  tax
  inc copy_left_0
  inx
  cpx #$04
  bcc type10_bios13_loop
  ldx #$20
  ldy #$00
  stx move_length_low
  sty move_dst_high
  jsr exe_read_data_at_8000
  lda #$00
  sta move_dst_low
  sta move_dst_page
  ldy #$01
  jsr exe_bios_15
  sec
  lda copy_left_1
  sbc #$20
  bcs type10_count_down
  dec copy_left_2
  bmi type10_done
type10_count_down:
  sta copy_left_1
  ora copy_left_2
  bne type10_block_loop
type10_done:
  clc
  rts
exe_seg10_index:
  .byte $00

; 装入并运行 .EXC 映像。步骤：读 $1A 字节文件头到 $043D 起，检查标识“MY”、类型 exe_type、显示方式与所需页数和资源（出错码：$0B 头无效，$28 显示方式不符，$08 页数不够，$29 资源不足）；读段表到 $045D 起，逐段按类型调用 exe_load_type_ 例程；装完后将入口地址（exe_entry_low、exe_entry_high 减 1）压栈，用 BIOS 功能 1 切到程序页并经 RTS 进入程序，需要时先把 DOS 复制到页 0。出错时 A 为出错码、C=1。
exe_load_and_run:
  ldx #$04
  ldy #$3D
  stx fb_buffer_high
  sty fb_buffer_low
  ldx #$00
  ldy #$1A
  stx fb_bytes_high
  sty fb_bytes_low
  jsr exe_read_checked
  bcs exe_error_return
  lda exe_magic_0
  cmp #$4D
  bne exe_bad_header
  lda exe_magic_1
  cmp #$59
  bne exe_bad_header
  lda #$00
  sta exe_type_flag
  sta screen_initialized
  lda exe_type
  cmp #$01
  beq exe_check_mode
  cmp #$00
  beq exe_flag_decrement
  cmp #$03
  bne exe_bad_header
exe_flag_decrement:
  dec exe_type_flag
exe_check_mode:
  lda exe_flags
  and #$08
  beq exe_check_mode_10
  lda display_mode
  bmi exe_mode_mismatch
exe_check_mode_10:
  lda exe_flags
  and #$10
  beq exe_check_pages
  lda display_mode
  bmi exe_check_pages
exe_mode_mismatch:
  lda #$28
  sec
  rts
exe_check_pages:
  ldx free_page
  dex
  bit exe_type_flag
  bpl exe_compare_pages
  lda #$07
  jsr BIOS_ENTRY
  tax
  dex
exe_compare_pages:
  cpx exe_min_pages
  bcc exe_check_resource
  lda #$08
  rts
exe_bad_header:
  lda #$0B
  sec
exe_error_return:
  rts
exe_check_resource:
  lda #$0A
  jsr BIOS_ENTRY
  cmp exe_min_resource
  bcs exe_read_segment_table
  lda #$29
  sec
  rts
exe_read_segment_table:
  ldx #$04
  ldy #$5D
  stx fb_buffer_high
  sty fb_buffer_low
  ldx exe_table_pos_high
  ldy exe_table_pos_low
  stx fb_position_1
  sty fb_position_0
  lda #$00
  sta fb_bytes_high
  lda exe_segment_count
  asl a
  bcc exe_table_shift
  inc fb_bytes_high
exe_table_shift:
  asl a
  bcc exe_table_read
  inc fb_bytes_high
exe_table_read:
  sta fb_bytes_low
  jsr exe_read_checked
  bcs exe_error_return
  lda #$FF
  sta exe_seg10_index
  lda #$00
exe_segment_loop:
  pha
  asl a
  asl a
  tay
  lda exe_seg_type,y
  cmp #$0B
  beq exe_segment_0b_0c
  cmp #$0C
  bne exe_segment_check_0e
exe_segment_0b_0c:
  jsr exe_load_type_0b_0c
  jmp exe_segment_result
exe_segment_check_0e:
  cmp #$0E
  bne exe_segment_check_0d
  jsr exe_load_type_0e
  jmp exe_segment_result
exe_segment_check_0d:
  cmp #$0D
  bne exe_segment_check_02
  jsr exe_load_type_0d
  jmp exe_segment_result
exe_segment_check_02:
  cmp #$02
  bne exe_segment_check_04
  jsr exe_load_type_02
  jmp exe_segment_result
exe_segment_check_04:
  cmp #$04
  bne exe_segment_check_05
  jsr exe_load_type_04
  jmp exe_segment_result
exe_segment_check_05:
  bit exe_type_flag
  bpl exe_segment_next
  cmp #$05
  bne exe_segment_check_06
  jsr exe_load_type_05
  jmp exe_segment_result
exe_segment_check_06:
  cmp #$06
  bne exe_segment_check_10
  jsr exe_load_type_06
  jmp exe_segment_result
exe_segment_check_10:
  cmp #$10
  bne exe_segment_next
  sty exe_seg10_index
  beq exe_segment_next
exe_segment_result:
  bcs exe_segment_error
exe_segment_next:
  pla
  clc
  adc #$01
  cmp exe_segment_count
  bne exe_segment_loop
  bit exe_seg10_index
  bmi exe_launch
  ldy exe_seg10_index
  jsr exe_load_type_10
  bcc exe_launch
exe_segment_error:
  tax
  pla
  ldy screen_initialized
  bne exe_segment_loop
  txa
  rts
exe_launch:
  bit exe_type_flag
  bmi exe_launch_screen_mode
  ldx exe_entry_high
  ldy exe_entry_low
  bne exe_launch_entry_adjust
  dex
exe_launch_entry_adjust:
  dey
  txa
  pha
  tya
  pha
  ldx free_page
  bit exe_type_flag
  bpl exe_launch_switch_page
  ldx exe_start_page
exe_launch_switch_page:
  lda #$01
  jmp BIOS_ENTRY
exe_launch_screen_mode:
  ldx screen_initialized
  bne exe_launch_check_flag_20
  lda #$04
  ldx exe_flags
  jsr BIOS_ENTRY
  ldy #$00
  jsr display_mode_apply
  lda #$00
  sta ppu_mask
  sta ppu_control
exe_launch_check_flag_20:
  lda exe_flags
  and #$20
  bne exe_launch_check_flag_04
  jsr dos_copy_to_page_0
  ldx file_stub_addr_high
  ldy file_stub_addr_low
  stx far_ptr_high
  sty far_ptr_low
  ldy #$18
  lda #$00
  sta (far_ptr_low),y
exe_launch_check_flag_04:
  lda exe_type
  cmp #$03
  beq exe_launch_run
  lda exe_flags
  and #$04
  beq exe_launch_run
  lda #$00
  tax
exe_compact_loop:
  pha
  asl a
  asl a
  tay
  lda exe_seg_type,y
  cmp #$01
  beq exe_compact_copy
  cmp #$03
  beq exe_compact_copy
  cmp #$09
  bne exe_compact_next
exe_compact_copy:
  sta exe_seg_type,x
  lda exe_seg_offset_2,y
  sta exe_seg_offset_2,x
  lda exe_seg_offset_1,y
  sta exe_seg_offset_1,x
  lda exe_seg_offset_0,y
  sta exe_seg_offset_0,x
  inx
  inx
  inx
  inx
exe_compact_next:
  pla
  clc
  adc #$01
  cmp exe_segment_count
  bne exe_compact_loop
  lda #$04
  sta fb_buffer_high
  lda #$5D
  sta fb_buffer_low
  ldx #$04
  ldy #$00
  stx far_ptr_high
  sty far_ptr_low
exe_launch_run:
  lda #$08
  ldx #$04
  ldy #$3D
  jmp BIOS_ENTRY
launch_stub_template:
  .byte $A9, $01, $AE, $D9, $A7, $20, $00, $58, $6C, $FE, $00

; 装入并运行 .CMD 程序：先读 2 字节头到 cmd_header_low/high（装入地址，也是入口地址），再把正文读进 free_page 页的该地址处，超过 $C000 的部分另读一次（未确认），关闭文件后把 launch_stub_template 复制到零页 launch_stub 执行，经 BIOS 功能 1 切到该页再用 JMP (far_ptr) 进入程序。出错时 A 为出错码、C=1。
cmd_load_and_run:
  ldx #$61
  ldy #$30
  stx fb_buffer_high
  sty fb_buffer_low
  ldx #$00
  ldy #$02
  stx fb_bytes_high
  sty fb_bytes_low
  jsr exe_read_checked
  bcs cmd_load_done
  lda free_page
  sta fb_page
  ldx cmd_header_high
  ldy cmd_header_low
  stx far_ptr_high
  sty far_ptr_low
  cpx #$C0
  bcs cmd_load_rest
  stx fb_buffer_high
  sty fb_buffer_low
  lda #$00
  sec
  sbc far_ptr_low
  sta fb_bytes_low
  lda #$C0
  sbc far_ptr_high
  sta fb_bytes_high
  jsr exe_read_checked
  bcc cmd_load_full_page
  cmp #$1E
  clc
  bne cmd_close_file
  sec
cmd_load_done:
  rts
cmd_load_full_page:
  ldx #$C0
  ldy #$00
cmd_load_rest:
  lda #$00
  sec
  sbc fb_buffer_low
  sta fb_bytes_low
  lda #$FF
  sbc fb_buffer_high
  sta fb_bytes_high
  lda #$0B
  bcc cmd_load_failed
  lda #$02
  jsr file_call_with_buffer
cmd_close_file:
  php
  lda #$05
  jsr file_call_function
  plp
  bcs cmd_read_error
  bit critical_error_flags
  bmi cmd_install_stub
  asl critical_error_flags
cmd_install_stub:
  ldx #$0A
cmd_stub_copy_loop:
  lda launch_stub_template,x
  sta launch_stub,x
  dex
  bpl cmd_stub_copy_loop
  jmp a:launch_stub
cmd_read_error:
  lda #$1E
cmd_load_failed:
  sec
  rts
command_cmd_path:
  .byte $5C, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D
  .byte $44, $00
program_ext_table:
  .byte $43, $4D, $44, $45, $58, $43, $47, $41, $4D

; 把 far_ptr 指向的 11 字节名字（调用者页中，主名 8、扩展名 3，空格补齐）转成“主名.扩展名”的 0 结尾字符串放进 run_name_buffer（$0430）。出口：X:Y 为该字符串地址（$04:$30）。
run_name_format:
  ldy #$00
name_format_main_loop:
  jsr read_far_byte
  cmp #$20
  beq name_format_dot
  sta run_name_buffer,y
  iny
  cpy #$08
  bne name_format_main_loop
name_format_dot:
  lda #$2E
  sta run_name_buffer,y
  iny
  tya
  tax
  ldy #$08
name_format_ext_loop:
  jsr read_far_byte
  cmp #$20
  beq name_format_end
  sta run_name_buffer,x
  inx
  iny
  cpy #$0B
  bne name_format_ext_loop
name_format_end:
  lda #$00
  sta run_name_buffer,x
  ldx #$04
  ldy #$30
  rts
