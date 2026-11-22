// ------------------------------------------------------------------------
// 名称：index.ts
// 说明：operations 目录的出口：Copilot 反汇编工具对应的 18 个操作，按名字调用；每次调用从工程目录读
//       出工程，改动后写回。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：操作名、参数和结果的结构由 package.json 里的语言模型工具声明，测试核对两边一致。
// ------------------------------------------------------------------------
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { callgraph, detect, list, reference, routine, trace, view, xrefs } from './analysis-operations';
import { apply, symbols } from './editing-operations';
import { generate, verify } from './output-operations';
import { find, hex, layout, open, status } from './project-operations';
import { importRecord } from './record-operations';

/** 一个操作：参数为运行环境与操作参数，结果可同步给出，也可异步给出。 */
type Operation = (environment: DisasmEnvironment, params: Record<string, unknown>) => Record<string, unknown> | Promise<Record<string, unknown>>;

/** 操作名到实现 */
export const OPERATIONS: Readonly<Record<string, Operation>> = Object.freeze({ open, status, layout, hex, find, trace, list, view, xrefs, routine, callgraph, detect, reference, apply, symbols, importRecord, generate, verify });

/** 全部操作名 */
export const OPERATION_NAMES: readonly string[] = Object.freeze(Object.keys(OPERATIONS));

/**
 * 按名字运行一个操作。
 * @param environment 运行环境
 * @param name 操作名，见 OPERATION_NAMES
 * @param params 参数
 * @returns 可序列化的结果
 * @throws FcdevError 操作名不认识、参数不合法，或操作失败
 */
export async function runOperation(environment: DisasmEnvironment, name: string, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const operation = Object.hasOwn(OPERATIONS, name) ? OPERATIONS[name] : undefined;
  if (operation === undefined) throw new FcdevError(`未知的操作：${String(name)}`, { code: 'DISASM_OPERATION_UNKNOWN', hint: `可用的操作：${OPERATION_NAMES.join('、')}` });
  if (params === null || typeof params !== 'object' || Array.isArray(params)) throw new FcdevError('参数必须是对象', { code: 'DISASM_PARAM_INVALID' });
  return operation(environment, params);
}
