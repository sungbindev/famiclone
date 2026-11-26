// ------------------------------------------------------------------------
// 名称：dos33-image.ts
// 说明：在已有的 Apple DOS 3.3 软盘镜像（.dsk，DOS 扇区顺序）上添加文件，返回新镜像；
//       也能列出镜像里的文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：镜像是 35 或 40 个磁道、每磁道 16 个扇区、每扇区 256 字节，扇区在文件里的位置是
//       磁道 * 4096 + 扇区 * 256；磁道 17 的 0 扇区是 VTOC（磁盘空闲位图与目录位置），目录扇区用
//       链表串起来。文件放程序（二进制，类型 B）与其他文件（类型 T，内容原样放入）；同名文件
//       先删除再写入（锁定的同名文件不替换，报错）；
//       文件的数据扇区从磁道 18 往后取，用完再取 16 往前，
//       磁道 0 与目录所在磁道不放文件数据；底盘镜像本身不改。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import type { DiskFile } from './fat12-image';

/** 每个扇区的字节数。 */
const SECTOR_BYTES = 256;

/** 每个磁道的扇区数。 */
const SECTORS_PER_TRACK = 16;

/** 一个磁道的字节数。 */
const TRACK_BYTES = SECTORS_PER_TRACK * SECTOR_BYTES;

/** 允许的磁道数：标准的 35，以及 Ember 也接受的 40。 */
const TRACK_COUNTS: readonly number[] = [35, 40];

/** VTOC 所在的磁道与扇区。 */
const VTOC_TRACK = 17;
const VTOC_SECTOR = 0;

/** VTOC 里的字段偏移。 */
const VTOC_CATALOG_TRACK = 0x01;
const VTOC_CATALOG_SECTOR = 0x02;
const VTOC_TRACKS = 0x34;
const VTOC_SECTORS = 0x35;
const VTOC_SECTOR_BYTES = 0x36;
const VTOC_BITMAP = 0x38;

/** 目录扇区里：下一个目录扇区的位置、第一个目录项的偏移、目录项大小与每扇区的项数。 */
const CATALOG_NEXT = 0x01;
const CATALOG_FIRST_ENTRY = 0x0b;
const CATALOG_ENTRY_BYTES = 35;
const CATALOG_ENTRIES = 7;

/** 目录项里的字段偏移。 */
const ENTRY_LIST_TRACK = 0;
const ENTRY_LIST_SECTOR = 1;
const ENTRY_TYPE = 2;
const ENTRY_NAME = 3;
const ENTRY_LENGTH = 33;

/** 文件名占 30 个字节，不足的补高位为 1 的空格。 */
const NAME_BYTES = 30;
const NAME_PADDING = 0xa0;

/** 目录项第一个字节：0 表示从未使用，0xFF 表示已删除。 */
const ENTRY_UNUSED = 0x00;
const ENTRY_DELETED = 0xff;

/**
 * 轨道扇区列表：下一个列表的位置、本列表第一个数据扇区在文件里的序号、
 * 第一对的偏移与最多的对数。
 */
const LIST_NEXT = 0x01;
const LIST_OFFSET = 0x05;
const LIST_FIRST_PAIR = 0x0c;
const LIST_PAIRS = 122;

/** 文件类型码；位 7 为 1 表示文件被锁定。 */
const TYPE_TEXT = 0x00;
const TYPE_BINARY = 0x04;
const TYPE_LOCKED = 0x80;

/** 文件名允许的字符（转成大写之后）：可见的 ASCII，第一个字符不能是空格，另外不能有逗号。 */
const NAME_PATTERN = /^[\x21-\x5f][\x20-\x5f]{0,29}$/;

/** 目录扇区链的最大长度，防止镜像里的链表成环。 */
const MAX_CATALOG_SECTORS = 64;

/** 镜像里的一个文件。 */
export interface Dos33Entry {
  /** 文件名，已去掉高位与末尾的空格。 */
  readonly name: string;
  /** 类型码：0 是文本，4 是二进制。 */
  readonly type: number;
  /** 占用的扇区数（含轨道扇区列表）。 */
  readonly sectors: number;
}

/** 磁盘的位置：磁道与扇区。 */
interface Location {
  readonly track: number;
  readonly sector: number;
}

/**
 * 抛出底盘镜像不可用的错误。
 * @param reason 原因。
 * @returns 不会返回。
 */
function invalidBase(reason: string): never {
  throw new FcdevError(`底盘镜像不是可用的 DOS 3.3 软盘镜像：${reason}`, {
    code: 'DISK_BASE_INVALID',
    hint: '在 fcdev.json 的 diskImage 里指向 DOS 3.3 格式、DOS 扇区顺序的 .dsk 镜像（143360 字节）'
  });
}

/**
 * 磁道与扇区在镜像里的偏移。
 * @param track 磁道。
 * @param sector 扇区。
 * @returns 字节偏移。
 */
function offsetOf(track: number, sector: number): number {
  return track * TRACK_BYTES + sector * SECTOR_BYTES;
}

/**
 * 检查镜像是 DOS 3.3 格式，返回磁道数。
 * @param image 镜像。
 * @returns 磁道数。
 * @throws FcdevError 大小不对，或 VTOC 的参数不合理。
 */
function checkImage(image: Buffer): number {
  const tracks = image.length / TRACK_BYTES;
  if (!TRACK_COUNTS.includes(tracks)) return invalidBase(`大小是 ${image.length} 字节，应该是 35 或 40 个磁道`);
  const vtoc = offsetOf(VTOC_TRACK, VTOC_SECTOR);
  if (image.readUInt8(vtoc + VTOC_TRACKS) !== tracks || image.readUInt8(vtoc + VTOC_SECTORS) !== SECTORS_PER_TRACK || image.readUInt16LE(vtoc + VTOC_SECTOR_BYTES) !== SECTOR_BYTES) {
    return invalidBase('VTOC 里的磁道数、扇区数或扇区大小不对');
  }
  const catalogTrack = image.readUInt8(vtoc + VTOC_CATALOG_TRACK);
  const catalogSector = image.readUInt8(vtoc + VTOC_CATALOG_SECTOR);
  if (catalogTrack >= tracks || catalogSector >= SECTORS_PER_TRACK) return invalidBase('VTOC 里的目录位置不对');
  return tracks;
}

/**
 * 空闲位图里一个扇区所在的字节与位。
 * @param track 磁道。
 * @param sector 扇区。
 * @returns 相对 VTOC 起点的字节偏移与掩码。
 */
function bitmapPosition(track: number, sector: number): { readonly at: number; readonly mask: number } {
  // 每个磁道 4 个字节，只用前两个：第一个是扇区 15 到 8（位 7 是 15），第二个是扇区 7 到 0
  return { at: VTOC_BITMAP + track * 4 + (sector >= 8 ? 0 : 1), mask: 1 << (sector % 8) };
}

/**
 * 读空闲位图：一个扇区是否空闲。
 * @param image 镜像。
 * @param track 磁道。
 * @param sector 扇区。
 * @returns 空闲为 true。
 */
function isFree(image: Buffer, track: number, sector: number): boolean {
  const { at, mask } = bitmapPosition(track, sector);
  return (image.readUInt8(offsetOf(VTOC_TRACK, VTOC_SECTOR) + at) & mask) !== 0;
}

/**
 * 写空闲位图。
 * @param image 镜像。
 * @param location 磁道与扇区。
 * @param free 标成空闲为 true，标成占用为 false。
 */
function setFree(image: Buffer, location: Location, free: boolean): void {
  const { at, mask } = bitmapPosition(location.track, location.sector);
  const index = offsetOf(VTOC_TRACK, VTOC_SECTOR) + at;
  image.writeUInt8(free ? image.readUInt8(index) | mask : image.readUInt8(index) & ~mask & 0xff, index);
}

/**
 * 依次给出目录里全部目录项的位置（含空项与已删除的项）。
 * @param image 镜像。
 * @param tracks 磁道数。
 * @returns 目录项在镜像里的偏移列表。
 * @throws FcdevError 目录扇区的链表出错。
 */
function catalogEntryOffsets(image: Buffer, tracks: number): number[] {
  const vtoc = offsetOf(VTOC_TRACK, VTOC_SECTOR);
  let track = image.readUInt8(vtoc + VTOC_CATALOG_TRACK);
  let sector = image.readUInt8(vtoc + VTOC_CATALOG_SECTOR);
  const offsets: number[] = [];
  for (let count = 0; track !== 0 || sector !== 0; count += 1) {
    if (count >= MAX_CATALOG_SECTORS || track >= tracks || sector >= SECTORS_PER_TRACK) return invalidBase('目录扇区的链表不对');
    const base = offsetOf(track, sector);
    for (let index = 0; index < CATALOG_ENTRIES; index += 1) offsets.push(base + CATALOG_FIRST_ENTRY + index * CATALOG_ENTRY_BYTES);
    track = image.readUInt8(base + CATALOG_NEXT);
    sector = image.readUInt8(base + CATALOG_NEXT + 1);
  }
  return offsets;
}

/**
 * 读出目录项里的文件名。
 * @param image 镜像。
 * @param entry 目录项的偏移。
 * @returns 去掉高位与末尾空格的文件名。
 */
function readName(image: Buffer, entry: number): string {
  let name = '';
  for (let index = 0; index < NAME_BYTES; index += 1) name += String.fromCharCode(image.readUInt8(entry + ENTRY_NAME + index) & 0x7f);
  return name.trimEnd();
}

/**
 * 释放一个文件占用的全部扇区（数据扇区与轨道扇区列表）。
 * @param image 镜像。
 * @param entry 目录项的偏移。
 * @param tracks 磁道数。
 * @throws FcdevError 轨道扇区列表的链表出错。
 */
function freeFile(image: Buffer, entry: number, tracks: number): void {
  let list: Location = { track: image.readUInt8(entry + ENTRY_LIST_TRACK), sector: image.readUInt8(entry + ENTRY_LIST_SECTOR) };
  for (let count = 0; list.track !== 0 || list.sector !== 0; count += 1) {
    if (count > tracks * SECTORS_PER_TRACK || list.track >= tracks || list.sector >= SECTORS_PER_TRACK) return invalidBase('文件的轨道扇区列表不对');
    const base = offsetOf(list.track, list.sector);
    for (let pair = 0; pair < LIST_PAIRS; pair += 1) {
      const track = image.readUInt8(base + LIST_FIRST_PAIR + pair * 2);
      const sector = image.readUInt8(base + LIST_FIRST_PAIR + pair * 2 + 1);
      if (track !== 0 && track < tracks && sector < SECTORS_PER_TRACK) setFree(image, { track, sector }, true);
    }
    setFree(image, list, true);
    list = { track: image.readUInt8(base + LIST_NEXT), sector: image.readUInt8(base + LIST_NEXT + 1) };
  }
}

/**
 * 取出 count 个空闲扇区并在位图里标成占用。
 * @param image 镜像。
 * @param count 个数。
 * @param tracks 磁道数。
 * @returns 取到的扇区；不够时为 undefined（位图保持原样）。
 */
function takeFreeSectors(image: Buffer, count: number, tracks: number): Location[] | undefined {
  const order: number[] = [];
  for (let track = VTOC_TRACK + 1; track < tracks; track += 1) order.push(track);
  for (let track = VTOC_TRACK - 1; track >= 1; track -= 1) order.push(track);
  const found: Location[] = [];
  for (const track of order) {
    for (let sector = SECTORS_PER_TRACK - 1; sector >= 0 && found.length < count; sector -= 1) {
      if (isFree(image, track, sector)) found.push({ track, sector });
    }
    if (found.length === count) break;
  }
  if (found.length < count) return undefined;
  for (const location of found) setFree(image, location, false);
  return found;
}

/**
 * 把文件名转成目录项里的 30 个字节。
 * @param name 文件名。
 * @returns 大写、高位为 1、不足补空格的 30 个字节。
 * @throws FcdevError 文件名不符合 DOS 3.3 的规则。
 */
function encodeName(name: string): Buffer {
  const upper = name.toUpperCase();
  if (!NAME_PATTERN.test(upper) || upper.includes(',')) {
    throw new FcdevError(`文件名“${name}”不能放进 DOS 3.3 镜像：1 到 30 个可见的 ASCII 字符，不含逗号，不能以空格开头`, {
      code: 'DISK_NAME_INVALID',
      hint: '改名后重新构建'
    });
  }
  const bytes = Buffer.alloc(NAME_BYTES, NAME_PADDING);
  for (let index = 0; index < upper.length; index += 1) bytes.writeUInt8(upper.charCodeAt(index) | 0x80, index);
  return bytes;
}

/**
 * 在目录里找同名文件与第一个空位。
 * @param image 镜像。
 * @param offsets 全部目录项的偏移。
 * @param encoded 编码后的文件名。
 * @returns 同名文件的目录项偏移与第一个空位的偏移，找不到时对应项为 undefined。
 */
function findEntries(image: Buffer, offsets: readonly number[], encoded: Buffer): { readonly existing: number | undefined; readonly vacant: number | undefined } {
  let existing: number | undefined;
  let vacant: number | undefined;
  for (const entry of offsets) {
    const first = image.readUInt8(entry + ENTRY_LIST_TRACK);
    if (first === ENTRY_UNUSED || first === ENTRY_DELETED) {
      vacant ??= entry;
      continue;
    }
    if (image.subarray(entry + ENTRY_NAME, entry + ENTRY_NAME + NAME_BYTES).equals(encoded)) existing = entry;
  }
  return { existing, vacant };
}

/**
 * 把一个文件写进镜像。
 * @param image 镜像。
 * @param tracks 磁道数。
 * @param file 文件。
 * @throws FcdevError 文件名不合规，目录满，或空间不足。
 */
function addFile(image: Buffer, tracks: number, file: DiskFile): void {
  const encoded = encodeName(file.name);
  const { existing, vacant } = findEntries(image, catalogEntryOffsets(image, tracks), encoded);
  if (existing !== undefined && (image.readUInt8(existing + ENTRY_TYPE) & TYPE_LOCKED) !== 0) {
    throw new FcdevError(`底盘镜像里已有同名的锁定文件，不能替换：${file.name}`, {
      code: 'DISK_FILE_LOCKED',
      hint: '锁定的文件多半是系统盘的启动程序（如 HELLO）；在 fcdev.json 里用 name 换一个程序名'
    });
  }
  if (existing !== undefined) freeFile(image, existing, tracks);
  const entry = existing ?? vacant;
  if (entry === undefined) throw new FcdevError(`底盘镜像的目录已满，放不下：${file.name}`, { code: 'DISK_FULL' });

  const data = Buffer.from(file.data);
  const dataSectors = Math.ceil(data.length / SECTOR_BYTES);
  const listCount = Math.max(1, Math.ceil(dataSectors / LIST_PAIRS));
  const sectors = takeFreeSectors(image, listCount + dataSectors, tracks);
  if (sectors === undefined) {
    // 同名文件的旧内容已经在副本里释放，抛错后整个副本都不会使用
    throw new FcdevError(`底盘镜像的剩余空间不足，放不下：${file.name}`, { code: 'DISK_FULL', hint: '换一个空间更大的底盘镜像，或删掉 disk 目录里不需要的文件' });
  }
  const lists = sectors.slice(0, listCount);
  const blocks = sectors.slice(listCount);
  lists.forEach((list, index) => {
    const base = offsetOf(list.track, list.sector);
    image.fill(0, base, base + SECTOR_BYTES);
    const next = lists[index + 1];
    if (next !== undefined) {
      image.writeUInt8(next.track, base + LIST_NEXT);
      image.writeUInt8(next.sector, base + LIST_NEXT + 1);
    }
    image.writeUInt16LE(index * LIST_PAIRS, base + LIST_OFFSET);
    blocks.slice(index * LIST_PAIRS, (index + 1) * LIST_PAIRS).forEach((block, pair) => {
      image.writeUInt8(block.track, base + LIST_FIRST_PAIR + pair * 2);
      image.writeUInt8(block.sector, base + LIST_FIRST_PAIR + pair * 2 + 1);
    });
  });
  blocks.forEach((block, index) => {
    const base = offsetOf(block.track, block.sector);
    image.fill(0, base, base + SECTOR_BYTES);
    data.copy(image, base, index * SECTOR_BYTES, (index + 1) * SECTOR_BYTES);
  });

  image.fill(0, entry, entry + CATALOG_ENTRY_BYTES);
  image.writeUInt8(lists[0]?.track ?? 0, entry + ENTRY_LIST_TRACK);
  image.writeUInt8(lists[0]?.sector ?? 0, entry + ENTRY_LIST_SECTOR);
  image.writeUInt8(file.program === true ? TYPE_BINARY : TYPE_TEXT, entry + ENTRY_TYPE);
  encoded.copy(image, entry + ENTRY_NAME);
  image.writeUInt16LE(sectors.length, entry + ENTRY_LENGTH);
}

/**
 * 在底盘镜像上添加文件。
 * @param base 底盘镜像，不会被修改。
 * @param files 要添加的文件；与底盘里同名的文件被替换，标了 program 的是二进制（类型 B），
 *   其余是文本（类型 T）。
 * @returns 新镜像。
 * @throws FcdevError 底盘不是 DOS 3.3 镜像，文件名不合规，或目录、空间不够。
 */
export function overlayDos33Image(base: Uint8Array, files: readonly DiskFile[]): Buffer {
  const image = Buffer.from(base);
  const tracks = checkImage(image);
  for (const file of files) addFile(image, tracks, file);
  return image;
}

/**
 * 列出镜像里的文件，按目录顺序。
 * @param image DOS 3.3 镜像。
 * @returns 文件列表。
 * @throws FcdevError 镜像不是 DOS 3.3 格式。
 */
export function listDos33(image: Uint8Array): Dos33Entry[] {
  const buffer = Buffer.from(image);
  const tracks = checkImage(buffer);
  const entries: Dos33Entry[] = [];
  for (const entry of catalogEntryOffsets(buffer, tracks)) {
    const first = buffer.readUInt8(entry + ENTRY_LIST_TRACK);
    if (first === ENTRY_UNUSED || first === ENTRY_DELETED) continue;
    entries.push({ name: readName(buffer, entry), type: buffer.readUInt8(entry + ENTRY_TYPE) & 0x7f, sectors: buffer.readUInt16LE(entry + ENTRY_LENGTH) });
  }
  return entries;
}

/**
 * 读出镜像里一个文件的内容（按扇区读出，末尾可能带补齐的 0；二进制文件按文件头的长度截取）。
 * @param image DOS 3.3 镜像。
 * @param name 文件名。
 * @returns 文件内容；没有这个文件时为 undefined。
 * @throws FcdevError 镜像不是 DOS 3.3 格式，或文件的轨道扇区列表出错。
 */
export function readDos33File(image: Uint8Array, name: string): Buffer | undefined {
  const buffer = Buffer.from(image);
  const tracks = checkImage(buffer);
  const encoded = encodeName(name);
  const { existing } = findEntries(buffer, catalogEntryOffsets(buffer, tracks), encoded);
  if (existing === undefined) return undefined;
  const chunks: Buffer[] = [];
  let list: Location = { track: buffer.readUInt8(existing + ENTRY_LIST_TRACK), sector: buffer.readUInt8(existing + ENTRY_LIST_SECTOR) };
  for (let count = 0; list.track !== 0 || list.sector !== 0; count += 1) {
    if (count > tracks * SECTORS_PER_TRACK || list.track >= tracks || list.sector >= SECTORS_PER_TRACK) return invalidBase('文件的轨道扇区列表不对');
    const base = offsetOf(list.track, list.sector);
    for (let pair = 0; pair < LIST_PAIRS; pair += 1) {
      const track = buffer.readUInt8(base + LIST_FIRST_PAIR + pair * 2);
      const sector = buffer.readUInt8(base + LIST_FIRST_PAIR + pair * 2 + 1);
      if (track === 0 && sector === 0) continue;
      chunks.push(buffer.subarray(offsetOf(track, sector), offsetOf(track, sector) + SECTOR_BYTES));
    }
    list = { track: buffer.readUInt8(base + LIST_NEXT), sector: buffer.readUInt8(base + LIST_NEXT + 1) };
  }
  const content = Buffer.concat(chunks);
  if ((buffer.readUInt8(existing + ENTRY_TYPE) & 0x7f) !== TYPE_BINARY || content.length < 4) return content;
  return content.subarray(0, 4 + content.readUInt16LE(2));
}
