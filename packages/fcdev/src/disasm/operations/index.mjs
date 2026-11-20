// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：operations 目录的出口：Copilot 反汇编工具对应的 18 个操作，按名字调用；每次调用从工程目录读出工程，改动后写回。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：API_VERSION 在操作的名字、参数或结果的含义发生不兼容变化时加 1；VS Code 扩展据此检查工作区里 fcdev 的版本。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { callgraph, detect, list, reference, routine, trace, view, xrefs } from './analysis-operations.mjs';
import { apply, symbols } from './editing-operations.mjs';
import { generate, verify } from './output-operations.mjs';
import { find, hex, layout, open, status } from './project-operations.mjs';
import { importRecord } from './record-operations.mjs';

/** 操作接口的版本 */
export const API_VERSION = 1;

/** 操作名到实现 */
export const OPERATIONS = Object.freeze({ open, status, layout, hex, find, trace, list, view, xrefs, routine, callgraph, detect, reference, apply, symbols, importRecord, generate, verify });

/** 全部操作名 */
export const OPERATION_NAMES = Object.freeze(Object.keys(OPERATIONS));

/**
 * 按名字运行一个操作。
 * @param {string} name 操作名，见 OPERATION_NAMES
 * @param {Record<string, unknown>} params 参数
 * @param {{env?: NodeJS.ProcessEnv}} [options] 运行环境
 * @returns {Promise<object>} 可序列化的结果
 * @throws {FcdevError} 操作名不认识、参数不合法，或操作失败
 */
export async function runOperation(name, params, options = {}) {
  if (!Object.hasOwn(OPERATIONS, name)) throw new FcdevError(`未知的操作：${String(name)}`, { code: 'DISASM_OPERATION_UNKNOWN', hint: `可用的操作：${OPERATION_NAMES.join('、')}` });
  if (params === null || typeof params !== 'object' || Array.isArray(params)) throw new FcdevError('参数必须是对象', { code: 'DISASM_PARAM_INVALID' });
  return OPERATIONS[name](params, options);
}
