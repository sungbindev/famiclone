// ------------------------------------------------------------------------
// 名称：default-build-service.ts
// 说明：构建服务的实现：串起规划、工具链、编译链接与磁盘镜像；配置了 diskName 时生成镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：具体的文件系统、工具运行与资源位置都由组装根注入。
// ------------------------------------------------------------------------

import type { BuildOutcome, BuildService } from '../../contracts/build/build-service';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectConfigReader } from '../../contracts/project/project-config-reader';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { TargetRepository } from '../../contracts/targets/target-repository';
import type { ToolRunner, ToolchainLocator } from '../../contracts/toolchain/toolchain';
import type { BuildInput, BuildPlan, BuildReporter, DiskImageResult } from '../../shared/build/build-plan';
import { createBuildPlan } from './build-planner';
import { buildDiskImage } from './disk-builder';
import { buildProgram } from './program-builder';

/** 构建服务用到的能力。 */
export interface BuildServiceDependencies {
  /** 文件系统。 */
  readonly fileSystem: FileSystem;
  /** 项目配置读取。 */
  readonly configReader: ProjectConfigReader;
  /** 目标定义读取。 */
  readonly targets: TargetRepository;
  /** 工具链定位。 */
  readonly toolchainLocator: ToolchainLocator;
  /** 工具运行。 */
  readonly toolRunner: ToolRunner;
  /** 扩展资源位置。 */
  readonly resources: ExtensionResources;
}

/** 构建服务。 */
export class DefaultBuildService implements BuildService {
  /**
   * 创建服务。
   * @param dependencies 服务用到的能力。
   */
  constructor(private readonly dependencies: BuildServiceDependencies) {}

  /** @inheritdoc */
  plan(input: BuildInput): BuildPlan {
    return createBuildPlan(input, this.dependencies);
  }

  /** @inheritdoc */
  async build(input: BuildInput, reporter?: BuildReporter): Promise<BuildOutcome> {
    const plan = this.plan(input);
    const toolchain = this.dependencies.toolchainLocator.locate();
    const result = await buildProgram(plan, { ...this.dependencies, toolchain }, reporter);
    const outcome = { plan, result, disk: undefined };
    const { disk } = plan.target;
    // 需要底盘镜像的目标没有配置 diskImage 时只构建程序，运行时再提示缺底盘
    const wantsDisk = disk !== null && plan.config.diskName !== undefined && !(disk.baseImage && plan.config.diskImage === undefined);
    return wantsDisk ? { ...outcome, disk: this.buildDisk(outcome) } : outcome;
  }

  /** @inheritdoc */
  buildDisk(outcome: BuildOutcome): DiskImageResult {
    return buildDiskImage(this.dependencies.fileSystem, outcome.plan, outcome.result);
  }
}
