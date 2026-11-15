/*
 * ------------------------------------------------------------------------
 * 名称：AboutDialog.test.tsx
 * 说明：“关于 Ember”对话框的测试：显示名称、应用版本、核心版本、编译时间、作者、邮箱、QQ 号与许可证，第三方依赖表（名称与版本、作者、许可）来自传入的清单，读取版本失败时显示“读取失败”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的预加载接口，第三方依赖清单由测试给出。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { render, screen, within } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { AboutDialog, type ThirdPartyNotice } from './AboutDialog';

const NOTICES: readonly ThirdPartyNotice[] = [
  { name: 'react', version: '19.0.0', authors: ['Meta'], license: 'MIT', homepage: 'https://react.dev', usage: '界面框架' },
  { name: 'quickjs-ng', version: '0.9.0', authors: ['Ben Noordhuis', 'Fabrice Bellard'], license: 'MIT', homepage: 'https://example.com', usage: 'mapper 脚本沙箱' }
];

function setup(version: () => Promise<{ appVersion: string; coreVersion: string }>) {
  const bridge = { core: { system: { version: vi.fn(version) } } } as unknown as EmberBridge;
  render(
    <UiProvider>
      <AboutDialog bridge={bridge} notices={NOTICES} buildTime="2026-11-16 12:30:45" />
    </UiProvider>
  );
}

describe('AboutDialog', () => {
  it('显示名称、应用版本、核心版本与许可证', async () => {
    setup(() => Promise.resolve({ appVersion: '0.1.0', coreVersion: '0.2.0' }));
    expect(screen.getByRole('heading', { name: 'Ember' })).toBeTruthy();
    expect(await screen.findByText('版本 0.1.0')).toBeTruthy();
    expect(screen.getByText('核心版本 0.2.0')).toBeTruthy();
    expect(screen.getByText('GNU 通用公共许可证第 3 版（GPL-3.0）')).toBeTruthy();
  });

  it('显示编译时间、作者、邮箱与 QQ 号', () => {
    setup(() => Promise.resolve({ appVersion: '0.1.0', coreVersion: '0.2.0' }));
    expect(screen.getByText('2026-11-16 12:30:45')).toBeTruthy();
    expect(screen.getByText('sungbin')).toBeTruthy();
    expect(screen.getByText('sungbin@3578.cn')).toBeTruthy();
    expect(screen.getByText('46427991')).toBeTruthy();
  });

  it('第三方依赖表列出清单里的每一项：名称与版本、作者（多个用顿号）、许可', async () => {
    setup(() => Promise.resolve({ appVersion: '0.1.0', coreVersion: '0.2.0' }));
    const table = await screen.findByRole('table', { name: '第三方依赖' });
    expect(within(table).getAllByRole('row')).toHaveLength(3);
    const quickjs = within(table).getByText('quickjs-ng 0.9.0').closest('tr') as HTMLElement;
    expect(within(quickjs).getByText('Ben Noordhuis、Fabrice Bellard')).toBeTruthy();
    expect(within(quickjs).getByText('MIT')).toBeTruthy();
    expect(screen.getByText('Ember 使用了下列开源项目，各项目的许可全文随安装包分发。')).toBeTruthy();
  });

  it('读取版本时先显示“正在读取…”，失败后显示“读取失败”，第三方依赖仍然显示', async () => {
    setup(() => Promise.reject(new Error('核心未启动')));
    expect(screen.getAllByText(/正在读取…/).length).toBe(2);
    expect(await screen.findByText('版本 读取失败')).toBeTruthy();
    expect(screen.getByText('核心版本 读取失败')).toBeTruthy();
    expect(screen.getByText('react 19.0.0')).toBeTruthy();
  });
});
