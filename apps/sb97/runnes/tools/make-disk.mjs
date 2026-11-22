// 生成小霸王 97 用的 1.44MB FAT12 软盘镜像，把给定文件按 8.3 名写进根目录。
// 用法：node tools/make-disk.mjs <输出.ima> <文件>...
import { readFileSync, writeFileSync } from 'node:fs';
import { basename } from 'node:path';

const SECTOR = 512;
const TOTAL_SECTORS = 2880;
const FAT_SECTORS = 9;
const ROOT_ENTRIES = 224;
const ROOT_SECTORS = (ROOT_ENTRIES * 32) / SECTOR;
const ROOT_LBA = 1 + 2 * FAT_SECTORS;
const DATA_LBA = ROOT_LBA + ROOT_SECTORS;
const EOC = 0xfff;

/** 把文件名转成 8.3 目录项的 11 个字节（大写，主名与扩展名各自补空格）。 */
function toName83(fileName) {
  const upper = fileName.toUpperCase();
  const dot = upper.lastIndexOf('.');
  const base = dot < 0 ? upper : upper.slice(0, dot);
  const ext = dot < 0 ? '' : upper.slice(dot + 1);
  if (!/^[A-Z0-9_]{1,8}$/.test(base) || !/^[A-Z0-9_]{0,3}$/.test(ext)) {
    throw new Error(`文件名不符合 8.3 规则：${fileName}`);
  }
  return base.padEnd(8, ' ') + ext.padEnd(3, ' ');
}

/** 写 FAT12 表项 n 为 v（两字节打包，奇偶表项的半字节位置不同）。 */
function setFat(fat, n, v) {
  const off = Math.floor((n * 3) / 2);
  if (n & 1) {
    fat[off] = (fat[off] & 0x0f) | ((v & 0x0f) << 4);
    fat[off + 1] = (v >> 4) & 0xff;
  } else {
    fat[off] = v & 0xff;
    fat[off + 1] = (fat[off + 1] & 0xf0) | ((v >> 8) & 0x0f);
  }
}

function main() {
  const [outPath, ...files] = process.argv.slice(2);
  if (!outPath || files.length === 0) {
    throw new Error('用法：node tools/make-disk.mjs <输出.ima> <文件>...');
  }
  const image = Buffer.alloc(TOTAL_SECTORS * SECTOR);

  const boot = image.subarray(0, SECTOR);
  boot[0] = 0xeb;
  boot[1] = 0x3c;
  boot[2] = 0x90;
  boot.write('SB97RUN ', 3, 'latin1');
  boot.writeUInt16LE(SECTOR, 11);
  boot[13] = 1;
  boot.writeUInt16LE(1, 14);
  boot[16] = 2;
  boot.writeUInt16LE(ROOT_ENTRIES, 17);
  boot.writeUInt16LE(TOTAL_SECTORS, 19);
  boot[21] = 0xf0;
  boot.writeUInt16LE(FAT_SECTORS, 22);
  boot.writeUInt16LE(18, 24);
  boot.writeUInt16LE(2, 26);
  boot[36] = 0;
  boot[38] = 0x29;
  boot.write('NO NAME    ', 43, 'latin1');
  boot.write('FAT12   ', 54, 'latin1');
  boot[510] = 0x55;
  boot[511] = 0xaa;

  const fat = Buffer.alloc(FAT_SECTORS * SECTOR);
  fat[0] = 0xf0;
  fat[1] = 0xff;
  fat[2] = 0xff;

  let nextCluster = 2;
  const rootOffset = ROOT_LBA * SECTOR;
  files.forEach((path, index) => {
    const data = readFileSync(path);
    const entryOffset = rootOffset + index * 32;
    if (entryOffset >= rootOffset + ROOT_SECTORS * SECTOR) {
      throw new Error('根目录项已满');
    }
    const clusters = Math.ceil(data.length / SECTOR);
    const first = clusters > 0 ? nextCluster : 0;
    for (let i = 0; i < clusters; i++) {
      const c = nextCluster + i;
      setFat(fat, c, i === clusters - 1 ? EOC : c + 1);
      const at = (DATA_LBA + c - 2) * SECTOR;
      data.copy(image, at, i * SECTOR, Math.min(data.length, (i + 1) * SECTOR));
    }
    nextCluster += clusters;
    image.write(toName83(basename(path)), entryOffset, 'latin1');
    image[entryOffset + 11] = 0x20;
    image.writeUInt16LE(0x6000, entryOffset + 22);
    image.writeUInt16LE(0x5a21, entryOffset + 24);
    image.writeUInt16LE(first, entryOffset + 26);
    image.writeUInt32LE(data.length, entryOffset + 28);
  });
  if (nextCluster - 2 > (TOTAL_SECTORS - DATA_LBA)) {
    throw new Error('文件总大小超过软盘容量');
  }

  fat.copy(image, SECTOR);
  fat.copy(image, SECTOR + FAT_SECTORS * SECTOR);
  image.set(boot, 0);
  writeFileSync(outPath, image);
  console.log(`已写入 ${outPath}：${files.length} 个文件，使用 ${nextCluster - 2} 个簇`);
}

main();
