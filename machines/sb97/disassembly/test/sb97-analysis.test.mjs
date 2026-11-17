// ------------------------------------------------------------------------
// 名称：sb97-analysis.test.mjs
// 说明：sb97-analysis.mjs 的单元测试：递归下降、访问方式、寄存器统计、页号立即数；
//       并用真实 BIOS 文件核对已经从反汇编得到的结论。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：真实文件的结论都是静态反汇编的推断，没有经过真机核对，见 docs/README.md。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { ROM_PATH } from '../tools/sb97-scan.mjs';
import { accessKind, findBankImmediates, formatListing, scanRegisterAccess, traceCode } from '../tools/sb97-analysis.mjs';
import { PAGE_SIZE, createCpuView, loadRom, pageVectors } from '../tools/sb97-rom.mjs';

/** 把一段机器码放在页首构造一页。 */
function pageWith(bytes) {
  const page = new Uint8Array(PAGE_SIZE);
  page.set(bytes);
  return page;
}

test('访问方式：读、写、读改写与不访问内存', () => {
  assert.equal(accessKind('LDA'), 'r');
  assert.equal(accessKind('STA'), 'w');
  assert.equal(accessKind('INC'), 'rw');
  assert.equal(accessKind('JMP'), null);
});

test('递归下降：沿 JSR 与分支走，在 RTS 后停止，不进入不可达字节', () => {
  // 8000: JSR $8006 ; RTS ; 8004: 垃圾字节 ; 8006: LDA #$01 ; RTS
  const page = pageWith([0x20, 0x06, 0x80, 0x60, 0xff, 0xff, 0xa9, 0x01, 0x60]);
  const read = (address) => page[address - 0x8000];
  const code = traceCode(read, [0x8000], (address) => address >= 0x8000 && address < 0xc000);
  assert.deepEqual([...code.keys()].sort((a, b) => a - b), [0x8000, 0x8003, 0x8006, 0x8008]);
});

test('寄存器统计：绝对寻址的读写分别计数，零页与范围外的地址不计', () => {
  // LDA $5604 ; STA $5501 ; INC $5000 ; STA $10 ; LDA $9000
  const page = pageWith([0xad, 0x04, 0x56, 0x8d, 0x01, 0x55, 0xee, 0x00, 0x50, 0x85, 0x10, 0xad, 0x00, 0x90]);
  const table = scanRegisterAccess([page], [7]);
  assert.deepEqual(table.get(0x5604), { read: 1, write: 0, pages: new Set([7]) });
  assert.deepEqual(table.get(0x5501), { read: 0, write: 1, pages: new Set([7]) });
  assert.deepEqual(table.get(0x5000), { read: 1, write: 1, pages: new Set([7]) });
  assert.equal(table.has(0x9000), false);
});

test('页号立即数：写 $5000 之前装入的立即数', () => {
  // LDA #$3F ; STA $C0 ; STA $5000 ; LDA $C1 ; STA $5000
  const page = pageWith([0xa9, 0x3f, 0x85, 0xc0, 0x8d, 0x00, 0x50, 0xa5, 0xc1, 0x8d, 0x00, 0x50]);
  const table = findBankImmediates([page], [0]);
  assert.deepEqual([...table.keys()], [0x3f]);
});

test('清单：可达指令一行一条，其余字节按 .byte 输出', () => {
  const page = pageWith([0xa9, 0x01, 0x60, 0xde, 0xad]);
  const read = (address) => page[address - 0x8000];
  const code = traceCode(read, [0x8000], (address) => address >= 0x8000 && address < 0x8005);
  const lines = formatListing(read, code, 0x8000, 5);
  assert.deepEqual(lines, ['8000  A9 01     LDA #$01', '8002  60        RTS', '8003  .byte $DE,$AD']);
});

test('真实 BIOS：第 0 页复位入口 $FF00 起是 CLC、CLD、SEI，之后调用 $D9BE 初始化', () => {
  const rom = loadRom(ROM_PATH);
  const view = createCpuView(rom, 1);
  const range = (address) => address >= 0xc000 && address <= 0xffff;
  const code = traceCode(view.read, [pageVectors(rom.pages[0]).reset], range);
  assert.deepEqual([0xff00, 0xff01, 0xff02].map((address) => code.get(address).info.mnemonic), ['CLC', 'CLD', 'SEI']);
  assert.equal(code.get(0xff06).target, 0xd9be);
  assert.ok(code.has(0xd9be));
});

test('真实 BIOS：只有第 0 页与第 63 页写 $5000，第 0 页的调度器经 $5000 选页', () => {
  const rom = loadRom(ROM_PATH);
  const numbers = [0, 1, 4, 5, 8, 9, 63];
  const table = scanRegisterAccess(numbers.map((index) => rom.pages[index]), numbers);
  assert.deepEqual([...table.get(0x5000).pages].sort((a, b) => a - b), [0, 63]);
  assert.ok(table.get(0x5000).write >= 100);
});

test('真实 BIOS：第 63 页按软驱控制器的方式访问 $5501（只写）、$5604 与 $5605（只读）', () => {
  const rom = loadRom(ROM_PATH);
  const table = scanRegisterAccess([rom.pages[63]], [63]);
  assert.equal(table.get(0x5501).read, 0);
  assert.ok(table.get(0x5501).write >= 10);
  assert.ok(table.get(0x5604).read >= 5);
  assert.equal(table.get(0x5604).write, 0);
  assert.ok(table.get(0x5605).read >= 3);
  assert.equal(table.get(0x5605).write, 0);
});

test('真实 BIOS：第 1 页的 $BFFD 是 JMP $BBA7（第 0 页的复位流程经 $5000=1 调用它）', () => {
  const rom = loadRom(ROM_PATH);
  const view = createCpuView(rom, 1);
  const ins = view.read(0xbffd);
  assert.equal(ins, 0x4c);
  assert.equal(view.read(0xbffe) | (view.read(0xbfff) << 8), 0xbba7);
});
