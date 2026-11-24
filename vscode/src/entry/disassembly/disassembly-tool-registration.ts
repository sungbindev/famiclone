// ------------------------------------------------------------------------
// 名称：disassembly-tool-registration.ts
// 说明：注册反汇编与模拟器的语言模型工具：按 package.json 里声明的工具逐个注册，检查输入、
//       确认提示、调用反汇编核心、整理结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：输入的结构取自 package.json 的 inputSchema；出错时把原因作为结果文字返回给模型
//       （不抛出），模型据此改参数；
//       可预期的错误（UserFacingError）只给原因，其他错误记日志并给出通用说明；
//       结果里的截图作为图像部分返回（当前 VS Code 不支持时只给文字说明）。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { BuildComparer } from '../../contracts/disassembly/build-comparer';
import type { DisasmOperations } from '../../contracts/disassembly/disasm-operations';
import type { ToolInput, ToolSpec } from '../../contracts/disassembly/tool-spec';
import type { EmulatorOperations } from '../../contracts/emulator/emulator-operations';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { formatResult, splitImage, type ResultImage } from '../../shared/disassembly/result-text';
import { validateInput, type JsonSchema } from '../../shared/disassembly/schema-validator';
import { ANALYSIS_TOOLS } from './analysis-tools';
import { EDITING_TOOLS } from './editing-tools';
import { EMULATOR_TOOLS } from './emulator-tools';
import { OUTPUT_TOOLS } from './output-tools';
import { PROJECT_TOOLS } from './project-tools';
import { resolveToolInput } from './tool-paths';

/** 全部反汇编工具，按“工程与映像、分析、编辑、生成与校验”四组排列。 */
export const DISASSEMBLY_TOOLS: readonly ToolSpec[] = [...PROJECT_TOOLS, ...ANALYSIS_TOOLS, ...EDITING_TOOLS, ...OUTPUT_TOOLS];

/** 注册的全部工具：反汇编工具加模拟器工具。 */
export const ALL_TOOLS: readonly ToolSpec[] = [...DISASSEMBLY_TOOLS, ...EMULATOR_TOOLS];

/** 工具要用的外部能力。 */
export interface DisassemblyToolDependencies {
  readonly operations: DisasmOperations;
  readonly emulator: EmulatorOperations;
  readonly comparer: BuildComparer;
  readonly logger: Logger;
}

/** package.json 里声明的一个语言模型工具。 */
interface DeclaredTool {
  readonly name: string;
  readonly inputSchema?: JsonSchema;
}

/** 工具返回的内容：给模型看的文字和可选的图像。 */
interface ToolOutput {
  readonly text: string;
  readonly image?: ResultImage;
}

/** 较新的 VS Code 提供的工具结果数据部分（当前声明的接口版本里没有，运行时检测）。 */
interface DataPartFactory {
  image(data: Uint8Array, mimeType: string): vscode.LanguageModelTextPart;
}

/** 当前 VS Code 不能在工具结果里返回图像时的说明。 */
const IMAGE_UNSUPPORTED = '当前 VS Code 不支持在工具结果里返回图像；请给 file 参数把截图保存成文件';

/** 当前打开的工作区文件夹路径。 */
function workspaceRoots(): string[] {
  return (vscode.workspace.workspaceFolders ?? []).map((folder) => folder.uri.fsPath);
}

/** 这次调用是否会写东西、要用户确认。 */
function needsConfirmation(spec: ToolSpec, input: ToolInput): boolean {
  return typeof spec.writes === 'function' ? spec.writes(input) : spec.writes;
}

/** 把图像做成工具结果的部分；当前 VS Code 不支持时返回 undefined。 */
function imagePart(image: ResultImage): vscode.LanguageModelTextPart | undefined {
  const factory = (vscode as unknown as { LanguageModelDataPart?: DataPartFactory }).LanguageModelDataPart;
  return factory?.image(image.data, image.mimeType);
}

/** 一个反汇编或模拟器工具。 */
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

  /** 调用前：写操作或路径在工作区之外时要用户确认，并说明哪些路径在工作区之外。 */
  prepareInvocation(options: vscode.LanguageModelToolInvocationPrepareOptions<ToolInput>): vscode.PreparedToolInvocation {
    const input = options.input ?? {};
    const message = this.spec.describe(input);
    let outside: readonly string[] = [];
    try {
      outside = resolveToolInput(this.spec, input, workspaceRoots()).outside;
    } catch {
      // 路径不合法时调用本身会给出错误，这里照常让用户看到确认说明
    }
    const writes = needsConfirmation(this.spec, input);
    if (!writes && outside.length === 0) return { invocationMessage: message };
    let text = writes ? (this.spec.confirm?.(input) ?? `将执行 ${this.spec.name}。`) : '将读取工作区之外的文件。';
    if (outside.length > 0) text += `\n\n注意：这些路径在工作区之外：${outside.join('、')}`;
    return { invocationMessage: message, confirmationMessages: { title: `FCDEV：${message}`, message: new vscode.MarkdownString(text) } };
  }

  /** 调用：检查输入、换算路径、运行操作、整理结果。 */
  async invoke(options: vscode.LanguageModelToolInvocationOptions<ToolInput>): Promise<vscode.LanguageModelToolResult> {
    const { text, image } = await this.execute(options.input ?? {});
    const parts = [new vscode.LanguageModelTextPart(text)];
    if (image !== undefined) {
      const part = imagePart(image);
      if (part === undefined) parts[0] = new vscode.LanguageModelTextPart(`${text}\n\n${IMAGE_UNSUPPORTED}`);
      else parts.push(part);
    }
    return new vscode.LanguageModelToolResult(parts);
  }

  /** 运行并返回给模型的内容。 */
  private async execute(input: ToolInput): Promise<ToolOutput> {
    try {
      const problems = validateInput(this.schema, input);
      if (problems.length > 0) return { text: `错误：输入不合法\n- ${problems.join('\n- ')}` };
      const resolved = resolveToolInput(this.spec, input, workspaceRoots());
      if (this.spec.operation === 'build') {
        const comparison = await this.deps.comparer.buildAndCompare(String(resolved.params.projectDir), String(resolved.params.buildPath));
        return { text: formatResult(comparison) };
      }
      const raw =
        this.spec.target === 'emulator'
          ? await this.deps.emulator.run(this.spec.operation, resolved.params)
          : await this.deps.operations.run(this.spec.operation, resolved.params);
      const { result, image } = splitImage(raw);
      return { text: formatResult(result), image };
    } catch (error) {
      if (error instanceof UserFacingError) return { text: `错误：${error.message}` };
      this.deps.logger.error(`${this.spec.name} 执行出错`, error);
      return { text: `错误：${this.spec.name} 执行出错（${error instanceof Error ? error.message : String(error)}），详见“FC Dev”输出通道` };
    }
  }
}

/**
 * 注册全部反汇编与模拟器工具。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param deps 外部能力。
 */
export function registerDisassemblyTools(context: vscode.ExtensionContext, deps: DisassemblyToolDependencies): void {
  const manifest = context.extension.packageJSON as { contributes?: { languageModelTools?: DeclaredTool[] } };
  const declared = new Map((manifest.contributes?.languageModelTools ?? []).map((tool) => [tool.name, tool.inputSchema ?? {}]));
  for (const spec of ALL_TOOLS) {
    const schema = declared.get(spec.name);
    if (schema === undefined) {
      deps.logger.error(`工具 ${spec.name} 没有在 package.json 里声明，未注册`);
      continue;
    }
    context.subscriptions.push(vscode.lm.registerTool(spec.name, new DisassemblyTool(spec, schema, deps)));
  }
}
