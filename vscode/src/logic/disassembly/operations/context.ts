// ------------------------------------------------------------------------
// 名称：context.ts
// 说明：操作用的工程上下文：从工程目录读出工程、映像与符号集常量，操作改完后写回；每次调用都重新
//       读，不在内存里留状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：映像与工程记录的摘要不一致时读取就报错（工程已过期）；追踪结果（交叉引用等需要）按需用
//       analyze 重新算，不存盘。
// ------------------------------------------------------------------------
import type { FileSystem } from '../../../contracts/files/file-system';
import { traceProject } from '../../../shared/disassembly/analysis/index';
import { parseSymbolSet } from '../../../shared/disassembly/profile/index';
import { createSymbolIndex } from '../../../shared/disassembly/query/index';
import { readProject, readProjectImage, readProjectSymbolSets, writeProject } from '../workspace/index';
import type { Project } from '../../../shared/disassembly/model/project';
import type { SymbolIndex } from '../../../shared/disassembly/query/symbol-index';
import type { Analysis } from '../../../shared/disassembly/analysis/tracer';

export interface Context {
  /** 工程目录 */
  dir: string;
  /** 工程 */
  project: Project;
  /** 映像字节 */
  bytes: Buffer;
  /** 符号集文件名到文本 */
  symbolSetTexts: Map<string, string>;
  /** 符号集里的常量 */
  constants: Array<{name: string, value: number}>;
  /** 符号索引 */
  index: SymbolIndex;
}

/**
 * 读取工程上下文。
 * @param files 文件系统
 * @param dir 工程目录
 * @returns 上下文
 * @throws FcdevError 没有工程文件、映像读不到或已变化、符号集读不到
 */
export function loadContext(files: FileSystem, dir: string): Context {
  const project = readProject(files, dir);
  const bytes = readProjectImage(files, dir, project);
  const symbolSetTexts = readProjectSymbolSets(files, dir, project);
  const constants = [...symbolSetTexts.values()].flatMap((text) => parseSymbolSet(text));
  return { dir, project, bytes, symbolSetTexts, constants, index: createSymbolIndex(project, constants) };
}

/**
 * 用新的工程替换上下文里的工程，并写回磁盘。
 * @param files 文件系统
 * @param context 上下文
 * @param project 新工程
 * @returns 新上下文（符号集与映像不变，索引按新工程重建）
 */
export function saveContext(files: FileSystem, context: Context, project: Project): Context {
  writeProject(files, context.dir, project);
  return { ...context, project, index: createSymbolIndex(project, context.constants) };
}

/**
 * 对当前工程做一次追踪，得到分析结果（不修改工程）。
 * @param context 上下文
 * @returns 分析结果
 */
export function analyze(context: Context): Analysis {
  return traceProject(context.project, context.bytes).analysis;
}
