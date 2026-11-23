; 布局内的符号：在定义它的模块里是导出，在其他模块里是导入。
.global boot_jump, bpb_oem_name, bpb_bytes_per_sector, bpb_sectors_per_cluster, bpb_reserved_sectors, bpb_reserved_sectors_hi
.global bpb_fat_count, bpb_root_entries, bpb_root_entries_hi, bpb_total_sectors, bpb_media_descriptor, bpb_sectors_per_fat
.global bpb_sectors_per_fat_hi, bpb_sectors_per_track, bpb_heads, bpb_hidden_sectors, bpb_total_sectors_32, bpb_drive_number
.global bpb_signature, bpb_volume_serial, bpb_volume_label, bpb_file_system, boot_main, boot_root_sectors_shift
.global boot_name_compare, boot_read_first_part, boot_read_rest, boot_fail, boot_show_error, boot_show_char_loop
.global boot_halt, boot_lba_to_chs, boot_divide_by_spt, boot_divide_by_spt_next, boot_divide_by_heads, boot_divide_by_heads_next
.global boot_read_sectors, boot_read_retry, boot_read_done, boot_load_file_sectors, boot_load_loop, boot_track_remaining
.global boot_track_clamp, boot_track_read, boot_lba_no_carry, boot_load_done, boot_system_file_name, boot_error_text
.global boot_signature
