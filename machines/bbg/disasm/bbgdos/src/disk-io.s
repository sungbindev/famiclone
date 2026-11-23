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
  bmi disk_set_unit_store
  ldy drive_info_a
  cpy #$03
  beq disk_set_unit_store
  ldx drive_info_a
  dex
  txa
disk_set_unit_store:
  sta dio_unit
  rts

; 调用 BIOS_DISK_IO。入口：A 为命令（写入磁盘 I/O 块第 0 字节）。磁盘 I/O 块放在零页 $E5-$ED（X=0、Y=$E5）：$E5 命令、$E6 驱动器、$E7 柱面、$E8 磁头、$E9 扇区、$EA 缓冲页、$EB:$EC 缓冲地址、$ED 扇区数（见 DISK_BLOCK_*）。出口：BIOS 的返回值。
bios_disk_command:
  sta dio_command
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
  bcc disk_transfer_done
disk_retry_start:
  ldx #$05
  stx disk_retry_count
disk_retry_loop:
  lda disk_command_temp
  jsr bios_disk_command
  cmp #$01
  bcc disk_transfer_done
  sta disk_last_error
  cmp #$03
  beq disk_retry_failed
  lda #$00
  jsr bios_disk_command
  dec disk_retry_count
  bne disk_retry_loop
  lda disk_last_error
disk_retry_failed:
  bit critical_error_flags
  bpl disk_retry_give_up
  ldy #$1A
  jsr critical_error_report
  cpy #$01
  beq disk_retry_start
  lda #$00
  bcc disk_transfer_done
  jsr buffers_invalidate
  cpy #$03
  bcs disk_retry_give_up
  jmp abort_program
disk_retry_give_up:
  lda #$1F
  sec
disk_transfer_done:
  rts

; 除法：$ED:$E7 为 16 位被除数，A 为初始余数，$E8 为除数；返回商在 $ED:$E7，余数在 A。
divide_e7_ed_by_e8:
  ldy #$10
divide_bit_loop:
  rol dio_cylinder
  rol dio_count
  rol a
  cmp dio_head
  bcc divide_no_subtract
  sbc dio_head
divide_no_subtract:
  dey
  bne divide_bit_loop
  rol dio_cylinder
  rol dio_count
  rts

; 把逻辑扇区号（$E7 低字节、X 中、Y 高）换成柱面、磁头、扇区：用 sectors_per_track、head_count 两次相除；结果在 $E7（柱面）、$E8（磁头）、$E9（扇区号，位 6-7 为柱面高位）。
lba_to_chs:
  sty dio_count
  lda sectors_per_track
  sta dio_head
  txa
  jsr divide_e7_ed_by_e8
  tax
  inx
  stx dio_sector
  lda head_count
  sta dio_head
  lda #$00
  jsr divide_e7_ed_by_e8
  sta dio_head
  lda dio_count
  lsr a
  ror a
  ror a
  ora dio_sector
  sta dio_sector
  rts

; 传送一个扇区。入口：A 为命令（2 读、3 写），逻辑扇区号在 $E7、X、Y，缓冲区地址在 $EB:$EC（缓冲页为 dos_page）。出口：C=1 出错，A 为出错码。
disk_transfer_sector:
  pha
  jsr lba_to_chs
  lda #$01
  sta dio_count
  lda dos_page
  sta dio_page
  pla
  jmp disk_transfer_retry

; 把一次传送限制在当前磁道的剩余扇区内，从 sectors_left 里扣除已安排的扇区数，然后执行传送。
disk_limit_to_track:
  bit current_drive
  bmi track_limit_use_all
  ldx sectors_per_track
  inx
  txa
  sec
  sbc dio_sector
  ldx dio_head
  bne track_limit_compare
  clc
  adc sectors_per_track
track_limit_compare:
  cmp sectors_left
  bcc track_limit_apply
track_limit_use_all:
  lda sectors_left
track_limit_apply:
  sta dio_count
  lda sectors_left
  sec
  sbc dio_count
  sta sectors_left
  lda transfer_command
  jmp disk_transfer_retry

; 连续传送 A 个扇区：起始逻辑扇区号在 sector_lba_0-2，缓冲区的页与地址取自调用者的文件通道块（偏移 2-4）；遇到磁道边界分段（disk_limit_to_track）。
disk_transfer_sectors:
  sta sectors_left
  ldy #$02
  lda (block_ptr_low),y
  sta dio_page
  ldy #$04
  lda (block_ptr_low),y
  sta dio_addr_high
  dey
  lda (block_ptr_low),y
  sta dio_addr_low
  ldx sector_lba_2
  ldy sector_lba_1
  lda sector_lba_0
  sta dio_cylinder
  jsr lba_to_chs
  jsr disk_limit_to_track
  bcs disk_sectors_done
disk_sectors_loop:
  lda sectors_left
  beq disk_sectors_done
  lda dio_count
  asl a
  adc dio_addr_high
  sta dio_addr_high
  inc dio_cylinder
  ldx #$00
  stx dio_head
  inx
  stx dio_sector
  jsr disk_limit_to_track
  bcc disk_sectors_loop
disk_sectors_done:
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

; 把逻辑扇区 A:X:Y（Y 为低字节）读进数据缓冲区 sector_buffer，不检查是否已缓冲：先写回旧内容（data_buffer_flush），再读入并记入 data_buffer_lba_0-2。出口：C=1 出错。
data_sector_load:
  jsr data_buffer_flush
  bcs data_sector_return
  sta data_buffer_lba_2
  stx data_buffer_lba_1
  sty data_buffer_lba_0
  lda #$02

; 传送数据缓冲区 sector_buffer（$9200）与 data_buffer_lba_0-2 指定的扇区。入口：A 为命令（2 读、3 写）。
data_buffer_transfer:
  ldx #$92
  ldy #$00
  stx dio_addr_high
  sty dio_addr_low
  ldy data_buffer_lba_0
  sty dio_cylinder
  ldx data_buffer_lba_2
  ldy data_buffer_lba_1
  jsr disk_transfer_sector
  bcc data_sector_return
  ldy #$FF
  sty data_buffer_lba_2
data_sector_return:
  rts

; 驱动器状态表偏移：按 current_drive 算出该驱动器在 $9C33 起的状态表里的偏移（硬盘加 $A2，奇数驱动器加 $50，每个驱动器 80 字节）。
drive_state_offset:
  lda current_drive

; 驱动器状态块偏移（驱动器号取自 A，位 7 为硬盘）：软盘从 0 起，硬盘从 $A2 起，奇数号再加 $51；每个驱动器的状态块是 80 字节当前目录路径加 1 字节标志，共 81 字节。
drive_state_offset_of:
  ldx #$00
  tay
  bpl drive_offset_floppy
  ldx #$A2
drive_offset_floppy:
  lsr a
  bcc drive_offset_done
  txa
  adc #$50
  tax
drive_offset_done:
  rts

; 把当前驱动器保存在状态表里的 80 字节当前目录路径复制到 current_dir_path。
drive_state_load:
  jsr drive_state_offset
  ldy #$00
drive_state_copy_loop:
  lda drive_state_table,x
  sta current_dir_path,y
  inx
  iny
  cpy #$50
  bne drive_state_copy_loop
  lda drive_state_table,x
  rts

; 选定 current_drive 指定的驱动器：与 mounted_drive 不同就换盘——保存原驱动器的当前目录、读引导扇区并解析参数块（drive_read_boot_sector）、装入新驱动器的当前目录。出口：C=1 出错，A 为出错码。
drive_select:
  lda current_drive
  cmp mounted_drive
  bne drive_select_change
  clc
  tay
  bmi drive_select_done
  jsr disk_set_unit
  lda #$01
  jsr bios_disk_command
  clc
  tya
  bpl drive_select_done
  jsr drives_flush_and_save
  bcc drive_select_reload
drive_select_done:
  rts
drive_select_change:
  jsr drives_flush_and_save
  bcs drive_select_done
  lda current_drive
  sta mounted_drive
  bmi drive_select_saved_state
  ldx drive_info_a
  cpx #$03
  beq drive_select_probe
  cmp last_probed_drive
  beq drive_select_probe
  ldx #$00
  sta last_probed_drive
  tay
  jsr disk_prompt_entry
  jsr drive_state_offset
  jmp drive_select_reload
drive_select_probe:
  jsr disk_set_unit
  lda #$01
  jsr bios_disk_command
  tya
  bmi drive_select_reload
drive_select_saved_state:
  jsr drive_state_load
  beq drive_select_read_boot
  clc
  rts
drive_select_reload:
  jsr drive_state_load
drive_select_read_boot:
  lda #$00
  sta committed_dir_cluster_high
  sta committed_dir_cluster_low
  ldx current_drive
  lda #$FF
  sta drive_path_stale,x

; 读 0 号扇区（引导扇区），要求开头是 $E9 或 $EB $90 的跳转指令且媒体描述符高 4 位为 $F，否则按“未识别介质”处理（可经严重错误处理重试），出错 A=$1F。
drive_read_boot_sector:
  ldx #$FF
  stx head_count
  stx sectors_per_track
  inx
  txa
  tay
  jsr data_sector_load
  bcs boot_read_failed
  lda sector_buffer
  cmp #$E9
  beq boot_check_media
  cmp #$EB
  bne boot_media_unrecognized
  lda boot_byte_2
  cmp #$90
  bne boot_media_unrecognized
boot_check_media:
  lda boot_media_descriptor
  and #$F0
  cmp #$F0
  beq drive_parse_boot_sector
boot_media_unrecognized:
  jsr buffers_invalidate
  bit critical_error_flags
  bpl boot_read_failed
  ldy #$1A
  ldx #$0C
  jsr CRITICAL_ERROR_ENTRY
  cpy #$01
  beq drive_read_boot_sector
  cpy #$03
  beq boot_read_failed
  jmp abort_program
boot_read_failed:
  lda #$1F
  sec
  rts

; 解析引导扇区里的 BIOS 参数块（BPB），算出 sectors_per_cluster、sectors_per_track、head_count、fat1_start_sector、fat2_start_sector、root_dir_lba、root_dir_sectors、data_area_lba、cluster_limit，并以总簇数是否不小于 $FF0 判断 FAT12 或 FAT16（fat16_flag 为 $0F 或 $FF）；同时把 free_search_cluster 置为 2。
drive_parse_boot_sector:
  lda boot_sectors_per_cluster
  sta sectors_per_cluster
  lda boot_root_entries_low
  clc
  adc #$0F
  bcc parse_root_sectors
  inc boot_root_entries_high
parse_root_sectors:
  ldx #$04
parse_root_shift_loop:
  lsr boot_root_entries_high
  ror a
  dex
  bne parse_root_shift_loop
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
  beq parse_root_lba
  adc boot_sectors_per_fat
parse_root_lba:
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
  beq parse_total_sectors32
  lda #$00
  beq parse_cluster_count
parse_total_sectors32:
  lda boot_total_sectors32_2
  ldx boot_total_sectors32_1
  ldy boot_total_sectors32_0
parse_cluster_count:
  sta scratch_f0
  tya
  sec
  sbc data_area_lba_low
  sta cluster_limit_low
  txa
  sbc data_area_lba_high
  sta cluster_limit_high
  bcs parse_cluster_shift
  dec scratch_f0
parse_cluster_shift:
  lda sectors_per_cluster
parse_cluster_shift_loop:
  lsr a
  beq parse_cluster_limit
  lsr scratch_f0
  ror cluster_limit_high
  ror cluster_limit_low
  jmp parse_cluster_shift_loop
parse_cluster_limit:
  lda cluster_limit_low
  adc #$01
  sta cluster_limit_low
  bcc parse_fat_type
  inc cluster_limit_high
parse_fat_type:
  ldx #$0F
  cpx cluster_limit_high
  bcc parse_fat16
  bne parse_store_fat_type
  cmp #$F0
  bcc parse_store_fat_type
parse_fat16:
  ldx #$FF
parse_store_fat_type:
  stx fat16_flag
  ldx #$00
  ldy #$02
  stx free_search_cluster_high
  sty free_search_cluster_low
  clc
  rts
