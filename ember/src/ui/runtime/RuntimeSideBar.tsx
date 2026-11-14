/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeSideBar.tsx
 * 说明：运行页的边栏：机器（主机型号、卡带、mapper 插件、状态，只读）、控制（上电或断电、暂停或继续、软复位、硬复位）、声音（静音按钮、音量滑块与数值）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：规则见 docs/05-界面.md 第 4.1 节；速度下拉等核心支持速度请求后再做。按钮可用状态由控制器的 availability 决定，操作结束后焦点回到画面（控制器负责）；
 *       拖动音量滑块与点静音时不把焦点抢回画面。
 * ------------------------------------------------------------------------
 */

import { Button, Facts, GroupTitle, iconPlayerPause, iconPlayerPlay, iconPower, iconRefresh, iconRotate, iconVolume, iconVolumeOff, Slider, Stack } from '@rujianjs/ui-kit';
import type { RuntimeController } from './runtime-controller';
import { cartridgeText, controlAvailability, hostText, pauseToggleText, pluginText, powerToggleText, stateText } from './runtime-status';
import { useRuntimeState } from './use-runtime-state';

/** 用户可见的文字。 */
const TEXT = {
  machine: '机器',
  host: '主机型号',
  cartridge: '卡带',
  plugin: 'mapper 插件',
  state: '状态',
  control: '控制',
  softReset: '软复位',
  hardReset: '硬复位',
  sound: '声音',
  volume: '音量',
  mute: '静音'
} as const;

/** 运行页边栏的属性。 */
export interface RuntimeSideBarProps {
  readonly controller: RuntimeController;
}

/**
 * 运行页的边栏。
 * @param props 属性。
 */
export function RuntimeSideBar({ controller }: RuntimeSideBarProps) {
  const { status, volume, muted } = useRuntimeState(controller);
  const available = controlAvailability(status);
  const running = status?.power === 'on' && status.mode === 'running';

  return (
    <Stack className="ember-run-side">
      <Stack className="ember-run-side__group">
        <GroupTitle>{TEXT.machine}</GroupTitle>
        <Facts
          aria-label={TEXT.machine}
          items={[
            { label: TEXT.host, value: hostText(status) },
            { label: TEXT.cartridge, value: cartridgeText(status) },
            { label: TEXT.plugin, value: pluginText(status) },
            { label: TEXT.state, value: stateText(status) }
          ]}
        />
      </Stack>
      <Stack className="ember-run-side__group">
        <GroupTitle>{TEXT.control}</GroupTitle>
        <div className="ember-run-side__buttons">
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
      </Stack>
      <Stack className="ember-run-side__group">
        <GroupTitle>{TEXT.sound}</GroupTitle>
        <div className="ember-run-side__volume">
          <Button variant="ghost" iconOnly compact icon={muted ? iconVolumeOff : iconVolume} pressed={muted} title={TEXT.mute} onClick={() => controller.toggleMute()}>
            {TEXT.mute}
          </Button>
          <Slider className="ember-run-side__slider" aria-label={TEXT.volume} value={volume} onChange={(next) => controller.setVolume(next)} />
          <span className="ember-run-side__volume-value">{volume}%</span>
        </div>
      </Stack>
    </Stack>
  );
}
