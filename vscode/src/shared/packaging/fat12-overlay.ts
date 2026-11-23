// ------------------------------------------------------------------------
// 名称：fat12-overlay.ts
// 说明：在已有的 FAT12 软盘镜像（如带系统文件的启动盘）上添加文件，返回新镜像；底盘镜像本身不改。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：几何参数取自底盘的引导扇区，不要求是 1.44MB；文件只写在根目录，簇号从小到大取空闲簇，
//       不要求连续；同名文件先删除再写入；不改引导扇区、卷标与其他文件。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import { ATTR_VOLUME, END_OF_CHAIN, ENTRY_SIZE, dosDateTime, getFatEntry, setFatEntry, toShortName, type DiskFile } from './fat12-image';

/** 引导扇区里 BIOS 参数块的最小长度。 */
const BOOT_SECTOR_MIN = 36;

/** 数据区的第一个簇号。 */
const FIRST_CLUSTER = 2;

/** FAT12 的簇数上限（不含）：达到它就是 FAT16。 */
const FAT12_CLUSTER_LIMIT = 4085;

/** 目录项的第一个字节：0 表示此后没有目录项，0xE5 表示已删除。 */
const ENTRY_END = 0x00;
const ENTRY_DELETED = 0xe5;

/** 新文件的属性：存档。 */
const ATTR_ARCHIVE = 0x20;

/** 底盘镜像的布局。 */
interface Layout {
  readonly fatStart: number;
  readonly fatBytes: number;
  readonly fatCount: number;
  readonly rootStart: number;
  readonly rootEntries: number;
  readonly dataStart: number;
  readonly clusterBytes: number;
  readonly clusterCount: number;
}

/**
 * 抛出底盘镜像不可用的错误。
 * @param reason 原因。
 * @returns 不会返回。
 */
function invalidBase(reason: string): never {
  throw new FcdevError(`底盘镜像不是可用的 FAT12 软盘镜像：${reason}`, {
    code: 'DISK_BASE_INVALID',
    hint: '在 fcdev.json 的 diskImage 里指向 FAT12 格式的软盘镜像（如带系统文件的启动盘）'
  });
}

/**
 * 读取底盘镜像的布局。
 * @param image 底盘镜像。
 * @returns 布局。
 * @throws FcdevError 引导扇区的参数不合理，或不是 FAT12。
 */
function readLayout(image: Buffer): Layout {
  if (image.length < BOOT_SECTOR_MIN) return invalidBase('文件太小');
  const bytesPerSector = image.readUInt16LE(11);
  const sectorsPerCluster = image.readUInt8(13);
  const reservedSectors = image.readUInt16LE(14);
  const fatCount = image.readUInt8(16);
  const rootEntries = image.readUInt16LE(17);
  const totalSectors = image.readUInt16LE(19) === 0 ? image.readUInt32LE(32) : image.readUInt16LE(19);
  const sectorsPerFat = image.readUInt16LE(22);
  if (bytesPerSector < 128 || sectorsPerCluster === 0 || fatCount === 0 || sectorsPerFat === 0 || rootEntries === 0) return invalidBase('引导扇区的参数不合理');
  if (totalSectors * bytesPerSector > image.length) return invalidBase('文件比引导扇区声明的小');
  const rootSectors = Math.ceil((rootEntries * ENTRY_SIZE) / bytesPerSector);
  const dataSectors = totalSectors - reservedSectors - fatCount * sectorsPerFat - rootSectors;
  const clusterCount = Math.floor(dataSectors / sectorsPerCluster);
  if (clusterCount < 1 || clusterCount >= FAT12_CLUSTER_LIMIT) return invalidBase('簇数不在 FAT12 的范围内');
  const fatStart = reservedSectors * bytesPerSector;
  const fatBytes = sectorsPerFat * bytesPerSector;
  const rootStart = fatStart + fatCount * fatBytes;
  return {
    fatStart,
    fatBytes,
    fatCount,
    rootStart,
    rootEntries,
    dataStart: rootStart + rootSectors * bytesPerSector,
    clusterBytes: sectorsPerCluster * bytesPerSector,
    clusterCount
  };
}

/**
 * 释放一条簇链。
 * @param fat FAT 表。
 * @param first 起始簇。
 * @param layout 布局。
 */
function freeChain(fat: Buffer, first: number, layout: Layout): void {
  let cluster = first;
  for (let steps = 0; steps < layout.clusterCount && cluster >= FIRST_CLUSTER && cluster < layout.clusterCount + FIRST_CLUSTER; steps += 1) {
    const next = getFatEntry(fat, cluster);
    setFatEntry(fat, cluster, 0);
    cluster = next;
  }
}

/**
 * 取出 count 个空闲簇。
 * @param fat FAT 表。
 * @param count 个数。
 * @param layout 布局。
 * @returns 空闲簇号；不够时为 undefined。
 */
function takeFreeClusters(fat: Buffer, count: number, layout: Layout): number[] | undefined {
  const found: number[] = [];
  for (let cluster = FIRST_CLUSTER; cluster < layout.clusterCount + FIRST_CLUSTER && found.length < count; cluster += 1) {
    if (getFatEntry(fat, cluster) === 0) found.push(cluster);
  }
  return found.length === count ? found : undefined;
}

/**
 * 在底盘镜像的根目录里添加文件。
 * @param base 底盘镜像，不会被修改。
 * @param files 要添加的文件；与底盘里同名的文件被替换。
 * @returns 新镜像。
 * @throws FcdevError 底盘不是 FAT12、文件名不符合 8.3，或空间、根目录项不够。
 */
export function overlayFat12Image(base: Uint8Array, files: readonly DiskFile[]): Buffer {
  const image = Buffer.from(base);
  const layout = readLayout(image);
  const fat = Buffer.from(image.subarray(layout.fatStart, layout.fatStart + layout.fatBytes));

  for (const file of files) {
    const shortName = toShortName(file.name);
    let existing = -1;
    let vacant = -1;
    for (let index = 0; index < layout.rootEntries; index += 1) {
      const at = layout.rootStart + index * ENTRY_SIZE;
      const first = image.readUInt8(at);
      if (first === ENTRY_END || first === ENTRY_DELETED) {
        if (vacant < 0) vacant = index;
        if (first === ENTRY_END) break;
        continue;
      }
      if ((image.readUInt8(at + 11) & ATTR_VOLUME) === 0 && image.toString('ascii', at, at + 11) === shortName) existing = index;
    }
    if (existing >= 0) {
      const at = layout.rootStart + existing * ENTRY_SIZE;
      freeChain(fat, image.readUInt16LE(at + 26), layout);
      image.writeUInt8(ENTRY_DELETED, at);
      if (vacant < 0 || existing < vacant) vacant = existing;
    }
    if (vacant < 0) throw new FcdevError(`底盘镜像的根目录已满，放不下：${file.name}`, { code: 'DISK_FULL' });

    const data = Buffer.from(file.data);
    const need = Math.ceil(data.length / layout.clusterBytes);
    const clusters = takeFreeClusters(fat, need, layout);
    if (clusters === undefined) throw new FcdevError(`底盘镜像的剩余空间不足，放不下：${file.name}`, { code: 'DISK_FULL', hint: '换一个空间更大的底盘镜像，或删掉 disk 目录里不需要的文件' });
    clusters.forEach((cluster, index) => {
      const at = layout.dataStart + (cluster - FIRST_CLUSTER) * layout.clusterBytes;
      image.fill(0, at, at + layout.clusterBytes);
      data.copy(image, at, index * layout.clusterBytes, (index + 1) * layout.clusterBytes);
      setFatEntry(fat, cluster, index === clusters.length - 1 ? END_OF_CHAIN : (clusters[index + 1] ?? END_OF_CHAIN));
    });

    const entryAt = layout.rootStart + vacant * ENTRY_SIZE;
    const stamp = dosDateTime(file.mtime ?? new Date(1998, 0, 1));
    image.fill(0, entryAt, entryAt + ENTRY_SIZE);
    image.write(shortName, entryAt, 'ascii');
    image.writeUInt8(ATTR_ARCHIVE, entryAt + 11);
    image.writeUInt16LE(stamp.time, entryAt + 22);
    image.writeUInt16LE(stamp.date, entryAt + 24);
    image.writeUInt16LE(clusters[0] ?? 0, entryAt + 26);
    image.writeUInt32LE(data.length, entryAt + 28);
  }

  for (let copy = 0; copy < layout.fatCount; copy += 1) fat.copy(image, layout.fatStart + copy * layout.fatBytes);
  return image;
}
