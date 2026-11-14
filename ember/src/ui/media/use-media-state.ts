/*
 * ------------------------------------------------------------------------
 * 名称：use-media-state.ts
 * 说明：读取设备边栏控制器的状态：状态变化时重新渲染。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { useSyncExternalStore } from 'react';
import type { MediaController, MediaViewState } from './media-controller';

/**
 * 读取控制器的当前状态。
 * @param controller 设备边栏的控制器。
 */
export function useMediaState(controller: MediaController): MediaViewState {
  return useSyncExternalStore(controller.subscribe, controller.snapshot);
}
