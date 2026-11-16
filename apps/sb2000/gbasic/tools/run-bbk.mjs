// ------------------------------------------------------------------------
// 名称：run-bbk.mjs
// 说明：在步步高机器的模拟器里运行一个 GBASIC.CMD（原版或重新汇编后移过的版本），按固定的按键脚本操作并截图，用来比较两个版本的行为。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/run-bbk.mjs --name <标签> [--bin <二进制文件>] [--phases basic,graphic,dos] [--random <次数>] [--lines <BASIC 阶段输入的行数>]
//       BASIC 阶段不用 FRE(0)：它的结果随映像大小变化，后移的版本与原版会不同（差值就是后移的字节数）。
//       不给 --bin 就用原版 GBASIC.CMD；给了就把它加上 2 字节的加载地址 $7B00 做成 CMD，换进原版软盘镜像的副本里（只能放进原文件占用的簇）。
//       每个阶段都从开机重新开始；截图放在 build/shots-<标签>/ 里，两个标签的同名截图逐字节相同才算行为一致（见 compare-shots.mjs）。
//       需要调试版模拟器（machines/bbk 的 emu-session）与步步高 BIOS ROM（reference/rom/bbk_bios10.nes）。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { startSession } from '../../../../machines/bbk/tools/emu-session.mjs';
import { KEY, press, typeText } from '../../../bbk/gbasic/tools/keys.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const REPO_ROOT = resolve(ROOT, '..', '..', '..');
const BUILD = join(ROOT, 'build');
const ORIGINAL_IMAGE = join(REPO_ROOT, 'reference', 'floppy-images', 'bbk', 'GBASIC.IMG');
const ORIGINAL_CMD = join(REPO_ROOT, 'apps', 'bbk', 'gbasic', 'GBASIC.CMD');
const BIOS_ROM = join(REPO_ROOT, 'reference', 'rom', 'bbk_bios10.nes');
const CMD_NAME = 'GBASIC  CMD';
const LOAD_ADDRESS = 0x7b00;

const args = process.argv.slice(2);
const option = (name, fallback) => (args.includes(name) ? args[args.indexOf(name) + 1] : fallback);
const LABEL = option('--name', 'run');
const BINARY = option('--bin', null);
const PHASES = option('--phases', 'basic,graphic,dos').split(',');
const RANDOM_KEY_COUNT = Number(option('--random', '300'));
const LOAD = Number.parseInt(option('--load', LOAD_ADDRESS.toString(16)), 16);
const PORT = Number(option('--port', '5740'));
const BASIC_LIMIT = Number(option('--lines', String(Number.MAX_SAFE_INTEGER)));
const TRACE_LINE = Number(option('--trace-line', '0'));
const DUMP_LINE = Number(option('--dump-line', '0'));
const SCRIPT = option('--script', null);
const TRACE_LINES = Number(option('--trace-lines', '600000'));
const RANDOM_SEED = 20261113;

/** BASIC 阶段依次输入的行 */
const BASIC_LINES = [
  'CLS', 'PRINT "HELLO";123;-4.5', 'PRINT ABS(-5);SGN(-3);LEN("ABC");ASC("A")', 'PRINT STR$(12);VAL("34");CHR$(65);HEX$(255)',
  'PRINT MID$("HELLO",2,3);LEFT$("HELLO",2);RIGHT$("HELLO",2)', 'PRINT RND(10);PEEK(0);7 MOD 3;5<>4;NOT 0;1 AND 3;1 OR 2;1 XOR 3',
  'PRINT 2*3+4/2-1;1<2;3>=3;2<=1', 'PRINT STICK(0);STRIG(0);XPOS(0);YPOS(0);POS(0);CSRLIN', 'PRINT SCR$(1,1);INKEY$', 'DIM A(10),B$(3)',
  'A(1)=5:B$(1)="X":PRINT A(1);B$(1)', 'SWAP A(1),A(2):PRINT A(1);A(2)', '10 REM TEST', '20 FOR I=1 TO 5 STEP 2', '30 PRINT I;', '40 NEXT I',
  '50 GOSUB 100', '60 IF I>3 THEN PRINT "BIG"', '70 DATA 1,2,3,"X"', '80 READ B:PRINT B', '90 RESTORE:ON 1 GOTO 110:END', '100 PRINT "SUB":RETURN',
  '110 PRINT "ON":STOP', 'RUN', 'CONT', 'LIST', 'LIST 20-40', 'POKE 1,2:PRINT PEEK(1)', 'COLOR 1,1,2', 'COLOR 0', 'LOCATE 5,5:PRINT "AT"',
  'POSITION 0,10,10', 'PALET 0,1,2,3', 'PALET B,1,2,3,4', 'CGSET 0,1', 'CGEN 0', 'VIEW', 'SPRITE 0,10,20,1', 'SPRITE ON', 'SPRITE OFF', 'MOVE 0',
  'DEF MOVE(0)=SPRITE(0,1,1,1,1,1)', 'DEF SPRITE 0,(0,0,0,0,0)=1', 'KEY 1,"HELLO"', 'KEY LIST', 'BEEP', 'PLAY "CDEFG"', 'PAUSE 5', 'CUT 0', 'ERA',
  'CALL 0', 'XOR 1', 'INPUT A', '7', 'LINPUT A$', 'HELLO', 'INPUT "N";N', '3', 'SAVE "T"', 'LOAD "T"', 'LIST', 'CLEAR', 'NEW', 'PRINT LEN("FRE")',
  'FOR I=1 TO 3:PRINT I;:NEXT', 'GOSUB 100', 'RETURN', 'NEXT', 'DATA 1', 'READ A', 'PRINT 1/0', 'PRINT "A"+1', 'PRINT 99999*99999', 'X', '?',
];

/** 图形编辑阶段先按的固定按键，再按固定种子的随机按键 */
const GRAPHIC_KEYS = [
  ...['right', 'right', 'down', 'down', 'left', 'up', 'enter', 'space', 'tab', 'backspace', 'delete', 'insert', 'home', 'end', 'pageUp', 'pageDown'],
  ...['f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7', 'f8', 'f9', 'f10', 'escape'],
  ...'abcdefghijklmnopqrstuvwxyz0123456789'.split(''),
];
const RANDOM_KEYS = [
  'up', 'down', 'left', 'right', 'up', 'down', 'left', 'right', 'enter', 'enter', 'space', 'escape', 'tab', 'backspace', 'delete',
  'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7', 'f8', 'f9', 'f10', ...'abcdefghijklmnopqrstuvwxyz0123456789'.split(''),
];

/**
 * 固定种子的伪随机数（mulberry32）。
 * @param {number} seed 种子
 * @returns {() => number} 每次调用返回 [0, 1) 的数
 */
function createRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (state + 0x6d2b79f5) >>> 0;
    let value = state;
    value = Math.imul(value ^ (value >>> 15), value | 1);
    value ^= value + Math.imul(value ^ (value >>> 7), value | 61);
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
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
 * 启动机器并进入 gbasic 的菜单。
 * @param {{send: (command: string) => Promise<unknown>}} session 调试会话
 * @param {string} imageFile 软盘镜像
 * @returns {Promise<void>}
 */
async function startGbasic(session, imageFile) {
  await session.send(`disk ${imageFile}`);
  await session.send('reset');
  await session.send('run 600');
  await press(session, KEY.enter);
  await session.send('run 60');
  await press(session, KEY.enter);
  await session.send('run 120');
  await typeText(session, 'gbasic');
  if (args.includes('--trace')) {
    const stamp = join(BUILD, `trace-${LABEL}`);
    await session.send(`trace ${stamp}.txt 2000000 unique`);
    await session.send(`io ${stamp}-io.txt 6000000`);
  }
  await press(session, KEY.enter);
  await session.send('run 300');
}

/**
 * 保存一张截图。
 * @param {{send: (command: string) => Promise<{ok: boolean, head: string}>}} session 调试会话
 * @param {string} file 截图文件
 * @returns {Promise<void>}
 * @throws {Error} 截图失败
 */
async function shot(session, file) {
  await session.send('run 20');
  const reply = await session.send(`shot ${file}`);
  if (!reply.ok) throw new Error(`截图失败：${reply.head}`);
}

/**
 * 把 CPU 地址空间的 $0000-$07FF 与 $6000-$7FFF 转存成文本，每行 64 字节，行首是地址。
 * @param {{send: (command: string) => Promise<{ok: boolean, head: string, lines: string[]}>}} session 调试会话
 * @param {string} name 文件名（build/dump-<name>.txt）
 * @returns {Promise<void>}
 * @throws {Error} 读内存失败
 */
async function dumpMemory(session, name) {
  const rows = [];
  for (const [start, length] of [[0x0000, 0x800], [0x6000, 0x2000]]) {
    const reply = await session.send(`read cpu ${start.toString(16)} ${length}`);
    if (!reply.ok) throw new Error(`读内存失败：${reply.head}`);
    reply.lines.filter((line) => line.startsWith('HEX ')).forEach((line, index) => rows.push(`${(start + index * 64).toString(16).toUpperCase().padStart(4, '0')} ${line.slice(4)}`));
  }
  writeFileSync(join(BUILD, `dump-${name}.txt`), `${rows.join('\n')}\n`);
}

const phases = {
  async basic(session, shots) {
    await typeText(session, '1');
    await session.send('run 100');
    const lines = SCRIPT === null ? BASIC_LINES : SCRIPT.split('|');
    for (const [index, line] of lines.slice(0, BASIC_LIMIT).entries()) {
      const traced = index + 1 === TRACE_LINE;
      if (index + 1 === DUMP_LINE) {
        // 回车前后各转存一次 CPU 内存，比较两个版本的内存状态
        await typeText(session, line);
        await dumpMemory(session, `${LABEL}-before`);
        await press(session, KEY.enter);
        await session.send('run 40');
        await dumpMemory(session, `${LABEL}-after`);
        continue;
      }
      if (traced) {
        // 只记录按下回车之后的执行，免得被输入时的键盘扫描循环占满
        await typeText(session, line);
        await session.send(`trace ${join(BUILD, `trace-${LABEL}.txt`)} ${TRACE_LINES}`);
        await press(session, KEY.enter);
        await session.send('run 40');
        await session.send('trace off');
        continue;
      }
      await typeText(session, `${line}\n`);
      await session.send('run 40');
    }
    await shot(session, join(shots, 'basic.png'));
  },
  async graphic(session, shots) {
    await typeText(session, '2');
    await session.send('run 100');
    await shot(session, join(shots, 'graphic-start.png'));
    for (const key of GRAPHIC_KEYS) {
      await (KEY[key] === undefined ? typeText(session, key) : press(session, KEY[key]));
      await session.send('run 10');
    }
    await shot(session, join(shots, 'graphic-fixed.png'));
    const random = createRandom(RANDOM_SEED);
    for (let count = 0; count < RANDOM_KEY_COUNT; count += 1) {
      const key = RANDOM_KEYS[Math.floor(random() * RANDOM_KEYS.length)];
      await (KEY[key] === undefined ? typeText(session, key) : press(session, KEY[key]));
      await session.send('run 10');
    }
    await shot(session, join(shots, 'graphic.png'));
  },
  async dos(session, shots) {
    await typeText(session, '3');
    await session.send('run 200');
    await typeText(session, 'dir\n');
    await session.send('run 100');
    await shot(session, join(shots, 'dos.png'));
  },
};

mkdirSync(BUILD, { recursive: true });
const shots = join(BUILD, `shots-${LABEL}`);
mkdirSync(shots, { recursive: true });
const cmd = BINARY === null ? readFileSync(ORIGINAL_CMD) : Buffer.concat([Buffer.from([LOAD & 0xff, LOAD >> 8]), readFileSync(BINARY)]);
const imageFile = join(BUILD, `${LABEL}.img`);
writeFileSync(imageFile, BINARY === null ? readFileSync(ORIGINAL_IMAGE) : replaceCmd(readFileSync(ORIGINAL_IMAGE), cmd));
console.log(`${LABEL}：GBASIC.CMD ${cmd.length} 字节，镜像 ${imageFile}`);

let port = PORT;
for (const name of PHASES) {
  if (!phases[name]) throw new Error(`没有这个阶段：${name}，可用的有 ${Object.keys(phases).join('、')}`);
  const session = await startSession(BIOS_ROM, port);
  port += 1;
  try {
    await startGbasic(session, imageFile);
    await phases[name](session, shots);
  } finally {
    session.close();
  }
  console.log(`${LABEL}：${name} 完成`);
}
