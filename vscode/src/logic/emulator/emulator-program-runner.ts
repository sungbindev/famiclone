// ------------------------------------------------------------------------
// 名称：emulator-program-runner.ts
// 说明：构建程序后在 Ember 里运行：目标的运行方案是 rom 时直接加载产物；是 firmware-disk（学习机）
//       时加载项目配置里的固件，把程序放进磁盘镜像挂到软驱并开机。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：自由运行时整机再在 SB-DOS 里输入程序名运行；调试运行时停在开机状态，由调试命令与 emu 工具继
//       续操作。Ember 的位置与调试端口来自扩展设置。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { BuildReporter, BuildService } from '../../contracts/build/build-service';
import type { EmulatorEnvironment } from '../../contracts/emulator/emulator-environment';
import type { ProgramRunner, RunProgramInput, RunProgramResult } from '../../contracts/emulator/emulator-operations';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { launchEmulator, type LaunchInput } from './emulator-launcher';
import { requireEmulatorExecutable } from './operation-context';

/** 构建程序并在 Ember 里运行。 */
export class EmulatorProgramRunner implements ProgramRunner {
  /**
   * @param buildService 构建服务。
   * @param env 外部能力。
   */
  constructor(
    private readonly buildService: BuildService,
    private readonly env: EmulatorEnvironment
  ) {}

  /** @inheritdoc */
  async run(input: RunProgramInput, reporter?: BuildReporter): Promise<RunProgramResult> {
    const { target, config } = this.buildService.plan(input.build);
    if (target.run === null) throw new FcdevError(`目标 ${target.name} 没有运行方案`, { code: 'TARGET_NO_RUN' });
    const firmwareDisk = target.run.mode === 'firmware-disk';
    if (firmwareDisk && config.firmware === undefined) {
      throw new FcdevError(`${target.name} 目标运行需要固件，fcdev.json 里没有设置 firmware`, {
        code: 'FIRMWARE_MISSING',
        hint: '在 fcdev.json 里用 firmware 指向学习机的 BIOS 文件（iNES 格式），例如 "firmware": "SB2K_BIOS.NES"'
      });
    }
    const executable = requireEmulatorExecutable(this.env);
    const port = this.env.settings.debugPort();

    const outcome = await this.buildService.build(input.build, reporter);
    let launch: Pick<LaunchInput, 'romFile' | 'sb2000'>;
    let description: string;
    if (firmwareDisk && config.firmware !== undefined) {
      const { imageFile } = this.buildService.buildDisk(outcome);
      const programName = path.basename(outcome.result.outputFile, path.extname(outcome.result.outputFile));
      launch = { sb2000: { firmware: config.firmware, diskImage: imageFile, programName } };
      description = `磁盘镜像 ${imageFile}（程序 ${programName}）`;
    } else {
      launch = { romFile: outcome.result.outputFile };
      description = outcome.result.outputFile;
    }
    const { pid, client } = await launchEmulator(this.env, { executable, ...launch, debug: input.debug, port });
    client?.close();
    return { pid, description, debug: input.debug, port: input.debug ? port : undefined, firmwareDisk };
  }
}
