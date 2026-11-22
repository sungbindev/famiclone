// ------------------------------------------------------------------------
// 名称：build-comparer.ts
// 说明：构建一个项目，并把产物与反汇编工程的原映像比较的外部能力接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：由实现层提供；构建的项目是用户或模型按生成的源码整理出来的 fcdev 项目，
//       不是反汇编工程目录本身。
// ------------------------------------------------------------------------

/** 构建产物与原映像的比较结果。 */
export interface BuildComparison {
  /** 构建出的文件路径。 */
  readonly builtFile: string;
  /** 构建出的字节数。 */
  readonly builtSize: number;
  /** 原映像的字节数。 */
  readonly imageSize: number;
  /** 两者逐字节一致。 */
  readonly identical: boolean;
  /** 第一个不同的文件偏移；一致时为 null。 */
  readonly firstDifference: number | null;
}

/** 构建并比较。 */
export interface BuildComparer {
  /**
   * 构建项目并与工程的原映像比较。
   * @param projectDir 反汇编工程目录，里面有 disasm.json。
   * @param buildTarget 要构建的 fcdev 项目目录，或其中的一个源文件。
   * @returns 比较结果。
   * @throws UserFacingError 没有安装 fcdev、构建失败、读不到工程或映像。
   */
  buildAndCompare(projectDir: string, buildTarget: string): Promise<BuildComparison>;
}
