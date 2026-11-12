// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：toolchain 目录的对外入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
export {
  TOOLCHAIN_ENV_NAME,
  TOOLCHAIN_PACKAGE_NAME,
  getToolPath,
  getToolchainVersion,
  locateToolchain,
  runTool,
} from './toolchain-locator.mjs';
