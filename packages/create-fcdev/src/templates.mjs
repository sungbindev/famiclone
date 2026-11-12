// ------------------------------------------------------------------------
// 名称：templates.mjs
// 说明：读取项目模板：列出可用的模板，按目标与语言取模板信息。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：每个模板是 templates/<目标>-<语言>/ 目录，含 template.json（目标、语言、说明、额外的 npm 脚本）；以 _ 开头的目录是公共文件，不是模板。
// ------------------------------------------------------------------------
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { CreateError } from './errors.mjs';

/** 模板所在目录 */
export const TEMPLATES_DIR = fileURLToPath(new URL('../templates/', import.meta.url));

/** 可选的语言 */
export const LANGUAGES = ['c', 'asm'];

/**
 * @typedef {object} Template
 * @property {string} name 模板名，如 nes-c
 * @property {string} dir 模板目录
 * @property {string} target 目标名
 * @property {string} language 语言：c 或 asm
 * @property {string} title 目标的显示名
 * @property {Record<string, string>} scripts 额外的 npm 脚本
 * @property {string[]} notes 创建完成后提示的补充说明
 */

/**
 * 列出全部模板。
 * @param {string} [templatesDir] 模板目录
 * @returns {Template[]} 模板列表，按名称排序
 * @throws {CreateError} 模板定义不合法
 */
export function listTemplates(templatesDir = TEMPLATES_DIR) {
  return readdirSync(templatesDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && !entry.name.startsWith('_'))
    .map((entry) => {
      const dir = path.join(templatesDir, entry.name);
      const file = path.join(dir, 'template.json');
      if (!existsSync(file)) throw new CreateError(`模板 ${entry.name} 缺少 template.json`, { code: 'TEMPLATE_INVALID' });
      const raw = JSON.parse(readFileSync(file, 'utf8'));
      if (typeof raw.target !== 'string' || !LANGUAGES.includes(raw.language) || typeof raw.title !== 'string' || entry.name !== `${raw.target}-${raw.language}`) {
        throw new CreateError(`模板 ${entry.name} 的 template.json 不合法`, { code: 'TEMPLATE_INVALID' });
      }
      return { name: entry.name, dir, target: raw.target, language: raw.language, title: raw.title, scripts: raw.scripts ?? {}, notes: raw.notes ?? [] };
    })
    .sort((left, right) => left.name.localeCompare(right.name));
}

/**
 * 按目标与语言取模板。
 * @param {string} target 目标名
 * @param {string} language 语言
 * @param {string} [templatesDir] 模板目录
 * @returns {Template} 模板
 * @throws {CreateError} 没有这个目标或语言的模板
 */
export function findTemplate(target, language, templatesDir = TEMPLATES_DIR) {
  const templates = listTemplates(templatesDir);
  const found = templates.find((item) => item.target === target && item.language === language);
  if (!found) {
    const targets = [...new Set(templates.map((item) => item.target))];
    throw new CreateError(`没有目标 ${target}、语言 ${language} 的模板`, {
      code: 'TEMPLATE_NOT_FOUND',
      hint: `可用的目标：${targets.join('、')}；可用的语言：${LANGUAGES.join('、')}`,
    });
  }
  return found;
}
