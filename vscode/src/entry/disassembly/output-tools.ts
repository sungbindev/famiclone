// ------------------------------------------------------------------------
// 名称：output-tools.ts
// 说明：“生成与校验”一组工具：disasm_generate、disasm_verify、disasm_build。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：三个都会写文件或运行外部程序（汇编器、链接器、构建），需要用户确认；
//       disasm_build 的构建目录是按生成的源码整理出的 FC Dev 项目，必须在工作区里。
// ------------------------------------------------------------------------

import type { ToolSpec } from '../../contracts/disassembly/tool-spec';
import { projectLabel } from './project-tools';

/** “生成与校验”一组工具。 */
export const OUTPUT_TOOLS: readonly ToolSpec[] = [
  {
    name: 'disasm_generate',
    operation: 'generate',
    writes: true,
    paths: ['projectDir'],
    describe: (input) => `生成 ${projectLabel(input)} 的源码`,
    confirm: (input) => `将在 ${String(input.projectDir)} 下生成（覆盖）src、include 与 link.cfg。`
  },
  {
    name: 'disasm_verify',
    operation: 'verify',
    writes: true,
    paths: ['projectDir'],
    describe: (input) => `回读校验 ${projectLabel(input)}`,
    confirm: (input) => `将重新生成 ${String(input.projectDir)} 的源码，用 ca65 与 ld65 汇编链接（产物在 verify 目录），并把结果记进工程文件。`
  },
  {
    name: 'disasm_build',
    operation: 'build',
    writes: true,
    paths: ['projectDir', 'buildPath'],
    describe: (input) => `用 FC Dev 构建 ${String(input.buildPath ?? '')} 并与 ${projectLabel(input)} 的原映像比较`,
    confirm: (input) => `将在 ${String(input.buildPath)} 构建，并把产物与 ${String(input.projectDir)} 的原映像逐字节比较。`
  }
];
