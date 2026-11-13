// ------------------------------------------------------------------------
// 名称：doc-topics.mjs
// 说明：开发文档的主题登记表：每份文档的主题名、相对 docs 目录的文件、标题和一句话说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：新增、改名或删除文档时同步改这里与 docs/01-软件架构设计.md 第 9 节；主题名只用小写字母、数字与连字符，便于在任何终端里输入。
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
  { topic: 'architecture', file: '01-软件架构设计.md', title: '软件架构设计', summary: '包的划分、目录、分层规则、设计决策与文档清单' },
  { topic: 'build', file: '02-命令行与构建.md', title: '命令行与构建', summary: '命令、构建规则、配置文件 fcdev.json、构建流程' },
  { topic: 'targets', file: '03-目标规范.md', title: '目标规范', summary: '目标定义、打包器、新增机型的步骤' },
  { topic: 'toolchain', file: '04-cc65工具链.md', title: 'cc65 工具链', summary: 'cc65 源码、构建与工具链包' },
  { topic: 'emulator', file: '05-模拟器与调试.md', title: '模拟器与调试', summary: '运行方案、调试服务与命令表、模拟器包' },
  { topic: 'disasm', file: '06-反汇编.md', title: '反汇编', summary: '反汇编前端的流程、输入与限制' },
  { topic: 'create', file: '07-创建包与模板.md', title: '创建包与模板', summary: 'npm create fcdev 的命令、模板与生成内容' },
  { topic: 'testing', file: '08-测试与验证.md', title: '测试与验证', summary: '测试分层、环境变量与运行方法' },
  { topic: 'release', file: '09-发布.md', title: '发布', summary: '发布顺序、前提与检查' },
  { topic: 'vscode', file: '10-VS Code扩展.md', title: 'VS Code 扩展', summary: '智能感知、语法高亮、项目信息来源、目录分层与打包' },
  { topic: 'nes', file: 'nes/01-NES目标.md', title: 'NES 目标', summary: 'NES 目标的内存与映射器、fcnes 函数、字库、运行' },
  { topic: 'sb2000', file: 'sb2000/01-开发指南.md', title: 'SB2000 开发指南', summary: '程序格式、内存布局、运行库、大程序换页、常用做法、易错点与示例' },
  { topic: 'syscall', file: 'sb2000/02-系统调用手册.md', title: 'SB-DOS 系统调用手册', summary: '系统调用的参数、结果、出错码与可靠程度' },
  { topic: 'hardware', file: 'sb2000/03-硬件说明.md', title: 'SB2000 硬件说明', summary: 'SB2000 的硬件行为与寄存器' },
  { topic: 'api', file: 'targets/sb2000/api-reference.md', inPackage: true, title: 'SB2000 C 接口参考', summary: '运行库全部函数的签名、参数、返回值与系统调用，由头文件文档注释生成，查询用 npx fcdev api' },
  { topic: 'todo', file: '待办备忘.md', title: '待办备忘', summary: '已确定以后再做的事项' },
  { topic: 'bugs', file: '缺陷记录.md', title: '缺陷记录', summary: '发现的缺陷及修复情况' },
];
