// ------------------------------------------------------------------------
// 名称：trace-gbasic.mjs
// 说明：在步步高 DOS 下运行 gbasic，执行各种 BASIC 语句与菜单功能，记录执行过的指令地址，供 fcdev disasm 的 --trace 使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：用法 node tools/trace-gbasic.mjs [basic|graphic|dos]...，不带参数时三个阶段都跑；每个阶段重新启动机器，用 trace 的 unique 选项只记每个地址第一次，
//       结果合并成 build/gbasic-trace.txt（只含 GBASIC 的地址范围，保留 trace 的原行）。每个阶段在 build/shots 里留一张截图。
//       然后用：node ../../../packages/fcdev/bin/fcdev.mjs disasm <gbasic.bin> --format binary --base 7B00 --trace build/gbasic-trace.txt --out disasm
//       trace 按 CPU 地址去重，同一地址被别的代码（如 BIOS）先执行过会漏记，所以每个阶段都在输入 gbasic 回车之后才开始记录。
// ------------------------------------------------------------------------
import { copyFileSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { startSession } from '../../../../machines/bbk/tools/emu-session.mjs';
import { KEY, press, typeText } from './keys.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const REPO_ROOT = resolve(ROOT, '..', '..', '..');
const BUILD = join(ROOT, 'build');
const SHOTS = join(BUILD, 'shots');
const ORIGINAL_IMAGE = join(REPO_ROOT, 'reference', 'floppy-images', 'bbk', 'GBASIC.IMG');

/** 模拟器挂载的镜像用副本，运行中的写盘不会改到参考资料里的原文件 */
const IMAGE = join(BUILD, 'trace.img');
const BIOS_ROM = join(REPO_ROOT, 'reference', 'rom', 'bbk_bios10.nes');

const ORIGINAL_CMD = join(ROOT, 'GBASIC.CMD');

/** GBASIC 占用的地址范围：$7B00 起 26896 字节 */
const RANGE_START = 0x7b00;
const RANGE_END = 0x7b00 + 26896;

/** 每个阶段最多记录的行数（unique 模式下按实际写出的行数算） */
const TRACE_LINES = 2000000;

/** BASIC 阶段依次输入的行；INPUT、LINPUT 后面紧跟的行是给它们的输入 */
const BASIC_LINES = [
  'CLS',
  'PRINT "HELLO";123;-4.5',
  'PRINT ABS(-5);SGN(-3);LEN("ABC");ASC("A");FRE(0)',
  'PRINT STR$(12);VAL("34");CHR$(65);HEX$(255)',
  'PRINT MID$("HELLO",2,3);LEFT$("HELLO",2);RIGHT$("HELLO",2)',
  'PRINT RND(10);PEEK(0);7 MOD 3;5<>4;NOT 0;1 AND 3;1 OR 2;1 XOR 3',
  'PRINT 2*3+4/2-1;1<2;3>=3;2<=1',
  'PRINT STICK(0);STRIG(0);XPOS(0);YPOS(0);POS(0);CSRLIN',
  'PRINT SCR$(1,1);INKEY$',
  'DIM A(10),B$(3)',
  'A(1)=5:B$(1)="X":PRINT A(1);B$(1)',
  'SWAP A(1),A(2):PRINT A(1);A(2)',
  '10 REM TEST',
  '20 FOR I=1 TO 5 STEP 2',
  '30 PRINT I;',
  '40 NEXT I',
  '50 GOSUB 100',
  '60 IF I>3 THEN PRINT "BIG"',
  '70 DATA 1,2,3,"X"',
  '80 READ B:PRINT B',
  '90 RESTORE:ON 1 GOTO 110:END',
  '100 PRINT "SUB":RETURN',
  '110 PRINT "ON":STOP',
  'RUN',
  'CONT',
  'LIST',
  'LIST 20-40',
  'POKE 1,2:PRINT PEEK(1)',
  'COLOR 1,1,2',
  'COLOR 0',
  'LOCATE 5,5:PRINT "AT"',
  'POSITION 0,10,10',
  'PALET 0,1,2,3',
  'PALET B,1,2,3,4',
  'CGSET 0,1',
  'CGEN 0',
  'VIEW',
  'SPRITE 0,10,20,1',
  'SPRITE ON',
  'SPRITE OFF',
  'MOVE 0',
  'DEF MOVE(0)=SPRITE(0,1,1,1,1,1)',
  'DEF SPRITE 0,(0,0,0,0,0)=1',
  'KEY 1,"HELLO"',
  'KEY LIST',
  'BEEP',
  'PLAY "CDEFG"',
  'PAUSE 5',
  'CUT 0',
  'ERA',
  'CALL 0',
  'XOR 1',
  'INPUT A',
  '7',
  'LINPUT A$',
  'HELLO',
  'INPUT "N";N',
  '3',
  'SAVE "T"',
  'LOAD "T"',
  'LIST',
  'CLEAR',
  'NEW',
  'PRINT FRE(0)',
  'FOR I=1 TO 3:PRINT I;:NEXT',
  'GOSUB 100',
  'RETURN',
  'NEXT',
  'DATA 1',
  'READ A',
  'PRINT 1/0',
  'PRINT "A"+1',
  'PRINT 99999*99999',
  'X',
  '?',
];

/** 图形编辑阶段依次按的键 */
const GRAPHIC_KEYS = [
  ...['right', 'right', 'down', 'down', 'left', 'up', 'enter', 'space', 'tab', 'backspace', 'delete', 'insert', 'home', 'end', 'pageUp', 'pageDown'],
  ...['f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7', 'f8', 'f9', 'f10', 'escape'],
  ...'abcdefghijklmnopqrstuvwxyz0123456789'.split(''),
];

/** 图形编辑阶段随机按键的次数与随机数种子（固定种子让每次运行的按键相同） */
const RANDOM_KEY_COUNT = 1200;
const RANDOM_SEED = 20261113;

/** 随机按键的候选：方向键、确认、取消、功能键、字母与数字 */
const RANDOM_KEYS = [
  'up', 'down', 'left', 'right', 'up', 'down', 'left', 'right', 'enter', 'enter', 'space', 'escape', 'tab', 'backspace', 'delete',
  'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7', 'f8', 'f9', 'f10',
  ...'abcdefghijklmnopqrstuvwxyz0123456789'.split(''),
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
 * 启动机器并进入 gbasic 的菜单，输入 gbasic 回车之后才开始记录。
 * @param {{send: (command: string) => Promise<unknown>}} session 调试会话
 * @param {string} traceFile trace 文件
 * @returns {Promise<void>}
 */
async function startGbasic(session, traceFile) {
  await session.send(`disk ${IMAGE}`);
  await session.send('reset');
  await session.send('run 600');
  await press(session, KEY.enter);
  await session.send('run 60');
  await press(session, KEY.enter);
  await session.send('run 120');
  await typeText(session, 'gbasic');
  await session.send(`trace ${traceFile} ${TRACE_LINES} unique`);
  await press(session, KEY.enter);
  await session.send('run 300');
}

/**
 * 保存一张截图。
 * @param {{send: (command: string) => Promise<unknown>}} session 调试会话
 * @param {string} name 文件名（不含扩展名）
 * @returns {Promise<void>}
 */
async function shot(session, name) {
  await session.send(`run 20`);
  await session.send(`shot ${join(SHOTS, `${name}.png`)}`);
}

const phases = {
  /** 菜单 1：BASIC */
  async basic(session, traceFile) {
    await startGbasic(session, traceFile);
    await typeText(session, '1');
    await session.send('run 100');
    for (const line of BASIC_LINES) {
      await typeText(session, `${line}\n`);
      await session.send('run 40');
    }
    await shot(session, 'basic');
  },
  /** 菜单 2：BG GRAPHIC */
  async graphic(session, traceFile) {
    await startGbasic(session, traceFile);
    await typeText(session, '2');
    await session.send('run 100');
    await shot(session, 'graphic-start');
    for (const key of GRAPHIC_KEYS) {
      await (KEY[key] === undefined ? typeText(session, key) : press(session, KEY[key]));
      await session.send('run 10');
    }
    await shot(session, 'graphic-fixed');
    const random = createRandom(RANDOM_SEED);
    for (let count = 0; count < RANDOM_KEY_COUNT; count += 1) {
      const key = RANDOM_KEYS[Math.floor(random() * RANDOM_KEYS.length)];
      await (KEY[key] === undefined ? typeText(session, key) : press(session, KEY[key]));
      await session.send('run 10');
    }
    await shot(session, 'graphic');
  },
  /** 菜单 3：DOS */
  async dos(session, traceFile) {
    await startGbasic(session, traceFile);
    await typeText(session, '3');
    await session.send('run 200');
    await typeText(session, 'dir\n');
    await session.send('run 100');
    await shot(session, 'dos');
  },
};

mkdirSync(SHOTS, { recursive: true });
copyFileSync(ORIGINAL_IMAGE, IMAGE);
const requested = process.argv.slice(2);
const names = requested.length > 0 ? requested : Object.keys(phases);
const addresses = new Map();
const image = readFileSync(ORIGINAL_CMD).subarray(2);
let port = 5730;
for (const name of names) {
  if (!phases[name]) throw new Error(`没有这个阶段：${name}，可用的有 ${Object.keys(phases).join('、')}`);
  const traceFile = join(BUILD, `trace-${name}.txt`);
  const session = await startSession(BIOS_ROM, port);
  port += 1;
  try {
    await phases[name](session, traceFile);
    await session.send('trace off');
  } finally {
    session.close();
  }
  let count = 0;
  for (const line of readFileSync(traceFile, 'utf8').split('\n')) {
    if (line[0] !== 'T') continue;
    const address = Number.parseInt(line.slice(2, 6), 16);
    // BIOS 等别的代码也可能在同一个地址执行，操作码与 GBASIC 在该处的字节不同的记录丢掉
    const opcode = Number.parseInt(line.split(' ')[3], 16);
    if (address >= RANGE_START && address < RANGE_END && image[address - RANGE_START] === opcode) {
      if (!addresses.has(address)) addresses.set(address, line);
      count += 1;
    }
  }
  console.log(`${name}：范围内 ${count} 个地址，累计 ${addresses.size} 个`);
}
writeFileSync(join(BUILD, 'gbasic-trace.txt'), [...addresses].sort((a, b) => a[0] - b[0]).map((entry) => entry[1]).join('\n') + '\n');
console.log(`已写入 ${join(BUILD, 'gbasic-trace.txt')}`);
