; ------------------------------------------------------------------------
; 名称：system-entries.inc
; 说明：步步高系统调用入口向量，位于 $5800-$5818 的公共 RAM，每个入口是一条 JMP。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：地址与用途取自使用手册系统篇第一章，并经 BBGDOS 启动后在 Ember 里读内存核对（$5800 起为 JMP 序列）。
; ------------------------------------------------------------------------

BIOS_ENTRY              = $5800 ; BIOS 调用：A 为功能号
FILE_ENTRY              = $5803 ; 磁盘文件调用：X 为通道块地址高字节，Y 为低字节，A 不用设置
COLD_START_ENTRY        = $5806 ; 系统冷启动，按复位键时执行
DEVICE_ENTRY            = $5809 ; 外设调用
DOS_ENTRY               = $580C ; DOS 调用：A 为功能号
WARM_START_ENTRY        = $580F ; 系统热启动，按 Ctrl+Alt+Del 时执行
CTRL_BREAK_ENTRY        = $5812 ; Ctrl+Break 处理，键盘硬中断里调用，应用可改写其中的 JMP 目标
CTRL_C_ENTRY            = $5815 ; Ctrl+C 处理，DOS 输出字符时检测到后调用，应用可改写其中的 JMP 目标
CRITICAL_ERROR_ENTRY    = $5818 ; 严重错误处理，DOS 遇到磁盘错误时调用
