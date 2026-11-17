/*
 * ------------------------------------------------------------------------
 * 名称：default-bios.test.ts
 * 说明：随安装包带的默认 BIOS（bios/<机型>/）的测试：每个机型一个文件、大小等于机型的 PRG 容量、没有 iNES 文件头。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：与核心的 MachineManager 里各机型的 PRG 容量一致；原始资料在仓库的 reference/rom，这里只在文件存在时核对内容相同。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, expect, it } from 'vitest';

const ROOT = fileURLToPath(new URL('..', import.meta.url));
const BIOS_ROOT = join(ROOT, 'bios');
/** 机型、默认文件名、PRG 容量（字节）与原始资料（相对 reference/rom）。 */
const DEFAULTS = [
  { machine: 'sb97', name: 'SB97.bin', size: 1024 * 1024, source: '小霸王/SB97.nes' },
  { machine: 'sb2000', name: 'SB2K_BIOS.bin', size: 512 * 1024, source: '小霸王/SB2K_BIOS.NES' },
  { machine: 'bbk10', name: 'bbk_bios10.bin', size: 128 * 1024, source: '步步高/bbk_bios10.nes' },
  { machine: 'bbk20', name: 'bbk_bios20.bin', size: 2048 * 1024, source: '步步高/bbk_bios20.nes' }
];

describe('默认 BIOS', () => {
  it('bios 目录下只有这四个机型，每个机型只有一个文件', () => {
    expect(readdirSync(BIOS_ROOT).toSorted()).toEqual(DEFAULTS.map((item) => item.machine).toSorted());
    for (const item of DEFAULTS) expect(readdirSync(join(BIOS_ROOT, item.machine))).toEqual([item.name]);
  });

  for (const item of DEFAULTS) {
    it(`${item.machine}：大小等于 PRG 容量，没有 iNES 文件头`, () => {
      const file = readFileSync(join(BIOS_ROOT, item.machine, item.name));
      expect(file.length).toBe(item.size);
      expect(file.subarray(0, 4).toString('latin1')).not.toBe('NES\x1a');
    });

    it(`${item.machine}：内容与原始资料的 PRG 相同`, () => {
      const source = join(ROOT, '..', 'reference', 'rom', item.source);
      if (!existsSync(source)) return;
      const original = readFileSync(source);
      expect(readFileSync(join(BIOS_ROOT, item.machine, item.name)).equals(original.subarray(16, 16 + item.size))).toBe(true);
    });
  }
});
