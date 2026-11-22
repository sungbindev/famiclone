/*
 * ------------------------------------------------------------------------
 * 名称：sound.c
 * 说明：测试 sb2000-sound.h 里的声音与语音函数：停止与状态、调整移调与拍速、播放曲子与音效、播放 WAV、发送语音数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：曲子用 BIOS 页 $78 里 $8445 处的 17 字节提示音（单声道，音效格式，声道 5），起播后 $047D-$0480 立即变成 8F F9 BF 0B；
 *       WAV 与语音数据在程序自己的窗口里构造；语音数据用最小的合法流：静音帧 $00、结束帧 $0F。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 声道 5 的发声寄存器影子，播放提示音后变成 8F F9 BF 0B */
#define CHANNEL5_SHADOW 0x047D

/** 一个只有 4 个采样的 22050 Hz WAV：文件头 44 字节，偏移 $18 是采样率低字节，偏移 $28 是采样长度 */
static unsigned char wav[48];
/** 语音数据：静音帧、结束帧 */
static const unsigned char speech[] = { 0x00, 0x0F };
/** 词条表：头 2 字节是个数 1，一项 3 字节（页号增量 0、偏移 5），数据紧跟其后 */
static const unsigned char entries[] = { 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0F };

/** 检查声道 5 的影子是不是提示音起播后的样子 */
static unsigned char beep_started(void)
{
    return SB_BYTE_AT(CHANNEL5_SHADOW) == 0x8F && SB_BYTE_AT(CHANNEL5_SHADOW + 1) == 0xF9
        && SB_BYTE_AT(CHANNEL5_SHADOW + 2) == 0xBF && SB_BYTE_AT(CHANNEL5_SHADOW + 3) == 0x0B;
}

/** 把声道 5 的影子清 0 */
static void clear_shadow(void)
{
    unsigned char index;

    for (index = 0; index < 4; ++index) {
        SB_BYTE_AT(CHANNEL5_SHADOW + index) = 0;
    }
}

int main(void)
{
    unsigned char before[4];
    unsigned char index;
    unsigned char tempo;
    unsigned char ok;

    /* 停止后状态是 $FF */
    sb_sound_stop();
    SB_CHECK_EQ(sb_sound_status(), 0xFF);

    /* 调整移调：4 个声道都加上同一个值；调整拍速：拍速与备份单元都加上 */
    for (index = 0; index < 4; ++index) {
        before[index] = SB_BYTE_AT(SB_SOUND_TRANSPOSE + index);
    }
    sb_sound_transpose(5);
    ok = 1;
    for (index = 0; index < 4; ++index) {
        if (SB_BYTE_AT(SB_SOUND_TRANSPOSE + index) != (unsigned char)(before[index] + 5)) {
            ok = 0;
        }
    }
    SB_CHECK(ok);
    tempo = SB_BYTE_AT(SB_SOUND_TEMPO);
    sb_sound_tempo(3);
    SB_CHECK_EQ(SB_BYTE_AT(SB_SOUND_TEMPO), (unsigned char)(tempo + 3));
    SB_CHECK_EQ(SB_BYTE_AT(SB_SOUND_TEMPO + 1), (unsigned char)(tempo + 3));

    /* 播放提示音，再停止；清掉影子后用“音效”接口播放同一首，影子再次出现 */
    clear_shadow();
    sb_sound_play_from(0x78, 0x8445);
    sb_wait_frames(2);
    SB_CHECK(beep_started());
    sb_sound_stop();
    clear_shadow();
    sb_sound_effect_from(0x78, 0x8445);
    sb_wait_frames(2);
    SB_CHECK(beep_started());
    sb_sound_stop();
    SB_CHECK_EQ(sb_sound_status(), 0xFF);

    /* WAV：播完才返回；重复 0 遍与 2 遍也返回 */
    memset(wav, 0, sizeof(wav));
    wav[0x18] = 0x22;
    wav[0x28] = 4;
    wav[44] = 0x80;
    wav[45] = 0x90;
    wav[46] = 0x70;
    wav[47] = 0x80;
    sb_sound_play_wav(wav);
    SB_CHECK(1);
    sb_sound_repeat_wav_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)wav, 0);
    SB_CHECK(1);
    sb_sound_repeat_wav_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)wav, 2);
    SB_CHECK(1);

    /* 语音：关闭、同步发送一段数据、按序号发送词条（越界序号什么都不发），发完待发字节与发送标志都清 0 */
    sb_speech_off();
    sb_speech_send(speech);
    SB_CHECK(SB_BYTE_AT(0x0328) == 0 && SB_BYTE_AT(0x0329) == 0);
    sb_speech_send_entry_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)entries, 0);
    SB_CHECK(SB_BYTE_AT(0x0328) == 0 && SB_BYTE_AT(0x0329) == 0);
    sb_speech_send_entry_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)entries, 9);
    SB_CHECK(SB_BYTE_AT(0x0328) == 0 && SB_BYTE_AT(0x0329) == 0);
    sb_speech_off();

    sb_test_done();
    return 0;
}
