/*
 * ------------------------------------------------------------------------
 * 名称：use-preferences.ts
 * 说明：首选项数据：核心就绪后读取全部分组与设置项，订阅 preferences.changed 保持取值最新（在别处修改也会反映），核心重启后重新读取；提供修改与恢复默认。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：修改与恢复默认不在本地先改取值：请求成功后以核心给出的取值为准，失败时取值保持原样，由调用方提示用户。
 * ------------------------------------------------------------------------
 */

import { useCallback, useEffect, useState } from 'react';
import type { PreferenceGroup, PreferenceSetting } from '../../contracts/preferences/generated/preferences';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { errorText, withValue } from './preferences-model';

/** 数据的状态。 */
export type PreferencesState =
  | { readonly kind: 'loading' }
  | { readonly kind: 'ready'; readonly groups: readonly PreferenceGroup[]; readonly settings: readonly PreferenceSetting[] }
  | { readonly kind: 'failed'; readonly message: string };

/** 首选项数据与操作。 */
export interface PreferencesData {
  readonly state: PreferencesState;
  /** 修改一个设置项；失败时抛出。 */
  set(id: string, value: unknown): Promise<void>;
  /** 恢复默认值；失败时抛出。 */
  reset(id: string): Promise<void>;
}

/** 核心事件 preferences.changed 的数据。 */
function changeOf(name: string, data: unknown): { id: string; value: unknown } | undefined {
  if (name !== 'preferences.changed' || typeof data !== 'object' || data === null) return undefined;
  const { id, value } = data as { id?: unknown; value?: unknown };
  return typeof id === 'string' ? { id, value } : undefined;
}

/**
 * 读取并保持最新的首选项数据。
 * @param bridge 预加载脚本暴露的接口。
 */
export function usePreferences(bridge: EmberBridge): PreferencesData {
  const [state, setState] = useState<PreferencesState>({ kind: 'loading' });

  useEffect(() => {
    let alive = true;
    const load = (): void => {
      bridge.core.preferences.list({}).then(
        (result) => {
          if (alive) setState({ kind: 'ready', groups: result.groups, settings: result.settings });
        },
        (error: unknown) => {
          if (alive) setState({ kind: 'failed', message: errorText(error) });
        }
      );
    };
    const stopEvents = bridge.onCoreEvent((name, data) => {
      const change = changeOf(name, data);
      if (change === undefined) return;
      setState((current) => (current.kind === 'ready' ? { ...current, settings: withValue(current.settings, change.id, change.value) } : current));
    });
    const stopStatus = bridge.onCoreStatusChange((status) => {
      if (status.state === 'ready') load();
      else if (alive) setState({ kind: 'loading' });
    });
    load();
    return () => {
      alive = false;
      stopEvents();
      stopStatus();
    };
  }, [bridge]);

  const set = useCallback(
    async (id: string, value: unknown): Promise<void> => {
      const result = await bridge.core.preferences.set({ id, value });
      setState((current) => (current.kind === 'ready' ? { ...current, settings: withValue(current.settings, result.id, result.value) } : current));
    },
    [bridge]
  );

  const reset = useCallback(
    async (id: string): Promise<void> => {
      const result = await bridge.core.preferences.reset({ id });
      setState((current) => (current.kind === 'ready' ? { ...current, settings: withValue(current.settings, result.id, result.value) } : current));
    },
    [bridge]
  );

  return { state, set, reset };
}
