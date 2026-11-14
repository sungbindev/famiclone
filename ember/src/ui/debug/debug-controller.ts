/*
 * ------------------------------------------------------------------------
 * 名称：debug-controller.ts
 * 说明：调试服务状态的控制器：核心就绪后读取 debug.status，收到 debug.statusChanged 或核心重启后重新读取，供状态栏项显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：不依赖 React，状态放在外部存储里（subscribe 与 snapshot，配合 useSyncExternalStore）；由常驻组件 DebugHost 在外壳启动后接上预加载脚本的接口。
 *       状态事件里没有出错原因，所以每次变化都重新请求 debug.status；读取失败或核心不可用时回到“还没读到”。
 * ------------------------------------------------------------------------
 */

import type { EmberBridge } from '../../contracts/shell/shell-bridge';

/** 控制器用到的预加载接口。 */
export type DebugBackend = Pick<EmberBridge, 'core' | 'onCoreEvent' | 'onCoreStatusChange'>;

/** 界面显示用的调试服务状态。 */
export type DebugViewState =
  | { readonly kind: 'unknown' }
  | {
      readonly kind: 'ready';
      readonly enabled: boolean;
      readonly listening: boolean;
      readonly port: number;
      /** 当前的连接数。 */
      readonly connections: number;
      /** 开启了但没有监听时的原因。 */
      readonly error?: string;
    };

/** 调试服务状态的控制器。 */
export class DebugController {
  private state: DebugViewState = { kind: 'unknown' };
  private readonly listeners = new Set<() => void>();
  /** 最近一次读取的序号：较早发出的读取结果晚到时丢弃。 */
  private generation = 0;

  /** 订阅状态变化；返回取消订阅的函数。 */
  readonly subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };

  /** 当前状态。 */
  readonly snapshot = (): DebugViewState => this.state;

  /**
   * 接上预加载脚本的接口并开始读取；返回断开的函数。
   * @param backend 预加载脚本暴露的接口。
   */
  attach(backend: DebugBackend): () => void {
    let alive = true;
    const load = (): void => {
      this.generation += 1;
      const current = this.generation;
      backend.core.debug.status({}).then(
        (result) => {
          if (!alive || current !== this.generation) return;
          this.setState({
            kind: 'ready',
            enabled: result.enabled,
            listening: result.listening,
            port: result.port,
            connections: result.connections.length,
            ...(result.error === undefined ? {} : { error: result.error })
          });
        },
        () => {
          if (alive && current === this.generation) this.setState({ kind: 'unknown' });
        }
      );
    };
    const stopEvents = backend.onCoreEvent((name) => {
      if (name === 'debug.statusChanged') load();
    });
    const stopStatus = backend.onCoreStatusChange((status) => {
      if (status.state === 'ready') {
        load();
        return;
      }
      this.generation += 1;
      if (alive) this.setState({ kind: 'unknown' });
    });
    load();
    return () => {
      alive = false;
      stopEvents();
      stopStatus();
    };
  }

  private setState(next: DebugViewState): void {
    this.state = next;
    for (const listener of this.listeners) listener();
  }
}
