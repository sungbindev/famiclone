; 模块：ram-vectors
; 拷贝到 $5800 运行的系统调用入口桩、NMI 与 IRQ 处理、页切换和外设调用入口（存放在 ROM 的 $C000-$C228）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$5800-$5A28
bios_entry_jump:
  jmp loc_5867

file_entry_jump:
  jmp loc_588E

cold_start_entry_jump:
  jmp loc_5892

device_entry_jump:
  jmp loc_59D2

dos_entry_jump:
  jmp loc_588E

warm_start_entry_jump:
  jmp loc_5823

ctrl_break_entry_jump:
  rts
  .byte $00, $00

ctrl_c_entry_jump:
  brk
  .byte $00, $00

critical_error_entry_jump:
  brk
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
loc_5823:
  jsr sub_5829
  jmp (data_FFFC)

sub_5829:
  pha
  php
  lda #$7F
  and var_6000
  sta var_2008
  lda #$F7
  sei
  and var_5FFF
  sta var_5FFF
  sta sub_FF01
  plp
  lda var_6000
  sta var_2000
  pla
  rts

sub_5848:
  pha
  php
  lda #$7F
  and var_6000
  sta var_2008
  lda #$08
  sei
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
  plp
  lda var_6000
  sta var_2000
  pla
  rts
loc_5867:
  php
  cmp #$01
  beq loc_587F
  cmp #$02
  beq loc_5884
  cmp #$18
  beq loc_5889
  jsr sub_5829
  jsr bios_dispatch
  jsr sub_5848
  plp
  rts
loc_587F:
  jsr bios_set_page
  plp
  rts
loc_5884:
  jsr bios_get_page
  plp
  rts
loc_5889:
  jsr device_entry_jump
  plp
  rts
loc_588E:
  lda #$42
  sec

sub_5891:
  rts
loc_5892:
  jsr sub_5829
  jmp (data_FFFC)

sub_5898:
  pha
  lda var_5FFF
  and #$F7
  sta sub_FF01
  jsr sub_DDE1
  lda var_5FFF
  sta sub_FF01
  and #$04
  bne loc_58B1
loc_58AE:
data_58AF = * + 1
data_58B0 = * + 2
  jsr sub_5891
loc_58B1:
  pla
loc_58B2:
  rti
data_58B3:
data_58B4 = * + 1
  .word sub_5891

sub_58B5:
  jmp (data_58B3)

sub_58B8:
  pla
  sta var_5FFA
  sta loc_FF04
  lda var_5FFF
  sta sub_FF01
  pla
  rti

; NMI 处理（运行在 $5800 起的 RAM 里）：递减两个计数，检查 PPU 状态与键盘、显示事件，经 sub_58B5 的间接跳转（$58B3 里的向量）调用后续处理。
; 未确认：各计数与事件的含义。
nmi_handler:
  pha
  dec var_5FF8
  dec var_5FA0
  bne loc_58D8
  lda #$0C
  sta var_5FA3
  sta data_FF90
loc_58D8:
  bit var_2002
  bit var_6000
  bpl loc_58B1
  lda var_5FC2
  cmp var_5FC3
  beq loc_58F9
  lda var_5FFF
  and #$F7
  sta sub_FF01
  jsr sub_CF67
  lda var_5FFF
  sta sub_FF01
loc_58F9:
  lda var_5FBB
  beq loc_5914
  lda #$00
  sta var_5FBB
  lda var_5FFF
  and #$F7
  sta sub_FF01
  jsr sub_DDC7
  lda var_5FFF
  sta sub_FF01
loc_5914:
  lda var_6001
  sta var_2001
  jsr sub_58B5
  bit var_5FBD
  bvc loc_5940
  lda var_5FFA
  pha
  lda #$58
  pha
  lda #$B8
  pha
  php
  lda var_5FBE
  sta var_5FFA
  sta loc_FF04
  lda var_5FFF
  ora #$08
  sta sub_FF01
  bne loc_5941
loc_5940:
  pla
loc_5941:
  jmp loc_58B2

sub_5944:
  pha
  lda var_5FFF
  pha
  and #$F7
  sta var_5FFF
  sta sub_FF01
  jsr sub_EAE9
  pla
  and #$08
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
  pla
  rti

sub_5962:
  tya
  pha
  lda var_5FFA
  pha
  lda var_5FBF
  sta var_5FFA
  sta loc_FF04
  lda var_5FFF
  pha
  ora #$08
  sta var_5FFF
  sta sub_FF01
loc_597D:
data_597E = * + 1
data_597F = * + 2
  jsr sub_5891
  pla
  ora #$F7
  and var_5FFF
  sta var_5FFF
  sta sub_FF01
  pla
  sta var_5FFA
  sta loc_FF04
  pla
  tay
  rts

; IRQ 处理：保存寄存器后经 $58B2 的 RTI 返回，当前只做占位。
irq_handler:
  pha
  pla
  pha
  pla
  pha
  pla
  nop
  nop
loc_599E:
  jmp loc_58B2

sub_59A1:
  jsr sub_5848
loc_59A4:
  jsr sub_5891
  jmp sub_5829

; BIOS 功能 1 BIOS_SET_PAGE：X 为页号，0 取 $FE，$FE 取系统默认页 [$5FF9]；页号写入 [$5FFA] 与页寄存器 $FF04。
bios_set_page:
  cpx #$00
  bne loc_59B2
  ldx #$FE
  bne loc_59B9
loc_59B2:
  cpx #$FE
  bne loc_59B9
  ldx var_5FF9
loc_59B9:
  stx var_5FFA
  stx loc_FF04
  rts

; BIOS 功能 2 BIOS_GET_PAGE：A 返回当前页号；当前页为 $FE 时返回 0，等于系统默认页 [$5FF9] 时返回 $FE。
bios_get_page:
  lda var_5FFA
  cmp #$FE
  bne loc_59CA
  lda #$00
  rts
loc_59CA:
  cmp var_5FF9
  bne loc_59D1
  lda #$FE
loc_59D1:
  rts
loc_59D2:
  cpx #$01
  bne loc_59EF

sub_59D6:
  ldy var_5C78
  cpy var_5C79
  beq loc_59E7
  lda var_5C00,y
  ldx var_5C01,y
  ldy #$01
  rts
loc_59E7:
  bit var_5C40
  bvs loc_59EF
  ldy #$00
  rts
loc_59EF:
  jsr sub_5829
  jsr bios_device
  jmp sub_5848

sub_59F8:
  bit var_5C40
  bmi loc_5A18
  lda var_5FFF
  pha
  and #$F7
  sta var_5FFF
  sta sub_FF01
  jsr sub_F64F
  pla
  and #$08
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
loc_5A18:
  rts

sub_5A19:
  lda #$08
loc_5A1B:
  bit var_5FEF
  bne loc_5A1B
  jmp sub_5829

sub_5A23:
  jsr ctrl_break_entry_jump
  jmp sub_5829
