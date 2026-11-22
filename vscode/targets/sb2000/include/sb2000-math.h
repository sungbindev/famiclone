/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-math.h
 * 说明：SB-DOS 程序的数学服务：伪随机数、整数除法、时间文字、十进制文字与浮点数、直线步进、3 字节整数运算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。这些服务的参数区在 $069A-$06B3，函数帮你把数据放进去再取回来；指针参数必须在程序窗口 $8000-$BFFF 里。
 *       浮点数是 7 个字节：阶码、符号（$00 正、$FF 负）、5 字节尾数，值 = ±尾数 ÷ 2^40 × 2^阶码；十进制文字是 13 个字符：1 个符号字符（空格或 -）加 12 个带小数点的数字字符。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_MATH_H
#define SB2000_MATH_H

/** 浮点数，7 个字节。值 = ±尾数 ÷ 2^40 × 2^阶码。 */
typedef struct {
    /** 阶码。 */
    unsigned char exponent;
    /** 符号，$00 表示正，$FF 表示负。 */
    unsigned char sign;
    /** 尾数，5 个字节，高字节在前。 */
    unsigned char mantissa[5];
} sb_float_t;

/** 3 字节整数：1 个符号字节加 3 个数值字节。 */
typedef struct {
    /** 符号，$00 表示正，$FF 表示负。 */
    unsigned char sign;
    /** 数值，3 个字节，高字节在前。 */
    unsigned char value[3];
} sb_int24_t;

/** 直线步进的参数块，共 18 个字节。 */
typedef struct {
    /** 当前点的横坐标，每走一步更新。 */
    unsigned char x;
    /** 当前点的纵坐标，每走一步更新。 */
    unsigned char y;
    /** 终点的横坐标。 */
    unsigned char end_x;
    /** 终点的纵坐标。 */
    unsigned char end_y;
    /** 本次前进的步数，0 表示 256 步。 */
    unsigned char steps;
    /** 系统用来存两轴长度、方向、误差与中间量的工作区，不要改。 */
    unsigned char work[13];
} sb_line_t;

/**
 * 设置伪随机数寄存器。寄存器的两个高字节全为 0 时，下一次步进前系统先把次高字节置成 $FF，所以种子 0 也能用。
 *
 * @syscall 直接写 $0696-$0699
 * @param seed 32 位种子。
 */
void sb_random_seed(unsigned long seed);

/**
 * 把伪随机数寄存器步进一次，返回新的寄存器值。
 *
 * @syscall $4A/$01
 * @return 步进后的 32 位寄存器值。
 */
unsigned long sb_random_next(void);

/**
 * 把伪随机数寄存器连续步进 100 次，用来打散刚设置的种子。
 *
 * @syscall $4A/$00
 */
void sb_random_warm_up(void);

/**
 * 16 位数除以 8 位数。
 *
 * @syscall $4B
 * @param dividend 被除数。
 * @param divisor 除数。
 * @param quotient 返回商。
 * @param remainder 返回余数。
 * @return 0 成功；$FF 除数为 0，商与余数没有意义。
 */
unsigned char sb_math_div16(unsigned int dividend, unsigned char divisor, unsigned int *quotient, unsigned char *remainder);

/**
 * 把时、分、秒转成 HH:MM:SS 文字。
 *
 * @syscall $4C
 * @param hour 时。
 * @param minute 分。
 * @param second 秒。
 * @param text 返回 8 个字符加结尾的 0，要有 9 字节空间。
 */
void sb_math_time_text(unsigned char hour, unsigned char minute, unsigned char second, char *text);

/**
 * 十进制文字转浮点数。
 *
 * @syscall $4D/$00
 * @param text 13 个字符：第 1 个是符号（空格或 -），后面 12 个是带小数点的数字。
 * @param result 返回浮点数。
 */
void sb_float_from_text(const char *text, sb_float_t *result);

/**
 * 浮点数转十进制文字。
 *
 * @syscall $4E/$00
 * @param value 浮点数。
 * @param text 返回 13 个字符（符号加 12 位带小数点的数字）与结尾的 0，要有 14 字节空间。
 */
void sb_float_to_text(const sb_float_t *value, char *text);

/**
 * 浮点加法：a + b。
 *
 * @syscall $51
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果，可以与 a 或 b 是同一个变量。
 */
void sb_float_add(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);

/**
 * 浮点减法：a - b。
 *
 * @syscall $52
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果，可以与 a 或 b 是同一个变量。
 */
void sb_float_sub(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);

/**
 * 浮点乘法：a * b。
 *
 * @syscall $4F
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果，可以与 a 或 b 是同一个变量。
 */
void sb_float_mul(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);

/**
 * 浮点除法：a / b。除数的尾数为 0 时结果的第 1 个字节（阶码）为 $80。
 *
 * @syscall $50
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果，可以与 a 或 b 是同一个变量。
 */
void sb_float_div(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);

/**
 * 十进制数字文字转 40 位整数。前导空格当 0，超过 40 位时高位丢弃。
 *
 * @syscall $4D/$01
 * @param digits 数字字符，不带符号与小数点。
 * @param count 数字个数，0 到 12。
 * @param value 返回 5 个字节的整数，高字节在前。
 */
void sb_int40_from_text(const char *digits, unsigned char count, unsigned char *value);

/**
 * 40 位整数转十进制数字文字，数字左对齐、右边补字符 0。
 *
 * @syscall $4E/$01
 * @param value 5 个字节的整数，高字节在前。
 * @param digits 返回 13 个数字字符与结尾的 0，要有 14 字节空间。
 * @param length 返回去掉前导 0 后的有效位数，至少为 1。
 * @return 0 成功；$FF 数值不小于 10^12，放不下。
 */
unsigned char sb_int40_to_text(const unsigned char *value, char *digits, unsigned char *length);

/**
 * 直线步进（布雷森汉姆算法）：从当前点向终点前进指定的步数，落点写回 x、y。步数不小于主轴长度时直接落在终点。
 *
 * @syscall $53
 * @param line 参数块，调用前填好 x、y、end_x、end_y；系统服务换页后程序窗口不可见，所以函数把它复制到 ERAM 里再调用，返回后复制回来。
 * @param steps 前进的步数，0 表示 256 步。
 */
void sb_line_step(sb_line_t *line, unsigned char steps);

/**
 * 3 字节整数加法：a + b，带符号。
 *
 * @syscall $54/$00
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果。
 * @return 0 没有溢出；1 结果超过 24 位，result 里是低 24 位。
 */
unsigned char sb_int24_add(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *result);

/**
 * 3 字节整数减法：a - b，带符号。
 *
 * @syscall $54/$01
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param result 返回结果。
 * @return 0 没有溢出；1 结果超过 24 位，result 里是低 24 位。
 */
unsigned char sb_int24_sub(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *result);

/**
 * 3 字节整数乘法：a * b，乘积最多 48 位。
 *
 * @syscall $54/$02
 * @param a 第一个操作数。
 * @param b 第二个操作数。
 * @param sign 返回乘积的符号，$00 正、$FF 负。
 * @param product 返回 6 个字节的乘积，高字节在前。
 */
void sb_int24_mul(const sb_int24_t *a, const sb_int24_t *b, unsigned char *sign, unsigned char *product);

/**
 * 3 字节整数除法：a / b，得商与余数。
 *
 * @syscall $54/$03
 * @param a 被除数。
 * @param b 除数。
 * @param quotient 返回商，符号是商的符号。
 * @param remainder 返回余数的数值，符号字节与商相同。
 * @return 0 成功；1 除数为 0，商与余数没有意义。
 */
unsigned char sb_int24_div(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *quotient, sb_int24_t *remainder);

#endif
