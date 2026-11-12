// ------------------------------------------------------------------------
// 名称：config.mjs
// 说明：步步高工程用到的路径常量：原厂源码、ROM、本工程的符号文件目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：BIOS 源码在本工程的 src/bios 里；其余原厂资料放在仓库根目录的 reference/ 下，只引用不复制。
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

/** 本工程里的 BIOS 源码：SB2000 版（GBK 编码），由原厂版经 tools/port-bios.mjs 移植而来，构建 bbk.bios 只读这一份 */
export const BIOS_SOURCE = join(BBK_ROOT, 'src', 'bios', 'Bios.asm');

/**
 * 按文件名取原厂源文件的路径；分析工具（比对 ROM、查符号）用它，因为 ROM 对应的是原厂版而不是 SB2000 版。
 * @param {string} name 源文件名
 * @returns {string} 绝对路径
 */
export function resolveSource(name) {
  return join(ORIGINAL_SOURCE_DIR, name);
}

/** DOS 启动盘镜像（1.44MB，含 BBGDOS.SYS 与 COMMAND.CMD） */
export const DOS_FLOPPY = join(REPO_ROOT, 'reference', 'floppy-images', 'bbk', 'dos启动盘.IMG');

/** iNES 文件头长度 */
export const INES_HEADER_SIZE = 16;

/** 还原出的符号文件所在目录，汇编时作为包含目录 */
export const INCLUDE_DIR = join(BBK_ROOT, 'src', 'include');

/** 构建输出目录（不进 Git） */
export const BUILD_DIR = join(BBK_ROOT, 'build');
