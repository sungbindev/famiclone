// ------------------------------------------------------------------------
// 名称：port.test.mjs
// 说明：移植生成器（tools/port.mjs）的测试：生成的 core.s 里没有 rti、没有留在原位的内核冲突地址，NMI 例程成对去掉了入口与出口的栈操作，库页里的跳板齐全。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：测试会重新生成 exe/src/core.s 与 layout.inc；不构建、不运行模拟器（运行验证见 tools/exe-run.mjs）。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
execFileSync('node', [join(ROOT, 'tools', 'port.mjs')], { cwd: ROOT, encoding: 'utf8' });
const source = readFileSync(join(ROOT, 'exe', 'src', 'core.s'), 'utf8').split(/\r?\n/);
const instructions = source.filter((line) => /^(\S+:)?\s+[a-z]{3}(\s|$)/.test(line) && !/^\s+\.\w+/.test(line));

test('NMI 例程用 rts 返回：没有 rti，入口的 pha 与出口的 pla 都去掉了', () => {
  assert.equal(instructions.filter((line) => /\brti\b/.test(line)).length, 0);
  const entry = source.findIndex((line) => line.startsWith('L8971:'));
  assert.ok(entry >= 0);
  assert.equal(source[entry].trim(), 'L8971:', 'L8971 的入口只留标签');
  assert.match(source[entry + 1], /lda\s+#\$00/);
});

test('原版的 $0600 页改到 fb_page6，$1E/$1F 改到 $38/$39', () => {
  const absolute = instructions.filter((line) => /\$06[0-9A-F]{2}\b/.test(line));
  assert.deepEqual(absolute, [], '指令里不应留下 $06xx 的绝对地址');
  assert.ok(instructions.some((line) => /fb_page6/.test(line)));
  const zeroPage = instructions.filter((line) => /\s\(?\$1[EF]\)?(,[xy])?\)?(,[xy])?\s*$/.test(line));
  assert.deepEqual(zeroPage, [], '指令里不应留下零页 $1E/$1F');
});

test('程序文本区 $6000-$7FFF 搬到 $B000-$CFFF，文本区的高字节立即数同步加偏移', () => {
  const left = instructions.filter((line) => /\$[67][0-9A-F]{3}\b/.test(line));
  assert.deepEqual(left, [], '指令里不应留下 $6000-$7FFF 的绝对地址');
  const count = (pattern) => instructions.filter((line) => pattern.test(line)).length;
  assert.ok(count(/\blda\s+#\$B0\b/) >= 2, '$8148、$84D8');
  assert.ok(count(/\blda\s+#\$BF\b/) >= 1, '$86A3');
  assert.ok(count(/\bcmp\s+#\$C0\b/) >= 1, '$97D7');
  assert.ok(count(/\blda\s+#\$C0\b/) >= 1, '$ADC8');
  assert.ok(count(/\bcmp\s+#\$BC\b/) >= 1, '$B1BD');
  assert.ok(count(/\blda\s+#\$BC\b/) >= 2, '$B1CA、$B20B');
});

test('PPU 寄存器访问都改调转换层，只剩 $2003（精灵地址）', () => {
  const direct = instructions.filter((line) => /\$20(0[0-2]|0[4-9])\b/.test(line));
  assert.deepEqual(direct, [], '指令里不应直接访问 $2000-$2002、$2004-$2008');
  const calls = (name) => instructions.filter((line) => new RegExp(`\\bjsr\\s+${name}\\b`).test(line)).length;
  assert.equal(calls('ppu_w_ctrl'), 5);
  assert.equal(calls('ppu_w_mask'), 8);
  assert.equal(calls('ppu_w_scroll'), 4);
  assert.equal(calls('ppu_w_addr'), 16);
  assert.equal(calls('ppu_w_addr_x'), 1);
  assert.equal(calls('ppu_w_data'), 23);
  assert.equal(calls('ppu_r_data'), 20);
  assert.equal(calls('ppu_r_status'), 2);
});

test('每个跳板的目标都在生成的源码里有标签，跳板与库页号成对', () => {
  const stubs = [...source.join('\n').matchAll(/^(L[0-9A-F]{4})_far:\n\s+jsr\s+far_enter\n\s+\.word\s+(L[0-9A-F]{4})\n\s+\.byte\s+(\d+)$/gm)];
  assert.ok(stubs.length > 100);
  const labels = new Set(source.filter((line) => /^L[0-9A-F]{4}:/.test(line)).map((line) => line.split(':')[0]));
  for (const [, name, target, bank] of stubs) {
    assert.equal(name, target);
    assert.ok(labels.has(target), `${target} 没有定义`);
    assert.ok(Number(bank) >= 1);
  }
});
