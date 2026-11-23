; 模块：boot-init
; BBGDOS.SYS 装入后的初始化与 $5803、$580C 入口桩的模板（常驻部分，初始化后即可释放）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$7C00-$7FFF
; BBGDOS.SYS 装入后的入口（引导程序从 $7C00 开始执行）：跳到 dos_init_start 做初始化。A 为 BIOS 引导传入的盘号，X 为第二个引导参数（初始化里存入 far_ptr_high，用途未确认）。
dos_boot_entry:
  jmp dos_init_start
; 启动提示“Starting BBGDOS...”，以 $ 结束
msg_starting:
  .byte $0A, $53, $74, $61, $72, $74, $69, $6E, $67, $20, $42, $42, $47, $44, $4F, $53
  .byte $2E, $2E, $2E, $0D, $0A, $24
; CONFIG.SYS 的 11 字节文件名（主名 8 加扩展名 3）
name_config_sys:
  .byte $43, $4F, $4E, $46, $49, $47, $20, $20, $53, $59, $53
file_stub:
  .byte $A9, $02, $20, $00, $58, $48, $8A, $48, $A9, $01, $AE
file_stub_page_ref_low:
  .byte $3C
file_stub_page_ref_high:
  .byte $7C, $20, $00, $58, $68, $AA, $68, $20, $BE, $90, $98, $60
file_stub_page:
  .byte $00
dos_stub:
  .byte $48, $8A, $48, $A9, $02, $20, $00, $58, $48, $A9, $01, $AE
dos_stub_page_ref_low:
  .byte $69
dos_stub_page_ref_high:
  .byte $7C, $20, $00, $58, $68, $8D, $3D, $A5, $68, $AA, $68, $20, $86, $A6, $08, $48
  .byte $8A, $48, $A9, $01, $AE, $3D, $A5, $20, $00, $58, $68, $AA, $68, $28, $60
dos_stub_page:
  .byte $00
; 低端内存不足的提示，以 $ 结束
msg_no_low_ram:
  .byte $4E, $6F, $20, $65, $6E, $6F, $75, $67, $68, $20, $6C, $6F, $77, $20, $52, $41
  .byte $4D, $20, $74, $6F, $20, $77, $6F, $72, $6B, $2E, $24

; 把引导传入的驱动器号 A 换成 DOS 盘号：非负数（软盘）得 1（A 盘）；$FF 得 logical_drive_count（最后一个驱动器）；其余（位 7 为 1，硬盘）加 $83 取低 8 位（未确认）。
boot_drive_to_disk:
  tax
  bpl boot_drive_floppy
  inx
  beq boot_drive_default
  clc
  adc #$83
  rts
boot_drive_floppy:
  lda #$01
  rts
boot_drive_default:
  lda logical_drive_count
  rts
dos_init_start:
  stx far_ptr_high
  ldx #$FF
  txs
  pha
  jsr drive_info_read
  lda #$FF
  ldx #$FF
  jsr BIOS_ENTRY
  sta bios_version_high
  stx bios_version_low
  pla
  jsr boot_drive_to_disk
  sta boot_disk
  ldx bios_version_high
  cpx #$40
  bne init_store_default_disk
  ldx bios_version_low
  cpx #$62
  bcc init_store_default_disk
  lda far_ptr_high
  jsr boot_drive_to_disk
init_store_default_disk:
  sta default_disk
  ldx #$7C
  ldy #$03
  jsr startup_print_string
  lda #$07
  jsr BIOS_ENTRY
  tax
  stx dos_home_page
  stx dos_stub_page
  stx dos_page
  stx file_stub_page
  stx current_page
  stx init_screen_page
  inx
  stx free_page
  lda #$02
  jsr BIOS_ENTRY
  sta move_src_page
  lda dos_home_page
  sta move_dst_page
  ldx #$80
  ldy #$00
  stx move_src_high
  sty move_src_low
  stx move_dst_high
  sty move_dst_low
  ldx #$40
  ldy #$00
  stx move_length_high
  sty move_length_low
  lda #$0C
  ldx #$80
  jsr BIOS_ENTRY
  lda #$01
  ldx dos_home_page
  jsr BIOS_ENTRY
  lda #$0F
  ldx #$00
  ldy #$19
  jsr BIOS_ENTRY
  cmp #$00
  bne init_no_low_ram
  lda #$4C
  sta FILE_ENTRY
  stx file_entry_target_high
  sty file_entry_target_low
  stx far_ptr_high
  sty far_ptr_low
  stx file_stub_addr_high
  sty file_stub_addr_low
  tya
  clc
  adc #$18
  sta file_stub_page_ref_low
  txa
  adc #$00
  sta file_stub_page_ref_high
  ldy #$18
init_copy_file_stub:
  lda file_stub,y
  sta (far_ptr_low),y
  dey
  bpl init_copy_file_stub
  ldx #$00
  ldy #$2D
  lda #$0F
  jsr BIOS_ENTRY
  cmp #$00
  beq init_install_dos_stub
init_no_low_ram:
  ldx #$7C
  ldy #$6A
  jmp startup_print_halt
init_install_dos_stub:
  lda #$4C
  sta DOS_ENTRY
  stx dos_entry_target_high
  sty dos_entry_target_low
  stx move_src_low
  sty move_src_page
  tya
  clc
  adc #$2C
  sta dos_stub_page_ref_low
  txa
  adc #$00
  sta dos_stub_page_ref_high
  ldy #$2C
init_copy_dos_stub:
  lda dos_stub,y
  sta (move_src_page),y
  dey
  bpl init_copy_dos_stub
  lda #$01
  ldx #$FF
  jsr BIOS_ENTRY
  ldx #$58
  ldy #$06
  stx page_reset_vector_high
  sty page_reset_vector_low
  lda #$01
  ldx dos_stub_page
  jsr BIOS_ENTRY
  lda bios_version_high
  cmp #$40
  bcc init_install_vectors
  lda #$FF
  sta ctrl_check_enabled
init_install_vectors:
  lda #$60
  ldx #$B2
  ldy #$5C
  sta CTRL_C_ENTRY
  stx ctrl_c_target_high
  sty ctrl_c_target_low
  lda #$A0
  sta CRITICAL_ERROR_ENTRY
  lda #$03
  sta critical_target_low
  lda #$60
  sta critical_target_high
  lda #$4C
  ldx #$98
  ldy #$63
  sta disk_prompt_entry
  stx disk_prompt_target_high
  sty disk_prompt_target_low
  lda default_disk
  sta fb_drive
  lda #$01
  ldx #$7C
  ldy #$19
  jsr file_call_with_buffer
  bcs init_open_config_failed
  jmp config_read_line
init_open_config_failed:
  jmp shell_install_vectors
  .res 521, $FF
