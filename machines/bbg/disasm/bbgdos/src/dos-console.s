; 模块：dos-console
; 控制台输入输出与 DOS 调用分发：键盘、字符与字符串显示、命令行编辑

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_29BC"
; 段 0，$A5BC-$AAA8
sub_A5BC:
  cmp #$03
  beq loc_A5C6
  cmp #$00
  bne loc_A5C6
  cpx #$00
loc_A5C6:
  rts

sub_A5C7:
  lda #$34
  ldx #$00
  jmp BIOS_ENTRY

sub_A5CE:
  lda #$18
  ldx #$00
  jmp BIOS_ENTRY

sub_A5D5:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

sub_A5DC:
  jsr dos_check_keyboard
  beq loc_A5FC
  cmp #$13
  bne loc_A5F1
  jsr sub_A5CE
  jsr sub_A5CE
  jsr sub_A5BC
  beq loc_A5F9
  rts
loc_A5F1:
  jsr sub_A5BC
  bne loc_A5FC
  jsr sub_A5CE
loc_A5F9:
  jsr CTRL_C_ENTRY
loc_A5FC:
  rts

sub_A5FD:
  stx data_A543
  ldx data_A53D
  lda var_00FF
  pha
  lda #$20
  jsr BIOS_ENTRY
  tax
  pla
  sta var_00FF
  txa
  ldx data_A543
  rts

sub_A614:
  lda var_00FF
  pha
  lda #$21
  ldx data_A53D
  jsr BIOS_ENTRY
  pla
  sta var_00FF
  rts

sub_A623:
  cmp #$21
  bcc loc_A659
  beq loc_A662
  cmp #$23
  bcc loc_A659
  cmp #$2A
  bcc loc_A662
  cmp #$2D
  beq loc_A662
  cmp #$30
  bcc loc_A659
  cmp #$3A
  bcc loc_A662
  cmp #$3F
  bcc loc_A659
  cmp #$5B
  bcc loc_A662
  cmp #$5E
  bcc loc_A659
  cmp #$61
  bcc loc_A662
  cmp #$7B
  bcc loc_A660
  cmp #$7C
  beq loc_A659
  cmp #$7F
  bne loc_A662
loc_A659:
  lda #$02
  sta data_A53F
  sec
  rts
loc_A660:
  and #$DF
loc_A662:
  clc
  rts

; DOS 调用分发表：17 项（功能号 0-16），每项 2 字节，内容是处理例程地址减 1（用 RTS 跳转）。
dos_call_table:
data_A665 = * + 1
  .word dos_program_return - 1, dos_read_key_echo - 1
  .word dos_display_char - 1, dos_inquire_pages - 1
  .word dos_display_string - 1, dos_run_program - 1
  .word dos_get_kb_string - 1, dos_read_key_no_echo - 1
  .word dos_read_kb_clear - 1, dos_initial_screen - 1
  .word dos_release_pages - 1, dos_get_set_disp_mode - 1
  .word dos_file_name_access - 1, dos_get_set_init_screen - 1
  .word dos_check_keyboard - 1, dos_run_path_program - 1
  .word dos_get_version - 1

; DOS 调用分发（入口 $580C 的桩代码把页切到 DOS 页后调用本例程）。
; 入口：A 为功能号 0-16，X、Y 为各功能自己的参数。
; 出口：功能号大于 16 时 A=1（ErrorCommand）且 C=1；否则转到 dos_call_table 里的处理例程，由它设置 C 与 A。
dos_call_dispatch:
  asl a
  cmp #$22
  bcs loc_A69B
  stx data_A543
  tax
  lda data_A665,x
  pha
  lda dos_call_table,x
  pha
  ldx data_A543
  clc
loc_A69B:
  lda #$01
  rts

; DOS 功能 16 DosGetDOSVersion（手册未列）：取 DOS 版本。
; 出口：A=3，X=0，C=0，对应屏幕上显示的 BBGDOS Version 3.0。
dos_get_version:
  lda #$03
  ldx #$00
  clc
  rts

; DOS 功能 0 DosProgramReturn：程序结束返回。
; 入口：X 为返回码；Y 位 0=1 先初始化显示屏幕（DosInitialScreen），位 1=1 先初始化系统资源（BIOS 功能 4）。
; 副作用：把堆栈置空，恢复调用者的程序状态（弹出运行程序时压入的 4 字节与页号），随后返回上一层程序。
dos_program_return:
  stx data_A53F
  sty data_A540
  ldx #$FF
  txs
  lda #$22
  jsr BIOS_ENTRY
  lda data_A540
  and #$02
  beq loc_A6D9
  lda #$18
  ldx #$06
  ldy #$00
  jsr BIOS_ENTRY
  lda #$17
  jsr BIOS_ENTRY
  txa
  pha
  tya
  pha
  lda #$04
  jsr BIOS_ENTRY
  pla
  tay
  pla
  tax
  lda #$16
  jsr BIOS_ENTRY
loc_A6D9:
  lda data_A540
  lsr a
  bcc loc_A6E2
  jsr dos_initial_screen
loc_A6E2:
  ldx data_A548
  dex
  lda data_A549,x
  sta data_A53D
  ldy #$04
loc_A6EE:
  dex
  lda data_A549,x
  pha
  dey
  bne loc_A6EE
  stx data_A548
  lda data_A53F
  ldy data_A540
  clc
  rts

sub_A701:
  cmp #$00
  beq loc_A712
  cpx #$E2
  beq loc_A711
  cpx #$E3
  beq loc_A711
  cmp #$E0
  beq loc_A712
loc_A711:
  clc
loc_A712:
  rts

; DOS 功能 7 DosReadKeyNoEcho：读键盘，不回显。
; 出口：A 为字符码；0 表示功能键，下一次调用取功能键码。
dos_read_key_no_echo:
  ldx #$00
  lda data_A545
  bne loc_A72C
  jsr sub_A5C7
loc_A71D:
  jsr sub_A5DC
  beq loc_A71D
  jsr sub_A5CE
  jsr sub_A701
  bcc loc_A72F
  lda #$00
loc_A72C:
  stx data_A545
loc_A72F:
  rts

; DOS 功能 1 DosReadKeyEcho：读键盘并回显。
; 出口：A 为字符码；0 表示功能键，下一次调用取功能键码。
dos_read_key_echo:
  jsr dos_read_key_no_echo
  pha
  tax
  jsr dos_display_char
  pla
  rts

; DOS 功能 14 DosCheckKeyBoard：测试键盘是否有键。
; 出口：Z=1 无键；Z=0 有键，A 为键码（经外设调用 X=1 取得，有键时再读一次键盘）。
dos_check_keyboard:
  ldx #$01
  jsr DEVICE_ENTRY
  tya
  beq loc_A74B
  lda #$18
  ldx #$01
  jsr BIOS_ENTRY
  cpy #$00
loc_A74B:
  rts

; DOS 功能 8 DosReadKBClear：先清空键盘缓冲，再执行 $FF 里指定的键盘功能（1、6、7）。
; 入口：$FF 为要执行的 DOS 功能号。
dos_read_kb_clear:
  lda var_00FF
  pha
  lda #$00
  sta data_A545
  txa
  pha
  tya
  pha
  jsr sub_A5C7
loc_A75B:
  jsr sub_A5DC
  beq loc_A766
  jsr sub_A5CE
  jmp loc_A75B
loc_A766:
  pla
  tay
  pla
  tax
  pla
  jmp dos_call_dispatch

sub_A76E:
  pla
  pla
  rts
data_A771:
  .byte $00

sub_A772:
  ldx data_A771
  beq loc_A783
  lda #$1B
  ldy #$00
  jsr BIOS_ENTRY
  lda #$00
  sta data_A771
loc_A783:
  rts

; DOS 功能 2 DosDisplayChar：显示字符。
; 入口：X 为字符码；$0D 回车，$0A 换行，$09 按 8 列制表位补空格；中文方式下两个字节组成一个汉字。
dos_display_char:
  cpx #$09
  bne loc_A7AA
  jsr sub_A772
  lda #$1E
  jsr BIOS_ENTRY
  tya
  sta data_A543
  ora #$07
  sec
  sbc data_A543
  beq loc_A7A8
loc_A79C:
  pha
  ldx #$20
  jsr sub_A5D5
  pla
  sec
  sbc #$01
  bne loc_A79C
loc_A7A8:
  ldx #$20
loc_A7AA:
  bit data_A544
  bpl loc_A7BA
  cpx #$80
  bcs loc_A7BE
  txa
  pha
  jsr sub_A772
  pla
  tax
loc_A7BA:
  ldy #$00
  beq loc_A7C8
loc_A7BE:
  ldy data_A771
  beq loc_A7D5
  lda #$00
  sta data_A771
loc_A7C8:
  lda #$1B
  jsr BIOS_ENTRY
  lda data_A53E
  beq loc_A7D8
  jmp sub_A5DC
loc_A7D5:
  stx data_A771
loc_A7D8:
  rts
data_A7D9:
  .byte $00

; DOS 功能 3 DosInquirePages：申请内存页（16KB 一页）。
; 入口：X 为申请页数；0 表示只取当前空闲页号。
; 出口：C=0 时 A 为申请到的起始页号；C=1 时页数不够，A 为最大可申请页数。
dos_inquire_pages:
  txa
  clc
  adc data_A7D9
  bcs loc_A7EE
  cmp #$FF
  beq loc_A7EE
  ldx data_A7D9
  sta data_A7D9
  txa
  clc
  rts
loc_A7EE:
  lda #$00
  sbc data_A7D9
  sec
  rts
data_A7F5:
  .byte $00
data_A7F6:
  .byte $00, $68, $AA, $AD, $F5, $A7, $C9, $40, $D0, $16, $AD, $F6, $A7, $C9, $62, $B0
  .byte $0F, $E0, $00, $F0, $03, $38, $B0, $0F, $A9, $3C, $20, $00, $58, $18, $90, $07
  .byte $A9, $3C, $20, $00, $58, $C9, $01, $68, $AA, $A9, $01, $4C, $00, $58
data_A824:
  .byte $00
data_A825:
  .byte $F6
data_A826:
  .byte $A7

; DOS 功能 9 DosInitialScreen：初始化显示屏幕（字模、调色板、汉字提示行、光标），清屏。
; 处理分两步：先 BIOS 功能 $22 初始化显示，再链式调用 DosGetSetInitScreen 设置的向量。
dos_initial_screen:
  lda #$22
  jsr BIOS_ENTRY
  lda #$00
  sta data_A771
  sta data_A545
  lda data_A53A
  pha
  lda data_A544
  pha
  lda data_A826
  pha
  lda data_A825
  pha
  lda #$01
  ldx data_A824
  jmp BIOS_ENTRY

; DOS 功能 10 DosReleasePages：释放内存页。
; 入口：X 为要释放的起始页号，从该页起的所有页都被释放。
dos_release_pages:
  stx data_A7D9
  clc
  rts

; DOS 功能 4 DosDisplayString：显示字符串。
; 入口：X:Y 为字符串地址（X 高字节，Y 低字节，当前页），字符串以 '$' 结束；逐字符调用 dos_display_char。
dos_display_string:
  lda var_00FF
  pha
  lda var_00FE
  pha
  stx var_00FF
  sty var_00FE
  jsr sub_A772
  ldy #$00
loc_A860:
  jsr sub_A5FD
  cmp #$24
  beq loc_A877
  tax
  tya
  pha
  jsr dos_display_char
  pla
  tay
  iny
  bne loc_A860
  inc var_00FF
  jmp loc_A860
loc_A877:
  jsr sub_A772
  pla
  sta var_00FE
  pla
  sta var_00FF
  rts

sub_A881:
  bit data_A544
  bmi loc_A898
  ldx #$DB
  bne loc_A891

sub_A88A:
  bit data_A544
  bmi loc_A898

sub_A88F:
  ldx #$20
loc_A891:
  lda #$1D
  ldy #$01
  jsr BIOS_ENTRY
loc_A898:
  rts
data_A899:
  .byte $00
data_A89A:
  .byte $00
data_A89B:
  .byte $00
data_A89C:
  .byte $00
data_A89D:
  .byte $00

sub_A89E:
  ldx data_A89A
  inx
  cpx data_A899
  bcs loc_A8C6
  sta var_00FD
  ldy data_A89A
  jsr sub_A614
  inc data_A89A
  ldx var_00FD
  cpx #$20
  bcs loc_A8C2
  ldy #$5E
  jsr dos_display_char
  lda var_00FD
  ora #$40
  tax
loc_A8C2:
  jsr dos_display_char
  clc
loc_A8C6:
  rts

sub_A8C7:
  lda #$00
  sta data_A89C
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A8DC
  jsr sub_A89E
  inc data_A89B
loc_A8DC:
  rts

sub_A8DD:
  lda #$00
  sta data_A89C
  ldy data_A89A
  beq loc_A908
  dey
  sty data_A89A
  jsr sub_A5FD
  cmp #$20
  bcs loc_A8FD
  jsr sub_A88A
  ldx #$08
  jsr dos_display_char
  jsr sub_A88F
loc_A8FD:
  jsr sub_A88A
  ldx #$08
  jsr dos_display_char
  jsr sub_A88F
loc_A908:
  dec data_A89B
  bpl loc_A910
  inc data_A89B
loc_A910:
  rts

sub_A911:
  jsr sub_A881
  jsr sub_A5C7
loc_A917:
  jsr sub_A5DC
  beq loc_A917
  jsr sub_A5CE
  rts
  lda #$00
  sta data_A89C
  jsr sub_A911
  beq loc_A951
  ldx data_A89B
  cpx #$3E
  beq loc_A951
loc_A931:
  inx
  cpx #$3E
  beq loc_A951
  cmp data_A571,x
  bne loc_A931
  ldx data_A89B
loc_A93E:
  pha
  lda data_A571,x
  jsr sub_A89E
  pla
  inc data_A89B
  ldx data_A89B
  cmp data_A571,x
  bne loc_A93E
loc_A951:
  rts
  lda #$00
  sta data_A89C
loc_A957:
  jsr sub_A8C7
  bne loc_A957
  rts
  lda #$00
  sta data_A89C
  jsr sub_A911
  cmp #$0D
  beq loc_A97D
  ldx data_A89B
loc_A96C:
  cmp data_A571,x
  beq loc_A97A
  ldy data_A571,x
  inx
  cpy #$0D
  bne loc_A96C
  rts
loc_A97A:
  stx data_A89B
loc_A97D:
  rts
  inc data_A89C
  rts
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A98F
  inc data_A89B
loc_A98F:
  rts
data_A990:
  .byte $3B, $3C, $3D, $3E, $4B, $4D, $52, $53
data_A998:
  .byte $A8, $A9, $A9, $A9, $A8, $A8, $A9, $A9
data_A9A0:
  .byte $C6, $1F, $51, $5C, $DC, $C6, $7D, $81

sub_A9A8:
  lda data_A998,x
  pha
  lda data_A9A0,x
  pha
  rts

; DOS 功能 6 DosGetKBString：输入一行字符并回显，支持 F1-F4 命令行编辑。
; 入口：X:Y 为缓冲区地址；缓冲区第 0 字节是最大字符个数（含回车）。
; 出口：缓冲区第 1 字节为实际输入的字符个数（不含回车），第 2 字节起是输入内容，以回车结束。
dos_get_kb_string:
  lda var_00FF
  pha
  lda var_00FE
  pha
  lda var_00FD
  pha
  stx var_00FF
  sty var_00FE
  ldy #$00
  jsr sub_A5FD
  sta data_A899
  lda var_00FE
  clc
  adc #$02
  sta var_00FE
  bcc loc_A9D1
  inc var_00FF
loc_A9D1:
  lda #$00
  sta data_A89A
  sta data_A89B
  sta data_A89C
  lda #$1E
  jsr BIOS_ENTRY
  sty data_A89D
loc_A9E4:
  jsr sub_A911
  cmp #$08
  bne loc_A9F1
  jsr sub_A8DD
  jmp loc_A9E4
loc_A9F1:
  jsr sub_A701
  bcs loc_AA1F
  cmp #$1B
  bne loc_AA30
  ldx #$5C
  jsr dos_display_char
  jsr sub_B3D1
  lda data_A89D
  beq loc_AA12
  sta var_00FD
loc_AA09:
  ldx #$20
  jsr dos_display_char
  dec var_00FD
  bne loc_AA09
loc_AA12:
  lda #$00
  sta data_A89A
  sta data_A89B
  sta data_A89C
  beq loc_A9E4
loc_AA1F:
  txa
  ldx #$08
loc_AA22:
  dex
  bmi loc_A9E4
  cmp data_A990,x
  bne loc_AA22
  jsr sub_A9A8
  jmp loc_A9E4
loc_AA30:
  cmp #$0D
  beq loc_AA60
  cmp #$09
  bne loc_AA3A
  lda #$20
loc_AA3A:
  cmp #$03
  beq loc_AA58
  jsr sub_A89E
  bcs loc_A9E4
  lda data_A89C
  lsr a
  bcs loc_A9E4
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A9E4
  inc data_A89B
  bne loc_A9E4
loc_AA58:
  ldy #$00
  sty data_A89A
  sec
  bcs loc_AA6A
loc_AA60:
  pha
  jsr sub_A88A
  pla
  tax
  jsr dos_display_char
  clc
loc_AA6A:
  php
  lda #$0D
  sta var_00FD
  ldy data_A89A
  jsr sub_A614
  cpy #$3E
  bcc loc_AA80
  lda #$0D
  sta data_A5AE
  ldy #$3C
loc_AA80:
  jsr sub_A5FD
  sta data_A571,y
  dey
  bpl loc_AA80
  lda var_00FE
  sec
  sbc #$02
  sta var_00FE
  bcs loc_AA94
  dec var_00FF
loc_AA94:
  ldx data_A89A
  stx var_00FD
  ldy #$01
  jsr sub_A614
  plp
  pla
  sta var_00FD
  pla
  sta var_00FE
  pla
  sta var_00FF
  rts
