/*
 * ------------------------------------------------------------------------
 * 名称：BiosSettings.tsx
 * 说明：“机型与 BIOS”分组的内容：说明文字，加上每个整机机型一行（名称、BIOS 状态、按钮）；没有添加时按钮是“添加 BIOS 文件”，已添加时是“替换”和“移除”，暂未支持的机型整行置灰。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：经典 FC 不需要 BIOS，不在列表里；添加与移除的结果用通知，不在页面里显示。规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

import { Button, Callout, List, Panel, Row, SettingRow, Stack, State } from '@rujianjs/ui-kit';
import type { MachineInfo } from '../../contracts/machines/generated/machines';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { useShell } from '../shell/shell-context';
import { biosStatusText } from './preferences-model';
import { useBios } from './use-bios';

/** 用户可见的文字。 */
const TEXT = {
  intro: '已支持的机型自带默认 BIOS，启动时会自动装好，一般不用添加。想换成别的 BIOS，点“替换”；添加的文件会复制到软件的 BIOS 目录，开机时直接使用，原文件可以删除或移动。',
  loading: '正在读取…',
  add: '添加 BIOS 文件',
  replace: '替换',
  remove: '移除',
  of: (action: string, name: string) => `${action}：${name}`
} as const;

/** 分组内容的属性。 */
export interface BiosSettingsProps {
  readonly bridge: EmberBridge;
}

/** 一个机型的一行。 */
function MachineRow({ machine, onChoose, onRemove }: { readonly machine: MachineInfo; readonly onChoose: (machine: MachineInfo) => void; readonly onRemove: (machine: MachineInfo) => void }) {
  const disabled = !machine.supported;
  return (
    <SettingRow
      label={machine.name}
      description={biosStatusText(machine)}
      control={
        <Row>
          {machine.bios === undefined ? (
            <Button compact disabled={disabled} aria-label={TEXT.of(TEXT.add, machine.name)} onClick={() => onChoose(machine)}>
              {TEXT.add}
            </Button>
          ) : (
            <>
              <Button compact disabled={disabled} aria-label={TEXT.of(TEXT.replace, machine.name)} onClick={() => onChoose(machine)}>
                {TEXT.replace}
              </Button>
              <Button compact disabled={disabled} aria-label={TEXT.of(TEXT.remove, machine.name)} onClick={() => onRemove(machine)}>
                {TEXT.remove}
              </Button>
            </>
          )}
        </Row>
      }
    />
  );
}

/**
 * “机型与 BIOS”分组的内容。
 * @param props 属性。
 */
export function BiosSettings({ bridge }: BiosSettingsProps) {
  const { notify } = useShell();
  const { state, choose, remove } = useBios(bridge, notify);
  const machines = state.kind === 'ready' ? state.machines.filter((machine) => machine.needsBios) : [];

  let status = null;
  if (state.kind === 'failed') status = <State kind="error" text={state.message} />;
  else if (state.kind === 'loading') status = <State kind="busy" text={TEXT.loading} />;

  return (
    <Stack>
      <Callout text={TEXT.intro} />
      {status}
      {machines.length === 0 ? null : (
        <Panel fixed>
          <List variant="divided">
            {machines.map((machine) => (
              <MachineRow key={machine.id} machine={machine} onChoose={(item) => void choose(item)} onRemove={(item) => void remove(item)} />
            ))}
          </List>
        </Panel>
      )}
    </Stack>
  );
}
