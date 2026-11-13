// ------------------------------------------------------------------------
// 名称：disasm-gbasic.mjs
// 说明：用 build/gbasic-trace.txt（运行时执行过的指令地址）加上人工确认的入口，重新反汇编 GBASIC，结果写到 disasm/（会覆盖该目录里的文件）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：先运行 node tools/trace-gbasic.mjs 生成 trace；入口 $7B00 是 CMD 的执行起点；
//       下面的 EXTRA_ENTRIES 是运行记录没覆盖到、但单独追踪后没有警告且有几十条指令的代码块起点。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const FCDEV = resolve(ROOT, '..', '..', '..', 'packages', 'fcdev', 'bin', 'fcdev.mjs');
const ORIGINAL_DIR = join(ROOT, 'build', 'original');
const TRACE = join(ROOT, 'build', 'gbasic-trace.txt');

/** 加载地址与执行起点 */
const LOAD_ADDRESS = '7B00';

/** 运行记录之外人工确认的入口 */
const EXTRA_ENTRIES = ['994D', '97F6', '94DB'];

// 输入文件名决定输出文件名，原版映像放在单独的目录里，避免和 run-gbasic.mjs 汇编出的 build/gbasic.bin 混淆
mkdirSync(ORIGINAL_DIR, { recursive: true });
const input = join(ORIGINAL_DIR, 'gbasic.bin');
writeFileSync(input, readFileSync(join(ROOT, 'GBASIC.CMD')).subarray(2));

const args = [FCDEV, 'disasm', input, '--format', 'binary', '--base', LOAD_ADDRESS, '--entry', LOAD_ADDRESS];
for (const entry of EXTRA_ENTRIES) args.push('--entry', entry);
args.push('--trace', TRACE, '--out', join(ROOT, 'disasm'));
execFileSync(process.execPath, args, { stdio: 'inherit' });
