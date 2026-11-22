// ------------------------------------------------------------------------
// 名称：build-task-runner.ts
// 说明：执行一个 fcdev 任务：构建（同时生成磁盘镜像）、构建后在 Ember 里运行，进度与结果写给
//       调用者，返回退出码。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：工具（cc65、ca65、ld65）的输出原样透传，保持“文件(行): Error: 信息”的格式，
//       供问题匹配器 $fcdev 识别；不依赖 vscode，可直接在测试里运行。
// ------------------------------------------------------------------------

import type { BuildService } from '../../contracts/build/build-service';
import type { ProgramRunner } from '../../contracts/emulator/emulator-operations';
import { describeBuildResult, describeDiskImage, type FcdevTaskDefinition } from '../../shared/build/task-definition';
import { describeRunResult } from '../../shared/emulator/run-text';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 执行任务要用到的能力。 */
export interface BuildTaskDependencies {
  /** 构建服务。 */
  readonly buildService: BuildService;
  /** 构建后在 Ember 里运行。 */
  readonly runner: ProgramRunner;
}

/**
 * 执行构建或运行的任务。
 * @param definition 任务定义。
 * @param folder 工作区文件夹的绝对路径，任务里的路径相对它。
 * @param dependencies 要用到的能力。
 * @param write 输出文字，调用者负责换行转换。
 * @returns 退出码：0 成功，1 失败。
 */
export async function runBuildTask(
  definition: FcdevTaskDefinition,
  folder: string,
  dependencies: BuildTaskDependencies,
  write: (text: string) => void
): Promise<number> {
  const { buildService, runner } = dependencies;
  const input = { pathArg: definition.path, cwd: folder, targetOverride: definition.target };
  const reporter = { log: (message: string) => write(`${message}\n`), write };
  try {
    if (definition.command === 'run') {
      write(`${describeRunResult(await runner.run({ build: input, debug: definition.debug === true }, reporter))}\n`);
      return 0;
    }
    const outcome = await buildService.build(input, reporter);
    write(`${describeBuildResult(outcome.result, outcome.plan.target.name, folder)}\n`);
    if (outcome.disk !== undefined) {
      write(`${describeDiskImage(outcome.disk, folder)}\n`);
    }
    return 0;
  } catch (error) {
    if (!(error instanceof UserFacingError)) throw error;
    write(`错误：${error.message}\n`);
    if (error instanceof FcdevError && error.hint !== '') write(`${error.hint}\n`);
    return 1;
  }
}
