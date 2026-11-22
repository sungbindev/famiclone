// ------------------------------------------------------------------------
// 名称：core-disasm-operations.ts
// 说明：反汇编操作能力的实现：把运行环境交给各个操作，按名字分发。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：每次调用都从工程目录读出工程，改动后写回，对象本身不保存状态。
// ------------------------------------------------------------------------

import type { DisasmEnvironment } from '../../contracts/disassembly/disasm-environment';
import type { DisassembleInput, DisassemblyReport, DisasmOperations } from '../../contracts/disassembly/disasm-operations';
import { OPERATION_NAMES, runOperation } from './operations/index';
import { disassemble } from './workspace/pipeline';

/** 在给定的运行环境里运行反汇编操作。 */
export class CoreDisasmOperations implements DisasmOperations {
  /** @inheritdoc */
  readonly operationNames: readonly string[] = OPERATION_NAMES;

  /**
   * 创建操作集。
   * @param environment 文件系统、目标描述读取、工具链与工具运行。
   */
  constructor(private readonly environment: DisasmEnvironment) {}

  /** @inheritdoc */
  run(operation: string, params: Record<string, unknown>): Promise<Record<string, unknown>> {
    return runOperation(this.environment, operation, params);
  }

  /** @inheritdoc */
  disassemble(input: DisassembleInput): Promise<DisassemblyReport> {
    return disassemble(this.environment, input);
  }
}
