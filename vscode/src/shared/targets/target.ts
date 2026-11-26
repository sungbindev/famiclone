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
  /** 取值类型；address 是 16 位地址，写数字或 "$6000"、"0x6000" 字符串，解析后是数字。 */
  readonly type: 'boolean' | 'string' | 'number' | 'address';
  /** 缺省值。 */
  readonly default: TargetOptionValue;
  /** 布尔选项为 true 时额外链接的文件（路径值）。 */
  readonly links: readonly string[];
  /** 数字与地址选项：链接时用 ld65 的 -D 定义的符号名，链接配置里可以引用它；没有时为 null。 */
  readonly linkSymbol: string | null;
  /** 地址选项的最小值；没有时为 null。 */
  readonly min: number | null;
  /** 地址选项的最大值；没有时为 null。 */
  readonly max: number | null;
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

/**
 * 运行方案：rom 直接加载产物；firmware-disk 加载固件、挂载磁盘镜像并按整机机型开机；
 * firmware 把产物当整机固件，机型取自目标专用选项 machine，可挂载 diskImage。
 */
export type RunMode = 'rom' | 'firmware-disk' | 'firmware';

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
  /** 产物扩展名，含点；可以为空（如 DOS 3.3 的文件没有扩展名）。 */
  readonly extension: string;
  /** 程序名规则；null 表示不限制。 */
  readonly nameRule: string | null;
  /**
   * 磁盘镜像能力；不支持时为 null。baseImage 为 true 时，镜像建在 fcdev.json 的
   * diskImage（系统盘）之上。
   */
  readonly disk: { readonly packager: string; readonly baseImage: boolean } | null;
  /** 运行方案；没有时为 null。machine 是 firmware-disk 模式的整机机型，其他模式为 null。 */
  readonly run: { readonly emulator: string; readonly mode: RunMode; readonly machine: string | null } | null;
  /** 目标专用选项。 */
  readonly options: Readonly<Record<string, TargetOptionSpec>>;
  /** 多页程序的定义；不支持时为 null。 */
  readonly banked: BankedSpec | null;
  /** 机器用的文字编码；不为 null 时构建前把 UTF-8 源文件转成这个编码。 */
  readonly sourceCharset: 'gb2312' | null;
}
