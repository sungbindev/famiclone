// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：emulator 目录的对外入口：定位 Ember（含随包 Ember 的安装）、启动 Ember、调试 SDK 终端客户端的定位、整机启动步骤，以及 Copilot 模拟器工具对应的操作（runEmulatorOperation）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
export { debugEnvironment, resolveDebugCli } from './debug-cli.mjs';
export { DEFAULT_DEBUG_PORT, assertPortFree, debugUserDataDirectory, launchEmulator } from './emulator-launcher.mjs';
export { EMBER_HOME_ENV_NAME, EMBER_PACKAGE_ENV_NAME, bundledEmberDirectory, compareVersions, installBundledEmber, userEmberDirectory } from './emulator-install.mjs';
export { EMULATOR_ENV_NAME, locateEmulator } from './emulator-locator.mjs';
export { DOS_PROMPT_FRAMES, SB2000_MACHINE, bootMachine, bootSb2000, enterProgram, keyCodeOf, runFrames, tapKey } from './sb2000-session.mjs';
export { EMULATOR_API_VERSION, EMULATOR_OPERATION_NAMES, runEmulatorOperation } from './operations/index.mjs';
