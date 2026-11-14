/*
 * ------------------------------------------------------------------------
 * 名称：use-runtime-state.ts
 * 说明：读取运行页控制器的状态：状态变化时重新渲染。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { useSyncExternalStore } from 'react';
import type { RuntimeController, RuntimeViewState } from './runtime-controller';

/**
 * 读取控制器的当前状态。
 * @param controller 运行页的控制器。
 */
export function useRuntimeState(controller: RuntimeController): RuntimeViewState {
  return useSyncExternalStore(controller.subscribe, controller.snapshot);
}
