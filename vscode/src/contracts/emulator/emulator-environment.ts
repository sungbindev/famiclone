// ------------------------------------------------------------------------
// 名称：emulator-environment.ts
// 说明：模拟器操作要用到的外部能力：文件系统、目标描述读取、调试连接、设置、进程启动。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：操作的实现只经这些接口访问外部，测试时换成假的。
// ------------------------------------------------------------------------

import type { DebugConnector } from '../debug-client/debug-connector';
import type { FileSystem } from '../files/file-system';
import type { TargetProfileReader } from '../targets/target-profile-reader';
import type { EmulatorSettings } from './emulator-settings';
import type { EmulatorProcessLauncher } from './process-launcher';

/** 模拟器操作的运行环境。 */
export interface EmulatorEnvironment {
  /** 文件系统。 */
  readonly files: FileSystem;
  /** 目标描述的读取。 */
  readonly profiles: TargetProfileReader;
  /** 连接 Ember 调试服务。 */
  readonly connector: DebugConnector;
  /** 模拟器设置。 */
  readonly settings: EmulatorSettings;
  /** 启动与结束 Ember 进程。 */
  readonly processes: EmulatorProcessLauncher;
}
