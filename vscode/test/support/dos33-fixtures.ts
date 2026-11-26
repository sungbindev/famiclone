// ------------------------------------------------------------------------
// 名称：dos33-fixtures.ts
// 说明：测试用的 DOS 3.3 空白系统盘：35 个磁道、每磁道 16 个扇区，VTOC 与目录扇区链都已写好。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：磁道 0 到 2（DOS 映像所在）与磁道 17（VTOC 与目录）标为已占用，其余全部空闲；
//       目录从扇区 15 往 1 串成链，目录里没有文件。
// ------------------------------------------------------------------------

/** 磁道数与每磁道的扇区数。 */
const TRACKS = 35;
const SECTORS = 16;

/** 一个磁道的字节数。 */
const TRACK_BYTES = SECTORS * 256;

/** 目录所在的磁道与第一个目录扇区。 */
const CATALOG_TRACK = 17;
const FIRST_CATALOG_SECTOR = 15;

/**
 * 合成空白的 DOS 3.3 系统盘。
 * @param options.usedTracks 额外标成已占用的磁道（默认没有），用来凑出“空间不足”。
 * @returns 143360 字节的镜像。
 */
export function createBlankDos33Image(options: { readonly usedTracks?: readonly number[] } = {}): Buffer {
  const image = Buffer.alloc(TRACKS * TRACK_BYTES);
  const vtoc = CATALOG_TRACK * TRACK_BYTES;
  image.writeUInt8(0x04, vtoc);
  image.writeUInt8(CATALOG_TRACK, vtoc + 0x01);
  image.writeUInt8(FIRST_CATALOG_SECTOR, vtoc + 0x02);
  image.writeUInt8(3, vtoc + 0x03);
  image.writeUInt8(254, vtoc + 0x06);
  image.writeUInt8(122, vtoc + 0x27);
  image.writeUInt8(CATALOG_TRACK, vtoc + 0x30);
  image.writeUInt8(1, vtoc + 0x31);
  image.writeUInt8(TRACKS, vtoc + 0x34);
  image.writeUInt8(SECTORS, vtoc + 0x35);
  image.writeUInt16LE(256, vtoc + 0x36);
  const used = new Set([0, 1, 2, CATALOG_TRACK, ...(options.usedTracks ?? [])]);
  for (let track = 0; track < TRACKS; track += 1) {
    if (!used.has(track)) image.writeUInt16BE(0xffff, vtoc + 0x38 + track * 4);
  }
  for (let sector = FIRST_CATALOG_SECTOR; sector >= 1; sector -= 1) {
    const at = vtoc + sector * 256;
    if (sector > 1) {
      image.writeUInt8(CATALOG_TRACK, at + 0x01);
      image.writeUInt8(sector - 1, at + 0x02);
    }
  }
  return image;
}

/**
 * 空闲位图里有多少个扇区空闲。
 * @param image DOS 3.3 镜像。
 * @returns 空闲扇区数。
 */
export function countFreeSectors(image: Buffer): number {
  const vtoc = CATALOG_TRACK * TRACK_BYTES;
  let free = 0;
  for (let track = 0; track < TRACKS; track += 1) {
    for (const value of [image.readUInt8(vtoc + 0x38 + track * 4), image.readUInt8(vtoc + 0x38 + track * 4 + 1)]) {
      for (let bit = 0; bit < 8; bit += 1) if ((value & (1 << bit)) !== 0) free += 1;
    }
  }
  return free;
}
