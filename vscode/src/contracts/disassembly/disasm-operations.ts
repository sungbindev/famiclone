// ------------------------------------------------------------------------
// 名称：disasm-operations.ts
// 说明：反汇编操作的能力接口：按操作名与参数运行 Copilot 反汇编工具对应的操作，以及一次完成的反汇
//       编（读映像、追踪、生成、回读校验）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：操作的实现在扩展自己的逻辑层，不再加载工作区里安装的外部包；结果是可序列化的普通对象。
// ------------------------------------------------------------------------

import type { Status } from '../../shared/disassembly/verify/status';

/** 一次完成的反汇编的输入。 */
export interface DisassembleInput {
  /** 映像文件的绝对路径。 */
  readonly inputFile: string;
  /** 工程输出目录。 */
  readonly outDir: string;
  /** 映像格式名；省略时按文件头判断。 */
  readonly format?: string;
  /** 平铺格式的加载地址。 */
  readonly base?: number;
  /** 手工指定的入口地址。 */
  readonly entries?: number[];
  /** CDL 文件。 */
  readonly cdlFile?: string;
  /** 指令轨迹文件。 */
  readonly traceFile?: string;
  /** 符号文件。 */
  readonly symbolsFile?: string;
  /** 符号集（.inc）文件。 */
  readonly symbolSetFiles?: string[];
}

/** 一次完成的反汇编的报告。 */
export interface DisassemblyReport {
  /** 工程目录。 */
  projectDir: string;
  /** 工程文件。 */
  projectFile: string;
  /** 输入的来源说明。 */
  description: string;
  /** 映像文件的字节数。 */
  size: number;
  /** 映像里落在 CPU 地址空间（布局的段）里的字节数。 */
  mappedBytes: number;
  /** 本次追踪标出的指令条数。 */
  instructions: number;
  /** 区域类型为 code 的字节数。 */
  codeBytes: number;
  /** 追踪与生成中的警告。 */
  warnings: string[];
  /** 回读与原映像逐字节一致。 */
  verified: boolean;
  /** 进度报告。 */
  status: Status;
}

/** 运行反汇编操作。 */
export interface DisasmOperations {
  /** 全部操作名。 */
  readonly operationNames: readonly string[];

  /**
   * 运行一个操作。
   * @param operation 操作名，如 trace、view。
   * @param params 操作的参数，路径都已经是绝对路径。
   * @returns 操作的结果，是普通的可序列化对象。
   * @throws FcdevError 操作名不认识、参数不合法，或操作本身失败（说明与建议已写进错误信息）。
   */
  run(operation: string, params: Record<string, unknown>): Promise<Record<string, unknown>>;

  /**
   * 一次完成的反汇编：读映像、登记入口、追踪、自动命名、生成源码工程、回读校验并保存工程。
   * @param input 输入。
   * @returns 报告。
   * @throws FcdevError 输入不合法、没有入口、汇编链接失败，或回读不一致（工程目录仍然保留）。
   */
  disassemble(input: DisassembleInput): Promise<DisassemblyReport>;
}
