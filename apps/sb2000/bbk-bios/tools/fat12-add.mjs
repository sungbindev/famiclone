// ------------------------------------------------------------------------
// 名称：fat12-add.mjs
// 说明：向已有的 FAT12 软盘镜像的根目录追加文件，按镜像自己的 BPB（扇区、簇、FAT、根目录大小）计算，不要求标准 1.44MB 布局。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：步步高的 DOS 启动盘每簇 4 个扇区、根目录 208 项，和 packages 里按标准布局写盘的工具不兼容，所以单独写这一个。
//       只追加文件，不动已有的文件与引导扇区；写完同步两份 FAT。
// ------------------------------------------------------------------------

/**
 * 解析 BPB。
 * @param {Buffer} image 镜像
 * @returns {{bytesPerSector: number, sectorsPerCluster: number, reserved: number, fatCount: number, rootEntries: number, fatSectors: number, rootStart: number, dataStart: number, clusterCount: number}} 布局
 */
function readLayout(image) {
  const bytesPerSector = image.readUInt16LE(11);
  const sectorsPerCluster = image[13];
  const reserved = image.readUInt16LE(14);
  const fatCount = image[16];
  const rootEntries = image.readUInt16LE(17);
  const totalSectors = image.readUInt16LE(19);
  const fatSectors = image.readUInt16LE(22);
  const rootStart = reserved + fatCount * fatSectors;
  const rootSectors = Math.ceil((rootEntries * 32) / bytesPerSector);
  const dataStart = rootStart + rootSectors;
  const clusterCount = Math.floor((totalSectors - dataStart) / sectorsPerCluster);
  return { bytesPerSector, sectorsPerCluster, reserved, fatCount, rootEntries, fatSectors, rootStart, dataStart, clusterCount };
}

/**
 * 把 "name.ext" 转成 11 字节的 8.3 目录项名。
 * @param {string} fileName 文件名
 * @returns {Buffer} 11 字节，大写，空格补齐
 */
function shortName(fileName) {
  const [base, ext = ''] = fileName.toUpperCase().split('.');
  if (base.length === 0 || base.length > 8 || ext.length > 3) throw new Error(`不是 8.3 文件名：${fileName}`);
  return Buffer.from(base.padEnd(8, ' ') + ext.padEnd(3, ' '), 'latin1');
}

/**
 * 向镜像追加文件，返回新的镜像（不改传入的对象）。
 * @param {Buffer} source 原镜像
 * @param {{name: string, data: Buffer}[]} files 要追加的文件
 * @returns {Buffer} 新镜像
 */
export function addFat12Files(source, files) {
  const image = Buffer.from(source);
  const layout = readLayout(image);
  const fatOffset = layout.reserved * layout.bytesPerSector;
  const fatSize = layout.fatSectors * layout.bytesPerSector;
  const clusterBytes = layout.sectorsPerCluster * layout.bytesPerSector;
  const entryAt = (cluster) => (cluster * 3) >> 1;
  const getFat = (cluster) => {
    const value = image.readUInt16LE(fatOffset + entryAt(cluster));
    return cluster & 1 ? value >> 4 : value & 0xfff;
  };
  const setFat = (cluster, value) => {
    for (let copy = 0; copy < layout.fatCount; copy += 1) {
      const at = fatOffset + copy * fatSize + entryAt(cluster);
      const old = image.readUInt16LE(at);
      image.writeUInt16LE(cluster & 1 ? (old & 0x000f) | (value << 4) : (old & 0xf000) | value, at);
    }
  };

  let nextFree = 2;
  const allocate = () => {
    while (nextFree < layout.clusterCount + 2 && getFat(nextFree) !== 0) nextFree += 1;
    if (nextFree >= layout.clusterCount + 2) throw new Error('磁盘空间不足');
    return nextFree++;
  };

  const rootOffset = layout.rootStart * layout.bytesPerSector;
  const freeEntry = () => {
    for (let index = 0; index < layout.rootEntries; index += 1) {
      const first = image[rootOffset + index * 32];
      if (first === 0x00 || first === 0xe5) return rootOffset + index * 32;
    }
    throw new Error('根目录已满');
  };

  for (const file of files) {
    let first = 0;
    let previous = 0;
    for (let offset = 0; offset < file.data.length; offset += clusterBytes) {
      const cluster = allocate();
      setFat(cluster, 0xfff);
      if (previous) setFat(previous, cluster);
      else first = cluster;
      previous = cluster;
      const target = (layout.dataStart + (cluster - 2) * layout.sectorsPerCluster) * layout.bytesPerSector;
      image.fill(0, target, target + clusterBytes);
      file.data.copy(image, target, offset, Math.min(offset + clusterBytes, file.data.length));
    }
    const entry = freeEntry();
    image.fill(0, entry, entry + 32);
    shortName(file.name).copy(image, entry);
    image[entry + 11] = 0x20;
    image.writeUInt16LE(0x6000, entry + 22);
    image.writeUInt16LE(0x5a6c, entry + 24);
    image.writeUInt16LE(first, entry + 26);
    image.writeUInt32LE(file.data.length, entry + 28);
  }
  return image;
}
