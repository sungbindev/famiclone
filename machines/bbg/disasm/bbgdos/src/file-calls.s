; 模块：file-calls
; 路径解析与其余文件调用：查找文件、定位、删除、改名、属性、子目录

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2177"
; 段 0，$9D77-$A5BB
sub_9D77:
  jsr sub_85D6
  bcs sub_9D99
  lda data_802E
  cmp #$FF
  beq sub_9D99
  jsr sub_820C
  ldy #$00
loc_9D88:
  lda data_8032,y
  sta data_9C33,x
  inx
  iny
  cpy #$50
  bne loc_9D88
  lda #$FF
  sta data_9C33,x

sub_9D99:
  ldx #$FF
  stx data_801B
  stx data_801A
  stx data_801E
  inx
  stx data_8017
  stx data_8016
  clc
  rts

sub_9DAD:
  lda #$FF
  sta data_802A

sub_9DB2:
  cpx data_802A
  beq loc_9DD5
  stx var_00F8
  sty var_00F9
  lda data_8028
  sta var_00FF
  lda data_8027
  sta var_00FE
  lda #$20
  ldx data_8029
  ldy var_00F8
  jsr BIOS_ENTRY
  ldx var_00F8
  ldy var_00F9
  cmp #$00
loc_9DD5:
  rts

sub_9DD6:
  ldy #$02
  lda (var_00EE),y
  sta data_8029
  iny
  lda (var_00EE),y
  sta data_8027
  iny
  lda (var_00EE),y
  sta data_8028
  rts

sub_9DEA:
  jsr sub_9DD6
  ldx #$00
  stx var_00F3
loc_9DF1:
  jsr sub_9DAD
  beq loc_9DFF
  inx
  cmp #$5C
  bne loc_9DF1
  stx var_00F3
  beq loc_9DF1
loc_9DFF:
  ldx var_00F3
  beq loc_9E14
  dex
  beq loc_9E0F
  jsr sub_A28A
  bcs loc_9E1C
  ldx var_00F3
  bne loc_9E14
loc_9E0F:
  jsr sub_A27B
  ldx #$01
loc_9E14:
  jsr sub_9DAD
  beq loc_9E1D
  jsr sub_A151
loc_9E1C:
  rts
loc_9E1D:
  lda #$03
  rts

sub_9E20:
  ldy #$05
  ldx #$01
loc_9E24:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E34
  inx
  iny
  cpy #$0D
  bne loc_9E24
loc_9E34:
  ldy #$0D
  lda (var_00EE),y
  cmp #$20
  beq loc_9E4D
  inx
loc_9E3D:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E4D
  inx
  iny
  cpy #$10
  bne loc_9E3D
loc_9E4D:
  clc
  rts
loc_9E4F:
  lda #$05
  rts

; 文件调用 24（手册未列）：按通道块偏移 $16 的属性值改写目录项的属性，保留目录与卷标位。属性值含 $D8 的位时返回 $05（AccessDenied）。
file_call_write_attribute:
  ldy #$16
  lda (var_00EE),y
  and #$D8
  bne loc_9E79
  lda #$37
  ldy #$08
  jsr sub_84CE
  bcs loc_9E78
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ldy #$16
  ora (var_00EE),y
  ldy #$0B
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_9E78
  lda #$00
loc_9E78:
  rts
loc_9E79:
  lda #$05
  rts

; 文件调用 22 FilePathFirst：按路径查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名，偏移 $16 是属性掩码（取 $A1 的补码与掩码合成）；查找状态记在偏移 $2B 起（页号、$9BF5 与当前目录簇）。
; 出口与 FileFirst 相同，之后用 FileNext 继续。
file_call_path_first:
  ldy #$16
  lda (var_00EE),y
  tax
  and #$10
  beq loc_9E89
  txa
  and #$F7
  tax
loc_9E89:
  txa
  ora #$A1
  eor #$FF
  jsr sub_9F22
  pha
  ldy #$2B
  lda data_A53A
  sta (var_00EE),y
  iny
  lda #$F5
  sta (var_00EE),y
  iny
  lda #$9B
  sta (var_00EE),y
  lda #$00
  tax
  bit data_802D
  bpl loc_9EB1
  ldx data_802C
  lda data_802B
loc_9EB1:
  ldy #$2E
  sta (var_00EE),y
  iny
  txa
  sta (var_00EE),y
  pla
  rts

; 文件调用 8 FileCreate、21 FilePathCreate：建立文件。
; 已读代码：文件名里含 '?' 通配符时返回 $05（AccessDenied）；已有同名的目录或卷标项时也返回 $05。
; 未确认：其余行为来自手册功能名，代码尚未逐条核对。
file_call_create:
  jsr sub_9E20
  bcs loc_9EE4
  lda #$3F
  ldy #$00
  jsr sub_8B91
  bcs loc_9EE5
  lda data_8023
  and #$18
  beq loc_9ED3
  lda #$05
  rts
loc_9ED3:
  jsr sub_8EEB
  bcs loc_9EE4
  lda #$20
  jsr sub_8AB6
  jsr sub_840D
  bcs loc_9EE4
  lda #$00
loc_9EE4:
  rts
loc_9EE5:
  cmp #$02
  bne loc_9EF5
  ldx var_00FE
  inx
  bne loc_9EFB
  bit data_802D
  bmi loc_9EF6
  lda #$05
loc_9EF5:
  rts
loc_9EF6:
  jsr sub_8B0C
  bcs loc_9EF5
loc_9EFB:
  jsr sub_8AEF
  bcs loc_9EF5
  ldx #$20
  jsr sub_8AB6
  jsr sub_840D
  bcs loc_9EF5
  jmp loc_8BBA

sub_9F0D:
  lda data_800D
  clc
  adc #$17
  ldy #$03
  sta (var_00EE),y
  lda data_800E
  adc #$00
  iny
  sta (var_00EE),y
  rts

; 文件调用 6 FileFirst：查找第一个匹配的文件。
; 入口：通道块偏移 5 起是文件名（'?' 通配），偏移 $16 是属性掩码（本入口先置为 $FF）。
; 出口：通道块偏移 3、4 指向偏移 $17 的查找结果区，其中是目录项副本；查找状态记在偏移 $0F-$15 与 $2B 起，供 FileNext 继续使用。
file_call_first:
  lda #$FF

sub_9F22:
  ldy #$16
  sta (var_00EE),y
  sta data_8023
  pha
  jsr sub_9F0D
  pla
  jsr sub_8457
  bcs loc_9F38
  jsr sub_8FC0
  bcc loc_9F80
loc_9F38:
  rts

; 文件调用 7 FileNext：接着 FileFirst 的状态查找下一个匹配的文件；没有更多文件时返回出错码 $12（SearchEnding）。
file_call_next:
  ldy #$16
  lda (var_00EE),y
  sta data_8023
  jsr sub_9F0D
  jsr sub_8230
  jsr sub_83F5
  ldy #$15
  lda (var_00EE),y
  sta data_8022
  dey
  lda (var_00EE),y
  sta var_00FB
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  clc
  adc #$10
  sta var_00FA
  pla
  tay
  pla
  bcc loc_9F7B
  jsr sub_84FE
  bcc loc_9F7B
  cmp #$02
  bne loc_9F79
  lda #$12
loc_9F79:
  sec
  rts
loc_9F7B:
  jsr sub_8FC0
  bcs loc_9F79
loc_9F80:
  ldy #$15
  lda data_8022
  sta (var_00EE),y
  dey
  lda var_00FB
  sta (var_00EE),y
  dey
  lda data_801A
  sta (var_00EE),y
  dey
  lda data_8019
  sta (var_00EE),y
  dey
  lda data_8018
  sta (var_00EE),y
  dey
  lda var_00FA
  sta (var_00EE),y
  ldy #$0B
  lda (var_00F1),y
  pha
  ldy #$16
loc_9FAA:
  lda (var_00F1),y
  pha
  iny
  cpy #$1A
  bne loc_9FAA
  ldy #$1C
loc_9FB4:
  lda (var_00F1),y
  pha
  iny
  cpy #$20
  bne loc_9FB4
  ldy #$20
loc_9FBE:
  dey
  pla
  sta (var_00EE),y
  cpy #$17
  bne loc_9FBE
  ldy #$00
loc_9FC8:
  lda (var_00F1),y
  pha
  iny
  cpy #$0B
  bne loc_9FC8
  ldy #$2B
loc_9FD2:
  dey
  pla
  sta (var_00EE),y
  cpy #$20
  bne loc_9FD2
  lda #$00
  rts

; 文件调用 9 FileDrive：设置当前默认驱动器。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_set_drive:
  ldy #$02
  lda data_8006
  sta (var_00EE),y
  ldy #$01
  lda (var_00EE),y
  beq loc_9FFD
  cmp data_8006
  bcc loc_9FF1
  bne loc_A00C
loc_9FF1:
  cmp #$03
  bcs loc_9FFA
  ldy data_8001
  beq loc_A00C
loc_9FFA:
  sta data_8003
loc_9FFD:
  lda data_8003
  sta (var_00EE),y
  ldy #$04
  lda data_802F
  sta (var_00EE),y
  lda #$00
  rts
loc_A00C:
  lda #$0F
  rts

; 文件调用 4 FileSeek：移动文件读写位置指针。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_seek:
  ldx #$03
  ldy #$0D
loc_A013:
  lda (var_00EE),y
  sta var_00F1,x
  iny
  dex
  bpl loc_A013
  ldy #$07
  ldx #$03
  sec
loc_A020:
  lda var_00F1,x
  sbc (var_00EE),y
  iny
  dex
  bpl loc_A020
  bcs loc_A036
  ldx #$03
  ldy #$07
loc_A02E:
  lda var_00F1,x
  sta (var_00EE),y
  iny
  dex
  bpl loc_A02E
loc_A036:
  lda #$00
  rts

; 文件调用 11 FileDriveSize：取驱动器容量与剩余空间。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_drive_size:
  ldx #$00
  ldy #$02
  stx var_00F5
  sty var_00F4
  stx var_00F2
  stx var_00F1
loc_A045:
  stx var_00F7
  sty var_00F6
  jsr sub_8667
  bcs loc_A0A5
  jsr sub_8885
  bcs loc_A0A5
  ora var_00F6
  bne loc_A05D
  inc var_00F1
  bne loc_A05D
  inc var_00F2
loc_A05D:
  inc var_00F4
  bne loc_A063
  inc var_00F5
loc_A063:
  ldx var_00F5
  ldy var_00F4
  cpx data_8074
  bne loc_A045
  cpy data_8073
  bne loc_A045
  ldy #$06
  lda #$02
  sta (var_00EE),y
  lda #$00
  dey
  sta (var_00EE),y
  ldy #$04
  sta (var_00EE),y
  lda data_8070
  dey
  sta (var_00EE),y
  ldy #$09
  lda data_8073
  sec
  sbc #$02
  sta (var_00EE),y
  iny
  lda data_8074
  sbc #$00
  sta (var_00EE),y
  ldy #$08
  lda var_00F2
  sta (var_00EE),y
  dey
  lda var_00F1
  sta (var_00EE),y
  lda #$00
loc_A0A5:
  rts

; 文件调用 10 FileDelete、23 FilePathDelete：删除文件。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_delete:
  jsr file_call_open
  bcs loc_A0C9
  jsr sub_8EEB
  bcs loc_A0C9
  jsr sub_859F
  bcs loc_A0C9
  ldy #$11
  lda (var_00EE),y
  jsr sub_8403
  lda #$E5
  ldy #$00
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_A0C9
  lda #$00
loc_A0C9:
  rts

; 文件调用 14 FileReadAbs：按绝对扇区号读磁盘。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_read_absolute:
  lda #$02
  bne loc_A0D0

; 文件调用 15 FileWriteAbs：按绝对扇区号写磁盘。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_write_absolute:
  lda #$03
loc_A0D0:
  sta var_00FD
  jsr sub_8230
  ldy #$09
  lda (var_00EE),y
  sta data_8011
  dey
  lda (var_00EE),y
  sta data_8010
  dey
  lda (var_00EE),y
  sta data_800F
  ldy #$05
  lda (var_00EE),y
  jsr sub_8188
  bcs loc_A0F3
  lda #$00
loc_A0F3:
  rts

; 文件调用 13 FileRename：给文件改名。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_rename:
  jsr sub_9E20
  bcs loc_A10B
  jsr sub_8F62
  bcs loc_A10B
  jsr sub_83F5
  jsr sub_8AA2
  jsr sub_840D
  bcs loc_A10B
  lda #$00
loc_A10B:
  rts

; 文件调用 12 FileAttribute：取或设置文件属性。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_attribute:
  jsr sub_8F62
  bcs loc_A14D
  ldy #$02
  lda (var_00EE),y
  sta var_00F0
  and #$18
  bne loc_A14E
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ora var_00F0
  sta (var_00F1),y
  ldy #$03
  lda (var_00EE),y
  tax
  iny
  lda (var_00EE),y
  ldy #$19
  sta (var_00F1),y
  dey
  txa
  sta (var_00F1),y
  ldy #$06
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  ldy #$16
  sta (var_00F1),y
  iny
  txa
  sta (var_00F1),y
  ldy #$19
  lda #$01
  sta (var_00EE),y
  lda #$00
loc_A14D:
  rts
loc_A14E:
  lda #$05
  rts

sub_A151:
  ldy #$05
  jsr sub_9DB2
  cmp #$2E
  bne loc_A174
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
  cmp #$2E
  bne loc_A170
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
loc_A170:
  lda #$02
  sec
  rts
loc_A174:
  inx
  cmp #$2A
  beq loc_A196
  cpy #$0D
  beq loc_A1A2
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  jsr sub_9DB2
  beq loc_A192
  cmp #$5C
  beq loc_A192
  cmp #$2E
  bne loc_A174
loc_A192:
  lda #$20
  bne loc_A198
loc_A196:
  lda #$3F
loc_A198:
  cpy #$0D
  beq loc_A1A2
  sta (var_00EE),y
  iny
  jmp loc_A198
loc_A1A2:
  jsr sub_9DB2
  beq loc_A1B0
  cmp #$5C
  beq loc_A1B0
  inx
  cmp #$2E
  bne loc_A1A2
loc_A1B0:
  jsr sub_9DB2
  beq loc_A1D0
  cmp #$5C
  beq loc_A1D0
  inx
  cmp #$2A
  beq loc_A1CC
  cpy #$10
  beq loc_A1DC
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  bne loc_A1B0
loc_A1CC:
  lda #$3F
  bne loc_A1D2
loc_A1D0:
  lda #$20
loc_A1D2:
  cpy #$10
  beq loc_A1DC
  sta (var_00EE),y
  iny
  jmp loc_A1D2
loc_A1DC:
  jsr sub_9DB2
  beq loc_A1E8
  cmp #$5C
  beq loc_A1E8
  inx
  bne loc_A1DC
loc_A1E8:
  clc
loc_A1E9:
  rts

sub_A1EA:
  ldx #$FF
loc_A1EC:
  inx
  lda data_9BF5,x
  bne loc_A1EC
  stx var_00F0
  lda #$5C
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
  ldy #$00
loc_A200:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A20F
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A20F:
  cpy #$08
  bne loc_A200
  lda (var_00F1),y
  cmp #$20
  beq loc_A236
  lda #$2E
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A223:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A232
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A232:
  cpy #$0B
  bne loc_A223
loc_A236:
  clc
  bcc loc_A23B
loc_A239:
  ldx var_00F0
loc_A23B:
  lda #$00
  sta data_9BF5,x
  rts

sub_A241:
  bit data_802D
  bpl loc_A277
  jsr sub_8480
  jsr sub_841C
  bcs loc_A27A
  ldx data_943B
  ldy data_943A
  txa
  ora data_943A
  beq sub_A27B
  stx data_802C
  sty data_802B
  ldx #$FF
loc_A262:
  inx
  lda data_9BF5,x
  bne loc_A262
loc_A268:
  dex
  lda data_9BF5,x
  cmp #$5C
  bne loc_A268
  lda #$00
  sta data_9BF5,x
  clc
  rts
loc_A277:
  lda #$03
  sec
loc_A27A:
  rts

sub_A27B:
  lda #$00
  sta data_802D
  sta data_9BF5
  sta data_802C
  sta data_802B
  rts

sub_A28A:
  stx data_802A
  ldx #$00
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A29C
  jsr sub_A27B
  inx
loc_A29C:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2B8
  inx
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2E7
  inx
  stx var_00F9
  jsr sub_A241
  bcc loc_A2E5
  rts
loc_A2B8:
  jsr sub_A151
  bcs loc_A2CD
  stx var_00F9
  lda #$10
  ldy #$08
  jsr sub_84D2
  bcc loc_A2CE
  cmp #$02
  beq loc_A2F5
  sec
loc_A2CD:
  rts
loc_A2CE:
  jsr sub_A1EA
  bcs loc_A2F5
  ldy #$1B
  lda (var_00F1),y
  sta data_802C
  dey
  lda (var_00F1),y
  sta data_802B
  lda #$FF
  sta data_802D
loc_A2E5:
  ldx var_00F9
loc_A2E7:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A2F5
  inx
  bne loc_A29C
loc_A2F3:
  clc
  rts
loc_A2F5:
  lda #$03
  rts

sub_A2F8:
  ldx data_807B
  ldy data_807A
  stx data_802C
  sty data_802B
  lda data_807C
  sta data_802D
  ldx #$FF
loc_A30C:
  inx
  lda data_8032,x
  sta data_9BF5,x
  bne loc_A30C
  rts

sub_A316:
  ldx data_802C
  ldy data_802B
  stx data_807B
  sty data_807A
  lda data_802D
  sta data_807C
  ldx #$FF
loc_A32A:
  inx
  lda data_9BF5,x
  sta data_8032,x
  bne loc_A32A
  rts

; 文件调用 16 FileChangeDir：改变当前目录。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_change_dir:
  jsr sub_9DD6
  ldx #$FF
  jsr sub_A28A
  bcs loc_A343
  jsr sub_A316
  lda #$00
loc_A343:
  rts

; 文件调用 17 FileMakeDir：建立子目录。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_make_dir:
  jsr sub_9E20
  bcs loc_A37B
  stx var_00F0
  ldx #$FF
loc_A34D:
  inx
  lda data_9BF5,x
  bne loc_A34D
  txa
  adc var_00F0
  cmp #$3E
  bcs loc_A363
  lda #$3F
  ldy #$00
  jsr sub_84CE
  bcs loc_A366
loc_A363:
  lda #$05
  rts
loc_A366:
  cmp #$02
  bne loc_A37B
  ldx var_00FE
  inx
  bne loc_A381
  bit data_802D
  bmi loc_A37C
  lda #$52
  sta data_8031
  lda #$05
loc_A37B:
  rts
loc_A37C:
  jsr sub_8B0C
  bcs loc_A390
loc_A381:
  ldx data_8081
  ldy data_8080
  stx var_00F7
  sty var_00F6
  jsr sub_883E
  bcc loc_A39B
loc_A390:
  cmp #$27
  sec
  bne loc_A37B
  sta data_8031
  lda #$05
  rts
loc_A39B:
  jsr sub_8AEF
  bcs loc_A37B
  ldx #$10
  jsr sub_8AB6
  ldy #$1B
  lda data_8081
  sta (var_00F1),y
  tax
  dey
  lda data_8080
  sta (var_00F1),y
  tay
  lda data_807F
  sta data_868F
  lda #$FF
  sta data_868E
  sta var_00FC
  jsr sub_86A5
  bcs loc_A37B
  jsr sub_840D
  bcs loc_A37B
  jsr sub_85D6
  bcs loc_A37B
  jsr sub_8ADF
  ldx data_8081
  ldy data_8080
  stx data_921B
  sty data_921A
  jsr sub_8486
  bcs loc_A456
  sta data_801E
  stx data_801D
  sty data_801C
  ldx #$0A
  lda #$20
loc_A3F1:
  sta data_9200,x
  sta data_9220,x
  dex
  bne loc_A3F1
  lda #$2E
  sta data_9200
  sta data_9220
  sta data_9221
  ldx #$92
  ldy #$00
  stx var_00F2
  sty var_00F1
  lda #$10
  jsr sub_8AC2
  lda #$20
  sta var_00F1
  lda #$10
  jsr sub_8AC2
  bit data_802D
  bpl loc_A42C
  ldx data_802C
  ldy data_802B
  stx data_923B
  sty data_923A
loc_A42C:
  lda #$03
  jsr sub_81EB
  bcs loc_A456
  jsr sub_8ADF
  ldx data_8070
  stx var_00F3
loc_A43B:
  dec var_00F3
  bne loc_A442
  lda #$00
  rts
loc_A442:
  inc data_801C
  bne loc_A44F
  inc data_801D
  bne loc_A44F
  inc data_801E
loc_A44F:
  lda #$03
  jsr sub_81EB
  bcc loc_A43B
loc_A456:
  rts

sub_A457:
  lda data_802C
  pha
  lda data_802B
  pha
  stx data_802C
  sty data_802B
  lda #$00
  sta data_8022
  lda data_8070
  sta var_00FB
  lda #$40
  sta var_00FA
  dec data_802D
  jsr sub_8486
  bcs loc_A4B1
loc_A47B:
  jsr sub_841C
  bcs loc_A4B1
  ldy var_00FA
loc_A482:
  lda data_9400,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A482
loc_A491:
  lda data_9500,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A491
  sty var_00FA
  jsr sub_84F5
  bcc loc_A47B
  cmp #$02
  beq loc_A4B1
  lda #$00
  beq loc_A4B1
loc_A4AF:
  lda #$05
loc_A4B1:
  sta var_00F0
  ldx data_802C
  ldy data_802B
  pla
  sta data_802B
  pla
  sta data_802C
  inc data_802D
  lda var_00F0
  rts

; 文件调用 18 FileRemoveDir：删除空子目录，试图删除当前目录时按手册返回 $10。
; 未确认：含义来自手册功能名与出错码表，代码尚未逐条核对。
file_call_remove_dir:
  lda #$10
  ldy #$08
  jsr sub_84CE
  bcc loc_A4D7
  cmp #$02
  bne loc_A524
  lda #$03
  rts
loc_A4D7:
  jsr sub_8BEC
  ldy #$1B
  lda (var_00F1),y
  tax
  dey
  lda (var_00F1),y
  tay
  cpx data_807B
  bne loc_A4F0
  cpy data_807A
  bne loc_A4F0
  lda #$10
  rts
loc_A4F0:
  jsr sub_A457
  cmp #$00
  bne loc_A524
  lda #$FF
  sta var_00FC
loc_A4FB:
  lda #$00
  sta data_868E
  sta data_868F
  jsr sub_86A5
  bcs loc_A524
  jsr sub_8659
  bcc loc_A4FB
  jsr sub_859F
  bcs loc_A524
  jsr sub_8F62
  bcs loc_A524
  ldy #$00
  lda #$E5
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_A524
  lda #$00
loc_A524:
  rts

; 文件调用 19 FileGetDir：取当前目录路径。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_get_dir:
  ldy #$02
  lda data_A53A
  sta (var_00EE),y
  ldy #$04
  lda #$80
  sta (var_00EE),y
  dey
  lda #$32
  sta (var_00EE),y
  lda #$00
  rts
data_A53A:
  .byte $00
data_A53B:
  .byte $00
data_A53C:
  .byte $00
data_A53D:
  .byte $00
data_A53E:
  .byte $00
data_A53F:
  .byte $00
data_A540:
  .byte $00, $00, $00
data_A543:
  .byte $00
data_A544:
  .byte $00
data_A545:
  .byte $00
data_A546:
  .byte $00
data_A547:
  .byte $00
data_A548:
  .byte $00
data_A549:
  .res 40, $00
data_A571:
  .byte $0D
  .res 60, $00
data_A5AE:
  .byte $00
data_A5AF:
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
