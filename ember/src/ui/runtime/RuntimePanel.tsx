/*
 * ------------------------------------------------------------------------
 * 名称：RuntimePanel.tsx
 * 说明：底部面板的“运行”内容，没有分组标题，排成一行：四个控制按钮（开机或断电、暂停或继续、软复位、硬复位）、声音（静音按钮、音量滑块与数值）；放不下时换行。主机型号、卡带、mapper 插件显示在系统栏，不在面板里。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.1 节；速度下拉等核心支持速度请求后再做。按钮可用状态由控制器的 availability 决定，操作结束后焦点回到画面（控制器负责）；
 *       拖动音量滑块与点静音时不把焦点抢回画面。
 * ------------------------------------------------------------------------
 */

import { Button, iconPlayerPause, iconPlayerPlay, iconPower, iconRefresh, iconRotate, iconVolume, iconVolumeOff, Slider } from '@rujianjs/ui-kit';
import type { RuntimeController } from './runtime-controller';
import { pauseToggleText, powerToggleText } from './runtime-status';
import { useRuntimeState } from './use-runtime-state';
import './ember-run.css';

/** 用户可见的文字。 */
const TEXT = {
  softReset: '软复位',
  hardReset: '硬复位',
  volume: '音量',
  mute: '静音'
} as const;

/** 运行面板的属性。 */
export interface RuntimePanelProps {
  readonly controller: RuntimeController;
}

/**
 * 底部面板的“运行”内容。
 * @param props 属性。
 */
export function RuntimePanel({ controller }: RuntimePanelProps) {
  const { status, volume, muted } = useRuntimeState(controller);
  const available = controller.availability();
  const running = status?.power === 'on' && status.mode === 'running';

  return (
    <div className="ember-run-panel">
      <div className="ember-run-panel__buttons">
        <Button variant="primary" icon={iconPower} disabled={!available.powerToggle} onClick={() => controller.togglePower()}>
          {powerToggleText(status)}
        </Button>
        <Button icon={running ? iconPlayerPause : iconPlayerPlay} disabled={!available.pauseToggle} onClick={() => controller.togglePause()}>
          {pauseToggleText(status)}
        </Button>
        <Button icon={iconRotate} disabled={!available.softReset} onClick={() => controller.softReset()}>
          {TEXT.softReset}
        </Button>
        <Button icon={iconRefresh} disabled={!available.hardReset} onClick={() => controller.hardReset()}>
          {TEXT.hardReset}
        </Button>
      </div>
      <div className="ember-run-panel__volume">
        <Button variant="ghost" iconOnly compact icon={muted ? iconVolumeOff : iconVolume} pressed={muted} title={TEXT.mute} onClick={() => controller.toggleMute()}>
          {TEXT.mute}
        </Button>
        <Slider className="ember-run-panel__slider" aria-label={TEXT.volume} value={volume} onChange={(next) => controller.setVolume(next)} />
        <span className="ember-run-panel__volume-value">{volume}%</span>
      </div>
    </div>
  );
}
