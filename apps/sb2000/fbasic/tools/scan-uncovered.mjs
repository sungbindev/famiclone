// 列出 scan-immediates 的 A、B 候选里，两条指令都没有被 CDL 执行过的（这些靠分块置换测试验证不到，要人工看）
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const cdl = readFileSync(join(root, 'analysis', 'prg.cdl'));
const output = execFileSync('node', [join(root, 'tools', 'scan-immediates.mjs'), '--window', '16', '--all'], { encoding: 'utf8', maxBuffer: 1 << 26 });
const covered = (address) => (cdl[address - 0x8000] & 1) !== 0;
for (const line of output.split('\n')) {
  const match = /^([ABC]) \$([0-9A-F]{4})\s+(\w+) #\$([0-9A-F]{2}) @([0-9A-F]{4}) 低\s+\+\s+(\w+) #\$([0-9A-F]{2}) @([0-9A-F]{4}) 高/.exec(line);
  if (!match) continue;
  const low = Number.parseInt(match[5], 16);
  const high = Number.parseInt(match[8], 16);
  if (!covered(low) && !covered(high)) console.log(line);
}
