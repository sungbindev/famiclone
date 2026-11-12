// ------------------------------------------------------------------------
// 名称：config.mjs
// 说明：bbk-bios 工程用到的路径常量；原厂资料、汇编器所在的 machines/bbk 的常量原样转出。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：BIOS 的汇编器、符号文件和字库 ROM 不复制，直接用 machines/bbk 里的。
// ------------------------------------------------------------------------

import { fileURLToPath } from 'node:url';
import { dirname, join, resolve } from 'node:path';

export { DOS_FLOPPY, INCLUDE_DIR, INES_HEADER_SIZE, REPO_ROOT, ROM_BIOS10 } from '../../../../machines/bbk/tools/config.mjs';
export { BIOS_SOURCE as ORIGINAL_BIOS_SOURCE } from '../../../../machines/bbk/tools/config.mjs';

const here = dirname(fileURLToPath(import.meta.url));

/** 本工程根目录（apps/sb2000/bbk-bios） */
export const BIOS_ROOT = resolve(here, '..');

/** SB2000 版 BIOS 源码（GBK 编码，不要转码保存）；由原版经 tools/port-bios.mjs 移植而来，构建只读这一份 */
export const BIOS_SOURCE = join(BIOS_ROOT, 'src', 'bios.asm');

/** 构建输出目录（不进 Git） */
export const BUILD_DIR = join(BIOS_ROOT, 'build');

/** 构建产物（BIOS.BBK）放这里 */
export const BIN_DIR = join(BUILD_DIR, 'bin');

/** 验证用的临时文件（合并启动盘、截图）放这里，可随时删除 */
export const WORK_DIR = join(BUILD_DIR, 'work');

/** 加载器 START.EXE，由 apps/sb2000/bbk-loader 构建 */
export const LOADER_EXE = join(BIOS_ROOT, '..', 'bbk-loader', 'build', 'sb2000', 'START.EXE');
