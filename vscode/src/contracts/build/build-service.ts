// ------------------------------------------------------------------------
// 名称：build-service.ts
// 说明：构建服务的接口：解析构建路径、构建程序、生成磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { BuildInput, BuildPlan, BuildReporter, BuildResult, DiskImageResult } from '../../shared/build/build-plan';

export type { BuildInput, BuildPlan, BuildReporter, BuildResult, DiskImageResult } from '../../shared/build/build-plan';

/** 构建一个程序的结果，连同使用的计划。 */
export interface BuildOutcome {
  /** 构建计划。 */
  readonly plan: BuildPlan;
  /** 构建结果。 */
  readonly result: BuildResult;
}

/** 构建服务。 */
export interface BuildService {
  /**
   * 解析构建路径，得到构建计划，不运行任何工具。
   * @param input 构建输入。
   * @returns 构建计划。
   * @throws FcdevError 路径不存在、没有指定目标、文件不是源文件等。
   */
  plan(input: BuildInput): BuildPlan;

  /**
   * 构建一个程序。
   * @param input 构建输入。
   * @param reporter 进度汇报。
   * @returns 计划与结果。
   * @throws FcdevError 编译、链接或打包失败。
   */
  build(input: BuildInput, reporter?: BuildReporter): Promise<BuildOutcome>;

  /**
   * 把已构建的程序放进磁盘镜像，镜像放在产物旁边，文件名与程序名相同。
   * @param outcome 构建的计划与结果。
   * @returns 镜像路径与大小。
   * @throws FcdevError 目标不支持磁盘镜像，或程序名不符合镜像文件名规则。
   */
  buildDisk(outcome: BuildOutcome): DiskImageResult;
}
