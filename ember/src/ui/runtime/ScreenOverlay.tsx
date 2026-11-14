/*
 * ------------------------------------------------------------------------
 * 名称：ScreenOverlay.tsx
 * 说明：画面上的提示：没有开机显示“未开机”并指明去“文件”菜单，断电显示“已断电”，暂停显示“已暂停”；运行中与还没读到状态时不显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：覆盖在画面区之上，不接收鼠标事件，所以不改变画面比例，也不影响画面区获得焦点与键盘采集；颜色与画面的黑底一样，不随主题变化（画面是被模拟的内容）。
 * ------------------------------------------------------------------------
 */

import { Icon, iconPlayerPause, iconPower, type IconData } from '@rujianjs/ui-kit';
import type { RuntimeController } from './runtime-controller';
import { machineState, type MachineState } from './runtime-status';
import { useRuntimeState } from './use-runtime-state';

/** 用户可见的文字。 */
const TEXT = {
  none: { title: '未开机', text: '在“文件”菜单的“开机…”或“打开卡带…”里选择要运行的卡带。' },
  off: { title: '已断电', text: '点击边栏的“上电”。' },
  paused: { title: '已暂停', text: '' }
} as const;

/** 各状态的提示：图标、标题与说明；blank 为真时提示的底色是纯黑（没有开机与断电时机器没有画面，盖住画布里残留的上一帧），暂停时是半透明，透出暂停的画面。 */
const HINTS: Partial<Record<MachineState, { icon: IconData; title: string; text: string; blank: boolean }>> = {
  none: { icon: iconPower, ...TEXT.none, blank: true },
  off: { icon: iconPower, ...TEXT.off, blank: true },
  paused: { icon: iconPlayerPause, ...TEXT.paused, blank: false }
};

/** 画面提示的属性。 */
export interface ScreenOverlayProps {
  readonly controller: RuntimeController;
}

/**
 * 画面提示。
 * @param props 属性。
 */
export function ScreenOverlay({ controller }: ScreenOverlayProps) {
  const { status } = useRuntimeState(controller);
  const hint = HINTS[machineState(status)];
  if (hint === undefined) return null;
  return (
    <div className="ember-run-overlay" role="status" data-blank={hint.blank}>
      <Icon icon={hint.icon} className="ember-run-overlay__icon" />
      <div className="ember-run-overlay__title">{hint.title}</div>
      {hint.text ? <div className="ember-run-overlay__text">{hint.text}</div> : null}
    </div>
  );
}
