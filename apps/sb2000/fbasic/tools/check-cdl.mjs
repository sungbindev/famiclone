// ------------------------------------------------------------------------
// 名称：check-cdl.mjs
// 说明：核对分类与 CDL 是否一致：CDL 里被执行过的字节必须被分类为代码；被当数据读过的字节如果分类成了代码，要列出来（可能是夹在代码里的表）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/check-cdl.mjs；CDL 来自 analysis/prg.cdl，分类来自 lib/image-model.mjs。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { OPCODE, OPERAND, SKIP_PREFIX } from './lib/code-discovery.mjs';
import { BASE, ROOT, hex4, loadImageModel } from './lib/image-model.mjs';

const model = loadImageModel();
const cdl = readFileSync(join(ROOT, 'analysis', 'prg.cdl'));
const isCode = (offset) => [OPCODE, OPERAND, SKIP_PREFIX].includes(model.map.kinds[offset]);

const executedAsData = [];
const readAsData = [];
for (let offset = 0; offset < cdl.length; offset += 1) {
  if ((cdl[offset] & 1) !== 0 && !isCode(offset)) executedAsData.push(BASE + offset);
  if ((cdl[offset] & 2) !== 0 && isCode(offset) && (cdl[offset] & 1) === 0) readAsData.push(BASE + offset);
}
const runs = (addresses) => {
  const result = [];
  for (const address of addresses) {
    const last = result.at(-1);
    if (last && last.end + 1 === address) last.end = address; else result.push({ start: address, end: address });
  }
  return result.map((run) => `${hex4(run.start)}${run.end > run.start ? `-${hex4(run.end)}` : ''}`).join(' ');
};
console.log(`执行过却分类成数据的字节：${executedAsData.length}${executedAsData.length > 0 ? `：${runs(executedAsData)}` : ''}`);
console.log(`被当数据读过、却分类成代码的字节（没执行过）：${readAsData.length}${readAsData.length > 0 ? `：${runs(readAsData)}` : ''}`);
let both = 0;
for (let offset = 0; offset < cdl.length; offset += 1) if ((cdl[offset] & 3) === 3) both += 1;
console.log(`既执行过又当数据读过的字节：${both}`);
