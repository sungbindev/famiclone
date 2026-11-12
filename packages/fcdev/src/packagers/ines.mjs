// ------------------------------------------------------------------------
// 名称：ines.mjs
// 说明：iNES 打包器：校验链接输出以 iNES 文件头开头，写成 .nes 文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：iNES 头由 cc65 的 nes.cfg 生成，这里只校验，不改写内容。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import { FcdevError } from '../shared/index.mjs';

/** iNES 文件头的前 4 个字节：“NES”加 $1A */
const INES_MAGIC = Buffer.from([0x4e, 0x45, 0x53, 0x1a]);

/** iNES 文件头的长度 */
const INES_HEADER_SIZE = 16;

/**
 * 校验并写出 .nes 文件。
 * @param {{binaryFile: string, outputFile: string}} input 链接输出与产物路径
 * @returns {{outputFile: string, size: number}} 产物路径与大小
 * @throws {FcdevError} 文件头不是 iNES，或文件长度与头里声明的不符
 */
export function packInes({ binaryFile, outputFile }) {
  const image = readFileSync(binaryFile);
  if (image.length < INES_HEADER_SIZE || !image.subarray(0, 4).equals(INES_MAGIC)) {
    throw new FcdevError('链接输出不是 iNES 格式（缺少 “NES” 文件头）', {
      code: 'PACKAGE_HEADER_MISMATCH',
      hint: '检查链接配置是否生成 iNES 头',
    });
  }
  const expectedSize = INES_HEADER_SIZE + image[4] * 0x4000 + image[5] * 0x2000;
  if (image.length !== expectedSize) {
    throw new FcdevError(`iNES 文件长度 ${image.length} 字节，与文件头声明的 ${expectedSize} 字节不符`, { code: 'PACKAGE_SIZE_MISMATCH' });
  }
  writeFileSync(outputFile, image);
  return { outputFile, size: image.length };
}
