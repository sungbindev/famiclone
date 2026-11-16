// ------------------------------------------------------------------------
// 名称：compare-shots.mjs
// 说明：比较两次 run-bbk.mjs 运行产生的截图：同名截图逐字节相同才算行为一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/compare-shots.mjs <标签甲> <标签乙>；退出码 0 表示全部相同，1 表示有差异或缺图。
// ------------------------------------------------------------------------
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const [labelA, labelB] = process.argv.slice(2);
if (!labelA || !labelB) {
  console.error('用法：node tools/compare-shots.mjs <标签甲> <标签乙>');
  process.exit(2);
}
const dirA = join(ROOT, 'build', `shots-${labelA}`);
const dirB = join(ROOT, 'build', `shots-${labelB}`);
const names = [...new Set([...readdirSync(dirA), ...readdirSync(dirB)])].filter((name) => name.endsWith('.png')).sort();
let different = 0;
for (const name of names) {
  const a = join(dirA, name);
  const b = join(dirB, name);
  if (!existsSync(a) || !existsSync(b)) { console.log(`${name}：只在一边有`); different += 1; continue; }
  const same = readFileSync(a).equals(readFileSync(b));
  console.log(`${name}：${same ? '相同' : '不同'}`);
  if (!same) different += 1;
}
console.log(different === 0 ? `全部 ${names.length} 张截图相同` : `有 ${different} 张不同或缺失`);
process.exit(different === 0 ? 0 : 1);
