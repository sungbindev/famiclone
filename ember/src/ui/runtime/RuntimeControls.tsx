/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeControls.tsx
 * 说明：运行页的开发期操作条：打开卡带、上电与断电、暂停与继续、软复位与硬复位、音量与静音，以及状态文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只为开发期验证准备，阶段 3a 会迁移到边栏。请求都经预加载脚本暴露的具名方法；状态文字取自 runtime.status，
 *       收到 runtime.stateChanged 与 cartridges.loaded 事件时重新读取；错误都经通知给出，不在页面里显示。操作结束后把焦点还给画面区。
 * ------------------------------------------------------------------------
 */

import { Button, Checkbox, Row, Slider, useNotify } from '@rujianjs/ui-kit';
import { useCallback, useEffect, useRef, useState, type RefObject } from 'react';
import type { CoreErrorInfo } from '../../contracts/core-gateway/core-messages';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { controlAvailability, statusText } from './runtime-status';

const TEXT = {
  open: '打开卡带…',
  powerOn: '上电',
  powerOff: '断电',
  pause: '暂停',
  resume: '继续',
  softReset: '软复位',
  hardReset: '硬复位',
  volume: '音量',
  mute: '静音',
  failed: '操作失败'
} as const;

/** 状态变化时需要重新读取状态的事件。 */
const STATUS_EVENTS: ReadonlySet<string> = new Set(['runtime.stateChanged', 'cartridges.loaded']);

/** 操作条的属性。 */
export interface RuntimeControlsProps {
  readonly bridge: EmberBridge;
  /** 画面区的引用：操作结束后把焦点还给它，键盘才能继续控制游戏。 */
  readonly screenRef: RefObject<HTMLDivElement | null>;
}

/**
 * 开发期操作条。
 * @param props 属性。
 */
export function RuntimeControls({ bridge, screenRef }: RuntimeControlsProps) {
  const notify = useNotify();
  const [status, setStatus] = useState<RuntimeStatusResult | undefined>(undefined);
  const [volume, setVolume] = useState(100);
  const [muted, setMuted] = useState(false);
  /** 最近一次读取的编号：迟到的旧结果不覆盖新结果。 */
  const requestId = useRef(0);

  const report = useCallback(
    (error: CoreErrorInfo): void => {
      notify(`${TEXT.failed}：${error.message}`, 'error');
    },
    [notify]
  );

  const refresh = useCallback((): void => {
    requestId.current += 1;
    const current = requestId.current;
    bridge.core.runtime.status({}).then(
      (result) => {
        if (requestId.current === current) setStatus(result);
      },
      (error: CoreErrorInfo) => report(error)
    );
  }, [bridge, report]);

  useEffect(() => {
    refresh();
    return bridge.onCoreEvent((name) => {
      if (STATUS_EVENTS.has(name)) refresh();
    });
  }, [bridge, refresh]);

  /** 执行一个操作：成功后重新读取状态，失败经通知给出说明，结束后焦点回到画面。 */
  const run = useCallback(
    (action: () => Promise<unknown>): void => {
      action()
        .then(refresh, report)
        .finally(() => screenRef.current?.focus({ preventScroll: true }));
    },
    [refresh, report, screenRef]
  );

  /** 调整声音：只报告失败，不重读状态；拖动音量滑块时不能抢走它的焦点，所以不还焦点。 */
  const adjustVolume = useCallback(
    (params: { volume?: number; muted?: boolean }): void => {
      bridge.core.output.setVolume(params).then(undefined, report);
    },
    [bridge, report]
  );

  const available = controlAvailability(status);
  const running = status?.power === 'on' && status.mode === 'running';
  const powered = status?.power === 'on';

  return (
    <Row className="ember-run-controls">
      <Button onClick={() => run(() => bridge.main.cartridges.open({}))}>{TEXT.open}</Button>
      <Button disabled={!available.powerToggle} onClick={() => run(() => (powered ? bridge.core.runtime.powerOff({}) : bridge.core.runtime.powerOn({ resume: true })))}>
        {powered ? TEXT.powerOff : TEXT.powerOn}
      </Button>
      <Button disabled={!available.pauseToggle} onClick={() => run(() => (running ? bridge.core.runtime.pause({}) : bridge.core.runtime.resume({})))}>
        {running ? TEXT.pause : TEXT.resume}
      </Button>
      <Button disabled={!available.softReset} onClick={() => run(() => bridge.core.runtime.reset({ kind: 'soft' }))}>
        {TEXT.softReset}
      </Button>
      <Button disabled={!available.hardReset} onClick={() => run(() => bridge.core.runtime.reset({ kind: 'hard' }))}>
        {TEXT.hardReset}
      </Button>
      <Slider
        className="ember-run-controls__volume"
        aria-label={TEXT.volume}
        value={volume}
        onChange={(next) => {
          setVolume(next);
          adjustVolume({ volume: next });
        }}
      />
      <Checkbox
        label={TEXT.mute}
        checked={muted}
        onChange={(next) => {
          setMuted(next);
          adjustVolume({ muted: next });
        }}
      />
      <span role="status">{statusText(status)}</span>
    </Row>
  );
}
