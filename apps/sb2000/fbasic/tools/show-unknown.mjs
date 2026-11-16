// 列出代码发现之后还没分类（既不是代码，也没写进 data-layout.txt）的区段的十六进制内容
// 用法 node tools/show-unknown.mjs [起始地址 [结束地址]]
import { UNKNOWN, hex } from './lib/code-discovery.mjs';
import { BASE } from './lib/image-model.mjs';
import { buildSeedMap } from './lib/seed-map.mjs';

const { bytes, map, inLayout } = buildSeedMap();
const [from = '0', to = 'FFFF'] = process.argv.slice(2);
const low = Number.parseInt(from, 16);
const high = Number.parseInt(to, 16);
let total = 0;
for (let offset = 0; offset < bytes.length;) {
  const address = BASE + offset;
  if (map.kinds[offset] !== UNKNOWN || inLayout(address)) { offset += 1; continue; }
  let last = offset;
  while (last + 1 < bytes.length && map.kinds[last + 1] === UNKNOWN && !inLayout(BASE + last + 1)) last += 1;
  total += last - offset + 1;
  if (BASE + last >= low && address <= high) {
    console.log(`--- ${hex(address)}-${hex(BASE + last)} (${last - offset + 1})`);
    for (let line = address; line <= BASE + last; line += 16) {
      const slice = bytes.subarray(line - BASE, Math.min(BASE + last + 1, line + 16) - BASE);
      console.log(`${hex(line)}  ${[...slice].map((value) => value.toString(16).toUpperCase().padStart(2, '0')).join(' ').padEnd(47)}  ${[...slice].map((value) => (value >= 0x20 && value < 0x7f ? String.fromCharCode(value) : '.')).join('')}`);
    }
  }
  offset = last + 1;
}
console.log(`未分类共 ${total} 字节，代码 ${map.codeBytes()} 字节`);
