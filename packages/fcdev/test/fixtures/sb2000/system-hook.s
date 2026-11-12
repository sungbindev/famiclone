; ------------------------------------------------------------------------
; 名称：system-hook.s
; 说明：测试用的 NMI 钩子：每次被调用就把 hook_count 加 1。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：钩子在 NMI 里运行，以 RTS 返回；hook_count 在 system.c 里定义。
; ------------------------------------------------------------------------
        .import         _hook_count
        .export         _nmi_hook_entry

        .code

_nmi_hook_entry:
        inc     _hook_count
        rts
