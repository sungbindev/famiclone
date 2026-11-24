// ------------------------------------------------------------------------
// 名称：scaffold-files.ts
// 说明：新建项目要生成的文件：模板清单的解析、fcdev.json、.vscode/tasks.json 与 README 的内容。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：模板是 templates/<目标>-<语言>/ 目录，含 template.json（目标、语言、显示名）、
//       README.md 与 src；以 _ 开头的目录是公共文件，不是模板；
//       程序名固定为 hello，也是 DOS 里要输入的命令。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import { isRecord } from '../json/json-text';

/** 可选的语言。 */
export const LANGUAGES = ['c', 'asm'] as const;

/** 语言。 */
export type TemplateLanguage = (typeof LANGUAGES)[number];

/** 语言的显示名。 */
export const LANGUAGE_LABELS: Readonly<Record<TemplateLanguage, string>> = { c: 'C 语言（cc65）', asm: '汇编（ca65）' };

/** 项目里 hello 程序的名字，同时是 fcdev.json 的 name，也是 DOS 里要输入的命令。 */
export const PROGRAM_NAME = 'hello';

/** 一个项目模板。 */
export interface ProjectTemplate {
  /** 模板名，如 nes-c。 */
  readonly name: string;
  /** 目标名。 */
  readonly target: string;
  /** 语言。 */
  readonly language: TemplateLanguage;
  /** 目标的显示名。 */
  readonly title: string;
}

/**
 * 判断值是不是支持的语言。
 * @param value 任意值。
 * @returns 是支持的语言时为 true。
 */
export function isTemplateLanguage(value: unknown): value is TemplateLanguage {
  return typeof value === 'string' && (LANGUAGES as readonly string[]).includes(value);
}

/**
 * 解析模板清单 template.json。
 * @param raw 解析后的 JSON。
 * @param name 模板目录名，应等于“目标-语言”。
 * @returns 模板。
 * @throws FcdevError 清单不合法。
 */
export function parseTemplateManifest(raw: unknown, name: string): ProjectTemplate {
  if (!isRecord(raw) || typeof raw.target !== 'string' || !isTemplateLanguage(raw.language) || typeof raw.title !== 'string' || name !== `${raw.target}-${raw.language}`) {
    throw new FcdevError(`模板 ${name} 的 template.json 不合法`, { code: 'TEMPLATE_INVALID' });
  }
  return { name, target: raw.target, language: raw.language, title: raw.title };
}

/**
 * 生成 fcdev.json 的内容。
 * @param target 目标名。
 * @returns 文件文本。
 */
export function buildProjectConfigText(target: string): string {
  return `${JSON.stringify({ target, name: PROGRAM_NAME }, null, 2)}\n`;
}

/**
 * 生成一个 FC Dev 任务。
 * @param label 任务名。
 * @param command 任务命令。
 * @param extra 额外字段。
 * @returns 任务对象。
 */
function createTask(label: string, command: string, extra: Record<string, unknown> = {}): Record<string, unknown> {
  return { label, type: 'fcdev', command, path: 'src', ...extra, problemMatcher: '$fcdev' };
}

/**
 * 生成 .vscode/tasks.json 的内容：构建（默认构建任务）、运行。
 * @returns 文件文本。
 */
export function buildTasksText(): string {
  const tasks = [createTask('FC Dev：构建', 'build', { group: { kind: 'build', isDefault: true } }), createTask('FC Dev：运行', 'run')];
  return `${JSON.stringify({ version: '2.0.0', tasks }, null, 2)}\n`;
}

/**
 * 把 README 里的 {{name}} 换成项目名。
 * @param text README 模板文本。
 * @param name 项目名。
 * @returns 换好的文本。
 */
export function expandReadme(text: string, name: string): string {
  return text.replaceAll('{{name}}', name);
}
