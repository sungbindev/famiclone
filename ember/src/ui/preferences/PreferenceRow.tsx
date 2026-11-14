/*
 * ------------------------------------------------------------------------
 * 名称：PreferenceRow.tsx
 * 说明：首选项页面里的一行设置项：名称与说明在左，控件在右（选择用下拉，开关用复选框，整数用输入框），取值与出厂默认不同时显示“恢复默认”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：控件的值始终取自设置项的当前值，保存成功后由数据刷新；保存失败时值保持原样。整数在失焦或按回车时才提交，超出范围时在控件旁提示且不提交；
 *       输入中的内容记下它基于哪个当前值，当前值变了（保存成功、恢复默认或在别处修改）后这份输入自动作废。
 * ------------------------------------------------------------------------
 */

import { Button, Checkbox, Row, Select, SettingRow, TextInput } from '@rujianjs/ui-kit';
import { useRef, useState } from 'react';
import type { PreferenceSetting } from '../../contracts/preferences/generated/preferences';
import { isChanged, parseInteger } from './preferences-model';

/** 用户可见的文字。 */
const TEXT = {
  switchOn: '开启',
  reset: '恢复默认',
  resetOf: (label: string) => `恢复默认：${label}`
} as const;

/** 设置项一行的属性。 */
export interface PreferenceRowProps {
  readonly setting: PreferenceSetting;
  /** 保存新值；成功返回 true，失败时页面已提示用户并返回 false。 */
  onChange(setting: PreferenceSetting, value: unknown): Promise<boolean>;
  /** 恢复默认值。 */
  onReset(setting: PreferenceSetting): Promise<void>;
}

/** 整数输入中的内容：输入的文字、校验提示，以及它基于的当前值。 */
interface IntegerEdit {
  readonly basedOn: unknown;
  readonly text: string;
  readonly error: string;
}

/**
 * 一行设置项。
 * @param props 属性。
 */
export function PreferenceRow({ setting, onChange, onReset }: PreferenceRowProps) {
  const { value } = setting;
  const [edit, setEdit] = useState<IntegerEdit | null>(null);
  const saving = useRef(false);

  // 输入中的内容只在当前值没变时有效。
  const active = edit !== null && Object.is(edit.basedOn, value) ? edit : null;
  const text = active?.text ?? String(value);
  const error = active?.error ?? '';

  /** 保存；上一次保存还没结束时忽略。 */
  const save = async (next: unknown): Promise<boolean> => {
    if (saving.current) return false;
    saving.current = true;
    try {
      return await onChange(setting, next);
    } finally {
      saving.current = false;
    }
  };

  const submitInput = async (): Promise<void> => {
    if (active === null) return;
    // 整数先解析与检查范围，文字原样提交（长度等限制由核心校验）。
    let next: unknown = active.text;
    if (setting.kind === 'integer') {
      const parsed = parseInteger(active.text, setting);
      if (!parsed.ok) {
        setEdit({ ...active, error: parsed.message });
        return;
      }
      next = parsed.value;
    }
    if (next === value) {
      setEdit(null);
      return;
    }
    setEdit({ ...active, error: '' });
    // 成功后输入作废，显示改由刷新后的当前值给出；失败时同样回到原值。
    await save(next);
    setEdit(null);
  };

  let control;
  if (setting.kind === 'switch') {
    control = <Checkbox label={TEXT.switchOn} aria-label={setting.label} checked={value === true} onChange={(checked) => void save(checked)} />;
  } else if (setting.kind === 'integer' || setting.kind === 'text') {
    control = (
      <TextInput
        inputMode={setting.kind === 'integer' ? 'numeric' : 'text'}
        aria-label={setting.label}
        value={text}
        invalid={error !== ''}
        onChange={(next) => setEdit({ basedOn: value, text: next, error: '' })}
        onBlur={() => void submitInput()}
        onEnter={() => void submitInput()}
      />
    );
  } else {
    control = <Select aria-label={setting.label} options={setting.options ?? []} value={String(value)} allowEmpty={false} onChange={(next) => void save(next)} />;
  }

  return (
    <SettingRow
      label={setting.label}
      description={setting.description}
      error={error}
      control={
        <Row>
          {control}
          {isChanged(setting) ? (
            <Button compact aria-label={TEXT.resetOf(setting.label)} onClick={() => void onReset(setting)}>
              {TEXT.reset}
            </Button>
          ) : null}
        </Row>
      }
    />
  );
}
