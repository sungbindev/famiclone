/*
 * ------------------------------------------------------------------------
 * 名称：SystemVersionPage.test.tsx
 * 说明：系统模块页面的测试：读取版本期间显示读取中，成功时显示应用版本与核心版本，失败时经通知给出说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { render, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { SystemVersionPage } from './SystemVersionPage';

/** 只实现页面用到的请求的接口。 */
function bridgeWith(version: () => Promise<{ appVersion: string; coreVersion: string }>): EmberBridge {
  return {
    core: { system: { version } } as unknown as EmberBridge['core'],
    main: {} as unknown as EmberBridge['main'],
    streams: { requestPorts: () => undefined },
    onCoreEvent: () => () => undefined,
    getCoreStatus: () => Promise.reject(new Error('未使用'))
  };
}

function renderPage(bridge: EmberBridge) {
  return render(
    <UiProvider>
      <SystemVersionPage bridge={bridge} />
    </UiProvider>
  );
}

describe('SystemVersionPage', () => {
  it('显示 system.version 返回的应用版本与核心版本', async () => {
    const version = vi.fn().mockResolvedValue({ appVersion: '0.1.0', coreVersion: '0.1.1' });
    renderPage(bridgeWith(version));
    expect(screen.getAllByText('读取中…')).toHaveLength(2);
    expect(await screen.findByText('0.1.0')).toBeTruthy();
    expect(screen.getByText('0.1.1')).toBeTruthy();
    expect(screen.getByText('应用版本')).toBeTruthy();
    expect(screen.getByText('核心版本')).toBeTruthy();
    expect(version).toHaveBeenCalledWith({});
  });

  it('读取失败时版本显示为破折号，并经通知给出说明', async () => {
    renderPage(bridgeWith(() => Promise.reject({ code: 'core.unavailable', message: '核心不可用' })));
    expect(await screen.findByText('无法读取版本：核心不可用')).toBeTruthy();
    expect(screen.getAllByText('—')).toHaveLength(2);
  });
});
