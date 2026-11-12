/*
 * ------------------------------------------------------------------------
 * 名称：sb-exec.c
 * 说明：sb_exec 的实现：运行另一个程序，运行完返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7F 子功能 0，路径放 PATH_BUFFER；系统按文件大小分配页、读入、检查文件头魔数，运行完释放页；
 *       子程序会改写 C 运行库的零页，由 sbi_exec_run 在运行前后保存、恢复。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_exec(const char *path)
{
    unsigned char error = sbi_set_path(path);

    if (error != 0) {
        return error;
    }
    return sbi_exec_run();
}
