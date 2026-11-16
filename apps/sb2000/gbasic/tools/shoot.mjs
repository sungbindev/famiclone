// ------------------------------------------------------------------------
// 名称：shoot.mjs
// 说明：在 Ember 里运行 GBASIC.EXE，按脚本依次等待帧数、按键并截图，用来逐步核对移植版的画面。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/shoot.mjs <标签> <步骤>...；步骤：w<帧数> 等待、k<键名> 按键（如 kKey1、kEnter）、s 截图（文件名 build/session/<标签>-<序号>.png）、m<地址十六进制>,<长度> 读内存并打印。
//       例：node tools/shoot.mjs menu w300 s kDigit1 w120 s
// ------------------------------------------------------------------------
import { join } from 'node:path';
import { ROOT } from './lib/image-model.mjs';
import { startGbasic } from './lib/sb2000-session.mjs';

const [label, ...steps] = process.argv.slice(2);
if (!label) { console.error('用法：node tools/shoot.mjs <标签> <步骤>...'); process.exit(2); }
const session = await startGbasic();
try {
  let shots = 0;
  for (const step of steps) {
    const kind = step[0];
    const rest = step.slice(1);
    if (kind === 'w') await session.frames(Number(rest));
    else if (kind === 'k') await session.tap(rest);
    else if (kind === 't') await session.type(rest.replaceAll('\\n', '\n').replaceAll('_', ' '));
    else if (kind === 's') { shots += 1; await session.shot(join(ROOT, 'build', 'session', `${label}-${shots}.png`)); }
    else if (kind === 'm') {
      const [address, length] = rest.split(',');
      const data = await session.read(Number.parseInt(address, 16), Number(length));
      console.log(`$${address}：${[...data].map((value) => value.toString(16).padStart(2, '0')).join(' ')}`);
    } else throw new Error(`不认识的步骤 ${step}`);
  }
  console.log('完成');
} finally {
  await session.close();
}
