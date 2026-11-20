// ------------------------------------------------------------------------
// 名称：analysis-tools.ts
// 说明：“分析”一组工具：disasm_trace、disasm_list、disasm_view、disasm_xrefs、
//       disasm_routine、disasm_callgraph、disasm_detect、disasm_reference。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只有 disasm_trace 会更新工程文件（区域与自动命名的符号），需要用户确认；其余只读。
// ------------------------------------------------------------------------

import type { ToolSpec } from '../../contracts/disassembly/tool-spec';
import { projectLabel } from './project-tools';

/** “分析”一组工具。 */
export const ANALYSIS_TOOLS: readonly ToolSpec[] = [
  {
    name: 'disasm_trace',
    operation: 'trace',
    writes: true,
    paths: ['projectDir'],
    describe: (input) => `追踪 ${projectLabel(input)} 的代码`,
    confirm: (input) => `将从入口重新追踪 ${String(input.projectDir)}，更新区域并给新发现的地址自动命名。`
  },
  {
    name: 'disasm_list',
    operation: 'list',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `列出 ${projectLabel(input)} 的${String(input.category ?? '项目')}`
  },
  {
    name: 'disasm_view',
    operation: 'view',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 的反汇编`
  },
  {
    name: 'disasm_xrefs',
    operation: 'xrefs',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 里 ${String(input.symbol ?? input.address ?? '')} 的引用`
  },
  {
    name: 'disasm_routine',
    operation: 'routine',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 里例程 ${String(input.routine ?? input.address ?? '')} 的上下文`
  },
  {
    name: 'disasm_callgraph',
    operation: 'callgraph',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 的调用图`
  },
  {
    name: 'disasm_detect',
    operation: 'detect',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `在 ${projectLabel(input)} 里识别地址表、字符串与填充`
  },
  {
    name: 'disasm_reference',
    operation: 'reference',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查询 ${String(input.query ?? '')}`
  }
];
