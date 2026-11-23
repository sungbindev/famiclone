; 模块：boot-messages
; 引导程序用的文件名、提示与扇区标志

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_01A3"
; 段 0，$05A3-$05FF
; 根目录第一项必须有的文件名（8 加 3 格式）。
boot_system_file_name:
  .byte $42, $42, $47, $44, $4F, $53, $20, $20, $53, $59, $53

; 加载失败时显示的提示：“Non-System disk or disk error，Replace and press RESET again”。
boot_error_text:
  .byte $0A, "Non-System disk or disk error", $0D, $0A
  .byte "Replace and press RESET again", $00
  .res 18, $FF
; 引导扇区结束标志 $55 $AA
boot_signature:
  .byte $55, $AA
