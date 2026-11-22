/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-key-wait.c
 * 说明：sb_ime_key_wait 的实现：等待并处理按键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 $0A。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ime_key_wait(sb_ime_result_t *result)
{
    sb_syscall(SB_FN_IME, 10);
    result->flag = SB_BYTE_AT(SB_IME_KEY_RESULT);
    sbi_peek(result->output, SB_IME_KEY_OUT, sizeof(result->output));
    return result->flag;
}
