// ------------------------------------------------------------------------
// 名称：scan-tables.mjs
// 说明：在还没分类的数据区段里找地址表：连续的 16 位字，每个字（或字减 1）都指向一条已知指令的起点。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/scan-tables.mjs [--min <连续个数，默认 3>]；输出每段表的起止、个数与是“目标”还是“目标减 1”（用于 RTS 分发），供人工写进 data-layout.txt。
// ------------------------------------------------------------------------
import { CodeMap, OPCODE, hex } from './lib/code-discovery.mjs';
import { BASE } from './lib/image-model.mjs';
import { buildSeedMap } from './lib/seed-map.mjs';

const args = process.argv.slice(2);
const minIndex = args.indexOf('--min');
const MIN_RUN = minIndex >= 0 ? Number(args[minIndex + 1]) : 3;
const { bytes, map, inLayout } = buildSeedMap();
const isStart = (address) => address >= BASE && address < BASE + bytes.length && map.kinds[address - BASE] === OPCODE;
const unknown = map.unknownRanges();
const inUnknown = (offset) => unknown.some((range) => BASE + offset >= range.start && BASE + offset <= range.end);

for (const delta of [0, 1]) {
  for (const parity of [0, 1]) {
    let run = [];
    const flush = () => {
      if (run.length >= MIN_RUN) console.log(`${delta === 0 ? '目标' : '目标-1'}  ${hex(run[0].at)}-${hex(run[run.length - 1].at + 1)}  ${run.length} 个字：${run.slice(0, 6).map((item) => hex(item.target)).join(' ')}${run.length > 6 ? ' ...' : ''}`);
      run = [];
    };
    for (let offset = parity; offset + 1 < bytes.length; offset += 2) {
      const word = bytes.readUInt16LE(offset);
      const target = (word + delta) & 0xffff;
      if (inUnknown(offset) && inUnknown(offset + 1) && isStart(target)) run.push({ at: BASE + offset, target });
      else flush();
    }
    flush();
  }
}
