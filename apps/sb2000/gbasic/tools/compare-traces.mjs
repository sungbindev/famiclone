// ------------------------------------------------------------------------
// 名称：compare-traces.mjs
// 说明：比较两次 run-bbk.mjs --trace-line 记录的指令轨迹，找出第一处执行流程不同的位置，用来发现后移版本里遗漏的地址常量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/compare-traces.mjs <标签甲> <标签乙> --shift <十六进制字节数>；乙是后移版本：$8000 及以上的地址减去偏移后再与甲比较；
//       $7B00-$7FFF 的常驻部分与映像之外的地址（BIOS、BBGDOS）不换算。每行比较地址与 A、X、Y、S、P 寄存器，寄存器里的地址类数值（如指针的高字节）不同时也会报告，需要人工判断。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const args = process.argv.slice(2);
const [labelA, labelB] = args;
const SHIFT = Number.parseInt(args[args.indexOf('--shift') + 1], 16);
const IMAGE_END = 0xe410;

const load = (label) => readFileSync(join(ROOT, 'build', `trace-${label}.txt`), 'utf8').split('\n').filter((line) => line.startsWith('T '));
const a = load(labelA);
const b = load(labelB);
const pattern = /^T ([0-9A-F]{4}) \S+ ([0-9A-F]{2}) ([0-9A-F]{2}) ([0-9A-F]{2}) A=([0-9A-F]{2}) X=([0-9A-F]{2}) Y=([0-9A-F]{2}) S=([0-9A-F]{2}) P=([0-9A-F]{2})/;
const parse = (line) => {
  const match = pattern.exec(line);
  return match === null ? null : { pc: Number.parseInt(match[1], 16), opcode: match[2], regs: match.slice(5, 10).join(' ') };
};
const unshift = (pc) => (pc >= 0x8000 + SHIFT && pc < IMAGE_END + SHIFT ? pc - SHIFT : pc);

console.log(`${labelA}：${a.length} 行；${labelB}：${b.length} 行`);
const count = Math.min(a.length, b.length);
let firstRegisterDifference = -1;
for (let index = 0; index < count; index += 1) {
  const left = parse(a[index]);
  const right = parse(b[index]);
  if (left === null || right === null) continue;
  if (left.pc !== unshift(right.pc) || left.opcode !== right.opcode) {
    console.log(`第 ${index + 1} 行起流程不同：\n  甲 ${a[index]}\n  乙 ${b[index]}`);
    for (let before = Math.max(0, index - 8); before < index; before += 1) console.log(`  之前 甲 ${a[before]}\n       乙 ${b[before]}`);
    if (firstRegisterDifference >= 0) console.log(`第一处寄存器不同在第 ${firstRegisterDifference + 1} 行：\n  甲 ${a[firstRegisterDifference]}\n  乙 ${b[firstRegisterDifference]}`);
    process.exit(1);
  }
  if (firstRegisterDifference < 0 && left.regs !== right.regs) firstRegisterDifference = index;
}
console.log(`前 ${count} 行流程一致${firstRegisterDifference >= 0 ? `；第一处寄存器不同在第 ${firstRegisterDifference + 1} 行（后移后地址类数值不同属正常）` : '，寄存器也一致'}`);
