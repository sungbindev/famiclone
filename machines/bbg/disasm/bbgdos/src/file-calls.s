; 模块：file-calls
; 路径解析与其余文件调用：查找文件、定位、删除、改名、属性、子目录

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2177"
; 段 0，$9D77-$A5BB
; 写回全部缓冲区，并把当前选定驱动器（mounted_drive）的当前目录路径（80 字节）存入 drive_state_table 里对应的状态块，块末标志置 $FF；写回失败或没有选定驱动器（$FF）时跳过保存。之后落入 buffers_invalidate。
drives_flush_and_save:
  jsr buffers_flush_all
  bcs buffers_invalidate
  lda mounted_drive
  cmp #$FF
  beq buffers_invalidate
  jsr drive_state_offset_of
  ldy #$00
save_dir_loop:
  lda current_dir_path,y
  sta drive_state_table,x
  inx
  iny
  cpy #$50
  bne save_dir_loop
  lda #$FF
  sta drive_state_table,x

; 把 FAT、目录、数据三个缓冲区都标成“没有内容”（扇区号置 $FF）并清除脏标志。出口：C=0。
buffers_invalidate:
  ldx #$FF
  stx fat_buffer_sector
  stx dir_buffer_lba_2
  stx data_buffer_lba_2
  inx
  stx data_buffer_dirty
  stx fat_buffer_dirty
  clc
  rts

; path_scan_limit 置 $FF（不限制路径长度），再落入 path_get_char。
path_get_char_unlimited:
  lda #$FF
  sta path_scan_limit

; 读路径字符串的第 X 个字符（页 path_page、地址 path_ptr 加 X，BIOS 功能 $20）。出口：A 为该字符；Z=1 表示字符串结束（字符为 0，或 X 已到 path_scan_limit）；X、Y 不变。
path_get_char:
  cpx path_scan_limit
  beq path_get_done
  stx scratch_f8
  sty scratch_f9
  lda path_ptr_high
  sta far_ptr_high
  lda path_ptr_low
  sta far_ptr_low
  lda #$20
  ldx path_page
  ldy scratch_f8
  jsr BIOS_ENTRY
  ldx scratch_f8
  ldy scratch_f9
  cmp #$00
path_get_done:
  rts

; 把通道块偏移 2-4（页、地址低、地址高）抄到 path_page、path_ptr_low、path_ptr_high，作为要解析的路径字符串位置。
path_load_pointer:
  ldy #$02
  lda (block_ptr_low),y
  sta path_page
  iny
  lda (block_ptr_low),y
  sta path_ptr_low
  iny
  lda (block_ptr_low),y
  sta path_ptr_high
  rts

; 解析路径：先装好路径位置；找出最后一个反斜杠，它之前的部分交给 path_walk 逐级进入目录（只有开头一个反斜杠时回根目录），剩下的文件名交给 path_parse_component 转成 11 字节名放进通道块偏移 5 起。路径末尾没有文件名时 A=3。出口：C=1 出错，A 为出错码。
path_resolve_directory:
  jsr path_load_pointer
  ldx #$00
  stx scratch_f3
resolve_scan_loop:
  jsr path_get_char_unlimited
  beq resolve_scan_done
  inx
  cmp #$5C
  bne resolve_scan_loop
  stx scratch_f3
  beq resolve_scan_loop
resolve_scan_done:
  ldx scratch_f3
  beq resolve_parse_name
  dex
  beq resolve_use_root
  jsr path_walk
  bcs resolve_done
  ldx scratch_f3
  bne resolve_parse_name
resolve_use_root:
  jsr dir_set_root
  ldx #$01
resolve_parse_name:
  jsr path_get_char_unlimited
  beq resolve_no_name
  jsr path_parse_component
resolve_done:
  rts
resolve_no_name:
  lda #$03
  rts

; 检查通道块偏移 5 起的 11 字节名字里没有通配符 ?（遇空格结束该字段）。出口：C=0 时 X 为名字的字符数加 1；有通配符时 A=5、C=1。
name_check_wildcards:
  ldy #$05
  ldx #$01
check_main_loop:
  lda (block_ptr_low),y
  cmp #$3F
  beq check_has_wildcard
  cmp #$20
  beq check_extension
  inx
  iny
  cpy #$0D
  bne check_main_loop
check_extension:
  ldy #$0D
  lda (block_ptr_low),y
  cmp #$20
  beq check_ok
  inx
check_ext_loop:
  lda (block_ptr_low),y
  cmp #$3F
  beq check_has_wildcard
  cmp #$20
  beq check_ok
  inx
  iny
  cpy #$10
  bne check_ext_loop
check_ok:
  clc
  rts
check_has_wildcard:
  lda #$05
  rts

; 文件调用 24（手册未列）：按通道块偏移 $16 的值改写目录项属性。Ember 实测对已打开的文件调用返回 A=$05（拒绝访问），用法未查清。
file_call_write_attribute:
  ldy #$16
  lda (block_ptr_low),y
  and #$D8
  bne write_attr_denied
  lda #$37
  ldy #$08
  jsr dir_find_entry
  bcs write_attr_done
  ldy #$0B
  lda (dir_entry_ptr_low),y
  and #$18
  ldy #$16
  ora (block_ptr_low),y
  ldy #$0B
  sta (dir_entry_ptr_low),y
  jsr dir_buffer_write
  bcs write_attr_done
  lda #$00
write_attr_done:
  rts
write_attr_denied:
  lda #$05
  rts

; 文件调用 22 FilePathFirst：按路径查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名，偏移 $16 是属性掩码（取 $A1 的补码与掩码合成）；查找状态记在偏移 $2B 起（页号、$9BF5 与当前目录簇）。
; 出口与 FileFirst 相同，之后用 FileNext 继续。
file_call_path_first:
  ldy #$16
  lda (block_ptr_low),y
  tax
  and #$10
  beq path_first_mask
  txa
  and #$F7
  tax
path_first_mask:
  txa
  ora #$A1
  eor #$FF
  jsr find_first_with_mask
  pha
  ldy #$2B
  lda dos_home_page
  sta (block_ptr_low),y
  iny
  lda #$F5
  sta (block_ptr_low),y
  iny
  lda #$9B
  sta (block_ptr_low),y
  lda #$00
  tax
  bit in_subdirectory
  bpl path_first_dir_cluster
  ldx current_dir_cluster_high
  lda current_dir_cluster_low
path_first_dir_cluster:
  ldy #$2E
  sta (block_ptr_low),y
  iny
  txa
  sta (block_ptr_low),y
  pla
  rts

; 文件调用 8 FileCreate、21 FilePathCreate：建立文件，建立后文件已打开、长度为 0。文件名含通配符、与目录或卷标同名时 A=$05。Ember 实测：建立后可写入并关闭，重新打开长度正确。
file_call_create:
  jsr name_check_wildcards
  bcs create_done
  lda #$3F
  ldy #$00
  jsr file_open_common
  bcs create_not_found
  lda dir_attr_filter
  and #$18
  beq create_new_entry
  lda #$05
  rts
create_new_entry:
  jsr file_truncate_chain
  bcs create_done
  lda #$20
  jsr dir_entry_init
  jsr dir_buffer_write
  bcs create_done
  lda #$00
create_done:
  rts
create_not_found:
  cmp #$02
  bne create_return
  ldx far_ptr_low
  inx
  bne create_claim_slot
  bit in_subdirectory
  bmi create_grow_dir
  lda #$05
create_return:
  rts
create_grow_dir:
  jsr dir_grow
  bcs create_return
create_claim_slot:
  jsr dir_claim_free_slot
  bcs create_return
  ldx #$20
  jsr dir_entry_init
  jsr dir_buffer_write
  bcs create_return
  jmp open_init_state

; 把查找结果区地址（调用者通道块地址加 $17）写进通道块偏移 3、4。
result_area_setup:
  lda caller_block_low
  clc
  adc #$17
  ldy #$03
  sta (block_ptr_low),y
  lda caller_block_high
  adc #$00
  iny
  sta (block_ptr_low),y
  rts

; 文件调用 6 FileFirst：查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名（'?' 通配），偏移 $16 是属性掩码（本入口先置为 $FF）。
; 出口：通道块偏移 3、4 指向偏移 $17 的查找结果区，其中是目录项副本；查找状态记在偏移 $0F-$15 与 $2B 起，供 FileNext 继续使用。
file_call_first:
  lda #$FF

; FileFirst 与 FilePathFirst 的公共部分：A 为属性掩码，写入通道块偏移 $16 与 dir_attr_filter，设好结果区，用 dir_scan_start 与 dir_match_next 找第一个匹配的目录项，找到后落入 find_report_entry 把结果填进通道块。
find_first_with_mask:
  ldy #$16
  sta (block_ptr_low),y
  sta dir_attr_filter
  pha
  jsr result_area_setup
  pla
  jsr dir_scan_start
  bcs find_first_done
  jsr dir_match_next
  bcc find_report_entry
find_first_done:
  rts

; 文件调用 7 FileNext：接着 FileFirst 的状态查找下一个匹配的文件；没有更多文件时返回出错码 $12（SearchEnding）。
file_call_next:
  ldy #$16
  lda (block_ptr_low),y
  sta dir_attr_filter
  jsr result_area_setup
  jsr drive_select
  jsr block_name_pointer
  ldy #$15
  lda (block_ptr_low),y
  sta dir_cluster_index
  dey
  lda (block_ptr_low),y
  sta dir_sectors_left
  dey
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  tax
  dey
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  clc
  adc #$10
  sta dir_entry_offset
  pla
  tay
  pla
  bcc find_next_search
  jsr dir_next_sector
  bcc find_next_search
  cmp #$02
  bne find_next_error
  lda #$12
find_next_error:
  sec
  rts
find_next_search:
  jsr dir_match_next
  bcs find_next_error
find_report_entry:
  ldy #$15
  lda dir_cluster_index
  sta (block_ptr_low),y
  dey
  lda dir_sectors_left
  sta (block_ptr_low),y
  dey
  lda dir_buffer_lba_2
  sta (block_ptr_low),y
  dey
  lda dir_buffer_lba_1
  sta (block_ptr_low),y
  dey
  lda dir_buffer_lba_0
  sta (block_ptr_low),y
  dey
  lda dir_entry_offset
  sta (block_ptr_low),y
  ldy #$0B
  lda (dir_entry_ptr_low),y
  pha
  ldy #$16
find_copy_attr_loop:
  lda (dir_entry_ptr_low),y
  pha
  iny
  cpy #$1A
  bne find_copy_attr_loop
  ldy #$1C
find_copy_times_loop:
  lda (dir_entry_ptr_low),y
  pha
  iny
  cpy #$20
  bne find_copy_times_loop
  ldy #$20
find_store_times_loop:
  dey
  pla
  sta (block_ptr_low),y
  cpy #$17
  bne find_store_times_loop
  ldy #$00
find_copy_name_loop:
  lda (dir_entry_ptr_low),y
  pha
  iny
  cpy #$0B
  bne find_copy_name_loop
  ldy #$2B
find_store_name_loop:
  dey
  pla
  sta (block_ptr_low),y
  cpy #$20
  bne find_store_name_loop
  lda #$00
  rts

; 文件调用 9 FileDriver：FH_DRIVE 为 0 时读取默认盘号（写回 FH_DRIVE）与逻辑驱动器数（偏移 2），否则设置默认盘号（1 为 A 盘）。Ember 实测读取返回盘号 1、驱动器数 2。
file_call_set_drive:
  ldy #$02
  lda logical_drive_count
  sta (block_ptr_low),y
  ldy #$01
  lda (block_ptr_low),y
  beq set_drive_report
  cmp logical_drive_count
  bcc set_drive_check_count
  bne set_drive_invalid
set_drive_check_count:
  cmp #$03
  bcs set_drive_store
  ldy drive_info_a
  beq set_drive_invalid
set_drive_store:
  sta default_disk
set_drive_report:
  lda default_disk
  sta (block_ptr_low),y
  ldy #$04
  lda last_probed_drive
  sta (block_ptr_low),y
  lda #$00
  rts
set_drive_invalid:
  lda #$0F
  rts

; 文件调用 4 FileSeek：把读写位置改为通道块偏移 7 起的 4 字节值。Ember 实测。
file_call_seek:
  ldx #$03
  ldy #$0D
fileseek_copy_loop:
  lda (block_ptr_low),y
  sta dir_entry_ptr_low,x
  iny
  dex
  bpl fileseek_copy_loop
  ldy #$07
  ldx #$03
  sec
fileseek_compare_loop:
  lda dir_entry_ptr_low,x
  sbc (block_ptr_low),y
  iny
  dex
  bpl fileseek_compare_loop
  bcs fileseek_done
  ldx #$03
  ldy #$07
fileseek_store_loop:
  lda dir_entry_ptr_low,x
  sta (block_ptr_low),y
  iny
  dex
  bpl fileseek_store_loop
fileseek_done:
  lda #$00
  rts

; 文件调用 11 DriveSize：按 FH_DRIVE 返回每簇扇区数（偏移 3）、每扇区字节数（偏移 5）、空闲簇数（偏移 7）、总簇数（偏移 9）。Ember 实测：1.44MB 系统盘每簇 4 个扇区、每扇区 512 字节、总簇数 715。
file_call_drive_size:
  ldx #$00
  ldy #$02
  stx name_ptr_high
  sty name_ptr_low
  stx dir_entry_ptr_high
  stx dir_entry_ptr_low
size_scan_loop:
  stx cluster_high
  sty cluster_low
  jsr fat_sector_for_cluster
  bcs size_done
  jsr fat_follow_chain
  bcs size_done
  ora cluster_low
  bne size_next_cluster
  inc dir_entry_ptr_low
  bne size_next_cluster
  inc dir_entry_ptr_high
size_next_cluster:
  inc name_ptr_low
  bne size_check_end
  inc name_ptr_high
size_check_end:
  ldx name_ptr_high
  ldy name_ptr_low
  cpx cluster_limit_high
  bne size_scan_loop
  cpy cluster_limit_low
  bne size_scan_loop
  ldy #$06
  lda #$02
  sta (block_ptr_low),y
  lda #$00
  dey
  sta (block_ptr_low),y
  ldy #$04
  sta (block_ptr_low),y
  lda sectors_per_cluster
  dey
  sta (block_ptr_low),y
  ldy #$09
  lda cluster_limit_low
  sec
  sbc #$02
  sta (block_ptr_low),y
  iny
  lda cluster_limit_high
  sbc #$00
  sta (block_ptr_low),y
  ldy #$08
  lda dir_entry_ptr_high
  sta (block_ptr_low),y
  dey
  lda dir_entry_ptr_low
  sta (block_ptr_low),y
  lda #$00
size_done:
  rts

; 文件调用 10 FileDelete、23 FilePathDelete：删除文件（目录项标成已删除并释放簇链），不检查只读属性。Ember 实测。
file_call_delete:
  jsr file_call_open
  bcs delete_done
  jsr file_truncate_chain
  bcs delete_done
  jsr fat_buffer_flush
  bcs delete_done
  ldy #$11
  lda (block_ptr_low),y
  jsr dir_entry_pointer
  lda #$E5
  ldy #$00
  sta (dir_entry_ptr_low),y
  jsr dir_buffer_write
  bcs delete_done
  lda #$00
delete_done:
  rts

; 文件调用 14 FileReadAbs：从绝对扇区号（通道块偏移 7，从 0 起）起读 FH_BYTES 个扇区到 FH_PAGE:FH_BUFFER。Ember 实测读 0 号扇区得到引导扇区。
file_call_read_absolute:
  lda #$02
  bne absolute_transfer

; 文件调用 15 FileWriteAbs：参数同 FileReadAbs，写扇区。Ember 实测把 0 号扇区原样写回。
file_call_write_absolute:
  lda #$03
absolute_transfer:
  sta transfer_command
  jsr drive_select
  ldy #$09
  lda (block_ptr_low),y
  sta sector_lba_2
  dey
  lda (block_ptr_low),y
  sta sector_lba_1
  dey
  lda (block_ptr_low),y
  sta sector_lba_0
  ldy #$05
  lda (block_ptr_low),y
  jsr disk_transfer_sectors
  bcs absolute_done
  lda #$00
absolute_done:
  rts

; 文件调用 13 FileRename：给已打开的文件改名，新文件名取自 FH_PAGE:FH_BUFFER 指向的 11 字节，并复制到通道块偏移 5。Ember 实测。
file_call_rename:
  jsr name_check_wildcards
  bcs rename_done
  jsr block_load_dir_entry
  bcs rename_done
  jsr block_name_pointer
  jsr dta_copy_name
  jsr dir_buffer_write
  bcs rename_done
  lda #$00
rename_done:
  rts

; 文件调用 12 FileAttribute：设置已打开文件的属性（偏移 2）、日期（偏移 3-4）、时间（偏移 5-6），关闭时写入目录项。Ember 实测。
file_call_attribute:
  jsr block_load_dir_entry
  bcs attribute_done
  ldy #$02
  lda (block_ptr_low),y
  sta scratch_f0
  and #$18
  bne attribute_denied
  ldy #$0B
  lda (dir_entry_ptr_low),y
  and #$18
  ora scratch_f0
  sta (dir_entry_ptr_low),y
  ldy #$03
  lda (block_ptr_low),y
  tax
  iny
  lda (block_ptr_low),y
  ldy #$19
  sta (dir_entry_ptr_low),y
  dey
  txa
  sta (dir_entry_ptr_low),y
  ldy #$06
  lda (block_ptr_low),y
  tax
  dey
  lda (block_ptr_low),y
  ldy #$16
  sta (dir_entry_ptr_low),y
  iny
  txa
  sta (dir_entry_ptr_low),y
  ldy #$19
  lda #$01
  sta (block_ptr_low),y
  lda #$00
attribute_done:
  rts
attribute_denied:
  lda #$05
  rts

; 把路径里从下标 X 起的一个分量转成 11 字节名字（主名 8、扩展名 3，空格补齐，* 展开成 ?），写进通道块偏移 5 起；分量为“.”或“..”时只写点号。出口：C=0 成功，X 指向该分量之后；字符非法或点号过多时 C=1、A=2。
path_parse_component:
  ldy #$05
  jsr path_get_char
  cmp #$2E
  bne component_main_loop
  sta (block_ptr_low),y
  iny
  inx
  jsr path_get_char
  beq component_pad_space
  cmp #$2E
  bne component_bad_dots
  sta (block_ptr_low),y
  iny
  inx
  jsr path_get_char
  beq component_pad_space
component_bad_dots:
  lda #$02
  sec
  rts
component_main_loop:
  inx
  cmp #$2A
  beq component_star
  cpy #$0D
  beq component_skip_to_ext
  jsr name_char_check
  bcs component_done
  sta (block_ptr_low),y
  iny
  jsr path_get_char
  beq component_pad_space
  cmp #$5C
  beq component_pad_space
  cmp #$2E
  bne component_main_loop
component_pad_space:
  lda #$20
  bne component_fill_main
component_star:
  lda #$3F
component_fill_main:
  cpy #$0D
  beq component_skip_to_ext
  sta (block_ptr_low),y
  iny
  jmp component_fill_main
component_skip_to_ext:
  jsr path_get_char
  beq component_ext_loop
  cmp #$5C
  beq component_ext_loop
  inx
  cmp #$2E
  bne component_skip_to_ext
component_ext_loop:
  jsr path_get_char
  beq component_ext_pad
  cmp #$5C
  beq component_ext_pad
  inx
  cmp #$2A
  beq component_ext_star
  cpy #$10
  beq component_skip_rest
  jsr name_char_check
  bcs component_done
  sta (block_ptr_low),y
  iny
  bne component_ext_loop
component_ext_star:
  lda #$3F
  bne component_fill_ext
component_ext_pad:
  lda #$20
component_fill_ext:
  cpy #$10
  beq component_skip_rest
  sta (block_ptr_low),y
  iny
  jmp component_fill_ext
component_skip_rest:
  jsr path_get_char
  beq component_ok
  cmp #$5C
  beq component_ok
  inx
  bne component_skip_rest
component_ok:
  clc
component_done:
  rts

; 把 dir_entry_ptr 指向的目录项名字（去掉空格，主名与扩展名之间加点）以反斜杠开头追加到 work_dir_path；总长不得超过 $3E，超过时恢复原长度并返回 C=1。
work_path_append:
  ldx #$FF
append_find_end:
  inx
  lda work_dir_path,x
  bne append_find_end
  stx scratch_f0
  lda #$5C
  sta work_dir_path,x
  inx
  cpx #$3E
  bcs append_overflow
  ldy #$00
append_name_loop:
  lda (dir_entry_ptr_low),y
  iny
  cmp #$20
  beq append_name_check
  sta work_dir_path,x
  inx
  cpx #$3E
  bcs append_overflow
append_name_check:
  cpy #$08
  bne append_name_loop
  lda (dir_entry_ptr_low),y
  cmp #$20
  beq append_ok
  lda #$2E
  sta work_dir_path,x
  inx
  cpx #$3E
  bcs append_overflow
append_ext_loop:
  lda (dir_entry_ptr_low),y
  iny
  cmp #$20
  beq append_ext_check
  sta work_dir_path,x
  inx
  cpx #$3E
  bcs append_overflow
append_ext_check:
  cpy #$0B
  bne append_ext_loop
append_ok:
  clc
  bcc append_terminate
append_overflow:
  ldx scratch_f0
append_terminate:
  lda #$00
  sta work_dir_path,x
  rts

; 回到上一级目录：读入当前子目录的第二项（“..”）取得上级目录首簇作为当前目录，并从 work_dir_path 去掉最后一级；上级为根目录（簇号 0）时转 dir_set_root。当前不在子目录里时 A=3、C=1。
dir_go_parent:
  bit in_subdirectory
  bpl parent_in_root
  jsr current_dir_to_lba
  jsr dir_buffer_read
  bcs parent_done
  ldx dir_dotdot_cluster_high
  ldy dir_dotdot_cluster_low
  txa
  ora dir_dotdot_cluster_low
  beq dir_set_root
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  ldx #$FF
parent_find_end:
  inx
  lda work_dir_path,x
  bne parent_find_end
parent_strip_loop:
  dex
  lda work_dir_path,x
  cmp #$5C
  bne parent_strip_loop
  lda #$00
  sta work_dir_path,x
  clc
  rts
parent_in_root:
  lda #$03
  sec
parent_done:
  rts

; 把当前目录设为根目录：in_subdirectory、current_dir_cluster 清零，work_dir_path 置空。
dir_set_root:
  lda #$00
  sta in_subdirectory
  sta work_dir_path
  sta current_dir_cluster_high
  sta current_dir_cluster_low
  rts

; 沿路径逐级进入目录。入口：路径位置已由 path_load_pointer 设好；X 为路径有效长度（存入 path_scan_limit），$FF 表示整个路径。以反斜杠开头从根目录起；“.”留在原处，“..”回上级，其余分量在当前目录里查找子目录并进入（同时更新 work_dir_path）。出口：C=0 成功；找不到目录时 A=3；分量非法时 A 同 path_parse_component。
path_walk:
  stx path_scan_limit
  ldx #$00
  jsr path_get_char
  beq walk_ok
  cmp #$5C
  bne walk_component
  jsr dir_set_root
  inx
walk_component:
  jsr path_get_char
  beq walk_ok
  cmp #$2E
  bne walk_name
  inx
  jsr path_get_char
  beq walk_ok
  cmp #$2E
  bne walk_next_separator
  inx
  stx scratch_f9
  jsr dir_go_parent
  bcc walk_after_dir
  rts
walk_name:
  jsr path_parse_component
  bcs walk_error_return
  stx scratch_f9
  lda #$10
  ldy #$08
  jsr dir_find_entry_with_mask
  bcc walk_enter_dir
  cmp #$02
  beq walk_not_found
  sec
walk_error_return:
  rts
walk_enter_dir:
  jsr work_path_append
  bcs walk_not_found
  ldy #$1B
  lda (dir_entry_ptr_low),y
  sta current_dir_cluster_high
  dey
  lda (dir_entry_ptr_low),y
  sta current_dir_cluster_low
  lda #$FF
  sta in_subdirectory
walk_after_dir:
  ldx scratch_f9
walk_next_separator:
  jsr path_get_char
  beq walk_ok
  cmp #$5C
  bne walk_not_found
  inx
  bne walk_component
walk_ok:
  clc
  rts
walk_not_found:
  lda #$03
  rts

; 把已确认的当前目录（committed_*、current_dir_path）恢复成工作状态：当前簇、是否在子目录、work_dir_path。
dir_restore_committed:
  ldx committed_dir_cluster_high
  ldy committed_dir_cluster_low
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  lda committed_in_subdirectory
  sta in_subdirectory
  ldx #$FF
restore_path_loop:
  inx
  lda current_dir_path,x
  sta work_dir_path,x
  bne restore_path_loop
  rts

; 把工作状态里的当前目录（簇号、是否在子目录、work_dir_path）保存为已确认的当前目录：committed_* 与 current_dir_path。
dir_commit_current:
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  stx committed_dir_cluster_high
  sty committed_dir_cluster_low
  lda in_subdirectory
  sta committed_in_subdirectory
  ldx #$FF
commit_path_loop:
  inx
  lda work_dir_path,x
  sta current_dir_path,x
  bne commit_path_loop
  rts

; 文件调用 16 FileChangeDir：改变当前目录，路径是以 0 结尾的字符串（\ 分隔，“..”回上一级），成功后更新 current_dir_path 与目录首簇；路径不存在 A=$03。Ember 实测。
file_call_change_dir:
  jsr path_load_pointer
  ldx #$FF
  jsr path_walk
  bcs change_dir_done
  jsr dir_commit_current
  lda #$00
change_dir_done:
  rts

; 文件调用 17 FileMakeDir：建立目录，路径是以 0 结尾的字符串。Ember 实测。
file_call_make_dir:
  jsr name_check_wildcards
  bcs make_dir_done
  stx scratch_f0
  ldx #$FF
make_dir_len_loop:
  inx
  lda work_dir_path,x
  bne make_dir_len_loop
  txa
  adc scratch_f0
  cmp #$3E
  bcs make_dir_denied
  lda #$3F
  ldy #$00
  jsr dir_find_entry
  bcs make_dir_not_found
make_dir_denied:
  lda #$05
  rts
make_dir_not_found:
  cmp #$02
  bne make_dir_done
  ldx far_ptr_low
  inx
  bne make_dir_find_cluster
  bit in_subdirectory
  bmi make_dir_grow
  lda #$52
  sta extended_error
  lda #$05
make_dir_done:
  rts
make_dir_grow:
  jsr dir_grow
  bcs make_dir_alloc_failed
make_dir_find_cluster:
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx cluster_high
  sty cluster_low
  jsr fat_find_free
  bcc make_dir_allocated
make_dir_alloc_failed:
  cmp #$27
  sec
  bne make_dir_done
  sta extended_error
  lda #$05
  rts
make_dir_allocated:
  jsr dir_claim_free_slot
  bcs make_dir_done
  ldx #$10
  jsr dir_entry_init
  ldy #$1B
  lda free_search_cluster_high
  sta (dir_entry_ptr_low),y
  tax
  dey
  lda free_search_cluster_low
  sta (dir_entry_ptr_low),y
  tay
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
  sta fat_access_mode
  jsr fat_entry_access
  bcs make_dir_done
  jsr dir_buffer_write
  bcs make_dir_done
  jsr buffers_flush_all
  bcs make_dir_done
  jsr data_buffer_clear
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx dot_entry_cluster_high
  sty boot_head_count
  jsr cluster_to_lba
  bcs make_dir_return
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  ldx #$0A
  lda #$20
make_dir_blank_loop:
  sta sector_buffer,x
  sta boot_total_sectors32_0,x
  dex
  bne make_dir_blank_loop
  lda #$2E
  sta sector_buffer
  sta boot_total_sectors32_0
  sta boot_total_sectors32_1
  ldx #$92
  ldy #$00
  stx dir_entry_ptr_high
  sty dir_entry_ptr_low
  lda #$10
  jsr dir_entry_set_timestamp
  lda #$20
  sta dir_entry_ptr_low
  lda #$10
  jsr dir_entry_set_timestamp
  bit in_subdirectory
  bpl make_dir_write_first
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  stx dotdot_entry_cluster_high
  sty dotdot_entry_cluster_low
make_dir_write_first:
  lda #$03
  jsr data_buffer_transfer
  bcs make_dir_return
  jsr data_buffer_clear
  ldx sectors_per_cluster
  stx scratch_f3
make_dir_clear_loop:
  dec scratch_f3
  bne make_dir_next_sector
  lda #$00
  rts
make_dir_next_sector:
  inc data_buffer_lba_0
  bne make_dir_write_next
  inc data_buffer_lba_1
  bne make_dir_write_next
  inc data_buffer_lba_2
make_dir_write_next:
  lda #$03
  jsr data_buffer_transfer
  bcc make_dir_clear_loop
make_dir_return:
  rts

; 检查簇 X:Y 的子目录是否为空（除“.”和“..”外没有有效目录项）。出口：A=0 为空；A=5 不空；其他为读盘出错码。结束时恢复原来的当前目录。
dir_check_empty:
  lda current_dir_cluster_high
  pha
  lda current_dir_cluster_low
  pha
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  lda #$00
  sta dir_cluster_index
  lda sectors_per_cluster
  sta dir_sectors_left
  lda #$40
  sta dir_entry_offset
  dec in_subdirectory
  jsr cluster_to_lba
  bcs empty_restore
empty_next_sector:
  jsr dir_buffer_read
  bcs empty_restore
  ldy dir_entry_offset
empty_scan_loop:
  lda dir_buffer,y
  beq empty_restore
  cmp #$E5
  bne empty_not_empty
  tya
  adc #$1F
  tay
  bne empty_scan_loop
empty_scan_high_loop:
  lda dir_buffer_high,y
  beq empty_restore
  cmp #$E5
  bne empty_not_empty
  tya
  adc #$1F
  tay
  bne empty_scan_high_loop
  sty dir_entry_offset
  jsr dir_next_sector_current
  bcc empty_next_sector
  cmp #$02
  beq empty_restore
  lda #$00
  beq empty_restore
empty_not_empty:
  lda #$05
empty_restore:
  sta scratch_f0
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  pla
  sta current_dir_cluster_low
  pla
  sta current_dir_cluster_high
  inc in_subdirectory
  lda scratch_f0
  rts

; 文件调用 18 FileRemoveDir：删除目录，路径不存在 A=$03，删除当前目录 A=$10（后者只依据手册）。Ember 实测删除空目录成功。
file_call_remove_dir:
  lda #$10
  ldy #$08
  jsr dir_find_entry
  bcc remove_dir_found
  cmp #$02
  bne remove_dir_done
  lda #$03
  rts
remove_dir_found:
  jsr block_store_dir_position
  ldy #$1B
  lda (dir_entry_ptr_low),y
  tax
  dey
  lda (dir_entry_ptr_low),y
  tay
  cpx committed_dir_cluster_high
  bne remove_dir_not_current
  cpy committed_dir_cluster_low
  bne remove_dir_not_current
  lda #$10
  rts
remove_dir_not_current:
  jsr dir_check_empty
  cmp #$00
  bne remove_dir_done
  lda #$FF
  sta fat_access_mode
remove_dir_free_loop:
  lda #$00
  sta fat_value_low
  sta fat_value_high
  jsr fat_entry_access
  bcs remove_dir_done
  jsr fat_value_is_last
  bcc remove_dir_free_loop
  jsr fat_buffer_flush
  bcs remove_dir_done
  jsr block_load_dir_entry
  bcs remove_dir_done
  ldy #$00
  lda #$E5
  sta (dir_entry_ptr_low),y
  jsr dir_buffer_write
  bcs remove_dir_done
  lda #$00
remove_dir_done:
  rts

; 文件调用 19 FileGetDir：取当前目录：出口 FH_PAGE 为 dos_page、FH_BUFFER 为 current_dir_path（DOS 页里以 0 结尾的路径串，根目录为空串）。Ember 实测。
file_call_get_dir:
  ldy #$02
  lda dos_home_page
  sta (block_ptr_low),y
  ldy #$04
  lda #$80
  sta (block_ptr_low),y
  dey
  lda #$32
  sta (block_ptr_low),y
  lda #$00
  rts
dos_home_page:
  .byte $00
file_stub_addr_low:
  .byte $00
file_stub_addr_high:
  .byte $00
current_page:
  .byte $00
ctrl_check_enabled:
  .byte $00
result_code:
  .byte $00
result_flags:
  .byte $00, $00, $00
saved_x:
  .byte $00
display_mode:
  .byte $00
pending_function_key:
  .byte $00
screen_initialized:
  .byte $00
exe_type_flag:
  .byte $00
run_stack_top:
  .byte $00
run_stack:
  .res 40, $00
line_buffer:
  .byte $0D
  .res 60, $00
line_buffer_end:
  .byte $00
name_buffer:
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
