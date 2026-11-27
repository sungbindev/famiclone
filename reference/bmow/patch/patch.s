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

; ---- T_clampA / T_clampB：NIB 读头位置限幅（四分之一磁道单位）----
; 原固件 NIB 路径把读头钳在 140（35 道×4），超出写 139；WOZ 等类型走 160/159（40 道）。
; 金字塔 40 道盘需要读到 35–39 道：模式 9 改走 160/159 限幅（复用原 WOZ 限幅入口，
; 它们自己再查 <160 则继续、否则写 159）；其余模式原 140/139 逻辑不变。
; 下溢（位置绕回 ≥251）归零的回零校准在别处，不动。

; 补丁点 0x8C62（主循环路径，原 ld r18,Z ; cpi r18,0x8C）
T_clampA:
    ld    r18, Z
    push  r24
    lds   r24, 0x33FE
    cpi   r24, 9
    pop   r24
    breq  T_clampA9
    cpi   r18, 0x8C         ; 原判定：≥140？
    brcc  T_clampA_ge
    jmp   0x8B76            ; <140 → 原继续路径
T_clampA_ge:
    jmp   0x8C6A            ; ≥140 → 原类型分发（WOZ/17/18→160 限幅，否则写 139）
T_clampA9:
    jmp   0x937E            ; 模式 9：160/159 限幅

; 补丁点 0x47C8（定时器中断路径，原 ld r24,Z ; cpi r24,0x8C）
T_clampB:
    ld    r24, Z
    push  r18
    lds   r18, 0x33FE
    cpi   r18, 9
    pop   r18
    breq  T_clampB9
    cpi   r24, 0x8C
    brcc  T_clampB_ge
    jmp   0x4612            ; <140 → 原继续路径
T_clampB_ge:
    jmp   0x47D0            ; ≥140 → 原类型分发
T_clampB9:
    jmp   0x4820            ; 模式 9：160/159 限幅

; ================= 菜单扩第 7 项（模式 9）=================
; 第 7 项名称与字库放高 flash（RAMPZ=1），用克隆的远地址绘制器显示。
; 名称 "9 Pyramid 40-track" 在 0x13400，绘制用 +1 处 0x13401（低 16 位 0x3401）。
; 克隆绘制器在 0x13900（由 build.mjs 从 0xE902 变换生成）。
; 菜单项数计数器从 RAM 0x3D9 迁到 0x3B06（0x3D9 让给第 7 项的数组槽）。
; 槽位新基址：槽(行) = 0x3D2+行（行 1–7 → 0x3D3–0x3D9）。原循环存行 1、2（0x3114）与
; PREV（0x31C8）/NEXT（0x31FC）读槽的 subi 立即数在 build.mjs 里同步改基址，
; 否则行 2 槽被插入项覆盖、按键读错槽（NEXT 跳项、PREV 读到 0x3DA 死机）。
    .equ ITEM7_NAME, 0x3401
    .equ CLONE,      0x13900
    .equ COUNT_RAM,  0x3B06

; ---- T_menu：菜单构建循环（补丁点 0x3142，原 cp/cpc/brne）----
; 把模式 9 插到菜单第 3 位：原循环绘前 2 项（行 1、2，表项 0、1）后
; r29==3，此时插入模式 9（行 3），再把表项 2..5 绘于行 4..7，共 7 项。
; 循环变量只用绘制函数保留的寄存器：r16:r17 表指针、r14:r15 表尾、
; r29 行号、r28 模式号（r18–r27、r30:r31 会被绘制函数破坏，不能跨 call 存值）。
T_menu:
    cpi   r29, 3
    brne  T_menu_norm       ; 未到插入点 → 走原表尾判断
    rjmp  T_menu_ins        ; 到插入点（短跳板）
T_menu_norm:
    cp    r14, r16          ; 原表尾判断
    cpc   r15, r17
    brne  T_menu_back
    jmp   0x3148            ; 表扫完（未插入的兑底）→ 原收尾
T_menu_back:
    jmp   0x30FA            ; 回原循环
T_menu_ins:
    ; 表项 0、1 已绘于行 1、2。插入模式 9 于行 3（数组下标 3）。
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
    ; 表项 2..5（0x61B 起，步长 23）绘于行 4 起。
    ; 此刻 r16:r17 正指向表项 2（0x61B），r14:r15 是表尾 0x677。
    ; 与原循环一致：模式 7 需 0x3413!=0、模式 8 需 0x3413>=2，否则跳过不占行。
    ldi   r29, 4            ; 行 4
T_menu_sl:
    movw  r30, r16
    lpm   r28, Z            ; 表项首字符 = 模式号 ASCII
    subi  r28, 0x30
    cpi   r28, 7
    breq  T_menu_g7
    cpi   r28, 8
    breq  T_menu_g8
T_menu_draw:
    mov   r30, r29
    ldi   r31, 0
    subi  r30, 0x2E         ; Z = 0x3D2 + 行
    sbci  r31, 0xFC
    st    Z, r28            ; array[行] = 模式号
    mov   r22, r29
    ldi   r24, 0
    call  0xE4C2            ; 光标行 r29 列 0
    ldi   r22, 1
    lds   r24, 0x33FE
    cp    r24, r28
    breq  T_menu_shsel
    ldi   r22, 0
T_menu_shsel:
    movw  r24, r16          ; 名称地址 = 表项 + 1
    adiw  r24, 1
    call  0xE902            ; 原绘制器（低 flash）
    lds   r24, 0x33FE
    cp    r24, r28
    brne  T_menu_rowinc
    sts   0x3D2, r29        ; 当前模式 → 选中下标 = 行
T_menu_rowinc:
    subi  r29, 0xFF         ; 行++（绘制了才占行）
T_menu_shnext:
    subi  r16, 0xE9         ; 表指针 +23
    sbci  r17, 0xFF
    cp    r16, r14
    cpc   r17, r15
    brne  T_menu_sl         ; 未到表尾 → 下一项
    subi  r29, 0x01         ; 项数 = 实际绘制的最后一行
    sts   COUNT_RAM, r29
    jmp   0x314E            ; 收尾 pop/ret
T_menu_g7:
    lds   r24, 0x3413
    and   r24, r24
    brne  T_menu_draw
    rjmp  T_menu_shnext     ; 硬件缺失 → 跳过
T_menu_g8:
    lds   r24, 0x3413
    cpi   r24, 2
    brcc  T_menu_draw
    rjmp  T_menu_shnext     ; 硬件缺失 → 跳过


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
