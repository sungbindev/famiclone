// ------------------------------------------------------------------------
// 名称：build-exe.mjs
// 说明：构建 GBASIC.EXE：先链接映像（build-image.mjs），再用 fcdev 构建外壳程序（exe/）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/build-exe.mjs；输出 exe/build/sb2000/GBASIC.EXE。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { join } from 'node:path';
import { ROOT } from './lib/image-model.mjs';

const steps = [
  { cwd: ROOT, args: [join(ROOT, 'tools', 'build-image.mjs')] },
  { cwd: join(ROOT, 'exe'), args: [join(ROOT, '..', '..', '..', 'packages', 'fcdev', 'bin', 'fcdev.mjs'), 'build', 'src'] },
];
for (const step of steps) {
  const result = spawnSync(process.execPath, step.args, { cwd: step.cwd, encoding: 'utf8' });
  process.stdout.write(result.stdout ?? '');
  process.stderr.write(result.stderr ?? '');
  if (result.status !== 0) process.exit(result.status ?? 1);
}
