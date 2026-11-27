// Floppy Emu 固件补丁构建脚本（功能核心：新增模式 9，40 磁道 NIB）。
// 流程：汇编 patch.s → 链接到 0x12F00 → 提取 .text 二进制 → 用 nm 取蹦床地址生成 jmp
//       → 把补丁拼回 femu.bin 副本 → 逐字节校验未改动区域与原固件一致 → 输出 build/femu.bin。
// 用法：node patch/build.mjs   （在 reference/bmow 目录下，或任意目录用绝对路径）
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));      // .../reference/bmow/patch
const root = resolve(here, '..');                          // .../reference/bmow
const binDir = join(root, 'tools', 'avr-gcc-16.1.0-x64-windows', 'bin');
const tool = (n) => join(binDir, n + '.exe');
const origPath = join(root, 'bin', 'femu.bin');
const outDir = join(root, 'build');
const outPath = join(outDir, 'femu.bin');

const run = (exe, args) => execFileSync(exe, args, { encoding: 'utf8' });

// 1. 汇编 + 链接 + 提取二进制
const obj = join(here, 'patch.o');
const elf = join(here, 'patch.elf');
const blob = join(here, 'patch.bin');
run(tool('avr-as'), ['-mmcu=atmega1284', '-o', obj, join(here, 'patch.s')]);
run(tool('avr-ld'), ['-m', 'avr51', '-T', join(here, 'patch.ld'), '-o', elf, obj]);
run(tool('avr-objcopy'), ['-O', 'binary', '-j', '.text', elf, blob]);
const code = readFileSync(blob);

// 2. 用 nm 取蹦床入口字节地址
const nm = run(tool('avr-nm'), [elf]);
const sym = {};
for (const line of nm.split(/\r?\n/)) {
  const m = line.match(/^([0-9a-fA-F]+)\s+[Tt]\s+(\S+)/);
  if (m) sym[m[2]] = parseInt(m[1], 16);
}
const need = ['T_disp', 'T_emu', 'T_compat', 'T_init', 'T_size', 'T_menu', 'T_lookup', 'draw_far', 'T_clampA', 'T_clampB', 'T_wstart'];
for (const s of need) {
  if (!(s in sym)) throw new Error('缺少符号 ' + s);
}
const codeBase = sym['patch_start'];
if (codeBase !== 0x12F00) throw new Error('蹦床基址不是 0x12F00：' + codeBase.toString(16));

// 菜单扩项的高 flash 布局（都在原全 0 空闲区 0x12F00–0x1EFF7 内）
const ITEM7_AT = 0x13400;   // "9 Pyramid 40-track\0"
const FONT_AT = 0x13500;    // 字库副本（原 0x1CB9–0x1FD7）
const CLONE_AT = 0x13900;   // 克隆的远地址字符串绘制器（原 0xE902–0xEA5A）
const FONT_SRC = 0x1CB9;
const FONT_END = 0x1FD8;
const DRAWER_SRC = 0xE902;
const DRAWER_END = 0xEA5A;
// 克隆里两处字库基址 ldi 对，改成副本地址（RAMPZ=1，低 16 位）
const oledBase = FONT_AT + (0x1EA3 - FONT_SRC);   // 原 OLED 字库 0x1EA3
const nonOledBase = FONT_AT + (0x1CB9 - FONT_SRC); // 原非 OLED 字库 0x1CB9

// jmp 指令编码：0C 94 lo hi，(hi<<8|lo) = 目标字节地址 / 2
const jmp = (addr) => {
  if (addr & 1) throw new Error('jmp 目标非偶数：' + addr.toString(16));
  const w = addr >> 1;
  return Buffer.from([0x0C, 0x94, w & 0xFF, (w >> 8) & 0xFF]);
};

// 3. 拼回
const orig = readFileSync(origPath);
const out = Buffer.from(orig);
const patches = [
  { at: 0x283A, bytes: jmp(sym.T_disp),   desc: '模式名显示分发：模式 9 同模式 4' },
  { at: 0x3342, bytes: jmp(sym.T_emu),    desc: 'emumode.txt 接受数字 9' },
  { at: 0x5C34, bytes: jmp(sym.T_compat), desc: '模式兼容检查：模式 9 类型 15..24 合法' },
  { at: 0x6414, bytes: jmp(sym.T_init),   desc: '磁盘初始化：模式 9 置 5.25 标志' },
  { at: 0xC912, bytes: jmp(sym.T_size),   desc: 'NIB 大小识别：模式 9 认 266240' },
  { at: 0x3DAE, bytes: Buffer.from([0x8A]), expect: [0x89], desc: 'main 模式范围 cpi 9 → cpi 10' },
  { at: 0x3142, bytes: jmp(sym.T_menu),   desc: '菜单构建循环出口：追加第 7 项（模式 9）' },
  { at: 0x31F2, bytes: Buffer.from([0x80, 0x91, 0x06, 0x3B]), expect: [0x80, 0x91, 0xD9, 0x03], desc: '菜单项数读取改到 RAM 0x3B06' },
  { at: 0x32B4, bytes: jmp(sym.T_lookup), desc: '状态栏名称查找：模式 9 远地址绘名称' },
  // 菜单槽位新基址 0x3D2+行号（行 1–7 → 0x3D3–0x3D9）：原循环存行 1、2 与 PREV/NEXT 读槽的
  // subi 立即数同步改基址，否则行 2 槽被插入项覆盖、按键读错槽（跳项/读到 0x3DA 死机）。
  { at: 0x3114, bytes: Buffer.from([0xEE]), expect: [0xED], desc: '菜单行 1、2 存槽：subi 0x2D → 0x2E（槽 = 0x3D2+行）' },
  { at: 0x31C8, bytes: Buffer.from([0xEF]), expect: [0xEE], desc: 'PREV 读上一槽：subi 0x2E → 0x2F（读 0x3D1+下标）' },
  { at: 0x31FC, bytes: Buffer.from([0xEE]), expect: [0xED], desc: 'NEXT 读下一槽：subi 0x2D → 0x2E（ldd Z+1 落在 0x3D3+下标）' },
  // NIB 读头限幅：原固件钳在 140 四分之一磁道（35 道），模式 9 改走 160/159（40 道）限幅，
  // 否则金字塔 40 道盘的 35–39 道读写全落在 34 道附近（I/O ERROR、写坏盘）。
  { at: 0x8C62, bytes: jmp(sym.T_clampA), expect: [0x20, 0x81, 0x2C, 0x38], desc: 'NIB 读头限幅（主循环）：模式 9 改 40 道' },
  { at: 0x47C8, bytes: jmp(sym.T_clampB), expect: [0x80, 0x81, 0x8C, 0x38], desc: 'NIB 读头限幅（定时器中断）：模式 9 改 40 道' },
  // 写会话启动竞态（2026-11-27 真机取证）：0x35EB 位 2 在写会话启动时置位、主循环收尾时清零；
  // 新会话首半字节处若上一段收尾未完成，原固件置故障标志 0x381F 中止整段写入 →
  // 偶发 I/O ERROR，重试即成功。改为有界宽限（最多 200 半字节≈6.4ms），窗口耗尽
  // 仍走原故障路径报错，不掩盖持续性错误。替换 0x4B8C 的 lds r19,0x35EB（4 字节）。
  { at: 0x4B8C, bytes: jmp(sym.T_wstart), expect: [0x30, 0x91, 0xEB, 0x35], desc: '写会话启动：收尾未完成时有界宽限重试，超时仍报原错误' },
];
const changed = [];   // 记录所有被改动的字节偏移
const mark = (at, len) => { for (let i = 0; i < len; i++) changed.push(at + i); };
for (const p of patches) {
  if (p.expect) {
    for (let i = 0; i < p.expect.length; i++) {
      if (orig[p.at + i] !== p.expect[i]) {
        throw new Error(`补丁点 0x${p.at.toString(16)} 原字节不是预期值：0x${orig[p.at + i].toString(16)} ≠ 0x${p.expect[i].toString(16)}`);
      }
    }
  }
  p.bytes.copy(out, p.at);
  mark(p.at, p.bytes.length);
}
// 蹦床代码写入空闲区
code.copy(out, codeBase);
mark(codeBase, code.length);

// 第 7 项名称（首字符模式号 9，+1 起是显示名）
const item7 = Buffer.from('9 Pyramid 40-track\0', 'ascii');
item7.copy(out, ITEM7_AT);
mark(ITEM7_AT, item7.length);

// 字库副本（OLED 3 字节/字 @0x1EA3、非 OLED 5 字节/字 @0x1CB9，整段搬过去保持相对布局）
const font = orig.subarray(FONT_SRC, FONT_END);
font.copy(out, FONT_AT);
mark(FONT_AT, font.length);

// 克隆字符串绘制器：lpm r24,Z(0x9184) → elpm r24,Z(0x9186，无自增)，改两处字库基址，尺寸不变
const clone = Buffer.from(orig.subarray(DRAWER_SRC, DRAWER_END));
let lpmCount = 0;
for (let i = 0; i + 1 < clone.length; i += 2) {
  if (clone[i] === 0x84 && clone[i + 1] === 0x91) { clone[i] = 0x86; lpmCount++; }
}
if (lpmCount !== 5) throw new Error('克隆绘制器 lpm 站点数应为 5，实际 ' + lpmCount);
// ldi r26,lo ; ldi r27,hi 编码：1110 KKKK dddd KKKK，r26 d=0xA，r27 d=0xB
const ldiPair = (val) => {
  const lo = val & 0xFF, hi = (val >> 8) & 0xFF;
  const w26 = 0xE000 | ((lo >> 4) << 8) | (0xA << 4) | (lo & 0xF);
  const w27 = 0xE000 | ((hi >> 4) << 8) | (0xB << 4) | (hi & 0xF);
  return Buffer.from([w26 & 0xFF, w26 >> 8, w27 & 0xFF, w27 >> 8]);
};
ldiPair(oledBase).copy(clone, 0xE954 - DRAWER_SRC);
ldiPair(nonOledBase).copy(clone, 0xEA44 - DRAWER_SRC);
clone.copy(out, CLONE_AT);
mark(CLONE_AT, clone.length);

// 布局不重叠断言
if (codeBase + code.length > ITEM7_AT) throw new Error('蹦床代码与第 7 项数据重叠');
if (ITEM7_AT + item7.length > FONT_AT) throw new Error('第 7 项与字库副本重叠');
if (FONT_AT + font.length > CLONE_AT) throw new Error('字库副本与克隆绘制器重叠');

// 4. 校验
const errors = [];
if (out.length !== orig.length) errors.push(`长度变化：${orig.length} → ${out.length}`);
// 所有写入区（蹦床/第 7 项/字库副本/克隆）原本必须全 0
for (const [base, len, name] of [[codeBase, code.length, '蹦床'], [ITEM7_AT, item7.length, '第7项'], [FONT_AT, font.length, '字库'], [CLONE_AT, clone.length, '克隆']]) {
  for (let i = base; i < base + len; i++) {
    if (orig[i] !== 0) { errors.push(`${name}区 0x${i.toString(16)} 原本非 0（0x${orig[i].toString(16)}）`); break; }
  }
}
// 除改动点外逐字节一致
const changedSet = new Set(changed);
let diffOutside = 0;
for (let i = 0; i < orig.length; i++) {
  if (changedSet.has(i)) continue;
  if (out[i] !== orig[i]) { diffOutside++; if (diffOutside <= 5) errors.push(`未授权差异 0x${i.toString(16)}`); }
}
if (diffOutside) errors.push(`未授权差异共 ${diffOutside} 字节`);
// CPLD 段（0x20000 起）必须完全不动
for (let i = 0x20000; i < orig.length; i++) {
  if (out[i] !== orig[i]) { errors.push(`CPLD 段被改 0x${i.toString(16)}`); break; }
}
if (errors.length) {
  console.error('校验失败：\n' + errors.join('\n'));
  process.exit(1);
}

// 5. 输出
if (!existsSync(outDir)) mkdirSync(outDir, { recursive: true });
writeFileSync(outPath, out);

console.log('构建成功：' + outPath);
console.log(`蹦床代码 ${code.length} 字节 @ 0x${codeBase.toString(16).toUpperCase()}，原固件 ${orig.length} 字节，输出 ${out.length} 字节`);
console.log('补丁点：');
for (const p of patches) {
  console.log('  0x' + p.at.toString(16).toUpperCase() + '  ' + p.desc);
}
console.log(`菜单扩项：第7项 @0x${ITEM7_AT.toString(16).toUpperCase()}，字库副本 @0x${FONT_AT.toString(16).toUpperCase()}（${font.length}B），克隆绘制器 @0x${CLONE_AT.toString(16).toUpperCase()}（${clone.length}B，${lpmCount} 处 lpm→elpm）。`);
console.log('校验：未改动区域逐字节一致；CPLD 段不动；所有写入区原为全 0；总长不变。');