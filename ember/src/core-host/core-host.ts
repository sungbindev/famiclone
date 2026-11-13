/*
 * ------------------------------------------------------------------------
 * 名称：core-host.ts
 * 说明：核心进程入口：加载 C++ 原生模块，把主进程的消息转为原生模块的调用，把结果与事件发回主进程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只做转接，不含业务判断；原生模块的参数与结果是 JSON 文本，这里负责与 JSON 值之间的转换。
 *       消息按收到的顺序处理：start 同步完成，之后的请求才会交给原生模块。
 * ------------------------------------------------------------------------
 */

import { CORE_ERROR_CODES, type CoreErrorInfo, type CoreHostInbound, type CoreHostOutbound } from '../contracts/core-gateway/core-messages';

/** 原生模块的导出。 */
interface NativeCore {
  start(optionsJson: string): string;
  request(id: string, name: string, paramsJson: string): Promise<string>;
  cancel(id: string): void;
  onEvent(callback: (name: string, dataJson: string) => void): void;
  stop(): Promise<void>;
}

/** 用户可见的说明。 */
const TEXT = {
  notStarted: '核心未启动',
  loadFailed: '无法加载核心模块',
  badResult: '核心返回的数据无法解析'
} as const;

let native: NativeCore | undefined;

/** 发给主进程。 */
function send(message: CoreHostOutbound): void {
  process.parentPort.postMessage(message);
}

/** 把异常转为错误码与说明。 */
function errorInfo(error: unknown, fallback: string): CoreErrorInfo {
  if (error instanceof Error) {
    const code = (error as Error & { code?: unknown }).code;
    return { code: typeof code === 'string' && code.includes('.') ? code : CORE_ERROR_CODES.internal, message: error.message || fallback };
  }
  return { code: CORE_ERROR_CODES.internal, message: fallback };
}

/** 加载原生模块。 */
function loadNative(nativePath: string): NativeCore {
  const module = { exports: {} as NativeCore };
  process.dlopen(module, nativePath);
  return module.exports;
}

/** 启动核心。 */
function start(message: Extract<CoreHostInbound, { type: 'start' }>): void {
  try {
    const loaded = loadNative(message.nativePath);
    loaded.onEvent((name, dataJson) => {
      try {
        send({ type: 'event', name, data: JSON.parse(dataJson) as unknown });
      } catch {
        // 无法解析的事件丢弃。
      }
    });
    const { userDataPath, appVersion } = message;
    const info = JSON.parse(loaded.start(JSON.stringify({ userDataPath, appVersion }))) as { coreVersion: string };
    native = loaded;
    send({ type: 'ready', coreVersion: info.coreVersion });
  } catch (error) {
    send({ type: 'startFailed', error: errorInfo(error, TEXT.loadFailed) });
  }
}

/** 转发请求。 */
function request(id: string, name: string, params: unknown): void {
  if (native === undefined) {
    send({ type: 'response', id, ok: false, error: { code: CORE_ERROR_CODES.unavailable, message: TEXT.notStarted } });
    return;
  }
  native.request(id, name, JSON.stringify(params ?? {})).then(
    (text) => {
      try {
        send({ type: 'response', id, ok: true, result: JSON.parse(text) as unknown });
      } catch {
        send({ type: 'response', id, ok: false, error: { code: CORE_ERROR_CODES.internal, message: TEXT.badResult } });
      }
    },
    (error: unknown) => send({ type: 'response', id, ok: false, error: errorInfo(error, TEXT.notStarted) })
  );
}

/** 停止核心并退出进程。 */
async function stop(): Promise<void> {
  try {
    await native?.stop();
  } finally {
    process.exit(0);
  }
}

process.parentPort.on('message', (event) => {
  const message = event.data as CoreHostInbound;
  switch (message.type) {
    case 'start':
      start(message);
      return;
    case 'request':
      request(message.id, message.name, message.params);
      return;
    case 'cancel':
      native?.cancel(message.id);
      return;
    case 'stop':
      void stop();
      return;
    default:
      return;
  }
});
