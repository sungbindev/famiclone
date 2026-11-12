// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：emulator 目录的对外入口：定位模拟器、启动模拟器、调试服务客户端。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
export { DEFAULT_DEBUG_PORT, DebugClient, parseReply } from './debug-client.mjs';
export { assertPortFree, launchEmulator } from './emulator-launcher.mjs';
export { EMULATOR_ENV_NAME, EMULATOR_PACKAGE_NAME, locateEmulator } from './emulator-locator.mjs';
