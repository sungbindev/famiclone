/*
 * ------------------------------------------------------------------------
 * 名称：use-input-bindings.ts
 * 说明：手柄绑定数据：核心就绪后读取全部玩家槽位，订阅 input.bindingsChanged 保持最新，核心重启后重新读取；提供保存与恢复默认。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：保存与恢复默认不在本地先改：请求成功后以核心给出的结果为准，失败时保持原样，由调用方提示用户。
 * ------------------------------------------------------------------------
 */

import { useCallback, useEffect, useState } from 'react';
import type { PadSlot } from '../../contracts/input/generated/input';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { errorText } from './input-model';

/** 数据的状态。 */
export type InputBindingsState = { readonly kind: 'loading' } | { readonly kind: 'ready'; readonly slots: readonly PadSlot[] } | { readonly kind: 'failed'; readonly message: string };

/** 手柄绑定的数据与操作。 */
export interface InputBindingsData {
  readonly state: InputBindingsState;
  /** 保存全部玩家槽位；失败时抛出。 */
  save(slots: readonly PadSlot[]): Promise<void>;
  /** 恢复默认绑定；失败时抛出。 */
  reset(): Promise<void>;
}

/** 核心事件 input.bindingsChanged 带来的槽位。 */
function slotsOf(name: string, data: unknown): PadSlot[] | undefined {
  if (name !== 'input.bindingsChanged' || typeof data !== 'object' || data === null) return undefined;
  const { slots } = data as { slots?: unknown };
  return Array.isArray(slots) ? (slots as PadSlot[]) : undefined;
}

/**
 * 读取并保持最新的手柄绑定。
 * @param bridge 预加载脚本暴露的接口。
 */
export function useInputBindings(bridge: EmberBridge): InputBindingsData {
  const [state, setState] = useState<InputBindingsState>({ kind: 'loading' });

  useEffect(() => {
    let alive = true;
    const load = (): void => {
      bridge.core.input.getBindings({}).then(
        (result) => {
          if (alive) setState({ kind: 'ready', slots: result.slots });
        },
        (error: unknown) => {
          if (alive) setState({ kind: 'failed', message: errorText(error) });
        }
      );
    };
    const stopEvents = bridge.onCoreEvent((name, data) => {
      const slots = slotsOf(name, data);
      if (slots !== undefined) setState({ kind: 'ready', slots });
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

  const save = useCallback(
    async (slots: readonly PadSlot[]): Promise<void> => {
      const result = await bridge.core.input.setBindings({ slots: [...slots] });
      setState({ kind: 'ready', slots: result.slots });
    },
    [bridge]
  );

  const reset = useCallback(async (): Promise<void> => {
    const result = await bridge.core.input.resetBindings({});
    setState({ kind: 'ready', slots: result.slots });
  }, [bridge]);

  return { state, save, reset };
}
