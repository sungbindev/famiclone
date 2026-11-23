; 模块：dos-variables
; DOS 的全局变量区（驱动器状态、磁盘参数、当前盘号等）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0400"
; 段 0，$8000-$809D
data_8000:
  .byte $00
data_8001:
  .byte $00
data_8002:
  .byte $00
data_8003:
  .byte $03
data_8004:
  .byte $00
data_8005:
  .byte $00
data_8006:
  .byte $03
data_8007:
  .byte $FE
data_8008:
  .byte $FE
data_8009:
  .byte $00
data_800A:
  .byte $00
data_800B:
  .byte $00
data_800C:
  .byte $00
data_800D:
  .byte $00
data_800E:
  .byte $00
data_800F:
  .byte $00
data_8010:
  .byte $00
data_8011:
  .byte $00
data_8012:
  .byte $00
data_8013:
  .byte $00
data_8014:
  .byte $00
data_8015:
  .byte $00
data_8016:
  .byte $00
data_8017:
  .byte $00
data_8018:
  .byte $00
data_8019:
  .byte $00
data_801A:
  .byte $00
data_801B:
  .byte $00
data_801C:
  .byte $00
data_801D:
  .byte $00
data_801E:
  .byte $00
data_801F:
  .byte $00
data_8020:
  .byte $00
data_8021:
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
data_802B:
  .byte $00
data_802C:
  .byte $00
data_802D:
  .byte $00
data_802E:
  .byte $FF
data_802F:
  .byte $00
data_8030:
  .byte $00
data_8031:
  .byte $00
data_8032:
  .res 62, $00
data_8070:
  .byte $00
data_8071:
  .byte $00
data_8072:
  .byte $00
data_8073:
  .byte $00
data_8074:
  .byte $00
data_8075:
  .byte $00
data_8076:
  .byte $00
data_8077:
  .byte $00
data_8078:
  .byte $00
data_8079:
  .byte $00
data_807A:
  .byte $00
data_807B:
  .byte $00
data_807C:
  .byte $00
data_807D:
  .byte $00
data_807E:
  .byte $00
data_807F:
  .byte $00
data_8080:
  .byte $00
data_8081:
  .byte $00
data_8082:
  .res 27, $00
data_809D:
  .byte $00
