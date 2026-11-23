// ------------------------------------------------------------------------
// 名称：fat12-image.ts
// 说明：构建与读取 1.44MB 的 FAT12 软盘镜像（512 字节扇区、2 面、80 磁道、每磁道 18 扇区）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：几何参数与模拟器的软驱控制器一致（LBA = 磁道*36 + 磁头*18 + 扇区-1，共 2880 个扇区）。
//       文件只写在根目录，每簇 1 个扇区，不支持子目录。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';

/** 1.44MB 软盘的固定参数。 */
export const GEOMETRY = Object.freeze({
  bytesPerSector: 512,
  sectorsPerCluster: 1,
  reservedSectors: 1,
  fatCount: 2,
  rootEntries: 224,
  totalSectors: 2880,
  mediaDescriptor: 0xf0,
  sectorsPerFat: 9,
  sectorsPerTrack: 18,
  heads: 2
});

/** 要写进镜像根目录的文件。 */
export interface DiskFile {
  /** 8.3 文件名，如 EDIT.EXE。 */
  readonly name: string;
  /** 文件内容。 */
  readonly data: Uint8Array;
  /** 修改时间；不给时用 1998-01-01。 */
  readonly mtime?: Date;
}

/** 镜像根目录里的一个文件。 */
export interface Fat12Entry {
  /** NAME.EXT 形式的文件名。 */
  readonly name: string;
  /** 文件字节数。 */
  readonly size: number;
  /** 起始簇号。 */
  readonly startCluster: number;
}

/** 目录项大小。 */
export const ENTRY_SIZE = 32;

/** FAT12 的簇链结束标记。 */
export const END_OF_CHAIN = 0xfff;

/** 卷标属性位。 */
export const ATTR_VOLUME = 0x08;

/** 8.3 名字里允许的字符。 */
const NAME_PART = /^[A-Z0-9_$~!#%&\-{}()@'`^]+$/;

/**
 * 读取 Buffer 的一个字节；越界时为 0。
 * @param buffer 缓冲区。
 * @param index 下标。
 * @returns 字节值。
 */
function byteAt(buffer: Uint8Array, index: number): number {
  return buffer[index] ?? 0;
}

/**
 * 把文件名转成 11 字节的 8.3 目录项名字。
 * @param fileName 例如 "edit.exe"。
 * @returns 11 个字符，不足补空格。
 * @throws FcdevError 文件名不符合 8.3 规则。
 */
export function toShortName(fileName: string): string {
  const upper = fileName.toUpperCase();
  const dot = upper.lastIndexOf('.');
  const base = dot < 0 ? upper : upper.slice(0, dot);
  const ext = dot < 0 ? '' : upper.slice(dot + 1);
  if (base.length < 1 || base.length > 8 || ext.length > 3 || !NAME_PART.test(base) || (ext !== '' && !NAME_PART.test(ext))) {
    throw new FcdevError(`文件名不符合 8.3 规则：${fileName}`, { code: 'DISK_NAME_INVALID' });
  }
  return base.padEnd(8, ' ') + ext.padEnd(3, ' ');
}

/**
 * 把 11 字节目录项名字还原成 NAME.EXT。
 * @param raw 11 个字符。
 * @returns 文件名。
 */
function fromShortName(raw: string): string {
  const base = raw.slice(0, 8).trimEnd();
  const ext = raw.slice(8, 11).trimEnd();
  return ext === '' ? base : `${base}.${ext}`;
}

/**
 * 编码 DOS 日期与时间。
 * @param date 日期时间。
 * @returns 目录项里的日期与时间字段。
 */
export function dosDateTime(date: Date): { date: number; time: number } {
  const year = Math.min(Math.max(date.getFullYear(), 1980), 2107);
  return {
    date: ((year - 1980) << 9) | ((date.getMonth() + 1) << 5) | date.getDate(),
    time: (date.getHours() << 11) | (date.getMinutes() << 5) | (date.getSeconds() >> 1)
  };
}

/**
 * 写入 FAT12 的一个 12 位条目。
 * @param fat FAT 表。
 * @param cluster 簇号。
 * @param value 条目值。
 */
export function setFatEntry(fat: Buffer, cluster: number, value: number): void {
  const offset = Math.floor((cluster * 3) / 2);
  if (cluster % 2 === 0) {
    fat[offset] = value & 0xff;
    fat[offset + 1] = (byteAt(fat, offset + 1) & 0xf0) | ((value >> 8) & 0x0f);
  } else {
    fat[offset] = (byteAt(fat, offset) & 0x0f) | ((value & 0x0f) << 4);
    fat[offset + 1] = (value >> 4) & 0xff;
  }
}

/**
 * 读取 FAT12 的一个 12 位条目。
 * @param fat FAT 表。
 * @param cluster 簇号。
 * @returns 条目值。
 */
export function getFatEntry(fat: Uint8Array, cluster: number): number {
  const offset = Math.floor((cluster * 3) / 2);
  const pair = byteAt(fat, offset) | (byteAt(fat, offset + 1) << 8);
  return cluster % 2 === 0 ? pair & 0x0fff : pair >> 4;
}

/**
 * 生成引导扇区（含 BIOS 参数块与 55 AA 签名，不含启动代码）。
 * @param label 卷标。
 * @returns 512 字节。
 */
function buildBootSector(label: string | undefined): Buffer {
  const g = GEOMETRY;
  const sector = Buffer.alloc(g.bytesPerSector);
  sector.set([0xeb, 0x3c, 0x90], 0);
  sector.write('FCDEV   ', 3, 'ascii');
  sector.writeUInt16LE(g.bytesPerSector, 11);
  sector.writeUInt8(g.sectorsPerCluster, 13);
  sector.writeUInt16LE(g.reservedSectors, 14);
  sector.writeUInt8(g.fatCount, 16);
  sector.writeUInt16LE(g.rootEntries, 17);
  sector.writeUInt16LE(g.totalSectors, 19);
  sector.writeUInt8(g.mediaDescriptor, 21);
  sector.writeUInt16LE(g.sectorsPerFat, 22);
  sector.writeUInt16LE(g.sectorsPerTrack, 24);
  sector.writeUInt16LE(g.heads, 26);
  sector.writeUInt8(0x29, 38);
  sector.writeUInt32LE(0x20261110, 39);
  sector.write((label ?? 'NO NAME').toUpperCase().padEnd(11, ' ').slice(0, 11), 43, 'ascii');
  sector.write('FAT12   ', 54, 'ascii');
  sector.writeUInt16LE(0xaa55, 510);
  return sector;
}

/**
 * 构建 FAT12 软盘镜像。
 * @param files 要写入根目录的文件。
 * @param options 卷标。
 * @returns 1474560 字节的镜像。
 * @throws FcdevError 文件名不符合 8.3、空间不足或根目录项过多。
 */
export function buildFat12Image(files: readonly DiskFile[], options: { readonly label?: string | undefined } = {}): Buffer {
  const g = GEOMETRY;
  const image = Buffer.alloc(g.totalSectors * g.bytesPerSector);
  const fatBytes = g.sectorsPerFat * g.bytesPerSector;
  const rootSectors = (g.rootEntries * ENTRY_SIZE) / g.bytesPerSector;
  const fatStart = g.reservedSectors * g.bytesPerSector;
  const rootStart = fatStart + g.fatCount * fatBytes;
  const dataStart = rootStart + rootSectors * g.bytesPerSector;
  const clusterBytes = g.sectorsPerCluster * g.bytesPerSector;
  const clusterCount = Math.floor((image.length - dataStart) / clusterBytes);

  buildBootSector(options.label).copy(image, 0);

  const fat = Buffer.alloc(fatBytes);
  setFatEntry(fat, 0, 0xf00 | g.mediaDescriptor);
  setFatEntry(fat, 1, 0xfff);

  const entries: Buffer[] = [];
  if (options.label !== undefined && options.label !== '') {
    const labelEntry = Buffer.alloc(ENTRY_SIZE);
    labelEntry.write(options.label.toUpperCase().padEnd(11, ' ').slice(0, 11), 0, 'ascii');
    labelEntry.writeUInt8(ATTR_VOLUME, 11);
    entries.push(labelEntry);
  }

  let nextCluster = 2;
  for (const file of files) {
    const data = Buffer.from(file.data);
    const need = Math.max(1, Math.ceil(data.length / clusterBytes));
    if (data.length > 0 && nextCluster - 2 + need > clusterCount) {
      throw new FcdevError(`软盘空间不足，放不下：${file.name}`, { code: 'DISK_FULL' });
    }
    const entry = Buffer.alloc(ENTRY_SIZE);
    entry.write(toShortName(file.name), 0, 'ascii');
    entry.writeUInt8(0x20, 11);
    const stamp = dosDateTime(file.mtime ?? new Date(1998, 0, 1));
    entry.writeUInt16LE(stamp.time, 22);
    entry.writeUInt16LE(stamp.date, 24);
    entry.writeUInt32LE(data.length, 28);

    if (data.length > 0) {
      entry.writeUInt16LE(nextCluster, 26);
      for (let index = 0; index < need; index += 1) {
        const cluster = nextCluster + index;
        setFatEntry(fat, cluster, index === need - 1 ? END_OF_CHAIN : cluster + 1);
        data.subarray(index * clusterBytes, (index + 1) * clusterBytes).copy(image, dataStart + (cluster - 2) * clusterBytes);
      }
      nextCluster += need;
    }
    entries.push(entry);
  }

  if (entries.length > g.rootEntries) throw new FcdevError('根目录项过多', { code: 'DISK_FULL' });
  entries.forEach((entry, index) => entry.copy(image, rootStart + index * ENTRY_SIZE));
  for (let copy = 0; copy < g.fatCount; copy += 1) fat.copy(image, fatStart + copy * fatBytes);
  return image;
}

/**
 * 列出镜像根目录里的文件。
 * @param image FAT12 镜像。
 * @returns 文件列表。
 */
export function listFat12(image: Uint8Array): Fat12Entry[] {
  const g = GEOMETRY;
  const buffer = Buffer.from(image.buffer, image.byteOffset, image.byteLength);
  const rootStart = (g.reservedSectors + g.fatCount * g.sectorsPerFat) * g.bytesPerSector;
  const result: Fat12Entry[] = [];
  for (let index = 0; index < g.rootEntries; index += 1) {
    const at = rootStart + index * ENTRY_SIZE;
    const first = byteAt(buffer, at);
    if (first === 0x00) break;
    if (first === 0xe5 || (byteAt(buffer, at + 11) & ATTR_VOLUME) !== 0) continue;
    result.push({
      name: fromShortName(buffer.toString('ascii', at, at + 11)),
      size: buffer.readUInt32LE(at + 28),
      startCluster: buffer.readUInt16LE(at + 26)
    });
  }
  return result;
}

/**
 * 读取镜像中一个文件的内容。
 * @param image FAT12 镜像。
 * @param entry listFat12 返回的目录项。
 * @returns 文件内容。
 */
export function readFat12File(image: Uint8Array, entry: Pick<Fat12Entry, 'size' | 'startCluster'>): Buffer {
  const g = GEOMETRY;
  const fatStart = g.reservedSectors * g.bytesPerSector;
  const fat = image.subarray(fatStart, fatStart + g.sectorsPerFat * g.bytesPerSector);
  const dataStart = (g.reservedSectors + g.fatCount * g.sectorsPerFat) * g.bytesPerSector + g.rootEntries * ENTRY_SIZE;
  const clusterBytes = g.sectorsPerCluster * g.bytesPerSector;
  const chunks: Uint8Array[] = [];
  let cluster = entry.startCluster;
  let remaining = entry.size;
  while (remaining > 0 && cluster >= 2 && cluster < 0xff8) {
    const start = dataStart + (cluster - 2) * clusterBytes;
    chunks.push(image.subarray(start, start + Math.min(clusterBytes, remaining)));
    remaining -= clusterBytes;
    cluster = getFatEntry(fat, cluster);
  }
  return Buffer.concat(chunks);
}
