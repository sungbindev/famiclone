// ------------------------------------------------------------------------
// 名称：seed-map.mjs
// 说明：用已知的种子建立代码分类图：CDL 里的指令起点、人工确认的入口，再加上地址表指向的代码，反复追踪到不再增加。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：由 image-model 使用，保证各工具看到的分类一致。
//       地址表里的字（layout 里 addrs 与 rec 的 a 元素）如果指向映像内、不在数据区段里的未分类字节，就当作代码入口。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { readAddressList } from './analysis-files.mjs';
import { CodeMap, UNKNOWN } from './code-discovery.mjs';
import { BASE, ROOT, readRom } from './rom.mjs';
import { dataElements, readLayout } from './layout.mjs';

const analysis = (name) => join(ROOT, 'analysis', name);
const readList = (name) => (existsSync(analysis(name)) ? readAddressList(analysis(name)) : []);

/**
 * 建立种子分类图。
 * @returns {{bytes: Buffer, map: CodeMap, layout: object[], inLayout: (address: number) => boolean, tableTargets: number[]}} PRG 字节、分类图、数据布局、是否在布局里的判断、从地址表追到的入口
 */
export function buildSeedMap() {
  const bytes = readRom().prg;
  const layout = readLayout(analysis('data-layout.txt'));
  const inLayout = (address) => layout.some((range) => address >= range.start && address <= range.end);
  const map = new CodeMap(bytes, BASE, new Set(readList('skip-prefixes.txt')));
  for (const entry of [...readList('cdl-entries.txt'), ...readList('code-entries.txt')]) if (!inLayout(entry)) map.trace([entry], { strict: false, commit: true });
  const tableTargets = [];
  for (let changed = true; changed;) {
    changed = false;
    for (const range of layout) {
      if (range.kind !== 'addrs' && range.kind !== 'rec') continue;
      for (const element of dataElements(range)) {
        if (element.kind !== 'a') continue;
        const target = bytes.readUInt16LE(element.start - BASE);
        if (target < BASE || target >= BASE + bytes.length || inLayout(target) || map.kinds[target - BASE] !== UNKNOWN) continue;
        if (map.trace([target], { strict: false, commit: true }).count > 0) { tableTargets.push(target); changed = true; }
      }
    }
  }
  return { bytes, map, layout, inLayout, tableTargets };
}
