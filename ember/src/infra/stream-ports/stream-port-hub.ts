/*
 * ------------------------------------------------------------------------
 * 名称：stream-port-hub.ts
 * 说明：画面与输入端口的建立：页面请求端口时创建两条通道，一端交给核心进程，另一端交给请求的窗口；核心进程重启后重新建立。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：采用“页面请求”的方式，页面先监听再请求，不会因为页面还没加载好而丢端口；页面重载后再次请求会得到新的端口；
 *       核心进程重启时（旧端口随进程失效）自动给当前窗口新的端口。同一时间只有一个窗口持有端口，后来的请求替换先前的。
 * ------------------------------------------------------------------------
 */

import type { ErrorLogger } from '../../contracts/logging/error-logger';
import type { CoreStreamSink, StreamChannelFactory, StreamTarget } from '../../contracts/stream-ports/stream-ports';

/** 主进程端口错误的错误码。 */
const STREAM_ERROR_CODE = 'shell.streamPortsFailed';

/** 端口集线器的依赖。 */
export interface StreamPortHubOptions {
  readonly channels: StreamChannelFactory;
  readonly core: CoreStreamSink;
  readonly logger: ErrorLogger;
}

/** 画面与输入端口的集线器。 */
export class StreamPortHub {
  private target: StreamTarget | undefined;
  private unsubscribe: (() => void) | undefined;

  constructor(private readonly options: StreamPortHubOptions) {}

  /** 开始订阅核心进程的启动：核心每次启动（含重启）都给当前窗口重新建立端口。 */
  start(): void {
    this.unsubscribe ??= this.options.core.onCoreStarted(() => this.establish());
  }

  /** 停止订阅并忘记窗口。 */
  stop(): void {
    this.unsubscribe?.();
    this.unsubscribe = undefined;
    this.target = undefined;
  }

  /**
   * 窗口请求端口：记住这个窗口，并立即建立一对新的端口交给它和核心进程；旧端口作废。
   * 核心进程暂时不在运行时只记住窗口，等核心进程启动后再建立。
   */
  connect(target: StreamTarget): void {
    // 同一个窗口重复请求时只更新引用，销毁通知只登记一次；销毁通知按窗口标识判断，不受引用更新的影响。
    if (this.target?.id !== target.id) {
      target.onDestroyed(() => {
        if (this.target?.id === target.id) this.target = undefined;
      });
    }
    this.target = target;
    this.establish();
  }

  /** 创建两条通道：核心进程拿 first，窗口拿 second。 */
  private establish(): void {
    const target = this.target;
    if (target === undefined || !target.isAlive()) return;
    try {
      const frames = this.options.channels.create();
      const input = this.options.channels.create();
      if (!this.options.core.attachStreams({ frames: frames.first, input: input.first })) return;
      target.send({ frames: frames.second, input: input.second });
    } catch (error) {
      this.options.logger.write({ process: 'main', code: STREAM_ERROR_CODE, message: `无法建立画面与输入端口：${error instanceof Error ? error.message : String(error)}` });
    }
  }
}
