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
loc_8A1D:
  lda var_00E5,x
  sta saved_zero_page,x
  dex
  bpl loc_8A1D
  pla
  sta caller_block_high
  sty caller_block_low
  cmp #$80
  bcc loc_8A58
  cmp #$C0
  bcs loc_8A58
  sta var_00F2
  sty var_00F1
  lda caller_page
  sta var_00F0
  ldx #$07
  ldy #$00
  stx var_00F5
  sty var_00F4
  ldx #$00
  ldy #$30
  stx var_00F7
  sty var_00F6
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
  lda #$07
  ldy #$00
loc_8A58:
  sta var_00EF
  sty var_00EE
  rts

; 文件调用退出：按进入时的逆序把 $0700 里的通道块经 BIOS 功能 17 交换回调用者的窗口页，恢复零页 $E5-$FF，再把窗口切回调用者页（BIOS 功能 1）并返回；返回前 A 为出错码，并按它设 C。
file_call_leave:
  pha
  lda caller_block_high
  cmp #$80
  bcc loc_8A8C
  cmp #$C0
  bcs loc_8A8C
  sta var_00F2
  lda caller_block_low
  sta var_00F1
  lda caller_page
  sta var_00F0
  ldx #$07
  ldy #$00
  stx var_00F5
  sty var_00F4
  ldx #$00
  ldy #$30
  stx var_00F7
  sty var_00F6
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
loc_8A8C:
  ldx #$1A
loc_8A8E:
  lda saved_zero_page,x
  sta var_00E5,x
  dex
  bpl loc_8A8E
  pla
  cmp #$01
  tay
  lda #$01
  ldx caller_page
  jmp BIOS_ENTRY

; 把目录项（$F4 指向）的 11 字节文件名复制到 $F1 指向的结果区，第 0 字节的 $E5 改成 $05。
dta_copy_name:
  ldy #$0A
loc_8AA4:
  lda (var_00F4),y
  sta (var_00F1),y
  dey
  bne loc_8AA4
  lda (var_00F4),y
  cmp #$E5
  bne loc_8AB3
  lda #$05
loc_8AB3:
  sta (var_00F1),y
  rts

; 初始化新目录项（$F1 指向）：偏移 $0C-$1F 清零，偏移 $0B 写属性 X，再写入当前时间。
dir_entry_init:
  ldy #$0C
  lda #$00
loc_8ABA:
  sta (var_00F1),y
  iny
  cpy #$20
  bne loc_8ABA
  txa

; 把属性 A 写进目录项偏移 $0B，并用 BIOS 功能 $3F（取日期时间）把当前时间与日期写进偏移 $16-$19。
dir_entry_set_timestamp:
  ldy #$0B
  sta (var_00F1),y
  lda #$3F
  jsr BIOS_ENTRY
  pha
  tya
  ldy #$16
  sta (var_00F1),y
  iny
  txa
  sta (var_00F1),y
  iny
  lda var_00FF
  sta (var_00F1),y
  iny
  pla
  sta (var_00F1),y
  rts

; 把数据缓冲区 sector_buffer（$9200-$93FF）清零。
data_buffer_clear:
  ldy #$00
  tya
loc_8AE2:
  sta sector_buffer,y
  iny
  bne loc_8AE2
loc_8AE8:
  sta data_9300,y
  iny
  bne loc_8AE8
  rts

sub_8AEF:
  lda free_slot_lba_2
  ldx free_slot_lba_1
  ldy free_slot_lba_0
  jsr dir_buffer_read
  bcs loc_8B0B
  lda var_00FE
  sta var_00FA
  jsr dir_entry_pointer
  jsr block_name_pointer
  jsr dta_copy_name
  clc
loc_8B0B:
  rts

sub_8B0C:
  lda #$00
  sta var_00FC
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
loc_8B16:
  stx chain_cluster_high
  sty chain_cluster_low
  jsr fat_entry_access
  bcs loc_8B8C
  jsr fat_value_is_last
  bcc loc_8B16
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx var_00F7
  sty var_00F6
  jsr fat_allocate_cluster
  bcc loc_8B40
  cmp #$27
  sec
  bne loc_8B8C
  sta data_8031
  lda #$05
  rts
loc_8B40:
  jsr fat_advance_search
  jsr fat_write_entry
  bcs loc_8B8C
  jsr buffers_flush_all
  ldx chain_cluster_high
  ldy chain_cluster_low
  jsr cluster_to_lba
  bcs loc_8B8C
  sta free_slot_lba_2
  stx free_slot_lba_1
  sty free_slot_lba_0
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  lda #$00
  sta var_00FE
  jsr data_buffer_clear
  ldx sectors_per_cluster
  stx var_00F3
loc_8B74:
  lda #$03
  jsr data_buffer_transfer
  bcs loc_8B8C
  inc data_buffer_lba_0
  bne loc_8B88
  inc data_buffer_lba_1
  bne loc_8B88
  inc data_buffer_lba_2
loc_8B88:
  dec var_00F3
  bne loc_8B74
loc_8B8C:
  rts

; 文件调用 1 FileOpen、20 FilePathOpen：打开文件。出口：通道块偏移 2 为属性、3-4 为日期、5-6 为时间、$0B-$0C 为首簇、$0D-$10 为长度，读写位置（偏移 7-10）为 0；文件不存在 A=$02。Ember 实测。
file_call_open:
  lda #$27
  ldy #$18

; 打开文件的公共部分：用 dir_find_entry 找到目录项后，把属性、时间、日期、首簇、长度从目录项复制到通道块（偏移 2-6、$0B-$10），读写位置（偏移 7-10）清零，并记下目录项位置（block_store_dir_position）。
file_open_common:
  jsr dir_find_entry
  bcs loc_8C07
  ldy #$0B
  lda (var_00F1),y
  ldy #$02
  sta (var_00EE),y
  ldy #$19
loc_8BA0:
  lda (var_00F1),y
  pha
  dey
  cpy #$15
  bne loc_8BA0
  ldy #$05
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
  ldy #$03
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
loc_8BBA:
  ldy #$16
  lda #$FF
  sta (var_00EE),y
  dey
  sta (var_00EE),y
  lda #$00
  ldy #$19
  sta (var_00EE),y
  ldy #$1A
  sta (var_00EE),y
  ldy #$1F
loc_8BCF:
  lda (var_00F1),y
  pha
  dey
  cpy #$1A
  bcs loc_8BCF
  ldy #$0B
loc_8BD9:
  pla
  sta (var_00EE),y
  iny
  cpy #$11
  bne loc_8BD9
  ldy #$07
  lda #$00
loc_8BE5:
  sta (var_00EE),y
  iny
  cpy #$0B
  bne loc_8BE5

; 把目录项位置存进通道块：偏移 $11 为项偏移除以 2，偏移 $12-$14 为目录扇区号（3 字节），用于关闭文件时回写。
block_store_dir_position:
  ldy #$11
  lda var_00FA
  sta (var_00EE),y
  iny
  lda dir_buffer_lba_0
  sta (var_00EE),y
  iny
  lda dir_buffer_lba_1
  sta (var_00EE),y
  iny
  lda dir_buffer_lba_2
  sta (var_00EE),y
  lda #$00
  clc
loc_8C07:
  rts

; 把本次传送字节数 chunk_bytes_low/high 加到通道块里从偏移 Y 起的 2 字节字里（用来推进缓冲区地址、字节数与读写位置）。
add_chunk_to_pointer:
  clc
  lda (var_00EE),y
  adc chunk_bytes_low
  sta (var_00EE),y
  iny
  lda (var_00EE),y
  adc chunk_bytes_high
  sta (var_00EE),y
loc_8C18:
  rts

; 传送不满一个扇区的一段：把读写位置所在的扇区取进 sector_buffer，经 BIOS_MOVE_BLOCK 在 sector_buffer 与调用者缓冲区之间复制 chunk_bytes 字节（读时从 sector_buffer 复制出去，写时复制进来并标记 data_buffer_dirty），再推进缓冲区地址、字节数与读写位置。
file_transfer_partial_sector:
  lda #$00
  jsr sub_88EA
  bcs loc_8C18
  lda sector_lba_2
  ldx sector_lba_1
  ldy sector_lba_0
  jsr data_sector_select
  bcs loc_8C18
  lda remaining_bytes_low
  sec
  sbc chunk_bytes_low
  sta remaining_bytes_low
  lda remaining_bytes_high
  sbc chunk_bytes_high
  sta remaining_bytes_high
  lda var_00FD
  cmp #$02
  beq loc_8C74
  lda #$FF
  sta data_buffer_dirty
  lda dos_page
  sta var_00F3
  ldy #$07
  lda (var_00EE),y
  sta var_00F4
  iny
  lda (var_00EE),y
  and #$01
  ora #$92
  sta var_00F5
  ldy #$02
  lda (var_00EE),y
  sta var_00F0
  ldy #$03
  lda (var_00EE),y
  sta var_00F1
  iny
  lda (var_00EE),y
  sta var_00F2
  jmp loc_8C99
loc_8C74:
  lda dos_page
  sta var_00F0
  ldy #$07
  lda (var_00EE),y
  sta var_00F1
  iny
  lda (var_00EE),y
  and #$01
  ora #$92
  sta var_00F2
  ldy #$02
  lda (var_00EE),y
  sta var_00F3
  ldy #$03
  lda (var_00EE),y
  sta var_00F4
  iny
  lda (var_00EE),y
  sta var_00F5
loc_8C99:
  lda chunk_bytes_low
  sta var_00F6
  lda chunk_bytes_high
  sta var_00F7
  lda #$0C
  ldx #$F0
  jsr BIOS_ENTRY
  ldy #$03
  jsr add_chunk_to_pointer
  ldy #$05
  jsr add_chunk_to_pointer
  ldy #$07
  jsr add_chunk_to_pointer

carry_into_position:
  ldx #$02
loc_8CBB:
  iny
  lda (var_00EE),y
  adc #$00
  sta (var_00EE),y
  dex
  bne loc_8CBB
  rts

sub_8CC6:
  ldx remaining_bytes_high
  ldy remaining_bytes_low
  bne loc_8CCF
  dex
loc_8CCF:
  dey
  tya
  ldy #$07
  clc
  adc (var_00EE),y
  iny
  txa
  adc (var_00EE),y
  sta a:var_00F8
  iny
  lda #$00
  adc (var_00EE),y
  sta a:var_00F9
  iny
  lda #$00
  adc (var_00EE),y
  sta var_00F0
  lda sectors_per_cluster
loc_8CEF:
  lsr var_00F0
  ror a:var_00F9
  ror a:var_00F8
  lsr a
  bne loc_8CEF
  lda a:var_00F8
  ldy #$15
  cmp (var_00EE),y
  iny
  lda a:var_00F9
  sbc (var_00EE),y
  bcs loc_8D14
  ldx #$FF
  stx var_00F2
  stx var_00F1
  inx
  ldy #$00
  beq loc_8D28
loc_8D14:
  ldy #$16
  lda (var_00EE),y
  sta var_00F2
  dey
  lda (var_00EE),y
  sta var_00F1
  ldy #$18
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  tay
loc_8D28:
  lda #$00
  sta var_00FC
loc_8D2C:
  lda var_00F9
  cmp var_00F2
  bne loc_8D38
  lda var_00F8
  cmp var_00F1
  beq loc_8D7C
loc_8D38:
  stx chain_cluster_high
  sty chain_cluster_low
  jsr fat_entry_access
  bcs loc_8D7D
  jsr fat_value_is_last
  bcs loc_8D51
  inc var_00F1
  bne loc_8D2C
  inc var_00F2
  jmp loc_8D2C
loc_8D51:
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx var_00F7
  sty var_00F6
loc_8D5B:
  jsr fat_allocate_cluster
  bcs loc_8D7D
  inc var_00F1
  bne loc_8D66
  inc var_00F2
loc_8D66:
  lda var_00F1
  cmp var_00F8
  lda var_00F2
  sbc var_00F9
  bcs loc_8D79
  jsr fat_advance_search
  bne loc_8D5B
  lda #$27
  sec
  rts
loc_8D79:
  jmp fat_write_entry
loc_8D7C:
  clc
loc_8D7D:
  rts

; 算出文件长度（偏移 $0D，4 字节）减读写位置（偏移 7，4 字节）的差：低 16 位在 $F0:$F1，高 16 位在 X:A；差为负时 C=0。
file_bytes_left:
  ldy #$10
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  ldy #$07
  sec
  sbc (var_00EE),y
  sta var_00F0
  txa
  iny
  sbc (var_00EE),y
  sta var_00F1
  pla
  iny
  sbc (var_00EE),y
  tax
  pla
  iny
  sbc (var_00EE),y
  rts

; 取出通道块里的读写字节数（偏移 5-6）到 remaining_bytes_low/high 并把它清零（之后用它累计实际传送的字节数）；Z=1 表示要求的字节数为 0。
block_take_length:
  ldy #$06
  lda (var_00EE),y
  sta remaining_bytes_high
  lda #$00
  sta (var_00EE),y
  dey
  lda (var_00EE),y
  sta remaining_bytes_low
  pha
  lda #$00
  sta (var_00EE),y
  pla
  ora remaining_bytes_high
  rts

; 文件调用 2 FileRead：从读写位置（偏移 7）起读 FH_BYTES 字节到 FH_PAGE:FH_BUFFER，可跨扇区；读到文件末尾时 FH_BYTES 是实际读到的字节数。出口：FH_BUFFER 为缓冲区结束地址，读写位置前进。Ember 实测。
file_call_read:
  jsr drive_select
  bcs loc_8E0B
  jsr file_bytes_left
  sta var_00FE
  jsr block_take_length
  bcc loc_8DF5
  beq loc_8E09
  lda var_00FE
  bne loc_8DF0
  txa
  bne loc_8DF0
  ldx var_00F1
  ldy var_00F0
  cpy remaining_bytes_low
  txa
  sbc remaining_bytes_high
  bcs loc_8DF0
  txa
  ora var_00F0
  beq loc_8E0B
  stx remaining_bytes_high
  sty remaining_bytes_low
loc_8DF0:
  lda #$02
  jmp file_transfer
loc_8DF5:
  ldy #$0D
loc_8DF7:
  lda (var_00EE),y
  pha
  iny
  cmp #$11
  bcc loc_8DF7
  ldy #$0A
loc_8E01:
  pla
  sta (var_00EE),y
  dey
  cpy #$07
  bcs loc_8E01
loc_8E09:
  lda #$00
loc_8E0B:
  rts

; 文件调用 3 FileWrite：从读写位置起把 FH_PAGE:FH_BUFFER 里的 FH_BYTES 字节写进文件，必要时分配新簇并扩大文件长度；FH_BYTES 为 0 表示在读写位置截断文件。不检查只读属性。出口同 FileRead。Ember 实测。
file_call_write:
  jsr drive_select
  bcs loc_8E4A
  jsr block_take_length
  bne loc_8E1C
  jsr sub_8EEB
  bcc loc_8E48
  rts
loc_8E1C:
  jsr file_bytes_left
  bcc loc_8E32
  bne loc_8E4B
  txa
  bne loc_8E4B
  lda remaining_bytes_low
  cmp var_00F0
  lda remaining_bytes_high
  sbc var_00F1
  bcc loc_8E4B
loc_8E32:
  jsr sub_8CC6
  bcs loc_8E44
  lda #$03
  jsr file_transfer
  bcs loc_8E43
  jsr sub_8F46
  lda #$00
loc_8E43:
  rts
loc_8E44:
  cmp #$27
  bne loc_8E4A
loc_8E48:
  lda #$00
loc_8E4A:
  rts
loc_8E4B:
  lda #$03

; 读写文件的主体。入口：A 为命令（2 读、3 写），remaining_bytes 为要传送的字节数。分三段：先补齐当前扇区的剩余部分（file_transfer_partial_sector），再整扇区、整簇连续传送（disk_transfer_sectors），最后传送尾部不满一个扇区的部分。
file_transfer:
  sta var_00FD
  ldy #$07
  lda (var_00EE),y
  eor #$FF
  tax
  iny
  lda (var_00EE),y
  eor #$FF
  and #$01
  tay
  inx
  bne loc_8E66
  iny
  cpy #$02
  beq loc_8E81
loc_8E66:
  cpy remaining_bytes_high
  bne loc_8E6E
  cpx remaining_bytes_low
loc_8E6E:
  bcc loc_8E76
  ldy remaining_bytes_high
  ldx remaining_bytes_low
loc_8E76:
  sty chunk_bytes_high
  stx chunk_bytes_low
  jsr file_transfer_partial_sector
  bcs loc_8EEA
loc_8E81:
  lda remaining_bytes_high
  lsr a
  beq loc_8ED0
  jsr sub_898B
  bcs loc_8EEA
  asl a
  bcs loc_8E96
  beq loc_8EEA
  cmp remaining_bytes_high
  bcc loc_8E99
loc_8E96:
  lda remaining_bytes_high
loc_8E99:
  and #$FE
  sta data_800A
  lsr a
  jsr disk_transfer_sectors
  bcs loc_8EEA
  lda remaining_bytes_high
  sec
  sbc data_800A
  sta remaining_bytes_high
  ldy #$08
  clc
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  jsr carry_into_position
  ldy #$06
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  ldy #$04
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  jmp loc_8E81
loc_8ED0:
  ldx remaining_bytes_high
  txa
  ora remaining_bytes_low
  beq loc_8EE9
  ldy remaining_bytes_low
  stx chunk_bytes_high
  sty chunk_bytes_low
  jsr file_transfer_partial_sector
  bcs loc_8EEA
  lda #$00
loc_8EE9:
  clc
loc_8EEA:
  rts

sub_8EEB:
  ldy #$07
  ldx #$04
  clc
loc_8EF0:
  lda (var_00EE),y
  sbc #$00
  sta (var_00EE),y
  iny
  dex
  bne loc_8EF0
  bcs loc_8F00
  ldy #$00
  beq loc_8F0E
loc_8F00:
  txa
  jsr sub_88EA
  bcc loc_8F0A
  tax
  beq loc_8F37
  rts
loc_8F0A:
  ldx var_00F7
  ldy var_00F6
loc_8F0E:
  stx chain_cluster_high
  sty chain_cluster_low
  lda #$FF
  sta var_00FC
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
loc_8F23:
  jsr fat_entry_access
  bcs loc_8EEA
  jsr fat_value_is_last
  bcs loc_8F37
  lda #$00
  sta fat_value_low
  sta fat_value_high
  beq loc_8F23
loc_8F37:
  ldy #$07
  ldx #$04
  sec
loc_8F3C:
  lda (var_00EE),y
  adc #$00
  sta (var_00EE),y
  iny
  dex
  bne loc_8F3C

sub_8F46:
  ldy #$07
loc_8F48:
  lda (var_00EE),y
  pha
  iny
  cpy #$0B
  bne loc_8F48
  ldy #$11
loc_8F52:
  dey
  pla
  sta (var_00EE),y
  cpy #$0D
  bne loc_8F52
  ldy #$19
  lda #$01
  sta (var_00EE),y
  clc
  rts

sub_8F62:
  jsr drive_select
  ldy #$14
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  tay
  pla
  jsr dir_buffer_read
  bcs loc_8F80
  ldy #$11
  lda (var_00EE),y
  jsr dir_entry_pointer
  clc
loc_8F80:
  rts

; 文件调用 5 FileClose：把文件长度、日期、时间和属性写回目录项，并把缓冲区写回磁盘。Ember 实测。
file_call_close:
  lda data_buffer_dirty
  beq loc_8F90
  inc data_buffer_dirty
  lda #$03
  jsr data_buffer_transfer
  bcs loc_8FBF
loc_8F90:
  ldy #$1A
  lda (var_00EE),y
  beq loc_8F99
  jsr fat_buffer_flush
loc_8F99:
  ldy #$19
  lda (var_00EE),y
  beq loc_8FBD
  jsr sub_8F62
  bcs loc_8FBF
  ldy #$0B
loc_8FA6:
  lda (var_00EE),y
  pha
  iny
  cpy #$11
  bne loc_8FA6
  ldy #$20
loc_8FB0:
  dey
  pla
  sta (var_00F1),y
  cpy #$1A
  bne loc_8FB0
  jsr dir_buffer_write
  bcs loc_8FBF
loc_8FBD:
  lda #$00
loc_8FBF:
  rts

sub_8FC0:
  jsr dir_buffer_read
  bcs loc_900E
  lda var_00FA
loc_8FC7:
  jsr dir_entry_pointer
  ldy #$00
  lda (var_00F1),y
  beq loc_900B
  cmp #$E5
  beq loc_8FFD
loc_8FD4:
  lda (var_00F4),y
  cmp #$3F
  beq loc_8FE6
  lda (var_00F1),y
  cmp #$05
  bne loc_8FE2
  lda #$E5
loc_8FE2:
  cmp (var_00F4),y
  bne loc_8FFD
loc_8FE6:
  iny
  cpy #$0B
  bne loc_8FD4
  lda (var_00F1),y
  bit data_8023
  bmi loc_8FFB
  bne loc_8FFD
  ora data_8023
  and #$08
  beq loc_8FFD
loc_8FFB:
  clc
  rts
loc_8FFD:
  lda var_00FA
  clc
  adc #$10
  sta var_00FA
  bne loc_8FC7
  jsr dir_next_sector_current
  bcc sub_8FC0
loc_900B:
  lda #$12
  sec
loc_900E:
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

file_call_fetch_name:
  lda file_call_flags,x
  asl a
  bpl loc_90A9
  pha
  ldy #$04
  lda (var_00EE),y
  sta var_00FF
  dey
  lda (var_00EE),y
  sta var_00FE
  lda #$00
  sta var_00F0
  ldy #$05
loc_9072:
  lda #$00
  sta var_00F8
loc_9076:
  sty var_00F3
  ldy #$02
  lda (var_00EE),y
  tax
  lda #$20
  ldy var_00F0
  jsr BIOS_ENTRY
  cmp #$20
  bne loc_9090
  ldy var_00F0
  beq loc_90AB
  dec var_00F8
  bne loc_9099
loc_9090:
  bit var_00F8
  bvs loc_90AB
  jsr sub_A623
  bcs loc_90AB
loc_9099:
  ldy var_00F3
  sta (var_00EE),y
  inc var_00F0
  iny
  cpy #$0D
  beq loc_9072
  cpy #$10
  bne loc_9076
  pla
loc_90A9:
  clc
  rts
loc_90AB:
  pla
  lda #$02
  sec
  rts

file_call_jump_to_handler:
  ldy #$00
  lda (var_00EE),y
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
  lda (var_00EE),y
  cmp #$19
  bcc loc_90CE
  lda #$01
  jmp file_call_leave
loc_90CE:
  cmp #$09
  beq loc_9132
  tax
  ldy #$01
  lda (var_00EE),y
  bne loc_90DE
  lda data_8003
  sta (var_00EE),y
loc_90DE:
  cmp #$FF
  bne loc_90E5
  lda data_8004
loc_90E5:
  cmp logical_drive_count
  bcc loc_90F1
  bne loc_9138
  ldy drive_info_y
  bne loc_9146
loc_90F1:
  sec
  sbc #$01
  cmp #$02
  bcc loc_90FE
  sbc #$02
  ora #$80
  bne loc_9103
loc_90FE:
  ldy drive_info_a
  beq loc_9138
loc_9103:
  sta current_drive
  lda file_call_flags,x
  bpl loc_9115
  jsr sub_83A8
  bcs loc_913A
  ldy #$00
  lda (var_00EE),y
  tax
loc_9115:
  jsr file_call_fetch_name
  bcs loc_913A
  asl a
  bpl loc_9132
  cpx #$16
  beq loc_9125
  cpx #$18
  bne loc_912D
loc_9125:
  ldy #$05
  lda (var_00EE),y
  ldy #$16
  sta (var_00EE),y
loc_912D:
  jsr sub_9DEA
  bcs loc_913A
loc_9132:
  jsr file_call_jump_to_handler
  jmp file_call_leave
loc_9138:
  lda #$0F
loc_913A:
  jmp file_call_leave

sub_913D:
  lda #$0D
  ldx var_00EF
  ldy var_00EE
  jmp BIOS_ENTRY
loc_9146:
  cpx #$0D
  bne loc_9163
  ldy #$0D
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  pha
  jsr file_call_fetch_name
  ldy #$03
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
  bcs loc_91AD
  bcc loc_91A6
loc_9163:
  jsr file_call_fetch_name
  bcs loc_91AD
  ldy #$00
  lda (var_00EE),y
  cmp #$06
  beq loc_9190
  cmp #$07
  beq loc_9190
  cmp #$16
  bne loc_91A6
  ldy #$03
  lda (var_00EE),y
  tax
  iny
  lda (var_00EE),y
  ldy #$06
  sta (var_00EE),y
  dey
  lda (var_00EE),y
  pha
  txa
  sta (var_00EE),y
  pla
  ldy #$07
  sta (var_00EE),y
loc_9190:
  ldx var_00EF
  ldy var_00EE
  jsr sub_91B0
  jsr sub_913D
  ldx caller_block_high
  ldy caller_block_low
  jsr sub_91B0
  jmp loc_91A9
loc_91A6:
  jsr sub_913D
loc_91A9:
  ldy #$0B
  lda (var_00EE),y
loc_91AD:
  jmp file_call_leave

sub_91B0:
  tya
  pha
  lda caller_page
  ldy #$02
  sta (var_00EE),y
  pla
  clc
  adc #$17
  ldy #$03
  sta (var_00EE),y
  txa
  adc #$00
  iny
  sta (var_00EE),y
  rts
  .res 56, $FF
; 数据缓冲区（512 字节，$9200-$93FF），引导扇区也读到这里解析
sector_buffer:
  .byte $FF, $FF
data_9202:
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
data_921B:
  .res 5, $FF
boot_total_sectors32_0:
  .byte $FF
boot_total_sectors32_1:
  .byte $FF
boot_total_sectors32_2:
  .res 24, $FF
data_923A:
  .byte $FF
data_923B:
  .res 197, $FF
data_9300:
  .res 256, $FF
; 目录缓冲区（512 字节，$9400-$95FF）
dir_buffer:
  .res 58, $FF
data_943A:
  .byte $FF
data_943B:
  .res 197, $FF
data_9500:
  .res 256, $FF
; FAT 缓冲区（512 字节，$9600-$97FF）
fat_buffer:
  .res 256, $FF
data_9700:
  .res 256, $FF
