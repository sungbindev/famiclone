; 模块：fat-directory
; FAT12 与目录：簇与扇区换算、FAT 缓存、目录项查找

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_07A8"
; 段 0，$83A8-$8A14
; 选定 current_drive 指定的驱动器（drive_select），并在换盘后重新确认当前目录：软盘且 drive_path_stale 非 0 时先执行 BIOS 磁盘命令 6（参数在 dio_sector，含义未确认），再用 path_walk 沿 current_dir_path 回到该目录，路径不存在（A=3）就退回根目录；否则直接恢复已保存的当前目录（dir_restore_committed）。出口：选定失败时 A=3、C=1；其余出错时 A 为出错码。
drive_activate:
  jsr drive_select
  bcs drive_activate_error
  ldx current_drive
  bmi drive_activate_restore_dir
  lda drive_path_stale,x
  beq drive_activate_restore_dir
  inc drive_path_stale,x
  lda sectors_per_track
  sta dio_sector
  lda #$06
  jsr bios_disk_command
  lda current_dir_path
  beq drive_activate_root
  lda dos_page
  ldx #$80
  ldy #$32
  sta path_page
  stx path_ptr_high
  sty path_ptr_low
  ldx #$FF
  jsr path_walk
  bcc drive_activate_commit_dir
  cmp #$03
  bne drive_activate_fail
drive_activate_root:
  jsr dir_set_root
drive_activate_commit_dir:
  jsr dir_commit_current
  clc
  rts
drive_activate_restore_dir:
  jsr dir_restore_committed
  clc
  rts
drive_activate_fail:
  sec
drive_activate_error:
  lda #$03
  rts

; 算出文件通道块里文件名字段（偏移 5）的地址，放入 $F4:$F5（通道块地址在 $EE:$EF）。
block_name_pointer:
  lda #$05
  clc
  adc block_ptr_low
  sta name_ptr_low
  lda #$00
  adc block_ptr_high
  sta name_ptr_high
  rts

; 目录项指针：dir_buffer（$9400）加 A 乘 2，放入 $F1:$F2。A 是目录项在扇区里的字节偏移除以 2（即通道块里的 FH_DIR_ENTRY）。
dir_entry_pointer:
  asl a
  sta dir_entry_ptr_low
  lda #$94
  adc #$00
  sta dir_entry_ptr_high
  rts

; 把 dir_buffer 写回磁盘上 dir_buffer_lba_0-2 指定的扇区。
dir_buffer_write:
  lda #$03
  pha
  lda dir_buffer_lba_2
  ldx dir_buffer_lba_1
  ldy dir_buffer_lba_0
  jmp dir_buffer_transfer

; 把逻辑扇区 A:X:Y（Y 为低字节）读进目录缓冲区 dir_buffer（$9400）：已缓冲该扇区时直接返回 C=0；读失败时清除缓冲记录。
dir_buffer_read:
  cmp dir_buffer_lba_2
  bne dir_read_set_lba_2
  cpx dir_buffer_lba_1
  bne dir_read_set_lba_1
  cpy dir_buffer_lba_0
  bne dir_read_set_lba_0
  clc
  rts
dir_read_set_lba_2:
  sta dir_buffer_lba_2
dir_read_set_lba_1:
  stx dir_buffer_lba_1
dir_read_set_lba_0:
  sty dir_buffer_lba_0
  lda #$02
  pha
  lda dir_buffer_lba_2
dir_buffer_transfer:
  sty dio_cylinder
  pha
  txa
  tay
  pla
  tax
  lda #$94
  sta dio_addr_high
  lda #$00
  sta dio_addr_low
  pla
  jsr disk_transfer_sector
  bcc dir_buffer_done
  ldy #$FF
  sty dir_buffer_lba_2
dir_buffer_done:
  rts

; 开始扫描目录。入口：A 的位 3=1 且当前在子目录里时从子目录首簇起，否则从根目录起。出口：A:X:Y 为起始逻辑扇区号（Y 低），$FB 为该段的扇区数，$FA 清零。
dir_scan_start:
  pha
  jsr block_name_pointer
  lda #$00
  sta dir_cluster_index
  sta dir_entry_offset
  lda sectors_per_cluster
  sta dir_sectors_left
  pla
  and #$08
  beq dir_scan_root
  bit in_subdirectory
  bmi current_dir_to_lba
dir_scan_root:
  lda root_dir_sectors
  sta dir_sectors_left
  lda #$00
  ldx root_dir_lba_high
  ldy root_dir_lba_low
  clc
  rts

; 把当前子目录的首簇（current_dir_cluster_low/high）换成逻辑扇区号，参数与结果同 cluster_to_lba。
current_dir_to_lba:
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low

; 簇号 X:Y 换成逻辑扇区号（存入 sector_lba_0-2 并返回 A:X:Y）：簇号减 2，乘 sectors_per_cluster，加 data_area_lba；簇号超出 cluster_limit 时 A=$1B（扇区未找到）。
cluster_to_lba:
  tya
  sec
  sbc #$02
  sta dir_entry_ptr_low
  tay
  txa
  sbc #$00
  sta dir_entry_ptr_high
  cmp cluster_limit_high
  beq lba_check_low
  bcc lba_scale_clusters
lba_check_low:
  cpy cluster_limit_low
  bcc lba_scale_clusters
  lda #$1B
  rts
lba_scale_clusters:
  lda #$00
  sta scratch_f0
  lda sectors_per_cluster
lba_scale_loop:
  lsr a
  beq lba_add_data_area
  asl dir_entry_ptr_low
  rol dir_entry_ptr_high
  rol scratch_f0
  bcc lba_scale_loop
lba_add_data_area:
  lda dir_entry_ptr_low
  clc
  adc data_area_lba_low
  tay
  sta sector_lba_0
  lda dir_entry_ptr_high
  adc data_area_lba_high
  sta sector_lba_1
  tax
  lda scratch_f0
  adc #$00
  sta sector_lba_2
  rts

; 在当前目录里查找与文件通道块里文件名相同的目录项；入口 A、Y 是两个属性过滤值（data_8023、data_8024，具体用法未确认）。找到 C=0，$FA 为目录项偏移；找不到 C=1、A=$02；查找时会记下第一个空闲项（$FE）。
dir_find_entry:
  ldx #$FF
  stx far_ptr_low

; dir_find_entry 的入口之二（不清除空闲项记录 $FE）：A、Y 存入 dir_attr_filter、dir_attr_forbidden，从当前目录的起始扇区起逐扇区读入 dir_buffer，用 dir_compare_entries 比较名字。出口同 dir_find_entry。
dir_find_entry_with_mask:
  sta dir_attr_filter
  sty dir_attr_forbidden
  lda #$08
  jsr dir_scan_start
  bcs dir_find_done
dir_find_loop:
  jsr dir_buffer_read
  bcs dir_find_done
  jsr dir_compare_entries
  bcc dir_find_done
  bne dir_find_not_found
  jsr dir_next_sector_current
  bcc dir_find_loop
  rts
dir_find_not_found:
  lda #$02
  sec
dir_find_done:
  rts

; 以 dir_buffer_lba_0-2 记录的当前目录扇区为起点取下一个目录扇区，转 dir_next_sector。
dir_next_sector_current:
  lda dir_buffer_lba_2
  ldx dir_buffer_lba_1
  ldy dir_buffer_lba_0

; 下一个目录扇区：逻辑扇区号 A:X:Y 加 1；当前簇的扇区用完时（$FB 减到 0）在子目录里沿 FAT 链转到下一簇，根目录走完则 A=$02、C=1。
dir_next_sector:
  clc
  iny
  bne dir_next_check_chunk
  inx
  bne dir_next_check_chunk
  adc #$01
dir_next_check_chunk:
  dec dir_sectors_left
  bne dir_next_done
  bit in_subdirectory
  bpl dir_next_end
  lda current_dir_cluster_high
  ldy current_dir_cluster_low
  sta cluster_high
  sty cluster_low
  ldx #$00
dir_next_chain_loop:
  stx scratch_f8
  tax
  ldy cluster_low
  jsr fat_sector_for_cluster
  bcs dir_next_done
  jsr fat_follow_chain
  bcs dir_next_done
  ldx scratch_f8
  cpx dir_cluster_index
  inx
  bcc dir_next_chain_loop
  stx dir_cluster_index
  jsr fat_cluster_is_last
  bcs dir_next_end
  lda sectors_per_cluster
  sta dir_sectors_left
  jmp cluster_to_lba
dir_next_end:
  lda #$02
  sec
dir_next_done:
  rts

; 在 dir_buffer 里逐项比较：目录项第 0 字节为 0 表示目录到此为止（记下空闲项），为 $E5 的是已删除项（同样记下），名字第 0 字节 $05 按 $E5 比较；11 字节全部相同才匹配，匹配后把属性按位与 data_8023、data_8024 比较（未确认），相符返回 C=0，否则 A=$05、C=1。
dir_compare_entries:
  jsr dir_entry_pointer
  ldy #$00
  lda (dir_entry_ptr_low),y
  beq dir_note_free_slot
  cmp #$E5
  beq dir_compare_deleted
  cmp #$05
  bne dir_compare_name_loop
  lda #$E5
dir_compare_name_loop:
  cmp (name_ptr_low),y
  bne dir_compare_advance
  iny
  lda (dir_entry_ptr_low),y
  cpy #$0B
  bne dir_compare_name_loop
  lda (dir_entry_ptr_low),y
  ora #$20
  bit dir_attr_filter
  beq dir_compare_attr_mismatch
  bit dir_attr_forbidden
  bne dir_compare_attr_mismatch
  sta dir_attr_filter
dir_compare_match:
  clc
  rts
dir_compare_deleted:
  jsr dir_note_free_slot
dir_compare_advance:
  lda dir_entry_offset
  clc
  adc #$10
  sta dir_entry_offset
  bne dir_compare_entries
  rts
dir_compare_attr_mismatch:
  lda #$05
  sec
  rts

; 记下第一个空闲目录项：$FE 为 $FF（尚未记录）时，把当前目录项偏移 $FA 与目录缓冲区的扇区号 dir_buffer_lba_0-2 存到 $FE 与 free_slot_lba_0-2。
dir_note_free_slot:
  ldx far_ptr_low
  inx
  bne free_slot_done
  lda dir_entry_offset
  sta far_ptr_low
  ldx #$02
free_slot_copy_loop:
  lda dir_buffer_lba_0,x
  sta free_slot_lba_0,x
  dex
  bpl free_slot_copy_loop
free_slot_done:
  sec
  rts

; FAT 缓冲区有修改（fat_buffer_dirty 非 0）时，把 fat_buffer 写回第一份与第二份 FAT。
fat_buffer_flush:
  lda fat_buffer_dirty
  beq dir_compare_match
  inc fat_buffer_dirty
  ldx #$96
  ldy #$00
  stx dio_addr_high
  sty dio_addr_low
  ldx #$00
  lda fat_buffer_sector
  sta dio_cylinder
  lda #$03
  jsr disk_transfer_sector
  bcs free_slot_done
  ldx #$00
  ldy #$00
  lda fat_buffer_sector
  sec
  sbc fat1_start_sector
  clc
  adc fat2_start_sector
  sta dio_cylinder
  bcc fat_flush_second_copy
  iny
fat_flush_second_copy:
  lda #$03
  jmp disk_transfer_sector

; 写回全部缓冲区：先 FAT 缓冲区（fat_buffer_flush），再数据缓冲区（data_buffer_flush）。出口：C=1 出错。
buffers_flush_all:
  jsr fat_buffer_flush
  bcs data_flush_done

; 数据缓冲区有修改（data_buffer_dirty 为负）时把 sector_buffer 写回磁盘。
data_buffer_flush:
  bit data_buffer_dirty
  bpl dir_compare_match
  inc data_buffer_dirty
  pha
  txa
  pha
  tya
  pha
  lda #$03
  jsr data_buffer_transfer
  pla
  tay
  pla
  tax
  pla
  bcc data_flush_done
  lda disk_last_error
data_flush_done:
  rts

; 把 FAT 的第 A 个扇区读进 fat_buffer（$9600）：先把旧内容写回，再读入，并记在 fat_buffer_sector（读失败时为 $FF）。
fat_buffer_load:
  pha
  jsr fat_buffer_flush
  pla
  bcs fat_load_failed
  ldx #$96
  ldy #$00
  stx dio_addr_high
  sty dio_addr_low
  sta fat_buffer_sector
  sta dio_cylinder
  ldx #$00
  lda #$02
  jsr disk_transfer_sector
  bcc fat_load_done
fat_load_failed:
  ldy #$FF
  sty fat_buffer_sector
fat_load_done:
  rts

; FAT12 的 FAT 扇区序号：X:Y 与 cluster_low/high 都是簇号，把 X:Y 变为簇号乘 3，A 为它除以 1024 的结果（即簇号乘 1.5 除以 512，FAT 内的扇区序号）。
fat12_sector_index:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc cluster_low
  tay
  txa
  adc cluster_high
  tax
  lsr a
  lsr a
  rts

; 把 cluster_low/high 指定簇所在的 FAT 扇区读进 fat_buffer（fat_sector_for_cluster）；出错或该扇区已在缓冲区里（A=0）时直接返回，否则继续执行 block_store_chain_position。Y 暂存在 scratch_f0。
fat_load_cluster_sector:
  ldx cluster_high
  ldy cluster_low
  jsr fat_sector_for_cluster
  bcs fat_load_cluster_done
  beq fat_load_cluster_done
  sty scratch_f0

; 把当前簇链位置记入文件通道块：偏移 $15-$16 存 name_ptr（簇序号），偏移 $17-$18 存 cluster_low/high（簇号），供下次从该处继续查找簇链（见 file_seek_cluster）。出口：Y 取回 scratch_f0。
block_store_chain_position:
  ldy #$15
  lda name_ptr_low
  sta (block_ptr_low),y
  iny
  lda name_ptr_high
  sta (block_ptr_low),y
  ldy #$17
  lda cluster_low
  sta (block_ptr_low),y
  iny
  lda cluster_high
  sta (block_ptr_low),y
  ldy scratch_f0
fat_load_cluster_done:
  rts

; cluster_low/high 里的簇号是否为链结束标记，转 fat_xy_is_last。C=1 表示结束。
fat_cluster_is_last:
  ldx cluster_high
  ldy cluster_low
  jmp fat_xy_is_last

; FAT 项的值是否表示链结束（不小于 $FF0 或 FAT16 的 $FFF0）：C=1 为结束；fat_xy_is_last 用 X:Y 作为值。
fat_value_is_last:
  ldx fat_value_high
  ldy fat_value_low

; X:Y 是否为 FAT 链结束值（高字节不小于 fat16_flag，且低字节不小于 $F0，即 FAT12 的 $FF0 以上或 FAT16 的 $FFF0 以上）。C=1 表示结束。
fat_xy_is_last:
  cpx fat16_flag
  bcc fat_last_done
  cpy #$F0
fat_last_done:
  rts

; 把簇 X:Y 的 FAT 项所在的 FAT 扇区读进 fat_buffer；重新读过返回 A=1，已在缓冲区里返回 A=0。
fat_sector_for_cluster:
  txa
  bit fat16_flag
  bmi fat_sector_compute
  jsr fat12_sector_index
fat_sector_compute:
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  clc
  beq fat_sector_done
  stx scratch_f0
  sty scratch_f3
  jsr fat_buffer_load
  ldx scratch_f0
  ldy scratch_f3
  bcs fat_sector_done
  lda #$01
fat_sector_done:
  rts
fat12_high_byte_keep:
  .byte $F0
fat12_low_byte_keep:
  .byte $00, $0F
fat_odd_flag:
  .byte $00
fat_value_low:
  .byte $00
fat_value_high:
  .byte $00

; 把簇 chain_cluster 的 FAT 项写成链结束标记（具体值由 fat_entry_access 按 FAT12 或 FAT16 处理）。
fat_write_entry:
  lda #$FF
  sta fat_access_mode
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
  ldx chain_cluster_high
  ldy chain_cluster_low

; 读写 FAT 项。入口：簇号 X:Y；$FC 位 7=0 读出，位 7=1 写入（新值在 fat_value_low、fat_value_high，原值换到这里）。簇号为 0 表示文件的首簇，它不在 FAT 里，存在文件通道块偏移 $0B、$0C。FAT12 按 12 位拆包（半字节 fat_odd_flag），FAT16 直接取 2 字节。出口：读出的值在 fat_value_low、fat_value_high。
fat_entry_access:
  stx name_ptr_high
  sty name_ptr_low
  txa
  ora name_ptr_low
  bne fat_entry_normal
  bit fat_access_mode
  bpl fat_entry_first_read
  jsr fat_value_is_last
  bcc fat_entry_first_swap
  lda #$00
  sta fat_value_low
  sta fat_value_high
fat_entry_first_swap:
  ldy #$0B
  lda (block_ptr_low),y
  tax
  lda fat_value_low
  sta (block_ptr_low),y
  stx fat_value_low
  iny
  lda (block_ptr_low),y
  tax
  lda fat_value_high
  sta (block_ptr_low),y
  stx fat_value_high
  ldy #$19
  lda #$01
  sta (block_ptr_low),y
  bne fat_entry_first_done
fat_entry_first_read:
  ldy #$0B
  lda (block_ptr_low),y
  sta fat_value_low
  iny
  lda (block_ptr_low),y
  sta fat_value_high
fat_entry_first_done:
  lda fat_value_low
  ora fat_value_high
  bne fat_entry_ok
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
fat_entry_ok:
  clc
  rts
fat_entry_normal:
  bit fat16_flag
  bpl fat12_entry
  txa
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  beq fat16_entry_have_sector
  jsr fat_buffer_load
  bcs fat16_entry_done
fat16_entry_have_sector:
  ldx #$96
  ldy #$00
  bit name_ptr_low
  bpl fat16_entry_pointer
  inx
fat16_entry_pointer:
  lda name_ptr_low
  stx name_ptr_high
  sty name_ptr_low
  asl a
  tay
  bit fat_access_mode
  bpl fat16_entry_read
  lda (name_ptr_low),y
  tax
  lda fat_value_low
  sta (name_ptr_low),y
  stx fat_value_low
  iny
  lda (name_ptr_low),y
  tax
  lda fat_value_high
  sta (name_ptr_low),y
  stx fat_value_high
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (block_ptr_low),y
  clc
  rts
fat16_entry_read:
  lda (name_ptr_low),y
  sta fat_value_low
  iny
  lda (name_ptr_low),y
  sta fat_value_high
  clc
fat16_entry_done:
  rts
fat12_entry:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc name_ptr_low
  sta scratch_f0
  txa
  adc name_ptr_high
  lsr a
  sta scratch_f3
  ror scratch_f0
  lsr a
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  beq fat12_entry_have_sector
  jsr fat_buffer_load
  bcs fat12_entry_fail
fat12_entry_have_sector:
  lda name_ptr_low
  and #$01
  sta fat_odd_flag
  beq fat12_entry_pointer
  ldx #$04
fat12_shift_new_loop:
  asl fat_value_low
  rol fat_value_high
  dex
  bne fat12_shift_new_loop
fat12_entry_pointer:
  ldx #$96
  ldy #$00
  stx name_ptr_high
  sty name_ptr_low
  lda scratch_f3
  ldy scratch_f0
  lsr a
  bcc fat12_read_low_byte
  inc name_ptr_high
fat12_read_low_byte:
  lda (name_ptr_low),y
  bit fat_access_mode
  bpl fat12_store_low
  pha
  ldx fat_odd_flag
  and fat12_low_byte_keep,x
  ora fat_value_low
  sta (name_ptr_low),y
  tya
  tax
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (block_ptr_low),y
  txa
  tay
  pla
fat12_store_low:
  sta fat_value_low
  iny
  bne fat12_read_high_byte
  lda #$96
  cmp name_ptr_high
  beq fat12_page_boundary
  dec name_ptr_high
  ldx fat_buffer_sector
  inx
  txa
  jsr fat_buffer_load
  ldy #$00
  bcc fat12_read_high_byte
fat12_entry_fail:
  rts
fat12_page_boundary:
  inc name_ptr_high
fat12_read_high_byte:
  lda (name_ptr_low),y
  bit fat_access_mode
  bpl fat12_store_high
  pha
  ldx fat_odd_flag
  and fat12_high_byte_keep,x
  ora fat_value_high
  sta (name_ptr_low),y
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (block_ptr_low),y
  pla
fat12_store_high:
  sta fat_value_high
  lda fat_odd_flag
  beq fat12_even_mask
  ldx #$04
fat12_shift_old_loop:
  lsr fat_value_high
  ror fat_value_low
  dex
  bne fat12_shift_old_loop
  clc
  rts
fat12_even_mask:
  lda fat_value_high
  and #$0F
  sta fat_value_high
  clc
  rts

; 查找空闲簇的起点 free_search_cluster 前进一簇，到 cluster_limit 回绕到 2；回到起点（$F6:$F7）时 Z=1。
fat_advance_search:
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  iny
  bne search_no_carry
  inx
search_no_carry:
  cpx cluster_limit_high
  bne search_store
  cpy cluster_limit_low
  bne search_store
  ldx #$00
  ldy #$02
search_store:
  stx free_search_cluster_high
  sty free_search_cluster_low
  cpx cluster_high
  bne search_done
  cpy cluster_low
search_done:
  rts

; 从 free_search_cluster 起顺序找空闲簇（FAT 项为 0）；整张磁盘没有空闲簇时 A=$27（磁盘已满）、C=1。
fat_find_free:
  lda #$00
  sta fat_access_mode
  ldx free_search_cluster_high
  ldy free_search_cluster_low
find_free_loop:
  jsr fat_entry_access
  bcs find_free_done
  lda fat_value_low
  ora fat_value_high
  beq find_free_done
  jsr fat_advance_search
  bne find_free_loop
  lda #$27
  sec
find_free_done:
  rts
chain_cluster_low:
  .byte $00
chain_cluster_high:
  .byte $00

; 分配一个空闲簇：用 fat_find_free 找到后，把前一簇（chain_cluster）的链接改成它，并把它标成链结束。
fat_allocate_cluster:
  jsr fat_find_free
  bcs allocate_done
  lda #$FF
  sta fat_access_mode
  ldx chain_cluster_high
  ldy chain_cluster_low
  lda free_search_cluster_high
  sta chain_cluster_high
  sta fat_value_high
  lda free_search_cluster_low
  sta chain_cluster_low
  sta fat_value_low
  jsr fat_entry_access
allocate_done:
  rts

; fat_read_value 的入口，A=0：允许跨 FAT 缓冲区边界再读下一个 FAT 扇区。取 cluster_low/high 指定簇的下一簇号，结果放回 cluster_low/high。
fat_follow_chain:
  lda #$00

; 从 fat_buffer 取出 FAT 项并放进 cluster_low/high：入口 Y 为项在缓冲区里的偏移（fat_sector_for_cluster 的返回值），FAT12 先解包 12 位，FAT16 取 2 字节并按 fat16_flag 屏蔽高字节。A 位 7 为 1 时不再读下一个 FAT 扇区。出口：C=1 为读盘出错。
fat_read_value:
  sta scratch_f0
  bit fat16_flag
  bpl fat_read_12bit
  lsr cluster_high
  tya
  asl a
  tay
  jmp fat_read_have_index
fat_read_12bit:
  jsr fat12_unpack_index
fat_read_have_index:
  bcs fat_read_second_half
  lda fat_buffer,y
  sta cluster_low
  iny
  beq fat_read_high_buffer
  lda fat_buffer,y
  bcc fat_read_unpack
fat_read_second_half:
  lda fat_buffer_high,y
  sta cluster_low
  iny
  bne fat_read_high_buffer
  bit scratch_f0
  bmi fat_read_done
  ldx fat_buffer_sector
  inx
  txa
  jsr fat_buffer_load
  bcs fat_read_done
  inc scratch_f0
  lda fat_buffer
  bcc fat_read_unpack
fat_read_high_buffer:
  lda fat_buffer_high,y
fat_read_unpack:
  bit cluster_high
  bpl fat_read_mask
  lsr a
  ror cluster_low
  lsr a
  ror cluster_low
  lsr a
  ror cluster_low
  lsr a
  ror cluster_low
fat_read_mask:
  and fat16_flag
  sta cluster_high
  clc
fat_read_done:
  rts

; FAT12 取项前的下标换算：X:Y 右移 1 位，被移出的位（簇号奇偶）存入 cluster_high 的位 7，A 为 X 再右移一位的结果。
fat12_unpack_index:
  txa
  lsr a
  tax
  tya
  ror a
  tay
  ror cluster_high
  txa
  lsr a
  rts

; 定位文件读写位置所在的簇。入口：通道块偏移 7-10 为读写位置；A 非 0 时先对 cluster_low/high 指定的簇调用 fat_load_cluster_sector。处理：读写位置除以簇的字节数得簇序号（放 $F1-$F3），从通道块缓存的位置（偏移 $15-$18）或首簇（偏移 $0B-$0C）出发沿簇链前进到该序号，并记下位置（block_store_chain_position）。出口：找到时 C=0，cluster_low/high 为簇号，sector_lba_0-2 为簇内对应扇区，X 为扇区在簇内的序号；链结束仍未到达时 A=0、C=1；出错 A 为出错码、C=1。
file_seek_cluster:
  pha
  ldy #$0A
  lda (block_ptr_low),y
  sta scratch_f3
  dey
  lda (block_ptr_low),y
  sta dir_entry_ptr_high
  dey
  lda (block_ptr_low),y
  sta dir_entry_ptr_low
  lda sectors_per_cluster
chain_seek_shift_loop:
  lsr scratch_f3
  ror dir_entry_ptr_high
  ror dir_entry_ptr_low
  lsr a
  bne chain_seek_shift_loop
  lda dir_entry_ptr_low
  ldy #$15
  cmp (block_ptr_low),y
  iny
  lda dir_entry_ptr_high
  sbc (block_ptr_low),y
  bcs chain_seek_from_cached
  lda #$00
  sta name_ptr_high
  sta name_ptr_low
  ldy #$0B
  bne chain_seek_load_start
chain_seek_from_cached:
  lda (block_ptr_low),y
  sta name_ptr_high
  dey
  lda (block_ptr_low),y
  sta name_ptr_low
  ldy #$17
chain_seek_load_start:
  lda (block_ptr_low),y
  sta cluster_low
  iny
  lda (block_ptr_low),y
  sta cluster_high
  pla
  beq chain_seek_walk_check
  jsr fat_load_cluster_sector
  bcs chain_seek_done
chain_seek_walk_check:
  lda name_ptr_low
  cmp dir_entry_ptr_low
  bne chain_seek_walk_step
  lda name_ptr_high
  cmp dir_entry_ptr_high
  beq chain_seek_found
chain_seek_walk_step:
  jsr fat_load_cluster_sector
  bcs chain_seek_done
  jsr fat_follow_chain
  bcs chain_seek_done
  inc name_ptr_low
  bne chain_seek_no_carry
  inc name_ptr_high
chain_seek_no_carry:
  lda scratch_f0
  beq chain_seek_chain_end
  jsr block_store_chain_position
chain_seek_chain_end:
  jsr fat_cluster_is_last
  bcc chain_seek_walk_check
  lda #$00
chain_seek_done:
  rts
chain_seek_found:
  ldx cluster_high
  ldy cluster_low
  jsr cluster_to_lba
  lda sectors_per_cluster
  asl a
  sbc #$00
  ldy #$08
  and (block_ptr_low),y
  lsr a
  tax
  clc
  adc sector_lba_0
  sta sector_lba_0
  bcc chain_seek_found_done
  clc
  inc sector_lba_1
  bne chain_seek_found_done
  inc sector_lba_2
chain_seek_found_done:
  rts

; 估算从读写位置所在扇区起可以连续读写多少扇区（所在簇及其后相邻编号的簇，不含需要换 FAT 扇区的情形）。入口：A 为要传送的扇区数。出口：C=0 时 A 为可连续传送的扇区数的页数（扇区数乘 2，不超过要求值），sector_lba_0-2 为起始扇区；C=1 为簇链出错。
file_contiguous_run:
  sta sectors_left
  lda sectors_per_cluster
  asl a
  sec
  sbc #$01
  ldy #$08
  and (block_ptr_low),y
  lsr a
  clc
  adc sectors_left
  adc sectors_per_cluster
  sbc #$00
  sta sectors_left
  lda sectors_per_cluster
run_shift_loop:
  lsr a
  beq run_seek
  lsr sectors_left
  bne run_shift_loop
run_seek:
  lda #$01
  jsr file_seek_cluster
  bcc run_count_start
  cmp #$01
  rts
run_count_start:
  stx dio_count
  lda #$01
  sta contiguous_count
run_extend_loop:
  dec sectors_left
  beq run_scale
  ldx cluster_high
  ldy cluster_low
  jsr fat_xy_is_last
  bcs run_scale
  stx scratch_f9
  sty scratch_f8
  txa
  bit fat16_flag
  bmi run_check_sector
  jsr fat12_sector_index
  clc
run_check_sector:
  adc fat1_start_sector
  cmp fat_buffer_sector
  bne run_scale
  lda #$80
  jsr fat_read_value
  bcs run_scale
  tax
  ldy cluster_low
  bne run_compare
  dex
run_compare:
  dey
  cpy scratch_f8
  bne run_scale
  cpx scratch_f9
  bne run_scale
  inc contiguous_count
  bcs run_extend_loop
run_scale:
  lda sectors_per_cluster
run_scale_loop:
  lsr a
  beq run_result
  asl contiguous_count
  jmp run_scale_loop
run_result:
  lda contiguous_count
  sec
  sbc dio_count
  clc
  rts
