// ------------------------------------------------------------------------
// 名称：target.ts
// 说明：目标（一种机型的构建定义）的数据类型。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：字段含义见 docs/02-目标规范.md；路径值相对目标目录，
//       或以 toolchain: 开头表示相对工具链根目录。
// ------------------------------------------------------------------------

/** 目标专用选项的取值。 */
export type TargetOptionValue = boolean | string | number;

/** 目标专用选项的定义。 */
export interface TargetOptionSpec {
  /** 取值类型。 */
  readonly type: 'boolean' | 'string' | 'number';
  /** 缺省值。 */
  readonly default: TargetOptionValue;
  /** 布尔选项为 true 时额外链接的文件（路径值）。 */
  readonly links: readonly string[];
}

/** 多页（换页）程序的定义。 */
export interface BankedSpec {
  /** 目标专用选项名，值为库页个数。 */
  readonly option: string;
  /** 链接配置模板（路径值），含 %BANK_MEMORY% 与 %BANK_SEGMENTS% 两行。 */
  readonly linkConfig: string;
  /** 模板里已写出前面的库页，从这一页起按个数生成。 */
  readonly generatedFrom: number;
  /** 库页个数的上限。 */
  readonly maxBanks: number;
  /** 一个库页的内存区行，{n} 换成库页号。 */
  readonly memory: string;
  /** 一个库页的段行，{n} 换成库页号。 */
  readonly segment: string;
}

/** 运行方案：rom 直接加载产物；firmware-disk 加载固件并挂载磁盘镜像。 */
export type RunMode = 'rom' | 'firmware-disk';

/** 一个目标。 */
export interface Target {
  /** 标识，与目标目录同名。 */
  readonly name: string;
  /** 显示名。 */
  readonly title: string;
  /** 目标目录的绝对路径。 */
  readonly dir: string;
  /** CPU 型号。 */
  readonly cpu: string;
  /** 传给 cc65 工具的目标名。 */
  readonly cc65Target: string;
  /** 链接配置（路径值）。 */
  readonly linkConfig: string;
  /** 启动代码（路径值）。 */
  readonly startup: readonly string[];
  /** 链接时强制引入的符号。 */
  readonly forceImports: readonly string[];
  /** 链接的库（路径值）。 */
  readonly libraries: readonly string[];
  /** 头文件目录（路径值）。 */
  readonly include: readonly string[];
  /** 汇编引入目录（路径值）。 */
  readonly asminclude: readonly string[];
  /** 预定义宏。 */
  readonly defines: Readonly<Record<string, string | number>>;
  /** 打包器名。 */
  readonly packager: string;
  /** 打包器选项。 */
  readonly packagerOptions: Readonly<Record<string, unknown>>;
  /** 产物扩展名。 */
  readonly extension: string;
  /** 程序名规则；null 表示不限制。 */
  readonly nameRule: string | null;
  /** 磁盘镜像能力；不支持时为 null。 */
  readonly disk: { readonly packager: string } | null;
  /** 运行方案；没有时为 null。 */
  readonly run: { readonly emulator: string; readonly mode: RunMode } | null;
  /** 目标专用选项。 */
  readonly options: Readonly<Record<string, TargetOptionSpec>>;
  /** 多页程序的定义；不支持时为 null。 */
  readonly banked: BankedSpec | null;
  /** 机器用的文字编码；不为 null 时构建前把 UTF-8 源文件转成这个编码。 */
  readonly sourceCharset: 'gb2312' | null;
}
