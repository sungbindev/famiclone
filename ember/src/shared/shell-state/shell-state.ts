/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.ts
 * 说明：界面状态的类型、默认值与校验：窗口位置与大小、是否最大化，以及底部面板高度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：规则见 docs/05-界面.md 第 9 节；读到的内容逐项校验，不合格的项用默认值，不影响其他项；
 *       边栏、底部面板、右边栏是否展开不保存（每次启动都是全部关闭）；旧文件里的这几项读取时被忽略；
 *       渲染进程发来的布局修改同样经 parseLayoutPatch 校验，含未知字段或不合格的值时整条丢弃。
 * ------------------------------------------------------------------------
 */

/** 窗口的位置与大小（非最大化时的边界）以及是否最大化。 */
export interface WindowPlacement {
  readonly x: number;
  readonly y: number;
  readonly width: number;
  readonly height: number;
  readonly maximized: boolean;
}

/** 外壳需要保存的版面：只有底部面板高度，各面板是否展开每次启动都是关闭。 */
export interface ShellLayout {
  /** 底部面板的高度（像素）；null 表示用默认高度。 */
  readonly bottomPanelHeight: number | null;
}

/** 需要保存的全部界面状态。 */
export interface ShellState {
  /** 窗口的位置；null 表示没有保存过，用默认位置。 */
  readonly window: WindowPlacement | null;
  readonly layout: ShellLayout;
}

/** 没有保存过时：窗口用默认位置，底部面板用默认高度。 */
export const DEFAULT_SHELL_STATE: ShellState = {
  window: null,
  layout: { bottomPanelHeight: null }
};

/** 窗口坐标的合理范围（多显示器时可以为负）。 */
const MAX_COORDINATE = 100_000;
/** 窗口尺寸的合理范围。 */
const MAX_SIZE = 20_000;
/** 底部面板高度的合理范围。 */
const MAX_PANEL_HEIGHT = 10_000;

/** 对象形态的值。 */
function asRecord(value: unknown): Record<string, unknown> | undefined {
  return typeof value === 'object' && value !== null && !Array.isArray(value) ? (value as Record<string, unknown>) : undefined;
}

/** 范围内的整数。 */
function isIntegerIn(value: unknown, min: number, max: number): value is number {
  return typeof value === 'number' && Number.isInteger(value) && value >= min && value <= max;
}

/** 读出窗口位置；任一项不合格时整个位置不用。 */
function parseWindow(raw: unknown): WindowPlacement | null {
  const record = asRecord(raw);
  if (record === undefined) return null;
  const { x, y, width, height, maximized } = record;
  if (!isIntegerIn(x, -MAX_COORDINATE, MAX_COORDINATE) || !isIntegerIn(y, -MAX_COORDINATE, MAX_COORDINATE)) return null;
  if (!isIntegerIn(width, 1, MAX_SIZE) || !isIntegerIn(height, 1, MAX_SIZE) || typeof maximized !== 'boolean') return null;
  return { x, y, width, height, maximized };
}

/** 布局各字段的校验函数。 */
const LAYOUT_VALIDATORS: { readonly [K in keyof ShellLayout]: (value: unknown) => value is ShellLayout[K] } = {
  bottomPanelHeight: (value): value is number | null => value === null || isIntegerIn(value, 1, MAX_PANEL_HEIGHT)
};

/**
 * 解析保存的界面状态；内容缺失或损坏时返回默认值，单项不合格时只替换那一项。
 * @param raw 读到的内容。
 * @returns 界面状态。
 */
export function parseShellState(raw: unknown): ShellState {
  const record = asRecord(raw);
  if (record === undefined) return DEFAULT_SHELL_STATE;
  const layoutRecord = asRecord(record.layout) ?? {};
  const layout: Record<string, unknown> = { ...DEFAULT_SHELL_STATE.layout };
  for (const key of Object.keys(LAYOUT_VALIDATORS) as Array<keyof ShellLayout>) {
    if (LAYOUT_VALIDATORS[key](layoutRecord[key])) layout[key] = layoutRecord[key];
  }
  return { window: parseWindow(record.window), layout: layout as unknown as ShellLayout };
}

/**
 * 校验渲染进程发来的布局修改：只能含布局字段，值都合格，且至少有一项。
 * @param raw 收到的内容。
 * @returns 合格的修改；不合格时为 null。
 */
export function parseLayoutPatch(raw: unknown): Partial<ShellLayout> | null {
  const record = asRecord(raw);
  if (record === undefined) return null;
  const keys = Object.keys(record);
  if (keys.length === 0) return null;
  const patch: Record<string, unknown> = {};
  for (const key of keys) {
    if (!Object.hasOwn(LAYOUT_VALIDATORS, key)) return null;
    const validate = LAYOUT_VALIDATORS[key as keyof ShellLayout] as (value: unknown) => boolean;
    if (!validate(record[key])) return null;
    patch[key] = record[key];
  }
  return patch as Partial<ShellLayout>;
}
