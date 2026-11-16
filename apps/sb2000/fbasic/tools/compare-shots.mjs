// 比较两个目录里同名 PNG 截图是否逐字节相同。用法 node tools/compare-shots.mjs <目录甲> <目录乙>
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const [first, second] = process.argv.slice(2);
let same = 0;
const different = [];
for (const name of readdirSync(first).filter((file) => file.endsWith('.png')).sort()) {
  if (!existsSync(join(second, name))) { different.push(`${name}（乙里没有）`); continue; }
  if (readFileSync(join(first, name)).equals(readFileSync(join(second, name)))) same += 1;
  else different.push(name);
}
console.log(`相同 ${same} 张，不同 ${different.length} 张${different.length > 0 ? `：${different.join('、')}` : ''}`);
process.exit(different.length > 0 ? 1 : 0);
