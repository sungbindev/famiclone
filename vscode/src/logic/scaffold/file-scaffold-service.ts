// ------------------------------------------------------------------------
// 名称：file-scaffold-service.ts
// 说明：新建项目的实现：检查目标目录为空，复制模板与公共文件，
//       生成 fcdev.json 与 .vscode/tasks.json。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：模板在扩展目录的 templates 下，不从网络下载；已有文件绝不覆盖，目录不为空时直接报错。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { CreateProjectInput, CreatedProject, ProjectTemplate, ScaffoldService } from '../../contracts/scaffold/scaffold-service';
import type { TargetRepository } from '../../contracts/targets/target-repository';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { parseJsonText } from '../../shared/json/json-text';
import {
  LANGUAGES,
  buildProjectConfigText,
  buildTasksText,
  expandReadme,
  parseTemplateManifest
} from '../../shared/scaffold/scaffold-files';

/** 目录里允许已经存在的内容。 */
const ALLOWED_EXISTING = new Set(['.git']);

/** 模板里不复制到项目的文件。 */
const TEMPLATE_ONLY_FILES = new Set(['template.json']);

/** 公共文件在模板目录里的名字与在项目里的名字：模板里的 .gitignore 不带点，避免被工具忽略。 */
const COMMON_FILES: ReadonlyArray<readonly [string, string]> = [['gitignore', '.gitignore']];

/** 从扩展目录里的模板生成项目。 */
export class FileScaffoldService implements ScaffoldService {
  /**
   * 创建服务。
   * @param fileSystem 文件系统。
   * @param resources 扩展资源位置。
   * @param targets 目标定义读取，用来判断目标是否支持磁盘镜像。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly resources: ExtensionResources,
    private readonly targets: TargetRepository
  ) {}

  /** @inheritdoc */
  listTemplates(): readonly ProjectTemplate[] {
    const root = this.resources.templatesDirectory;
    return this.fileSystem
      .list(root)
      .filter((entry) => entry.isDirectory && !entry.name.startsWith('_'))
      .map((entry) => {
        const file = path.join(root, entry.name, 'template.json');
        if (this.fileSystem.kindOf(file) !== 'file') throw new FcdevError(`模板 ${entry.name} 缺少 template.json`, { code: 'TEMPLATE_INVALID' });
        return parseTemplateManifest(parseJsonText(this.fileSystem.readText(file), file), entry.name);
      })
      .toSorted((left, right) => left.name.localeCompare(right.name));
  }

  /** @inheritdoc */
  create(input: CreateProjectInput): CreatedProject {
    const templates = this.listTemplates();
    const template = templates.find((item) => item.target === input.target && item.language === input.language);
    if (template === undefined) {
      const targetNames = [...new Set(templates.map((item) => item.target))];
      throw new FcdevError(`没有目标 ${input.target}、语言 ${input.language} 的模板`, {
        code: 'TEMPLATE_NOT_FOUND',
        hint: `可用的目标：${targetNames.join('、')}；可用的语言：${LANGUAGES.join('、')}`
      });
    }
    const { directory } = input;
    this.assertDirectoryUsable(directory);
    this.fileSystem.ensureDirectory(directory);

    const templateDirectory = path.join(this.resources.templatesDirectory, template.name);
    this.copyTemplate(templateDirectory, directory, path.basename(directory));
    for (const [from, to] of COMMON_FILES) {
      this.fileSystem.writeBytes(path.join(directory, to), this.fileSystem.readBytes(path.join(this.resources.templatesDirectory, '_common', from)));
    }
    this.fileSystem.writeText(path.join(directory, 'fcdev.json'), buildProjectConfigText(template.target));
    this.fileSystem.ensureDirectory(path.join(directory, '.vscode'));
    this.fileSystem.writeText(path.join(directory, '.vscode', 'tasks.json'), buildTasksText(this.targets.load(template.target).disk !== null));
    return { directory, template, files: this.listFiles(directory) };
  }

  /**
   * 确认目标目录可以用来创建项目：不存在，或为空（允许已有 .git）。
   * @param directory 目录的绝对路径。
   * @throws FcdevError 目录里已有其他内容。
   */
  private assertDirectoryUsable(directory: string): void {
    if (this.fileSystem.kindOf(directory) === undefined) return;
    const existing = this.fileSystem.list(directory).filter((entry) => !ALLOWED_EXISTING.has(entry.name));
    if (existing.length > 0) {
      throw new FcdevError(`目录 ${directory} 不为空`, {
        code: 'DIRECTORY_NOT_EMPTY',
        hint: `已有：${existing.slice(0, 5).map((entry) => entry.name).join('、')}${existing.length > 5 ? ' 等' : ''}；请换一个空目录，或指定一个新目录名`
      });
    }
  }

  /**
   * 复制模板目录里的文件到项目，README.md 里的 {{name}} 换成项目名。
   * @param from 模板目录。
   * @param to 项目目录。
   * @param projectName 项目名。
   */
  private copyTemplate(from: string, to: string, projectName: string): void {
    for (const entry of this.fileSystem.list(from)) {
      if (TEMPLATE_ONLY_FILES.has(entry.name)) continue;
      const source = path.join(from, entry.name);
      const target = path.join(to, entry.name);
      if (entry.isDirectory) {
        this.fileSystem.ensureDirectory(target);
        this.copyTemplate(source, target, projectName);
      } else if (entry.name === 'README.md') {
        this.fileSystem.writeText(target, expandReadme(this.fileSystem.readText(source), projectName));
      } else {
        this.fileSystem.writeBytes(target, this.fileSystem.readBytes(source));
      }
    }
  }

  /**
   * 列出目录里的全部文件（不含 .git）。
   * @param directory 项目目录。
   * @returns 相对项目目录、用 / 分隔、已排序的文件路径。
   */
  private listFiles(directory: string): string[] {
    const files: string[] = [];
    const walk = (current: string): void => {
      for (const entry of this.fileSystem.list(current)) {
        const full = path.join(current, entry.name);
        if (entry.isDirectory) {
          if (entry.name !== '.git') walk(full);
        } else {
          files.push(path.relative(directory, full).split(path.sep).join('/'));
        }
      }
    };
    walk(directory);
    return files.toSorted();
  }
}
