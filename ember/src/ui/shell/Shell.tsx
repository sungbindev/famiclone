/*
 * ------------------------------------------------------------------------
 * 名称：Shell.tsx
 * 说明：外壳：显示应用名，并按模块清单的顺序显示各模块登记的页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：本阶段只有系统模块的版本信息与运行页；活动栏、边栏、标签页等外壳布局见 docs/05-界面.md，以后在此基础上扩展。
 * ------------------------------------------------------------------------
 */

import { Title } from '@rujianjs/ui-kit';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellModule } from './shell-registry';
import './shell.css';

/** 应用名。 */
const APP_NAME = 'Ember';

/** 外壳的属性。 */
export interface ShellProps {
  readonly bridge: EmberBridge;
  readonly modules: readonly ShellModule[];
}

/**
 * 外壳。
 * @param props 属性。
 */
export function Shell({ bridge, modules }: ShellProps) {
  return (
    <div className="ember-shell">
      <Title as="h1">{APP_NAME}</Title>
      {modules.map(({ id, Page, fill }) => (
        <div key={id} className={fill === true ? 'ember-shell__page--fill' : undefined}>
          <Page bridge={bridge} />
        </div>
      ))}
    </div>
  );
}
