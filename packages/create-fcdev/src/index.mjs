// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：create-fcdev 的主流程：解析参数，选择目标与语言，生成项目，安装依赖并生成 IDE 配置，打印下一步。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：runCreate 不直接读写进程状态，终端目录、输入输出流与外部命令都由调用者传入，便于测试。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import path from 'node:path';
import { parseCreateArguments } from './args.mjs';
import { CreateError } from './errors.mjs';
import { createPrompter } from './prompts.mjs';
import { readOwnVersion, scaffoldProject } from './scaffold.mjs';
import { LANGUAGES, findTemplate, listTemplates } from './templates.mjs';

export { CreateError } from './errors.mjs';

/**
 * @typedef {object} CreateContext
 * @property {string} cwd 终端当前目录
 * @property {NodeJS.ReadableStream} stdin 标准输入
 * @property {{write: (text: string) => unknown}} stdout 标准输出
 * @property {{write: (text: string) => unknown}} stderr 标准错误
 * @property {boolean} interactive 是否可以交互询问
 * @property {(command: string, args: string[], cwd: string) => Promise<number>} runCommand 运行外部命令，返回退出码
 */

/** 帮助文字 */
const HELP_TEXT = `create-fcdev：创建 6502 兼容机程序的项目

用法：npm create fcdev@latest [目录] [-- 选项]

目录          默认当前目录（必须为空）；不存在时会创建
选项：
  --target <名>      目标，如 nes、sb2000；缺省时询问
  --template <语言>  c 或 asm；缺省时询问
  --no-install       不运行 npm install 与 npm run sync
  -h, --help         显示帮助
  -v, --version      显示版本
`;

/** 语言的显示名 */
const LANGUAGE_LABELS = { c: 'C 语言（cc65）', asm: '汇编（ca65）' };

/**
 * 运行外部命令并继承终端输出。
 * @param {string} command 命令名
 * @param {string[]} args 参数（固定值，不含用户输入）
 * @param {string} cwd 工作目录
 * @returns {Promise<number>} 退出码
 */
export function runCommandInTerminal(command, args, cwd) {
  return new Promise((resolve, reject) => {
    // npm 在 Windows 上是 .cmd 文件，必须经 shell 启动；参数都是固定值
    const child = spawn(command, args, { cwd, stdio: 'inherit', shell: true });
    child.once('error', reject);
    child.once('close', (code) => resolve(code ?? 1));
  });
}

/**
 * 确定目标与语言：命令行给了就用，没给就询问。
 * @param {import('./args.mjs').CreateArguments} args 命令行参数
 * @param {CreateContext} context 运行环境
 * @returns {Promise<{target: string, language: string}>} 目标与语言
 * @throws {CreateError} 缺少选项又不能交互
 */
async function chooseTemplate(args, context) {
  const templates = listTemplates();
  let { target, language } = args;
  if (target === undefined || language === undefined) {
    if (!context.interactive) {
      throw new CreateError('没有指定目标或语言，且当前不能交互', { code: 'ARGUMENT_MISSING', hint: '加上 --target <名> 和 --template <c|asm>' });
    }
    const prompter = createPrompter({ input: context.stdin, output: context.stdout });
    try {
      if (target === undefined) {
        const seen = new Map(templates.map((item) => [item.target, item.title]));
        target = await prompter.askChoice({
          question: '选择目标：',
          choices: [...seen].map(([value, title]) => ({ value, label: `${value}  ${title}` })),
        });
      }
      if (language === undefined) {
        language = await prompter.askChoice({
          question: '选择语言：',
          choices: LANGUAGES.map((value) => ({ value, label: `${value}  ${LANGUAGE_LABELS[value]}` })),
        });
      }
    } finally {
      prompter.close();
    }
  }
  return { target, language };
}

/**
 * 运行创建流程。
 * @param {string[]} argv 去掉 node 与脚本路径后的参数
 * @param {CreateContext} context 运行环境
 * @returns {Promise<number>} 进程退出码
 */
export async function runCreate(argv, context) {
  try {
    const args = parseCreateArguments(argv);
    if (args.version) {
      context.stdout.write(`${readOwnVersion()}\n`);
      return 0;
    }
    if (args.help) {
      context.stdout.write(HELP_TEXT);
      return 0;
    }

    const { target, language } = await chooseTemplate(args, context);
    const template = findTemplate(target, language);
    const directory = path.resolve(context.cwd, args.directory ?? '.');
    const { name, files } = scaffoldProject({ directory, template });
    context.stdout.write(`已在 ${directory} 创建项目 ${name}（模板 ${template.name}），共 ${files.length} 个文件\n`);

    if (args.install) {
      context.stdout.write('安装依赖……\n');
      if ((await context.runCommand('npm', ['install'], directory)) !== 0) {
        throw new CreateError('npm install 失败', { code: 'INSTALL_FAILED', hint: `项目文件已生成；解决网络或权限问题后，在 ${directory} 里重新运行 npm install` });
      }
      if ((await context.runCommand('npm', ['run', 'sync'], directory)) !== 0) {
        throw new CreateError('生成 IDE 配置失败', { code: 'SYNC_FAILED', hint: `在 ${directory} 里运行 npm run sync 查看原因` });
      }
    }

    const relative = path.relative(context.cwd, directory);
    context.stdout.write(`\n下一步：\n${relative === '' ? '' : `  cd ${relative}\n`}${args.install ? '' : '  npm install      安装依赖\n  npm run sync     生成 IDE 配置\n'}`);
    for (const note of template.notes) context.stdout.write(`${note}\n`);
    return 0;
  } catch (error) {
    if (!(error instanceof CreateError)) throw error;
    context.stderr.write(`错误：${error.message}\n`);
    if (error.hint) context.stderr.write(`${error.hint}\n`);
    return 1;
  }
}
