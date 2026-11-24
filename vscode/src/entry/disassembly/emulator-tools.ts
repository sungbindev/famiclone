// ------------------------------------------------------------------------
// 名称：emulator-tools.ts
// 说明：“模拟器”一组工具：emu_connect、emu_launch、emu_status、emu_load、emu_run、emu_break、
//       emu_step、emu_inspect、emu_screen、emu_input、emu_record、emu_profile、emu_events。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：启动进程、写模拟器内存与寄存器、把截图存成文件的调用要用户确认；
//       载入的文件在工作区之外时同样在确认里说明（由注册文件统一处理）。
// ------------------------------------------------------------------------

import type { ToolInput, ToolSpec } from '../../contracts/disassembly/tool-spec';
import { DEFAULT_DEBUG_PORT } from '../../contracts/emulator/emulator-settings';

/** 端口的显示：给了就显示它，否则说明用设置里的调试端口。 */
function portText(input: ToolInput): string {
  return input.port === undefined ? `设置里的调试端口，默认 ${DEFAULT_DEBUG_PORT}` : String(input.port);
}

/** emu_run 的授权量显示：给了周期、指令、扫描线就显示它，否则是帧数（默认 60）。 */
function runAmountText(input: ToolInput): string {
  if (input.cycles !== undefined) return `${String(input.cycles)} 个 CPU 周期`;
  if (input.instructions !== undefined) return `${String(input.instructions)} 条指令`;
  if (input.scanlines !== undefined) return `${String(input.scanlines)} 行扫描线`;
  return `${String(input.frames ?? 60)} 帧`;
}

/** 写模拟器状态的 emu_inspect 项。 */
const WRITING_INSPECTIONS: readonly unknown[] = ['write', 'setState'];

/** 一个简单的模拟器工具：只读、没有路径参数。 */
function simple(name: string, operation: string, describe: (input: ToolInput) => string): ToolSpec {
  return { name, operation, target: 'emulator', writes: false, describe };
}

/** “模拟器”一组工具。 */
export const EMULATOR_TOOLS: readonly ToolSpec[] = [
  simple('emu_connect', 'connect', (input) => `连接 Ember（端口 ${portText(input)}）`),
  {
    name: 'emu_launch',
    operation: 'launch',
    target: 'emulator',
    writes: true,
    describe: (input) => `启动 Ember（端口 ${portText(input)}）`,
    confirm: (input) => `将启动设置 fcdev.emulator.path 里的 Ember（调试模式，端口 ${portText(input)}，独立的用户数据目录），不载入任何东西；关闭会话或停用扩展时会结束它。`
  },
  simple('emu_status', 'status', (input) => (input.close === true ? '关闭模拟器会话' : '查看模拟器状态')),
  {
    name: 'emu_load',
    operation: 'load',
    target: 'emulator',
    writes: false,
    paths: ['file', 'firmware', 'diskImage'],
    outsideAllowed: ['file', 'firmware', 'diskImage', 'target'],
    references: ['target'],
    describe: (input) => (input.target === undefined ? (input.file === undefined ? `载入 ${String(input.machine)} 整机` : `载入 ${String(input.file)}`) : `按目标 ${String(input.target)} 载入`)
  },
  simple('emu_run', 'run', (input) => `运行 ${runAmountText(input)}`),
  simple('emu_break', 'breakpoint', (input) => `断点：${String(input.action ?? 'list')}`),
  simple('emu_step', 'step', (input) => `单步 ${String(input.count ?? 1)} 个${String(input.unit ?? 'instruction')}`),
  {
    name: 'emu_inspect',
    operation: 'inspect',
    target: 'emulator',
    writes: (input) => WRITING_INSPECTIONS.includes(input.what),
    describe: (input) => `检查模拟器：${String(input.what)}${input.address === undefined ? '' : ` ${String(input.address)}`}`,
    confirm: (input) => (input.what === 'setState' ? '将修改模拟器里 CPU 的寄存器。' : `将向模拟器的 ${String(input.space ?? 'cpu')} 空间写内存（地址 ${String(input.address)}）。`)
  },
  {
    name: 'emu_screen',
    operation: 'screenshot',
    target: 'emulator',
    writes: (input) => input.file !== undefined,
    paths: ['file'],
    describe: () => '截取模拟器画面',
    confirm: (input) => `将把截图保存为 ${String(input.file)}。`
  },
  simple('emu_input', 'input', (input) => `模拟器输入：${String(input.kind)}`),
  simple('emu_record', 'record', (input) => `${input.action === 'stop' ? '结束' : input.action === 'read' ? '读取' : '开始'}记录 ${String(input.kind)}`),
  simple('emu_profile', 'profile', (input) => `子程序周期统计：${String(input.action ?? 'get')}`),
  simple('emu_events', 'events', (input) => `事件记录：${String(input.action ?? 'get')}`)
];
