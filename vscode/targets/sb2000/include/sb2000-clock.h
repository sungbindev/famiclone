/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-clock.h
 * 说明：SB-DOS 程序的系统日期与时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。日期的年份范围是 1980 到 2099，时间的秒精度是 2 秒（设置时秒数取一半，读出时再乘 2）。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_CLOCK_H
#define SB2000_CLOCK_H

/** 日期。 */
typedef struct {
    /** 年，1980 到 2099。 */
    unsigned int year;
    /** 月，1 到 12。 */
    unsigned char month;
    /** 日，1 到当月天数。 */
    unsigned char day;
} sb_date_t;

/** 时间。 */
typedef struct {
    /** 时，0 到 23。 */
    unsigned char hour;
    /** 分，0 到 59。 */
    unsigned char minute;
    /** 秒，0 到 59，实际精度 2 秒。 */
    unsigned char second;
} sb_time_t;

/**
 * 设置系统日期。系统按年份能否被 4 整除判断闰年（不处理世纪年）。
 *
 * @syscall $7F/$01
 * @param date 新日期。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 日期不合法，系统日期不变。
 */
unsigned char sb_clock_set_date(const sb_date_t *date);

/**
 * 取系统日期。过了午夜后第一次取时，系统先把日期加一天。
 *
 * @syscall $7F/$02
 * @param date 返回当前日期。
 */
void sb_clock_get_date(sb_date_t *date);

/**
 * 设置系统时间。
 *
 * @syscall $7F/$03
 * @param time 新时间。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 时间不合法，系统时间不变。
 */
unsigned char sb_clock_set_time(const sb_time_t *time);

/**
 * 取系统时间。
 *
 * @syscall $7F/$04
 * @param time 返回当前时间。
 */
void sb_clock_get_time(sb_time_t *time);

#endif
