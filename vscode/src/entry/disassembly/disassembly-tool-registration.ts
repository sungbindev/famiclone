// ------------------------------------------------------------------------
// 名称：disassembly-tool-registration.ts
// 说明：注册反汇编的语言模型工具：按 package.json 里声明的工具逐个注册，检查输入、
//       确认提示、调用 fcdev、整理结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：输入的结构取自 package.json 的 inputSchema；出错时把原因作为结果文字返回给模型
//       （不抛出），模型据此改参数；
//       可预期的错误（UserFacingError）只给原因，其他错误记日志并给出通用说明。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { BuildComparer } from '../../contracts/disassembly/build-comparer';
import type { DisasmRunner } from '../../contracts/disassembly/disasm-runner';
import type { ToolInput, ToolSpec } from '../../contracts/disassembly/tool-spec';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { formatResult } from '../../shared/disassembly/result-text';
import { validateInput, type JsonSchema } from '../../shared/disassembly/schema-validator';
import { ANALYSIS_TOOLS } from './analysis-tools';
import { EDITING_TOOLS } from './editing-tools';
import { OUTPUT_TOOLS } from './output-tools';
import { PROJECT_TOOLS } from './project-tools';
import { resolveToolInput } from './tool-paths';

/** 全部反汇编工具，按“工程与映像、分析、编辑、生成与校验”四组排列。 */
export const DISASSEMBLY_TOOLS: readonly ToolSpec[] = [...PROJECT_TOOLS, ...ANALYSIS_TOOLS, ...EDITING_TOOLS, ...OUTPUT_TOOLS];

/** 反汇编工具要用的外部能力。 */
export interface DisassemblyToolDependencies {
  readonly runner: DisasmRunner;
  readonly comparer: BuildComparer;
  readonly logger: Logger;
}

/** package.json 里声明的一个语言模型工具。 */
interface DeclaredTool {
  readonly name: string;
  readonly inputSchema?: JsonSchema;
}

/** 当前打开的工作区文件夹路径。 */
function workspaceRoots(): string[] {
  return (vscode.workspace.workspaceFolders ?? []).map((folder) => folder.uri.fsPath);
}

/** 这次调用是否要用户确认。 */
function needsConfirmation(spec: ToolSpec, input: ToolInput): boolean {
  return typeof spec.writes === 'function' ? spec.writes(input) : spec.writes;
}

/** 一个反汇编工具。 */
class DisassemblyTool implements vscode.LanguageModelTool<ToolInput> {
  /**
   * @param spec 工具的描述。
   * @param schema 输入的结构。
   * @param deps 外部能力。
   */
  constructor(
    private readonly spec: ToolSpec,
    private readonly schema: JsonSchema,
    private readonly deps: DisassemblyToolDependencies
  ) {}

  /** 调用前：写操作要用户确认，路径在工作区之外时在确认里说明。 */
  prepareInvocation(options: vscode.LanguageModelToolInvocationPrepareOptions<ToolInput>): vscode.PreparedToolInvocation {
    const input = options.input ?? {};
    const message = this.spec.describe(input);
    if (!needsConfirmation(this.spec, input)) return { invocationMessage: message };
    let text = this.spec.confirm?.(input) ?? `将执行 ${this.spec.name}。`;
    try {
      const { outside } = resolveToolInput(this.spec, input, workspaceRoots());
      if (outside.length > 0) text += `\n\n注意：这些路径在工作区之外：${outside.join('、')}`;
    } catch {
      // 路径不合法时调用本身会给出错误，这里照常让用户看到确认说明
    }
    return { invocationMessage: message, confirmationMessages: { title: `fcdev：${message}`, message: new vscode.MarkdownString(text) } };
  }

  /** 调用：检查输入、换算路径、运行操作、整理结果。 */
  async invoke(options: vscode.LanguageModelToolInvocationOptions<ToolInput>): Promise<vscode.LanguageModelToolResult> {
    return new vscode.LanguageModelToolResult([new vscode.LanguageModelTextPart(await this.execute(options.input ?? {}))]);
  }

  /** 运行并返回给模型的文字。 */
  private async execute(input: ToolInput): Promise<string> {
    try {
      const problems = validateInput(this.schema, input);
      if (problems.length > 0) return `错误：输入不合法\n- ${problems.join('\n- ')}`;
      const resolved = resolveToolInput(this.spec, input, workspaceRoots());
      if (this.spec.operation === 'build') {
        const comparison = await this.deps.comparer.buildAndCompare(String(resolved.params.projectDir), String(resolved.params.buildPath));
        return formatResult(comparison);
      }
      return formatResult(await this.deps.runner.run(resolved.startDirectory, this.spec.operation, resolved.params));
    } catch (error) {
      if (error instanceof UserFacingError) return `错误：${error.message}`;
      this.deps.logger.error(`${this.spec.name} 执行出错`, error);
      return `错误：${this.spec.name} 执行出错（${error instanceof Error ? error.message : String(error)}），详见“fcdev”输出通道`;
    }
  }
}

/**
 * 注册全部反汇编工具。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param deps 外部能力。
 */
export function registerDisassemblyTools(context: vscode.ExtensionContext, deps: DisassemblyToolDependencies): void {
  const manifest = context.extension.packageJSON as { contributes?: { languageModelTools?: DeclaredTool[] } };
  const declared = new Map((manifest.contributes?.languageModelTools ?? []).map((tool) => [tool.name, tool.inputSchema ?? {}]));
  for (const spec of DISASSEMBLY_TOOLS) {
    const schema = declared.get(spec.name);
    if (schema === undefined) {
      deps.logger.error(`工具 ${spec.name} 没有在 package.json 里声明，未注册`);
      continue;
    }
    context.subscriptions.push(vscode.lm.registerTool(spec.name, new DisassemblyTool(spec, schema, deps)));
  }
}
