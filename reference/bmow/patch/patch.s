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

; ---- T_menu：菜单构建循环出口（补丁点 0x3142，原 cp/cpc/brne）----
; 6 项表扫完后追加第 7 项（模式 9），再走收尾。r29 = 已绘项数+1。
T_menu:
    cp    r14, r16
    cpc   r15, r17
    brne  T_menu_loop       ; 未到表尾 → 回循环
    ; 数组[r29] = 9（Z = 0x3D2 + r29）
    mov   r30, r29
    ldi   r31, 0
    subi  r30, 0x2D
    sbci  r31, 0xFC
    ldi   r28, 9
    st    Z, r28
    ; 光标行 r29 列 0
    mov   r22, r29
    ldi   r24, 0
    call  0xE4C2
    ; 选中标志
    ldi   r22, 1
    lds   r24, 0x33FE
    cpi   r24, 9
    breq  T_menu_sel
    ldi   r22, 0
T_menu_sel:
    ldi   r24, lo8(ITEM7_NAME)
    ldi   r25, hi8(ITEM7_NAME)
    call  draw_far
    ; 当前模式==9 → 记下选中下标
    lds   r24, 0x33FE
    cpi   r24, 9
    brne  T_menu_cnt
    sts   0x3D2, r29
T_menu_cnt:
    sts   COUNT_RAM, r29    ; 项数 = r29（追加一项后未自增）
    jmp   0x314E            ; 收尾 pop/ret
T_menu_loop:
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