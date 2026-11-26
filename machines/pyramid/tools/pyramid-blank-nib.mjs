/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-blank-nib.mjs
 * 说明：直接生成空白的金字塔 PEC-9588 .nib 软盘镜像：35 道 × 每道 6656 半字节，
 *       共 232960 字节（.nib 的标准尺寸，BMOW Floppy Emu 只认 232960 与 366080
 *       两种大小）；每道按金字塔盘片格式编出 14 个扇区（地址场、数据场齐全，
 *       数据全零），DOS 信息区未初始化，放入虚拟软驱后在机器上用 INIT 初始化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-27
 * 备注：编码与模拟器核心 logic/hardware/disk2/disk2-codec 的 encodeTrack（PYRAMID_FORMAT）一致：
 *       6 加 2 半字节编码，地址场 D5 AA 96、4 加 4 编码的卷号/磁道/扇区/校验、收尾 DE AA EB；
 *       数据场 D5 AA AD、343 个半字节（86 辅助 + 256 主体 + 1 校验）、收尾 DE AA EB；
 *       同步字节 FF 后跟两个 0 位；一圈 50000 位，道首 128、地址场后 6、数据场后 50 个同步字节；
 *       交错表是 ROM $BEB8 的逻辑到物理对应表；
 *       一圈编完去掉同步字节的两个 0 位得到半字节序列，不足 6656 个用 FF 补齐。
 *       用法：node machines/pyramid/tools/pyramid-blank-nib.mjs [输出.nib]，
 *       不给输出时写到 reference/floppy-images/pec9588/blank.nib。
 * ------------------------------------------------------------------------
 */

import { writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

/** 盘片格式：与模拟器核心 PYRAMID_FORMAT 一致；.nib 只放标准的 35 道（232960 字节）。 */
const FORMAT = {
  trackCount: 35,
  sectorsPerTrack: 14,
  logicalToPhysical: [0, 11, 9, 7, 5, 3, 1, 12, 10, 8, 6, 4, 2, 13],
  volume: 254,
  trackBits: 50000,
  gapTrackStart: 128,
  gapAfterAddress: 6,
  gapAfterData: 50
};

/** 扇区大小与 .nib 每道的半字节数。 */
const SECTOR_SIZE = 256;
const NIBBLE_TRACK_BYTES = 6656;
/** 数据场里辅助半字节的个数。 */
const AUX_COUNT = 86;
/** 6 位值到磁盘半字节的编码表（与模拟器核心 ENCODE_TABLE 一致）。 */
const ENCODE_TABLE = [
  0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6, 0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
  0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
  0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
  0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
];
const ADDRESS_MARK = [0xd5, 0xaa, 0x96];
const DATA_MARK = [0xd5, 0xaa, 0xad];
const TRAILER = [0xde, 0xaa, 0xeb];

/** 往位序列里追加内容的写入器（与模拟器核心 BitWriter 一致）。 */
class BitWriter {
  constructor() {
    this.bits = [];
  }

  /** 一个半字节：8 位，高位在前。 */
  nibble(value) {
    for (let bit = 7; bit >= 0; bit -= 1) this.bits.push((value >> bit) & 1);
  }

  marks(bytes) {
    for (const value of bytes) this.nibble(value);
  }

  /** 同步字节：FF 后面跟两个 0 位。 */
  sync(count) {
    for (let index = 0; index < count; index += 1) {
      this.nibble(0xff);
      this.bits.push(0, 0);
    }
  }

  /** 4 加 4 编码：一个字节拆成两个半字节，奇数位在前。 */
  fourAndFour(value) {
    this.nibble((value >> 1) | 0xaa);
    this.nibble(value | 0xaa);
  }

  /** 一个扇区的数据场：起始标记、343 个半字节、收尾标记。 */
  dataField(data) {
    const aux = Array.from({ length: AUX_COUNT }, () => 0);
    const body = Array.from({ length: SECTOR_SIZE }, () => 0);
    for (let step = 0; step < 3 * AUX_COUNT; step += 1) {
      const index = (1 - step) & 0xff;
      const byte = data[index];
      const slot = step % AUX_COUNT;
      aux[slot] = (aux[slot] << 2) | ((byte & 1) << 1) | ((byte >> 1) & 1);
      body[index] = byte >> 2;
    }
    for (let index = 0; index < AUX_COUNT; index += 1) aux[index] &= 0x3f;
    this.marks(DATA_MARK);
    let previous = 0;
    for (let index = AUX_COUNT - 1; index >= 0; index -= 1) {
      this.nibble(ENCODE_TABLE[aux[index] ^ previous]);
      previous = aux[index];
    }
    for (let index = 0; index < SECTOR_SIZE; index += 1) {
      this.nibble(ENCODE_TABLE[body[index] ^ previous]);
      previous = body[index];
    }
    this.nibble(ENCODE_TABLE[previous]);
    this.marks(TRAILER);
  }
}

/** 把一个磁道编成一圈位（每个元素是 0 或 1，长度 FORMAT.trackBits）。 */
function encodeTrack(track, sectors) {
  const toLogical = Array.from({ length: FORMAT.sectorsPerTrack }, () => 0);
  for (let logical = 0; logical < FORMAT.sectorsPerTrack; logical += 1) toLogical[FORMAT.logicalToPhysical[logical]] = logical;
  const writer = new BitWriter();
  writer.sync(FORMAT.gapTrackStart);
  for (let physical = 0; physical < FORMAT.sectorsPerTrack; physical += 1) {
    writer.marks(ADDRESS_MARK);
    writer.fourAndFour(FORMAT.volume);
    writer.fourAndFour(track);
    writer.fourAndFour(physical);
    writer.fourAndFour(FORMAT.volume ^ track ^ physical);
    writer.marks(TRAILER);
    writer.sync(FORMAT.gapAfterAddress);
    writer.dataField(sectors.subarray(toLogical[physical] * SECTOR_SIZE, (toLogical[physical] + 1) * SECTOR_SIZE));
    writer.sync(FORMAT.gapAfterData);
  }
  // 剩下的位补成同步字节的样子（FF 加两个 0 位，循环）。
  for (let index = writer.bits.length; index < FORMAT.trackBits; index += 1) writer.bits.push(index % 10 < 8 ? 1 : 0);
  writer.bits.length = FORMAT.trackBits;
  return writer.bits;
}

/** 位序列换成半字节：从第一个 1 位起每 8 位一个，最多 count 个，不够的用 FF 补齐。 */
function bitsToNibbles(bits, count) {
  const nibbles = [];
  let shift = 0;
  for (let index = 0; index < bits.length && nibbles.length < count; index += 1) {
    const bit = bits[index] !== 0 ? 1 : 0;
    if (shift === 0 && bit === 0) continue;
    shift = ((shift << 1) | bit) & 0xff;
    if ((shift & 0x80) !== 0) {
      nibbles.push(shift);
      shift = 0;
    }
  }
  while (nibbles.length < count) nibbles.push(0xff);
  return nibbles;
}

/** 生成空白的金字塔 .nib 镜像：35 道，每道 14 个数据全零的扇区。 */
export function blankNib() {
  const result = new Uint8Array(FORMAT.trackCount * NIBBLE_TRACK_BYTES);
  const sectors = new Uint8Array(FORMAT.sectorsPerTrack * SECTOR_SIZE);
  for (let track = 0; track < FORMAT.trackCount; track += 1) {
    result.set(bitsToNibbles(encodeTrack(track, sectors), NIBBLE_TRACK_BYTES), track * NIBBLE_TRACK_BYTES);
  }
  return result;
}

// 直接运行时写出文件；被引用时只导出函数。
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const here = dirname(fileURLToPath(import.meta.url));
  const output = process.argv[2] ?? join(here, '..', '..', '..', 'reference', 'floppy-images', 'pec9588', 'blank.nib');
  const nib = blankNib();
  writeFileSync(output, nib);
  console.log(`已生成 ${output}（${nib.length} 字节，35 道 × 14 扇区，数据全零，需 INIT）`);
}