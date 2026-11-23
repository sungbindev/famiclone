; 模块：file-io
; 文件调用入口与文件读写：分发、打开、读、写、关闭

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0E15"
; 段 0，$8A15-$97FF
; 文件调用进入（入口桩代码把 $8000 窗口切到 DOS 页后调用）。入口：A 为调用者所在页，X:Y 为调用者的通道块地址。先把零页 $E5-$FF 保存到 saved_zero_page；通道块在 $8000-$BFFF 窗口里时，经 BIOS 功能 17 把它的 48 字节与低端内存 $0700 交换，$EE:$EF 指向 $0700，否则直接指向调用者的块。
file_call_enter:
  sta caller_page
  cld
  txa
  pha
  ldx #$1A
enter_save_zeropage_loop:
  lda dio_command,x
  sta saved_zero_page,x
  dex
  bpl enter_save_zeropage_loop
  pla
  sta caller_block_high
  sty caller_block_low
  cmp #$80
  bcc enter_set_block_ptr
  cmp #$C0
  bcs enter_set_block_ptr
  sta dir_entry_ptr_high
  sty dir_entry_ptr_low
  lda caller_page
  sta scratch_f0
  ldx #$07
  ldy #$00
  stx name_ptr_high
  sty name_ptr_low
  ldx #$00
  ldy #$30
  stx cluster_high
  sty cluster_low
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
  lda #$07
  ldy #$00
enter_set_block_ptr:
  sta block_ptr_high
  sty block_ptr_low
  rts

; 文件调用退出：按进入时的逆序把 $0700 里的通道块经 BIOS 功能 17 交换回调用者的窗口页，恢复零页 $E5-$FF，再把窗口切回调用者页（BIOS 功能 1）并返回；返回前 A 为出错码，并按它设 C。
file_call_leave:
  pha
  lda caller_block_high
  cmp #$80
  bcc leave_restore_zeropage
  cmp #$C0
  bcs leave_restore_zeropage
  sta dir_entry_ptr_high
  lda caller_block_low
  sta dir_entry_ptr_low
  lda caller_page
  sta scratch_f0
  ldx #$07
  ldy #$00
  stx name_ptr_high
  sty name_ptr_low
  ldx #$00
  ldy #$30
  stx cluster_high
  sty cluster_low
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
leave_restore_zeropage:
  ldx #$1A
leave_restore_loop:
  lda saved_zero_page,x
  sta dio_command,x
  dex
  bpl leave_restore_loop
  pla
  cmp #$01
  tay
  lda #$01
  ldx caller_page
  jmp BIOS_ENTRY

; 把目录项（$F4 指向）的 11 字节文件名复制到 $F1 指向的结果区，第 0 字节的 $E5 改成 $05。
dta_copy_name:
  ldy #$0A
copy_name_loop:
  lda (name_ptr_low),y
  sta (dir_entry_ptr_low),y
  dey
  bne copy_name_loop
  lda (name_ptr_low),y
  cmp #$E5
  bne copy_name_first
  lda #$05
copy_name_first:
  sta (dir_entry_ptr_low),y
  rts

; 初始化新目录项（$F1 指向）：偏移 $0C-$1F 清零，偏移 $0B 写属性 X，再写入当前时间。
dir_entry_init:
  ldy #$0C
  lda #$00
entry_init_clear_loop:
  sta (dir_entry_ptr_low),y
  iny
  cpy #$20
  bne entry_init_clear_loop
  txa

; 把属性 A 写进目录项偏移 $0B，并用 BIOS 功能 $3F（取日期时间）把当前时间与日期写进偏移 $16-$19。
dir_entry_set_timestamp:
  ldy #$0B
  sta (dir_entry_ptr_low),y
  lda #$3F
  jsr BIOS_ENTRY
  pha
  tya
  ldy #$16
  sta (dir_entry_ptr_low),y
  iny
  txa
  sta (dir_entry_ptr_low),y
  iny
  lda far_ptr_high
  sta (dir_entry_ptr_low),y
  iny
  pla
  sta (dir_entry_ptr_low),y
  rts

; 把数据缓冲区 sector_buffer（$9200-$93FF）清零。
data_buffer_clear:
  ldy #$00
  tya
buffer_clear_low:
  sta sector_buffer,y
  iny
  bne buffer_clear_low
buffer_clear_high:
  sta sector_buffer_high,y
  iny
  bne buffer_clear_high
  rts

; 占用 free_slot_lba_0-2 与 $FE 记下的空闲目录项：读入该目录扇区，目录项偏移取 $FE，定位目录项并把通道块里的文件名复制进去（dta_copy_name）。出口：C=0 成功。
dir_claim_free_slot:
  lda free_slot_lba_2
  ldx free_slot_lba_1
  ldy free_slot_lba_0
  jsr dir_buffer_read
  bcs claim_done
  lda far_ptr_low
  sta dir_entry_offset
  jsr dir_entry_pointer
  jsr block_name_pointer
  jsr dta_copy_name
  clc
claim_done:
  rts

; 给当前子目录追加一个簇：沿簇链走到最后一簇，分配新簇接在后面，把新簇的所有扇区清零，并让 free_slot_lba_0-2 指向新簇的第一个扇区、$FE 清零（第一个目录项）。磁盘已满时 extended_error=$27、A=5。
dir_grow:
  lda #$00
  sta fat_access_mode
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
grow_walk_chain:
  stx chain_cluster_high
  sty chain_cluster_low
  jsr fat_entry_access
  bcs grow_done
  jsr fat_value_is_last
  bcc grow_walk_chain
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx cluster_high
  sty cluster_low
  jsr fat_allocate_cluster
  bcc grow_allocated
  cmp #$27
  sec
  bne grow_done
  sta extended_error
  lda #$05
  rts
grow_allocated:
  jsr fat_advance_search
  jsr fat_write_entry
  bcs grow_done
  jsr buffers_flush_all
  ldx chain_cluster_high
  ldy chain_cluster_low
  jsr cluster_to_lba
  bcs grow_done
  sta free_slot_lba_2
  stx free_slot_lba_1
  sty free_slot_lba_0
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  lda #$00
  sta far_ptr_low
  jsr data_buffer_clear
  ldx sectors_per_cluster
  stx scratch_f3
grow_clear_loop:
  lda #$03
  jsr data_buffer_transfer
  bcs grow_done
  inc data_buffer_lba_0
  bne grow_next_sector
  inc data_buffer_lba_1
  bne grow_next_sector
  inc data_buffer_lba_2
grow_next_sector:
  dec scratch_f3
  bne grow_clear_loop
grow_done:
  rts

; 文件调用 1 FileOpen、20 FilePathOpen：打开文件。出口：通道块偏移 2 为属性、3-4 为日期、5-6 为时间、$0B-$0C 为首簇、$0D-$10 为长度，读写位置（偏移 7-10）为 0；文件不存在 A=$02。Ember 实测。
file_call_open:
  lda #$27
  ldy #$18

; 打开文件的公共部分：用 dir_find_entry 找到目录项后，把属性、时间、日期、首簇、长度从目录项复制到通道块（偏移 2-6、$0B-$10），读写位置（偏移 7-10）清零，并记下目录项位置（block_store_dir_position）。
file_open_common:
  jsr dir_find_entry
  bcs open_done
  ldy #$0B
  lda (dir_entry_ptr_low),y
  ldy #$02
  sta (block_ptr_low),y
  ldy #$19
open_push_time_loop:
  lda (dir_entry_ptr_low),y
  pha
  dey
  cpy #$15
  bne open_push_time_loop
  ldy #$05
  pla
  sta (block_ptr_low),y
  iny
  pla
  sta (block_ptr_low),y
  ldy #$03
  pla
  sta (block_ptr_low),y
  iny
  pla
  sta (block_ptr_low),y
open_init_state:
  ldy #$16
  lda #$FF
  sta (block_ptr_low),y
  dey
  sta (block_ptr_low),y
  lda #$00
  ldy #$19
  sta (block_ptr_low),y
  ldy #$1A
  sta (block_ptr_low),y
  ldy #$1F
open_push_size_loop:
  lda (dir_entry_ptr_low),y
  pha
  dey
  cpy #$1A
  bcs open_push_size_loop
  ldy #$0B
open_pop_size_loop:
  pla
  sta (block_ptr_low),y
  iny
  cpy #$11
  bne open_pop_size_loop
  ldy #$07
  lda #$00
open_clear_position_loop:
  sta (block_ptr_low),y
  iny
  cpy #$0B
  bne open_clear_position_loop

; 把目录项位置存进通道块：偏移 $11 为项偏移除以 2，偏移 $12-$14 为目录扇区号（3 字节），用于关闭文件时回写。
block_store_dir_position:
  ldy #$11
  lda dir_entry_offset
  sta (block_ptr_low),y
  iny
  lda dir_buffer_lba_0
  sta (block_ptr_low),y
  iny
  lda dir_buffer_lba_1
  sta (block_ptr_low),y
  iny
  lda dir_buffer_lba_2
  sta (block_ptr_low),y
  lda #$00
  clc
open_done:
  rts

; 把本次传送字节数 chunk_bytes_low/high 加到通道块里从偏移 Y 起的 2 字节字里（用来推进缓冲区地址、字节数与读写位置）。
add_chunk_to_pointer:
  clc
  lda (block_ptr_low),y
  adc chunk_bytes_low
  sta (block_ptr_low),y
  iny
  lda (block_ptr_low),y
  adc chunk_bytes_high
  sta (block_ptr_low),y
chunk_pointer_done:
  rts

; 传送不满一个扇区的一段：把读写位置所在的扇区取进 sector_buffer，经 BIOS_MOVE_BLOCK 在 sector_buffer 与调用者缓冲区之间复制 chunk_bytes 字节（读时从 sector_buffer 复制出去，写时复制进来并标记 data_buffer_dirty），再推进缓冲区地址、字节数与读写位置。
file_transfer_partial_sector:
  lda #$00
  jsr file_seek_cluster
  bcs chunk_pointer_done
  lda sector_lba_2
  ldx sector_lba_1
  ldy sector_lba_0
  jsr data_sector_select
  bcs chunk_pointer_done
  lda remaining_bytes_low
  sec
  sbc chunk_bytes_low
  sta remaining_bytes_low
  lda remaining_bytes_high
  sbc chunk_bytes_high
  sta remaining_bytes_high
  lda transfer_command
  cmp #$02
  beq partial_sector_read
  lda #$FF
  sta data_buffer_dirty
  lda dos_page
  sta scratch_f3
  ldy #$07
  lda (block_ptr_low),y
  sta name_ptr_low
  iny
  lda (block_ptr_low),y
  and #$01
  ora #$92
  sta name_ptr_high
  ldy #$02
  lda (block_ptr_low),y
  sta scratch_f0
  ldy #$03
  lda (block_ptr_low),y
  sta dir_entry_ptr_low
  iny
  lda (block_ptr_low),y
  sta dir_entry_ptr_high
  jmp partial_sector_move
partial_sector_read:
  lda dos_page
  sta scratch_f0
  ldy #$07
  lda (block_ptr_low),y
  sta dir_entry_ptr_low
  iny
  lda (block_ptr_low),y
  and #$01
  ora #$92
  sta dir_entry_ptr_high
  ldy #$02
  lda (block_ptr_low),y
  sta scratch_f3
  ldy #$03
  lda (block_ptr_low),y
  sta name_ptr_low
  iny
  lda (block_ptr_low),y
  sta name_ptr_high
partial_sector_move:
  lda chunk_bytes_low
  sta cluster_low
  lda chunk_bytes_high
  sta cluster_high
  lda #$0C
  ldx #$F0
  jsr BIOS_ENTRY
  ldy #$03
  jsr add_chunk_to_pointer
  ldy #$05
  jsr add_chunk_to_pointer
  ldy #$07
  jsr add_chunk_to_pointer

; 把上一步 add_chunk_to_pointer 产生的进位传进读写位置的高两个字节（通道块偏移 9、10）；入口 Y 为 8。
carry_into_position:
  ldx #$02
carry_loop:
  iny
  lda (block_ptr_low),y
  adc #$00
  sta (block_ptr_low),y
  dex
  bne carry_loop
  rts

; 保证文件的簇链够长：算出写入结束位置所在的簇序号，沿簇链走到链尾，必要时用 fat_allocate_cluster 逐个追加新簇（fat_advance_search 推进空闲簇搜索位置）。磁盘已满时 A=$27、C=1。
file_extend_chain:
  ldx remaining_bytes_high
  ldy remaining_bytes_low
  bne extend_no_borrow
  dex
extend_no_borrow:
  dey
  tya
  ldy #$07
  clc
  adc (block_ptr_low),y
  iny
  txa
  adc (block_ptr_low),y
  sta a:scratch_f8
  iny
  lda #$00
  adc (block_ptr_low),y
  sta a:scratch_f9
  iny
  lda #$00
  adc (block_ptr_low),y
  sta scratch_f0
  lda sectors_per_cluster
extend_shift_loop:
  lsr scratch_f0
  ror a:scratch_f9
  ror a:scratch_f8
  lsr a
  bne extend_shift_loop
  lda a:scratch_f8
  ldy #$15
  cmp (block_ptr_low),y
  iny
  lda a:scratch_f9
  sbc (block_ptr_low),y
  bcs extend_from_cached
  ldx #$FF
  stx dir_entry_ptr_high
  stx dir_entry_ptr_low
  inx
  ldy #$00
  beq extend_walk_start
extend_from_cached:
  ldy #$16
  lda (block_ptr_low),y
  sta dir_entry_ptr_high
  dey
  lda (block_ptr_low),y
  sta dir_entry_ptr_low
  ldy #$18
  lda (block_ptr_low),y
  tax
  dey
  lda (block_ptr_low),y
  tay
extend_walk_start:
  lda #$00
  sta fat_access_mode
extend_walk_loop:
  lda scratch_f9
  cmp dir_entry_ptr_high
  bne extend_read_entry
  lda scratch_f8
  cmp dir_entry_ptr_low
  beq extend_ok
extend_read_entry:
  stx chain_cluster_high
  sty chain_cluster_low
  jsr fat_entry_access
  bcs extend_done
  jsr fat_value_is_last
  bcs extend_allocate_start
  inc dir_entry_ptr_low
  bne extend_walk_loop
  inc dir_entry_ptr_high
  jmp extend_walk_loop
extend_allocate_start:
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx cluster_high
  sty cluster_low
extend_allocate_loop:
  jsr fat_allocate_cluster
  bcs extend_done
  inc dir_entry_ptr_low
  bne extend_check_done
  inc dir_entry_ptr_high
extend_check_done:
  lda dir_entry_ptr_low
  cmp scratch_f8
  lda dir_entry_ptr_high
  sbc scratch_f9
  bcs extend_finish
  jsr fat_advance_search
  bne extend_allocate_loop
  lda #$27
  sec
  rts
extend_finish:
  jmp fat_write_entry
extend_ok:
  clc
extend_done:
  rts

; 算出文件长度（偏移 $0D，4 字节）减读写位置（偏移 7，4 字节）的差：低 16 位在 $F0:$F1，高 16 位在 X:A；差为负时 C=0。
file_bytes_left:
  ldy #$10
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  tax
  dey
  lda (block_ptr_low),y
  ldy #$07
  sec
  sbc (block_ptr_low),y
  sta scratch_f0
  txa
  iny
  sbc (block_ptr_low),y
  sta dir_entry_ptr_low
  pla
  iny
  sbc (block_ptr_low),y
  tax
  pla
  iny
  sbc (block_ptr_low),y
  rts

; 取出通道块里的读写字节数（偏移 5-6）到 remaining_bytes_low/high 并把它清零（之后用它累计实际传送的字节数）；Z=1 表示要求的字节数为 0。
block_take_length:
  ldy #$06
  lda (block_ptr_low),y
  sta remaining_bytes_high
  lda #$00
  sta (block_ptr_low),y
  dey
  lda (block_ptr_low),y
  sta remaining_bytes_low
  pha
  lda #$00
  sta (block_ptr_low),y
  pla
  ora remaining_bytes_high
  rts

; 文件调用 2 FileRead：从读写位置（偏移 7）起读 FH_BYTES 字节到 FH_PAGE:FH_BUFFER，可跨扇区；读到文件末尾时 FH_BYTES 是实际读到的字节数。出口：FH_BUFFER 为缓冲区结束地址，读写位置前进。Ember 实测。
file_call_read:
  jsr drive_select
  bcs read_done
  jsr file_bytes_left
  sta far_ptr_low
  jsr block_take_length
  bcc read_beyond_end
  beq read_zero_bytes
  lda far_ptr_low
  bne read_transfer
  txa
  bne read_transfer
  ldx dir_entry_ptr_low
  ldy scratch_f0
  cpy remaining_bytes_low
  txa
  sbc remaining_bytes_high
  bcs read_transfer
  txa
  ora scratch_f0
  beq read_done
  stx remaining_bytes_high
  sty remaining_bytes_low
read_transfer:
  lda #$02
  jmp file_transfer
read_beyond_end:
  ldy #$0D
read_push_loop:
  lda (block_ptr_low),y
  pha
  iny
  cmp #$11
  bcc read_push_loop
  ldy #$0A
read_pop_loop:
  pla
  sta (block_ptr_low),y
  dey
  cpy #$07
  bcs read_pop_loop
read_zero_bytes:
  lda #$00
read_done:
  rts

; 文件调用 3 FileWrite：从读写位置起把 FH_PAGE:FH_BUFFER 里的 FH_BYTES 字节写进文件，必要时分配新簇并扩大文件长度；FH_BYTES 为 0 表示在读写位置截断文件。不检查只读属性。出口同 FileRead。Ember 实测。
file_call_write:
  jsr drive_select
  bcs write_done
  jsr block_take_length
  bne write_nonzero
  jsr file_truncate_chain
  bcc write_success
  rts
write_nonzero:
  jsr file_bytes_left
  bcc write_extend
  bne write_in_place
  txa
  bne write_in_place
  lda remaining_bytes_low
  cmp scratch_f0
  lda remaining_bytes_high
  sbc dir_entry_ptr_low
  bcc write_in_place
write_extend:
  jsr file_extend_chain
  bcs write_extend_failed
  lda #$03
  jsr file_transfer
  bcs write_return
  jsr file_set_length_to_position
  lda #$00
write_return:
  rts
write_extend_failed:
  cmp #$27
  bne write_done
write_success:
  lda #$00
write_done:
  rts
write_in_place:
  lda #$03

; 读写文件的主体。入口：A 为命令（2 读、3 写），remaining_bytes 为要传送的字节数。分三段：先补齐当前扇区的剩余部分（file_transfer_partial_sector），再整扇区、整簇连续传送（disk_transfer_sectors），最后传送尾部不满一个扇区的部分。
file_transfer:
  sta transfer_command
  ldy #$07
  lda (block_ptr_low),y
  eor #$FF
  tax
  iny
  lda (block_ptr_low),y
  eor #$FF
  and #$01
  tay
  inx
  bne transfer_head_chunk
  iny
  cpy #$02
  beq transfer_middle_loop
transfer_head_chunk:
  cpy remaining_bytes_high
  bne transfer_head_min
  cpx remaining_bytes_low
transfer_head_min:
  bcc transfer_head_store
  ldy remaining_bytes_high
  ldx remaining_bytes_low
transfer_head_store:
  sty chunk_bytes_high
  stx chunk_bytes_low
  jsr file_transfer_partial_sector
  bcs transfer_done
transfer_middle_loop:
  lda remaining_bytes_high
  lsr a
  beq transfer_tail
  jsr file_contiguous_run
  bcs transfer_done
  asl a
  bcs transfer_middle_all
  beq transfer_done
  cmp remaining_bytes_high
  bcc transfer_middle_even
transfer_middle_all:
  lda remaining_bytes_high
transfer_middle_even:
  and #$FE
  sta contiguous_count
  lsr a
  jsr disk_transfer_sectors
  bcs transfer_done
  lda remaining_bytes_high
  sec
  sbc contiguous_count
  sta remaining_bytes_high
  ldy #$08
  clc
  lda (block_ptr_low),y
  adc contiguous_count
  sta (block_ptr_low),y
  jsr carry_into_position
  ldy #$06
  lda (block_ptr_low),y
  adc contiguous_count
  sta (block_ptr_low),y
  ldy #$04
  lda (block_ptr_low),y
  adc contiguous_count
  sta (block_ptr_low),y
  jmp transfer_middle_loop
transfer_tail:
  ldx remaining_bytes_high
  txa
  ora remaining_bytes_low
  beq transfer_ok
  ldy remaining_bytes_low
  stx chunk_bytes_high
  sty chunk_bytes_low
  jsr file_transfer_partial_sector
  bcs transfer_done
  lda #$00
transfer_ok:
  clc
transfer_done:
  rts

; 把文件在读写位置处截断：释放位置之后的簇（含最后一个保留字节的簇标成链结束，其后各簇的 FAT 项清零；位置为 0 时首簇也一并释放），随后读写位置恢复，并落入 file_set_length_to_position。
file_truncate_chain:
  ldy #$07
  ldx #$04
  clc
truncate_dec_loop:
  lda (block_ptr_low),y
  sbc #$00
  sta (block_ptr_low),y
  iny
  dex
  bne truncate_dec_loop
  bcs truncate_find_cluster
  ldy #$00
  beq truncate_mark_end
truncate_find_cluster:
  txa
  jsr file_seek_cluster
  bcc truncate_have_cluster
  tax
  beq truncate_restore_position
  rts
truncate_have_cluster:
  ldx cluster_high
  ldy cluster_low
truncate_mark_end:
  stx chain_cluster_high
  sty chain_cluster_low
  lda #$FF
  sta fat_access_mode
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
truncate_free_loop:
  jsr fat_entry_access
  bcs transfer_done
  jsr fat_value_is_last
  bcs truncate_restore_position
  lda #$00
  sta fat_value_low
  sta fat_value_high
  beq truncate_free_loop
truncate_restore_position:
  ldy #$07
  ldx #$04
  sec
truncate_inc_loop:
  lda (block_ptr_low),y
  adc #$00
  sta (block_ptr_low),y
  iny
  dex
  bne truncate_inc_loop

; 把读写位置（通道块偏移 7-10）复制成文件长度（偏移 $0D-$10），并把偏移 $19 置 1（目录项待更新）。出口：C=0。
file_set_length_to_position:
  ldy #$07
set_length_push_loop:
  lda (block_ptr_low),y
  pha
  iny
  cpy #$0B
  bne set_length_push_loop
  ldy #$11
set_length_pop_loop:
  dey
  pla
  sta (block_ptr_low),y
  cpy #$0D
  bne set_length_pop_loop
  ldy #$19
  lda #$01
  sta (block_ptr_low),y
  clc
  rts

; 按通道块偏移 $11-$14 记录的位置读入文件所在的目录扇区并定位目录项（结果在 dir_entry_ptr_low/high）；先 drive_select。出口：C=1 出错。
block_load_dir_entry:
  jsr drive_select
  ldy #$14
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  tax
  dey
  lda (block_ptr_low),y
  tay
  pla
  jsr dir_buffer_read
  bcs load_entry_done
  ldy #$11
  lda (block_ptr_low),y
  jsr dir_entry_pointer
  clc
load_entry_done:
  rts

; 文件调用 5 FileClose：把文件长度、日期、时间和属性写回目录项，并把缓冲区写回磁盘。Ember 实测。
file_call_close:
  lda data_buffer_dirty
  beq close_flush_fat
  inc data_buffer_dirty
  lda #$03
  jsr data_buffer_transfer
  bcs close_done
close_flush_fat:
  ldy #$1A
  lda (block_ptr_low),y
  beq close_update_entry
  jsr fat_buffer_flush
close_update_entry:
  ldy #$19
  lda (block_ptr_low),y
  beq close_ok
  jsr block_load_dir_entry
  bcs close_done
  ldy #$0B
close_push_loop:
  lda (block_ptr_low),y
  pha
  iny
  cpy #$11
  bne close_push_loop
  ldy #$20
close_pop_loop:
  dey
  pla
  sta (dir_entry_ptr_low),y
  cpy #$1A
  bne close_pop_loop
  jsr dir_buffer_write
  bcs close_done
close_ok:
  lda #$00
close_done:
  rts

; 从 dir_entry_offset 起在目录里查找下一个与 name_ptr 指向的名字匹配的目录项：名字里的 ? 匹配任意字符，$05 按 $E5 比较；属性用 dir_attr_filter 筛选；需要时顺着目录扇区往后读。找到时 C=0，dir_entry_ptr 指向它；到目录末尾时 A=$12（SearchEnding）、C=1。
dir_match_next:
  jsr dir_buffer_read
  bcs match_done
  lda dir_entry_offset
match_entry:
  jsr dir_entry_pointer
  ldy #$00
  lda (dir_entry_ptr_low),y
  beq match_end_of_dir
  cmp #$E5
  beq match_next_entry
match_name_loop:
  lda (name_ptr_low),y
  cmp #$3F
  beq match_next_char
  lda (dir_entry_ptr_low),y
  cmp #$05
  bne match_compare_char
  lda #$E5
match_compare_char:
  cmp (name_ptr_low),y
  bne match_next_entry
match_next_char:
  iny
  cpy #$0B
  bne match_name_loop
  lda (dir_entry_ptr_low),y
  bit dir_attr_filter
  bmi match_found
  bne match_next_entry
  ora dir_attr_filter
  and #$08
  beq match_next_entry
match_found:
  clc
  rts
match_next_entry:
  lda dir_entry_offset
  clc
  adc #$10
  sta dir_entry_offset
  bne match_entry
  jsr dir_next_sector_current
  bcc dir_match_next
match_end_of_dir:
  lda #$12
  sec
match_done:
  rts

; 文件调用分发表：功能号 0-24 共 25 项，分成高字节表（本表，$900F）与低字节表（$9028），每项都是处理例程地址减 1。
; 功能号 20（FilePathOpen）与 1（FileOpen）、21（FilePathCreate）与 8（FileCreate）、23（FilePathDelete）与 10（FileDelete）共用处理例程，区别在于 file_call_flags 里的取路径标志。
file_call_table_high:
  .byte $98, $8B, $8D, $8E, $A0, $8F, $9F, $9F
  .byte $9E, $9F, $A0, $A0, $A1, $A0, $A0, $A0
  .byte $A3, $A3, $A4, $A5, $8B, $9E, $9E, $A0
  .byte $9E
file_call_table_low:
  .byte $9D
  .byte $8C, $BF, $0B, $0E, $80, $1F, $38, $BA
  .byte $DC, $A5, $38, $0B, $F3, $C9, $CD, $33
  .byte $43, $C6, $24, $8C, $BA, $7B, $A5, $51

; 文件调用的功能标志（25 项，下标为功能号）：位 7 表示先准备驱动器（检查介质），位 6 表示从通道块里的缓冲区页与地址（偏移 2、3、4）取出文件名并存入偏移 5 起的 11 字节，位 5 表示要解析路径。开头的 $00 表示三个都不需要。
file_call_flags:
  .byte $00
  .byte $C0, $00, $00, $00, $00, $C0, $00, $C0, $00, $C0, $80, $00
  .byte $40, $00, $00, $80, $A0, $A0, $80, $A0, $A0, $A0, $A0, $A0

; 按 file_call_flags 的位 6 取文件名。入口：X 为功能号；标志位 6 为 0 时直接返回 C=0。否则从调用者缓冲区（页取通道块偏移 2，地址取偏移 3、4）读 11 字节名字（主名 8、扩展名 3，空格补齐）复制到通道块偏移 5 起；字符经 name_char_check 检查并转大写，空格之后又出现非空格字符、首字符为空格或有非法字符时 A=2、C=1。
file_call_fetch_name:
  lda file_call_flags,x
  asl a
  bpl fetch_name_ok
  pha
  ldy #$04
  lda (block_ptr_low),y
  sta far_ptr_high
  dey
  lda (block_ptr_low),y
  sta far_ptr_low
  lda #$00
  sta scratch_f0
  ldy #$05
fetch_name_reset_gap:
  lda #$00
  sta scratch_f8
fetch_name_loop:
  sty scratch_f3
  ldy #$02
  lda (block_ptr_low),y
  tax
  lda #$20
  ldy scratch_f0
  jsr BIOS_ENTRY
  cmp #$20
  bne fetch_name_check_char
  ldy scratch_f0
  beq fetch_name_bad
  dec scratch_f8
  bne fetch_name_store
fetch_name_check_char:
  bit scratch_f8
  bvs fetch_name_bad
  jsr name_char_check
  bcs fetch_name_bad
fetch_name_store:
  ldy scratch_f3
  sta (block_ptr_low),y
  inc scratch_f0
  iny
  cpy #$0D
  beq fetch_name_reset_gap
  cpy #$10
  bne fetch_name_loop
  pla
fetch_name_ok:
  clc
  rts
fetch_name_bad:
  pla
  lda #$02
  sec
  rts

; 按通道块里的功能号用 RTS 跳转（file_call_table_high、file_call_table_low 里的地址减 1）进入对应的 file_call_ 例程，该例程返回后回到 file_call_dispatch 的调用处。
file_call_jump_to_handler:
  ldy #$00
  lda (block_ptr_low),y
  tax
  lda file_call_table_high,x
  pha
  lda file_call_table_low,x
  pha
  rts

; 文件调用分发（入口 $5803 的桩代码先把页切到 DOS 页，再调用本例程）。
; 入口：A 是桩代码取得的调用者当前页号（调用者不用设置 A），X 为通道块地址高字节，Y 为低字节；通道块第 0 字节是功能号（0-24），第 1 字节是盘号（0 取默认盘，$FF 取启动盘）。
; 出口：A 为出错码，0 表示成功；C=1 表示出错；功能号大于 24 时 A=1。
; 处理：先 file_call_enter 保存零页并取得通道块，再按 file_call_flags 准备驱动器、取文件名、解析路径，然后按 file_call_table_high 与 file_call_table_low 分发到各 file_call_ 例程，最后 file_call_leave 恢复页与零页。
file_call_dispatch:
  jsr file_call_enter
  ldy #$00
  lda (block_ptr_low),y
  cmp #$19
  bcc file_dispatch_in_range
  lda #$01
  jmp file_call_leave
file_dispatch_in_range:
  cmp #$09
  beq file_dispatch_call_handler
  tax
  ldy #$01
  lda (block_ptr_low),y
  bne file_dispatch_check_boot
  lda default_disk
  sta (block_ptr_low),y
file_dispatch_check_boot:
  cmp #$FF
  bne file_dispatch_check_count
  lda boot_disk
file_dispatch_check_count:
  cmp logical_drive_count
  bcc file_dispatch_floppy_number
  bne file_dispatch_bad_drive
  ldy drive_info_y
  bne file_dispatch_bios_drive
file_dispatch_floppy_number:
  sec
  sbc #$01
  cmp #$02
  bcc file_dispatch_check_physical
  sbc #$02
  ora #$80
  bne file_dispatch_set_drive
file_dispatch_check_physical:
  ldy drive_info_a
  beq file_dispatch_bad_drive
file_dispatch_set_drive:
  sta current_drive
  lda file_call_flags,x
  bpl file_dispatch_after_prepare
  jsr drive_activate
  bcs file_dispatch_leave
  ldy #$00
  lda (block_ptr_low),y
  tax
file_dispatch_after_prepare:
  jsr file_call_fetch_name
  bcs file_dispatch_leave
  asl a
  bpl file_dispatch_call_handler
  cpx #$16
  beq file_dispatch_move_attribute
  cpx #$18
  bne file_dispatch_resolve_path
file_dispatch_move_attribute:
  ldy #$05
  lda (block_ptr_low),y
  ldy #$16
  sta (block_ptr_low),y
file_dispatch_resolve_path:
  jsr path_resolve_directory
  bcs file_dispatch_leave
file_dispatch_call_handler:
  jsr file_call_jump_to_handler
  jmp file_call_leave
file_dispatch_bad_drive:
  lda #$0F
file_dispatch_leave:
  jmp file_call_leave

; 把文件调用交给 BIOS 功能 13 处理（逻辑驱动器编号等于 logical_drive_count 的那个驱动器由 BIOS 管理）：X 为通道块地址高字节，Y 为低字节。
bios_file_call:
  lda #$0D
  ldx block_ptr_high
  ldy block_ptr_low
  jmp BIOS_ENTRY
file_dispatch_bios_drive:
  cpx #$0D
  bne file_dispatch_bios_normal
  ldy #$0D
  lda (block_ptr_low),y
  pha
  dey
  lda (block_ptr_low),y
  pha
  jsr file_call_fetch_name
  ldy #$03
  pla
  sta (block_ptr_low),y
  iny
  pla
  sta (block_ptr_low),y
  bcs file_dispatch_bios_leave
  bcc file_dispatch_bios_plain
file_dispatch_bios_normal:
  jsr file_call_fetch_name
  bcs file_dispatch_bios_leave
  ldy #$00
  lda (block_ptr_low),y
  cmp #$06
  beq file_dispatch_bios_find
  cmp #$07
  beq file_dispatch_bios_find
  cmp #$16
  bne file_dispatch_bios_plain
  ldy #$03
  lda (block_ptr_low),y
  tax
  iny
  lda (block_ptr_low),y
  ldy #$06
  sta (block_ptr_low),y
  dey
  lda (block_ptr_low),y
  pha
  txa
  sta (block_ptr_low),y
  pla
  ldy #$07
  sta (block_ptr_low),y
file_dispatch_bios_find:
  ldx block_ptr_high
  ldy block_ptr_low
  jsr bios_block_setup
  jsr bios_file_call
  ldx caller_block_high
  ldy caller_block_low
  jsr bios_block_setup
  jmp file_dispatch_bios_result
file_dispatch_bios_plain:
  jsr bios_file_call
file_dispatch_bios_result:
  ldy #$0B
  lda (block_ptr_low),y
file_dispatch_bios_leave:
  jmp file_call_leave

; 为交给 BIOS 的文件调用准备通道块：偏移 2 写调用者页，偏移 3、4 写 X:Y 加 $17（结果区地址）；入口 X 为高字节，Y 为低字节。
bios_block_setup:
  tya
  pha
  lda caller_page
  ldy #$02
  sta (block_ptr_low),y
  pla
  clc
  adc #$17
  ldy #$03
  sta (block_ptr_low),y
  txa
  adc #$00
  iny
  sta (block_ptr_low),y
  rts
  .res 56, $FF
; 数据缓冲区（512 字节，$9200-$93FF），引导扇区也读到这里解析
sector_buffer:
  .byte $FF, $FF
boot_byte_2:
  .res 11, $FF
; 引导扇区里的 BPB 字段：每簇扇区数（偏移 $0D）
boot_sectors_per_cluster:
  .byte $FF
boot_reserved_sectors:
  .byte $FF, $FF
boot_fat_count:
  .byte $FF
boot_root_entries_low:
  .byte $FF
boot_root_entries_high:
  .byte $FF
boot_total_sectors_low:
  .byte $FF
boot_total_sectors_high:
  .byte $FF
boot_media_descriptor:
  .byte $FF
boot_sectors_per_fat:
  .byte $FF, $FF
boot_sectors_per_track:
  .byte $FF, $FF
boot_head_count:
  .byte $FF
dot_entry_cluster_high:
  .res 5, $FF
boot_total_sectors32_0:
  .byte $FF
boot_total_sectors32_1:
  .byte $FF
boot_total_sectors32_2:
  .res 24, $FF
dotdot_entry_cluster_low:
  .byte $FF
dotdot_entry_cluster_high:
  .res 197, $FF
sector_buffer_high:
  .res 256, $FF
; 目录缓冲区（512 字节，$9400-$95FF）
dir_buffer:
  .res 58, $FF
dir_dotdot_cluster_low:
  .byte $FF
dir_dotdot_cluster_high:
  .res 197, $FF
dir_buffer_high:
  .res 256, $FF
; FAT 缓冲区（512 字节，$9600-$97FF）
fat_buffer:
  .res 256, $FF
fat_buffer_high:
  .res 256, $FF
