// ------------------------------------------------------------------------
// 名称：build-plan.ts
// 说明：构建的数据类型：构建输入、构建计划、构建结果与进度汇报。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：无
// ------------------------------------------------------------------------

import type { ProjectConfig } from '../project/project-config';
import type { Target, TargetOptionValue } from '../targets/target';

/** 构建输入。 */
export interface BuildInput {
  /** 构建路径（文件或目录），相对 cwd；不给时为 cwd。 */
  readonly pathArg?: string | undefined;
  /** 起始目录，相对路径以它为基准。 */
  readonly cwd: string;
  /** 覆盖 fcdev.json 里的目标。 */
  readonly targetOverride?: string | undefined;
}

/** 构建计划。 */
export interface BuildPlan {
  /** 构建单位：单个文件或整个目录。 */
  readonly mode: 'file' | 'directory';
  /** 源文件的公共根目录，用于生成中间文件名。 */
  readonly sourceBase: string;
  /** 源文件的绝对路径。 */
  readonly sources: readonly string[];
  /** 程序名（中间文件目录用）。 */
  readonly programName: string;
  /** 最终产物的绝对路径。 */
  readonly outputFile: string;
  /** 中间文件目录。 */
  readonly objectDir: string;
  /** 目标。 */
  readonly target: Target;
  /** 合并后的目标专用选项。 */
  readonly targetOptions: Readonly<Record<string, TargetOptionValue>>;
  /** 项目配置。 */
  readonly config: ProjectConfig;
}

/** 构建结果。 */
export interface BuildResult {
  /** 程序名。 */
  readonly programName: string;
  /** 产物路径。 */
  readonly outputFile: string;
  /** 产物大小。 */
  readonly size: number;
  /** 本次重新编译的源文件数。 */
  readonly compiled: number;
  /** 沿用上次结果的源文件数。 */
  readonly reused: number;
}

/** 磁盘镜像的生成结果。 */
export interface DiskImageResult {
  /** 镜像路径。 */
  readonly imageFile: string;
  /** 镜像大小。 */
  readonly size: number;
}

/** 构建进度的汇报方。 */
export interface BuildReporter {
  /** 进度信息。 */
  log(message: string): void;
  /** 工具的原始输出，原样透传（保持“文件(行): Error: 信息”格式供问题匹配器使用）。 */
  write(text: string): void;
}
