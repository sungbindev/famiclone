// ------------------------------------------------------------------------
// 名称：config.mjs
// 说明：步步高工程用到的路径常量：原厂源码、ROM、本工程的符号文件目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：原厂资料放在仓库根目录的 reference/ 下，本工程只引用不复制。
// ------------------------------------------------------------------------

import { fileURLToPath } from 'node:url';
import { dirname, join, resolve } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));

/** 步步高工程根目录（machines/bbk） */
export const BBK_ROOT = resolve(here, '..');

/** 仓库根目录 */
export const REPO_ROOT = resolve(BBK_ROOT, '..', '..');

/** 原厂 DOS 源码目录（GBK 编码，缺 tvdef.asm） */
export const ORIGINAL_SOURCE_DIR = join(REPO_ROOT, 'reference', 'docs', 'BBKDOS源代码');

/** 步步高 BIOS 1.0 的 ROM（128KB，含 16 字节 iNES 头） */
export const ROM_BIOS10 = join(REPO_ROOT, 'reference', 'rom', 'bbk_bios10.nes');

/** DOS 启动盘镜像（1.44MB，含 BBGDOS.SYS 与 COMMAND.CMD） */
export const DOS_FLOPPY = join(REPO_ROOT, 'reference', 'floppy-images', 'bbk', 'dos启动盘.IMG');

/** iNES 文件头长度 */
export const INES_HEADER_SIZE = 16;

/** 还原出的符号文件所在目录，汇编时作为包含目录 */
export const INCLUDE_DIR = join(BBK_ROOT, 'src', 'include');

/** 构建输出目录（不进 Git） */
export const BUILD_DIR = join(BBK_ROOT, 'build');
