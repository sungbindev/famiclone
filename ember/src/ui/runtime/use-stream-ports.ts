/*
 * ------------------------------------------------------------------------
 * 名称：use-stream-ports.ts
 * 说明：画面与输入端口的页面一侧：监听预加载脚本交来的端口，把画面端口交给渲染 Worker，保留输入端口用来发送键盘、鼠标、游戏手柄事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先监听 message 事件，再调用 requestPorts()；只接受来自本窗口自己、标记与端口数量都对的消息。
 *       核心进程重启或页面再次请求时会收到新的端口，旧的输入端口随即关闭，画面端口由 Worker 一侧替换。
 *       输入消息格式见 src/contracts/stream-ports/stream-messages.ts。
 * ------------------------------------------------------------------------
 */

import { useEffect, useMemo, useRef } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { InputMessage, StreamPortsWindowMessage } from '../../contracts/stream-ports/stream-messages';

/** 预加载脚本交端口时窗口消息的类型，与契约一致。 */
const PORTS_MESSAGE_TYPE: StreamPortsWindowMessage['type'] = 'ember.stream-ports';

/** 端口的个数：画面端口与输入端口。 */
const PORT_COUNT = 2;

/** 一对端口。 */
export interface StreamPortPair {
  readonly frames: MessagePort;
  readonly input: MessagePort;
}

/** 检查窗口消息所需的部分。 */
export interface PortsEventLike {
  readonly source: unknown;
  readonly data: unknown;
  readonly ports: readonly MessagePort[];
}

/**
 * 判断一条窗口消息是不是预加载脚本交来的端口：消息来自本窗口自己、类型对、端口数量对。
 * @param event 窗口消息。
 * @param self 本窗口。
 * @returns 端口；不是时为 undefined。
 */
export function acceptStreamPorts(event: PortsEventLike, self: unknown): StreamPortPair | undefined {
  if (event.source !== self || typeof event.data !== 'object' || event.data === null) return undefined;
  if ((event.data as { type?: unknown }).type !== PORTS_MESSAGE_TYPE || event.ports.length !== PORT_COUNT) return undefined;
  const [frames, input] = event.ports;
  return frames === undefined || input === undefined ? undefined : { frames, input };
}

/** 页面向核心发送输入的接口。 */
export interface InputLink {
  /** 发送一条键盘、鼠标或游戏手柄事件；还没有输入端口时丢弃。 */
  send(message: InputMessage): void;
}

/**
 * 请求端口并接上：画面端口交给 onFramePort，输入端口留在本钩子里。
 * @param bridge 预加载脚本暴露的接口。
 * @param onFramePort 每次收到新的画面端口时调用。
 * @returns 输入发送接口。
 */
export function useStreamPorts(bridge: EmberBridge, onFramePort: (port: MessagePort) => void): InputLink {
  const inputRef = useRef<MessagePort | undefined>(undefined);
  const onFramePortRef = useRef(onFramePort);

  useEffect(() => {
    onFramePortRef.current = onFramePort;
  }, [onFramePort]);

  useEffect(() => {
    const listener = (event: MessageEvent): void => {
      const ports = acceptStreamPorts(event, window);
      if (ports === undefined) return;
      inputRef.current?.close();
      inputRef.current = ports.input;
      onFramePortRef.current(ports.frames);
    };
    window.addEventListener('message', listener);
    bridge.streams.requestPorts();
    return () => {
      window.removeEventListener('message', listener);
      inputRef.current?.close();
      inputRef.current = undefined;
    };
  }, [bridge]);

  return useMemo<InputLink>(
    () => ({
      send: (message) => {
        try {
          inputRef.current?.postMessage(message);
        } catch {
          // 端口已经失效，等新端口到达。
        }
      }
    }),
    []
  );
}
