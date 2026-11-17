/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.ts
 * 说明：界面状态的类型、默认值与校验：窗口位置，底部面板高度，上次选择的机型，开着“自动收缩”的底部面板，以及上次显示更新记录时的应用版本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 9 节；读到的内容逐项校验，不合格的项用默认值，不影响其他项；
 *       上次显示更新记录时的应用版本只由主进程修改，渲染进程发来的布局修改改不了它；
 *       窗口大小与是否最大化不保存（每次启动都是默认尺寸）；
 *       底部面板是否展开与当前页签不保存（每次启动都是关闭）；
 *       渲染进程发来的布局修改同样经 parseLayoutPatch 校验，含未知字段或不合格的值时整条丢弃。
 * ------------------------------------------------------------------------
 */

/** 窗口的位置（左上角坐标），大小与是否最大化不保存。 */
export interface WindowPlacement {
  readonly x: number;
  readonly y: number;
}

/** 外壳需要保存的版面与选择：底部面板高度、上次选的机型、开着“自动收缩”的面板；底部面板是否展开与当前页签每次启动都是关闭，不保存。 */
export interface ShellLayout {
  /** 底部面板的高度（像素）；null 表示用默认高度。 */
  readonly bottomPanelHeight: number | null;
  /** “文件→选择机型”里上次选的机型标识；null 表示没选过，用经典 FC。 */
  readonly selectedMachine: string | null;
  /** 开着“自动收缩”的底部面板标识（如 runtime.panel），没有重复；不在清单里的面板为关闭。 */
  readonly autoCollapsePanels: readonly string[];
}

/** 需要保存的全部界面状态。 */
export interface ShellState {
  /** 窗口的位置；null 表示没有保存过，用默认位置。 */
  readonly window: WindowPlacement | null;
  readonly layout: ShellLayout;
  /** 上次显示更新记录时的应用版本；null 表示没有记录（全新安装，或还没有显示过）。 */
  readonly releaseNotesVersion: string | null;
}

/** 没有保存过时：窗口用默认位置，底部面板用默认高度，没有显示过更新记录。 */
export const DEFAULT_SHELL_STATE: ShellState = {
  window: null,
  layout: { bottomPanelHeight: null, selectedMachine: null, autoCollapsePanels: [] },
  releaseNotesVersion: null
};

/** 应用版本的写法：数字开头，只含字母、数字与 . + -，最长 64。 */
const VERSION_PATTERN = /^[0-9][0-9A-Za-z.+-]{0,63}$/;

/** 窗口坐标的合理范围（多显示器时可以为负）。 */
const MAX_COORDINATE = 100_000;
/** 底部面板高度的合理范围。 */
const MAX_PANEL_HEIGHT = 10_000;
/** 机型标识的写法（小写字母开头，字母与数字，最长 32）；是否真的存在由机型清单决定。 */
const MACHINE_ID_PATTERN = /^[a-z][a-z0-9]{0,31}$/;
/** 底部面板标识的写法（模块名加点加面板名，最长 64）；是否真的存在由登记表决定。 */
const PANEL_ID_PATTERN = /^[a-z][a-z0-9-]*\.[A-Za-z][A-Za-z0-9-]*$/;
/** 自动收缩清单的最大长度。 */
const MAX_AUTO_COLLAPSE_PANELS = 16;

/** 合格的面板标识清单：长度有限、每项写法合格、没有重复。 */
function isPanelIdList(value: unknown): value is readonly string[] {
  return (
    Array.isArray(value) &&
    value.length <= MAX_AUTO_COLLAPSE_PANELS &&
    value.every((item) => typeof item === 'string' && item.length <= 64 && PANEL_ID_PATTERN.test(item)) &&
    new Set(value).size === value.length
  );
}

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
  const { x, y } = record;
  if (!isIntegerIn(x, -MAX_COORDINATE, MAX_COORDINATE) || !isIntegerIn(y, -MAX_COORDINATE, MAX_COORDINATE)) return null;
  return { x, y };
}

/** 布局各字段的校验函数。 */
const LAYOUT_VALIDATORS: { readonly [K in keyof ShellLayout]: (value: unknown) => value is ShellLayout[K] } = {
  bottomPanelHeight: (value): value is number | null => value === null || isIntegerIn(value, 1, MAX_PANEL_HEIGHT),
  selectedMachine: (value): value is string | null => value === null || (typeof value === 'string' && MACHINE_ID_PATTERN.test(value)),
  autoCollapsePanels: isPanelIdList
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
  return {
    window: parseWindow(record.window),
    layout: layout as unknown as ShellLayout,
    releaseNotesVersion: typeof record.releaseNotesVersion === 'string' && VERSION_PATTERN.test(record.releaseNotesVersion) ? record.releaseNotesVersion : null
  };
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
