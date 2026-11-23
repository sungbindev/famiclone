; 模块：fat-directory
; FAT12 与目录：簇与扇区换算、FAT 缓存、目录项查找

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_07A8"
; 段 0，$83A8-$8A14
sub_83A8:
  jsr drive_select
  bcs loc_83F2
  ldx current_drive
  bmi loc_83EC
  lda data_8025,x
  beq loc_83EC
  inc data_8025,x
  lda sectors_per_track
  sta var_00E9
  lda #$06
  jsr bios_disk_command
  lda current_dir_path
  beq loc_83E4
  lda dos_page
  ldx #$80
  ldy #$32
  sta data_8029
  stx data_8028
  sty data_8027
  ldx #$FF
  jsr sub_A28A
  bcc loc_83E7
  cmp #$03
  bne loc_83F1
loc_83E4:
  jsr sub_A27B
loc_83E7:
  jsr sub_A316
  clc
  rts
loc_83EC:
  jsr sub_A2F8
  clc
  rts
loc_83F1:
  sec
loc_83F2:
  lda #$03
  rts

; 算出文件通道块里文件名字段（偏移 5）的地址，放入 $F4:$F5（通道块地址在 $EE:$EF）。
block_name_pointer:
  lda #$05
  clc
  adc var_00EE
  sta var_00F4
  lda #$00
  adc var_00EF
  sta var_00F5
  rts

; 目录项指针：dir_buffer（$9400）加 A 乘 2，放入 $F1:$F2。A 是目录项在扇区里的字节偏移除以 2（即通道块里的 FH_DIR_ENTRY）。
dir_entry_pointer:
  asl a
  sta var_00F1
  lda #$94
  adc #$00
  sta var_00F2
  rts

; 把 dir_buffer 写回磁盘上 dir_buffer_lba_0-2 指定的扇区。
dir_buffer_write:
  lda #$03
  pha
  lda dir_buffer_lba_2
  ldx dir_buffer_lba_1
  ldy dir_buffer_lba_0
  jmp loc_843C

; 把逻辑扇区 A:X:Y（Y 为低字节）读进目录缓冲区 dir_buffer（$9400）：已缓冲该扇区时直接返回 C=0；读失败时清除缓冲记录。
dir_buffer_read:
  cmp dir_buffer_lba_2
  bne loc_842D
  cpx dir_buffer_lba_1
  bne loc_8430
  cpy dir_buffer_lba_0
  bne loc_8433
  clc
  rts
loc_842D:
  sta dir_buffer_lba_2
loc_8430:
  stx dir_buffer_lba_1
loc_8433:
  sty dir_buffer_lba_0
  lda #$02
  pha
  lda dir_buffer_lba_2
loc_843C:
  sty var_00E7
  pha
  txa
  tay
  pla
  tax
  lda #$94
  sta var_00EC
  lda #$00
  sta var_00EB
  pla
  jsr disk_transfer_sector
  bcc loc_8456
  ldy #$FF
  sty dir_buffer_lba_2
loc_8456:
  rts

; 开始扫描目录。入口：A 的位 3=1 且当前在子目录里时从子目录首簇起，否则从根目录起。出口：A:X:Y 为起始逻辑扇区号（Y 低），$FB 为该段的扇区数，$FA 清零。
dir_scan_start:
  pha
  jsr block_name_pointer
  lda #$00
  sta data_8022
  sta var_00FA
  lda sectors_per_cluster
  sta var_00FB
  pla
  and #$08
  beq loc_8471
  bit in_subdirectory
  bmi sub_8480
loc_8471:
  lda root_dir_sectors
  sta var_00FB
  lda #$00
  ldx root_dir_lba_high
  ldy root_dir_lba_low
  clc
  rts

sub_8480:
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low

; 簇号 X:Y 换成逻辑扇区号（存入 sector_lba_0-2 并返回 A:X:Y）：簇号减 2，乘 sectors_per_cluster，加 data_area_lba；簇号超出 cluster_limit 时 A=$1B（扇区未找到）。
cluster_to_lba:
  tya
  sec
  sbc #$02
  sta var_00F1
  tay
  txa
  sbc #$00
  sta var_00F2
  cmp cluster_limit_high
  beq loc_8499
  bcc loc_84A1
loc_8499:
  cpy cluster_limit_low
  bcc loc_84A1
  lda #$1B
  rts
loc_84A1:
  lda #$00
  sta var_00F0
  lda sectors_per_cluster
loc_84A8:
  lsr a
  beq loc_84B3
  asl var_00F1
  rol var_00F2
  rol var_00F0
  bcc loc_84A8
loc_84B3:
  lda var_00F1
  clc
  adc data_area_lba_low
  tay
  sta sector_lba_0
  lda var_00F2
  adc data_area_lba_high
  sta sector_lba_1
  tax
  lda var_00F0
  adc #$00
  sta sector_lba_2
  rts

; 在当前目录里查找与文件通道块里文件名相同的目录项；入口 A、Y 是两个属性过滤值（data_8023、data_8024，具体用法未确认）。找到 C=0，$FA 为目录项偏移；找不到 C=1、A=$02；查找时会记下第一个空闲项（$FE）。
dir_find_entry:
  ldx #$FF
  stx var_00FE

dir_find_entry_with_mask:
  sta data_8023
  sty data_8024
  lda #$08
  jsr dir_scan_start
  bcs loc_84F4
loc_84DF:
  jsr dir_buffer_read
  bcs loc_84F4
  jsr dir_compare_entries
  bcc loc_84F4
  bne loc_84F1
  jsr dir_next_sector_current
  bcc loc_84DF
  rts
loc_84F1:
  lda #$02
  sec
loc_84F4:
  rts

dir_next_sector_current:
  lda dir_buffer_lba_2
  ldx dir_buffer_lba_1
  ldy dir_buffer_lba_0

; 下一个目录扇区：逻辑扇区号 A:X:Y 加 1；当前簇的扇区用完时（$FB 减到 0）在子目录里沿 FAT 链转到下一簇，根目录走完则 A=$02、C=1。
dir_next_sector:
  clc
  iny
  bne loc_8507
  inx
  bne loc_8507
  adc #$01
loc_8507:
  dec var_00FB
  bne loc_8546
  bit in_subdirectory
  bpl loc_8543
  lda current_dir_cluster_high
  ldy current_dir_cluster_low
  sta var_00F7
  sty var_00F6
  ldx #$00
loc_851C:
  stx var_00F8
  tax
  ldy var_00F6
  jsr fat_sector_for_cluster
  bcs loc_8546
  jsr sub_8885
  bcs loc_8546
  ldx var_00F8
  cpx data_8022
  inx
  bcc loc_851C
  stx data_8022
  jsr fat_cluster_is_last
  bcs loc_8543
  lda sectors_per_cluster
  sta var_00FB
  jmp cluster_to_lba
loc_8543:
  lda #$02
  sec
loc_8546:
  rts

; 在 dir_buffer 里逐项比较：目录项第 0 字节为 0 表示目录到此为止（记下空闲项），为 $E5 的是已删除项（同样记下），名字第 0 字节 $05 按 $E5 比较；11 字节全部相同才匹配，匹配后把属性按位与 data_8023、data_8024 比较（未确认），相符返回 C=0，否则 A=$05、C=1。
dir_compare_entries:
  jsr dir_entry_pointer
  ldy #$00
  lda (var_00F1),y
  beq dir_note_free_slot
  cmp #$E5
  beq loc_8578
  cmp #$05
  bne loc_855A
  lda #$E5
loc_855A:
  cmp (var_00F4),y
  bne loc_857B
  iny
  lda (var_00F1),y
  cpy #$0B
  bne loc_855A
  lda (var_00F1),y
  ora #$20
  bit data_8023
  beq loc_8585
  bit data_8024
  bne loc_8585
  sta data_8023
loc_8576:
  clc
  rts
loc_8578:
  jsr dir_note_free_slot
loc_857B:
  lda var_00FA
  clc
  adc #$10
  sta var_00FA
  bne dir_compare_entries
  rts
loc_8585:
  lda #$05
  sec
  rts

; 记下第一个空闲目录项：$FE 为 $FF（尚未记录）时，把当前目录项偏移 $FA 与目录缓冲区的扇区号 dir_buffer_lba_0-2 存到 $FE 与 free_slot_lba_0-2。
dir_note_free_slot:
  ldx var_00FE
  inx
  bne loc_859D
  lda var_00FA
  sta var_00FE
  ldx #$02
loc_8594:
  lda dir_buffer_lba_0,x
  sta free_slot_lba_0,x
  dex
  bpl loc_8594
loc_859D:
  sec
  rts

; FAT 缓冲区有修改（fat_buffer_dirty 非 0）时，把 fat_buffer 写回第一份与第二份 FAT。
fat_buffer_flush:
  lda fat_buffer_dirty
  beq loc_8576
  inc fat_buffer_dirty
  ldx #$96
  ldy #$00
  stx var_00EC
  sty var_00EB
  ldx #$00
  lda fat_buffer_sector
  sta var_00E7
  lda #$03
  jsr disk_transfer_sector
  bcs loc_859D
  ldx #$00
  ldy #$00
  lda fat_buffer_sector
  sec
  sbc fat1_start_sector
  clc
  adc fat2_start_sector
  sta var_00E7
  bcc loc_85D1
  iny
loc_85D1:
  lda #$03
  jmp disk_transfer_sector

buffers_flush_all:
  jsr fat_buffer_flush
  bcs loc_85F7

; 数据缓冲区有修改（data_buffer_dirty 为负）时把 sector_buffer 写回磁盘。
data_buffer_flush:
  bit data_buffer_dirty
  bpl loc_8576
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
  bcc loc_85F7
  lda disk_last_error
loc_85F7:
  rts

; 把 FAT 的第 A 个扇区读进 fat_buffer（$9600）：先把旧内容写回，再读入，并记在 fat_buffer_sector（读失败时为 $FF）。
fat_buffer_load:
  pha
  jsr fat_buffer_flush
  pla
  bcs loc_8615
  ldx #$96
  ldy #$00
  stx var_00EC
  sty var_00EB
  sta fat_buffer_sector
  sta var_00E7
  ldx #$00
  lda #$02
  jsr disk_transfer_sector
  bcc loc_861A
loc_8615:
  ldy #$FF
  sty fat_buffer_sector
loc_861A:
  rts

sub_861B:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc var_00F6
  tay
  txa
  adc var_00F7
  tax
  lsr a
  lsr a
  rts

sub_862C:
  ldx var_00F7
  ldy var_00F6
  jsr fat_sector_for_cluster
  bcs loc_8651
  beq loc_8651
  sty var_00F0

sub_8639:
  ldy #$15
  lda var_00F4
  sta (var_00EE),y
  iny
  lda var_00F5
  sta (var_00EE),y
  ldy #$17
  lda var_00F6
  sta (var_00EE),y
  iny
  lda var_00F7
  sta (var_00EE),y
  ldy var_00F0
loc_8651:
  rts

fat_cluster_is_last:
  ldx var_00F7
  ldy var_00F6
  jmp fat_xy_is_last

; FAT 项的值是否表示链结束（不小于 $FF0 或 FAT16 的 $FFF0）：C=1 为结束；fat_xy_is_last 用 X:Y 作为值。
fat_value_is_last:
  ldx fat_value_high
  ldy fat_value_low

fat_xy_is_last:
  cpx fat16_flag
  bcc loc_8666
  cpy #$F0
loc_8666:
  rts

; 把簇 X:Y 的 FAT 项所在的 FAT 扇区读进 fat_buffer；重新读过返回 A=1，已在缓冲区里返回 A=0。
fat_sector_for_cluster:
  txa
  bit fat16_flag
  bmi loc_8670
  jsr sub_861B
loc_8670:
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  clc
  beq loc_8689
  stx var_00F0
  sty var_00F3
  jsr fat_buffer_load
  ldx var_00F0
  ldy var_00F3
  bcs loc_8689
  lda #$01
loc_8689:
  rts
data_868A:
  .byte $F0
data_868B:
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
  sta var_00FC
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
  ldx chain_cluster_high
  ldy chain_cluster_low

; 读写 FAT 项。入口：簇号 X:Y；$FC 位 7=0 读出，位 7=1 写入（新值在 fat_value_low、fat_value_high，原值换到这里）。簇号为 0 表示文件的首簇，它不在 FAT 里，存在文件通道块偏移 $0B、$0C。FAT12 按 12 位拆包（半字节 fat_odd_flag），FAT16 直接取 2 字节。出口：读出的值在 fat_value_low、fat_value_high。
fat_entry_access:
  stx var_00F5
  sty var_00F4
  txa
  ora var_00F4
  bne loc_8702
  bit var_00FC
  bpl loc_86E0
  jsr fat_value_is_last
  bcc loc_86BF
  lda #$00
  sta fat_value_low
  sta fat_value_high
loc_86BF:
  ldy #$0B
  lda (var_00EE),y
  tax
  lda fat_value_low
  sta (var_00EE),y
  stx fat_value_low
  iny
  lda (var_00EE),y
  tax
  lda fat_value_high
  sta (var_00EE),y
  stx fat_value_high
  ldy #$19
  lda #$01
  sta (var_00EE),y
  bne loc_86ED
loc_86E0:
  ldy #$0B
  lda (var_00EE),y
  sta fat_value_low
  iny
  lda (var_00EE),y
  sta fat_value_high
loc_86ED:
  lda fat_value_low
  ora fat_value_high
  bne loc_8700
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
loc_8700:
  clc
  rts
loc_8702:
  bit fat16_flag
  bpl loc_875A
  txa
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  beq loc_8716
  jsr fat_buffer_load
  bcs loc_8759
loc_8716:
  ldx #$96
  ldy #$00
  bit var_00F4
  bpl loc_871F
  inx
loc_871F:
  lda var_00F4
  stx var_00F5
  sty var_00F4
  asl a
  tay
  bit var_00FC
  bpl loc_874D
  lda (var_00F4),y
  tax
  lda fat_value_low
  sta (var_00F4),y
  stx fat_value_low
  iny
  lda (var_00F4),y
  tax
  lda fat_value_high
  sta (var_00F4),y
  stx fat_value_high
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (var_00EE),y
  clc
  rts
loc_874D:
  lda (var_00F4),y
  sta fat_value_low
  iny
  lda (var_00F4),y
  sta fat_value_high
  clc
loc_8759:
  rts
loc_875A:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc var_00F4
  sta var_00F0
  txa
  adc var_00F5
  lsr a
  sta var_00F3
  ror var_00F0
  lsr a
  clc
  adc fat1_start_sector
  cmp fat_buffer_sector
  beq loc_877C
  jsr fat_buffer_load
  bcs loc_87DB
loc_877C:
  lda var_00F4
  and #$01
  sta fat_odd_flag
  beq loc_8790
  ldx #$04
loc_8787:
  asl fat_value_low
  rol fat_value_high
  dex
  bne loc_8787
loc_8790:
  ldx #$96
  ldy #$00
  stx var_00F5
  sty var_00F4
  lda var_00F3
  ldy var_00F0
  lsr a
  bcc loc_87A1
  inc var_00F5
loc_87A1:
  lda (var_00F4),y
  bit var_00FC
  bpl loc_87C1
  pha
  ldx fat_odd_flag
  and data_868B,x
  ora fat_value_low
  sta (var_00F4),y
  tya
  tax
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (var_00EE),y
  txa
  tay
  pla
loc_87C1:
  sta fat_value_low
  iny
  bne loc_87DE
  lda #$96
  cmp var_00F5
  beq loc_87DC
  dec var_00F5
  ldx fat_buffer_sector
  inx
  txa
  jsr fat_buffer_load
  ldy #$00
  bcc loc_87DE
loc_87DB:
  rts
loc_87DC:
  inc var_00F5
loc_87DE:
  lda (var_00F4),y
  bit var_00FC
  bpl loc_87FA
  pha
  ldx fat_odd_flag
  and data_868A,x
  ora fat_value_high
  sta (var_00F4),y
  lda #$FF
  sta fat_buffer_dirty
  ldy #$1A
  sta (var_00EE),y
  pla
loc_87FA:
  sta fat_value_high
  lda fat_odd_flag
  beq loc_880F
  ldx #$04
loc_8804:
  lsr fat_value_high
  ror fat_value_low
  dex
  bne loc_8804
  clc
  rts
loc_880F:
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
  bne loc_8823
  inx
loc_8823:
  cpx cluster_limit_high
  bne loc_8831
  cpy cluster_limit_low
  bne loc_8831
  ldx #$00
  ldy #$02
loc_8831:
  stx free_search_cluster_high
  sty free_search_cluster_low
  cpx var_00F7
  bne loc_883D
  cpy var_00F6
loc_883D:
  rts

; 从 free_search_cluster 起顺序找空闲簇（FAT 项为 0）；整张磁盘没有空闲簇时 A=$27（磁盘已满）、C=1。
fat_find_free:
  lda #$00
  sta var_00FC
  ldx free_search_cluster_high
  ldy free_search_cluster_low
loc_8848:
  jsr fat_entry_access
  bcs loc_885D
  lda fat_value_low
  ora fat_value_high
  beq loc_885D
  jsr fat_advance_search
  bne loc_8848
  lda #$27
  sec
loc_885D:
  rts
chain_cluster_low:
  .byte $00
chain_cluster_high:
  .byte $00

; 分配一个空闲簇：用 fat_find_free 找到后，把前一簇（chain_cluster）的链接改成它，并把它标成链结束。
fat_allocate_cluster:
  jsr fat_find_free
  bcs loc_8884
  lda #$FF
  sta var_00FC
  ldx chain_cluster_high
  ldy chain_cluster_low
  lda free_search_cluster_high
  sta chain_cluster_high
  sta fat_value_high
  lda free_search_cluster_low
  sta chain_cluster_low
  sta fat_value_low
  jsr fat_entry_access
loc_8884:
  rts

sub_8885:
  lda #$00

fat_read_value:
  sta var_00F0
  bit fat16_flag
  bpl loc_8896
  lsr var_00F7
  tya
  asl a
  tay
  jmp loc_8899
loc_8896:
  jsr sub_88DF
loc_8899:
  bcs loc_88A8
  lda fat_buffer,y
  sta var_00F6
  iny
  beq loc_88C5
  lda fat_buffer,y
  bcc loc_88C8
loc_88A8:
  lda data_9700,y
  sta var_00F6
  iny
  bne loc_88C5
  bit var_00F0
  bmi loc_88DE
  ldx fat_buffer_sector
  inx
  txa
  jsr fat_buffer_load
  bcs loc_88DE
  inc var_00F0
  lda fat_buffer
  bcc loc_88C8
loc_88C5:
  lda data_9700,y
loc_88C8:
  bit var_00F7
  bpl loc_88D8
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
loc_88D8:
  and fat16_flag
  sta var_00F7
  clc
loc_88DE:
  rts

sub_88DF:
  txa
  lsr a
  tax
  tya
  ror a
  tay
  ror var_00F7
  txa
  lsr a
  rts

sub_88EA:
  pha
  ldy #$0A
  lda (var_00EE),y
  sta var_00F3
  dey
  lda (var_00EE),y
  sta var_00F2
  dey
  lda (var_00EE),y
  sta var_00F1
  lda sectors_per_cluster
loc_88FE:
  lsr var_00F3
  ror var_00F2
  ror var_00F1
  lsr a
  bne loc_88FE
  lda var_00F1
  ldy #$15
  cmp (var_00EE),y
  iny
  lda var_00F2
  sbc (var_00EE),y
  bcs loc_891E
  lda #$00
  sta var_00F5
  sta var_00F4
  ldy #$0B
  bne loc_8929
loc_891E:
  lda (var_00EE),y
  sta var_00F5
  dey
  lda (var_00EE),y
  sta var_00F4
  ldy #$17
loc_8929:
  lda (var_00EE),y
  sta var_00F6
  iny
  lda (var_00EE),y
  sta var_00F7
  pla
  beq loc_893A
  jsr sub_862C
  bcs loc_8964
loc_893A:
  lda var_00F4
  cmp var_00F1
  bne loc_8946
  lda var_00F5
  cmp var_00F2
  beq loc_8965
loc_8946:
  jsr sub_862C
  bcs loc_8964
  jsr sub_8885
  bcs loc_8964
  inc var_00F4
  bne loc_8956
  inc var_00F5
loc_8956:
  lda var_00F0
  beq loc_895D
  jsr sub_8639
loc_895D:
  jsr fat_cluster_is_last
  bcc loc_893A
  lda #$00
loc_8964:
  rts
loc_8965:
  ldx var_00F7
  ldy var_00F6
  jsr cluster_to_lba
  lda sectors_per_cluster
  asl a
  sbc #$00
  ldy #$08
  and (var_00EE),y
  lsr a
  tax
  clc
  adc sector_lba_0
  sta sector_lba_0
  bcc loc_898A
  clc
  inc sector_lba_1
  bne loc_898A
  inc sector_lba_2
loc_898A:
  rts

sub_898B:
  sta sectors_left
  lda sectors_per_cluster
  asl a
  sec
  sbc #$01
  ldy #$08
  and (var_00EE),y
  lsr a
  clc
  adc sectors_left
  adc sectors_per_cluster
  sbc #$00
  sta sectors_left
  lda sectors_per_cluster
loc_89A9:
  lsr a
  beq loc_89B1
  lsr sectors_left
  bne loc_89A9
loc_89B1:
  lda #$01
  jsr sub_88EA
  bcc loc_89BB
  cmp #$01
  rts
loc_89BB:
  stx var_00ED
  lda #$01
  sta data_800A
loc_89C2:
  dec sectors_left
  beq loc_8A01
  ldx var_00F7
  ldy var_00F6
  jsr fat_xy_is_last
  bcs loc_8A01
  stx var_00F9
  sty var_00F8
  txa
  bit fat16_flag
  bmi loc_89DE
  jsr sub_861B
  clc
loc_89DE:
  adc fat1_start_sector
  cmp fat_buffer_sector
  bne loc_8A01
  lda #$80
  jsr fat_read_value
  bcs loc_8A01
  tax
  ldy var_00F6
  bne loc_89F3
  dex
loc_89F3:
  dey
  cpy var_00F8
  bne loc_8A01
  cpx var_00F9
  bne loc_8A01
  inc data_800A
  bcs loc_89C2
loc_8A01:
  lda sectors_per_cluster
loc_8A04:
  lsr a
  beq loc_8A0D
  asl data_800A
  jmp loc_8A04
loc_8A0D:
  lda data_800A
  sec
  sbc var_00ED
  clc
  rts
