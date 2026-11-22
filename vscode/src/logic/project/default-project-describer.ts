// ------------------------------------------------------------------------
// 名称：default-project-describer.ts
// 说明：项目描述的实现：读取项目配置与目标，定位随扩展发布的工具链，汇总成项目描述。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：从 cwd 向上查找 fcdev.json；没有配置时必须指定目标。
// ------------------------------------------------------------------------

import type { ProjectConfigReader } from '../../contracts/project/project-config-reader';
import type { ProjectDescriber, ProjectDescription } from '../../contracts/project/project-describer';
import type { TargetRepository } from '../../contracts/targets/target-repository';
import type { ToolchainLocator } from '../../contracts/toolchain/toolchain';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { describeProject } from '../../shared/project/project-info';

/** 由配置、目标与工具链汇总项目描述。 */
export class DefaultProjectDescriber implements ProjectDescriber {
  /**
   * 创建描述器。
   * @param configReader 项目配置读取。
   * @param targets 目标定义读取。
   * @param toolchainLocator 工具链定位。
   */
  constructor(
    private readonly configReader: ProjectConfigReader,
    private readonly targets: TargetRepository,
    private readonly toolchainLocator: ToolchainLocator
  ) {}

  /** @inheritdoc */
  describe(input: { readonly cwd: string; readonly target?: string | undefined }): ProjectDescription {
    const config = this.configReader.read(input.cwd, input.cwd);
    const targetName = input.target ?? config.target;
    if (targetName === undefined) {
      throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或指定 target' });
    }
    const target = this.targets.load(targetName);
    return describeProject(config, target, this.toolchainLocator.locate().home);
  }
}
