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

/** 步步高工程根目录（machines/bbg） */
export const BBG_ROOT = resolve(here, '..');

/** 仓库根目录 */
export const REPO_ROOT = resolve(BBG_ROOT, '..', '..');

/** 原厂 DOS 源码目录（GBK 编码，缺 tvdef.asm） */
export const ORIGINAL_SOURCE_DIR = join(REPO_ROOT, 'reference', 'docs', 'BBGDOS源代码');

/** 步步高 BIOS 1.0 的 ROM（128KB，纯 BIOS，没有文件头） */
export const ROM_BIOS10 = join(REPO_ROOT, 'ember', 'roms', 'bbg1_bios.bin');

/** 本工程里的原版 BIOS 源码：从原厂资料原样复制（GBK 编码，不要转码保存），分析工具读这一份 */
export const BIOS_SOURCE = join(BBG_ROOT, 'src', 'bios', 'bios.asm');

/**
 * 按文件名取原厂源文件的路径：Bios.asm 用本工程里的副本，其余取原厂目录。
 * @param {string} name 源文件名
 * @returns {string} 绝对路径
 */
export function resolveSource(name) {
  return name.toLowerCase() === 'bios.asm' ? BIOS_SOURCE : join(ORIGINAL_SOURCE_DIR, name);
}

/** DOS 启动盘镜像（1.44MB，含 BBGDOS.SYS 与 COMMAND.CMD） */
export const DOS_FLOPPY = join(REPO_ROOT, 'reference', 'floppy-images', 'bbg', 'dos启动盘.IMG');

/** iNES 文件头长度 */
export const INES_HEADER_SIZE = 16;

/** 还原出的符号文件所在目录，汇编时作为包含目录 */
export const INCLUDE_DIR = join(BBG_ROOT, 'src', 'include');
