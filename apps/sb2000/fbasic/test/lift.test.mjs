// ------------------------------------------------------------------------
// 名称：lift.test.mjs
// 说明：Family BASIC v3 PRG 分析的测试：分类覆盖完整、跳过前缀与立即数地址有效、CDL 与分类一致、重新生成的源码汇编后与原 PRG 逐字节一致，重排后的源码也能汇编。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：需要 reference/nes-games 里的 Family BASIC ROM 与 fcdev 的 cc65 工具链；不需要模拟器（重排后的行为比较要用 Ember，见 tools/record-coverage.mjs）。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';
import { readAddressList } from '../tools/lib/analysis-files.mjs';
import { OPCODE, OPERAND, SKIP_PREFIX } from '../tools/lib/code-discovery.mjs';
import { BASE, loadImageModel } from '../tools/lib/image-model.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');

test('每个字节恰好属于代码或一段数据，条目首尾相接', () => {
  const model = loadImageModel();
  let next = BASE;
  for (const item of model.items) {
    assert.equal(item.start, next, `条目 $${item.start.toString(16)} 与上一个条目之间有空隙或重叠`);
    next = item.start + item.size;
  }
  assert.equal(next, BASE + model.bytes.length);
});

test('跳过前缀都被当作一字节的伪指令', () => {
  const model = loadImageModel();
  const prefixes = readAddressList(join(ROOT, 'analysis', 'skip-prefixes.txt'));
  assert.equal(prefixes.length, 16);
  for (const address of prefixes) assert.equal(model.map.kinds[address - BASE], SKIP_PREFIX, `$${address.toString(16)} 没有被识别为跳过前缀`);
});

test('立即数里拆开的地址都落在立即数加载指令上', () => {
  const model = loadImageModel();
  assert.ok(model.immediates.size >= 40);
  for (const address of model.immediates.keys()) {
    const item = model.items[model.itemIndexAt(address)];
    assert.equal(item.start, address);
    assert.equal(item.type, 'code');
  }
});

test('运行时 CDL 里执行过的字节都被分类为代码', () => {
  const model = loadImageModel();
  const cdl = readFileSync(join(ROOT, 'analysis', 'prg.cdl'));
  assert.ok(cdl.length === model.bytes.length);
  for (let offset = 0; offset < cdl.length; offset += 1) {
    if ((cdl[offset] & 1) === 0) continue;
    assert.ok([OPCODE, OPERAND, SKIP_PREFIX].includes(model.map.kinds[offset]), `$${(BASE + offset).toString(16)} 执行过却不是代码`);
  }
});

test('重新生成的源码汇编后与原 PRG 逐字节一致，重排后的源码也能汇编', () => {
  const output = execFileSync('node', [join(ROOT, 'tools', 'lift.mjs')], { encoding: 'utf8' });
  assert.match(output, /逐字节一致/);
  const permuted = execFileSync('node', [join(ROOT, 'tools', 'lift.mjs'), '--permute', '3'], { encoding: 'utf8' });
  assert.match(permuted, /已生成重排的 ROM/);
  assert.ok(readFileSync(join(ROOT, 'reference', 'fbasic-nes.s'), 'utf8').includes('.setcpu "6502"'));
});
