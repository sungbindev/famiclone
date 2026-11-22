// ------------------------------------------------------------------------
// 名称：scaffold-service.ts
// 说明：新建项目的接口：列出模板，在空目录里按模板生成项目。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { ProjectTemplate, TemplateLanguage } from '../../shared/scaffold/scaffold-files';

export type { ProjectTemplate, TemplateLanguage } from '../../shared/scaffold/scaffold-files';

/** 新建项目的输入。 */
export interface CreateProjectInput {
  /** 项目目录的绝对路径；不存在时创建，必须为空（允许已有 .git）。 */
  readonly directory: string;
  /** 目标名，如 nes、sb2000。 */
  readonly target: string;
  /** 语言。 */
  readonly language: TemplateLanguage;
}

/** 新建项目的结果。 */
export interface CreatedProject {
  /** 项目目录。 */
  readonly directory: string;
  /** 使用的模板。 */
  readonly template: ProjectTemplate;
  /** 生成的文件，相对项目目录，用 / 分隔，已排序。 */
  readonly files: readonly string[];
}

/** 新建项目服务。 */
export interface ScaffoldService {
  /**
   * 列出全部模板。
   * @returns 模板列表，按名称排序。
   * @throws FcdevError 模板定义不合法。
   */
  listTemplates(): readonly ProjectTemplate[];

  /**
   * 在目录里生成项目：模板文件、fcdev.json、.gitignore 与 .vscode/tasks.json；已有文件绝不覆盖。
   * @param input 目录、目标与语言。
   * @returns 生成结果。
   * @throws FcdevError 目录不为空，或没有这个目标与语言的模板。
   */
  create(input: CreateProjectInput): CreatedProject;
}
