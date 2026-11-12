/*
 * ------------------------------------------------------------------------
 * 名称：sbi-dir-state.c
 * 说明：库内部数据：目录遍历的状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：sb_dir_open 设置，sb_dir_read 推进；同一时间只能遍历一个目录。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sbi_dir_lba;
unsigned char sbi_dir_index;
unsigned char sbi_dir_ended = 1;
