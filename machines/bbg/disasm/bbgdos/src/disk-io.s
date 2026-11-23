; 模块：disk-io
; 磁盘扇区读写：LBA 到柱面、磁头、扇区的换算，经 BIOS 功能 6 读写，介质检测与引导扇区参数解析

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_049E"
; 段 0，$809E-$83A7
; 写磁盘 I/O 块的驱动器号 $E6：当前盘是硬盘或 drive_info_a 为 3 时用 A，否则用 drive_info_a 减 1（未确认各取值的含义）。
disk_set_unit:
  lda mounted_drive
  bmi loc_80AF
  ldy drive_info_a
  cpy #$03
  beq loc_80AF
  ldx drive_info_a
  dex
  txa
loc_80AF:
  sta var_00E6
  rts

; 调用 BIOS_DISK_IO。入口：A 为命令（写入磁盘 I/O 块第 0 字节）。磁盘 I/O 块放在零页 $E5-$ED（X=0、Y=$E5）：$E5 命令、$E6 驱动器、$E7 柱面、$E8 磁头、$E9 扇区、$EA 缓冲页、$EB:$EC 缓冲地址、$ED 扇区数（见 DISK_BLOCK_*）。出口：BIOS 的返回值。
bios_disk_command:
  sta var_00E5
  lda #$06
  ldx #$00
  ldy #$E5
  jmp BIOS_ENTRY

; 带重试的磁盘传送。入口：A 为命令（2 读、3 写），其余参数已在 $E7-$ED。出错时先复位（命令 0）再重试，最多 5 次；仍失败则经 critical_error_report 调用严重错误处理（放弃、忽略、重试、失败），返回 A=$1F、C=1。
disk_transfer_retry:
  sta disk_command_temp
  jsr disk_set_unit
  lda disk_command_temp
  jsr bios_disk_command
  cmp #$01
  bcc loc_810F
loc_80CD:
  ldx #$05
  stx disk_retry_count
loc_80D2:
  lda disk_command_temp
  jsr bios_disk_command
  cmp #$01
  bcc loc_810F
  sta disk_last_error
  cmp #$03
  beq loc_80F0
  lda #$00
  jsr bios_disk_command
  dec disk_retry_count
  bne loc_80D2
  lda disk_last_error
loc_80F0:
  bit data_8030
  bpl loc_810C
  ldy #$1A
  jsr critical_error_report
  cpy #$01
  beq loc_80CD
  lda #$00
  bcc loc_810F
  jsr sub_9D99
  cpy #$03
  bcs loc_810C
  jmp loc_B270
loc_810C:
  lda #$1F
  sec
loc_810F:
  rts

; 除法：$ED:$E7 为 16 位被除数，A 为初始余数，$E8 为除数；返回商在 $ED:$E7，余数在 A。
divide_e7_ed_by_e8:
  ldy #$10
loc_8112:
  rol var_00E7
  rol var_00ED
  rol a
  cmp var_00E8
  bcc loc_811D
  sbc var_00E8
loc_811D:
  dey
  bne loc_8112
  rol var_00E7
  rol var_00ED
  rts

; 把逻辑扇区号（$E7 低字节、X 中、Y 高）换成柱面、磁头、扇区：用 sectors_per_track、head_count 两次相除；结果在 $E7（柱面）、$E8（磁头）、$E9（扇区号，位 6-7 为柱面高位）。
lba_to_chs:
  sty var_00ED
  lda sectors_per_track
  sta var_00E8
  txa
  jsr divide_e7_ed_by_e8
  tax
  inx
  stx var_00E9
  lda head_count
  sta var_00E8
  lda #$00
  jsr divide_e7_ed_by_e8
  sta var_00E8
  lda var_00ED
  lsr a
  ror a
  ror a
  ora var_00E9
  sta var_00E9
  rts

; 传送一个扇区。入口：A 为命令（2 读、3 写），逻辑扇区号在 $E7、X、Y，缓冲区地址在 $EB:$EC（缓冲页为 dos_page）。出口：C=1 出错，A 为出错码。
disk_transfer_sector:
  pha
  jsr lba_to_chs
  lda #$01
  sta var_00ED
  lda dos_page
  sta var_00EA
  pla
  jmp disk_transfer_retry

; 把一次传送限制在当前磁道的剩余扇区内，从 sectors_left 里扣除已安排的扇区数，然后执行传送。
disk_limit_to_track:
  bit current_drive
  bmi loc_8175
  ldx sectors_per_track
  inx
  txa
  sec
  sbc var_00E9
  ldx var_00E8
  bne loc_8170
  clc
  adc sectors_per_track
loc_8170:
  cmp sectors_left
  bcc loc_8178
loc_8175:
  lda sectors_left
loc_8178:
  sta var_00ED
  lda sectors_left
  sec
  sbc var_00ED
  sta sectors_left
  lda var_00FD
  jmp disk_transfer_retry

; 连续传送 A 个扇区：起始逻辑扇区号在 sector_lba_0-2，缓冲区的页与地址取自调用者的文件通道块（偏移 2-4）；遇到磁道边界分段（disk_limit_to_track）。
disk_transfer_sectors:
  sta sectors_left
  ldy #$02
  lda (var_00EE),y
  sta var_00EA
  ldy #$04
  lda (var_00EE),y
  sta var_00EC
  dey
  lda (var_00EE),y
  sta var_00EB
  ldx sector_lba_2
  ldy sector_lba_1
  lda sector_lba_0
  sta var_00E7
  jsr lba_to_chs
  jsr disk_limit_to_track
  bcs loc_81C9
loc_81AF:
  lda sectors_left
  beq loc_81C9
  lda var_00ED
  asl a
  adc var_00EC
  sta var_00EC
  inc var_00E7
  ldx #$00
  stx var_00E8
  inx
  stx var_00E9
  jsr disk_limit_to_track
  bcc loc_81AF
loc_81C9:
  rts

; 把逻辑扇区 A:X:Y（Y 为低字节）取进数据缓冲区 sector_buffer：与 data_buffer_lba_0-2 相同则已在缓冲区里，直接返回；否则先写回旧内容再读入。
data_sector_select:
  cmp data_buffer_lba_2
  bne data_sector_load
  cpx data_buffer_lba_1
  bne data_sector_load
  cpy data_buffer_lba_0
  bne data_sector_load
  clc
  rts

data_sector_load:
  jsr data_buffer_flush
  bcs loc_8208
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  lda #$02

; 传送数据缓冲区 sector_buffer（$9200）与 data_buffer_lba_0-2 指定的扇区。入口：A 为命令（2 读、3 写）。
data_buffer_transfer:
  ldx #$92
  ldy #$00
  stx var_00EC
  sty var_00EB
  ldy data_buffer_lba_0
  sty var_00E7
  ldx data_buffer_lba_2
  ldy data_buffer_lba_1
  jsr disk_transfer_sector
  bcc loc_8208
  ldy #$FF
  sty data_buffer_lba_2
loc_8208:
  rts

; 驱动器状态表偏移：按 current_drive 算出该驱动器在 $9C33 起的状态表里的偏移（硬盘加 $A2，奇数驱动器加 $50，每个驱动器 80 字节）。
drive_state_offset:
  lda current_drive

drive_state_offset_of:
  ldx #$00
  tay
  bpl loc_8213
  ldx #$A2
loc_8213:
  lsr a
  bcc loc_821A
  txa
  adc #$50
  tax
loc_821A:
  rts

; 把当前驱动器保存在状态表里的 80 字节当前目录路径复制到 current_dir_path。
drive_state_load:
  jsr drive_state_offset
  ldy #$00
loc_8220:
  lda data_9C33,x
  sta current_dir_path,y
  inx
  iny
  cpy #$50
  bne loc_8220
  lda data_9C33,x
  rts

; 选定 current_drive 指定的驱动器：与 mounted_drive 不同就换盘——保存原驱动器的当前目录、读引导扇区并解析参数块（drive_read_boot_sector）、装入新驱动器的当前目录。出口：C=1 出错，A 为出错码。
drive_select:
  lda current_drive
  cmp mounted_drive
  bne loc_824E
  clc
  tay
  bmi loc_824D
  jsr disk_set_unit
  lda #$01
  jsr bios_disk_command
  clc
  tya
  bpl loc_824D
  jsr sub_9D77
  bcc loc_8288
loc_824D:
  rts
loc_824E:
  jsr sub_9D77
  bcs loc_824D
  lda current_drive
  sta mounted_drive
  bmi loc_8281
  ldx drive_info_a
  cpx #$03
  beq loc_8276
  cmp data_802F
  beq loc_8276
  ldx #$00
  sta data_802F
  tay
  jsr var_581B
  jsr drive_state_offset
  jmp loc_8288
loc_8276:
  jsr disk_set_unit
  lda #$01
  jsr bios_disk_command
  tya
  bmi loc_8288
loc_8281:
  jsr drive_state_load
  beq loc_828B
  clc
  rts
loc_8288:
  jsr drive_state_load
loc_828B:
  lda #$00
  sta data_807B
  sta data_807A
  ldx current_drive
  lda #$FF
  sta data_8025,x

; 读 0 号扇区（引导扇区），要求开头是 $E9 或 $EB $90 的跳转指令且媒体描述符高 4 位为 $F，否则按“未识别介质”处理（可经严重错误处理重试），出错 A=$1F。
drive_read_boot_sector:
  ldx #$FF
  stx head_count
  stx sectors_per_track
  inx
  txa
  tay
  jsr data_sector_load
  bcs loc_82E0
  lda sector_buffer
  cmp #$E9
  beq loc_82BD
  cmp #$EB
  bne loc_82C6
  lda data_9202
  cmp #$90
  bne loc_82C6
loc_82BD:
  lda boot_media_descriptor
  and #$F0
  cmp #$F0
  beq drive_parse_boot_sector
loc_82C6:
  jsr sub_9D99
  bit data_8030
  bpl loc_82E0
  ldy #$1A
  ldx #$0C
  jsr CRITICAL_ERROR_ENTRY
  cpy #$01
  beq drive_read_boot_sector
  cpy #$03
  beq loc_82E0
  jmp loc_B270
loc_82E0:
  lda #$1F
  sec
  rts

; 解析引导扇区里的 BIOS 参数块（BPB），算出 sectors_per_cluster、sectors_per_track、head_count、fat1_start_sector、fat2_start_sector、root_dir_lba、root_dir_sectors、data_area_lba、cluster_limit，并以总簇数是否不小于 $FF0 判断 FAT12 或 FAT16（fat16_flag 为 0 或 $FF）。
drive_parse_boot_sector:
  lda boot_sectors_per_cluster
  sta sectors_per_cluster
  lda boot_root_entries_low
  clc
  adc #$0F
  bcc loc_82F5
  inc boot_root_entries_high
loc_82F5:
  ldx #$04
loc_82F7:
  lsr boot_root_entries_high
  ror a
  dex
  bne loc_82F7
  sta root_dir_sectors
  lda boot_sectors_per_track
  sta sectors_per_track
  lda boot_head_count
  sta head_count
  lda boot_reserved_sectors
  sta fat1_start_sector
  clc
  adc boot_sectors_per_fat
  sta fat2_start_sector
  dec boot_fat_count
  beq loc_8322
  adc boot_sectors_per_fat
loc_8322:
  sta root_dir_lba_low
  tay
  lda #$00
  sta in_subdirectory
  adc #$00
  sta root_dir_lba_high
  tax
  tya
  adc root_dir_sectors
  sta data_area_lba_low
  txa
  adc #$00
  sta data_area_lba_high
  ldx boot_total_sectors_high
  ldy boot_total_sectors_low
  tya
  ora boot_total_sectors_high
  beq loc_834E
  lda #$00
  beq loc_8357
loc_834E:
  lda boot_total_sectors32_2
  ldx boot_total_sectors32_1
  ldy boot_total_sectors32_0
loc_8357:
  sta var_00F0
  tya
  sec
  sbc data_area_lba_low
  sta cluster_limit_low
  txa
  sbc data_area_lba_high
  sta cluster_limit_high
  bcs loc_836C
  dec var_00F0
loc_836C:
  lda sectors_per_cluster
loc_836F:
  lsr a
  beq loc_837D
  lsr var_00F0
  ror cluster_limit_high
  ror cluster_limit_low
  jmp loc_836F
loc_837D:
  lda cluster_limit_low
  adc #$01
  sta cluster_limit_low
  bcc loc_838A
  inc cluster_limit_high
loc_838A:
  ldx #$0F
  cpx cluster_limit_high
  bcc loc_8397
  bne loc_8399
  cmp #$F0
  bcc loc_8399
loc_8397:
  ldx #$FF
loc_8399:
  stx fat16_flag
  ldx #$00
  ldy #$02
  stx free_search_cluster_high
  sty free_search_cluster_low
  clc
  rts
