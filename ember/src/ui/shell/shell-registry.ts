/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry.ts
 * 说明：外壳登记项的类型：各模块向外壳登记自己的页面，外壳按清单顺序显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：登记项只在渲染进程的组装根（bootstrap/renderer/module-list.ts）里汇总；外壳不认识具体的模块。
 * ------------------------------------------------------------------------
 */

import type { ComponentType } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';

/** 页面组件收到的属性。 */
export interface ShellPageProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
}

/** 一个模块在外壳中的登记项。 */
export interface ShellModule {
  /** 模块名，与 schema 与目录名一致。 */
  readonly id: string;
  /** 模块的页面。 */
  readonly Page: ComponentType<ShellPageProps>;
}
