; 模块：bios-services
; 显示初始化、日期时间、解压、键盘扫描支持（未确认：边界）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_27D0"
; 段 1，$E9F9-$F3B7
sub_E9F9:
  ldy #$00

sub_E9FB:
  bit var_2002
  stx var_2006
  sty var_2006
  rts

sub_EA05:
  lda #$00

sub_EA07:
  sta var_2007
  dey
  bne sub_EA07
  rts

bios_display_initial:
  lda #$00
  sta var_6000
  sta var_2000
  sta var_2001
  jsr sub_D350
  lda var_5FFF
  and #$BB
  ora #$01
  sta var_5FFF
  sta sub_FF01
  jsr sub_D33E
  ldx #$00
  jsr sub_E9F9
  ldx #$E1
  ldy #$F5
  stx var_00FF
  sty var_00FE
  ldx #$00
  ldy #$00
loc_EA3D:
  lda (var_00FE),y
  sta var_2007
  iny
  cpy #$08
  bne loc_EA3D
  jsr sub_EA05
  lda var_00FE
  clc
  adc #$08
  sta var_00FE
  bcc loc_EA55
  inc var_00FF
loc_EA55:
  dex
  bne loc_EA3D
  ldx #$20
  jsr sub_E9F9
  ldx #$04
  ldy #$C0
  lda #$20
loc_EA63:
  jsr sub_EA07
  dex
  bne loc_EA63
  ldy #$40
  jsr sub_EA05
  ldx #$28
  jsr sub_E9F9
  ldx #$04
  ldy #$C0
  lda #$20
loc_EA79:
  jsr sub_EA07
  dex
  bne loc_EA79
  ldy #$40
  jsr sub_EA05
  ldx #$01
  jsr bios_wait_vsync_count
  ldx #$3F
  jsr sub_E9F9
  ldy #$00
loc_EA90:
  lda data_E9F5,y
  sta var_2007
  iny
  cpy #$04
  bne loc_EA90
  ldx #$20
  jsr sub_E9F9
  lda #$00
  sta var_2005
  sta var_2005
  sta var_5FF7
  sta var_5FF6
  lda #$0C
  sta var_6001
  sta var_2001
  bit var_2002
  lda #$80
  sta var_6000
  sta var_2000
  rts

; BIOS 功能 40 与 63 BIOS_GET_DATE_TIME：A=[$5FE7]，X=[$5FE9]，Y=[$5FE8]，零页 $FF=[$5FE6]，$FE=[$5FEA]。BIOS 1.0 只读写这些变量。
bios_get_date_time:
  lda var_5FE6
  sta var_00FF
  lda var_5FEA
  sta var_00FE
  lda var_5FE7
  ldx var_5FE9
  ldy var_5FE8
  rts

; BIOS 功能 41 与 64 BIOS_SET_SYSTEM_DATE：X 存入 [$5FE7]，Y 存入 [$5FE6]。
bios_set_system_date:
  stx var_5FE7
  sty var_5FE6
  rts

; BIOS 功能 42 与 65 BIOS_SET_SYSTEM_TIME：X 存入 [$5FE9]，Y 存入 [$5FE8]，零页 $FF 存入 [$5FEA]。
bios_set_system_time:
  stx var_5FE9
  sty var_5FE8
  lda var_00FF
  sta var_5FEA
  rts

sub_EAE9:
  txa
  pha
  ldx var_5FEA
  inx
  cpx #$64
  bcc loc_EB54
  ldx var_5FE8
  inx
  txa
  and #$1F
  cmp #$1E
  bne sub_EB4F
  txa
  and #$E0
  adc #$1F
  tax
  bcc loc_EB0C
  inc var_5FE9
  jmp sub_EB4F
loc_EB0C:
  bpl sub_EB4F
  lda var_5FE9
  and #$07
  cmp #$07
  bne sub_EB4F
  ldx var_5FE9
  inx
  cpx #$C0
  bne loc_EB4A
  ldx var_5FE6
  inx
  txa
  and #$1F
  cmp #$1F
  bne loc_EB45
  txa
  and #$E0
  adc #$20
  tax
  bcc loc_EB38
  inc var_5FE7
  jmp loc_EB45
loc_EB38:
  bpl loc_EB45
  lda var_5FE7
  lsr a
  bcc loc_EB45
  inc var_5FE7
  ldx #$21
loc_EB45:
  stx var_5FE6
  ldx #$00
loc_EB4A:
  stx var_5FE9
  ldx #$00

sub_EB4F:
  stx var_5FE8
  ldx #$00
loc_EB54:
  stx var_5FEA
  pla
  tax
  rts

bios_unzip_block:
  sty var_5FB2
  ldy #$2F
loc_EB5F:
  lda a:var_00D0,y
  sta var_6100,y
  dey
  bpl loc_EB5F
  lda var_0000,x
  sta var_00DE
  lda var_0001,x
  sta var_00DC
  lda var_0002,x
  sta var_00DD
  lda var_0003,x
  sta var_00E1
  sta var_5FB3
  ldx #$80
  ldy #$00
  stx var_00E0
  sty var_00DF
  lda var_5FFA
  pha
  jsr sub_F2FB
  bit var_5FB2
  bpl loc_EBD3
  lda #$FF
  sta var_00D0
  sta var_00D1
  sta var_00D2
  sta var_00D3
  jsr sub_EF5D
loc_EB9C:
  ldx #$40
  ldy #$00
  lda var_5FB3
  cmp var_00E1
  bcc loc_EBB4
  bne loc_EBC0
  lda var_00E0
  and #$7F
  tax
  ora var_00DF
  beq loc_EBC0
  ldy var_00DF
loc_EBB4:
  lda var_5FB3
  jsr sub_EBE5
  inc var_5FB3
  jmp loc_EB9C
loc_EBC0:
  ldx #$03
loc_EBC2:
  lda var_00D0,x
  eor #$FF
  cmp var_00D4,x
  bne loc_EBD1
  dex
  bpl loc_EBC2
  ldy #$00
  beq loc_EBD3
loc_EBD1:
  ldy #$01
loc_EBD3:
  ldx #$2F
loc_EBD5:
  lda var_6100,x
  sta var_00D0,x
  dex
  bpl loc_EBD5
  pla
  sta var_5FFA
  sta loc_FF04
  rts

sub_EBE5:
  stx var_00FD
  sty var_00FC
  tax
  jsr bios_set_page
  tya
  beq loc_EBF2
  inc var_00FD
loc_EBF2:
  ldx #$80
  ldy #$00
  stx var_00F7
  sty var_00F6
  ldy #$00
loc_EBFC:
  lda (var_00F6),y
  eor var_00D0
  tax
  lda var_00D1
  sta var_00D0
  lda var_00D2
  sta var_00D1
  lda var_00D3
  sta var_00D2
  lda var_00D0
  eor var_6C00,x
  sta var_00D0
  lda var_00D1
  eor var_6D00,x
  sta var_00D1
  lda var_00D2
  eor var_6E00,x
  sta var_00D2
  lda var_6F00,x
  sta var_00D3
  iny
  bne loc_EC2C
  inc var_00F7
loc_EC2C:
  dec var_00FC
  bne loc_EBFC
  dec var_00FD
  bne loc_EBFC
  rts

sub_EC35:
  clc
  adc var_0000,x
  sta var_0000,x
  bcc loc_EC3E
  inc var_0001,x
loc_EC3E:
  rts

sub_EC3F:
  sty var_00FF
  lda var_00D8
  sec
  sbc var_00FF
  bcs loc_EC60
  ldx var_00D9
  bne loc_EC5E
  ldx var_00DA
  bne loc_EC5C
  ldx var_00DB
  bne loc_EC5A
  ldy var_00D8
  lda #$00
  beq loc_EC60
loc_EC5A:
  dec var_00DB
loc_EC5C:
  dec var_00DA
loc_EC5E:
  dec var_00D9
loc_EC60:
  sta var_00D8
  sty var_00EF
  inc var_00EF
  tya
  ldx #$62
  ldy #$00
  stx var_00FF
  sty var_00FE
loc_EC6F:
  sta var_00FA
  sta var_00FB
  ldx var_00DD
  ldy var_00DC
  stx var_00FD
  sty var_00FC
  bit var_5FB2
  bvc loc_EC96
  cpx #$BF
  bcc loc_EC91
  lda var_00FC
  clc
  adc var_00FA
  bcc loc_EC91
  lda #$00
  sbc var_00FC
  sta var_00FB
loc_EC91:
  ldx var_00DE
  jsr bios_set_page
loc_EC96:
  ldy #$00
loc_EC98:
  lda (var_00FC),y
  sta (var_00FE),y
  iny
  cpy var_00FB
  bne loc_EC98
  tya
  clc
  adc var_00DC
  sta var_00DC
  bcc loc_ECBA
  inc var_00DD
  bit var_5FB2
  bvc loc_ECBA
  bit var_00DD
  bvc loc_ECBA
  inc var_00DE
  lda #$80
  sta var_00DC
loc_ECBA:
  lda var_00FA
  cmp var_00FB
  beq loc_ECD1
  sbc var_00FB
  sta var_00FA
  tya
  clc
  adc var_00FE
  sta var_00FE
  bcc loc_EC6F
  inc var_00FF
  jmp loc_EC6F
loc_ECD1:
  rts

sub_ECD2:
  sty var_00F0
  ldy #$01
  jsr sub_EC3F
  ldy var_6200
  iny
  jsr sub_EC3F
  ldx #$00
loc_ECE2:
  lda var_6200,x
  lsr a
  lsr a
  lsr a
  lsr a
  tay
  sty var_00F1
  lda var_6200,x
  and #$0F
  clc
  adc #$01
loc_ECF4:
  sta (var_00F2),y
  dey
  bpl loc_ECF4
  inx
  lda var_00F2
  sec
  adc var_00F1
  sta var_00F2
  bcc loc_ED05
  inc var_00F3
loc_ED05:
  dec var_00F0
  beq loc_ED16
  lda var_00F0
  sec
  sbc var_00F1
loc_ED0E:
  beq loc_ED16
  bcc loc_ED16
  sta var_00F0
  bcs loc_ECE2
loc_ED16:
  rts
data_ED17:
  .byte $01, $02, $04, $08, $10, $20, $40, $80

sub_ED1F:
  ldy #$3F
loc_ED21:
  lda var_6B00,y
  sta var_00FF
  cmp #$10
  bcs loc_ED5E
  tya
  asl a
  tax
  lda var_6B40,x
  pha
  ldx var_00FF
  cpx #$08
  bcs loc_ED3F
  lda data_ED17,x
  tax
  lda #$00
  beq loc_ED44
loc_ED3F:
  lda loc_ED0E+1,x
  ldx #$00
loc_ED44:
  sta var_00F1
  stx var_00F0
  pla
  tax
loc_ED4A:
  lda var_00FF
  sta var_7F00,x
  tya
  sta var_7E00,x
  txa
  clc
  adc var_00F0
  tax
  lda #$00
  adc var_00F1
  beq loc_ED4A
loc_ED5E:
  tya
  asl a
  tax
  lda var_6C01,x
  sta var_00F3
  lda var_6C00,x
  sta var_00F2
  lda var_6BC0,y
  cmp #$09
  bcs loc_ED7D
  ldx #$70
  stx var_00F1
  ldx #$00
  stx var_00F0
  jmp loc_EDB6
loc_ED7D:
  ldx var_00F2
  beq loc_ED9C
  lda #$FF
  sta var_7000,x
  lda #$72
  sta var_00F1
  lda #$00
  sta var_00F0
  ldx #$04
  lda var_00F3
loc_ED92:
  lsr a
  ror var_00F2
  dex
  bne loc_ED92
  lda #$04
  bne loc_EDAE
loc_ED9C:
  lda #$74
  sta var_00F1
  lda #$00
  sta var_00F0
  lda var_00F3
  sta var_00F2
  lda #$00
  sta var_00F3
  lda #$08
loc_EDAE:
  sta var_00FF
  lda var_6BC0,y
  sec
  sbc var_00FF
loc_EDB6:
  tax
  tya
  clc
  adc var_00E7
  jsr sub_EE39
  dey
  bmi loc_EDC4
  jmp loc_ED21
loc_EDC4:
  rts

sub_EDC5:
  ldy #$00
loc_EDC7:
  dey
  tya
  asl a
  tax
  bcs loc_EDD8
  lda var_6E01,x
  sta var_00F3
  lda var_6E00,x
  jmp loc_EDE0
loc_EDD8:
  lda var_6F01,x
  sta var_00F3
  lda var_6F00,x
loc_EDE0:
  sta var_00F2
  ldx var_6D00,y
  cpx #$09
  bcs loc_EDF4
  lda #$76
  sta var_00F1
  lda #$00
  sta var_00F0
  jmp loc_EE31
loc_EDF4:
  ldx var_00F2
  beq loc_EE0C
  lda #$FF
  sta var_7600,x
  lda #$3F
  bit var_00F2
  beq loc_EE15
  lda #$78
  pha
  lda #$00
  ldx #$04
  bne loc_EE1C
loc_EE0C:
  lda #$7C
  pha
  lda #$00
  ldx #$08
  bne loc_EE1C
loc_EE15:
  lda #$7A
  pha
  lda #$00
  ldx #$06
loc_EE1C:
  sta var_00F0
  pla
  sta var_00F1
  stx var_00FF
loc_EE23:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_EE23
  lda var_6D00,y
  sec
  sbc var_00FF
  tax
loc_EE31:
  jsr sub_EE38
  tya
  bne loc_EDC7
  rts

sub_EE38:
  tya

sub_EE39:
  sty var_00FF
  sta var_00FE
  stx var_00FD
  cpx #$08
  bcs loc_EE4B
  lda data_ED17,x
  tay
  lda #$00
  beq loc_EE50
loc_EE4B:
  lda loc_ED0E+1,x
  ldy #$00
loc_EE50:
  sta var_00F5
  sty var_00F4
  ldy var_00F2
loc_EE56:
  lda var_00FE
  sta (var_00F0),y
  inc var_00F1
  txa
  sta (var_00F0),y
  dec var_00F1
  tya
  clc
  adc var_00F4
  tay
loc_EE66:
  lda var_00F5
  adc #$00
  beq loc_EE56
  ldy var_00FF
  rts
data_EE6F:
  .byte $80, $40, $20, $10, $08, $04, $02, $01

sub_EE77:
  sty var_00FE
  ldy #$10
  lda #$00
loc_EE7D:
  sta var_66FF,y
  dey
  bne loc_EE7D
  ldy #$00
loc_EE85:
  lda (var_00F2),y
  tax
  inc var_66FF,x
  iny
  cpy var_00FE
  bne loc_EE85
  lda #$00
  tax
loc_EE93:
  clc
  adc var_6700,x
  sta var_6700,x
  inx
  cpx #$10
  bne loc_EE93
  ldy var_00FE
loc_EEA1:
  dey
  lda (var_00F2),y
  tax
  dec var_66FF,x
  lda var_66FF,x
  tax
  lda (var_00F2),y
  sta var_6710,x
  tya
  bne loc_EEA1
  lda #$00
  sta var_00FF
  sta var_00F4
  sta var_00F5
  sta var_00F6
  sta var_00F7
  ldy var_00FE
loc_EEC2:
  dey
  clc
  lda var_00F4
  adc var_00F6
  sta var_00F4
  lda var_00F5
  adc var_00F7
  sta var_00F5
  lda var_6710,y
  cmp var_00FF
  beq loc_EEF0
  sta var_00FF
  tax
  dex
  cpx #$08
  bcs loc_EEE6
  lda data_EE6F,x
  ldx #$00
  beq loc_EEEC
loc_EEE6:
  lda loc_EE66+1,x
  tax
  lda #$00
loc_EEEC:
  sta var_00F7
  stx var_00F6
loc_EEF0:
  tya
  asl a
  tax
  lda var_00F4
  bcs loc_EF02
  sta var_6810,x
  lda var_00F5
  sta var_6811,x
  jmp loc_EF0A
loc_EF02:
  sta var_6910,x
  lda var_00F5
  sta var_6911,x
loc_EF0A:
  tya
  bne loc_EEC2
  lda var_00F0
  sta var_00F4
  ldy #$00
loc_EF13:
  lda (var_00F2),y
  tax
  tya
  pha
  asl a
  tay
  lda var_00F1
  adc #$00
  sta var_00F5
  lda var_66FF,x
  inc var_66FF,x
  asl a
  tax
  bcs loc_EF34
  lda var_6810,x
  pha
  lda var_6811,x
  jmp loc_EF3B
loc_EF34:
  lda var_6910,x
  pha
  lda var_6911,x
loc_EF3B:
  ldx #$08
loc_EF3D:
  asl a
  ror var_00FF
  dex
  bne loc_EF3D
  lda var_00FF
  sta (var_00F4),y
  iny
  pla
  ldx #$08
loc_EF4B:
  asl a
  ror var_00FF
  dex
  bne loc_EF4B
  lda var_00FF
  sta (var_00F4),y
  pla
  tay
  iny
  cpy var_00FE
  bne loc_EF13
  rts

sub_EF5D:
  lda #$00
  sta var_00F0
  ldy #$00
loc_EF63:
  dey
  lda #$00
  sta var_00F2
  sta var_00F3
  sta var_00F4
  sty var_00F5
  ldx #$08
loc_EF70:
  lsr var_00F2
  ror var_00F3
  ror var_00F4
  ror var_00F5
  bcc loc_EF92
  lda var_00F2
  eor #$ED
  sta var_00F2
  lda var_00F3
  eor #$B8
  sta var_00F3
  lda var_00F4
  eor #$83
  sta var_00F4
  lda var_00F5
  eor #$20
  sta var_00F5
loc_EF92:
  dex
  bne loc_EF70
  lda #$6C
  sta var_00F1
  ldx #$03
loc_EF9B:
  lda var_00F2,x
  sta (var_00F0),y
  inc var_00F1
  dex
  bpl loc_EF9B
  tya
  bne loc_EF63
  rts

sub_EFA8:
  sty var_00FE
  txa
  ldx var_00E1
  jsr sub_D316
  ldx #$63
  ldy #$00
  stx var_00FD
  sty var_00FC
  ldy #$00
  tax
  beq loc_EFCB
loc_EFBD:
  lda (var_00FC),y
  sta (var_00DF),y
  iny
  bne loc_EFBD
  inc var_00FD
  inc var_00E0
  dex
  bne loc_EFBD
loc_EFCB:
  ldx var_00FE
  beq loc_EFD9
loc_EFCF:
  lda (var_00FC),y
  sta (var_00DF),y
  iny
  dex
  bne loc_EFCF
  sty var_00DF
loc_EFD9:
  lda var_00E0
  cmp #$C0
  bcc loc_EFE5
  lda #$80
  sta var_00E0
  inc var_00E1
loc_EFE5:
  rts

sub_EFE6:
  lda #$00
  sec
  sbc var_00F8
  sta var_00F8
  lda #$63
  sbc var_00F9
  sta var_00F9
  ldx var_00E1
  lda var_00DF
  sec
  sbc var_00F8
  sta var_00EB
  lda var_00E0
  sbc var_00F9
  bcs loc_F00B
loc_F002:
  cpx var_5FB3
  beq loc_F086
  clc
  adc #$40
  dex
loc_F00B:
  cmp #$80
  bcc loc_F002
  sta var_00EC
  stx var_00ED
loc_F013:
  lda var_00F6
  sec
  sbc var_00F8
  tay
  lda var_00F7
  sbc var_00F9
  tax
  bcs loc_F02C
  lda var_00F6
  sta var_00F8
  lda var_00F7
  sta var_00F9
  ldx #$00
  ldy #$00
loc_F02C:
  stx var_00F7
  sty var_00F6
loc_F030:
  ldx var_00ED
  jsr sub_D316
  lda #$FF
  ldx var_00EC
  cpx #$BF
  bcc loc_F045
  ldx var_00EB
  beq loc_F045
  lda #$00
  sbc var_00EB
loc_F045:
  sta var_00FB
  tax
  lda var_00F8
  sec
  sbc var_00FB
  bcs loc_F059
  dec var_00F9
  bpl loc_F059
  ldx var_00F8
  lda #$00
  sta var_00F9
loc_F059:
  sta var_00F8
  ldy #$00
loc_F05D:
  lda (var_00EB),y
  sta (var_00F0),y
  iny
  dex
  bne loc_F05D
  ldx #$EB
  tya
  jsr sub_EC35
  ldx #$F0
  tya
  jsr sub_EC35
  lda var_00F8
  ora var_00F9
  beq loc_F085
  lda var_00EC
  cmp #$C0
  bcc loc_F030
  inc var_00ED
  lda #$80
  sta var_00EC
  bne loc_F030
loc_F085:
  rts
loc_F086:
  stx var_00ED
  sta var_00EC
  lda #$00
  sec
  sbc var_00EB
  tax
  lda #$80
  sbc var_00EC
  tay
  txa
  cmp var_00F6
  tya
  sbc var_00F7
  bcc loc_F0A1
  ldx var_00F6
  ldy var_00F7
loc_F0A1:
  stx var_00EB
  sty var_00EC
  lda var_00F6
  sec
  sbc var_00EB
  sta var_00F6
  lda var_00F7
  sbc var_00EC
  sta var_00F7
  txa
  bne loc_F0B6
  dey
loc_F0B6:
  sty var_00EB
  lda #$00
  tay
loc_F0BB:
  sta (var_00F0),y
  iny
  bne loc_F0C2
  inc var_00F1
loc_F0C2:
  dex
  bne loc_F0BB
  dec var_00EB
  bpl loc_F0BB
  tya
  clc
  adc var_00F0
  sta var_00F0
  bcc loc_F0D3
  inc var_00F1
loc_F0D3:
  lda var_00F6
  ora var_00F7
  beq loc_F085
  sty var_00EB
  lda var_00F8
  sec
  sbc var_00EB
  sta var_00F8
  bcs loc_F0E6
  dec var_00F9
loc_F0E6:
  lda #$80
  sta var_00EC
  lda #$00
  sta var_00EB
  jmp loc_F013

sub_F0F1:
  lda #$00
  sta var_00E4
  ldy #$FF
  jsr sub_EC3F
  ldx #$63
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldx #$00
  stx var_00F5
  lda var_6200,x
  sta var_00F2
  stx var_00EE
  inc var_00EE
  dec var_00EF
  jmp loc_F1A0
loc_F114:
  ldx #$7A
  ldy #$00
  lda #$06
loc_F11A:
  sta var_00F4
  stx var_00F7
  sty var_00F6
  jsr sub_F2C1
  inc var_00F7
  lda (var_00F6),y
  sta var_00F4
  dec var_00F7
  lda (var_00F6),y
  sta var_00F6
  jmp loc_F15A
loc_F132:
  ldx #$7C
  ldy #$00
  lda #$08
  bne loc_F11A
loc_F13A:
  lda #$3F
  bit var_00F2
  beq loc_F114
  ldx #$78
  ldy #$00
  lda #$04
  bne loc_F11A
loc_F148:
  lda var_00F2
  beq loc_F132
  lda var_7600,y
  cmp #$FF
  beq loc_F13A
  sta var_00F6
  lda var_7700,y
  sta var_00F4
loc_F15A:
  jsr sub_F2C1
  lda var_00F6
  ldy #$00
  sta (var_00F0),y
  inc var_00F0
  bne loc_F169
  inc var_00F1
loc_F169:
  jmp loc_F253
loc_F16C:
  lda var_00E3
  bne loc_F148
  lda var_00F2
  sta var_00F6
  lda #$08
  sta var_00F4
  bne loc_F15A
loc_F17A:
  ldx #$74
  ldy #$00
  lda #$08
loc_F180:
  sta var_00F4
  stx var_00F7
  sty var_00F6
  jsr sub_F2C1
  inc var_00F7
  lda (var_00F6),y
  sta var_00F4
  dec var_00F7
  lda (var_00F6),y
  sta var_00F6
  jmp loc_F1E1
loc_F198:
  ldx #$72
  ldy #$00
  lda #$04
  bne loc_F180
loc_F1A0:
  lda #$01
  sta var_00F4
  jsr sub_F2C1
  bcs loc_F16C
  lda var_00F2
  and var_00E8
  sta var_00F7
  lda var_00E9
  sta var_00F4
  jsr sub_F2C1
  lda var_7F00,y
  sta var_00F4
  lda #$00
  sta var_00E6
  lda var_7E00,y
  ldy var_00E9
loc_F1C4:
  asl a
  rol var_00E6
  dey
  bne loc_F1C4
  ora var_00F7
  sta var_00E5
  jsr sub_F2C1
  lda var_00F2
  beq loc_F17A
  lda var_7000,y
  bmi loc_F198
  sta var_00F6
  lda var_7100,y
  sta var_00F4
loc_F1E1:
  jsr sub_F2C1
  lda #$00
  sta var_00F7
  lda var_00F6
  cmp var_00EA
  bne loc_F1FE
  clc
  adc var_00F2
  sta var_00F6
  bcc loc_F1F7
  inc var_00F7
loc_F1F7:
  lda #$08
  sta var_00F4
  jsr sub_F2C1
loc_F1FE:
  lda var_00F0
  clc
  sbc var_00E5
  sta var_00F8
  lda var_00F1
  sbc var_00E6
  sta var_00F9
  bcc loc_F211
  cmp #$63
  bcs loc_F21C
loc_F211:
  jsr sub_EFE6
  ldx #$63
  ldy #$00
  stx var_00F9
  sty var_00F8
loc_F21C:
  lda var_00F6
  ora var_00F7
  beq loc_F253
  ldy #$FF
  lda var_00F6
  sec
  sbc #$FF
  bcs loc_F235
  dec var_00F7
  bpl loc_F235
  ldy var_00F6
  lda #$00
  sta var_00F7
loc_F235:
  sta var_00F6
  sty var_00FF
  ldy #$00
loc_F23B:
  lda (var_00F8),y
  sta (var_00F0),y
  iny
  cpy var_00FF
  bne loc_F23B
  ldx #$F8
  tya
  jsr sub_EC35
  ldx #$F0
  tya
  jsr sub_EC35
  jmp loc_F21C
loc_F253:
  lda var_00F1
  cmp #$6B
  bcs loc_F25C
  jmp loc_F1A0
loc_F25C:
  ldx #$08
  ldy #$00
  jsr sub_EFA8
  lda var_00F0
  sta var_00FF
  ldy #$00
  lda var_00F1
  sec
  sbc #$08
  sta var_00F1
  cmp #$63
  beq loc_F28B
loc_F274:
  lda var_6B00,y
  sta var_6300,y
  iny
  bne loc_F274
loc_F27D:
  cpy var_00FF
  beq loc_F298
  lda var_6C00,y
  sta var_6400,y
  iny
  jmp loc_F27D
loc_F28B:
  cpy var_00FF
  beq loc_F298
  lda var_6B00,y
  sta var_6300,y
  iny
  bne loc_F28B
loc_F298:
  jmp loc_F1A0
loc_F29B:
  pla
  pla
  pla
  pla
  lda var_00F0
  sec
  sbc #$00
  tay
  lda var_00F1
  sbc #$63
  tax
  rts

sub_F2AB:
  lda var_00E4
  bne loc_F29B
  ldy #$FF
  jsr sub_EC3F
  lda #$00
  sta var_00EE
  dec var_00EF
  bne loc_F2C0
  inc var_00EF
  inc var_00E4
loc_F2C0:
  rts

sub_F2C1:
  lda var_00F5
  cmp var_00F4
  bcs loc_F2EB
  tax
  beq loc_F2D1
loc_F2CA:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_F2CA
loc_F2D1:
  dec var_00EF
  bne loc_F2D8
  jsr sub_F2AB
loc_F2D8:
  ldx var_00EE
  lda var_6200,x
  inc var_00EE
  sta var_00F3
  lda var_00F4
  sec
  sbc var_00F5
  sta var_00F4
  lda #$08
  sec
loc_F2EB:
  sbc var_00F4
  sta var_00F5
  ldx var_00F4
loc_F2F1:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_F2F1
  ldy var_00F2
  rts

sub_F2FB:
  lda #$FF
  sta var_00D8
  ldy #$0A
  jsr sub_EC3F
  lda var_6200
  sta var_00E2
  ldx #$07
loc_F30B:
  lda var_6202,x
  sta var_00D4,x
  dex
  bpl loc_F30B
  ldx #$00
  lda #$04
  bit var_00E2
  beq loc_F342
  ldx #$6D
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldy #$00
  jsr sub_ECD2
  ldx #$6D
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldx #$6E
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldy #$00
  jsr sub_EE77
  jsr sub_EDC5
  ldx #$01
loc_F342:
  stx var_00E3
  lda #$02
  bit var_00E2
  bne loc_F358
  lda #$02
  sta var_00E7
  lda #$3F
  sta var_00E8
  lda #$06
  sta var_00E9
  bne loc_F364
loc_F358:
  lda #$03
  sta var_00E7
  lda #$7F
  sta var_00E8
  lda #$07
  sta var_00E9
loc_F364:
  lda var_00E7
  clc
  adc #$3F
  sta var_00EA
  ldx #$6B
  ldy #$C0
  stx var_00F3
  sty var_00F2
  ldy #$40
  jsr sub_ECD2
  ldx #$6B
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldy #$40
  jsr sub_ECD2
  ldx #$6B
  ldy #$C0
  stx var_00F3
  sty var_00F2
  ldx #$6C
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldy #$40
  jsr sub_EE77
  ldx #$6B
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldx #$6B
  ldy #$40
  stx var_00F1
  sty var_00F0
  ldy #$40
  jsr sub_EE77
  jsr sub_ED1F
  jsr sub_F0F1
  jmp sub_EFA8
