// ------------------------------------------------------------------------
// 名称：fcdev-tool-definitions.ts
// 说明：新建项目、构建、运行、项目信息、C 接口与使用文档六个语言模型工具的定义：调用时
//       显示的文字、是否要用户确认、路径规则与执行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：工具的名字、说明与参数结构只写在 package.json 的 languageModelTools 里，这里不重复；
//       路径参数相对第一个工作区文件夹，必须在工作区里；执行结果都是给模型看的文字，出错时
//       由注册处转成“错误：…”，不抛给模型。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { BuildService } from '../../contracts/build/build-service';
import type { CApiService } from '../../contracts/c-api/c-api-service';
import type { ProgramRunner } from '../../contracts/emulator/emulator-operations';
import type { ProjectDescriber } from '../../contracts/project/project-describer';
import type { ScaffoldService } from '../../contracts/scaffold/scaffold-service';
import type { UsageDocsService } from '../../contracts/usage-docs/usage-docs-service';
import { formatModules, formatQueryResult, groupByModule, queryApi } from '../../shared/c-api/api-functions';
import { describeBuildResult, describeDiskImage } from '../../shared/build/task-definition';
import { describeRunResult } from '../../shared/emulator/run-text';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { formatProjectDescription } from '../../shared/project/project-info';
import { isTemplateLanguage } from '../../shared/scaffold/scaffold-files';
import { formatDocList } from '../../shared/usage-docs/doc-topics';

/** 工具的输入：模型给出的参数。 */
export type ToolInput = Record<string, unknown>;

/** 一个工具在代码里的定义。 */
export interface FcdevToolDefinition {
  /** 工具名，与 package.json 里声明的一致。 */
  readonly name: string;
  /** 会写文件（创建项目、构建产物），调用前要用户确认。 */
  readonly writes: boolean;
  /** 调用时在聊天里显示的文字。 */
  describe(input: ToolInput): string;
  /** 需要确认时，对这次调用要做什么的说明。 */
  confirm?(input: ToolInput): string;
  /** 取值是路径、要按工作区规则检查的参数名。 */
  readonly paths: readonly string[];
  /** 执行，返回给模型的文字。 */
  run(input: ToolInput, context: ToolContext): Promise<string>;
}

/** 执行时的环境。 */
export interface ToolContext {
  /** 工作区文件夹路径；第一个是相对路径的基准。 */
  readonly roots: readonly string[];
}

/** 工具要用到的能力。 */
export interface FcdevToolServices {
  readonly build: BuildService;
  readonly runner: ProgramRunner;
  readonly scaffold: ScaffoldService;
  readonly describer: ProjectDescriber;
  readonly api: CApiService;
  readonly docs: UsageDocsService;
}

/** 路径 child 是否在目录 parent 里面（含相同）。 */
function isInside(parent: string, child: string): boolean {
  const relative = path.relative(parent, child);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}

/**
 * 把路径参数换成绝对路径：相对路径按第一个工作区文件夹解析，结果必须在工作区里。
 * @param value 参数值；不给时取第一个工作区文件夹。
 * @param name 参数名，用在出错提示里。
 * @param roots 工作区文件夹路径。
 * @returns 绝对路径。
 * @throws UserFacingError 没有工作区文件夹，或路径在工作区之外。
 */
export function resolveWorkspacePath(value: unknown, name: string, roots: readonly string[]): string {
  const base = roots[0];
  if (base === undefined) throw new UserFacingError('没有打开工作区文件夹');
  if (value === undefined) return base;
  if (typeof value !== 'string' || value === '') throw new UserFacingError(`参数 ${name} 必须是路径`);
  const absolute = path.isAbsolute(value) ? path.resolve(value) : path.resolve(base, value);
  if (!roots.some((root) => isInside(root, absolute))) throw new UserFacingError(`参数 ${name} 必须在工作区里：${absolute}`);
  return absolute;
}

/**
 * 把错误转成给模型看的说明：可预期的错误带上建议。
 * @param error 捕获到的错误。
 * @returns 说明文字；不是可预期的错误时为 undefined。
 */
export function describeFailure(error: unknown): string | undefined {
  if (!(error instanceof UserFacingError)) return undefined;
  return error instanceof FcdevError && error.hint !== '' ? `${error.message}\n${error.hint}` : error.message;
}

/**
 * 创建六个工具的定义。
 * @param services 工具要用到的能力。
 * @returns 工具定义。
 */
export function createFcdevTools(services: FcdevToolServices): readonly FcdevToolDefinition[] {
  const newProject: FcdevToolDefinition = {
    name: 'fcdev_new_project',
    writes: true,
    paths: ['directory'],
    describe: (input) => `新建 FC Dev 项目：${String(input.directory)}`,
    confirm: (input) => `将在 ${String(input.directory)} 里按模板 ${String(input.target)}（${String(input.language)}）生成项目文件。目录必须为空，已有文件不会被覆盖。`,
    async run(input, context) {
      if (!isTemplateLanguage(input.language)) throw new UserFacingError('参数 language 必须是 c 或 asm');
      const created = services.scaffold.create({
        directory: resolveWorkspacePath(input.directory, 'directory', context.roots),
        target: String(input.target),
        language: input.language
      });
      return `已在 ${created.directory} 创建项目（模板 ${created.template.name}），共 ${created.files.length} 个文件：\n${created.files.join('\n')}`;
    }
  };

  const build: FcdevToolDefinition = {
    name: 'fcdev_build',
    writes: true,
    paths: ['path'],
    describe: (input) => `构建：${String(input.path ?? '工作区')}`,
    confirm: (input) => `将构建 ${String(input.path ?? '工作区文件夹')}，产物、磁盘镜像与中间文件写在项目的输出目录（默认 build）里。`,
    async run(input, context) {
      const base = resolveWorkspacePath(undefined, 'path', context.roots);
      const target = resolveWorkspacePath(input.path, 'path', context.roots);
      const output: string[] = [];
      const reporter = { log: (message: string) => output.push(`${message}\n`), write: (text: string) => output.push(text) };
      try {
        const outcome = await services.build.build(
          { pathArg: path.relative(base, target) || '.', cwd: base, targetOverride: typeof input.target === 'string' ? input.target : undefined },
          reporter
        );
        output.push(`${describeBuildResult(outcome.result, outcome.plan.target.name, base)}\n`);
        if (outcome.disk !== undefined) output.push(`${describeDiskImage(outcome.disk, base)}\n`);
        return output.join('').trim();
      } catch (error) {
        const failure = describeFailure(error);
        if (failure === undefined) throw error;
        return `${output.join('')}错误：${failure}`.trim();
      }
    }
  };

  const run: FcdevToolDefinition = {
    name: 'fcdev_run',
    writes: true,
    paths: ['path'],
    describe: (input) => `${input.debug === true ? '调试运行' : '运行'}：${String(input.path ?? '工作区')}`,
    confirm: (input) => `将构建 ${String(input.path ?? '工作区文件夹')} 并启动设置里的 Ember 运行它${input.debug === true ? '（调试模式，程序载入后停在开机状态）' : ''}。Ember 与扩展脱离，窗口会一直打开，由你关闭。`,
    async run(input, context) {
      const base = resolveWorkspacePath(undefined, 'path', context.roots);
      const target = resolveWorkspacePath(input.path, 'path', context.roots);
      const debug = input.debug === true;
      const output: string[] = [];
      const reporter = { log: (message: string) => output.push(`${message}\n`), write: (text: string) => output.push(text) };
      try {
        const result = await services.runner.run(
          { build: { pathArg: path.relative(base, target) || '.', cwd: base, targetOverride: typeof input.target === 'string' ? input.target : undefined }, debug },
          reporter
        );
        output.push(`${describeRunResult(result)}\n`);
        return output.join('').trim();
      } catch (error) {
        const failure = describeFailure(error);
        if (failure === undefined) throw error;
        return `${output.join('')}错误：${failure}`.trim();
      }
    }
  };

  const info: FcdevToolDefinition = {
    name: 'fcdev_info',
    writes: false,
    paths: ['path'],
    describe: (input) => `读取 FC Dev 项目信息：${String(input.path ?? '工作区')}`,
    run: (input, context) =>
      Promise.resolve(
        formatProjectDescription(
          services.describer.describe({
            cwd: resolveWorkspacePath(input.path, 'path', context.roots),
            target: typeof input.target === 'string' ? input.target : undefined
          })
        )
      )
  };

  const api: FcdevToolDefinition = {
    name: 'fcdev_api',
    writes: false,
    paths: ['path'],
    describe: (input) => `查询 C 接口：${String(input.query ?? '全部模块')}`,
    run(input, context) {
      const target = services.api.describe({
        cwd: resolveWorkspacePath(input.path, 'path', context.roots),
        target: typeof input.target === 'string' ? input.target : undefined
      });
      const query = typeof input.query === 'string' ? input.query.trim() : '';
      if (query === '') return Promise.resolve(formatModules(target.targetTitle, groupByModule(target.functions)));
      const result = queryApi(target.functions, query);
      return Promise.resolve(result.matches.length === 0 ? `没有找到“${query}”` : formatQueryResult(query, result));
    }
  };

  const docs: FcdevToolDefinition = {
    name: 'fcdev_docs',
    writes: false,
    paths: [],
    describe: (input) => `查询 FC Dev 使用文档：${String(input.query ?? '全部主题')}`,
    run(input) {
      const query = typeof input.query === 'string' ? input.query.trim() : '';
      if (query === '') return Promise.resolve(`使用文档：${services.docs.list().length} 份\n${formatDocList(services.docs.list())}`);
      const { matches } = services.docs.query(query);
      if (matches.length === 0) return Promise.resolve(`没有找到“${query}”，不带 query 调用可以看到全部主题`);
      const [only] = matches;
      if (matches.length === 1 && only !== undefined) return Promise.resolve(`# ${only.title}（主题 ${only.topic}）\n\n${services.docs.read(only)}`);
      return Promise.resolve(`包含“${query}”的文档：${matches.length} 份\n${formatDocList(matches)}\n\n用主题名再查一次可读取全文`);
    }
  };

  return [newProject, build, run, info, api, docs];
}
