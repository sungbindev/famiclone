// ------------------------------------------------------------------------
// 名称：sb97-analysis.test.mjs
// 说明：SB97 静态分析的测试：用合成的小程序检查控制流、内嵌跳转表与字符串、常量追踪，再用真实 BIOS 核对已经查明的结论。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：合成程序放在第 1 页的 $8000 起，用 16K 窗口视图分析；真实 BIOS 的用例在文件不存在时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import { test } from 'node:test';
import { analyzeView, collectAccesses, summarizeAccesses } from '../tools/sb97-analysis.mjs';
import { renderListing } from '../tools/sb97-listing.mjs';
import { windowView } from '../tools/sb97-memory.mjs';
import { analyzeRom } from '../tools/sb97-pages.mjs';
import { DEFAULT_ROM_PATH, PAGE_SIZE, PRG_SIZE, loadRom } from '../tools/sb97-rom.mjs';

const hasRom = existsSync(DEFAULT_ROM_PATH);

/** 造一份只有第 1 页有内容的 BIOS：bytes 从第 1 页的开头写起。 */
function syntheticView(bytes, extra = {}) {
  const prg = new Uint8Array(PRG_SIZE);
  prg.set(bytes, PAGE_SIZE);
  for (const [offset, content] of Object.entries(extra)) prg.set(content, PAGE_SIZE + Number(offset));
  return windowView({ prg }, 1);
}

const mnemonicAt = (analysis, address) => analysis.instructions.get(address)?.info.mnemonic;

test('跟着 JSR、条件分支与 JMP 找代码，RTS 之后的字节不算代码', () => {
  const view = syntheticView([
    0x20, 0x10, 0x80, // 8000 JSR $8010
    0xd0, 0x01, // 8003 BNE $8006
    0x60, // 8005 RTS
    0x4c, 0x20, 0x80, // 8006 JMP $8020
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 8009 数据
    0xa9, 0x01, 0x60, // 8010 LDA #$01 / RTS
  ], { 0x20: [0xea, 0x60] });
  const analysis = analyzeView(view, [{ address: 0x8000 }]);
  assert.deepEqual([...analysis.instructions.keys()].sort((a, b) => a - b), [0x8000, 0x8003, 0x8005, 0x8006, 0x8010, 0x8012, 0x8020, 0x8021]);
  assert.ok(analysis.references.get(0x8010).has('jsr'));
  assert.ok(analysis.references.get(0x8006).has('branch'));
  assert.ok(analysis.references.get(0x8020).has('jmp'));
  assert.equal(analysis.rejected.length, 0);
});

test('遇到非官方指令、BRK 就停下并记下原因', () => {
  const view = syntheticView([0xea, 0xa7, 0x00]);
  const analysis = analyzeView(view, [{ address: 0x8000 }]);
  assert.equal(analysis.instructions.size, 1);
  assert.match(analysis.rejected[0].reason, /非官方指令 LAX/);
  const brk = analyzeView(syntheticView([0xea, 0x00, 0x00]), [{ address: 0x8000 }]);
  assert.match(brk.rejected[0].reason, /BRK/);
});

test('JSR 后面内嵌跳转表：表里的字当入口，表之后接着往下走', () => {
  const dispatcher = [0x0a, 0xa8, 0x68, 0x85, 0xaa, 0x68, 0x85, 0xab, 0x60]; // ASL A / TAY / PLA / STA $AA / PLA / STA $AB / RTS
  const view = syntheticView([
    0x20, 0x20, 0x80, // 8000 JSR $8020
    0x10, 0x80, 0x12, 0x80, // 8003 .word $8010,$8012
    0x60, // 8007 表之后的代码
  ], { 0x10: [0x60, 0x00, 0x60], 0x20: dispatcher });
  const analysis = analyzeView(view, [{ address: 0x8000 }]);
  assert.deepEqual([...analysis.tableWords.entries()], [[0x8003, 0x8010], [0x8005, 0x8012]]);
  assert.equal(mnemonicAt(analysis, 0x8007), 'RTS');
  assert.equal(mnemonicAt(analysis, 0x8010), 'RTS');
  assert.equal(mnemonicAt(analysis, 0x8012), 'RTS');
  assert.equal(analysis.instructions.has(0x8003), false);
});

test('JSR 后面内嵌以 $00 结尾的字符串：字符串不当代码', () => {
  const dispatcher = [0x68, 0x85, 0xaa, 0x68, 0x85, 0xab, 0x60];
  const view = syntheticView([
    0x20, 0x20, 0x80, // 8000 JSR $8020
    0x41, 0x42, 0x43, 0x00, // 8003 "ABC\0"
    0x60, // 8007 RTS
  ], { 0x20: dispatcher });
  const analysis = analyzeView(view, [{ address: 0x8000 }]);
  assert.equal(analysis.tableWords.size, 0);
  assert.equal(mnemonicAt(analysis, 0x8007), 'RTS');
  assert.equal(analysis.instructions.has(0x8003), false);
  assert.equal(analysis.rejected.length, 0);
});

test('常量追踪：立即数装入后写 $5000、$5200 能得到写入的值；跳转目标、JSR 之后与来源不明的值不追踪', () => {
  const view = syntheticView([
    0xa9, 0x80, // 8000 LDA #$80
    0x8d, 0x00, 0x50, // 8002 STA $5000
    0xa2, 0x03, // 8005 LDX #$03
    0x8e, 0x00, 0x52, // 8007 STX $5200
    0xa5, 0x44, // 800A LDA $44
    0x8d, 0x00, 0x50, // 800C STA $5000（A 不明）
    0xf0, 0x02, // 800F BEQ $8013
    0xa9, 0x01, // 8011 LDA #$01
    0x8d, 0x00, 0x50, // 8013 STA $5000（是分支目标，不追踪）
    0x60,
  ]);
  const accesses = collectAccesses(analyzeView(view, [{ address: 0x8000 }]));
  assert.deepEqual(accesses.map((item) => [item.address, item.value]), [[0x5000, 0x80], [0x5200, 0x03], [0x5000, undefined], [0x5000, undefined]]);
  const summary = summarizeAccesses(accesses);
  assert.deepEqual(summary.map((item) => [item.address, item.writes, item.values]), [[0x5000, 3, [0x80]], [0x5200, 1, [0x03]]]);
});

test('读、写与读后写回分开统计', () => {
  const view = syntheticView([0xad, 0x02, 0x20, 0x8d, 0x01, 0x20, 0xee, 0x00, 0x50, 0x60]);
  const accesses = collectAccesses(analyzeView(view, [{ address: 0x8000 }]));
  assert.deepEqual(accesses.map((item) => item.access), ['read', 'write', 'rw']);
});

test('清单：标签、指令、字表与数据行', () => {
  const dispatcher = [0x68, 0x85, 0xaa, 0x68, 0x85, 0xab, 0x60];
  const view = syntheticView([0x20, 0x20, 0x80, 0x10, 0x80, 0x60, 0xff, 0xff, 0x60, 0, 0, 0, 0, 0, 0, 0, 0x60], { 0x20: dispatcher });
  const text = renderListing(analyzeView(view, [{ address: 0x8000, name: 'start' }]), { title: '测试', first: 0x8000, last: 0x802f });
  assert.match(text, /^start:$/m);
  assert.match(text, /8000 {2}20 20 80 {2}JSR L_8020/);
  assert.match(text, /8003 {2}\.word L_8010/);
  assert.match(text, /\.byte \$FF,\$FF/);
});

test('真实 BIOS：第 0 页从复位向量、功能表与跳转桩出发分析得很干净，功能分派器在 $C0C2', { skip: !hasRom }, () => {
  const result = analyzeRom(loadRom());
  const { fixed } = result;
  assert.equal(fixed.rejected.length, 0);
  assert.equal(fixed.names.get(0xff00), 'reset');
  assert.equal(mnemonicAt(fixed, 0xff00), 'CLC');
  assert.equal(mnemonicAt(fixed, 0xc0c2), 'PHA');
  assert.equal(mnemonicAt(fixed, 0xffe0), 'JMP');
  assert.ok(fixed.codeBytes > 9000);
});

test('真实 BIOS：整机寄存器的访问统计与已查明的结论一致', { skip: !hasRom }, () => {
  const summary = summarizeAccesses(collectAccesses(analyzeRom(loadRom()).fixed));
  const at = (address) => summary.find((item) => item.address === address);
  // $5000 选程序页：功能分派器写的页号
  for (const page of [0x01, 0x05, 0x2f, 0x3f, 0x40, 0x80]) assert.ok(at(0x5000).values.includes(page), `$5000 应写过 $${page.toString(16)}`);
  // 键盘扫描：$4016 写 $05、$04、$06，读 $4017
  for (const value of [0x04, 0x05, 0x06]) assert.ok(at(0x4016).values.includes(value));
  assert.ok(at(0x4017).reads >= 1);
  // 语音：$5300 读状态、写结束字节 $0F
  assert.ok(at(0x5300).reads >= 1);
  assert.ok(at(0x5300).values.includes(0x0f));
  // 整机控制 $5200 只有写
  assert.equal(at(0x5200).reads, 0);
  assert.ok(at(0x5200).writes >= 1);
});

test('真实 BIOS：第 1、2、5、63 页被认作程序页，功能表入口取名 fn_XX', { skip: !hasRom }, () => {
  const result = analyzeRom(loadRom());
  for (const page of [1, 2, 5, 63]) assert.equal(result.pages[page].role, 'window', `第 ${page} 页`);
  assert.ok([...result.windows.get(1).names.values()].some((name) => name.startsWith('fn_C')));
  assert.ok([...result.windows.get(5).names.values()].some((name) => name.startsWith('fn_F')));
  assert.equal(result.pages[0].role, 'fixed');
});
