; 模块：dos-variables
; DOS 的全局变量区（驱动器状态、磁盘参数、当前盘号等）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0400"
; 段 0，$8000-$809D
drive_info_y:
  .byte $00
drive_info_a:
  .byte $00
drive_info_x:
  .byte $00
data_8003:
  .byte $03
data_8004:
  .byte $00
; 当前操作的逻辑驱动器号（0 起，位 7 为硬盘）
current_drive:
  .byte $00
logical_drive_count:
  .byte $03
; DOS 自己所在的内存页号，磁盘 I/O 的缓冲页
dos_page:
  .byte $FE
; 文件调用入口桩传入的调用者页号
caller_page:
  .byte $FE
sectors_left:
  .byte $00
data_800A:
  .byte $00
disk_retry_count:
  .byte $00
disk_last_error:
  .byte $00
caller_block_low:
  .byte $00
caller_block_high:
  .byte $00
sector_lba_0:
  .byte $00
sector_lba_1:
  .byte $00
sector_lba_2:
  .byte $00
chunk_bytes_low:
  .byte $00
chunk_bytes_high:
  .byte $00
remaining_bytes_low:
  .byte $00
remaining_bytes_high:
  .byte $00
fat_buffer_dirty:
  .byte $00
data_buffer_dirty:
  .byte $00
dir_buffer_lba_0:
  .byte $00
dir_buffer_lba_1:
  .byte $00
dir_buffer_lba_2:
  .byte $00
fat_buffer_sector:
  .byte $00
data_buffer_lba_0:
  .byte $00
data_buffer_lba_1:
  .byte $00
data_buffer_lba_2:
  .byte $00
free_slot_lba_0:
  .byte $00
free_slot_lba_1:
  .byte $00
free_slot_lba_2:
  .byte $00
data_8022:
  .byte $00
data_8023:
  .byte $00
data_8024:
  .byte $00
data_8025:
  .byte $FF, $FF
data_8027:
  .byte $00
data_8028:
  .byte $00
data_8029:
  .byte $00
data_802A:
  .byte $00
current_dir_cluster_low:
  .byte $00
current_dir_cluster_high:
  .byte $00
in_subdirectory:
  .byte $00
mounted_drive:
  .byte $FF
data_802F:
  .byte $00
data_8030:
  .byte $00
data_8031:
  .byte $00
; 当前目录路径串（80 字节，以 0 结尾，根目录为空串），FILE_GET_DIR 返回它的地址
current_dir_path:
  .res 62, $00
; 每簇扇区数（从引导扇区解析得到）
sectors_per_cluster:
  .byte $00
sectors_per_track:
  .byte $00
head_count:
  .byte $00
cluster_limit_low:
  .byte $00
cluster_limit_high:
  .byte $00
fat1_start_sector:
  .byte $00
fat2_start_sector:
  .byte $00
root_dir_lba_low:
  .byte $00
root_dir_lba_high:
  .byte $00
root_dir_sectors:
  .byte $00
data_807A:
  .byte $00
data_807B:
  .byte $00
data_807C:
  .byte $00
data_area_lba_low:
  .byte $00
data_area_lba_high:
  .byte $00
; FAT12 或 FAT16：0 为 FAT12，$FF 为 FAT16
fat16_flag:
  .byte $00
free_search_cluster_low:
  .byte $00
free_search_cluster_high:
  .byte $00
saved_zero_page:
  .res 27, $00
disk_command_temp:
  .byte $00
