; 模块：bios-dispatch
; BIOS 功能分发表与分发例程

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_0E7C"
; 段 1，$D0A5-$D250
; BIOS 功能分发表：43 项（下标 0-42），每项 2 字节，内容是处理例程地址减 1，下标为功能号乘 2。
bios_function_table:
data_D0A6 = * + 1
  .word bios_no_operation - 1, bios_set_page - 1
  .word bios_get_page - 1, bios_set_av10 - 1
  .word bios_run_initial - 1, bios_low_ram_free - 1
  .word bios_disk_io - 1, bios_get_dram_size - 1
  .word bios_function_8 - 1, bios_function_9 - 1
  .word bios_function_10 - 1, bios_read_key - 1
  .word bios_move_block - 1, bios_function_13 - 1
  .word bios_wait_vsync_count - 1, bios_low_ram_alloc - 1
  .word bios_function_16 - 1, bios_exchange_block - 1
  .word bios_split_screen - 1, bios_function_19 - 1
  .word bios_get_drive_message - 1, bios_function_21 - 1
  .word bios_set_nmi_vector - 1, bios_get_nmi_vector - 1
  .word bios_device - 1, bios_function_25 - 1
  .word bios_function_26 - 1, bios_display_tty - 1
  .word bios_set_cursor - 1, bios_display_chars - 1
  .word bios_get_cursor - 1, bios_function_31 - 1
  .word bios_get_paged_char - 1, bios_set_paged_char - 1
  .word bios_wait_display - 1, bios_function_35 - 1
  .word bios_function_36 - 1, bios_unzip_block - 1
  .word bios_function_38 - 1, bios_set_end_vector - 1
  .word bios_get_date_time - 1, bios_set_system_date - 1
  .word bios_set_system_time - 1

; BIOS 功能分发（入口 $5800 的主路径）：A 为功能号。功能 $3C 直接转显示初始化；$3F-$41 先减 $17，因此 63、64、65 与 40、41、42 相同；其余按 bios_function_table 里的地址（减 1）用 RTS 跳转；功能号大于 42 的被忽略；$FF 返回版本 A=$40、X=$61、Y=1。
bios_dispatch:
  cmp #$3C
  beq loc_D12B
  cmp #$42
  bcs loc_D109
  cmp #$3F
  bcc loc_D109
  sbc #$17
loc_D109:
  asl a
  bcs loc_D120
  cmp #$56
  bcs loc_D12A
  stx var_5FB2
  tax
  lda data_D0A6,x
  pha
  lda bios_function_table,x
  pha
  ldx var_5FB2
  rts
loc_D120:
  cmp #$FE
  bne loc_D12A
  lda #$40
  ldx #$61
  ldy #$01
loc_D12A:
  rts
loc_D12B:
  jmp bios_display_initial

bios_set_end_vector:
  stx data_597F
  sty data_597E
  ldx var_5FFA
  stx var_5FBF
  ldx #$59
  ldy #$62
  stx data_58B0
  sty data_58AF
  rts

bios_function_35:
  txa

sub_D146:
  bit var_4348
  bpl sub_D146
  pha
  asl a
  asl a
  asl a
  asl a
  sta var_4340
  lda #$00
  sta var_4348
loc_D158:
  bit var_4348
  bmi loc_D158
  lda #$80
  sta var_4348
loc_D162:
  bit var_4348
  bpl loc_D162
  pla
  and #$F0
  sta var_4340
  lda #$00
  sta var_4348
loc_D172:
  bit var_4348
  bmi loc_D172
  lda #$80
  sta var_4348
  rts

bios_function_36:
  bit var_4348
  bpl bios_function_36
  lda var_4340
  pha
  lda #$00
  sta var_4348
loc_D18B:
  bit var_4348
  bmi loc_D18B
  lda #$80
  sta var_4348
  pla
  rts

sub_D197:
  jsr bios_function_36
  tay
  jsr bios_function_36
  tax
  beq loc_D1A2
  iny
loc_D1A2:
  lda var_00FD
  pha
  sty var_00FD
  jsr bios_function_36
  sta var_00FF
  jsr bios_function_36
  tay
  lda #$00
  sta var_00FE
  lda var_5FFA
  sta loc_FF04
loc_D1BA:
  lda (var_00FE),y
  jsr sub_D146
  iny
  bne loc_D1C4
  inc var_00FF
loc_D1C4:
  dex
  bne loc_D1BA
  dec var_00FD
  bne loc_D1BA
  pla
  sta var_00FD
  rts

sub_D1CF:
  jsr bios_function_36
  tay
  jsr bios_function_36
  tax
  beq loc_D1DA
  iny
loc_D1DA:
  lda var_00FD
  pha
  sty var_00FD
  jsr bios_function_36
  sta var_00FF
  jsr bios_function_36
  tay
  lda #$00
  sta var_00FE
  lda var_5FFA
  sta loc_FF04
loc_D1F2:
  bit var_4348
  bpl loc_D1F2
  lda #$00
  sta var_4348
loc_D1FC:
  bit var_4348
  bmi loc_D1FC
  lda var_4340
  sta (var_00FE),y
  lda #$80
  sta var_4348
  iny
  bne loc_D210
  inc var_00FF
loc_D210:
  dex
  bne loc_D1F2
  dec var_00FD
  bne loc_D1F2
  pla
  sta var_00FD
  rts

sub_D21B:
  jsr bios_function_36
  cmp #$08
  bne loc_D22C
  jsr bios_function_36
  tax
  jsr bios_set_page
  jmp sub_D21B
loc_D22C:
  cmp #$0B
  bne loc_D236
  jsr sub_D1CF
  jmp sub_D21B
loc_D236:
  cmp #$15
  bne loc_D240
  jsr sub_D197
  jmp sub_D21B
loc_D240:
  cmp #$0A
  bne loc_D24D
  jsr bios_get_page
  jsr sub_D146
  jmp sub_D21B
loc_D24D:
  tax
  bne sub_D21B
  rts
