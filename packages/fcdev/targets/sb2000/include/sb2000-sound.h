/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-sound.h
 * 说明：SB-DOS 程序的声音与语音服务：播放曲子与音效、停止、调整移调与拍速、播放内存里的 WAV、发送语音数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。曲子、WAV、语音数据的位置用“页组 + 窗口地址”表示，约定同 sb_call_banked：页组是连续 4 页的首页号，
 *       这 4 页映射到 $8000-$BFFF，窗口地址是数据在窗口里的地址。WAV 与语音数据在程序自己的窗口里时可用不带 _from 的版本，函数自动用程序的起始页做页组。
 *       曲子只提供带页组的版本：把曲子放进程序窗口里起播后，引擎在 NMI 里读曲子会读乱（实测），要把曲子放在 ROM 页，或自己申请的扩展 RAM 页里（用 sb_far_write 写入）。
 *       曲子格式与音乐引擎见系统调用手册（docs/sb2000/02-系统调用手册.md）5.5 节；发声在 NMI 中断里更新，更新要求 NMI 开着（sb_nmi_enable）且 $0630 的声音位开着。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_SOUND_H
#define SB2000_SOUND_H

/** 语音数据传送完时的结束帧（低 4 位全 1）。 */
#define SB_SPEECH_END_FRAME 0x0F

/**
 * 载入并播放一首曲子（音乐格式或音效格式）。先关闭声音更新，复位发声状态，起播后再打开声音更新。
 *
 * @syscall $65/$00
 * @param group 曲子所在页组的首页号。
 * @param address 曲子在页组窗口里的地址。
 */
void sb_sound_play_from(unsigned char group, unsigned int address);

/**
 * 停止播放：复位发声状态，关闭声音更新，播放状态变成 $FF。
 *
 * @syscall $65/$02
 */
void sb_sound_stop(void);

/**
 * 取播放状态。
 *
 * @syscall $65/$03
 * @return 播放状态，停止播放后是 $FF。
 */
unsigned char sb_sound_status(void);

/**
 * 调整 4 个音乐声道的移调值：每个声道的移调值都加上 delta（8 位回绕）。
 *
 * @syscall $65/$04
 * @param delta 要加的值，负数用补码。
 */
void sb_sound_transpose(unsigned char delta);

/**
 * 调整拍速：拍速值加上 delta（8 位回绕），同步写进备份单元。
 *
 * @syscall $65/$05
 * @param delta 要加的值，负数用补码。
 */
void sb_sound_tempo(unsigned char delta);

/**
 * 复位后播放一首音效格式的曲子（音效声道 4-7）。音乐格式的曲子用它起不了播，要用 sb_sound_play_from。
 *
 * @syscall $65/$0B
 * @param group 曲子所在页组的首页号。
 * @param address 曲子在页组窗口里的地址。
 */
void sb_sound_effect_from(unsigned char group, unsigned int address);

/**
 * 播放内存里的 WAV（8 位采样）。屏蔽中断播放，播完才返回。
 * 采样率取文件头偏移 $18 的低字节：$44 是 44100 Hz，$22 是 22050 Hz，其他按 11025 Hz；采样长度取偏移 $28 的 4 个字节，跳过 44 字节文件头；
 * 实际会多播一个采样（总数是长度加 1）。
 *
 * @syscall $65/$0C
 * @param group WAV 所在页组的首页号。
 * @param address WAV 文件头在页组窗口里的地址。
 */
void sb_sound_play_wav_from(unsigned char group, unsigned int address);

/**
 * 播放程序自己窗口里的 WAV，规则同 sb_sound_play_wav_from。
 *
 * @syscall $65/$0C
 * @param wav WAV 文件头的地址，在程序窗口 $8000-$BFFF 里。
 */
void sb_sound_play_wav(const void *wav);

/**
 * 重复播放内存里的 WAV。次数不为 0 时先等 5 帧再开始；播完（含次数为 0）后声音寄存器恢复。
 *
 * @syscall $65/$0D
 * @param group WAV 所在页组的首页号。
 * @param address WAV 文件头在页组窗口里的地址。
 * @param times 重复的遍数。
 */
void sb_sound_repeat_wav_from(unsigned char group, unsigned int address, unsigned char times);

/**
 * 关闭语音：清语音芯片的使能位与语音控制标志的“已使用”位。
 *
 * @syscall $74/$00
 */
void sb_speech_off(void);

/**
 * 同步发送一段语音数据（LPC 码流）：先复位语音芯片，写命令字节 $0A，再把数据逐字节写给语音端口，读到结束帧后返回。
 * 数据要以结束帧（SB_SPEECH_END_FRAME，低 4 位全 1）收尾，否则不会返回。
 *
 * @syscall $75/$00
 * @param group 数据所在页组的首页号。
 * @param address 数据在页组窗口里的地址。
 */
void sb_speech_send_from(unsigned char group, unsigned int address);

/**
 * 同步发送程序自己窗口里的语音数据，规则同 sb_speech_send_from。
 *
 * @syscall $75/$00
 * @param data 语音数据的地址，在程序窗口 $8000-$BFFF 里。
 */
void sb_speech_send(const void *data);

/**
 * 按序号发送词条表里的一个词条。词条表头 2 字节是词条个数（低字节在前），之后每项 3 字节：页号增量、相对表起点的偏移低字节、偏移高字节。
 * 序号不小于词条个数时什么都不发送。
 *
 * @syscall $75/$01
 * @param group 词条表所在页组的首页号。
 * @param table 词条表在页组窗口里的地址。
 * @param index 词条序号，从 0 起。
 */
void sb_speech_send_entry_from(unsigned char group, unsigned int table, unsigned int index);

#endif
