/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-input.h
 * 说明：SB-DOS 程序的输入：键盘、手柄、鼠标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。
 *       键盘：系统把按键放进 5 个按键容量的队列（超过的丢弃），每个按键有 ASCII 码和扫描码（IBM 扫描码集 1），
 *             功能键、方向键等没有字符的键 ASCII 码为 0；按键组合下的结果与标准 IBM 美式键盘一致。
 *       手柄：由 NMI 每帧读取，所以 NMI 要开着；键码 $FF 表示没有键，1 号手柄 $0F（A）、$0E（B）、$0D（选择）、$0C（开始）、
 *             $0B（上）、$0A（下）、$09（左）、$08（右），2 号手柄同序为 $07-$00，同时按下多个键时取最高位的。
 *       鼠标：模拟器与真机都把鼠标当成串口鼠标，数据在中断里解析，程序只读坐标与按键状态。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_INPUT_H
#define SB2000_INPUT_H

/** 两个鼠标键的状态，下标 0 是按键 1，下标 1 是按键 2。 */
typedef struct {
    /** 状态：0 没按，1 按住，2 刚松开。 */
    unsigned char state[2];
    /** 双击：1 表示发生了双击，读走即清。 */
    unsigned char double_click[2];
    /** 单击：1 表示按下后松开过，读走即清；拖动后松开不算单击。 */
    unsigned char click[2];
    /** 拖动：1 表示按住并移动。 */
    unsigned char drag[2];
} sb_mouse_buttons_t;

/**
 * 等待按键并返回它的 ASCII 码。
 *
 * @return 按键的 ASCII 码；没有字符的键（如功能键）为 0。
 */
unsigned char sb_getkey(void);

/**
 * 等到有按键，取出它的 ASCII 码与扫描码。
 *
 * @syscall $6B/$04
 * @return 低字节是 ASCII 码，高字节是扫描码。
 */
unsigned int sb_key_wait(void);

/**
 * 不等待，取出队列里的一个按键。
 *
 * @syscall $6B/$03
 * @return 低字节是 ASCII 码，高字节是扫描码；队列为空时为 0。
 */
unsigned int sb_key_poll(void);

/**
 * 按键队列里的按键数。
 *
 * @syscall $6B/$06
 * @return 按键数，0 到 5。
 */
unsigned char sb_key_count(void);

/**
 * 清空按键队列。
 *
 * @syscall $6B/$05
 */
void sb_key_flush(void);

/**
 * 复位键盘与鼠标，清空按键队列。约需几十帧。
 *
 * @syscall $6B/$00
 */
void sb_key_init(void);

/**
 * 打开按 ESC 中断程序：之后按 ESC，系统取出上下文栈顶的恢复点（sb_context_save 保存的）并跳转过去。
 *
 * @syscall $6B/$01
 */
void sb_break_enable(void);

/**
 * 关闭按 ESC 中断程序。
 *
 * @syscall $6B/$02
 */
void sb_break_disable(void);

/**
 * 修饰键状态：位 0 右 Shift，位 1 左 Shift，位 2 Ctrl，位 3 Alt，位 4 ScrollLock，位 5 NumLock，位 6 CapsLock，位 7 Insert。
 *
 * @return 修饰键状态。
 */
unsigned char sb_key_modifiers(void);

/**
 * 复位并识别键盘。复位后键盘不再上报按键，需要再调用 sb_key_init 才恢复，一般用 sb_key_init 即可。
 *
 * @syscall $17
 * @return 1 通过，0 失败。
 */
unsigned char sb_keyboard_reset(void);

/**
 * 初始化手柄状态，并打开 NMI 里读手柄的服务。
 *
 * @syscall $12/$85、$6A/$00
 */
void sb_pad_init(void);

/**
 * 不等待，取一个按下的手柄键。
 *
 * @syscall $6A/$01
 * @return 键码，$FF 表示没有键。
 */
unsigned char sb_pad_poll(void);

/**
 * 等到有手柄键按下。
 *
 * @syscall $6A/$02
 * @return 键码。
 */
unsigned char sb_pad_wait(void);

/**
 * 取一个手柄的原始按键状态，由 sb_pad_poll、sb_pad_wait 更新：位 7 是 A，位 6 是 B，位 5 选择，位 4 开始，
 * 位 3 上，位 2 下，位 1 左，位 0 右。
 *
 * @param pad 手柄号，0 是 1 号手柄，其他是 2 号手柄。
 * @return 原始状态。
 */
unsigned char sb_pad_state(unsigned char pad);

/**
 * 复位鼠标，返回鼠标是否接入。复位要尝试多次，约需几十帧；之后要用 sb_irq_enable(SB_IRQ_SOURCE_MOUSE) 允许鼠标中断，
 * 再设置坐标范围与初始坐标。
 *
 * @syscall $14
 * @return 非 0 表示鼠标已接入，0 表示没有鼠标。
 */
unsigned char sb_mouse_reset(void);

/**
 * 设置鼠标光标坐标的范围，原有的双击间隔不变。
 *
 * @syscall $6C/$00
 * @param x_min X 下限。
 * @param y_min Y 下限。
 * @param x_max X 上限。
 * @param y_max Y 上限。
 */
void sb_mouse_set_limits(unsigned char x_min, unsigned char y_min, unsigned char x_max, unsigned char y_max);

/**
 * 设置鼠标光标的坐标。
 *
 * @syscall $6C/$04
 * @param x X 坐标。
 * @param y Y 坐标。
 */
void sb_mouse_set_position(unsigned char x, unsigned char y);

/**
 * 取鼠标光标的坐标。每收到一个数据包，坐标就加上位移，并限制在设置的范围内。
 *
 * @syscall $6C/$02
 * @param x 返回 X 坐标。
 * @param y 返回 Y 坐标。
 */
void sb_mouse_position(unsigned char *x, unsigned char *y);

/**
 * 取两个鼠标键的状态。单击、双击读走即清。
 *
 * @syscall $6C/$03
 * @param buttons 返回按键状态。
 */
void sb_mouse_buttons(sb_mouse_buttons_t *buttons);

/**
 * 设置鼠标双击的间隔：按下后在这个间隔内再次按下算双击，间隔由定时器中断计时，默认 10。
 * 间隔为 0 时不产生双击。
 *
 * @syscall $6C/$01
 * @param interval 间隔，单位是定时器中断的次数。
 */
void sb_mouse_set_double_click(unsigned char interval);

#endif
