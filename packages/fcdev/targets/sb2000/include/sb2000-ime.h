/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-ime.h
 * 说明：SB-DOS 程序的中文输入法服务：初始化与选择输入法、状态与全半角、处理按键得到汉字、十六进制与区位码转换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。输入法状态放在 ERAM $6900 起，状态行画在屏幕上（调用显示服务 $45）。
 *       输入法序号：0 区位、1 全拼、2 双拼、3 五笔、4 表形、5 认知、6 自定义、7 扩充、8 图形、9 英文；3-6 需要外挂模块，7 不可选。
 *       处理按键的结果在 sb_ime_result_t 里：flag 为 0 无输出、2 输出汉字内码（output 里每个汉字 2 个字节）、$82 按键原样传回。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_IME_H
#define SB2000_IME_H

/** 输入法序号：区位。 */
#define SB_IME_QUWEI 0
/** 输入法序号：全拼。 */
#define SB_IME_PINYIN 1
/** 输入法序号：双拼。 */
#define SB_IME_SHUANGPIN 2
/** 输入法序号：英文。 */
#define SB_IME_ENGLISH 9
/** sb_ime_select 的 flags：同时核对 ERAM 里的允许位。 */
#define SB_IME_CHECK_ALLOWED 0x80

/** 初始化参数。 */
typedef struct {
    /** 状态行所在的行号。 */
    unsigned char row;
    /** 状态行的图块基数，16 位。 */
    unsigned int tile;
    /** 状态行显示用的子功能号（选名称表等）。 */
    unsigned char subfunction;
    /** 状态行颜色。 */
    unsigned char color;
    /** 初始输入法序号，0 到 9。 */
    unsigned char method;
    /** 允许位（ERAM $6901）。 */
    unsigned char allow;
    /** 标志位（ERAM $6902）。 */
    unsigned char flags;
} sb_ime_config_t;

/** 处理一个键的结果。 */
typedef struct {
    /** 结果标志：0 无输出、2 输出汉字内码、$82 按键原样传回。 */
    unsigned char flag;
    /** 输出的内容，每个汉字 2 个字节（GB2312）。 */
    unsigned char output[16];
} sb_ime_result_t;

/**
 * 初始化输入法：记下状态行的位置与颜色，选择初始输入法，登记说明文字。
 *
 * @syscall $6D/$00
 * @param config 初始化参数。
 * @return 0 成功；1 输入法序号不小于 10、没有权限或不可选，其余参数已写入。
 */
unsigned char sb_ime_init(const sb_ime_config_t *config);

/**
 * 选择输入法。成功时清按键队列与输入缓冲，状态行换成该输入法的名称。
 *
 * @syscall $6D/$01
 * @param method 输入法序号 0 到 9；序号 3 到 6 要 ERAM $0392 里对应的位允许，7 不可选。
 * @param flags 0，或 SB_IME_CHECK_ALLOWED 表示同时核对允许位。
 * @return 0 成功；1 失败，状态不变。
 */
unsigned char sb_ime_select(unsigned char method, unsigned char flags);

/**
 * 设置输入法状态：允许位与标志位（只取低 7 位，位 7 保留）。
 *
 * @syscall $6D/$02
 * @param allow 允许位。
 * @param flags 标志位。
 */
void sb_ime_set_state(unsigned char allow, unsigned char flags);

/**
 * 取输入法状态。
 *
 * @syscall $6D/$08
 * @param allow 返回允许位。
 * @param flags 返回标志位的低 7 位。
 */
void sb_ime_get_state(unsigned char *allow, unsigned char *flags);

/**
 * 取当前输入法序号。
 *
 * @syscall $6D/$07
 * @return 序号 0 到 9。
 */
unsigned char sb_ime_get_method(void);

/**
 * 恢复被暂停的输入法：暂停标志置位时清除它并刷新状态行，否则什么都不做。
 *
 * @syscall $6D/$03
 */
void sb_ime_resume(void);

/**
 * 暂停输入法：清候选字，状态行清为空，暂停标志置位。已经暂停时什么都不做。暂停期间按键原样传回。
 *
 * @syscall $6D/$04
 */
void sb_ime_suspend(void);

/**
 * 设置半角或全角：状态行文字换成“半角”或“全角”，标志位里的全角位相应清除或置位。全角时输出的半角符号会换成全角内码。
 *
 * @syscall $6D/$05、$6D/$06
 * @param full 0 半角，非 0 全角。
 */
void sb_ime_set_width(unsigned char full);

/**
 * 处理一个按键（不等待）：从键盘队列取一个键交给输入法，没有键时直接返回。
 * 输入法的热键：Alt+F1 到 Alt+F10 选输入法 0 到 9，Ctrl+F1 暂停与恢复，Ctrl+F8、Ctrl+F9 切换全角半角，数字键 1 到 5 与空格选候选字。
 *
 * @syscall $6D/$0B
 * @param result 返回处理结果。
 * @return 结果标志，同 result->flag：0 无输出，2 输出汉字内码，$82 按键原样传回。
 */
unsigned char sb_ime_key(sb_ime_result_t *result);

/**
 * 等待并处理按键，直到输入法有结果（结果标志不为 0）才返回，没有键时一直等下去。
 *
 * @syscall $6D/$0A
 * @param result 返回处理结果。
 * @return 结果标志，同 result->flag。
 */
unsigned char sb_ime_key_wait(sb_ime_result_t *result);

/**
 * 把一个 16 位内码转成 4 个小写十六进制字符。
 *
 * @syscall $6D/$0E
 * @param code 内码。
 * @param text 返回 4 个字符与结尾的 0，要有 5 字节空间。
 */
void sb_ime_hex(unsigned int code, char *text);

/**
 * 把 4 个输入字符转成汉字内码：全是数字当区位码（4 位十进制），第一个字符是 a 到 f 当 4 位十六进制内码。
 *
 * @syscall $6D/$0D
 * @param chars 4 个输入字符。
 * @param code 返回内码。
 * @return 0 成功；1 不合法。
 */
unsigned char sb_ime_parse(const char *chars, unsigned int *code);

#endif
