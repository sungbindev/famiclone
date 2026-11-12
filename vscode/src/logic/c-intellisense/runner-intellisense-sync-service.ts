// ------------------------------------------------------------------------
// 名称：runner-intellisense-sync-service.ts
// 说明：C 智能感知配置同步服务的实现：经命令行运行 `fcdev sync`，同一项目的同步依次进行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：可预期的错误（没装 fcdev、找不到 node）转成失败结果；其他异常原样抛出。
// ------------------------------------------------------------------------

import type { IntelliSenseSyncService, SyncResult } from '../../contracts/c-intellisense/intellisense-sync-service';
import type { FcdevCommandRunner, FcdevProjectLocator } from '../../contracts/project/fcdev-command-runner';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 命令没有输出时的成功说明。 */
const DEFAULT_SUCCESS_MESSAGE = '已同步 C/C++ 配置';

/** 命令失败又没有任何输出时的说明。 */
const DEFAULT_FAILURE_MESSAGE = 'fcdev sync 失败，没有输出原因';

/** 经命令行运行 fcdev sync 的同步服务。 */
export class RunnerIntelliSenseSyncService implements IntelliSenseSyncService {
  private readonly queues = new Map<string, Promise<unknown>>();

  /**
   * 创建服务。
   * @param runner 运行 fcdev 命令的能力。
   * @param locator 查找工作区里 fcdev 项目的能力。
   */
  constructor(
    private readonly runner: FcdevCommandRunner,
    private readonly locator: FcdevProjectLocator
  ) {}

  /** @inheritdoc */
  syncProject(root: string): Promise<SyncResult> {
    const previous = this.queues.get(root) ?? Promise.resolve();
    const next = previous.then(() => this.runSync(root));
    // 队列只关心先后顺序，上一次的失败不影响下一次。
    this.queues.set(root, next.catch(() => undefined));
    return next;
  }

  /** @inheritdoc */
  async syncWorkspace(): Promise<readonly SyncResult[]> {
    const roots = await this.locator.findProjectRoots();
    return Promise.all(roots.map((root) => this.syncProject(root)));
  }

  /** 运行一次 fcdev sync，把结果整理成同步结果。 */
  private async runSync(root: string): Promise<SyncResult> {
    try {
      const result = await this.runner.run(root, ['sync']);
      if (result.exitCode === 0) return { root, ok: true, message: result.stdout.trim() || DEFAULT_SUCCESS_MESSAGE };
      return { root, ok: false, message: result.stderr.trim() || result.stdout.trim() || DEFAULT_FAILURE_MESSAGE };
    } catch (error) {
      if (error instanceof UserFacingError) return { root, ok: false, message: error.message };
      throw error;
    }
  }
}
