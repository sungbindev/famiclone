; Floppy Emu 固件补丁：新增顶层模拟模式 9（金字塔 PEC-9588 40 磁道 5.25 寸 NIB）
; 目标固件 femu.bin（250225A），AVR ATmega1284，flash 字节地址 = 文件偏移。
; 本文件只含「功能核心」蹦床，放在空闲 flash 0x12F00 起；原函数补丁点用 jmp 跳入。
; 复用现有类型 19（NIB）流水线，不改几何表、不改模式名表、不改图形数据、不改 RAM 布局。
;
; 模式 9 行为等同模式 4（Apple II 5.25），唯一差别：NIB 大小识别认 266240（40×6656）。
; 其余模式行为完全不变。

    .text
    .global patch_start

patch_start:

; ---- T_disp：模式名显示分发（补丁点 0x283A，原 cpi r24,4 ; brne 0x2840）----
; 模式 4 或 9 → 0x2C20（显示 "Apple II 5.25 Floppy"）；其余 → 0x2840 继续原分发。
T_disp:
    cpi   r24, 4
    breq  T_disp4
    cpi   r24, 9
    breq  T_disp4
    jmp   0x2840
T_disp4:
    jmp   0x2C20

; ---- T_emu：emumode.txt 数字范围（补丁点 0x3342，原 cpi r24,8 ; brcs 0x3352）----
; 数字 9 → 接受；3..7 → 接受；其余 → 0x3346 走双驱校验（8 需 0x3413>=2）。
T_emu:
    cpi   r24, 9
    breq  T_emu_acc
    cpi   r24, 8
    brcs  T_emu_acc
    jmp   0x3346
T_emu_acc:
    jmp   0x3352

; ---- T_compat：模式兼容检查（补丁点 0x5C34，原 cpi r24,7 ; breq 0x5C5C）----
; 模式 9 → 0x5C4E（类型 15..24 合法，同模式 4）；模式 7 → 0x5C5C；其余 → 0x5C38。
T_compat:
    cpi   r24, 9
    breq  T_compat_ok
    cpi   r24, 7
    breq  T_compat7
    jmp   0x5C38
T_compat_ok:
    jmp   0x5C4E
T_compat7:
    jmp   0x5C5C

; ---- T_init：磁盘初始化 5.25 标志（补丁点 0x6414，原 cpi r25,7 ; breq 0x641A）----
; 模式 7 或 9 → 0x641A（置 0x381C=1，r24 已在 0x640A 置 1）；其余 → 0x6418（置 0）。
T_init:
    cpi   r25, 7
    breq  T_init_set
    cpi   r25, 9
    breq  T_init_set
    jmp   0x6418
T_init_set:
    jmp   0x641A

; ---- T_size：NIB 大小识别（补丁点 0xC912，原 cp r4,r1 ; ldi r18,0x8E）----
; 文件大小在 r4:r7。模式 9 认 266240(0x00041000)；其余模式认 232960(0x00038E00)。
; 命中 → 0xCA62（类型 19 NIB）；未命中 → 0xC922 继续后续类型判定。
T_size:
    lds   r18, 0x33FE
    cpi   r18, 9
    breq  T_size9
    cp    r4, r1
    ldi   r18, 0x8E
    cpc   r5, r18
    ldi   r18, 0x03
    cpc   r6, r18
    cpc   r7, r1
    breq  T_size_nib
    jmp   0xC922
T_size9:
    cp    r4, r1
    ldi   r18, 0x10
    cpc   r5, r18
    ldi   r18, 0x04
    cpc   r6, r18
    cpc   r7, r1
    breq  T_size_nib
    jmp   0xC922
T_size_nib:
    jmp   0xCA62

; ================= 菜单扩第 7 项（模式 9）=================
; 第 7 项名称与字库放高 flash（RAMPZ=1），用克隆的远地址绘制器显示。
; 名称 "9 Pyramid 40-track" 在 0x13400，绘制用 +1 处 0x13401（低 16 位 0x3401）。
; 克隆绘制器在 0x13900（由 build.mjs 从 0xE902 变换生成）。
; 菜单项数计数器从 RAM 0x3D9 迁到 0x3B06（0x3D9 让给第 7 项的数组槽）。
    .equ ITEM7_NAME, 0x3401
    .equ CLONE,      0x13900
    .equ COUNT_RAM,  0x3B06

; ---- T_menu：菜单构建循环（补丁点 0x3142，原 cp/cpc/brne）----
; 把模式 9 插到菜单第 3 位（下标 3，行 3）：原循环绘前 2 项（下标 1、2，
; 即表项 0、1），到 r29==3 时先绘模式 9（下标 3），再把原表项 2..5 绘到
; 下标 4..7。共 7 项。r29 = 当前显示行（1 起）。
T_menu:
    cp    r14, r16
    cpc   r15, r17
    brne  T_menu_loop       ; 未到表尾 → 回循环
    ; 表项 0、1 已绘于下标 1、2（r29==3）。插入模式 9 于下标 3。
    ldi   r28, 9
    sts   0x3D5, r28        ; array[3] = 9
    ldi   r22, 3
    ldi   r24, 0
    call  0xE4C2            ; 光标行 3 列 0
    ldi   r22, 1
    lds   r24, 0x33FE
    cpi   r24, 9
    breq  T_menu_sel9
    ldi   r22, 0
T_menu_sel9:
    ldi   r24, lo8(ITEM7_NAME)
    ldi   r25, hi8(ITEM7_NAME)
    call  draw_far
    lds   r24, 0x33FE
    cpi   r24, 9
    brne  T_menu_shift
    ldi   r24, 3
    sts   0x3D2, r24        ; 当前模式 9 → 选中下标 3
T_menu_shift:
    ; 原表项 2..5（名表 0x61B 起，步长 23）绘于下标 4..7
    ldi   r30, 0x1B         ; Z = 0x61B
    ldi   r31, 0x06
    ldi   r20, 4            ; 行 4
T_menu_sl:
    mov   r22, r20
    ldi   r24, 0
    call  0xE4C2            ; 光标行 r20 列 0
    lpm   r28, Z            ; 表项首字符 = 模式号 ASCII
    subi  r28, 0x30
    mov   r24, r20
    ldi   r25, 0
    subi  r24, 0x2E         ; Z = 0x3D2 + 行
    sbci  r25, 0xFC
    st    Z, r28            ; array[行] = 模式号
    ldi   r22, 1
    lds   r24, 0x33FE
    cp    r24, r28
    breq  T_menu_shsel
    ldi   r22, 0
T_menu_shsel:
    movw  r24, r30          ; 名称地址 = 表项
    adiw  r24, 1
    call  0xE902            ; 原绘制器（低 flash）
    lds   r24, 0x33FE
    cp    r24, r28
    brne  T_menu_shnext
    sts   0x3D2, r20        ; 当前模式 → 选中下标 = 行
T_menu_shnext:
    adiw  r30, 0x17         ; 下一表项（+23）
    subi  r20, 0xFF         ; 行++
    cpi   r20, 8
    brne  T_menu_sl         ; 行 4..7
    ldi   r24, 7
    sts   COUNT_RAM, r24    ; 项数 = 7
    jmp   0x314E            ; 收尾 pop/ret
T_menu_loop:
    cpi   r29, 3
    brne  T_menu_orig       ; 未绘完前 2 项 → 回原循环
    rjmp  T_menu            ; 绘完前 2 项 → 插入模式 9（短跳板）
T_menu_orig:
    jmp   0x30FA

; ---- T_lookup：选模式后状态栏名称查找循环尾（补丁点 0x32B4）----
; 原循环扫 6 项首字符匹配模式号；扫完未中且模式==9 → 远地址绘名称。
T_lookup:
    subi  r18, 0xFF
    sbci  r19, 0xFF
    adiw  r30, 0x17
    cpi   r18, 6
    cpc   r19, r1
    brne  T_lookup_cont     ; 不足 6 项 → 回循环
    lds   r24, 0x33FE
    cpi   r24, 9
    brne  T_lookup_miss
    ldi   r22, 0
    ldi   r24, lo8(ITEM7_NAME)
    ldi   r25, hi8(ITEM7_NAME)
    call  draw_far
    jmp   0x32C0
T_lookup_cont:
    jmp   0x32A6
T_lookup_miss:
    jmp   0x32C0

; ---- draw_far：远地址字符串绘制包装 ----
; 入参 r24:r25 = 高 flash 地址低 16 位（RAMPZ=1 区），r22 = 选中标志。
; 保存并恢复 r16 与 RAMPZ；克隆绘制器自身保存 r13-r17/r28/r29。
draw_far:
    push  r16
    in    r16, 0x3B         ; 存 RAMPZ
    push  r16
    ldi   r16, 1
    out   0x3B, r16         ; RAMPZ=1
    call  CLONE
    pop   r16
    out   0x3B, r16         ; 还原 RAMPZ
    pop   r16
    ret

patch_end: