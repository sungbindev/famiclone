// ------------------------------------------------------------------------
// 名称：c-intellisense-commands.test.ts
// 说明：测试 C 智能感知的入口：开启时随工作区自动同步、命令的结果提示、fcdev.json 变化后的同步，
//       以及同步后提示选择 C/C++ 配置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用 vscode 替身与假定时器；同步服务与项目信息提供者用替身。
// ------------------------------------------------------------------------

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { Mock } from 'vitest';
import type { IntelliSenseSyncService, SyncResult } from '../../src/contracts/c-intellisense/intellisense-sync-service';
import type { CppConfigurationInspector } from '../../src/contracts/c-intellisense/cpp-configuration-inspector';
import type { ProjectInfoProvider } from '../../src/contracts/project/project-info';
import { commands, mockState, resetMock, window } from '../support/vscode-mock';

vi.mock('vscode', () => import('../support/vscode-mock'));

const { registerCIntelliSense, SYNC_COMMAND } = await import('../../src/entry/c-intellisense/c-intellisense-commands');

const OK: SyncResult = { root: 'd:\\proj\\game', ok: true, message: '已写入' };
const FAILED: SyncResult = { root: 'd:\\proj\\bad', ok: false, message: '项目里没有安装 fcdev' };

const logger = { info: vi.fn(), error: vi.fn() };

/** 同步服务替身。 */
function fakeSync(results: readonly SyncResult[] = [OK]): IntelliSenseSyncService & {
  syncProject: Mock<IntelliSenseSyncService['syncProject']>;
  syncWorkspace: Mock<IntelliSenseSyncService['syncWorkspace']>;
} {
  return {
    syncProject: vi.fn(async (root: string) => ({ root, ok: true, message: '已写入' })),
    syncWorkspace: vi.fn(async () => results)
  };
}

/** 项目信息提供者替身。 */
function fakeProjects(): ProjectInfoProvider & { invalidate: Mock<ProjectInfoProvider['invalidate']> } {
  return { get: vi.fn(async () => undefined), invalidate: vi.fn<ProjectInfoProvider['invalidate']>() };
}

/** 配置检查器替身：按项目根目录返回是否有其他配置。 */
function fakeInspector(withOthers: readonly string[] = []): CppConfigurationInspector & {
  hasOtherConfigurations: Mock<CppConfigurationInspector['hasOtherConfigurations']>;
} {
  return { hasOtherConfigurations: vi.fn(async (root: string) => withOthers.includes(root)) };
}

/** 注册并返回替身，便于检查。 */
function setup(results?: readonly SyncResult[], withOthers?: readonly string[]) {
  const sync = fakeSync(results);
  const projects = fakeProjects();
  const inspector = fakeInspector(withOthers);
  registerCIntelliSense({ subscriptions: [] } as never, sync, projects, inspector, logger);
  return { sync, projects, inspector };
}

/** 等待已排队的异步任务完成。 */
async function settle(): Promise<void> {
  await vi.advanceTimersByTimeAsync(0);
}

/** 运行“同步”命令。 */
async function runCommand(): Promise<void> {
  await mockState.commands.get(SYNC_COMMAND)?.();
}

beforeEach(() => {
  vi.useFakeTimers();
  resetMock();
  logger.info.mockClear();
  logger.error.mockClear();
});

afterEach(() => {
  vi.useRealTimers();
});

describe('自动同步', () => {
  it('默认开启：激活时对工作区里的项目同步一次，结果只写日志', async () => {
    const { sync } = setup();
    await settle();
    expect(sync.syncWorkspace).toHaveBeenCalledTimes(1);
    expect(logger.info).toHaveBeenCalledWith('已同步（d:\\proj\\game）：已写入');
    expect(window.showInformationMessage).not.toHaveBeenCalled();
    expect(window.showWarningMessage).not.toHaveBeenCalled();
  });

  it('关闭设置后：激活、工作区文件夹变化、fcdev.json 变化都不同步，但仍丢弃项目信息缓存', async () => {
    mockState.settings['fcdev.intelliSense.autoSync'] = false;
    const { sync, projects } = setup();
    mockState.workspaceFolderListeners.forEach((callback) => callback());
    mockState.watchers.get('**/fcdev.json')?.changed.forEach((callback) => callback({ fsPath: 'd:\\proj\\game\\fcdev.json' } as never));
    await vi.advanceTimersByTimeAsync(600);
    expect(sync.syncWorkspace).not.toHaveBeenCalled();
    expect(sync.syncProject).not.toHaveBeenCalled();
    expect(projects.invalidate).toHaveBeenCalledWith('d:\\proj\\game');
  });

  it('工作区文件夹变化时重新同步', async () => {
    const { sync } = setup();
    await settle();
    mockState.workspaceFolderListeners.forEach((callback) => callback());
    await settle();
    expect(sync.syncWorkspace).toHaveBeenCalledTimes(2);
  });

  it('fcdev.json 连续变化合并成一次：先丢弃缓存，再只同步该项目', async () => {
    const { sync, projects } = setup();
    await settle();
    const watcher = mockState.watchers.get('**/fcdev.json');
    const changed = { fsPath: 'd:\\proj\\game\\fcdev.json' } as never;
    watcher?.changed.forEach((callback) => callback(changed));
    await vi.advanceTimersByTimeAsync(300);
    watcher?.created.forEach((callback) => callback(changed));
    await vi.advanceTimersByTimeAsync(300);
    expect(sync.syncProject).not.toHaveBeenCalled();
    await vi.advanceTimersByTimeAsync(300);
    expect(sync.syncProject).toHaveBeenCalledTimes(1);
    expect(sync.syncProject).toHaveBeenCalledWith('d:\\proj\\game');
    expect(projects.invalidate).toHaveBeenCalledTimes(1);
  });

  it('自动同步出错只记日志，不弹提示', async () => {
    const sync = fakeSync();
    sync.syncWorkspace.mockRejectedValueOnce(new Error('坏了'));
    registerCIntelliSense({ subscriptions: [] } as never, sync, fakeProjects(), fakeInspector(), logger);
    await settle();
    expect(logger.error).toHaveBeenCalledTimes(1);
    expect(window.showErrorMessage).not.toHaveBeenCalled();
  });
});

describe('同步命令', () => {
  it('全部成功：提示同步了几个项目', async () => {
    setup([OK, { ...OK, root: 'd:\\proj\\two' }]);
    await runCommand();
    expect(window.showInformationMessage).toHaveBeenCalledWith('已同步 2 个 fcdev 项目的 C/C++ 配置');
    expect(window.withProgress).toHaveBeenCalled();
  });

  it('有项目失败：用警告列出项目目录名与原因', async () => {
    setup([OK, FAILED]);
    await runCommand();
    expect(window.showWarningMessage).toHaveBeenCalledWith('bad：项目里没有安装 fcdev');
  });

  it('工作区里没有 fcdev 项目：提示找不到 fcdev.json', async () => {
    setup([]);
    await runCommand();
    expect(window.showInformationMessage).toHaveBeenCalledWith('工作区里没有 fcdev 项目（找不到 fcdev.json）');
  });

  it('同步出错：记日志并提示去输出面板看详情', async () => {
    const sync = fakeSync();
    registerCIntelliSense({ subscriptions: [] } as never, sync, fakeProjects(), fakeInspector(), logger);
    await settle();
    sync.syncWorkspace.mockRejectedValueOnce(new Error('坏了'));
    await runCommand();
    expect(logger.error).toHaveBeenCalled();
    expect(window.showErrorMessage).toHaveBeenCalledWith('同步 C/C++ 配置时出错，详情见“输出”面板里的 fcdev');
  });
});

describe('提示选择 C/C++ 配置', () => {
  const SELECT_COMMAND = 'C_Cpp.ConfigurationSelect';
  const OTHER_ROOT = OK.root;

  beforeEach(() => {
    mockState.installedExtensions.add('ms-vscode.cpptools');
  });

  it('自动同步后有其他配置：弹出提示，点“选择配置”打开 C/C++ 扩展的选择列表', async () => {
    window.showInformationMessage.mockResolvedValueOnce('选择配置');
    setup([OK], [OTHER_ROOT]);
    await settle();
    expect(window.showInformationMessage).toHaveBeenCalledWith(expect.stringContaining('game 的 C/C++ 配置里除 fcdev 外还有其他配置'), '选择配置');
    expect(commands.executeCommand).toHaveBeenCalledWith(SELECT_COMMAND);
  });

  it('用户没点按钮就关掉提示：不执行选择命令', async () => {
    setup([OK], [OTHER_ROOT]);
    await settle();
    expect(window.showInformationMessage).toHaveBeenCalledTimes(1);
    expect(commands.executeCommand).not.toHaveBeenCalled();
  });

  it('每个项目每次会话只提示一次', async () => {
    setup([OK], [OTHER_ROOT]);
    await settle();
    mockState.workspaceFolderListeners.forEach((callback) => callback());
    await settle();
    await runCommand();
    await settle();
    expect(window.showInformationMessage).toHaveBeenCalledTimes(2);
    const prompts = window.showInformationMessage.mock.calls.filter(([message]) => message.includes('除 fcdev 外'));
    expect(prompts).toHaveLength(1);
  });

  it('没有其他配置时不提示', async () => {
    setup([OK], []);
    await settle();
    expect(window.showInformationMessage).not.toHaveBeenCalled();
  });

  it('没安装 C/C++ 扩展时不提示，也不读配置文件', async () => {
    mockState.installedExtensions.clear();
    const { inspector } = setup([OK], [OTHER_ROOT]);
    await settle();
    expect(window.showInformationMessage).not.toHaveBeenCalled();
    expect(inspector.hasOtherConfigurations).not.toHaveBeenCalled();
  });

  it('关闭设置后不提示', async () => {
    mockState.settings['fcdev.intelliSense.promptConfigurationSelection'] = false;
    setup([OK], [OTHER_ROOT]);
    await settle();
    expect(window.showInformationMessage).not.toHaveBeenCalled();
  });

  it('同步失败的项目不检查、不提示', async () => {
    const { inspector } = setup([FAILED], [FAILED.root]);
    await settle();
    expect(inspector.hasOtherConfigurations).not.toHaveBeenCalled();
    expect(window.showInformationMessage).not.toHaveBeenCalled();
  });

  it('多个项目合并成一条提示，项目名用顿号隔开', async () => {
    const two = { ...OK, root: 'd:\\proj\\two' };
    setup([OK, two], [OK.root, two.root]);
    await settle();
    expect(window.showInformationMessage).toHaveBeenCalledTimes(1);
    expect(window.showInformationMessage).toHaveBeenCalledWith(expect.stringContaining('game、two 的 C/C++ 配置'), '选择配置');
  });

  it('手动运行同步命令后也提示', async () => {
    const sync = fakeSync([OK]);
    registerCIntelliSense({ subscriptions: [] } as never, sync, fakeProjects(), fakeInspector([OTHER_ROOT]), logger);
    mockState.settings['fcdev.intelliSense.autoSync'] = false;
    await runCommand();
    await settle();
    expect(window.showInformationMessage).toHaveBeenCalledWith(expect.stringContaining('除 fcdev 外还有其他配置'), '选择配置');
  });

  it('检查配置文件出错只记日志，不影响同步结果', async () => {
    const inspector = fakeInspector();
    inspector.hasOtherConfigurations.mockRejectedValueOnce(new Error('没有权限'));
    registerCIntelliSense({ subscriptions: [] } as never, fakeSync([OK]), fakeProjects(), inspector, logger);
    await settle();
    expect(logger.error).toHaveBeenCalledWith('提示选择 C/C++ 配置出错', expect.any(Error));
    expect(logger.info).toHaveBeenCalledWith('已同步（d:\\proj\\game）：已写入');
  });
});
