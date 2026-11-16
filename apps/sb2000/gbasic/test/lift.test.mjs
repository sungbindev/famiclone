// ------------------------------------------------------------------------
// 名称：lift.test.mjs
// 说明：GBASIC 原版映像分析的测试：分类覆盖完整、跳过前缀与立即数地址有效、重新生成的源码汇编后与原映像逐字节一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：需要 apps/bbk/gbasic/disasm/gbasic.bin（原版映像）与 fcdev 的 cc65 工具链；不需要模拟器。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';
import { readAddressList } from '../tools/lib/analysis-files.mjs';
import { SKIP_PREFIX } from '../tools/lib/code-discovery.mjs';
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
  assert.ok(prefixes.length > 0);
  for (const address of prefixes) assert.equal(model.map.kinds[address - BASE], SKIP_PREFIX, `$${address.toString(16)} 没有被识别为跳过前缀`);
});

test('立即数里拆开的地址都落在立即数加载指令上', () => {
  const model = loadImageModel();
  assert.ok(model.immediates.size >= 60);
  for (const address of model.immediates.keys()) {
    const item = model.items[model.itemIndexAt(address)];
    assert.equal(item.start, address);
    assert.equal(item.type, 'code');
  }
});

test('重新生成的源码汇编后与原映像逐字节一致，整体平移后也能汇编', () => {
  const output = execFileSync('node', [join(ROOT, 'tools', 'lift.mjs')], { encoding: 'utf8' });
  assert.match(output, /逐字节一致/);
  const shifted = execFileSync('node', [join(ROOT, 'tools', 'lift.mjs'), '--base', '7C00'], { encoding: 'utf8' });
  assert.match(shifted, /已汇编/);
  // 平移那一次会覆盖 build 里的二进制，再生成一次保持 reference/gbasic-bbk.s 与分析结果同步
  execFileSync('node', [join(ROOT, 'tools', 'lift.mjs')], { encoding: 'utf8' });
  assert.ok(readFileSync(join(ROOT, 'reference', 'gbasic-bbk.s'), 'utf8').includes('.setcpu "6502"'));
});
