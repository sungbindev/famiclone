; ------------------------------------------------------------------------
; 名称：layout.s
; 说明：GBASIC 映像的固定布局：DOS 现场交换缓冲、保留区与向量表。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：与 image.cfg、stub.s 的 BUFFER_HIGH 一致：缓冲 $F700-$FEFF（2K），保留 $FF00-$FFF9，向量 $FFFA-$FFFF。
;       NMI 向量指向零页 $00ED：原版在 $ED 放一条 jmp 指向 NMI 例程（L80AD 写入），这里保持不变。
; ------------------------------------------------------------------------
        .import         image_entry, irq_handler

        .segment        "BUFFER"
        .res    $0800

        .segment        "TAIL"
        .res    $00FA

        .segment        "VECTORS"
        .word   $00ED, image_entry, irq_handler
