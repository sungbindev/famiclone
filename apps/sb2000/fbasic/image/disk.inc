; ------------------------------------------------------------------------
; 名称：disk.inc
; 说明：FBASIC 移植版的磁盘存取：SAVE、LOAD 的文件名固定加 .BS 扩展名，程序存成 SB-DOS 磁盘文件；替代原版的磁带例程。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-17
; 备注：被 fbasic.s 包含，和解释器在同一个翻译单元里。文件内容是原版磁带记录的原样：22 字节的头（$0500-$0515：类型 $02、16 字符文件名、$00、长度、起始地址）加程序正文。
;       文件名取第一个 “.” 或空格之前的部分（最多 10 个字符，含 A: 之类的盘符），再加 “.BS”，所以 SAVE ABC、SAVE ABC.BS 存的是同一个文件。
;       SB-DOS 的文件服务只能整个文件读写，数据要放在扩展 RAM 页里：存时先在 DOS 分配 1 页，临时把它映射到窗口 6（映像的 $E000-$EFFF 没有代码）复制进去；
;       读时整个文件读进这一页，头复制到 $0500，正文用 DMA 搬进程序区。每次 DOS 调用经入口门，请求块一次最多写 8 个单元，所以路径分段写入。
;       背景图（BG）存成一个文件：类型 $03 的头（$0516 是屏幕 1 的滚动值）加 1024 字节的 NES 格式背景图（960 个图块号、64 个属性字节），直接在暂存页里与名称表互相转换。
;       出错一律显示原版的 “TP ERROR”（出错码 $11）。
; ------------------------------------------------------------------------

DISK_HEADER_SIZE        = 22
DISK_MAX_NAME           = 10
DISK_STAGE_BASE         = $E000                 ; 暂存页映射到窗口 6 后的地址
DISK_PATH               = $0700                 ; 整理好的路径（NMI 的文字缓冲，这时没有命令在用）

; 请求块助手：A = 功能号，X = 子功能号；清写读表，并把 DOS_ERROR 作为第一个要读的单元（结果区第 0 项）
req_begin:
        sta     SHELL_REQ_FUNCTION
        stx     SHELL_REQ_SUB
        lda     #$00
        sta     SHELL_REQ_WRITE_COUNT
        sta     SHELL_REQ_READ_COUNT
        ldx     #<DOS_ERROR
        ldy     #>DOS_ERROR
; 加一个要读的单元：X = 地址低，Y = 地址高
req_read:
        stx     V_TMP2
        ldx     SHELL_REQ_READ_COUNT
        lda     V_TMP2
        sta     SHELL_REQ_READ_LO,x
        tya
        sta     SHELL_REQ_READ_HI,x
        inc     SHELL_REQ_READ_COUNT
        rts
; 加一个要写的单元：A = 值，X = 地址低，Y = 地址高
req_write:
        sta     V_TMP1
        stx     V_TMP2
        ldx     SHELL_REQ_WRITE_COUNT
        lda     V_TMP1
        sta     SHELL_REQ_WRITE_VALUE,x
        lda     V_TMP2
        sta     SHELL_REQ_WRITE_LO,x
        tya
        sta     SHELL_REQ_WRITE_HI,x
        inc     SHELL_REQ_WRITE_COUNT
        rts

; 调用已填好的请求：返回 C=1 表示 DOS_ERROR 不为 0；DOS 的中断和显示会改掉画面与键盘状态，返回后还原
dos_gate:
        jsr     SHELL_DOS_CALL
        lda     #$00
        sta     KB_KEY
        sta     KB_EXT
        sta     KB_PREFIX
        sta     KB_MODS
        jsr     LB53C
        lda     $33
        sta     $2001
        lda     SHELL_REQ_RESULT
        cmp     #$01
        rts

; 把 DISK_PATH 里的路径写进 DOS 的 PATH_BUFFER，8 个字节一段（用什么也不做的功能号 $FF 带出）
dos_path:
        ldy     #$00
@chunk: lda     #$FF
        sta     SHELL_REQ_FUNCTION
        lda     #$00
        sta     SHELL_REQ_SUB
        sta     SHELL_REQ_READ_COUNT
        ldx     #$00
@fill:  tya
        clc
        adc     #<PATH_BUFFER
        sta     SHELL_REQ_WRITE_LO,x
        lda     #>PATH_BUFFER
        sta     SHELL_REQ_WRITE_HI,x
        lda     DISK_PATH,y
        sta     SHELL_REQ_WRITE_VALUE,x
        beq     @last
        iny
        inx
        cpx     #$08
        bne     @fill
        stx     SHELL_REQ_WRITE_COUNT
        jsr     dos_gate
        jmp     @chunk
@last:  inx
        stx     SHELL_REQ_WRITE_COUNT
        jmp     dos_gate

; 文件名（$0580，$EA 为 1 才有）整理成 DISK_PATH 里的“名字.BS”：C=1 表示没有名字或名字太长
disk_make_path:
        lda     $EA
        beq     @bad
        ldx     #$00
@copy:  cpx     #$10
        bcs     @end
        lda     $0580,x
        beq     @end
        cmp     #$20
        beq     @end
        cmp     #$2E
        beq     @end
        sta     DISK_PATH,x
        inx
        bne     @copy
@end:   cpx     #$00
        beq     @bad
        cpx     #DISK_MAX_NAME + 1
        bcs     @bad
        lda     #$2E
        sta     DISK_PATH,x
        lda     #$42
        sta     DISK_PATH + 1,x
        lda     #$53
        sta     DISK_PATH + 2,x
        lda     #$00
        sta     DISK_PATH + 3,x
        clc
        rts
@bad:   sec
        rts

; 按 A 指定的方式打开 DISK_PATH 里的文件：C=1 表示出错
disk_open:
        pha
        jsr     dos_path
        lda     #FN_FILE
        ldx     #SUB_FS_OPEN
        jsr     req_begin
        pla
        ldx     #<FILE_MODE
        ldy     #>FILE_MODE
        jsr     req_write
        jmp     dos_gate

; 关闭文件并让软驱停转，不看出错
disk_close:
        lda     #FN_FILE_HANDLE
        ldx     #SUB_FH_CLOSE_FLUSH
        jsr     req_begin
        jmp     dos_gate

; 分配 1 页暂存页，页号放进 SHELL_DISK_PAGE：C=1 表示出错
disk_alloc:
        lda     #FN_PAGE
        ldx     #SUB_PAGE_ALLOC
        jsr     req_begin
        lda     #$01
        ldx     #<ALLOC_COUNT
        ldy     #>ALLOC_COUNT
        jsr     req_write
        ldx     #<ALLOC_START
        ldy     #>ALLOC_START
        jsr     req_read
        jsr     dos_gate
        bcs     @done
        lda     SHELL_REQ_RESULT + 1
        sta     SHELL_DISK_PAGE
        clc
@done:  rts

; 释放暂存页（没有就什么也不做）
disk_free:
        lda     SHELL_DISK_PAGE
        beq     @none
        lda     #FN_PAGE
        ldx     #SUB_PAGE_FREE
        jsr     req_begin
        lda     SHELL_DISK_PAGE
        ldx     #<ALLOC_START
        ldy     #>ALLOC_START
        jsr     req_write
        lda     #$01
        ldx     #<ALLOC_COUNT
        ldy     #>ALLOC_COUNT
        jsr     req_write
        jsr     dos_gate
        lda     #$00
        sta     SHELL_DISK_PAGE
@none:  rts

; 出错：释放暂存页，报“TP ERROR”
disk_error:
        jsr     disk_free
        lda     #$11
        jmp     LB237

; 暂存页映射进窗口 6，用完还原
disk_map_stage:
        lda     SHELL_DISK_PAGE
        sta     HW_WINDOW_0 + 6
        rts
disk_unmap_stage:
        lda     SHELL_IMAGE_WINDOW + 6
        sta     HW_WINDOW_0 + 6
        rts

; 复制 V_COPY_LEN 个字节：从 V_COPY_SRC 到 V_COPY_DST
disk_copy:
        ldy     #$00
@loop:  lda     V_COPY_LEN
        ora     V_COPY_LEN + 1
        beq     @done
        lda     (V_COPY_SRC),y
        sta     (V_COPY_DST),y
        inc     V_COPY_SRC
        bne     @src
        inc     V_COPY_SRC + 1
@src:   inc     V_COPY_DST
        bne     @dst
        inc     V_COPY_DST + 1
@dst:   lda     V_COPY_LEN
        bne     @len
        dec     V_COPY_LEN + 1
@len:   dec     V_COPY_LEN
        jmp     @loop
@done:  rts

; 取 $0514/$0515 与 $0512/$0513 作源起点与长度，目的是暂存页里头之后的位置
disk_set_data_pointers:
        lda     $0514
        sta     V_COPY_SRC
        lda     $0515
        sta     V_COPY_SRC + 1
        lda     #<(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     V_COPY_DST
        lda     #>(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     V_COPY_DST + 1
        lda     $0512
        sta     V_COPY_LEN
        lda     $0513
        sta     V_COPY_LEN + 1
        rts

; ---- SAVE：LBF1F 写头什么也不做，LBF30 把头和正文整个写成文件 ----
LBF1F:  rts

LBF30:  lda     $0500
        cmp     #$03
        beq     @fail                      ; 背景图不走这里
        lda     $0513
        cmp     #$0F
        bcs     @fail                      ; 程序区不会这么大，防备头里的长度不对
        jsr     disk_make_path
        bcs     @fail
        jsr     disk_alloc
        bcs     @fail
        jsr     disk_map_stage
        ldx     #DISK_HEADER_SIZE - 1
@head:  lda     $0500,x
        sta     DISK_STAGE_BASE,x
        dex
        bpl     @head
        jsr     disk_set_data_pointers
        jsr     disk_copy
        jsr     disk_unmap_stage
        jmp     disk_write_stage
@fail:  jmp     disk_error

; 把已填好头与正文的暂存页写成文件，长度是 $0512/$0513 加头
disk_write_stage:
        clc
        lda     $0512
        adc     #DISK_HEADER_SIZE
        sta     SHELL_DISK_SIZE
        lda     $0513
        adc     #$00
        sta     SHELL_DISK_SIZE + 1
        lda     #MODE_CREATE
        jsr     disk_open
        bcs     @fail
        lda     #FN_FILE_HANDLE
        ldx     #SUB_FH_WRITE_FILE
        jsr     req_begin
        lda     SHELL_DISK_SIZE
        ldx     #<FILE_SIZE_0
        ldy     #>FILE_SIZE_0
        jsr     req_write
        lda     SHELL_DISK_SIZE + 1
        ldx     #<FILE_SIZE_1
        ldy     #>FILE_SIZE_1
        jsr     req_write
        lda     #$00
        ldx     #<FILE_SIZE_2
        ldy     #>FILE_SIZE_2
        jsr     req_write
        lda     SHELL_DISK_PAGE
        ldx     #<FILE_START_PAGE
        ldy     #>FILE_START_PAGE
        jsr     req_write
        jsr     dos_gate
        php
        jsr     disk_close
        jsr     disk_free
        plp
        bcs     @fail
        rts
@fail:  jmp     disk_error

; BG 存盘：头已在 $0500，名字在 $0580；名称表 1 转成 NES 格式直接放进暂存页，再整个写成文件
bg_save_file:
        jsr     disk_make_path
        bcs     @fail
        jsr     disk_alloc
        bcs     @fail
        jsr     disk_map_stage
        ldx     #DISK_HEADER_SIZE - 1
@head:  lda     $0500,x
        sta     DISK_STAGE_BASE,x
        dex
        bpl     @head
        lda     #<(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     $19
        lda     #>(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     $1A
        jsr     LB559
        jsr     bg_read_page
        jsr     LB53C
        jsr     LB562
        jsr     disk_unmap_stage
        jmp     disk_write_stage
@fail:  jmp     disk_error

; BG 读盘：整个文件已在暂存页里（LBE53 读好），按 NES 格式写进名称表 1，要求长度正好是 1024
bg_load_file:
        lda     $0512
        bne     @fail
        lda     $0513
        cmp     #$04
        bne     @fail
        jsr     disk_map_stage
        lda     #<(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     $19
        lda     #>(DISK_STAGE_BASE + DISK_HEADER_SIZE)
        sta     $1A
        jsr     LB559
        jsr     bg_write_page
        jsr     LB53C
        jsr     LB562
        jsr     disk_unmap_stage
        jsr     disk_free
        clc
        rts
@fail:  jmp     disk_error

; ---- LOAD：LBE53 把整个文件读进暂存页并取出头，LBE24 把正文搬进程序区，LBE1C 把正文与程序区比较 ----
LBE53:  jsr     disk_make_path
        bcc     @path_ok
@early: jmp     disk_error
@path_ok:
        lda     #MODE_OPEN
        jsr     disk_open
        bcs     @early
        lda     #FN_FILE_HANDLE
        ldx     #SUB_FH_GET_SIZE
        jsr     req_begin
        ldx     #<FILE_SIZE_0
        ldy     #>FILE_SIZE_0
        jsr     req_read
        ldx     #<FILE_SIZE_1
        ldy     #>FILE_SIZE_1
        jsr     req_read
        ldx     #<FILE_SIZE_2
        ldy     #>FILE_SIZE_2
        jsr     req_read
        jsr     dos_gate
        bcs     @bad_close
        lda     SHELL_REQ_RESULT + 3
        bne     @bad_close                      ; 大于 64K
        lda     SHELL_REQ_RESULT + 2
        cmp     #$10
        bcs     @bad_close                      ; 一页放不下
        sta     SHELL_DISK_SIZE + 1
        lda     SHELL_REQ_RESULT + 1
        sta     SHELL_DISK_SIZE
        lda     SHELL_DISK_SIZE + 1
        bne     @size_ok
        lda     SHELL_DISK_SIZE
        cmp     #DISK_HEADER_SIZE
        bcc     @bad_close                      ; 连头都没有
@size_ok:
        jsr     disk_alloc
        bcs     @bad_close
        lda     #FN_FILE_HANDLE
        ldx     #SUB_FH_READ_FILE
        jsr     req_begin
        lda     SHELL_DISK_PAGE
        ldx     #<FILE_START_PAGE
        ldy     #>FILE_START_PAGE
        jsr     req_write
        jsr     dos_gate
        php
        jsr     disk_close
        plp
        bcs     @fail
        jsr     disk_map_stage
        ldx     #DISK_HEADER_SIZE - 1
@head:  lda     DISK_STAGE_BASE,x
        sta     $0500,x
        dex
        bpl     @head
        jsr     disk_unmap_stage
        clc                                     ; 头里的长度加头要在文件大小之内
        lda     $0512
        adc     #DISK_HEADER_SIZE
        sta     V_TMP1
        lda     $0513
        adc     #$00
        sta     V_TMP2
        lda     SHELL_DISK_SIZE
        cmp     V_TMP1
        lda     SHELL_DISK_SIZE + 1
        sbc     V_TMP2
        bcc     @fail
        ldx     #$0F                            ; 头里的文件名改成要找的名字，原版的“名字对不对”检查就一定通过
@name:  lda     $0580,x
        sta     $0501,x
        dex
        bpl     @name
        lda     #$00
        sta     $0511
        clc
        rts
@bad_close:
        jsr     disk_close
@fail:  jmp     disk_error

LBE24:  clc                                     ; 目的 $0514/$0515 加长度 = 程序结尾，要在 $6C00 之前
        lda     $0514
        adc     $0512
        sta     $AD
        lda     $0515
        adc     $0513
        sta     $AE
        bcs     @fail
        lda     $AE
        cmp     #$6C
        bcs     @fail
        lda     $0512
        ora     $0513
        beq     @done
        lda     $0514
        sta     $19
        lda     $0515
        sta     $1A
        lda     #DISK_HEADER_SIZE
        sta     $1B
        lda     #$00
        sta     $1C
        lda     SHELL_DISK_PAGE
        sec
        sbc     #$80
        sta     V_GAME_PAGE
        lda     $0512
        sta     V_TMP1
        lda     $0513
        sta     V_TMP2
        jsr     dma_piece
@done:  jsr     disk_free
        clc
        rts
@fail:  jmp     disk_error

LBE1C:  jsr     disk_map_stage
        jsr     disk_set_data_pointers
        ldy     #$00
@loop:  lda     V_COPY_LEN
        ora     V_COPY_LEN + 1
        beq     @same
        lda     (V_COPY_SRC),y
        cmp     (V_COPY_DST),y
        bne     @diff
        inc     V_COPY_SRC
        bne     @src
        inc     V_COPY_SRC + 1
@src:   inc     V_COPY_DST
        bne     @dst
        inc     V_COPY_DST + 1
@dst:   lda     V_COPY_LEN
        bne     @len
        dec     V_COPY_LEN + 1
@len:   dec     V_COPY_LEN
        jmp     @loop
@same:  jsr     disk_unmap_stage
        jsr     disk_free
        clc
        rts
@diff:  jsr     disk_unmap_stage
        jmp     disk_error
