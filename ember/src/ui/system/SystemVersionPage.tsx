/*
 * ------------------------------------------------------------------------
 * 名称：SystemVersionPage.tsx
 * 说明：系统模块的页面：经 system.version 取得应用版本与核心版本并显示；取不到时经通知给出错误说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：必须放在组件库根组件 UiProvider 之内（通知需要它）。
 * ------------------------------------------------------------------------
 */

import { Facts, Panel, useNotify } from '@rujianjs/ui-kit';
import { useEffect, useState } from 'react';
import type { CoreErrorInfo } from '../../contracts/core-gateway/core-messages';
import type { SystemVersionResult } from '../../contracts/system/generated/system';
import type { ShellPageProps } from '../shell/shell-registry';

/** 用户可见的文字。 */
const TEXT = {
  title: '版本',
  appVersion: '应用版本',
  coreVersion: '核心版本',
  loading: '读取中…',
  unknown: '—',
  failed: '无法读取版本'
} as const;

/** 读取的状态。 */
type VersionState = { readonly state: 'loading' } | { readonly state: 'ready'; readonly result: SystemVersionResult } | { readonly state: 'failed' };

/** 不同状态下两项的显示文字。 */
function versionTexts(version: VersionState): { app: string; core: string } {
  if (version.state === 'ready') return { app: version.result.appVersion, core: version.result.coreVersion };
  const text = version.state === 'loading' ? TEXT.loading : TEXT.unknown;
  return { app: text, core: text };
}

/**
 * 系统模块的页面。
 * @param props 属性。
 */
export function SystemVersionPage({ bridge }: ShellPageProps) {
  const notify = useNotify();
  const [version, setVersion] = useState<VersionState>({ state: 'loading' });

  useEffect(() => {
    let active = true;
    bridge.core.system.version({}).then(
      (result) => {
        if (active) setVersion({ state: 'ready', result });
      },
      (error: CoreErrorInfo) => {
        if (!active) return;
        setVersion({ state: 'failed' });
        notify(`${TEXT.failed}：${error.message}`, 'error');
      }
    );
    return () => {
      active = false;
    };
  }, [bridge, notify]);

  const texts = versionTexts(version);
  return (
    <Panel title={TEXT.title} fixed>
      <Facts
        items={[
          { label: TEXT.appVersion, value: texts.app },
          { label: TEXT.coreVersion, value: texts.core }
        ]}
        aria-label={TEXT.title}
      />
    </Panel>
  );
}
