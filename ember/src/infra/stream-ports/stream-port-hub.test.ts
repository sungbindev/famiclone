/*
 * ------------------------------------------------------------------------
 * 名称：stream-port-hub.test.ts
 * 说明：端口集线器的测试：页面请求时建立两条通道并分别交给核心与窗口、核心不在运行时等它启动、核心重启后重建、页面重载后换新端口、窗口销毁后不再发送、失败写日志不抛出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的通道工厂、假的核心入口与假的窗口，不依赖 Electron。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { ErrorLogEntry } from '../../contracts/logging/error-logger';
import type { CoreStreamSink, StreamChannelFactory, StreamPortPair, StreamTarget } from '../../contracts/stream-ports/stream-ports';
import { StreamPortHub } from './stream-port-hub';

/** 假端口：只有名字。 */
interface FakePort {
  readonly name: string;
}

/** 假通道工厂：按创建顺序给端口命名。 */
function createChannels(): StreamChannelFactory & { readonly created: number } {
  let created = 0;
  return {
    get created() {
      return created;
    },
    create() {
      created += 1;
      return { first: { name: `c${created}-first` } satisfies FakePort, second: { name: `c${created}-second` } satisfies FakePort };
    }
  };
}

/** 假核心入口：记录交来的端口，可以切换是否接受。 */
function createCore(accepting = true) {
  const attached: StreamPortPair[] = [];
  const listeners = new Set<() => void>();
  const state = { accepting };
  const core: CoreStreamSink = {
    attachStreams: (ports) => {
      if (!state.accepting) return false;
      attached.push(ports);
      return true;
    },
    onCoreStarted: (listener) => {
      listeners.add(listener);
      return () => listeners.delete(listener);
    }
  };
  return { core, attached, state, started: () => [...listeners].forEach((listener) => listener()), listenerCount: () => listeners.size };
}

/** 假窗口。 */
function createTarget(id = 1) {
  const received: StreamPortPair[] = [];
  const destroyed: Array<() => void> = [];
  const state = { alive: true, failing: false };
  const target: StreamTarget = {
    id,
    isAlive: () => state.alive,
    send: (ports) => {
      if (state.failing) throw new Error('窗口已销毁');
      received.push(ports);
    },
    onDestroyed: (listener) => destroyed.push(listener)
  };
  return { target, received, state, destroy: () => destroyed.forEach((listener) => listener()), destroyedListeners: destroyed };
}

function setup(accepting = true) {
  const channels = createChannels();
  const core = createCore(accepting);
  const logs: ErrorLogEntry[] = [];
  const hub = new StreamPortHub({ channels, core: core.core, logger: { write: (entry) => logs.push(entry) } });
  hub.start();
  return { hub, channels, core, logs };
}

/** 端口的名字。 */
function names(ports: StreamPortPair): [string, string] {
  return [(ports.frames as FakePort).name, (ports.input as FakePort).name];
}

describe('StreamPortHub', () => {
  it('页面请求时建立画面与输入两条通道：核心拿第一端，窗口拿第二端', () => {
    const { hub, core } = setup();
    const window = createTarget();
    hub.connect(window.target);
    expect(core.attached.map(names)).toEqual([['c1-first', 'c2-first']]);
    expect(window.received.map(names)).toEqual([['c1-second', 'c2-second']]);
  });

  it('核心不在运行时不发端口，核心启动后再建立', () => {
    const { hub, core } = setup(false);
    const window = createTarget();
    hub.connect(window.target);
    expect(window.received).toHaveLength(0);
    core.state.accepting = true;
    core.started();
    expect(core.attached).toHaveLength(1);
    expect(window.received).toHaveLength(1);
  });

  it('核心进程重启后给当前窗口重新建立端口', () => {
    const { hub, core } = setup();
    const window = createTarget();
    hub.connect(window.target);
    core.started();
    expect(window.received.map(names)).toEqual([
      ['c1-second', 'c2-second'],
      ['c3-second', 'c4-second']
    ]);
    expect(core.attached.map(names)).toEqual([
      ['c1-first', 'c2-first'],
      ['c3-first', 'c4-first']
    ]);
  });

  it('没有窗口请求过端口时核心启动不建立端口', () => {
    const { core, channels } = setup();
    core.started();
    expect(channels.created).toBe(0);
    expect(core.attached).toHaveLength(0);
  });

  it('同一个窗口重载页面后再次请求，得到新的端口，销毁通知只登记一次', () => {
    const { hub, core } = setup();
    const window = createTarget();
    hub.connect(window.target);
    hub.connect(window.target);
    expect(window.received).toHaveLength(2);
    expect(names(window.received[1]!)).toEqual(['c3-second', 'c4-second']);
    expect(core.attached).toHaveLength(2);
    expect(window.destroyedListeners).toHaveLength(1);
  });

  it('另一个窗口请求时替换先前的窗口', () => {
    const { hub, core } = setup();
    const first = createTarget(1);
    const second = createTarget(2);
    hub.connect(first.target);
    hub.connect(second.target);
    core.started();
    expect(first.received).toHaveLength(1);
    expect(second.received).toHaveLength(2);
  });

  it('窗口销毁后核心启动不再发送', () => {
    const { hub, core, channels } = setup();
    const window = createTarget();
    hub.connect(window.target);
    window.destroy();
    const before = channels.created;
    core.started();
    expect(channels.created).toBe(before);
    expect(window.received).toHaveLength(1);
  });

  it('窗口已不存在时不发送', () => {
    const { hub, core } = setup();
    const window = createTarget();
    hub.connect(window.target);
    window.state.alive = false;
    core.started();
    expect(window.received).toHaveLength(1);
  });

  it('发送给窗口失败时写日志，不抛出', () => {
    const { hub, logs } = setup();
    const window = createTarget();
    window.state.failing = true;
    expect(() => hub.connect(window.target)).not.toThrow();
    expect(logs).toHaveLength(1);
    expect(logs[0]).toMatchObject({ process: 'main', code: 'shell.streamPortsFailed' });
    expect(logs[0]?.message).toContain('窗口已销毁');
  });

  it('停止后取消订阅并忘记窗口', () => {
    const { hub, core } = setup();
    const window = createTarget();
    hub.connect(window.target);
    expect(core.listenerCount()).toBe(1);
    hub.stop();
    expect(core.listenerCount()).toBe(0);
    core.started();
    expect(window.received).toHaveLength(1);
  });

  it('重复调用 start 只订阅一次', () => {
    const { hub, core } = setup();
    hub.start();
    expect(core.listenerCount()).toBe(1);
  });
});
