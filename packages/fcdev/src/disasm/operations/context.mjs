// ------------------------------------------------------------------------
// 名称：context.mjs
// 说明：操作用的工程上下文：从工程目录读出工程、映像与符号集常量，操作改完后写回；每次调用都重新读，不在内存里留状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：映像与工程记录的摘要不一致时读取就报错（工程已过期）；追踪结果（交叉引用等需要）按需用 analyze 重新算，不存盘。
// ------------------------------------------------------------------------
import { traceProject } from '../analysis/index.mjs';
import { parseSymbolSet } from '../profile/index.mjs';
import { createSymbolIndex } from '../query/index.mjs';
import { readProject, readProjectImage, readProjectSymbolSets, writeProject } from '../workspace/index.mjs';

/**
 * @typedef {object} Context
 * @property {string} dir 工程目录
 * @property {import('../model/project.mjs').Project} project 工程
 * @property {Buffer} bytes 映像字节
 * @property {Map<string, string>} symbolSetTexts 符号集文件名到文本
 * @property {Array<{name: string, value: number}>} constants 符号集里的常量
 * @property {import('../query/symbol-index.mjs').SymbolIndex} index 符号索引
 */

/**
 * 读取工程上下文。
 * @param {string} dir 工程目录
 * @returns {Context} 上下文
 * @throws {FcdevError} 没有工程文件、映像读不到或已变化、符号集读不到
 */
export function loadContext(dir) {
  const project = readProject(dir);
  const bytes = readProjectImage(dir, project);
  const symbolSetTexts = readProjectSymbolSets(dir, project);
  const constants = [...symbolSetTexts.values()].flatMap((text) => parseSymbolSet(text));
  return { dir, project, bytes, symbolSetTexts, constants, index: createSymbolIndex(project, constants) };
}

/**
 * 用新的工程替换上下文里的工程，并写回磁盘。
 * @param {Context} context 上下文
 * @param {import('../model/project.mjs').Project} project 新工程
 * @returns {Context} 新上下文（符号集与映像不变，索引按新工程重建）
 */
export function saveContext(context, project) {
  writeProject(context.dir, project);
  return { ...context, project, index: createSymbolIndex(project, context.constants) };
}

/**
 * 对当前工程做一次追踪，得到分析结果（不修改工程）。
 * @param {Context} context 上下文
 * @returns {import('../analysis/tracer.mjs').Analysis} 分析结果
 */
export function analyze(context) {
  return traceProject(context.project, context.bytes).analysis;
}
