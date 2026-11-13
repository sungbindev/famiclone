/*
 * ------------------------------------------------------------------------
 * 名称：setup.ts
 * 说明：TypeScript 测试的初始化：给 jsdom 补上组件库用到、jsdom 不实现的浏览器接口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：jsdom 没有排版：ResizeObserver 只做空实现，测试里不触发回调。
 * ------------------------------------------------------------------------
 */

if (typeof window !== 'undefined' && !('ResizeObserver' in window)) {
  Object.assign(window, {
    ResizeObserver: class {
      observe(): void {}
      unobserve(): void {}
      disconnect(): void {}
    }
  });
}
