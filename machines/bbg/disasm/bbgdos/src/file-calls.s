; 模块：file-calls
; 路径解析与其余文件调用：查找文件、定位、删除、改名、属性、子目录

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2177"
; 段 0，$9D77-$A5BB
sub_9D77:
  jsr buffers_flush_all
  bcs sub_9D99
  lda mounted_drive
  cmp #$FF
  beq sub_9D99
  jsr drive_state_offset_of
  ldy #$00
loc_9D88:
  lda current_dir_path,y
  sta data_9C33,x
  inx
  iny
  cpy #$50
  bne loc_9D88
  lda #$FF
  sta data_9C33,x

sub_9D99:
  ldx #$FF
  stx fat_buffer_sector
  stx dir_buffer_lba_2
  stx data_buffer_lba_2
  inx
  stx data_buffer_dirty
  stx fat_buffer_dirty
  clc
  rts

sub_9DAD:
  lda #$FF
  sta data_802A

sub_9DB2:
  cpx data_802A
  beq loc_9DD5
  stx var_00F8
  sty var_00F9
  lda data_8028
  sta var_00FF
  lda data_8027
  sta var_00FE
  lda #$20
  ldx data_8029
  ldy var_00F8
  jsr BIOS_ENTRY
  ldx var_00F8
  ldy var_00F9
  cmp #$00
loc_9DD5:
  rts

sub_9DD6:
  ldy #$02
  lda (var_00EE),y
  sta data_8029
  iny
  lda (var_00EE),y
  sta data_8027
  iny
  lda (var_00EE),y
  sta data_8028
  rts

sub_9DEA:
  jsr sub_9DD6
  ldx #$00
  stx var_00F3
loc_9DF1:
  jsr sub_9DAD
  beq loc_9DFF
  inx
  cmp #$5C
  bne loc_9DF1
  stx var_00F3
  beq loc_9DF1
loc_9DFF:
  ldx var_00F3
  beq loc_9E14
  dex
  beq loc_9E0F
  jsr sub_A28A
  bcs loc_9E1C
  ldx var_00F3
  bne loc_9E14
loc_9E0F:
  jsr sub_A27B
  ldx #$01
loc_9E14:
  jsr sub_9DAD
  beq loc_9E1D
  jsr sub_A151
loc_9E1C:
  rts
loc_9E1D:
  lda #$03
  rts

sub_9E20:
  ldy #$05
  ldx #$01
loc_9E24:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E34
  inx
  iny
  cpy #$0D
  bne loc_9E24
loc_9E34:
  ldy #$0D
  lda (var_00EE),y
  cmp #$20
  beq loc_9E4D
  inx
loc_9E3D:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E4D
  inx
  iny
  cpy #$10
  bne loc_9E3D
loc_9E4D:
  clc
  rts
loc_9E4F:
  lda #$05
  rts

; 文件调用 24（手册未列）：按通道块偏移 $16 的值改写目录项属性。Ember 实测对已打开的文件调用返回 A=$05（拒绝访问），用法未查清。
file_call_write_attribute:
  ldy #$16
  lda (var_00EE),y
  and #$D8
  bne loc_9E79
  lda #$37
  ldy #$08
  jsr dir_find_entry
  bcs loc_9E78
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ldy #$16
  ora (var_00EE),y
  ldy #$0B
  sta (var_00F1),y
  jsr dir_buffer_write
  bcs loc_9E78
  lda #$00
loc_9E78:
  rts
loc_9E79:
  lda #$05
  rts

; 文件调用 22 FilePathFirst：按路径查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名，偏移 $16 是属性掩码（取 $A1 的补码与掩码合成）；查找状态记在偏移 $2B 起（页号、$9BF5 与当前目录簇）。
; 出口与 FileFirst 相同，之后用 FileNext 继续。
file_call_path_first:
  ldy #$16
  lda (var_00EE),y
  tax
  and #$10
  beq loc_9E89
  txa
  and #$F7
  tax
loc_9E89:
  txa
  ora #$A1
  eor #$FF
  jsr sub_9F22
  pha
  ldy #$2B
  lda data_A53A
  sta (var_00EE),y
  iny
  lda #$F5
  sta (var_00EE),y
  iny
  lda #$9B
  sta (var_00EE),y
  lda #$00
  tax
  bit in_subdirectory
  bpl loc_9EB1
  ldx current_dir_cluster_high
  lda current_dir_cluster_low
loc_9EB1:
  ldy #$2E
  sta (var_00EE),y
  iny
  txa
  sta (var_00EE),y
  pla
  rts

; 文件调用 8 FileCreate、21 FilePathCreate：建立文件，建立后文件已打开、长度为 0。文件名含通配符、与目录或卷标同名时 A=$05。Ember 实测：建立后可写入并关闭，重新打开长度正确。
file_call_create:
  jsr sub_9E20
  bcs loc_9EE4
  lda #$3F
  ldy #$00
  jsr file_open_common
  bcs loc_9EE5
  lda data_8023
  and #$18
  beq loc_9ED3
  lda #$05
  rts
loc_9ED3:
  jsr sub_8EEB
  bcs loc_9EE4
  lda #$20
  jsr dir_entry_init
  jsr dir_buffer_write
  bcs loc_9EE4
  lda #$00
loc_9EE4:
  rts
loc_9EE5:
  cmp #$02
  bne loc_9EF5
  ldx var_00FE
  inx
  bne loc_9EFB
  bit in_subdirectory
  bmi loc_9EF6
  lda #$05
loc_9EF5:
  rts
loc_9EF6:
  jsr sub_8B0C
  bcs loc_9EF5
loc_9EFB:
  jsr sub_8AEF
  bcs loc_9EF5
  ldx #$20
  jsr dir_entry_init
  jsr dir_buffer_write
  bcs loc_9EF5
  jmp loc_8BBA

sub_9F0D:
  lda caller_block_low
  clc
  adc #$17
  ldy #$03
  sta (var_00EE),y
  lda caller_block_high
  adc #$00
  iny
  sta (var_00EE),y
  rts

; 文件调用 6 FileFirst：查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名（'?' 通配），偏移 $16 是属性掩码（本入口先置为 $FF）。
; 出口：通道块偏移 3、4 指向偏移 $17 的查找结果区，其中是目录项副本；查找状态记在偏移 $0F-$15 与 $2B 起，供 FileNext 继续使用。
file_call_first:
  lda #$FF

sub_9F22:
  ldy #$16
  sta (var_00EE),y
  sta data_8023
  pha
  jsr sub_9F0D
  pla
  jsr dir_scan_start
  bcs loc_9F38
  jsr sub_8FC0
  bcc loc_9F80
loc_9F38:
  rts

; 文件调用 7 FileNext：接着 FileFirst 的状态查找下一个匹配的文件；没有更多文件时返回出错码 $12（SearchEnding）。
file_call_next:
  ldy #$16
  lda (var_00EE),y
  sta data_8023
  jsr sub_9F0D
  jsr drive_select
  jsr block_name_pointer
  ldy #$15
  lda (var_00EE),y
  sta data_8022
  dey
  lda (var_00EE),y
  sta var_00FB
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  clc
  adc #$10
  sta var_00FA
  pla
  tay
  pla
  bcc loc_9F7B
  jsr dir_next_sector
  bcc loc_9F7B
  cmp #$02
  bne loc_9F79
  lda #$12
loc_9F79:
  sec
  rts
loc_9F7B:
  jsr sub_8FC0
  bcs loc_9F79
loc_9F80:
  ldy #$15
  lda data_8022
  sta (var_00EE),y
  dey
  lda var_00FB
  sta (var_00EE),y
  dey
  lda dir_buffer_lba_2
  sta (var_00EE),y
  dey
  lda dir_buffer_lba_1
  sta (var_00EE),y
  dey
  lda dir_buffer_lba_0
  sta (var_00EE),y
  dey
  lda var_00FA
  sta (var_00EE),y
  ldy #$0B
  lda (var_00F1),y
  pha
  ldy #$16
loc_9FAA:
  lda (var_00F1),y
  pha
  iny
  cpy #$1A
  bne loc_9FAA
  ldy #$1C
loc_9FB4:
  lda (var_00F1),y
  pha
  iny
  cpy #$20
  bne loc_9FB4
  ldy #$20
loc_9FBE:
  dey
  pla
  sta (var_00EE),y
  cpy #$17
  bne loc_9FBE
  ldy #$00
loc_9FC8:
  lda (var_00F1),y
  pha
  iny
  cpy #$0B
  bne loc_9FC8
  ldy #$2B
loc_9FD2:
  dey
  pla
  sta (var_00EE),y
  cpy #$20
  bne loc_9FD2
  lda #$00
  rts

; 文件调用 9 FileDriver：FH_DRIVE 为 0 时读取默认盘号（写回 FH_DRIVE）与逻辑驱动器数（偏移 2），否则设置默认盘号（1 为 A 盘）。Ember 实测读取返回盘号 1、驱动器数 2。
file_call_set_drive:
  ldy #$02
  lda logical_drive_count
  sta (var_00EE),y
  ldy #$01
  lda (var_00EE),y
  beq loc_9FFD
  cmp logical_drive_count
  bcc loc_9FF1
  bne loc_A00C
loc_9FF1:
  cmp #$03
  bcs loc_9FFA
  ldy drive_info_a
  beq loc_A00C
loc_9FFA:
  sta data_8003
loc_9FFD:
  lda data_8003
  sta (var_00EE),y
  ldy #$04
  lda data_802F
  sta (var_00EE),y
  lda #$00
  rts
loc_A00C:
  lda #$0F
  rts

; 文件调用 4 FileSeek：把读写位置改为通道块偏移 7 起的 4 字节值。Ember 实测。
file_call_seek:
  ldx #$03
  ldy #$0D
loc_A013:
  lda (var_00EE),y
  sta var_00F1,x
  iny
  dex
  bpl loc_A013
  ldy #$07
  ldx #$03
  sec
loc_A020:
  lda var_00F1,x
  sbc (var_00EE),y
  iny
  dex
  bpl loc_A020
  bcs loc_A036
  ldx #$03
  ldy #$07
loc_A02E:
  lda var_00F1,x
  sta (var_00EE),y
  iny
  dex
  bpl loc_A02E
loc_A036:
  lda #$00
  rts

; 文件调用 11 DriveSize：按 FH_DRIVE 返回每簇扇区数（偏移 3）、每扇区字节数（偏移 5）、空闲簇数（偏移 7）、总簇数（偏移 9）。Ember 实测：1.44MB 系统盘每簇 4 个扇区、每扇区 512 字节、总簇数 715。
file_call_drive_size:
  ldx #$00
  ldy #$02
  stx var_00F5
  sty var_00F4
  stx var_00F2
  stx var_00F1
loc_A045:
  stx var_00F7
  sty var_00F6
  jsr fat_sector_for_cluster
  bcs loc_A0A5
  jsr sub_8885
  bcs loc_A0A5
  ora var_00F6
  bne loc_A05D
  inc var_00F1
  bne loc_A05D
  inc var_00F2
loc_A05D:
  inc var_00F4
  bne loc_A063
  inc var_00F5
loc_A063:
  ldx var_00F5
  ldy var_00F4
  cpx cluster_limit_high
  bne loc_A045
  cpy cluster_limit_low
  bne loc_A045
  ldy #$06
  lda #$02
  sta (var_00EE),y
  lda #$00
  dey
  sta (var_00EE),y
  ldy #$04
  sta (var_00EE),y
  lda sectors_per_cluster
  dey
  sta (var_00EE),y
  ldy #$09
  lda cluster_limit_low
  sec
  sbc #$02
  sta (var_00EE),y
  iny
  lda cluster_limit_high
  sbc #$00
  sta (var_00EE),y
  ldy #$08
  lda var_00F2
  sta (var_00EE),y
  dey
  lda var_00F1
  sta (var_00EE),y
  lda #$00
loc_A0A5:
  rts

; 文件调用 10 FileDelete、23 FilePathDelete：删除文件（目录项标成已删除并释放簇链），不检查只读属性。Ember 实测。
file_call_delete:
  jsr file_call_open
  bcs loc_A0C9
  jsr sub_8EEB
  bcs loc_A0C9
  jsr fat_buffer_flush
  bcs loc_A0C9
  ldy #$11
  lda (var_00EE),y
  jsr dir_entry_pointer
  lda #$E5
  ldy #$00
  sta (var_00F1),y
  jsr dir_buffer_write
  bcs loc_A0C9
  lda #$00
loc_A0C9:
  rts

; 文件调用 14 FileReadAbs：从绝对扇区号（通道块偏移 7，从 0 起）起读 FH_BYTES 个扇区到 FH_PAGE:FH_BUFFER。Ember 实测读 0 号扇区得到引导扇区。
file_call_read_absolute:
  lda #$02
  bne loc_A0D0

; 文件调用 15 FileWriteAbs：参数同 FileReadAbs，写扇区。Ember 实测把 0 号扇区原样写回。
file_call_write_absolute:
  lda #$03
loc_A0D0:
  sta var_00FD
  jsr drive_select
  ldy #$09
  lda (var_00EE),y
  sta sector_lba_2
  dey
  lda (var_00EE),y
  sta sector_lba_1
  dey
  lda (var_00EE),y
  sta sector_lba_0
  ldy #$05
  lda (var_00EE),y
  jsr disk_transfer_sectors
  bcs loc_A0F3
  lda #$00
loc_A0F3:
  rts

; 文件调用 13 FileRename：给已打开的文件改名，新文件名取自 FH_PAGE:FH_BUFFER 指向的 11 字节，并复制到通道块偏移 5。Ember 实测。
file_call_rename:
  jsr sub_9E20
  bcs loc_A10B
  jsr sub_8F62
  bcs loc_A10B
  jsr block_name_pointer
  jsr dta_copy_name
  jsr dir_buffer_write
  bcs loc_A10B
  lda #$00
loc_A10B:
  rts

; 文件调用 12 FileAttribute：设置已打开文件的属性（偏移 2）、日期（偏移 3-4）、时间（偏移 5-6），关闭时写入目录项。Ember 实测。
file_call_attribute:
  jsr sub_8F62
  bcs loc_A14D
  ldy #$02
  lda (var_00EE),y
  sta var_00F0
  and #$18
  bne loc_A14E
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ora var_00F0
  sta (var_00F1),y
  ldy #$03
  lda (var_00EE),y
  tax
  iny
  lda (var_00EE),y
  ldy #$19
  sta (var_00F1),y
  dey
  txa
  sta (var_00F1),y
  ldy #$06
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  ldy #$16
  sta (var_00F1),y
  iny
  txa
  sta (var_00F1),y
  ldy #$19
  lda #$01
  sta (var_00EE),y
  lda #$00
loc_A14D:
  rts
loc_A14E:
  lda #$05
  rts

sub_A151:
  ldy #$05
  jsr sub_9DB2
  cmp #$2E
  bne loc_A174
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
  cmp #$2E
  bne loc_A170
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
loc_A170:
  lda #$02
  sec
  rts
loc_A174:
  inx
  cmp #$2A
  beq loc_A196
  cpy #$0D
  beq loc_A1A2
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  jsr sub_9DB2
  beq loc_A192
  cmp #$5C
  beq loc_A192
  cmp #$2E
  bne loc_A174
loc_A192:
  lda #$20
  bne loc_A198
loc_A196:
  lda #$3F
loc_A198:
  cpy #$0D
  beq loc_A1A2
  sta (var_00EE),y
  iny
  jmp loc_A198
loc_A1A2:
  jsr sub_9DB2
  beq loc_A1B0
  cmp #$5C
  beq loc_A1B0
  inx
  cmp #$2E
  bne loc_A1A2
loc_A1B0:
  jsr sub_9DB2
  beq loc_A1D0
  cmp #$5C
  beq loc_A1D0
  inx
  cmp #$2A
  beq loc_A1CC
  cpy #$10
  beq loc_A1DC
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  bne loc_A1B0
loc_A1CC:
  lda #$3F
  bne loc_A1D2
loc_A1D0:
  lda #$20
loc_A1D2:
  cpy #$10
  beq loc_A1DC
  sta (var_00EE),y
  iny
  jmp loc_A1D2
loc_A1DC:
  jsr sub_9DB2
  beq loc_A1E8
  cmp #$5C
  beq loc_A1E8
  inx
  bne loc_A1DC
loc_A1E8:
  clc
loc_A1E9:
  rts

sub_A1EA:
  ldx #$FF
loc_A1EC:
  inx
  lda data_9BF5,x
  bne loc_A1EC
  stx var_00F0
  lda #$5C
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
  ldy #$00
loc_A200:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A20F
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A20F:
  cpy #$08
  bne loc_A200
  lda (var_00F1),y
  cmp #$20
  beq loc_A236
  lda #$2E
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A223:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A232
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A232:
  cpy #$0B
  bne loc_A223
loc_A236:
  clc
  bcc loc_A23B
loc_A239:
  ldx var_00F0
loc_A23B:
  lda #$00
  sta data_9BF5,x
  rts

sub_A241:
  bit in_subdirectory
  bpl loc_A277
  jsr sub_8480
  jsr dir_buffer_read
  bcs loc_A27A
  ldx data_943B
  ldy data_943A
  txa
  ora data_943A
  beq sub_A27B
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  ldx #$FF
loc_A262:
  inx
  lda data_9BF5,x
  bne loc_A262
loc_A268:
  dex
  lda data_9BF5,x
  cmp #$5C
  bne loc_A268
  lda #$00
  sta data_9BF5,x
  clc
  rts
loc_A277:
  lda #$03
  sec
loc_A27A:
  rts

sub_A27B:
  lda #$00
  sta in_subdirectory
  sta data_9BF5
  sta current_dir_cluster_high
  sta current_dir_cluster_low
  rts

sub_A28A:
  stx data_802A
  ldx #$00
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A29C
  jsr sub_A27B
  inx
loc_A29C:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2B8
  inx
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2E7
  inx
  stx var_00F9
  jsr sub_A241
  bcc loc_A2E5
  rts
loc_A2B8:
  jsr sub_A151
  bcs loc_A2CD
  stx var_00F9
  lda #$10
  ldy #$08
  jsr dir_find_entry_with_mask
  bcc loc_A2CE
  cmp #$02
  beq loc_A2F5
  sec
loc_A2CD:
  rts
loc_A2CE:
  jsr sub_A1EA
  bcs loc_A2F5
  ldy #$1B
  lda (var_00F1),y
  sta current_dir_cluster_high
  dey
  lda (var_00F1),y
  sta current_dir_cluster_low
  lda #$FF
  sta in_subdirectory
loc_A2E5:
  ldx var_00F9
loc_A2E7:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A2F5
  inx
  bne loc_A29C
loc_A2F3:
  clc
  rts
loc_A2F5:
  lda #$03
  rts

sub_A2F8:
  ldx data_807B
  ldy data_807A
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  lda data_807C
  sta in_subdirectory
  ldx #$FF
loc_A30C:
  inx
  lda current_dir_path,x
  sta data_9BF5,x
  bne loc_A30C
  rts

sub_A316:
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  stx data_807B
  sty data_807A
  lda in_subdirectory
  sta data_807C
  ldx #$FF
loc_A32A:
  inx
  lda data_9BF5,x
  sta current_dir_path,x
  bne loc_A32A
  rts

; 文件调用 16 FileChangeDir：改变当前目录，路径是以 0 结尾的字符串（\ 分隔，“..”回上一级），成功后更新 current_dir_path 与目录首簇；路径不存在 A=$03。Ember 实测。
file_call_change_dir:
  jsr sub_9DD6
  ldx #$FF
  jsr sub_A28A
  bcs loc_A343
  jsr sub_A316
  lda #$00
loc_A343:
  rts

; 文件调用 17 FileMakeDir：建立目录，路径是以 0 结尾的字符串。Ember 实测。
file_call_make_dir:
  jsr sub_9E20
  bcs loc_A37B
  stx var_00F0
  ldx #$FF
loc_A34D:
  inx
  lda data_9BF5,x
  bne loc_A34D
  txa
  adc var_00F0
  cmp #$3E
  bcs loc_A363
  lda #$3F
  ldy #$00
  jsr dir_find_entry
  bcs loc_A366
loc_A363:
  lda #$05
  rts
loc_A366:
  cmp #$02
  bne loc_A37B
  ldx var_00FE
  inx
  bne loc_A381
  bit in_subdirectory
  bmi loc_A37C
  lda #$52
  sta data_8031
  lda #$05
loc_A37B:
  rts
loc_A37C:
  jsr sub_8B0C
  bcs loc_A390
loc_A381:
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx var_00F7
  sty var_00F6
  jsr fat_find_free
  bcc loc_A39B
loc_A390:
  cmp #$27
  sec
  bne loc_A37B
  sta data_8031
  lda #$05
  rts
loc_A39B:
  jsr sub_8AEF
  bcs loc_A37B
  ldx #$10
  jsr dir_entry_init
  ldy #$1B
  lda free_search_cluster_high
  sta (var_00F1),y
  tax
  dey
  lda free_search_cluster_low
  sta (var_00F1),y
  tay
  lda fat16_flag
  sta fat_value_high
  lda #$FF
  sta fat_value_low
  sta var_00FC
  jsr fat_entry_access
  bcs loc_A37B
  jsr dir_buffer_write
  bcs loc_A37B
  jsr buffers_flush_all
  bcs loc_A37B
  jsr data_buffer_clear
  ldx free_search_cluster_high
  ldy free_search_cluster_low
  stx data_921B
  sty boot_head_count
  jsr cluster_to_lba
  bcs loc_A456
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  ldx #$0A
  lda #$20
loc_A3F1:
  sta sector_buffer,x
  sta boot_total_sectors32_0,x
  dex
  bne loc_A3F1
  lda #$2E
  sta sector_buffer
  sta boot_total_sectors32_0
  sta boot_total_sectors32_1
  ldx #$92
  ldy #$00
  stx var_00F2
  sty var_00F1
  lda #$10
  jsr dir_entry_set_timestamp
  lda #$20
  sta var_00F1
  lda #$10
  jsr dir_entry_set_timestamp
  bit in_subdirectory
  bpl loc_A42C
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  stx data_923B
  sty data_923A
loc_A42C:
  lda #$03
  jsr data_buffer_transfer
  bcs loc_A456
  jsr data_buffer_clear
  ldx sectors_per_cluster
  stx var_00F3
loc_A43B:
  dec var_00F3
  bne loc_A442
  lda #$00
  rts
loc_A442:
  inc data_buffer_lba_0
  bne loc_A44F
  inc data_buffer_lba_1
  bne loc_A44F
  inc data_buffer_lba_2
loc_A44F:
  lda #$03
  jsr data_buffer_transfer
  bcc loc_A43B
loc_A456:
  rts

sub_A457:
  lda current_dir_cluster_high
  pha
  lda current_dir_cluster_low
  pha
  stx current_dir_cluster_high
  sty current_dir_cluster_low
  lda #$00
  sta data_8022
  lda sectors_per_cluster
  sta var_00FB
  lda #$40
  sta var_00FA
  dec in_subdirectory
  jsr cluster_to_lba
  bcs loc_A4B1
loc_A47B:
  jsr dir_buffer_read
  bcs loc_A4B1
  ldy var_00FA
loc_A482:
  lda dir_buffer,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A482
loc_A491:
  lda data_9500,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A491
  sty var_00FA
  jsr dir_next_sector_current
  bcc loc_A47B
  cmp #$02
  beq loc_A4B1
  lda #$00
  beq loc_A4B1
loc_A4AF:
  lda #$05
loc_A4B1:
  sta var_00F0
  ldx current_dir_cluster_high
  ldy current_dir_cluster_low
  pla
  sta current_dir_cluster_low
  pla
  sta current_dir_cluster_high
  inc in_subdirectory
  lda var_00F0
  rts

; 文件调用 18 FileRemoveDir：删除目录，路径不存在 A=$03，删除当前目录 A=$10（后者只依据手册）。Ember 实测删除空目录成功。
file_call_remove_dir:
  lda #$10
  ldy #$08
  jsr dir_find_entry
  bcc loc_A4D7
  cmp #$02
  bne loc_A524
  lda #$03
  rts
loc_A4D7:
  jsr block_store_dir_position
  ldy #$1B
  lda (var_00F1),y
  tax
  dey
  lda (var_00F1),y
  tay
  cpx data_807B
  bne loc_A4F0
  cpy data_807A
  bne loc_A4F0
  lda #$10
  rts
loc_A4F0:
  jsr sub_A457
  cmp #$00
  bne loc_A524
  lda #$FF
  sta var_00FC
loc_A4FB:
  lda #$00
  sta fat_value_low
  sta fat_value_high
  jsr fat_entry_access
  bcs loc_A524
  jsr fat_value_is_last
  bcc loc_A4FB
  jsr fat_buffer_flush
  bcs loc_A524
  jsr sub_8F62
  bcs loc_A524
  ldy #$00
  lda #$E5
  sta (var_00F1),y
  jsr dir_buffer_write
  bcs loc_A524
  lda #$00
loc_A524:
  rts

; 文件调用 19 FileGetDir：取当前目录：出口 FH_PAGE 为 dos_page、FH_BUFFER 为 current_dir_path（DOS 页里以 0 结尾的路径串，根目录为空串）。Ember 实测。
file_call_get_dir:
  ldy #$02
  lda data_A53A
  sta (var_00EE),y
  ldy #$04
  lda #$80
  sta (var_00EE),y
  dey
  lda #$32
  sta (var_00EE),y
  lda #$00
  rts
data_A53A:
  .byte $00
data_A53B:
  .byte $00
data_A53C:
  .byte $00
data_A53D:
  .byte $00
data_A53E:
  .byte $00
data_A53F:
  .byte $00
data_A540:
  .byte $00, $00, $00
data_A543:
  .byte $00
data_A544:
  .byte $00
data_A545:
  .byte $00
data_A546:
  .byte $00
data_A547:
  .byte $00
data_A548:
  .byte $00
data_A549:
  .res 40, $00
data_A571:
  .byte $0D
  .res 60, $00
data_A5AE:
  .byte $00
data_A5AF:
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
