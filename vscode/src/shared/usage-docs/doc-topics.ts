// ------------------------------------------------------------------------
// 名称：doc-topics.ts
// 说明：使用文档的主题：登记表的解析、补全本机路径、按主题名、文件名或关键词查找。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：登记表是 vscode/docs/topics.json，扩展与 Ember 帮助站点的导入脚本读同一份；
//       主题名只用小写字母、数字与连字符，便于输入。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import { isRecord, parseJsonText } from '../json/json-text';

/** 登记表里的一份文档。 */
export interface DocTopic {
  /** 主题名。 */
  readonly topic: string;
  /** 文件路径，用 / 分隔；默认相对 docs 目录，root 为 extension 时相对扩展目录。 */
  readonly file: string;
  /** 文件不在 docs 目录里、而在扩展目录的其他位置（如目标目录里由头文件生成的接口参考）。 */
  readonly root?: 'extension';
  /** 标题。 */
  readonly title: string;
  /** 一句话说明。 */
  readonly summary: string;
}

/** 带本机路径的文档信息。 */
export interface DocInfo extends DocTopic {
  /** 本机的绝对路径。 */
  readonly localPath: string;
}

/** 查找的匹配方式。 */
export type DocQueryKind = 'topic' | 'file' | 'text' | 'none';

/** 查找结果。 */
export interface DocQueryResult {
  /** 匹配方式。 */
  readonly kind: DocQueryKind;
  /** 匹配到的文档。 */
  readonly matches: readonly DocInfo[];
}

/**
 * 解析并检查主题登记表。
 * @param text topics.json 的内容。
 * @param file 登记表路径，用在出错提示里。
 * @returns 主题列表，保持登记顺序。
 * @throws FcdevError 登记表不合法。
 */
export function parseTopics(text: string, file: string): DocTopic[] {
  const data = parseJsonText(text, file);
  const items = isRecord(data) ? data.topics : undefined;
  if (!Array.isArray(items)) throw new FcdevError(`${file} 里没有 topics 数组`, { code: 'DOCS_INVALID' });
  const seen = new Set<string>();
  return items.map((item: unknown, index) => {
    const where = `${file} 的第 ${index + 1} 项`;
    if (!isRecord(item)) throw new FcdevError(`${where}必须是对象`, { code: 'DOCS_INVALID' });
    for (const field of ['topic', 'file', 'title', 'summary']) {
      if (typeof item[field] !== 'string' || item[field] === '') throw new FcdevError(`${where}缺少字符串字段 ${field}`, { code: 'DOCS_INVALID' });
    }
    const topic = item.topic as string;
    if (!/^[a-z0-9-]+$/.test(topic)) throw new FcdevError(`${where}的主题名只能用小写字母、数字与连字符：${topic}`, { code: 'DOCS_INVALID' });
    if (seen.has(topic)) throw new FcdevError(`${where}的主题名重复：${topic}`, { code: 'DOCS_INVALID' });
    seen.add(topic);
    if (item.root !== undefined && item.root !== 'extension') throw new FcdevError(`${where}的 root 只能是 extension`, { code: 'DOCS_INVALID' });
    return {
      topic,
      file: item.file as string,
      ...(item.root === 'extension' ? { root: 'extension' as const } : {}),
      title: item.title as string,
      summary: item.summary as string
    };
  });
}

/**
 * 把登记项补全成带本机路径的文档信息。
 * @param entry 登记项。
 * @param docsDirectory docs 目录。
 * @param extensionDirectory 扩展目录。
 * @returns 文档信息。
 */
export function describeDoc(entry: DocTopic, docsDirectory: string, extensionDirectory: string): DocInfo {
  const base = entry.root === 'extension' ? extensionDirectory : docsDirectory;
  return { ...entry, localPath: path.join(base, ...entry.file.split('/')) };
}

/**
 * 文件名去掉序号前缀与扩展名后的部分，如 sb2000/02-系统调用手册.md 得到“系统调用手册”。
 * @param file 文件路径。
 * @returns 名称。
 */
function fileLabel(file: string): string {
  return path.posix.basename(file, '.md').replace(/^\d+-/, '');
}

/**
 * 按词查找文档：先按主题名精确匹配，再按文件名，最后按主题、标题与说明里的关键词。
 * @param docs 全部文档。
 * @param query 查询词，不分大小写。
 * @returns 匹配方式与结果。
 */
export function queryDocs(docs: readonly DocInfo[], query: string): DocQueryResult {
  const word = query.trim().toLowerCase();
  const byTopic = docs.filter((doc) => doc.topic === word);
  if (byTopic.length > 0) return { kind: 'topic', matches: byTopic };
  const byFile = docs.filter((doc) => path.posix.basename(doc.file, '.md').toLowerCase() === word || fileLabel(doc.file).toLowerCase() === word);
  if (byFile.length > 0) return { kind: 'file', matches: byFile };
  const byText = docs.filter((doc) => [doc.topic, doc.title, doc.summary].some((text) => text.toLowerCase().includes(word)));
  return { kind: byText.length > 0 ? 'text' : 'none', matches: byText };
}

/**
 * 格式化文档列表：每份文档一行。
 * @param docs 文档列表。
 * @returns 多行文字。
 */
export function formatDocList(docs: readonly DocInfo[]): string {
  const width = Math.max(...docs.map((doc) => doc.topic.length));
  return docs.map((doc) => `  ${doc.topic.padEnd(width)}  ${doc.title}：${doc.summary}`).join('\n');
}
