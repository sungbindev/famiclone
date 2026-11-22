/*
 * ------------------------------------------------------------------------
 * 名称：system.c
 * 说明：测试 sb2000-system.h 里的系统基础函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000-library.test.mjs 在模拟器里运行；会破坏性的检查（系统初始化、恢复上下文）放在最后。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>
#include "sb-test.h"

extern void nmi_hook_entry(void);

/** 钩子被调用的次数，system-hook.s 里加 1 */
unsigned char hook_count;
/** 被 sb_jump_banked 跳转到的函数是否执行过 */
static unsigned char landed;
/** 恢复点入口执行时的标志 */
static unsigned char restored;

static void landing(void)
{
    landed = 1;
}

/** 比调用者再深一层，用来检查 sb_stack_pointer；先存进变量，避免编译器把调用优化成跳转 */
static unsigned char nested_stack_pointer(void)
{
    static unsigned char sink;

    sink = sb_stack_pointer();
    return sink;
}

/** sb_context_restore 的跳转目标：新的执行起点，不返回 */
static void context_landing(void)
{
    restored = 1;
    SB_CHECK_EQ(SB_BYTE_AT(SB_CONTEXT_COUNT), 0xFF);
    SB_CHECK_EQ(SB_BYTE_AT(SB_NMI_FLAGS), 0);
    sb_test_done();
    for (;;) {
    }
}

int main(void)
{
    static const unsigned char service_bits[6] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x02 };
    unsigned char program_page = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    unsigned char frames_before;
    unsigned char saved_flags;
    unsigned char saved_irq;
    unsigned char after_on;
    unsigned char after_off;
    unsigned char stack_a;
    unsigned char stack_b;
    unsigned char saved_vector_lo;
    unsigned char saved_vector_hi;
    unsigned char count;
    unsigned char i;

    /* 等待若干帧：NMI 计数至少增加这么多 */
    frames_before = SB_BYTE_AT(SB_NMI_COUNTER_UP);
    sb_wait_frames(5);
    count = (unsigned char)(SB_BYTE_AT(SB_NMI_COUNTER_UP) - frames_before);
    SB_CHECK(count >= 5 && count <= 20);

    /* 关闭与打开 NMI */
    sb_nmi_disable();
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x80) == 0);
    sb_nmi_enable();
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x80) == 0x80);

    /* NMI 服务开关：先关 NMI，避免服务在检查期间真的运行 */
    saved_flags = SB_BYTE_AT(SB_NMI_FLAGS);
    sb_nmi_disable();
    for (i = 0; i < 6; ++i) {
        sb_nmi_service(i + 1, 1);
        after_on = SB_BYTE_AT(SB_NMI_FLAGS);
        sb_nmi_service(i + 1, 0);
        after_off = SB_BYTE_AT(SB_NMI_FLAGS);
        SB_CHECK((after_on & service_bits[i]) == service_bits[i] && (after_off & service_bits[i]) == 0);
    }
    SB_BYTE_AT(SB_NMI_FLAGS) = 0;
    sb_nmi_service(0, 1);
    sb_nmi_service(7, 1);
    SB_CHECK_EQ(SB_BYTE_AT(SB_NMI_FLAGS), 0);
    sb_nmi_service(5, 1);
    SB_CHECK_EQ(SB_BYTE_AT(SB_MOUSE_ABSENT), 0xFF);
    SB_BYTE_AT(SB_NMI_FLAGS) = saved_flags;
    sb_nmi_enable();

    /* 中断屏蔽：中断源 3、4 屏蔽后位置 1，允许后清 0 */
    saved_irq = SB_BYTE_AT(SB_IRQ_MASK_SHADOW);
    sb_irq_mask(3);
    after_on = SB_BYTE_AT(SB_IRQ_MASK_SHADOW);
    sb_irq_enable(3);
    after_off = SB_BYTE_AT(SB_IRQ_MASK_SHADOW);
    SB_CHECK((after_on & 0x10) == 0x10 && (after_off & 0x10) == 0);
    sb_irq_mask(4);
    after_on = SB_BYTE_AT(SB_IRQ_MASK_SHADOW);
    sb_irq_enable(4);
    after_off = SB_BYTE_AT(SB_IRQ_MASK_SHADOW);
    SB_CHECK((after_on & 0x08) == 0x08 && (after_off & 0x08) == 0);
    sb_irq_mask(7);
    SB_CHECK_EQ(SB_BYTE_AT(SB_IRQ_MASK_SHADOW), after_off);
    sb_irq_enable(1);
    SB_CHECK((SB_BYTE_AT(SB_IRQ_MASK_SHADOW) & 0x40) == 0
        && SB_BYTE_AT(SB_MOUSE_ABSENT) == (SB_BYTE_AT(SB_MOUSE_PRESENT) ? 0 : 0xFF));
    for (i = 1; i < 5; ++i) {
        if (i == 2) {
            continue;
        }
        if (saved_irq & (0x80 >> i)) {
            sb_irq_mask(i);
        } else {
            sb_irq_enable(i);
        }
    }

    /* 显示选项：选项 5 并入 $2000 的位 5 */
    SB_CHECK_EQ(sb_display_option(5, 0x20), 0);
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x20) == 0x20 && sb_system_state(5) == 0x20);
    SB_CHECK_EQ(sb_display_option(5, 0), 0);
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x20) == 0 && sb_system_state(5) == 0);
    SB_CHECK_EQ(sb_display_option(8, 0), 0xFF);

    /* 状态查询 */
    SB_CHECK_EQ(sb_system_state(10), 1);
    SB_CHECK_EQ(sb_system_state(9), SB_BYTE_AT(SB_MOUSE_PRESENT));
    SB_CHECK_EQ(sb_system_state(8), SB_BYTE_AT(SB_IRQ_MASK_SHADOW));
    SB_CHECK_EQ(sb_system_state(3), SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 3);
    SB_CHECK_EQ(sb_system_state(12), SB_BYTE_AT(SB_PPU_MASK_SHADOW) & 0x18);
    SB_CHECK_EQ(sb_system_state(13), 0);

    /* 显示的关闭与打开 */
    sb_display_off();
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x80) == 0);
    sb_display_on();
    SB_CHECK((SB_BYTE_AT(SB_PPU_CTRL_SHADOW) & 0x80) == 0x80);

    /* 调用 ROM 运行库里的例程：光标右移一列 */
    SB_BYTE_AT(SB_CURSOR_COL) = 5;
    sb_call_banked(0, SB_LIB_CURSOR_RIGHT);
    SB_CHECK_EQ(SB_BYTE_AT(SB_CURSOR_COL), 6);

    /* 跳转到自己程序里的函数，函数的 RTS 回到本调用之后 */
    sb_jump_banked(program_page, (unsigned int)landing);
    SB_CHECK_EQ(landed, 1);

    /* 栈指针：嵌套一层的值比外层小 2 */
    stack_a = sb_stack_pointer();
    stack_b = nested_stack_pointer();
    SB_CHECK_EQ(stack_b, (unsigned char)(stack_a - 2));

    /* NMI 钩子：登记、打开服务后每帧被调用，关闭服务后不再调用，复位后地址改变 */
    sb_nmi_hook_set(5, program_page, (unsigned int)nmi_hook_entry);
    SB_CHECK(SB_BYTE_AT(SB_NMI_HOOK5_VEC_LO) == (unsigned char)(unsigned int)nmi_hook_entry
        && SB_BYTE_AT(SB_NMI_HOOK5_VEC_LO + 1) == (unsigned char)((unsigned int)nmi_hook_entry >> 8)
        && SB_BYTE_AT(SB_NMI_HOOK5_PAGE) == program_page);
    hook_count = 0;
    sb_nmi_service(SB_NMI_SERVICE_HOOK5, 1);
    sb_wait_frames(3);
    SB_CHECK(hook_count >= 3);
    sb_nmi_service(SB_NMI_SERVICE_HOOK5, 0);
    count = hook_count;
    sb_wait_frames(3);
    SB_CHECK_EQ(hook_count, count);
    sb_nmi_hook_reset(5);
    SB_CHECK(SB_BYTE_AT(SB_NMI_HOOK5_VEC_LO) != (unsigned char)(unsigned int)nmi_hook_entry
        || SB_BYTE_AT(SB_NMI_HOOK5_VEC_LO + 1) != (unsigned char)((unsigned int)nmi_hook_entry >> 8));

    /* 中断钩子：登记后地址与页组正确，复位后地址改变；中断 2 钩子用完恢复原值 */
    sb_irq_hook_set(3, program_page, (unsigned int)nmi_hook_entry);
    SB_CHECK(SB_BYTE_AT(SB_IRQ4_HOOK_VEC_LO) == (unsigned char)(unsigned int)nmi_hook_entry
        && SB_BYTE_AT(SB_IRQ4_HOOK_PAGE) == program_page);
    sb_irq_hook_reset(3);
    SB_CHECK(SB_BYTE_AT(SB_IRQ4_HOOK_VEC_LO) != (unsigned char)(unsigned int)nmi_hook_entry
        || SB_BYTE_AT(SB_IRQ4_HOOK_VEC_LO + 1) != (unsigned char)((unsigned int)nmi_hook_entry >> 8));
    saved_vector_lo = SB_BYTE_AT(SB_IRQ2_HOOK_VEC_LO);
    saved_vector_hi = SB_BYTE_AT(SB_IRQ2_HOOK_VEC_LO + 1);
    sb_irq_hook_set(6, program_page, (unsigned int)nmi_hook_entry);
    SB_CHECK(SB_BYTE_AT(SB_IRQ2_HOOK_VEC_LO) == (unsigned char)(unsigned int)nmi_hook_entry
        && SB_BYTE_AT(SB_IRQ2_HOOK_PAGE) == program_page);
    SB_BYTE_AT(SB_IRQ2_HOOK_VEC_LO) = saved_vector_lo;
    SB_BYTE_AT(SB_IRQ2_HOOK_VEC_LO + 1) = saved_vector_hi;

    /* 系统初始化：时钟回到 12:00:00，上下文栈清空 */
    sb_system_init();
    SB_CHECK_EQ(SB_BYTE_AT(SB_CLOCK_HOURS), 12);
    SB_CHECK_EQ(SB_BYTE_AT(SB_CONTEXT_COUNT), 0xFF);

    /* 上下文：保存后个数为 0；恢复后跳到入口，入口里清点并结束 */
    sb_context_save(program_page, (unsigned int)context_landing, sb_stack_pointer());
    SB_CHECK_EQ(SB_BYTE_AT(SB_CONTEXT_COUNT), 0);
    sb_context_restore();

    /* 不会执行到这里 */
    SB_CHECK(0);
    sb_test_done();
    return 0;
}
