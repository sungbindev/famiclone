/*
 * ------------------------------------------------------------------------
 * 名称：use-bios.ts
 * 说明：“机型与 BIOS”分组的数据：核心就绪后读取机型清单，订阅 machines.changed 保持最新，核心重启后重新读取；提供 BIOS 的添加或替换（弹出系统文件选择窗口）与移除。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：添加、替换、移除的结果与错误都用通知给出（经调用方传入的 notify），页面里不显示结果；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

import { useCallback, useEffect, useState } from 'react';
import type { MachineInfo } from '../../contracts/machines/generated/machines';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { errorText } from './preferences-model';

/** 机型清单的状态。 */
export type BiosState = { readonly kind: 'loading' } | { readonly kind: 'ready'; readonly machines: readonly MachineInfo[] } | { readonly kind: 'failed'; readonly message: string };

/** “机型与 BIOS”分组的数据与操作。 */
export interface BiosData {
  readonly state: BiosState;
  /** 添加或替换 BIOS：弹出文件选择窗口，用户取消时什么都不发生。 */
  choose(machine: MachineInfo): Promise<void>;
  /** 移除 BIOS，不弹确认框。 */
  remove(machine: MachineInfo): Promise<void>;
}

/** 用户可见的文字。 */
const TEXT = {
  added: (name: string, file: string) => `已添加 ${name} 的 BIOS：${file}`,
  replaced: (name: string, file: string) => `已替换 ${name} 的 BIOS：${file}`,
  removed: (name: string) => `已移除 ${name} 的 BIOS`
} as const;

/**
 * 读取并保持最新的机型清单。
 * @param bridge 预加载脚本暴露的接口。
 * @param notify 通知函数。
 */
export function useBios(bridge: EmberBridge, notify: (text: string, level: 'success' | 'error') => void): BiosData {
  const [state, setState] = useState<BiosState>({ kind: 'loading' });

  useEffect(() => {
    let alive = true;
    const load = (): void => {
      bridge.core.machines.list({}).then(
        (result) => {
          if (alive) setState({ kind: 'ready', machines: result.machines });
        },
        (error: unknown) => {
          if (alive) setState({ kind: 'failed', message: errorText(error) });
        }
      );
    };
    const stopEvents = bridge.onCoreEvent((name) => {
      if (name === 'machines.changed') load();
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

  const choose = useCallback(
    async (machine: MachineInfo): Promise<void> => {
      try {
        const result = await bridge.main.machines.openBios({ machine: machine.id });
        if (result.canceled || result.machine?.bios === undefined) return;
        notify((machine.bios === undefined ? TEXT.added : TEXT.replaced)(machine.name, result.machine.bios.fileName), 'success');
      } catch (error) {
        notify(errorText(error), 'error');
      }
    },
    [bridge, notify]
  );

  const remove = useCallback(
    async (machine: MachineInfo): Promise<void> => {
      try {
        await bridge.core.machines.removeBios({ machine: machine.id });
        notify(TEXT.removed(machine.name), 'success');
      } catch (error) {
        notify(errorText(error), 'error');
      }
    },
    [bridge, notify]
  );

  return { state, choose, remove };
}
