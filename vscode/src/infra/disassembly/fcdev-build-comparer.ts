// ------------------------------------------------------------------------
// 名称：fcdev-build-comparer.ts
// 说明：用项目里的 fcdev 构建一个项目，并把产物与反汇编工程记录的原映像逐字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：构建经命令行 fcdev build，产物路径取自它输出的“已生成 …”那一行；
//       原映像的路径取自 disasm.json，相对工程目录。
// ------------------------------------------------------------------------

import { readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';
import type { BuildComparer, BuildComparison } from '../../contracts/disassembly/build-comparer';
import type { FcdevCommandRunner } from '../../contracts/project/fcdev-command-runner';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { firstDifference } from '../../shared/disassembly/image-compare';

/** fcdev build 成功时输出的产物行：已生成 路径（……。 */
const BUILT_LINE = /^已生成 (.+?)（/m;

/** 读工程文件里记录的映像路径。 */
function readImagePath(projectDir: string): string {
  let text: string;
  try {
    text = readFileSync(path.join(projectDir, 'disasm.json'), 'utf8');
  } catch (error) {
    throw new UserFacingError(`读不到工程文件 ${path.join(projectDir, 'disasm.json')}`, error);
  }
  try {
    const parsed = JSON.parse(text) as { image?: { path?: unknown } };
    if (typeof parsed.image?.path === 'string') return path.resolve(projectDir, parsed.image.path);
  } catch {
    // 解析失败按下面的统一说明处理
  }
  throw new UserFacingError('工程文件里没有映像路径，它可能已损坏');
}

/** 经 fcdev 命令行构建并比较。 */
export class FcdevBuildComparer implements BuildComparer {
  /** @param runner 运行 fcdev 命令行。 */
  constructor(private readonly runner: FcdevCommandRunner) {}

  /** @inheritdoc */
  async buildAndCompare(projectDir: string, buildTarget: string): Promise<BuildComparison> {
    const imageFile = readImagePath(projectDir);
    let image: Buffer;
    try {
      image = readFileSync(imageFile);
    } catch (error) {
      throw new UserFacingError(`读不到映像文件 ${imageFile}`, error);
    }
    let directory: boolean;
    try {
      directory = statSync(buildTarget).isDirectory();
    } catch (error) {
      throw new UserFacingError(`找不到要构建的 ${buildTarget}`, error);
    }
    const cwd = directory ? buildTarget : path.dirname(buildTarget);
    const result = await this.runner.run(cwd, directory ? ['build'] : ['build', path.basename(buildTarget)]);
    if (result.exitCode !== 0) {
      throw new UserFacingError(`fcdev build 失败（退出码 ${result.exitCode}）：\n${(result.stderr.trim() || result.stdout.trim())}`);
    }
    const match = BUILT_LINE.exec(result.stdout);
    if (match?.[1] === undefined) throw new UserFacingError(`fcdev build 的输出里找不到产物路径：\n${result.stdout.trim()}`);
    const builtFile = path.resolve(cwd, match[1]);
    const built = readFileSync(builtFile);
    const difference = firstDifference(image, built);
    return { builtFile, builtSize: built.length, imageSize: image.length, identical: difference === null, firstDifference: difference };
  }
}
