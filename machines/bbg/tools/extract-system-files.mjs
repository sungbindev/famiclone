// ------------------------------------------------------------------------
// 名称：extract-system-files.mjs
// 说明：从步步高系统盘镜像 BBGDOS.IMG 提取启动扇区、BBGDOS.SYS 与 COMMAND.CMD，供反汇编工程使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：输出到 machines/bbg/build/image/（不进 Git）；文件系统按镜像引导扇区里的 FAT12 参数解析。
// ------------------------------------------------------------------------

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..', '..');
const imagePath = path.join(repo, 'reference', 'floppy-images', 'bbg', 'BBGDOS.IMG');
const outDir = path.join(here, '..', 'build', 'image');
const SECTOR = 512;

/** 在 FAT12 根目录里按文件名取出文件内容，找不到时抛出错误。 */
function readRootFile(disk, wanted) {
  const sectorsPerCluster = disk[13];
  const reserved = disk.readUInt16LE(14);
  const fatCount = disk[16];
  const rootEntries = disk.readUInt16LE(17);
  const sectorsPerFat = disk.readUInt16LE(22);
  const rootStart = (reserved + fatCount * sectorsPerFat) * SECTOR;
  const dataStart = rootStart + Math.ceil((rootEntries * 32) / SECTOR) * SECTOR;
  const fat = disk.subarray(reserved * SECTOR, (reserved + sectorsPerFat) * SECTOR);
  const nextCluster = (cluster) => {
    const value = fat.readUInt16LE(Math.floor((cluster * 3) / 2));
    return cluster & 1 ? value >> 4 : value & 0xfff;
  };
  for (let i = 0; i < rootEntries; i++) {
    const entry = rootStart + i * 32;
    if (disk[entry] === 0) break;
    if (disk[entry] === 0xe5) continue;
    const name = `${disk.subarray(entry, entry + 8).toString('latin1').trimEnd()}.${disk.subarray(entry + 8, entry + 11).toString('latin1').trimEnd()}`;
    if (name !== wanted) continue;
    let cluster = disk.readUInt16LE(entry + 26);
    const size = disk.readUInt32LE(entry + 28);
    const parts = [];
    while (cluster >= 2 && cluster < 0xff0) {
      const begin = dataStart + (cluster - 2) * sectorsPerCluster * SECTOR;
      parts.push(disk.subarray(begin, begin + sectorsPerCluster * SECTOR));
      cluster = nextCluster(cluster);
    }
    return Buffer.concat(parts).subarray(0, size);
  }
  throw new Error(`镜像里没有 ${wanted}`);
}

const disk = fs.readFileSync(imagePath);
fs.mkdirSync(outDir, { recursive: true });
const files = {
  'boot-sector.bin': disk.subarray(0, SECTOR),
  'BBGDOS.SYS': readRootFile(disk, 'BBGDOS.SYS'),
  'COMMAND.CMD': readRootFile(disk, 'COMMAND.CMD'),
};
for (const [name, data] of Object.entries(files)) {
  fs.writeFileSync(path.join(outDir, name), data);
  console.log(name.padEnd(16), String(data.length).padStart(6), crypto.createHash('md5').update(data).digest('hex'));
}
