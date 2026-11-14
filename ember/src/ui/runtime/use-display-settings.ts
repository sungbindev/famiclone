/*
 * ------------------------------------------------------------------------
 * 名称：use-display-settings.ts
 * 说明：运行页用的显示设置：读取首选项里的画面比例、缩放方式与“显示全部 240 行”，订阅 preferences.changed 保持最新，核心重启后重新读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：设置项的标识与取值由核心的首选项模块登记（见 docs/05-界面设计.md 4.5）；还没读到或读到的取值不认识时用画面的默认值。
 *       界面各模块之间不互相引用，所以这里不复用首选项页面的数据钩子。
 * ------------------------------------------------------------------------
 */

import { useEffect, useState } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { DEFAULT_SCREEN_ASPECT, DEFAULT_SCREEN_SCALING, type ScreenAspect, type ScreenScaling } from './screen-layout';

/** 运行页用的显示设置。 */
export interface DisplaySettings {
  readonly aspect: ScreenAspect;
  readonly scaling: ScreenScaling;
  readonly showAllLines: boolean;
}

/** 还没读到设置时用的默认值。 */
export const DEFAULT_DISPLAY_SETTINGS: DisplaySettings = { aspect: DEFAULT_SCREEN_ASPECT, scaling: DEFAULT_SCREEN_SCALING, showAllLines: true };

/**
 * 把一个设置项的取值合并进显示设置；不是显示设置项或取值不认识时原样返回。
 * @param current 当前的显示设置。
 * @param id 设置项标识。
 * @param value 取值。
 */
export function applyDisplaySetting(current: DisplaySettings, id: string, value: unknown): DisplaySettings {
  if (id === 'display.aspect' && (value === '4:3' || value === '8:7')) return { ...current, aspect: value };
  if (id === 'display.scaling' && (value === 'fit' || value === 'integer')) return { ...current, scaling: value };
  if (id === 'display.showAllLines' && typeof value === 'boolean') return { ...current, showAllLines: value };
  return current;
}

/**
 * 读取并保持最新的显示设置。
 * @param bridge 预加载脚本暴露的接口。
 */
export function useDisplaySettings(bridge: EmberBridge): DisplaySettings {
  const [settings, setSettings] = useState<DisplaySettings>(DEFAULT_DISPLAY_SETTINGS);

  useEffect(() => {
    let alive = true;
    const load = (): void => {
      bridge.core.preferences.list({}).then(
        (result) => {
          if (alive) setSettings(result.settings.reduce((current, item) => applyDisplaySetting(current, item.id, item.value), DEFAULT_DISPLAY_SETTINGS));
        },
        () => {
          // 读不到时继续用当前值；核心就绪后会重新读取。
        }
      );
    };
    const stopEvents = bridge.onCoreEvent((name, data) => {
      if (name !== 'preferences.changed' || typeof data !== 'object' || data === null) return;
      const { id, value } = data as { id?: unknown; value?: unknown };
      if (typeof id === 'string') setSettings((current) => applyDisplaySetting(current, id, value));
    });
    const stopStatus = bridge.onCoreStatusChange((status) => {
      if (status.state === 'ready') load();
    });
    load();
    return () => {
      alive = false;
      stopEvents();
      stopStatus();
    };
  }, [bridge]);

  return settings;
}
