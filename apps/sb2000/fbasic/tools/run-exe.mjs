// ------------------------------------------------------------------------
// 名称：run-exe.mjs
// 说明：在 Ember 的学习机里运行 FBASIC.EXE，按参数依次等待帧数、读内存、截图，用来核对移植版的运行状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/run-exe.mjs <步骤>...；步骤：w<帧数> 等待、m<地址十六进制>,<长度> 读内存并打印、k<键名> 按键（如 kEnter）、s<文件名> 截图存到 build/run/。
//       例：node tools/run-exe.mjs w120 mED,3 sboot.png；先用 node tools/build-exe.mjs 构建。
// ------------------------------------------------------------------------
import { join } from 'node:path';
import { startProgram } from './lib/sb2000-run.mjs';

const steps = process.argv.slice(2);
if (steps.length === 0) { console.error('用法：node tools/run-exe.mjs <步骤>...（w帧数、m地址,长度、k键名、s文件名）'); process.exit(2); }
const session = await startProgram();
try {
  for (const step of steps) {
    const kind = step[0];
    const rest = step.slice(1);
    if (kind === 'w') await session.frames(Number(rest));
    else if (kind === 'k') await session.tap(rest);
    else if (kind === 's') await session.shot(join(process.cwd(), 'build', 'run', rest));
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
