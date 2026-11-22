// ------------------------------------------------------------------------
// 名称：disasm-environment.ts
// 说明：反汇编操作需要的外部能力：文件系统、目标描述读取、工具链定位与工具运行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：反汇编操作都经它读写工程目录与调用汇编、链接工具，测试时换成临时目录与替身。
// ------------------------------------------------------------------------

import type { FileSystem } from '../files/file-system';
import type { TargetProfileReader } from '../targets/target-profile-reader';
import type { ToolRunner, ToolchainLocator } from '../toolchain/toolchain';

/** 反汇编操作的运行环境。 */
export interface DisasmEnvironment {
  /** 读写工程目录、映像、符号集与生成的源码。 */
  readonly files: FileSystem;
  /** 读取目标描述（profile.json）。 */
  readonly profiles: TargetProfileReader;
  /** 定位随扩展发布的 cc65 工具链。 */
  readonly toolchain: ToolchainLocator;
  /** 运行 ca65、ld65。 */
  readonly tools: ToolRunner;
}
