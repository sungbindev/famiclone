; 模块：bpb
; 引导扇区的 BPB 参数块

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$0400-$043D
; 引导扇区的 BPB（BIOS 参数块，FAT12 格式）。BIOS 把本扇区读到 $0400，校验偏移 3 起的“SMDOS1.0”后，从偏移 1 的字节加 2（即 $043E）开始执行。1.44MB 软盘：512 字节一扇区，4 扇区一簇，FAT 两份各 3 扇区，根目录 208 项，18 扇区一磁道，2 个磁头。
; 跳转指令 EB 3C 90，越过参数块
boot_jump:
  .byte $EB, $3C, $90
; 系统标识 SMDOS1.0，BIOS 用它判断是否为可启动的步步高系统盘
bpb_oem_name:
  .byte $53, $4D, $44, $4F, $53, $31, $2E, $30
; 每扇区字节数（2 字节），512
bpb_bytes_per_sector:
  .byte $00, $02
; 每簇扇区数，4
bpb_sectors_per_cluster:
  .byte $04
bpb_reserved_sectors:
  .byte $01
bpb_reserved_sectors_hi:
  .byte $00
; FAT 表个数，2
bpb_fat_count:
  .byte $02
bpb_root_entries:
  .byte $D0
bpb_root_entries_hi:
  .byte $00
; 总扇区数（2 字节），2880
bpb_total_sectors:
  .byte $40, $0B
; 介质描述符，$F0 表示 1.44MB 软盘
bpb_media_descriptor:
  .byte $F0
bpb_sectors_per_fat:
  .byte $03
bpb_sectors_per_fat_hi:
  .byte $00
bpb_sectors_per_track:
  .byte $12, $00
bpb_heads:
  .byte $02, $00
; 隐藏扇区数（4 字节）
bpb_hidden_sectors:
  .byte $00, $00, $00, $00
; 32 位总扇区数（4 字节），未使用
bpb_total_sectors_32:
  .byte $00, $00, $00, $00
; 物理驱动器号
bpb_drive_number:
  .byte $00, $01
; 扩展引导标志 $29
bpb_signature:
  .byte $29
; 卷序列号（4 字节）
bpb_volume_serial:
  .byte $00, $00, $00, $00
; 卷标（11 字节，空格填充）
bpb_volume_label:
  .byte $20, $20, $20, $20, $20, $20, $20, $20
  .byte $20, $20, $20
; 文件系统类型 FAT12
bpb_file_system:
  .byte $46, $41, $54, $31, $32, $20, $20, $20
