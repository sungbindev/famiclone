// ------------------------------------------------------------------------
// 名称：editing-tools.ts
// 说明：“编辑”一组工具：disasm_apply、disasm_symbols。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：编辑会修改工程文件，需要用户确认；disasm_symbols 只有导入符号集时才写，列出与搜索只读。
// ------------------------------------------------------------------------

import type { ToolSpec } from '../../contracts/disassembly/tool-spec';
import { projectLabel } from './project-tools';

/** “编辑”一组工具。 */
export const EDITING_TOOLS: readonly ToolSpec[] = [
  {
    name: 'disasm_apply',
    operation: 'apply',
    writes: true,
    paths: ['projectDir'],
    describe: (input) => `编辑 ${projectLabel(input)}（${Array.isArray(input.edits) ? input.edits.length : 0} 项）`,
    confirm: (input) => `将对 ${String(input.projectDir)} 做 ${Array.isArray(input.edits) ? input.edits.length : 0} 项编辑（符号、区域、注释、操作数提示、模块、入口）；任何一项出错整批都不生效。`
  },
  {
    name: 'disasm_symbols',
    operation: 'symbols',
    writes: (input) => input.action === 'import',
    paths: ['projectDir', 'file'],
    outsideAllowed: ['file'],
    describe: (input) => (input.action === 'import' ? `导入符号集 ${String(input.file)}` : `查看 ${projectLabel(input)} 的符号`),
    confirm: (input) => `将把符号集 ${String(input.file)} 登记到 ${String(input.projectDir)} 的工程里。`
  }
];
