// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：命令行入口：解析参数、分发命令、把错误翻译成用户能看懂的提示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：runCli 不直接读写进程状态，终端目录、环境变量与输出流都由调用者传入，便于测试。
// ------------------------------------------------------------------------
import { FcdevError, readJsonFile } from '../shared/index.mjs';
import { fileURLToPath } from 'node:url';
import { parseArguments } from './args-parser.mjs';
import { runApiCommand } from './commands/api-command.mjs';
import { runBuildCommand } from './commands/build-command.mjs';
import { runDebugCommand } from './commands/debug-command.mjs';
import { runDiskCommand } from './commands/disk-command.mjs';
import { runDisasmCommand } from './commands/disasm-command.mjs';
import { runDocsCommand } from './commands/docs-command.mjs';
import { runInfoCommand } from './commands/info-command.mjs';
import { runRunCommand } from './commands/run-command.mjs';
import { runSyncCommand } from './commands/sync-command.mjs';

/**
 * @typedef {object} CliContext
 * @property {string} cwd 终端当前目录
 * @property {NodeJS.ProcessEnv} env 环境变量
 * @property {{write: (text: string) => unknown}} stdout 标准输出
 * @property {{write: (text: string) => unknown}} stderr 标准错误
 */

/** 命令表：命令名到处理函数 */
const COMMANDS = {
  api: runApiCommand,
  build: runBuildCommand,
  debug: runDebugCommand,
  disasm: runDisasmCommand,
  disk: runDiskCommand,
  docs: runDocsCommand,
  info: runInfoCommand,
  run: runRunCommand,
  sync: runSyncCommand,
};

/** 帮助文字 */
const HELP_TEXT = `fcdev：6502 兼容机的开发工具

用法：fcdev <命令> [选项]

命令：
  build [目录或文件]   构建：无参数为当前目录；目录整体构成一个程序；文件单独构成一个程序
  run [目录或文件]     构建后在 Ember 里运行（--debug 启动调试模式）
  disk [目录或文件]    构建后把程序放进磁盘镜像（只有支持磁盘的目标可用）
  disasm <文件>       反汇编 NES 程序或平铺二进制为 ca65 源码，并回读校验
  debug <参数>        交给 Ember 调试 SDK 的终端客户端，如 debug call runtime.status、debug screenshot out.png
  sync                 生成 IDE 的 C/C++ 配置
  info [--json]        显示目标、工具链与头文件目录
  api [词] [--json]    查询目标的 C 接口文档：函数名、模块名、$功能号或关键词；不带词列出全部模块
  docs [词] [--json]   查询使用文档：主题名或关键词，给出本机路径；不带词列出全部主题

选项：
  --target <名>        指定目标，覆盖 fcdev.json
  --debug              run 以调试模式启动 Ember
  --port <端口>       run 的调试端口，默认 5699；debug 命令读环境变量 FCDEV_DEBUG_PORT，默认也是 5699
  --json               info 输出机器可读格式

disasm 的选项：
  --format ines|binary 输入格式，缺省时按文件头判断
  --base <地址>       平铺二进制的加载地址（十六进制）
  --entry <地址>      追踪代码的入口，可重复；iNES 会自动使用中断向量
  --cdl <文件>        CDL（FCEUX、Mesen 格式），代码段的起点也当作入口
  --trace <文件>      Ember 的 inspector.traceStart（instruction）记录的指令轨迹，执行过的指令地址都当作入口
  --symbols <文件>    符号文件，每行“名字 = $地址”
  --out <目录>        输出目录，默认 build/disasm/<文件名>
  -h, --help           显示帮助
  -v, --version        显示版本
`;

/**
 * 运行命令行。
 * @param {string[]} argv 去掉 node 与脚本路径后的参数
 * @param {CliContext} context 运行环境
 * @returns {Promise<number>} 进程退出码
 */
export async function runCli(argv, context) {
  try {
    const { command, positionals, options } = parseArguments(argv);
    if (options.version) {
      const packageFile = fileURLToPath(new URL('../../package.json', import.meta.url));
      context.stdout.write(`${readJsonFile(packageFile).version}\n`);
      return 0;
    }
    if (options.help || command === null || command === 'help') {
      context.stdout.write(HELP_TEXT);
      return 0;
    }
    const handler = COMMANDS[command];
    if (!handler) {
      throw new FcdevError(`未知的命令：${command}`, { code: 'ARGUMENT_INVALID', hint: '用 fcdev --help 查看可用的命令' });
    }
    return await handler({ positionals, options, context });
  } catch (error) {
    if (!(error instanceof FcdevError)) throw error;
    context.stderr.write(`错误：${error.message}\n`);
    if (error.hint) context.stderr.write(`${error.hint}\n`);
    return 1;
  }
}
