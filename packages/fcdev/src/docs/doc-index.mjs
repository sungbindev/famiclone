// ------------------------------------------------------------------------
// 名称：doc-index.mjs
// 说明：使用文档的查询：列出全部主题，按主题名、文件名或关键词查找，给出每份文档的本机路径。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：文档随主包发布在包根的 docs 目录。
// ------------------------------------------------------------------------
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { DOC_TOPICS } from './doc-topics.mjs';

/** 主包根目录 */
const PACKAGE_DIR = fileURLToPath(new URL('../../', import.meta.url));

/**
 * @typedef {object} DocInfo
 * @property {string} topic 主题名
 * @property {string} title 标题
 * @property {string} summary 一句话说明
 * @property {string} file 文件路径：相对 docs 目录，`inPackage` 为 true 时相对主包根目录
 * @property {boolean} [inPackage] 文件在主包根目录下的其他位置，不在 docs 目录里
 * @property {string} localPath 本机的绝对路径
 */

/**
 * docs 目录的绝对路径。
 * @returns {string} 目录路径
 */
export function docsDirectory() {
  return path.join(PACKAGE_DIR, 'docs');
}

/**
 * 把登记项补全成带本机路径的文档信息。
 * @param {import('./doc-topics.mjs').DocTopic} entry 登记项
 * @returns {DocInfo} 文档信息
 */
function describe(entry) {
  const segments = entry.file.split('/');
  return {
    ...entry,
    localPath: path.join(entry.inPackage ? PACKAGE_DIR : docsDirectory(), ...segments),
  };
}

/**
 * 列出全部文档。
 * @returns {DocInfo[]} 按登记顺序
 */
export function listDocs() {
  return DOC_TOPICS.map((entry) => describe(entry));
}

/**
 * 文件名去掉序号前缀与扩展名后的部分，如 `sb2000/02-系统调用手册.md` 得到 `系统调用手册`。
 * @param {string} file 相对 docs 目录的文件路径
 * @returns {string} 名称
 */
function fileLabel(file) {
  return path.posix.basename(file, '.md').replace(/^\d+-/, '');
}

/**
 * 按词查找文档：先按主题名精确匹配，再按文件名，最后按主题、标题与说明里的关键词。
 * @param {string} query 查询词，不分大小写
 * @returns {{kind: 'topic'|'file'|'text'|'none', matches: DocInfo[]}} 匹配方式与结果
 */
export function queryDocs(query) {
  const word = query.trim().toLowerCase();
  const docs = listDocs();
  const byTopic = docs.filter((doc) => doc.topic === word);
  if (byTopic.length > 0) return { kind: 'topic', matches: byTopic };
  const byFile = docs.filter((doc) => path.posix.basename(doc.file, '.md').toLowerCase() === word || fileLabel(doc.file).toLowerCase() === word);
  if (byFile.length > 0) return { kind: 'file', matches: byFile };
  const byText = docs.filter((doc) => [doc.topic, doc.title, doc.summary].some((text) => text.toLowerCase().includes(word)));
  return { kind: byText.length > 0 ? 'text' : 'none', matches: byText };
}
