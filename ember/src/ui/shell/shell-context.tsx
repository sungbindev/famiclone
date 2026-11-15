/*
 * ------------------------------------------------------------------------
 * 名称：shell-context.tsx
 * 说明：外壳提供给显示区、面板与状态项的上下文：外壳接口（useShell）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：模块只经这里的 Hook 与外壳协作，不引用外壳的界面代码；外壳接口在外壳的生命周期内保持不变，用作 Effect 的依赖是安全的。
 * ------------------------------------------------------------------------
 */

import { createContext, useContext } from 'react';
import type { ShellApi } from './shell-registry';

/** 外壳接口的上下文。 */
export const ShellApiContext = createContext<ShellApi | null>(null);

/** 取外壳接口：打开对话框、展开面板、执行命令。 */
export function useShell(): ShellApi {
  const api = useContext(ShellApiContext);
  if (api === null) throw new Error('useShell 必须在外壳里使用');
  return api;
}
