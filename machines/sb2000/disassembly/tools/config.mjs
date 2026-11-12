// ------------------------------------------------------------------------
// 名称：config.mjs
// 说明：反汇编工具链共用的路径常量，所有路径都由 sb2000 机型目录和仓库根目录推出。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-09
// 备注：无
// ------------------------------------------------------------------------
import { readdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

/** 反汇编工作目录（本文件所在 tools 的上一级） */
export const WORK_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** sb2000 机型目录（machines/sb2000） */
export const MACHINE_DIR = path.resolve(WORK_DIR, '..');

/** 仓库根目录 */
export const REPO_ROOT = path.resolve(MACHINE_DIR, '..', '..');

/** BIOS 源码工程目录 */
export const BIOS_DIR = path.join(MACHINE_DIR, 'bios');

/** BIOS 源码工程的构建输出目录（构建出的 ROM 与调试变体） */
export const BIOS_BUILD_DIR = path.join(BIOS_DIR, 'build');

/** 带调试服务的模拟器可执行文件（在本仓库的 virtuanes/ 里维护，需先编译） */
export const EMULATOR_EXE = path.join(REPO_ROOT, 'build', 'virtuanes', 'Release', 'bin', 'VirtuaNES.exe');

/** SB2K BIOS 原始 ROM（含 16 字节 iNES 文件头） */
export const BIOS_ROM = path.join(MACHINE_DIR, 'reference', 'rom', 'SB2K_BIOS.NES');

/** DOS 场景与验证用例的开机镜像：1.44MB FAT12，不含 AUTOEXEC.BAT，开机停在 DOS 提示符，里面有 HELLO.TXT */
export const DOS_IMAGE = path.join(MACHINE_DIR, 'reference', 'floppy-dos.img');

/** 真实软件的软盘镜像目录 */
export const IMAGES_DIR = path.join(MACHINE_DIR, 'reference', 'floppy-images');

/** 分析产物输出目录 */
export const OUT_DIR = path.join(WORK_DIR, 'out');

/** 官方软盘（文件名 FLPPY<盘号>….img）的盘号与完整路径，按盘号排序 */
export function officialImages() {
  return readdirSync(IMAGES_DIR)
    .map((name) => ({ name, match: /^FLPPY(\d+).*\.img$/i.exec(name) }))
    .filter(({ match }) => match !== null)
    .map(({ name, match }) => ({ number: Number(match[1]), file: path.join(IMAGES_DIR, name) }))
    .sort((a, b) => a.number - b.number);
}

/** 某张官方软盘的完整路径（如 1 是 WPS 文字处理系统，20 是游戏集锦 027） */
export function officialImage(number) {
  const found = officialImages().find((image) => image.number === number);
  if (!found) throw new Error(`找不到官方软盘 ${number}：${IMAGES_DIR}`);
  return found.file;
}
