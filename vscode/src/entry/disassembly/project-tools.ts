// ------------------------------------------------------------------------
// 名称：project-tools.ts
// 说明：“工程与映像”一组工具：disasm_open、disasm_status、disasm_layout、
//       disasm_hex、disasm_find。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：新建工程与替换布局会写工程目录，需要用户确认；其余只读。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { ToolInput, ToolSpec } from '../../contracts/disassembly/tool-spec';

/** 工程目录的名字，用于显示。 */
export function projectLabel(input: ToolInput): string {
  return typeof input.projectDir === 'string' ? path.basename(input.projectDir) || String(input.projectDir) : '工程';
}

/** “工程与映像”一组工具。 */
export const PROJECT_TOOLS: readonly ToolSpec[] = [
  {
    name: 'disasm_open',
    operation: 'open',
    writes: (input) => input.image !== undefined,
    paths: ['projectDir', 'image', 'symbolSets'],
    outsideAllowed: ['image', 'symbolSets'],
    describe: (input) => (input.image === undefined ? `打开反汇编工程 ${projectLabel(input)}` : `新建反汇编工程 ${projectLabel(input)}`),
    confirm: (input) => `将在 ${String(input.projectDir)} 创建反汇编工程文件 disasm.json，映像为 ${String(input.image)}。${input.overwrite === true ? '已有的工程会被覆盖。' : ''}`
  },
  {
    name: 'disasm_status',
    operation: 'status',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 的进度`
  },
  {
    name: 'disasm_layout',
    operation: 'layout',
    writes: (input) => input.segments !== undefined,
    paths: ['projectDir'],
    describe: (input) => (input.segments === undefined ? `查看 ${projectLabel(input)} 的布局` : `替换 ${projectLabel(input)} 的布局`),
    confirm: (input) => `将替换 ${String(input.projectDir)} 的布局：手工标注的区域会被丢弃，不再有效的入口、符号与注释也会被丢弃。`
  },
  {
    name: 'disasm_hex',
    operation: 'hex',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `查看 ${projectLabel(input)} 的十六进制数据`
  },
  {
    name: 'disasm_find',
    operation: 'find',
    writes: false,
    paths: ['projectDir'],
    describe: (input) => `在 ${projectLabel(input)} 里搜索`
  }
];
