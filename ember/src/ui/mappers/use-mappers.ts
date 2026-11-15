/*
 * ------------------------------------------------------------------------
 * 名称：use-mappers.ts
 * 说明：mapper 插件数据：核心就绪后读取全部插件，订阅 mappers.changed 保持列表最新，核心重启后重新读取；选中一个插件时读取它的清单详情；提供安装、卸载、启用与停用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：启用与停用以核心返回的摘要为准，失败时列表保持原样，由调用方提示用户；插件被卸载后选中项自动清除。
 * ------------------------------------------------------------------------
 */

import { useCallback, useEffect, useState } from 'react';
import type { MapperDetail, MapperSummary } from '../../contracts/mappers/generated/mappers';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { errorText } from './mappers-model';

/** 用户可见的文字。 */
const TEXT = { listFailed: '读取插件列表失败', detailFailed: '读取插件详情失败' } as const;

/** 列表的状态。 */
export type MappersState =
  | { readonly kind: 'loading' }
  | { readonly kind: 'ready'; readonly plugins: readonly MapperSummary[] }
  | { readonly kind: 'failed'; readonly message: string };

/** 选中插件的详情状态：没有选中、读取中、已读到、读取失败。 */
export type DetailState =
  | { readonly kind: 'none' }
  | { readonly kind: 'loading' }
  | { readonly kind: 'ready'; readonly detail: MapperDetail }
  | { readonly kind: 'failed'; readonly message: string };

/** 一次详情读取的结果，带上读取的是哪个插件、读取时插件变化的次数。 */
type DetailResult = { readonly id: string; readonly revision: number; readonly detail: MapperDetail } | { readonly id: string; readonly revision: number; readonly message: string };

/** mapper 插件的数据与操作。 */
export interface MappersData {
  readonly state: MappersState;
  /** 选中的插件标识；插件不在列表里时为 undefined。 */
  readonly selectedId: string | undefined;
  readonly detail: DetailState;
  select(id: string): void;
  /** 弹出文件选择窗口安装插件包；返回安装后的摘要，用户取消时返回 undefined；失败时抛出。 */
  install(): Promise<MapperSummary | undefined>;
  /** 卸载用户安装的插件；失败时抛出。 */
  uninstall(id: string): Promise<void>;
  /** 启用或停用；失败时抛出。 */
  setEnabled(id: string, enabled: boolean): Promise<void>;
}

/** 用新的摘要替换列表里同标识的一项。 */
function replaced(plugins: readonly MapperSummary[], plugin: MapperSummary): readonly MapperSummary[] {
  return plugins.map((item) => (item.id === plugin.id ? plugin : item));
}

/**
 * 读取并保持最新的 mapper 插件数据。
 * @param bridge 预加载脚本暴露的接口。
 */
export function useMappers(bridge: EmberBridge): MappersData {
  const [state, setState] = useState<MappersState>({ kind: 'loading' });
  const [chosenId, setChosenId] = useState<string | undefined>(undefined);
  const [detailResult, setDetailResult] = useState<DetailResult | undefined>(undefined);
  // 插件有变化的次数：变化后重新读取选中插件的详情。
  const [revision, setRevision] = useState(0);

  useEffect(() => {
    let alive = true;
    const load = (): void => {
      bridge.core.mappers.list({}).then(
        (result) => {
          if (alive) setState({ kind: 'ready', plugins: result.plugins });
        },
        (error: unknown) => {
          if (alive) setState({ kind: 'failed', message: errorText(error, TEXT.listFailed) });
        }
      );
    };
    const stopEvents = bridge.onCoreEvent((name) => {
      if (name !== 'mappers.changed') return;
      load();
      setRevision((value) => value + 1);
    });
    const stopStatus = bridge.onCoreStatusChange((status) => {
      if (status.state === 'ready') {
        load();
        setRevision((value) => value + 1);
      } else if (alive) {
        setState({ kind: 'loading' });
      }
    });
    load();
    return () => {
      alive = false;
      stopEvents();
      stopStatus();
    };
  }, [bridge]);

  const selectedId = state.kind === 'ready' && state.plugins.some((plugin) => plugin.id === chosenId) ? chosenId : undefined;

  useEffect(() => {
    if (selectedId === undefined) return undefined;
    let alive = true;
    bridge.core.mappers.info({ id: selectedId }).then(
      (result) => {
        if (alive) setDetailResult({ id: selectedId, revision, detail: result.plugin });
      },
      (error: unknown) => {
        if (alive) setDetailResult({ id: selectedId, revision, message: errorText(error, TEXT.detailFailed) });
      }
    );
    return () => {
      alive = false;
    };
  }, [bridge, selectedId, revision]);

  let detail: DetailState = { kind: 'none' };
  if (selectedId !== undefined) {
    if (detailResult?.id !== selectedId) detail = { kind: 'loading' };
    else detail = 'detail' in detailResult ? { kind: 'ready', detail: detailResult.detail } : { kind: 'failed', message: detailResult.message };
  }

  const install = useCallback(async (): Promise<MapperSummary | undefined> => {
    const result = await bridge.main.mappers.openPackage({});
    if (result.canceled || result.plugin === undefined) return undefined;
    const { plugin } = result;
    setState((current) => (current.kind === 'ready' ? { kind: 'ready', plugins: current.plugins.some((item) => item.id === plugin.id) ? replaced(current.plugins, plugin) : [...current.plugins, plugin].toSorted((a, b) => a.id.localeCompare(b.id)) } : current));
    setChosenId(plugin.id);
    return plugin;
  }, [bridge]);

  const uninstall = useCallback(
    async (id: string): Promise<void> => {
      await bridge.core.mappers.uninstall({ id });
      setState((current) => (current.kind === 'ready' ? { kind: 'ready', plugins: current.plugins.filter((item) => item.id !== id) } : current));
    },
    [bridge]
  );

  const setEnabled = useCallback(
    async (id: string, enabled: boolean): Promise<void> => {
      const result = await bridge.core.mappers.setEnabled({ id, enabled });
      setState((current) => (current.kind === 'ready' ? { kind: 'ready', plugins: replaced(current.plugins, result.plugin) } : current));
    },
    [bridge]
  );

  return { state, selectedId, detail, select: setChosenId, install, uninstall, setEnabled };
}
