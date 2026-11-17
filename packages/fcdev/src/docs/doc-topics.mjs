// ------------------------------------------------------------------------
// 名称：doc-topics.mjs
// 说明：使用文档的主题登记表：每份文档的主题名、相对 docs 目录的文件、标题和一句话说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：新增、改名或删除使用文档时同步改这里与仓库根目录 docs/01-软件架构设计.md 第 9 节；主题名只用小写字母、数字与连字符，便于在任何终端里输入。
// ------------------------------------------------------------------------

/**
 * @typedef {object} DocTopic
 * @property {string} topic 主题名
 * @property {string} file 文件路径，用 / 分隔；默认相对 docs 目录，`inPackage` 为 true 时相对主包根目录
 * @property {boolean} [inPackage] 文件不在 docs 目录里、而在主包其他目录下（如目标目录里由头文件生成的接口参考）
 * @property {string} title 标题
 * @property {string} summary 一句话说明
 */

/** @type {DocTopic[]} */
export const DOC_TOPICS = [
  { topic: 'build', file: '01-命令行与构建.md', title: '命令行与构建', summary: '命令、构建规则、配置文件 fcdev.json、构建流程' },
  { topic: 'emulator', file: '02-模拟器与调试.md', title: '模拟器与调试', summary: '运行方案、Ember 定位、调试服务与 debug 命令' },
  { topic: 'disasm', file: '03-反汇编.md', title: '反汇编', summary: '反汇编的命令、输入、追踪规则与限制' },
  { topic: 'create', file: '04-创建包与模板.md', title: '创建包与模板', summary: 'npm create fcdev 的命令、模板与生成内容' },
  { topic: 'nes', file: 'nes/01-开发指南.md', title: 'NES 开发指南', summary: 'NES 目标的内存与映射器、fcnes 函数、字库、运行' },
  { topic: 'sb2000', file: 'sb2000/01-开发指南.md', title: 'SB2000 开发指南', summary: '程序格式、内存布局、运行库、大程序换页、常用做法、易错点与示例' },
  { topic: 'syscall', file: 'sb2000/02-系统调用手册.md', title: 'SB-DOS 系统调用手册', summary: '系统调用的参数、结果、出错码与可靠程度' },
  { topic: 'hardware', file: 'sb2000/03-硬件说明.md', title: 'SB2000 硬件说明', summary: 'SB2000 的硬件行为与寄存器' },
  { topic: 'api', file: 'targets/sb2000/api-reference.md', inPackage: true, title: 'SB2000 C 接口参考', summary: '运行库全部函数的签名、参数、返回值与系统调用，由头文件文档注释生成，查询用 npx fcdev api' },
  { topic: 'symbols', file: 'targets/sb2000/symbols-reference.md', inPackage: true, title: 'SB2000 符号参考', summary: '功能号、子功能号、系统变量地址、出错码、硬件寄存器与位、运行库入口的汇编名与 C 名，由 sb2000.inc 生成' },
];
