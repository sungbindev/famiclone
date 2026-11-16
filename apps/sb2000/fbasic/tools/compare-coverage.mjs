// ------------------------------------------------------------------------
// 名称：compare-coverage.mjs
// 说明：阶段 5 的对照：同一批覆盖脚本（analysis/coverage/）分别在 FC 模式的原版（Family BASIC 键盘）与学习机的新版（SB2000 键盘）里运行，比较每次截图的亮点分布与程序文本区（$6000-$65FF，$6600 起是 SB-DOS 自己的数据）的内容。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：用法 node tools/compare-coverage.mjs [脚本名片段...] [--dir <脚本目录>]；结果在 build/compare/{fc,sb}/，每个脚本的每张截图都比较，打印不同亮点的个数；
//       脚本格式同 record-coverage.mjs（一行一条要键入的文字，# 注释，@wait 帧数，@key 键位 [帧数]，@shot 名字）；
//       学习机的画面是 50Hz、原版 60Hz，所以 @wait 的帧数按 5/6 换算，让真实时间一致；STOP 键在原版是 Pause，在学习机是 F9；
//       SAVE、LOAD 这类磁盘命令键入后多等一会儿，DOS 读写期间键入的键会丢；每行键入后两边都等 60 帧，语句执行期间键入的键也会丢，两边丢的个数不同会让程序文本不同；颜色因两个模拟器的调色板表不同，所以只比较“亮不亮”（亮度大于 64）。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateSync } from 'node:zlib';
import { bootFamilyBasic, frames as fcFrames, screenshot as fcScreenshot, startEmber, typeText as fcTypeText } from './lib/ember-fc.mjs';
import { startProgram } from './lib/sb2000-run.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const SCRIPTS = join(ROOT, 'analysis', 'coverage');
const OUTPUT = join(ROOT, 'build', 'compare');
const TEXT_START = 0x6000;
const TEXT_LENGTH = 0x0600;
const LIT_LEVEL = 64;
const DISK_WORDS = /^(SAVE|LOAD|FILE|BACKUP|GAME)\b/;
const DISK_WAIT = 150;
const ENTER_WAIT = 60;

/** SB2000 键盘按美式键位：每个字符对应的物理键位与是否按住 Shift。 */
const US_PLAIN = { ' ': 'Space', '\n': 'Enter', '-': 'Minus', '=': 'Equal', ';': 'Semicolon', ',': 'Comma', '.': 'Period', '/': 'Slash', "'": 'Quote' };
const US_SHIFTED = { '!': 'Digit1', '@': 'Digit2', '#': 'Digit3', $: 'Digit4', '%': 'Digit5', '^': 'Digit6', '&': 'Digit7', '*': 'Digit8', '(': 'Digit9', ')': 'Digit0', '+': 'Equal', ':': 'Semicolon', '"': 'Quote', '<': 'Comma', '>': 'Period', '?': 'Slash' };

/**
 * 字符转学习机键盘的键位。
 * @param {string} char 单个字符
 * @returns {{code: string, shift: boolean}} 物理键位与是否按住 Shift
 * @throws {Error} 不支持的字符
 */
function usKeyFor(char) {
  if (/^[A-Za-z]$/.test(char)) return { code: `Key${char.toUpperCase()}`, shift: false };
  if (/^[0-9]$/.test(char)) return { code: `Digit${char}`, shift: false };
  if (char in US_PLAIN) return { code: US_PLAIN[char], shift: false };
  if (char in US_SHIFTED) return { code: US_SHIFTED[char], shift: true };
  throw new Error(`不支持的字符：${JSON.stringify(char)}`);
}

/**
 * 解析一个覆盖脚本。
 * @param {string} file 脚本路径
 * @returns {Array<{kind: 'type'|'wait'|'key'|'shot', text?: string, frames?: number, code?: string, label?: string}>} 步骤
 */
function parseScript(file) {
  const steps = [];
  for (const raw of readFileSync(file, 'utf8').split(/\r?\n/)) {
    const line = raw.trimEnd();
    if (line === '' || line.startsWith('#')) continue;
    if (line.startsWith('@wait ')) steps.push({ kind: 'wait', frames: Number(line.slice(6)) });
    else if (line.startsWith('@key ')) {
      const [code, count = '6'] = line.slice(5).split(/\s+/);
      steps.push({ kind: 'key', code, frames: Number(count) });
    } else if (line.startsWith('@shot ')) steps.push({ kind: 'shot', label: line.slice(6) });
    else steps.push({ kind: 'type', text: line });
  }
  return steps;
}

/**
 * 在 FC 模式的原版里运行脚本。
 * @param {Array<object>} steps 步骤
 * @param {string} name 脚本名
 * @param {string} directory 截图目录
 * @returns {Promise<Buffer>} 程序文本区的内容
 */
async function runOriginal(steps, name, directory) {
  const session = await startEmber();
  try {
    const { client } = session;
    await bootFamilyBasic(client, 200);
    for (const step of steps) {
      if (step.kind === 'wait') await fcFrames(client, step.frames);
      else if (step.kind === 'type') await fcTypeText(client, `${step.text}\n`, { enterWait: ENTER_WAIT });
      else if (step.kind === 'key') {
        await client.api.input.key({ code: step.code, down: true });
        await fcFrames(client, step.frames);
        await client.api.input.key({ code: step.code, down: false });
        await fcFrames(client, 6);
      } else await fcScreenshot(client, join(directory, `${name}-${step.label}.png`));
    }
    await fcFrames(client, 60);
    await fcScreenshot(client, join(directory, `${name}-end.png`));
    const memory = await client.request('inspector.readMemory', { space: 'cpu', address: TEXT_START, length: TEXT_LENGTH });
    return Buffer.from(memory.data);
  } finally {
    session.close();
  }
}

/**
 * 在学习机的新版里运行脚本。
 * @param {Array<object>} steps 步骤
 * @param {string} name 脚本名
 * @param {string} directory 截图目录
 * @returns {Promise<Buffer>} 程序文本区的内容
 */
async function runPort(steps, name, directory) {
  const session = await startProgram();
  try {
    await session.frames(300);
    const scale = (count) => Math.max(1, Math.round((count * 5) / 6));
    for (const step of steps) {
      if (step.kind === 'wait') await session.frames(scale(step.frames));
      else if (step.kind === 'type') {
        for (const char of `${step.text}\n`) {
          const { code, shift } = usKeyFor(char);
          if (shift) { await session.key('ShiftLeft', true); await session.frames(3); }
          await session.key(code, true);
          await session.frames(4);
          await session.key(code, false);
          await session.frames(4);
          if (shift) { await session.key('ShiftLeft', false); await session.frames(3); }
        }
        await session.frames(DISK_WORDS.test(step.text) ? DISK_WAIT : ENTER_WAIT);
      } else if (step.kind === 'key') {
        const code = step.code === 'Pause' ? 'F9' : step.code;
        await session.key(code, true);
        await session.frames(step.frames);
        await session.key(code, false);
        await session.frames(6);
      } else await session.shot(join(directory, `${name}-${step.label}.png`));
    }
    await session.frames(60);
    await session.shot(join(directory, `${name}-end.png`));
    return Buffer.from(await session.read(TEXT_START, TEXT_LENGTH));
  } finally {
    await session.close();
  }
}

/**
 * 解码 PNG（8 位、不隔行，灰度、真彩色、索引、带透明通道都支持），返回每个像素的亮度。
 * @param {Buffer} data PNG 文件内容
 * @returns {{width: number, height: number, luma: Uint8Array}} 宽、高与每个像素的亮度
 */
function decodeLuma(data) {
  let offset = 8;
  let width = 0;
  let height = 0;
  let colorType = 0;
  let palette = null;
  const parts = [];
  while (offset < data.length) {
    const length = data.readUInt32BE(offset);
    const type = data.toString('latin1', offset + 4, offset + 8);
    const body = data.subarray(offset + 8, offset + 8 + length);
    if (type === 'IHDR') { width = body.readUInt32BE(0); height = body.readUInt32BE(4); colorType = body[9]; }
    else if (type === 'PLTE') palette = body;
    else if (type === 'IDAT') parts.push(body);
    offset += 12 + length;
  }
  const channels = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[colorType];
  const stride = width * channels;
  const raw = inflateSync(Buffer.concat(parts));
  const pixels = Buffer.alloc(stride * height);
  for (let row = 0; row < height; row += 1) {
    const filter = raw[row * (stride + 1)];
    for (let index = 0; index < stride; index += 1) {
      const value = raw[row * (stride + 1) + 1 + index];
      const left = index >= channels ? pixels[row * stride + index - channels] : 0;
      const up = row > 0 ? pixels[(row - 1) * stride + index] : 0;
      const upLeft = row > 0 && index >= channels ? pixels[(row - 1) * stride + index - channels] : 0;
      let predicted = 0;
      if (filter === 1) predicted = left;
      else if (filter === 2) predicted = up;
      else if (filter === 3) predicted = (left + up) >> 1;
      else if (filter === 4) {
        const estimate = left + up - upLeft;
        const toLeft = Math.abs(estimate - left);
        const toUp = Math.abs(estimate - up);
        const toUpLeft = Math.abs(estimate - upLeft);
        predicted = toLeft <= toUp && toLeft <= toUpLeft ? left : toUp <= toUpLeft ? up : upLeft;
      }
      pixels[row * stride + index] = (value + predicted) & 0xFF;
    }
  }
  const luma = new Uint8Array(width * height);
  for (let pixel = 0; pixel < width * height; pixel += 1) {
    const base = pixel * channels;
    let red;
    let green;
    let blue;
    if (colorType === 3) { const entry = pixels[base] * 3; red = palette[entry]; green = palette[entry + 1]; blue = palette[entry + 2]; }
    else if (channels <= 2) { red = pixels[base]; green = red; blue = red; }
    else { red = pixels[base]; green = pixels[base + 1]; blue = pixels[base + 2]; }
    luma[pixel] = Math.round(red * 0.3 + green * 0.59 + blue * 0.11);
  }
  return { width, height, luma };
}

/**
 * 比较两张截图的亮点分布。
 * @param {string} first 甲 PNG 路径
 * @param {string} second 乙 PNG 路径
 * @returns {number} 亮不亮不同的像素个数
 */
function countLitDifferences(first, second) {
  const a = decodeLuma(readFileSync(first));
  const b = decodeLuma(readFileSync(second));
  if (a.width !== b.width || a.height !== b.height) return -1;
  let count = 0;
  for (let index = 0; index < a.luma.length; index += 1) if ((a.luma[index] > LIT_LEVEL) !== (b.luma[index] > LIT_LEVEL)) count += 1;
  return count;
}

const args = process.argv.slice(2);
const directoryIndex = args.indexOf('--dir');
const scriptDirectory = directoryIndex >= 0 ? resolve(args[directoryIndex + 1]) : SCRIPTS;
const filters = args.filter((arg, index) => !arg.startsWith('--') && index !== directoryIndex + 1);
const names = readdirSync(scriptDirectory).filter((name) => name.endsWith('.txt') && (filters.length === 0 || filters.some((part) => name.includes(part)))).sort();
const fcDirectory = join(OUTPUT, 'fc');
const sbDirectory = join(OUTPUT, 'sb');
mkdirSync(fcDirectory, { recursive: true });
mkdirSync(sbDirectory, { recursive: true });
const failures = [];
for (const file of names) {
  const name = file.replace(/\.txt$/, '');
  const steps = parseScript(join(scriptDirectory, file));
  const originalText = await runOriginal(steps, name, fcDirectory);
  const portText = await runPort(steps, name, sbDirectory);
  writeFileSync(join(OUTPUT, `${name}.fc.bin`), originalText);
  writeFileSync(join(OUTPUT, `${name}.sb.bin`), portText);
  let textDifferences = 0;
  for (let index = 0; index < TEXT_LENGTH; index += 1) if (originalText[index] !== portText[index]) textDifferences += 1;
  const shots = readdirSync(fcDirectory).filter((entry) => entry.startsWith(`${name}-`));
  const report = shots.map((entry) => `${entry.slice(name.length + 1, -4)}=${countLitDifferences(join(fcDirectory, entry), join(sbDirectory, entry))}`);
  console.log(`${name}: 文本区不同字节 ${textDifferences}；截图亮点差 ${report.join(' ')}`);
  if (textDifferences > 0) failures.push(name);
}
console.log(failures.length === 0 ? '全部脚本的文本区一致' : `文本区不一致的脚本：${failures.join('、')}`);
