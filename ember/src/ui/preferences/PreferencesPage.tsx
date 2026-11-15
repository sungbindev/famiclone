/*
 * ------------------------------------------------------------------------
 * 名称：PreferencesPage.tsx
 * 说明：“首选项”对话框的内容：左边是分组导航（只用来切换），右边是该分组的设置项（每项一行），改动立即保存，取值与出厂默认不同时可恢复默认。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：设置项来自核心（preferences.list），由各模块登记，页面不认识具体的设置项；隐藏的设置项（只保存取值）与没有显示项的分组不显示。
 *       页面不显示操作结果：保存失败用组件库的通知提示，控件保持原值。规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

import { List, Panel, ScrollArea, SplitView, Stack, State, Tree, type TreeItem } from '@rujianjs/ui-kit';
import { useState } from 'react';
import type { PreferenceSetting } from '../../contracts/preferences/generated/preferences';
import type { ShellPageProps } from '../shell/shell-registry';
import { useShell } from '../shell/shell-context';
import { BiosSettings } from './BiosSettings';
import { errorText, MACHINES_GROUP_ID, visibleGroups, visibleSettings, withMachinesGroup } from './preferences-model';
import { PreferenceRow } from './PreferenceRow';
import { usePreferences } from './use-preferences';

/** 用户可见的文字。 */
const TEXT = {
  groups: '分组',
  loading: '正在读取…'
} as const;

/** 左栏的可拖动范围与默认宽度（像素）；默认宽度取最小宽度。 */
const SIDE_WIDTH = { initial: 140, min: 140, max: 320 } as const;

/**
 * 首选项页面。
 * @param props 外壳传入的属性。
 */
export function PreferencesPage({ bridge }: ShellPageProps) {
  const { notify } = useShell();
  const { state, set, reset } = usePreferences(bridge);
  const [selected, setSelected] = useState<string | undefined>(undefined);

  /** 保存新值；失败时用通知提示，返回 false 让控件恢复原值。 */
  const change = async (setting: PreferenceSetting, value: unknown): Promise<boolean> => {
    try {
      await set(setting.id, value);
      return true;
    } catch (error) {
      notify(errorText(error), 'error');
      return false;
    }
  };

  const restore = async (setting: PreferenceSetting): Promise<void> => {
    try {
      await reset(setting.id);
    } catch (error) {
      notify(errorText(error), 'error');
    }
  };

  /** 弹出系统的选择文件夹窗口，选定后保存为这个设置项的取值；用户取消时什么都不发生。 */
  const pickDirectory = async (setting: PreferenceSetting): Promise<void> => {
    try {
      const picked = await bridge.main.preferences.pickDirectory({});
      if (picked.canceled || picked.path === undefined) return;
      await change(setting, picked.path);
    } catch (error) {
      notify(errorText(error), 'error');
    }
  };

  const groups = state.kind === 'ready' ? withMachinesGroup(visibleGroups(state.groups, state.settings)) : [];
  const current = groups.find((group) => group.id === selected) ?? groups[0];
  const rows = state.kind === 'ready' && current !== undefined && current.id !== MACHINES_GROUP_ID ? visibleSettings(state.settings).filter((setting) => setting.group === current.id) : [];
  const items: TreeItem[] = groups.map((group) => ({ id: group.id, label: group.label }));

  let status = null;
  if (state.kind === 'failed') status = <State kind="error" text={state.message} />;
  else if (state.kind === 'loading') status = <State kind="busy" text={TEXT.loading} />;

  return (
    <SplitView
      sideLabel={TEXT.groups}
      sideTitle={TEXT.groups}
      defaultSideWidth={SIDE_WIDTH.initial}
      minSideWidth={SIDE_WIDTH.min}
      maxSideWidth={SIDE_WIDTH.max}
      side={<Tree aria-label={TEXT.groups} items={items} selectedId={current?.id ?? null} onSelect={setSelected} />}
    >
      <ScrollArea bleed horizontal="hidden">
        <Stack>
          {status}
          {current?.id === MACHINES_GROUP_ID ? <BiosSettings bridge={bridge} /> : null}
          {rows.length === 0 ? null : (
            <Panel fixed>
              <List variant="divided">
                {rows.map((setting) => (
                  <PreferenceRow key={setting.id} setting={setting} onChange={change} onReset={restore} onPickDirectory={pickDirectory} />
                ))}
              </List>
            </Panel>
          )}
        </Stack>
      </ScrollArea>
    </SplitView>
  );
}
