/*
 * ------------------------------------------------------------------------
 * 名称：build-info.d.ts
 * 说明：虚拟模块 virtual:ember-build-info 的类型：编译时间（本机时间，格式 YYYY-MM-DD HH:mm:ss）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：模块由 tools/build/build-info.mjs 的 Vite 插件在构建与测试时生成。
 * ------------------------------------------------------------------------
 */

declare module 'virtual:ember-build-info' {
  const buildTime: string;
  export default buildTime;
}
