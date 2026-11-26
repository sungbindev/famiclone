; ------------------------------------------------------------------------
; 名称：cec1.inc
; 说明：中华学习机 CEC-I 的 DOS 3.3 程序用的符号定义：监控程序入口、软开关、DOS 3.3 的入口，以及汇编运行库
;       cec1.lib 的函数。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-26
; 备注：监控程序入口与 Apple II 相同，已核对 1.21 版 ROM 里这些地址的内容；DOS 3.3 的入口是页 3 里的
;       跳转向量，在 DOS 3.3 系统盘启动之后有效。调用库函数：参数放 A（字符、字节）或 A（低）、X（高）
;       （地址），JSR 调用；库函数不保留 A、X、Y，也不保留标志。
; ------------------------------------------------------------------------

;---------------------------------------; 监控程序入口（都在 ROM 里，JSR 调用）
MON_HOME                = $FC58 ; 清屏并把光标移到左上角
MON_WAIT                = $FCA8 ; 延时：A 越大越久，约 (26 + 27 * A + 5 * A * A) 个 CPU 周期
MON_RDKEY               = $FD0C ; 等待按键：A 返回按键码，位 7 为 1
MON_CROUT               = $FD8E ; 输出回车
MON_PRBYTE              = $FDDA ; 把 A 按两位十六进制输出
MON_PRHEX               = $FDE3 ; 把 A 的低 4 位按一位十六进制输出
MON_COUT                = $FDED ; 输出字符：A 的位 7 为 1 时按正常方式显示
MON_BELL                = $FF3A ; 响铃

;---------------------------------------; 软开关（读写这些地址产生作用，不用关心读到的值）
KBD                     = $C000 ; 读：键盘，位 7 为 1 表示有按键，低 7 位是按键码
KBDSTRB                 = $C010 ; 访问一次清除键盘的按键标志
SPKR                    = $C030 ; 访问一次让扬声器翻转一次
TXTCLR                  = $C050 ; 显示图形
TXTSET                  = $C051 ; 显示文本
MIXCLR                  = $C052 ; 全屏图形
MIXSET                  = $C053 ; 图形下面 4 行文本
LOWSCR                  = $C054 ; 第 1 页
HISCR                   = $C055 ; 第 2 页
LORES                   = $C056 ; 低分辨率
HIRES                   = $C057 ; 高分辨率

;---------------------------------------; DOS 3.3 的入口（页 3 里的跳转向量）
DOS_WARM_START          = $03D0 ; DOS 热启动：回到 BASIC，保留内存里的 BASIC 程序
DOS_COLD_START          = $03D3 ; DOS 冷启动
DOS_FILE_MANAGER        = $03D6 ; 文件管理器：A（高）、Y（低）为参数表地址
DOS_RWTS                = $03D9 ; 读写磁道扇区：A（高）、Y（低）为 IOB 地址
DOS_FM_PARAMETERS       = $03DC ; 取文件管理器参数表地址：Y（低）、A（高）返回
DOS_RWTS_IOB            = $03E3 ; 取 RWTS 的 IOB 地址：Y（低）、A（高）返回

;---------------------------------------; 运行库 cec1.lib 的零页单元（库内部使用，调用者不要使用）
        .globalzp       cec1i_ptr

;---------------------------------------; 运行库函数：text
        .global         cec1_get_key
        .global         cec1_home
        .global         cec1_newline
        .global         cec1_poll_key
        .global         cec1_put_char
        .global         cec1_put_cstr
        .global         cec1_put_hex8
