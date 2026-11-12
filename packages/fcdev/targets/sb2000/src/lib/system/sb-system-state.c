/*
 * ------------------------------------------------------------------------
 * 名称：sb-system-state.c
 * 说明：sb_system_state 的实现：查询显示与系统状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $1E，结果在 $0652；查询号超出 0-12 时不调用，返回 0。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

unsigned char sb_system_state(unsigned char query)
{
    if (query > 12) {
        return 0;
    }
    sb_syscall(SB_FN_QUERY_STATE, query);
    return SB_BYTE_AT(SB_PARAM_0652);
}
