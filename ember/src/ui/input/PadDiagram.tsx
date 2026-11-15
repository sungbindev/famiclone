/*
 * ------------------------------------------------------------------------
 * 名称：PadDiagram.tsx
 * 说明：手柄图：画出手柄外形，10 个键各是图上的一个按钮，按钮上写键名与当前的绑定；点哪个键就由使用方开始设置哪个键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：本组件只显示与转发点击，不保存数据；按钮是原生 button，键盘可以用 Tab 逐个到达；位置与大小见 pad-layout.ts，样式见 ember-input.css。
 *       当前选中的键 aria-pressed 为真，正在等待按键的键 data-learning 为真（外观用强调色边框）。
 * ------------------------------------------------------------------------
 */

import type { CSSProperties } from 'react';
import type { PadButtonName } from './input-model';
import { PAD_BODY, PAD_CANVAS, PAD_CROSS_CENTER, PAD_LAYOUT, percentOf } from './pad-layout';
import './ember-input.css';

/** 图上一个键要显示的内容。 */
export interface PadHotspot {
  readonly id: PadButtonName;
  /** 键名，如“上”“A”“连发 A”。 */
  readonly role: string;
  /** 当前的绑定文字；没有绑定时为“未绑定”。 */
  readonly current: string;
  readonly unbound: boolean;
  /** 按钮的可访问名称与悬停提示里的完整说明。 */
  readonly ariaLabel: string;
  readonly selected: boolean;
  readonly learning: boolean;
}

/** 手柄图的属性。 */
export interface PadDiagramProps {
  readonly hotspots: readonly PadHotspot[];
  /** 整张图的按钮都不可点（如还没有指定手柄）。 */
  readonly disabled: boolean;
  readonly onPick: (button: PadButtonName) => void;
}

/** 位置与大小经 CSS 自定义属性交给样式。 */
function positionStyle(left: number, top: number, width: number, height: number | undefined): CSSProperties {
  return {
    '--pad-left': `${left}%`,
    '--pad-top': `${top}%`,
    '--pad-width': `${width}%`,
    ...(height === undefined ? {} : { '--pad-height': `${height}%` })
  } as CSSProperties;
}

/**
 * 手柄图。
 * @param props 属性。
 */
export function PadDiagram({ hotspots, disabled, onPick }: PadDiagramProps) {
  return (
    <div className="ember-pad">
      <svg className="ember-pad__body" viewBox={`0 0 ${PAD_CANVAS.width} ${PAD_CANVAS.height}`} aria-hidden="true" focusable="false">
        <rect x={PAD_BODY.x} y={PAD_BODY.y} width={PAD_BODY.width} height={PAD_BODY.height} rx={PAD_BODY.radius} />
        <rect className="ember-pad__cross" x={PAD_CROSS_CENTER.x} y={PAD_CROSS_CENTER.y} width={PAD_CROSS_CENTER.width} height={PAD_CROSS_CENTER.height} />
      </svg>
      {PAD_LAYOUT.map((layout) => {
        const hotspot = hotspots.find((item) => item.id === layout.id);
        if (hotspot === undefined) return null;
        const place = percentOf(layout);
        return (
          <button
            key={layout.id}
            type="button"
            className={`ember-pad__key ember-pad__key--${layout.shape}`}
            style={positionStyle(place.left, place.top, place.width, place.height)}
            aria-label={hotspot.ariaLabel}
            aria-pressed={hotspot.selected}
            data-learning={hotspot.learning || undefined}
            title={hotspot.ariaLabel}
            disabled={disabled}
            onClick={() => onPick(layout.id)}
          >
            <span className="ember-pad__role">{hotspot.role}</span>
            <span className="ember-pad__binding" data-unbound={hotspot.unbound || undefined}>
              {hotspot.current}
            </span>
          </button>
        );
      })}
    </div>
  );
}
