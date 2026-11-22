// ------------------------------------------------------------------------
// 名称：program-packager.ts
// 说明：打包器：把链接输出校验后做成最终产物，或把一组文件做成磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：只做字节处理，读写文件由调用者负责；新增打包器时在 PROGRAM_PACKAGERS 或
//       DISK_PACKAGERS 登记。
//       flat-exe 的选项 header 为十六进制字符串（?? 表示任意字节），headerOffset 为它在映像里的
//       偏移（默认 0），maxSize 为映像大小上限，size 为映像必须正好等于的大小。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import { buildFat12Image, type DiskFile } from './fat12-image';

/** 打包器选项。 */
export type PackagerOptions = Readonly<Record<string, unknown>>;

/** iNES 文件头的前 4 个字节：“NES”加 $1A。 */
const INES_MAGIC = Buffer.from([0x4e, 0x45, 0x53, 0x1a]);

/** iNES 文件头的长度。 */
const INES_HEADER_SIZE = 16;

/**
 * 把十六进制模式拆成字节列表，null 表示通配。
 * @param pattern 如 "4C????AA55"。
 * @returns 字节列表。
 * @throws FcdevError 模式不合法。
 */
export function parseHeaderPattern(pattern: string): Array<number | null> {
  if (pattern.length % 2 !== 0 || !/^([0-9A-Fa-f]{2}|\?\?)*$/.test(pattern)) {
    throw new FcdevError(`header 选项“${pattern}”不是合法的十六进制模式`, { code: 'PACKAGER_OPTION_INVALID' });
  }
  const bytes: Array<number | null> = [];
  for (let index = 0; index < pattern.length; index += 2) {
    const pair = pattern.slice(index, index + 2);
    bytes.push(pair === '??' ? null : Number.parseInt(pair, 16));
  }
  return bytes;
}

/**
 * 校验平铺映像的文件头与大小。
 * @param image 链接输出。
 * @param options 打包选项。
 * @returns 要写出的内容。
 * @throws FcdevError 选项不合法，文件头不符，或大小不符。
 */
function packFlatBinary(image: Uint8Array, options: PackagerOptions): Uint8Array {
  const { header, headerOffset = 0, maxSize, size } = options;
  if (typeof headerOffset !== 'number' || !Number.isInteger(headerOffset) || headerOffset < 0) {
    throw new FcdevError('headerOffset 选项必须是不小于 0 的整数', { code: 'PACKAGER_OPTION_INVALID' });
  }
  if (header !== undefined) {
    if (typeof header !== 'string') throw new FcdevError('header 选项必须是字符串', { code: 'PACKAGER_OPTION_INVALID' });
    const expected = parseHeaderPattern(header);
    const matches = image.length >= headerOffset + expected.length && expected.every((byte, index) => byte === null || image[headerOffset + index] === byte);
    if (!matches) {
      const actual = Buffer.from(image.subarray(headerOffset, headerOffset + expected.length)).toString('hex').toUpperCase();
      throw new FcdevError(`程序文件头不符：偏移 ${headerOffset} 处应为 ${header}，实际为 ${actual}`, {
        code: 'PACKAGE_HEADER_MISMATCH',
        hint: headerOffset === 0 ? '检查启动代码是否放在映像最前面' : '检查链接配置，文件头要放在映像里的这个位置'
      });
    }
  }
  if (size !== undefined && typeof size === 'number' && image.length !== size) {
    throw new FcdevError(`程序大小 ${image.length} 字节，必须正好是 ${size} 字节`, { code: 'PACKAGE_SIZE_MISMATCH' });
  }
  if (maxSize !== undefined && typeof maxSize === 'number' && image.length > maxSize) {
    throw new FcdevError(`程序大小 ${image.length} 字节，超过上限 ${maxSize} 字节`, { code: 'PACKAGE_TOO_LARGE' });
  }
  return image;
}

/**
 * 校验链接输出是 iNES 格式。
 * @param image 链接输出。
 * @returns 要写出的内容。
 * @throws FcdevError 文件头不是 iNES，或文件长度与头里声明的不符。
 */
function packInes(image: Uint8Array): Uint8Array {
  if (image.length < INES_HEADER_SIZE || !Buffer.from(image.subarray(0, 4)).equals(INES_MAGIC)) {
    throw new FcdevError('链接输出不是 iNES 格式（缺少 “NES” 文件头）', {
      code: 'PACKAGE_HEADER_MISMATCH',
      hint: '检查链接配置是否生成 iNES 头'
    });
  }
  const expectedSize = INES_HEADER_SIZE + (image[4] ?? 0) * 0x4000 + (image[5] ?? 0) * 0x2000;
  if (image.length !== expectedSize) {
    throw new FcdevError(`iNES 文件长度 ${image.length} 字节，与文件头声明的 ${expectedSize} 字节不符`, { code: 'PACKAGE_SIZE_MISMATCH' });
  }
  return image;
}

/** 已登记的程序打包器。 */
const PROGRAM_PACKAGERS: Readonly<Record<string, (image: Uint8Array, options: PackagerOptions) => Uint8Array>> = {
  'flat-exe': packFlatBinary,
  ines: packInes
};

/** 已登记的磁盘打包器：输入一组文件，输出一个镜像。 */
const DISK_PACKAGERS: Readonly<Record<string, (files: readonly DiskFile[], options: { readonly label?: string }) => Uint8Array>> = {
  'fat-floppy': (files, options) => buildFat12Image(files, options)
};

/**
 * 按名称打包程序：校验链接输出并返回最终产物的内容。
 * @param name 打包器名。
 * @param image 链接输出。
 * @param options 打包选项。
 * @returns 最终产物的内容。
 * @throws FcdevError 打包器没有登记，或链接输出不符合要求。
 */
export function packageProgram(name: string, image: Uint8Array, options: PackagerOptions): Uint8Array {
  const packager = PROGRAM_PACKAGERS[name];
  if (packager === undefined) {
    throw new FcdevError(`未知的打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的打包器：${Object.keys(PROGRAM_PACKAGERS).join('、')}` });
  }
  return packager(image, options);
}

/**
 * 按名称把一组文件打包成磁盘镜像。
 * @param name 磁盘打包器名。
 * @param files 要放进镜像的文件。
 * @param options 打包选项。
 * @returns 镜像内容。
 * @throws FcdevError 打包器没有登记，文件名不符合 8.3，或空间不足。
 */
export function packageDisk(name: string, files: readonly DiskFile[], options: { readonly label?: string } = {}): Uint8Array {
  const packager = DISK_PACKAGERS[name];
  if (packager === undefined) {
    throw new FcdevError(`未知的磁盘打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的磁盘打包器：${Object.keys(DISK_PACKAGERS).join('、')}` });
  }
  return packager(files, options);
}
