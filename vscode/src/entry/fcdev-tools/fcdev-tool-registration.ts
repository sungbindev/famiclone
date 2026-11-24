// ------------------------------------------------------------------------
// 名称：fcdev-tool-registration.ts
// 说明：注册新建项目、构建、运行、项目信息、C 接口与使用文档的语言模型工具：按 package.json 里声明
//       的工具逐个注册，检查输入，需要时让用户确认，运行并整理结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：输入的结构取自 package.json 的 inputSchema；出错时把原因作为结果文字返回给模型（不抛出），
//       模型据此改参数；可预期的错误只给原因，其他错误记日志并给出通用说明。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { Logger } from '../../platform/logging/logger';
import { validateInput, type JsonSchema } from '../../shared/disassembly/schema-validator';
import { createFcdevTools, describeFailure, resolveWorkspacePath, type FcdevToolDefinition, type FcdevToolServices, type ToolInput } from './fcdev-tool-definitions';

/** package.json 里声明的一个语言模型工具。 */
interface DeclaredTool {
  readonly name: string;
  readonly inputSchema?: JsonSchema;
}

/** 当前打开的工作区文件夹路径。 */
function workspaceRoots(): string[] {
  return (vscode.workspace.workspaceFolders ?? []).map((folder) => folder.uri.fsPath);
}

/** 一个 FC Dev 语言模型工具。 */
class FcdevTool implements vscode.LanguageModelTool<ToolInput> {
  /**
   * @param definition 工具的定义。
   * @param schema 输入的结构。
   * @param logger 日志。
   */
  constructor(
    private readonly definition: FcdevToolDefinition,
    private readonly schema: JsonSchema,
    private readonly logger: Logger
  ) {}

  /** 调用前：写操作要用户确认。 */
  prepareInvocation(options: vscode.LanguageModelToolInvocationPrepareOptions<ToolInput>): vscode.PreparedToolInvocation {
    const input = options.input ?? {};
    const message = this.definition.describe(input);
    if (!this.definition.writes) return { invocationMessage: message };
    const text = this.definition.confirm?.(input) ?? `将执行 ${this.definition.name}。`;
    return { invocationMessage: message, confirmationMessages: { title: `FC Dev：${message}`, message: new vscode.MarkdownString(text) } };
  }

  /** 调用：检查输入、运行、整理结果。 */
  async invoke(options: vscode.LanguageModelToolInvocationOptions<ToolInput>): Promise<vscode.LanguageModelToolResult> {
    return new vscode.LanguageModelToolResult([new vscode.LanguageModelTextPart(await this.execute(options.input ?? {}))]);
  }

  /** 运行并返回给模型的文字。 */
  private async execute(input: ToolInput): Promise<string> {
    try {
      const problems = validateInput(this.schema, input);
      if (problems.length > 0) return `错误：输入不合法\n- ${problems.join('\n- ')}`;
      const roots = workspaceRoots();
      for (const name of this.definition.paths) {
        if (input[name] !== undefined) resolveWorkspacePath(input[name], name, roots);
      }
      return await this.definition.run(input, { roots });
    } catch (error) {
      const failure = describeFailure(error);
      if (failure !== undefined) return `错误：${failure}`;
      this.logger.error(`${this.definition.name} 执行出错`, error);
      return `错误：${this.definition.name} 执行出错（${error instanceof Error ? error.message : String(error)}），详见“FC Dev”输出通道`;
    }
  }
}

/**
 * 注册新建项目、构建、运行、项目信息、C 接口与使用文档工具。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param services 工具要用到的能力。
 * @param logger 日志。
 */
export function registerFcdevTools(context: vscode.ExtensionContext, services: FcdevToolServices, logger: Logger): void {
  const manifest = context.extension.packageJSON as { contributes?: { languageModelTools?: DeclaredTool[] } };
  const declared = new Map((manifest.contributes?.languageModelTools ?? []).map((tool) => [tool.name, tool.inputSchema ?? {}]));
  for (const definition of createFcdevTools(services)) {
    const schema = declared.get(definition.name);
    if (schema === undefined) {
      logger.error(`工具 ${definition.name} 没有在 package.json 里声明，未注册`);
      continue;
    }
    context.subscriptions.push(vscode.lm.registerTool(definition.name, new FcdevTool(definition, schema, logger)));
  }
}
