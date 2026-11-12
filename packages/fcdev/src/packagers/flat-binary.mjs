// ------------------------------------------------------------------------
// 名称：flat-binary.mjs
// 说明：平铺映像打包器：校验链接输出的文件头与大小，写成目标要求的程序文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：选项 header 为十六进制字符串，?? 表示任意字节；maxSize 为映像大小上限（字节）。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import { FcdevError } from '../shared/index.mjs';

/**
 * 把十六进制模式拆成字节列表，null 表示通配。
 * @param {string} pattern 如 "4C????AA55"
 * @returns {(number|null)[]} 字节列表
 * @throws {FcdevError} 模式不合法
 */
export function parseHeaderPattern(pattern) {
  if (typeof pattern !== 'string' || pattern.length % 2 !== 0 || !/^([0-9A-Fa-f]{2}|\?\?)*$/.test(pattern)) {
    throw new FcdevError(`header 选项“${pattern}”不是合法的十六进制模式`, { code: 'PACKAGER_OPTION_INVALID' });
  }
  const bytes = [];
  for (let index = 0; index < pattern.length; index += 2) {
    const pair = pattern.slice(index, index + 2);
    bytes.push(pair === '??' ? null : Number.parseInt(pair, 16));
  }
  return bytes;
}

/**
 * 校验并写出平铺映像。
 * @param {{binaryFile: string, outputFile: string, options: {header?: string, maxSize?: number}}} input 链接输出、产物路径与打包选项
 * @returns {{outputFile: string, size: number}} 产物路径与大小
 * @throws {FcdevError} 文件头不符或超过大小上限
 */
export function packFlatBinary({ binaryFile, outputFile, options }) {
  const image = readFileSync(binaryFile);
  if (options.header !== undefined) {
    const expected = parseHeaderPattern(options.header);
    const matches = image.length >= expected.length && expected.every((byte, index) => byte === null || image[index] === byte);
    if (!matches) {
      throw new FcdevError(`程序文件头不符：应为 ${options.header}，实际为 ${image.subarray(0, expected.length).toString('hex').toUpperCase()}`, {
        code: 'PACKAGE_HEADER_MISMATCH',
        hint: '检查启动代码是否放在映像最前面',
      });
    }
  }
  if (options.maxSize !== undefined && image.length > options.maxSize) {
    throw new FcdevError(`程序大小 ${image.length} 字节，超过上限 ${options.maxSize} 字节`, { code: 'PACKAGE_TOO_LARGE' });
  }
  writeFileSync(outputFile, image);
  return { outputFile, size: image.length };
}
