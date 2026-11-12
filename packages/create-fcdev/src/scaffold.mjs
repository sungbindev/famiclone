// ------------------------------------------------------------------------
// 名称：scaffold.mjs
// 说明：生成项目：检查目标目录为空，复制模板与公共文件，写 package.json 与 fcdev.json。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：模板在创建包内，不从网络下载；已有文件绝不覆盖，目录不为空时直接报错。
// ------------------------------------------------------------------------
import { cpSync, existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { CreateError } from './errors.mjs';
import { TEMPLATES_DIR } from './templates.mjs';

/** 目录里允许已经存在的内容 */
const ALLOWED_EXISTING = new Set(['.git']);

/** 模板里不复制到项目的文件 */
const TEMPLATE_ONLY_FILES = new Set(['template.json']);

/** 公共文件在模板目录里的名字与在项目里的名字：npm 发布时会丢掉 .gitignore，所以模板里不带点 */
const COMMON_FILES = [['gitignore', '.gitignore']];

/** 项目里 hello 程序的名字，同时是 fcdev.json 的 name，也是 DOS 里要输入的命令 */
export const PROGRAM_NAME = 'hello';

/**
 * 由目录名得到 npm 包名：小写，非法字符换成连字符。
 * @param {string} directoryName 目录名
 * @returns {string} 包名；目录名里没有可用字符时为 fcdev-app
 */
export function toPackageName(directoryName) {
  const cleaned = directoryName.toLowerCase().replace(/[^a-z0-9._-]+/g, '-').replace(/^[._-]+/, '').replace(/-+$/, '');
  return cleaned === '' ? 'fcdev-app' : cleaned;
}

/**
 * 读取创建包自己的版本号，生成的项目依赖同一个版本的 fcdev。
 * @returns {string} 版本号
 */
export function readOwnVersion() {
  return JSON.parse(readFileSync(fileURLToPath(new URL('../package.json', import.meta.url)), 'utf8')).version;
}

/**
 * 确认目标目录可以用来创建项目：不存在，或为空（允许已有 .git）。
 * @param {string} directory 目录的绝对路径
 * @throws {CreateError} 目录里已有其他内容
 */
export function assertDirectoryUsable(directory) {
  if (!existsSync(directory)) return;
  const existing = readdirSync(directory).filter((name) => !ALLOWED_EXISTING.has(name));
  if (existing.length > 0) {
    throw new CreateError(`目录 ${directory} 不为空`, {
      code: 'DIRECTORY_NOT_EMPTY',
      hint: `已有：${existing.slice(0, 5).join('、')}${existing.length > 5 ? ' 等' : ''}；请换一个空目录，或指定一个新目录名`,
    });
  }
}

/**
 * 在目录里生成项目。
 * @param {{directory: string, template: import('./templates.mjs').Template, fcdevVersion?: string, templatesDir?: string}} input 目标目录、模板与依赖的 fcdev 版本
 * @returns {{directory: string, name: string, files: string[]}} 项目目录、包名与生成的文件（相对路径）
 * @throws {CreateError} 目录不为空
 */
export function scaffoldProject({ directory, template, fcdevVersion = readOwnVersion(), templatesDir = TEMPLATES_DIR }) {
  assertDirectoryUsable(directory);
  mkdirSync(directory, { recursive: true });
  const name = toPackageName(path.basename(directory));

  cpSync(template.dir, directory, { recursive: true, filter: (source) => !TEMPLATE_ONLY_FILES.has(path.basename(source)) });
  for (const [from, to] of COMMON_FILES) cpSync(path.join(templatesDir, '_common', from), path.join(directory, to));

  const readme = path.join(directory, 'README.md');
  if (existsSync(readme)) writeFileSync(readme, readFileSync(readme, 'utf8').replaceAll('{{name}}', name));

  const packageJson = {
    name,
    version: '0.1.0',
    private: true,
    description: `${template.title} 程序`,
    scripts: { build: 'fcdev build src', start: 'fcdev run src', sync: 'fcdev sync', ...template.scripts },
    devDependencies: { fcdev: fcdevVersion },
  };
  writeFileSync(path.join(directory, 'package.json'), `${JSON.stringify(packageJson, null, 2)}\n`);
  writeFileSync(path.join(directory, 'fcdev.json'), `${JSON.stringify({ target: template.target, name: PROGRAM_NAME }, null, 2)}\n`);

  const files = [];
  const walk = (current) => {
    for (const entry of readdirSync(current, { withFileTypes: true })) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) {
        if (entry.name !== '.git') walk(full);
      } else {
        files.push(path.relative(directory, full).split(path.sep).join('/'));
      }
    }
  };
  walk(directory);
  return { directory, name, files: files.sort() };
}
