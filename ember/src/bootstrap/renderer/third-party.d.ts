/*
 * ------------------------------------------------------------------------
 * 名称：third-party.d.ts
 * 说明：虚拟模块 virtual:ember-third-party 的类型：使用的第三方开源代码清单（不含许可全文）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：模块由 tools/third-party/third-party.mjs 的 Vite 插件在构建与测试时生成。
 * ------------------------------------------------------------------------
 */

declare module 'virtual:ember-third-party' {
  const notices: readonly {
    readonly name: string;
    readonly version: string;
    readonly authors: readonly string[];
    readonly license: string;
    readonly homepage: string;
    readonly usage: string;
  }[];
  export default notices;
}
