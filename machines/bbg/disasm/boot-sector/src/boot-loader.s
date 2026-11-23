; 模块：boot-loader
; 引导程序：读根目录、装入 BBGDOS.SYS 并跳转

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_003E"
; 段 0，$043E-$05A2
; 引导程序入口：把根目录第一项指定的 BBGDOS.SYS 读到 $7C00 并跳转执行。
; 入口：A 为启动盘的驱动器号。
; 出口：成功时不返回，跳到 $7C00 并把驱动器号放在 A；失败时，BIOS 版本不小于 2 就显示提示并死循环，否则返回 BIOS。
; 副作用：使用零页 $80-$90 作磁盘 I/O 块与计算单元。
boot_main:
  sta disk_drive
  clc
  lda bpb_sectors_per_fat  ; 每个 FAT 的扇区数乘 FAT 个数 2，加保留扇区数，得到根目录起始扇区（LBA）
  asl a
  rol bpb_sectors_per_fat_hi
  adc bpb_reserved_sectors
  sta boot_root_lba_low
  lda bpb_sectors_per_fat_hi
  adc bpb_reserved_sectors_hi
  sta boot_root_lba_high
  lda bpb_root_entries  ; 根目录项数除以 16（每扇区 16 项），向上取整的扇区数加到根目录起点，得到数据区起点
  ldx #$04
boot_root_sectors_shift:
  lsr bpb_root_entries_hi
  ror a
  dex
  bne boot_root_sectors_shift
  adc boot_root_lba_low
  sta boot_next_lba_low
  lda bpb_root_entries_hi
  adc boot_root_lba_high
  sta boot_next_lba_high
  ldx #$7C  ; 读缓冲区设为 $7C00，缓冲区页号 $FE 表示当前页
  ldy #$00
  stx disk_buffer_high
  sty disk_buffer_low
  lda #$FE
  sta disk_buffer_page
  ldx #$89
  jsr boot_lba_to_chs  ; 把根目录起始扇区转成磁头、柱面、扇区
  lda #$01  ; 读 1 个扇区：根目录的第一个扇区
  jsr boot_read_sectors
  bne boot_fail
  ldx #$0A  ; 第一项必须是“BBGDOS  SYS”，起始簇必须是 2
boot_name_compare:
  lda system_file_load_address,x
  cmp boot_system_file_name,x
  bne boot_fail
  dex
  bpl boot_name_compare
  lda loaded_entry_cluster_high
  bne boot_fail
  lda loaded_entry_cluster_low
  cmp #$02
  bne boot_fail
  lda loaded_entry_size_low  ; 文件大小折算成 512 字节的扇区数（向上取整）
  adc #$FE
  lda loaded_entry_size_high
  adc #$01
  ror a
  cmp #$23  ; 不到 35 个扇区一次读完；多于 62 个扇区按 62 处理；超过 34 个扇区时先读 34 个，再读剩下的
  bcc boot_read_rest
  cmp #$3E
  bcc boot_read_first_part
  lda #$3E
boot_read_first_part:
  pha
  lda #$22
  jsr boot_load_file_sectors
  pla
  bcc boot_fail
  sbc #$22
boot_read_rest:
  jsr boot_load_file_sectors
  bcc boot_fail
  lda #$01  ; BIOS 功能 1（SetPage）：把 $8000 窗口设为 $FE 页（当前页）
  ldx #$FE
  jsr BIOS_ENTRY
  lda disk_drive  ; A 为驱动器号，跳转到刚读入的 BBGDOS.SYS
  jmp system_file_load_address
boot_fail:
  lda #$FF  ; BIOS 功能 $FF 取版本；BIOS 版本小于 2 时返回 BIOS，由 BIOS 提示
  jsr BIOS_ENTRY
  cmp #$02
  bcs boot_show_error
  rts
boot_show_error:
  ldy #$00
boot_show_char_loop:
  tya
  pha
  ldx boot_error_text,y
  beq boot_halt
  lda #$1B  ; BIOS 功能 27（DisplayTTY）逐字符显示错误提示
  ldy #$00
  jsr BIOS_ENTRY
  pla
  tay
  iny
  bne boot_show_char_loop
boot_halt:
  beq boot_halt

; 把零页里的 16 位扇区号（LBA）转成磁头、柱面、扇区。
; 入口：X 为零页中扇区号的低字节地址，扇区号的高字节在其后一个字节。
; 出口：disk_cylinder 为柱面低 8 位；disk_head 为磁头；disk_sector 为扇区号（从 1 起）与柱面高 2 位的组合（位 6、7 是柱面高位）。
; 依据 BPB 的每磁道扇区数与磁头数做两次长除。
boot_lba_to_chs:
  lda #$00
  lda zero_page_base,x
  sta disk_cylinder
  lda zero_page_base_next,x
  sta boot_quotient_high
  ldy #$11
  bne boot_divide_by_spt_next
boot_divide_by_spt:
  rol a
  cmp bpb_sectors_per_track
  bcc boot_divide_by_spt_next
  sbc bpb_sectors_per_track
boot_divide_by_spt_next:
  rol disk_cylinder
  rol boot_quotient_high
  dey
  bne boot_divide_by_spt
  tay
  iny
  sty disk_sector
  lda #$00
  ldy #$11
  bne boot_divide_by_heads_next
boot_divide_by_heads:
  rol a
  cmp bpb_heads
  bcc boot_divide_by_heads_next
  sbc bpb_heads
boot_divide_by_heads_next:
  rol disk_cylinder
  rol boot_quotient_high
  dey
  bne boot_divide_by_heads
  sta disk_head
  lda boot_quotient_high
  lsr a
  ror a
  ror a
  ora disk_sector
  sta disk_sector
  rts

; 按 disk_cylinder、disk_head、disk_sector 读若干扇区。
; 入口：A 为扇区数，缓冲区在 disk_buffer_page 与 disk_buffer_low、disk_buffer_high。
; 出口：Z 为 1 表示成功（BIOS 返回码为 0）。
; 失败时复位磁盘（命令 0）后重试，最多重试 2 次。磁盘 I/O 块在 $0080，由 BIOS 功能 6 处理，命令 2 为读。
boot_read_sectors:
  sta disk_sector_count
  lda #$02
  sta boot_retry_count
boot_read_retry:
  lda #$02
  sta disk_command
  lda #$06
  ldx #$00
  ldy #$80
  jsr BIOS_ENTRY
  tay
  beq boot_read_done
  dec boot_retry_count
  bmi boot_read_done
  lda #$00
  sta disk_command
  lda #$06
  ldx #$00
  ldy #$80
  jsr BIOS_ENTRY
  clc
  bcc boot_read_retry
boot_read_done:
  rts

; 从 boot_next_lba 起读入若干扇区，不跨磁道，一次只读到本磁道末尾，然后循环到读完。
; 入口：A 为要读的扇区总数。
; 出口：C 为 1 表示成功（BIOS 读盘的 Z 标志经 CLC/BNE 转换），C 为 0 表示失败。
; 副作用：读缓冲区高字节随读入的扇区数增加。
boot_load_file_sectors:
  sta boot_sectors_left
boot_load_loop:
  ldx #$8C
  jsr boot_lba_to_chs
  ldx bpb_sectors_per_track
  inx
  txa
  sec
  sbc disk_sector
  ldx disk_head
boot_track_remaining:
  inx
  cpx bpb_heads
  bcs boot_track_clamp
  adc bpb_sectors_per_track
  cmp boot_sectors_left
  bcc boot_track_remaining
boot_track_clamp:
  cmp boot_sectors_left
  bcc boot_track_read
  lda boot_sectors_left
boot_track_read:
  jsr boot_read_sectors
  clc
  bne boot_load_done
  lda disk_sector_count
  asl a
  adc disk_buffer_high
  sta disk_buffer_high
  lda boot_next_lba_low
  adc disk_sector_count
  sta boot_next_lba_low
  bcc boot_lba_no_carry
  inc boot_next_lba_high
boot_lba_no_carry:
  lda boot_sectors_left
  sec
  sbc disk_sector_count
  sta boot_sectors_left
  bne boot_load_loop
boot_load_done:
  rts
