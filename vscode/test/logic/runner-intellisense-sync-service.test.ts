// ------------------------------------------------------------------------
// 名称：runner-intellisense-sync-service.test.ts
// 说明：测试 C 智能感知同步服务：成功与失败的整理、同一项目按顺序运行、工作区同步。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用替身代替命令行与项目查找；可预期的错误转成失败结果，其他异常原样抛出。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import type { FcdevCommandResult, FcdevCommandRunner, FcdevProjectLocator } from '../../src/contracts/project/fcdev-command-runner';
import { RunnerIntelliSenseSyncService } from '../../src/logic/c-intellisense/runner-intellisense-sync-service';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';

/** 记录调用并按需返回结果的命令行替身。 */
class FakeRunner implements FcdevCommandRunner {
  readonly calls: Array<{ root: string; args: readonly string[] }> = [];
  readonly log: string[] = [];

  /** @param handler 按项目根目录决定结果或抛错。 */
  constructor(private readonly handler: (root: string) => Promise<FcdevCommandResult>) {}

  /** @inheritdoc */
  async run(root: string, args: readonly string[]): Promise<FcdevCommandResult> {
    this.calls.push({ root, args });
    this.log.push(`开始 ${root}`);
    try {
      return await this.handler(root);
    } finally {
      this.log.push(`结束 ${root}`);
    }
  }
}

const OK: FcdevCommandResult = { exitCode: 0, stdout: '已写入 .vscode/c_cpp_properties.json\n', stderr: '' };

/** 固定返回项目列表的替身。 */
function locator(...roots: string[]): FcdevProjectLocator {
  return { findProjectRoots: async () => roots };
}

describe('同步一个项目', () => {
  it('成功时返回 fcdev 的输出，并运行 sync 命令', async () => {
    const runner = new FakeRunner(async () => OK);
    const service = new RunnerIntelliSenseSyncService(runner, locator());
    expect(await service.syncProject('d:\\a')).toEqual({ root: 'd:\\a', ok: true, message: '已写入 .vscode/c_cpp_properties.json' });
    expect(runner.calls).toEqual([{ root: 'd:\\a', args: ['sync'] }]);
  });

  it('命令以非零退出码结束：失败，原因取标准错误，没有时取标准输出，再没有给默认说明', async () => {
    const results = [
      { exitCode: 1, stdout: '', stderr: '错误：找不到 cc65 工具链\n' },
      { exitCode: 1, stdout: '输出里的原因', stderr: '' },
      { exitCode: 1, stdout: '', stderr: '' }
    ];
    const runner = new FakeRunner(async () => results.shift() ?? OK);
    const service = new RunnerIntelliSenseSyncService(runner, locator());
    expect((await service.syncProject('d:\\a')).message).toBe('错误：找不到 cc65 工具链');
    expect((await service.syncProject('d:\\a')).message).toBe('输出里的原因');
    expect((await service.syncProject('d:\\a')).message).toBe('fcdev sync 失败，没有输出原因');
  });

  it('可预期的错误转成失败结果，其他异常原样抛出', async () => {
    const expected = new FakeRunner(async () => {
      throw new UserFacingError('项目里没有安装 fcdev');
    });
    expect(await new RunnerIntelliSenseSyncService(expected, locator()).syncProject('d:\\a')).toEqual({
      root: 'd:\\a',
      ok: false,
      message: '项目里没有安装 fcdev'
    });
    const unexpected = new FakeRunner(async () => {
      throw new Error('代码缺陷');
    });
    await expect(new RunnerIntelliSenseSyncService(unexpected, locator()).syncProject('d:\\a')).rejects.toThrow('代码缺陷');
  });

  it('同一项目的多次同步依次运行，上一次失败不影响下一次', async () => {
    let first = true;
    const runner = new FakeRunner(async () => {
      await new Promise((resolve) => setTimeout(resolve, 20));
      if (first) {
        first = false;
        throw new Error('第一次出错');
      }
      return OK;
    });
    const service = new RunnerIntelliSenseSyncService(runner, locator());
    const one = service.syncProject('d:\\a');
    const two = service.syncProject('d:\\a');
    await expect(one).rejects.toThrow('第一次出错');
    expect((await two).ok).toBe(true);
    expect(runner.log).toEqual(['开始 d:\\a', '结束 d:\\a', '开始 d:\\a', '结束 d:\\a']);
  });
});

describe('同步工作区', () => {
  it('对每个 fcdev 项目各同步一次，没有项目时返回空数组', async () => {
    const runner = new FakeRunner(async () => OK);
    const service = new RunnerIntelliSenseSyncService(runner, locator('d:\\a', 'd:\\b'));
    const results = await service.syncWorkspace();
    expect(results.map((result) => result.root)).toEqual(['d:\\a', 'd:\\b']);
    expect(await new RunnerIntelliSenseSyncService(runner, locator()).syncWorkspace()).toEqual([]);
  });
});
