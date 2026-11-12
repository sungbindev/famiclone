// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：packagers 目录的对外入口：按名称取打包器。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：新增打包器时在 PACKAGERS 里登记；每个打包器接收 { binaryFile, outputFile, options } 并返回 { outputFile, size }。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';
import { packFatFloppy } from './fat-floppy.mjs';
import { packFlatBinary } from './flat-binary.mjs';
import { packInes } from './ines.mjs';

export { listFat12, readFat12File } from './fat12-image.mjs';
export { parseHeaderPattern } from './flat-binary.mjs';

/** 已登记的程序打包器 */
const PACKAGERS = {
  'flat-exe': packFlatBinary,
  ines: packInes,
};

/** 已登记的磁盘打包器：输入一组文件，输出一个镜像 */
const DISK_PACKAGERS = {
  'fat-floppy': packFatFloppy,
};

/**
 * 按名称取打包器。
 * @param {string} name 打包器名
 * @returns {(input: {binaryFile: string, outputFile: string, options: Record<string, unknown>}) => {outputFile: string, size: number}} 打包函数
 * @throws {FcdevError} 打包器没有登记
 */
export function getPackager(name) {
  const packager = PACKAGERS[name];
  if (!packager) {
    throw new FcdevError(`未知的打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的打包器：${Object.keys(PACKAGERS).join('、')}` });
  }
  return packager;
}

/**
 * 按名称取磁盘打包器。
 * @param {string} name 打包器名
 * @returns {(input: {files: {name: string, data: Buffer, mtime?: Date}[], outputFile: string, options?: Record<string, unknown>}) => {outputFile: string, size: number}} 打包函数
 * @throws {FcdevError} 打包器没有登记
 */
export function getDiskPackager(name) {
  const packager = DISK_PACKAGERS[name];
  if (!packager) {
    throw new FcdevError(`未知的磁盘打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的磁盘打包器：${Object.keys(DISK_PACKAGERS).join('、')}` });
  }
  return packager;
}
