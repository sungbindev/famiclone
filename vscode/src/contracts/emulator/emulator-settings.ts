// ------------------------------------------------------------------------
// 名称：emulator-settings.ts
// 说明：模拟器相关设置的读取接口：Ember 可执行文件的位置与调试端口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：位置与端口由使用者在扩展设置里给出（fcdev.emulator.path、fcdev.emulator.debugPort），不随扩
//       展分发 Ember，也不从项目文件或环境变量里找；每次使用时读取，设置改了立即生效。
// ------------------------------------------------------------------------

/** 默认的调试端口。 */
export const DEFAULT_DEBUG_PORT = 5699;

/** 模拟器相关的设置。 */
export interface EmulatorSettings {
  /**
   * Ember 可执行文件的路径。
   * @returns 路径；没有设置时为 undefined。
   */
  executablePath(): string | undefined;

  /**
   * 调试运行用的端口。
   * @returns 端口，没有设置时为 DEFAULT_DEBUG_PORT。
   */
  debugPort(): number;
}
