// ------------------------------------------------------------------------
// 名称：program-packager.ts
// 说明：打包器：把链接输出校验后做成最终产物，或把一组文件做成磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：只做字节处理，读写文件由调用者负责；新增打包器时在 PROGRAM_PACKAGERS 或
//       DISK_PACKAGERS 登记。fat-floppy 生成空白 1.44MB 镜像；fat-floppy-overlay 在调用
//       者给的底盘镜像（如带系统文件的启动盘）上添加文件，没有底盘时报错。
//       flat-exe 的选项 header 为十六进制字符串（?? 表示任意字节），headerOffset 为它在映像里的
//       偏移（默认 0），maxSize 为映像大小上限，size 为映像必须正好等于的大小。
//       rom-overlay 的偏移与大小取自目标专用选项 offset、size（0 表示不限）：有底固件时，把链接
//       输出覆盖到底固件的 offset 处；没有底固件时，链接输出就是整块固件。
//       dos33-binary 给链接输出加 Apple DOS 3.3 二进制文件的 4 字节文件头（装入地址与
//       长度，低字节在前），地址取目标专用选项，选项名由打包选项 addressOption
//       指定（默认 loadAddress）；
//       dos33-floppy-overlay 在 DOS 3.3 的 .dsk 底盘上添加文件。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import { overlayDos33Image } from './dos33-image';
import { buildFat12Image, type DiskFile } from './fat12-image';
import { overlayFat12Image } from './fat12-overlay';

/** 打包器选项。 */
export type PackagerOptions = Readonly<Record<string, unknown>>;

/** 打包时由构建提供的上下文：目标专用选项，以及按需读取底固件的函数。 */
export interface PackagerContext {
  /** 合并后的目标专用选项。 */
  readonly targetOptions: Readonly<Record<string, unknown>>;
  /** 读取底固件（fcdev.json 的 firmware）；没有配置时返回 undefined，文件读不到时抛错。 */
  readonly readBase: () => Uint8Array | undefined;
}

/** 不提供底固件的上下文。 */
const EMPTY_CONTEXT: PackagerContext = { targetOptions: {}, readBase: () => undefined };

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

/**
 * 读取目标专用选项里的非负整数，缺省为 0。
 * @param targetOptions 目标专用选项。
 * @param key 选项名。
 * @returns 选项值。
 * @throws FcdevError 选项不是非负整数。
 */
function readNonNegativeInteger(targetOptions: Readonly<Record<string, unknown>>, key: string): number {
  const value = targetOptions[key] ?? 0;
  if (typeof value !== 'number' || !Number.isInteger(value) || value < 0) {
    throw new FcdevError(`目标选项 ${key} 必须是不小于 0 的整数`, { code: 'PACKAGER_OPTION_INVALID' });
  }
  return value;
}

/**
 * 把链接输出做成整块固件：有底固件时覆盖到 offset 处，没有时链接输出就是整块固件。
 * @param image 链接输出。
 * @param _options 打包选项（未使用）。
 * @param context 目标专用选项与底固件。
 * @returns 要写出的内容。
 * @throws FcdevError 选项不合法、缺少底固件、大小不符，或链接输出放不进底固件。
 */
function packRomOverlay(image: Uint8Array, _options: PackagerOptions, context: PackagerContext): Uint8Array {
  const offset = readNonNegativeInteger(context.targetOptions, 'offset');
  const size = readNonNegativeInteger(context.targetOptions, 'size');
  const base = context.readBase();
  if (base === undefined) {
    if (offset !== 0) {
      throw new FcdevError(`offset 为 ${offset}，链接输出只是整块固件的一部分，需要底固件`, {
        code: 'PACKAGE_BASE_MISSING',
        hint: '在 fcdev.json 里用 firmware 指向完整的固件文件，其余部分取自它'
      });
    }
    if (size !== 0 && image.length !== size) {
      throw new FcdevError(`固件大小 ${image.length} 字节，必须正好是 ${size} 字节`, { code: 'PACKAGE_SIZE_MISMATCH' });
    }
    return image;
  }
  if (size !== 0 && base.length !== size) {
    throw new FcdevError(`底固件 ${base.length} 字节，与 size 选项的 ${size} 字节不符`, { code: 'PACKAGE_SIZE_MISMATCH', hint: '检查 fcdev.json 的 firmware 指向的文件' });
  }
  if (offset + image.length > base.length) {
    throw new FcdevError(`链接输出 ${image.length} 字节，放在偏移 ${offset} 处超出底固件的 ${base.length} 字节`, { code: 'PACKAGE_TOO_LARGE' });
  }
  const result = Uint8Array.from(base);
  result.set(image, offset);
  return result;
}

/**
 * 给链接输出加 DOS 3.3 二进制文件的文件头：装入地址与长度，各 2 字节，低字节在前。
 * @param image 链接输出，第一个字节就是装入地址上的内容。
 * @param options 打包选项：addressOption 是保存装入地址的目标专用选项名，默认 loadAddress。
 * @param context 目标专用选项。
 * @returns 要写出的内容。
 * @throws FcdevError 选项不合法，或程序为空、超出 64K。
 */
function packDos33Binary(image: Uint8Array, options: PackagerOptions, context: PackagerContext): Uint8Array {
  const { addressOption = 'loadAddress' } = options;
  if (typeof addressOption !== 'string') throw new FcdevError('addressOption 选项必须是字符串', { code: 'PACKAGER_OPTION_INVALID' });
  const address = context.targetOptions[addressOption];
  if (typeof address !== 'number' || !Number.isInteger(address) || address < 0 || address > 0xffff) {
    throw new FcdevError(`目标选项 ${addressOption} 必须是 0 到 65535 的装入地址`, { code: 'PACKAGER_OPTION_INVALID' });
  }
  if (image.length === 0) throw new FcdevError('程序是空的', { code: 'PACKAGE_SIZE_MISMATCH' });
  if (address + image.length > 0x10000) {
    throw new FcdevError(`程序 ${image.length} 字节，从 ${address} 装入会超出 64K`, { code: 'PACKAGE_TOO_LARGE' });
  }
  const result = new Uint8Array(4 + image.length);
  result.set([address & 0xff, address >> 8, image.length & 0xff, image.length >> 8], 0);
  result.set(image, 4);
  return result;
}

/** 已登记的程序打包器。 */
const PROGRAM_PACKAGERS: Readonly<Record<string, (image: Uint8Array, options: PackagerOptions, context: PackagerContext) => Uint8Array>> = {
  'flat-exe': packFlatBinary,
  ines: packInes,
  'rom-overlay': packRomOverlay,
  'dos33-binary': packDos33Binary
};

/** 磁盘打包器的选项。 */
export interface DiskPackagerOptions {
  /** 卷标（只用于新建的镜像）。 */
  readonly label?: string;
  /** 底盘镜像（只用于覆盖打包器）。 */
  readonly base?: Uint8Array;
}

/** 已登记的磁盘打包器：输入一组文件，输出一个镜像。 */
const DISK_PACKAGERS: Readonly<Record<string, (files: readonly DiskFile[], options: DiskPackagerOptions) => Uint8Array>> = {
  'fat-floppy': (files, options) => buildFat12Image(files, options),
  'fat-floppy-overlay': (files, options) => {
    if (options.base === undefined) {
      throw new FcdevError('这个目标的磁盘镜像要在系统盘镜像上添加文件，没有给底盘镜像', {
        code: 'DISK_BASE_MISSING',
        hint: '在 fcdev.json 里用 diskImage 指向带系统文件的软盘镜像'
      });
    }
    return overlayFat12Image(options.base, files);
  },
  'dos33-floppy-overlay': (files, options) => {
    if (options.base === undefined) {
      throw new FcdevError('这个目标的磁盘镜像要在 DOS 3.3 系统盘镜像上添加文件，没有给底盘镜像', {
        code: 'DISK_BASE_MISSING',
        hint: '在 fcdev.json 里用 diskImage 指向带 DOS 3.3 的 .dsk 镜像'
      });
    }
    return overlayDos33Image(options.base, files);
  }
};

/**
 * 按名称打包程序：校验链接输出并返回最终产物的内容。
 * @param name 打包器名。
 * @param image 链接输出。
 * @param options 打包选项。
 * @param context 目标专用选项与底固件；不给时没有选项也没有底固件。
 * @returns 最终产物的内容。
 * @throws FcdevError 打包器没有登记，或链接输出不符合要求。
 */
export function packageProgram(name: string, image: Uint8Array, options: PackagerOptions, context: PackagerContext = EMPTY_CONTEXT): Uint8Array {
  const packager = PROGRAM_PACKAGERS[name];
  if (packager === undefined) {
    throw new FcdevError(`未知的打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的打包器：${Object.keys(PROGRAM_PACKAGERS).join('、')}` });
  }
  return packager(image, options, context);
}

/**
 * 按名称把一组文件打包成磁盘镜像。
 * @param name 磁盘打包器名。
 * @param files 要放进镜像的文件。
 * @param options 打包选项。
 * @returns 镜像内容。
 * @throws FcdevError 打包器没有登记，文件名不符合 8.3，空间不足，或覆盖打包器没有底盘。
 */
export function packageDisk(name: string, files: readonly DiskFile[], options: DiskPackagerOptions = {}): Uint8Array {
  const packager = DISK_PACKAGERS[name];
  if (packager === undefined) {
    throw new FcdevError(`未知的磁盘打包器：${name}`, { code: 'PACKAGER_UNKNOWN', hint: `可用的磁盘打包器：${Object.keys(DISK_PACKAGERS).join('、')}` });
  }
  return packager(files, options);
}
