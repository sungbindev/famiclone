// ------------------------------------------------------------------------
// 名称：emulator-session.ts
// 说明：模拟器操作的当前会话：一个已连接的调试会话，加上启动它的进程号与正在进行的记录；一个存储里
//       只有一个会话。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：会话放在存储对象里，所以多次调用之间保持连接；连接断开时自动清除；只结束自己启动的进程，不
//       碰用户自己打开的 Ember。
// ------------------------------------------------------------------------

import type { DebugSession } from '../../contracts/debug-client/debug-session';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 一个到 Ember 的会话。 */
export interface EmulatorSession {
  /** 已连接的调试会话。 */
  readonly client: DebugSession;
  /** 调试端口；不是由这里启动或不知道时为 undefined。 */
  readonly port: number | undefined;
  /** 这里启动的 Ember 的进程号；连接别人的 Ember 时为 undefined。 */
  readonly pid: number | undefined;
  /** 正在进行的跟踪与调用记录：编号到种类。 */
  readonly records: Map<number, { kind: string }>;
  /** 正在进行的 CDL 标记。 */
  readonly flags: { cdl: boolean };
}

/** 建立会话时的来源。 */
export interface SessionSource {
  readonly client: DebugSession;
  readonly port?: number | undefined;
  readonly pid?: number | undefined;
}

/** 会话关闭的结果。 */
export interface SessionClosed {
  /** 是否关闭了会话。 */
  closed: boolean;
  /** 是否结束了进程。 */
  terminated: boolean;
}

/** 当前会话的存储。 */
export class EmulatorSessionStore {
  private current: EmulatorSession | null = null;

  /** @param terminate 结束进程的函数，返回是否结束了进程。 */
  constructor(private readonly terminate: (pid: number) => boolean) {}

  /**
   * 取当前会话。
   * @returns 会话。
   * @throws FcdevError 还没有连接。
   */
  currentSession(): EmulatorSession {
    if (this.current === null) {
      throw new FcdevError('还没有连接 Ember', {
        code: 'EMULATOR_NOT_CONNECTED',
        hint: '先用 emu_connect 连接已运行且开启了调试服务的 Ember，或用 emu_launch 启动一个'
      });
    }
    return this.current;
  }

  /**
   * 是否已有会话。
   * @returns 有会话时为 true。
   */
  hasSession(): boolean {
    return this.current !== null;
  }

  /**
   * 建立新会话；已有会话先关闭（只关闭连接，不结束进程）。
   * @param source 调试会话与来源。
   * @returns 新会话。
   */
  openSession({ client, port, pid }: SessionSource): EmulatorSession {
    this.closeSession();
    const session: EmulatorSession = { client, port, pid, records: new Map(), flags: { cdl: false } };
    this.current = session;
    void client.closed.then(() => {
      if (this.current === session) this.current = null;
    });
    return session;
  }

  /**
   * 关闭当前会话。
   * @param options terminate 为 true 时结束自己启动的 Ember 进程。
   * @returns 是否关闭了会话、是否结束了进程。
   */
  closeSession({ terminate = false }: { terminate?: boolean } = {}): SessionClosed {
    const session = this.current;
    if (session === null) return { closed: false, terminated: false };
    this.current = null;
    try {
      session.client.close();
    } catch {
      // 连接已经断开
    }
    const terminated = terminate && session.pid !== undefined && this.terminate(session.pid);
    return { closed: true, terminated };
  }
}
