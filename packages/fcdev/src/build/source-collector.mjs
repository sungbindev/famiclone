// ------------------------------------------------------------------------
// 名称：source-collector.mjs
// 说明：收集目录下的源文件，以及判断源文件的语言。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：递归收集 .c、.s、.asm；跳过输出目录 build、node_modules 与以 . 开头的目录。
// ------------------------------------------------------------------------
import { readdirSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';

/** 递归收集时跳过的目录名 */
const SKIPPED_DIRECTORIES = new Set(['build', 'node_modules']);

/**
 * 判断源文件的语言。
 * @param {string} file 文件路径
 * @returns {'c'|'asm'|null} c、asm，或不是源文件时为 null
 */
export function getSourceLanguage(file) {
  const extension = path.extname(file).toLowerCase();
  if (extension === '.c') return 'c';
  if (extension === '.s' || extension === '.asm') return 'asm';
  return null;
}

/**
 * 递归收集目录下的全部源文件。
 * @param {string} dir 目录的绝对路径
 * @param {string[]} [excludedDirs] 额外跳过的目录（绝对路径，如配置的输出目录）
 * @returns {string[]} 源文件的绝对路径，按相对路径排序
 * @throws {FcdevError} 目录下没有任何源文件
 */
export function collectSources(dir, excludedDirs = []) {
  const excluded = new Set(excludedDirs.map((item) => path.resolve(item)));
  const found = [];
  const walk = (current) => {
    for (const entry of readdirSync(current, { withFileTypes: true })) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) {
        if (!SKIPPED_DIRECTORIES.has(entry.name) && !entry.name.startsWith('.') && !excluded.has(full)) walk(full);
      } else if (getSourceLanguage(entry.name)) {
        found.push(full);
      }
    }
  };
  walk(dir);
  found.sort((left, right) => path.relative(dir, left).localeCompare(path.relative(dir, right)));
  if (found.length === 0) {
    throw new FcdevError(`目录 ${dir} 下没有 .c、.s 或 .asm 源文件`, { code: 'NO_SOURCES' });
  }
  return found;
}
