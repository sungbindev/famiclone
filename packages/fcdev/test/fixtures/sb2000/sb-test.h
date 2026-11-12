/*
 * ------------------------------------------------------------------------
 * 名称：sb-test.h
 * 说明：SB2000 运行库测试程序的公共约定：把检查结果记在固定变量里，由测试脚本从模拟器内存读出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：测试程序每做一次检查就调用 SB_CHECK 或 SB_CHECK_EQ，结束时调用 sb_test_done；
 *       需要宿主（测试脚本）配合操作时调用 sb_test_sync，脚本在这里停下、注入按键等，再继续运行。
 * ------------------------------------------------------------------------
 */
#ifndef SB_TEST_H
#define SB_TEST_H

/* 最多记录的检查次数，测试程序可在包含本文件前自行定义 */
#ifndef SB_TEST_MAX
#define SB_TEST_MAX 80
#endif

/** 已记录的检查次数 */
extern unsigned char sb_test_count;
/** 每次检查的结果：1 通过，2 失败 */
extern unsigned char sb_test_ok[SB_TEST_MAX];
/** 每次检查所在的源文件行号 */
extern unsigned int sb_test_line[SB_TEST_MAX];
/** 每次检查的实际值，失败时用于诊断 */
extern unsigned int sb_test_value[SB_TEST_MAX];

/** 记录一次检查，由宏调用 */
void sb_test_record(unsigned char ok, unsigned int line, unsigned int value);
/** 全部检查结束，脚本在这里读取结果 */
void sb_test_done(void);
/** 需要宿主操作时的停靠点，脚本停在这里，操作完再继续 */
void sb_test_sync(void);

#define SB_CHECK(cond) sb_test_record((cond) != 0, __LINE__, 0)
#define SB_CHECK_EQ(actual, expected) \
    do { \
        unsigned int sb_actual_ = (unsigned int)(actual); \
        sb_test_record(sb_actual_ == (unsigned int)(expected), __LINE__, sb_actual_); \
    } while (0)

#endif
