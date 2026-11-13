// ------------------------------------------------------------------------
// 名称：run-gbasic.mjs
// 说明：重新汇编 GBASIC 的反汇编源码，放进步步高 DOS 启动盘镜像的副本，在模拟器里运行 gbasic，并与原版的运行结果比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：用法 node tools/run-gbasic.mjs [--no-run]；--no-run 只构建并写镜像。
//       构建：disasm/gbasic.s 经 ca65、ld65 得到 build/gbasic.bin，前面加 2 字节加载地址 $7B00 成为 build/GBASIC.CMD；
//       镜像：reference/floppy-images/bbk/GBASIC.IMG 的副本 build/gbasic-new.img，里面的 GBASIC.CMD 换成新构建的（只能改原文件占用的簇，放不下时报错）；
//       运行：bbk_bios10.nes 加载镜像，回车跳过日期与时间，输入 gbasic，按 1 进入 BASIC，截图；原版与新版各跑一遍，截图逐字节相同才算一致。
// ------------------------------------------------------------------------
import { copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { startSession, typeText } from '../../../../machines/bbk/tools/emu-session.mjs';
import { renderRoundtripConfig } from '../../../../packages/fcdev/src/disasm/index.mjs';
import { locateToolchain, runTool } from '../../../../packages/fcdev/src/toolchain/index.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const REPO_ROOT = resolve(ROOT, '..', '..', '..');
const SOURCE = join(ROOT, 'disasm', 'gbasic.s');
const BUILD = join(ROOT, 'build');
const ORIGINAL_IMAGE = join(REPO_ROOT, 'reference', 'floppy-images', 'bbk', 'GBASIC.IMG');
const BIOS_ROM = join(REPO_ROOT, 'reference', 'rom', 'bbk_bios10.nes');

/** 程序的加载地址，也是执行的起点 */
const LOAD_ADDRESS = 0x7b00;

/** CMD 程序能占用的最大范围：$7B00 到 $FFFF */
const MAX_SIZE = 0x10000 - LOAD_ADDRESS;

/** 要替换的文件在镜像根目录里的 11 字节名 */
const CMD_NAME = 'GBASIC  CMD';

/**
 * 汇编并链接源码，返回 CMD 文件内容。
 * @returns {Promise<Buffer>} 2 字节加载地址加程序
 * @throws {Error} 汇编或链接失败
 */
async function buildCmd() {
  mkdirSync(BUILD, { recursive: true });
  const objectFile = join(BUILD, 'gbasic.o');
  const configFile = join(BUILD, 'gbasic.cfg');
  const binaryFile = join(BUILD, 'gbasic.bin');
  const toolchain = locateToolchain({});
  writeFileSync(configFile, renderRoundtripConfig(LOAD_ADDRESS, MAX_SIZE));
  for (const [tool, args] of [['ca65', [SOURCE, '-o', objectFile]], ['ld65', ['-C', configFile, '-o', binaryFile, objectFile]]]) {
    const result = await runTool(toolchain, tool, args);
    if (result.code !== 0) throw new Error(`${tool} 失败：${result.stdout}${result.stderr}`.trim());
  }
  const header = Buffer.from([LOAD_ADDRESS & 0xff, LOAD_ADDRESS >> 8]);
  const cmd = Buffer.concat([header, readFileSync(binaryFile)]);
  writeFileSync(join(BUILD, 'GBASIC.CMD'), cmd);
  return cmd;
}

/**
 * 把镜像根目录里 GBASIC.CMD 的内容换成新的，文件占用的簇不变。
 * @param {Buffer} image 镜像
 * @param {Buffer} data 新内容
 * @returns {Buffer} 新镜像
 * @throws {Error} 找不到文件，或新内容比原来占用的簇还大
 */
function replaceCmd(image, data) {
  const result = Buffer.from(image);
  const bytesPerSector = result.readUInt16LE(11);
  const clusterBytes = result[13] * bytesPerSector;
  const reserved = result.readUInt16LE(14);
  const fatCount = result[16];
  const rootEntries = result.readUInt16LE(17);
  const fatSectors = result.readUInt16LE(22);
  const fatOffset = reserved * bytesPerSector;
  const rootOffset = (reserved + fatCount * fatSectors) * bytesPerSector;
  const dataOffset = rootOffset + rootEntries * 32;
  const nextCluster = (cluster) => {
    const value = result.readUInt16LE(fatOffset + ((cluster * 3) >> 1));
    return cluster & 1 ? value >> 4 : value & 0xfff;
  };

  for (let index = 0; index < rootEntries; index += 1) {
    const entry = rootOffset + index * 32;
    if (result.subarray(entry, entry + 11).toString('latin1') !== CMD_NAME) continue;
    const chain = [];
    for (let cluster = result.readUInt16LE(entry + 26); cluster >= 2 && cluster < 0xff8; cluster = nextCluster(cluster)) chain.push(cluster);
    if (data.length > chain.length * clusterBytes) throw new Error(`新的 GBASIC.CMD（${data.length} 字节）超过原文件占用的 ${chain.length * clusterBytes} 字节`);
    chain.forEach((cluster, order) => {
      const at = dataOffset + (cluster - 2) * clusterBytes;
      result.fill(0, at, at + clusterBytes);
      data.copy(result, at, order * clusterBytes, Math.min((order + 1) * clusterBytes, data.length));
    });
    result.writeUInt32LE(data.length, entry + 28);
    return result;
  }
  throw new Error('镜像里没有 GBASIC.CMD');
}

/**
 * 在模拟器里用步步高 BIOS 启动镜像，运行 gbasic 进入 BASIC，截图。
 * @param {string} imageFile 软盘镜像
 * @param {string} shotFile 截图文件
 * @param {number} port 调试端口
 * @returns {Promise<void>}
 */
async function runOnce(imageFile, shotFile, port) {
  const session = await startSession(BIOS_ROM, port);
  try {
    await session.send(`disk ${imageFile}`);
    await session.send('reset');
    await session.send('run 600');
    await typeText(session, '\n');
    await session.send('run 60');
    await typeText(session, '\n');
    await session.send('run 120');
    await typeText(session, 'gbasic\n');
    await session.send('run 300');
    await typeText(session, '1');
    await session.send('run 200');
    const reply = await session.send(`shot ${shotFile}`);
    if (!reply.ok) throw new Error(`截图失败：${reply.head}`);
  } finally {
    session.close();
  }
}

const original = readFileSync(ORIGINAL_IMAGE);
const cmd = await buildCmd();
const originalCmd = readFileSync(join(ROOT, 'GBASIC.CMD'));
console.log(`GBASIC.CMD：${cmd.length} 字节，与原版${cmd.equals(originalCmd) ? '逐字节一致' : '不同'}`);

const newImage = join(BUILD, 'gbasic-new.img');
writeFileSync(newImage, replaceCmd(original, cmd));
const oldImage = join(BUILD, 'gbasic-old.img');
copyFileSync(ORIGINAL_IMAGE, oldImage);
console.log(`已写入 ${newImage}`);

if (!process.argv.includes('--no-run')) {
  if (!existsSync(BIOS_ROM)) throw new Error(`找不到 ${BIOS_ROM}`);
  const oldShot = join(BUILD, 'shot-old.png');
  const newShot = join(BUILD, 'shot-new.png');
  await runOnce(oldImage, oldShot, 5712);
  await runOnce(newImage, newShot, 5713);
  const same = readFileSync(oldShot).equals(readFileSync(newShot));
  console.log(`截图 ${oldShot} 与 ${newShot}${same ? ' 逐字节相同' : ' 不同'}`);
  process.exitCode = same ? 0 : 1;
}
