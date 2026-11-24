// ------------------------------------------------------------------------
// 名称：disassembly-commands.ts
// 说明：反汇编相关的命令：“反汇编映像文件”，选映像文件与输出目录，一次完成读映像、追踪、生成源码
//       工程与回读校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：NES 卡带按文件头自动识别；平铺二进制缺加载地址或入口时再向用户询问；其余细节（段列表、
//       符号文件、CDL、指令轨迹）用语言模型工具 disasm_open 完成。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { DisassembleInput, DisasmOperations } from '../../contracts/disassembly/disasm-operations';
import type { Logger } from '../../platform/logging/logger';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 命令的标识，与 package.json 里声明的一致。 */
export const DISASSEMBLE_COMMAND = 'fcdev.disassemble';

/** 缺参数时最多追问的次数：加载地址与入口各一次。 */
const MAX_PROMPTS = 2;

/** 十六进制地址的写法：可带 $ 或 0x 前缀，最多四位。 */
const HEX_ADDRESS = /^(?:\$|0x)?([0-9a-f]{1,4})$/i;

/**
 * 解析十六进制地址。
 * @param text 输入的文字。
 * @returns 地址；写法不对时为 undefined。
 */
function parseAddress(text: string): number | undefined {
  const match = HEX_ADDRESS.exec(text.trim());
  return match?.[1] === undefined ? undefined : Number.parseInt(match[1], 16);
}

/**
 * 询问一个十六进制地址（或用逗号分隔的多个）。
 * @param title 输入框的标题。
 * @param prompt 输入框的说明。
 * @param multiple 是否允许多个。
 * @returns 地址列表；用户取消时为 undefined。
 */
async function askAddresses(title: string, prompt: string, multiple: boolean): Promise<number[] | undefined> {
  const text = await vscode.window.showInputBox({
    title,
    prompt,
    placeHolder: multiple ? '8000, 8010' : '8000',
    validateInput: (value) => {
      const parts = value.split(',').map((part) => part.trim());
      const valid = parts.every((part) => parseAddress(part) !== undefined) && (multiple || parts.length === 1);
      return valid ? undefined : '请写十六进制地址（如 8000 或 $8000）' + (multiple ? '，多个用逗号分隔' : '');
    }
  });
  if (text === undefined) return undefined;
  return text.split(',').map((part) => parseAddress(part) ?? 0);
}

/**
 * 选出映像文件：资源管理器里右键传来的文件，否则弹出选择框。
 * @param argument 命令的参数。
 * @returns 映像文件的路径；用户取消时为 undefined。
 */
async function pickImage(argument: unknown): Promise<string | undefined> {
  if (argument instanceof vscode.Uri && argument.scheme === 'file') return argument.fsPath;
  const picked = await vscode.window.showOpenDialog({ canSelectMany: false, title: '选择要反汇编的映像文件', openLabel: '反汇编' });
  return picked?.[0]?.fsPath;
}

/**
 * 注册反汇编相关的命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param operations 反汇编操作。
 * @param logger 日志。
 */
export function registerDisassemblyCommands(context: vscode.ExtensionContext, operations: DisasmOperations, logger: Logger): void {
  /** 运行一次，缺加载地址或入口时向用户追问后重试。 */
  const run = async (input: DisassembleInput): Promise<void> => {
    let current = input;
    for (let attempt = 0; ; attempt += 1) {
      try {
        const report = await vscode.window.withProgress(
          { location: vscode.ProgressLocation.Notification, title: '正在反汇编……' },
          () => operations.disassemble(current)
        );
        const source = path.join(report.projectDir, 'src', 'main.s');
        const open = '打开 main.s';
        const choice = await vscode.window.showInformationMessage(
          `反汇编完成：${report.description}，${report.instructions} 条指令，回读与原映像一致；工程在 ${report.projectDir}`,
          open
        );
        if (choice === open) await vscode.window.showTextDocument(vscode.Uri.file(source));
        return;
      } catch (error) {
        if (!(error instanceof FcdevError) || error.code !== 'ARGUMENT_MISSING' || attempt >= MAX_PROMPTS) throw error;
        if (error.message.includes('加载地址')) {
          const base = await askAddresses('加载地址', '平铺二进制要给出加载地址（十六进制）', false);
          if (base?.[0] === undefined) return;
          current = { ...current, base: base[0] };
        } else {
          const entries = await askAddresses('入口地址', '没有入口可供追踪代码，请给出入口地址（十六进制）', true);
          if (entries === undefined) return;
          current = { ...current, entries };
        }
      }
    }
  };

  /** 命令的执行：选映像、定输出目录，再运行。 */
  const disassemble = async (argument?: unknown): Promise<void> => {
    const inputFile = await pickImage(argument);
    if (inputFile === undefined) return;
    const defaultDir = path.join(path.dirname(inputFile), `${path.basename(inputFile, path.extname(inputFile))}-disasm`);
    const outDir = await vscode.window.showInputBox({
      title: '反汇编工程的输出目录',
      prompt: '源码工程写到这个目录',
      value: defaultDir,
      validateInput: (value) => (path.isAbsolute(value.trim()) ? undefined : '请写绝对路径')
    });
    if (outDir === undefined) return;
    try {
      await run({ inputFile, outDir: outDir.trim() });
    } catch (error) {
      if (error instanceof UserFacingError) {
        const hint = error instanceof FcdevError && error.hint !== '' ? `（${error.hint}）` : '';
        void vscode.window.showErrorMessage(`${error.message}${hint}`);
        return;
      }
      logger.error('反汇编映像文件出错', error);
      void vscode.window.showErrorMessage('反汇编映像文件时出错，详情见“输出”面板里的 FC Dev');
    }
  };

  context.subscriptions.push(vscode.commands.registerCommand(DISASSEMBLE_COMMAND, disassemble));
}
