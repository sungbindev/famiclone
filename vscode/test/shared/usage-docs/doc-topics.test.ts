// ------------------------------------------------------------------------
// 名称：doc-topics.test.ts
// 说明：测试使用文档主题：登记表的解析与检查、本机路径、按主题名、文件名、关键词查找。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用内存里的登记表测试，真实登记表与 docs 目录的一致性见 test/logic/usage-docs。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { describeDoc, formatDocList, parseTopics, queryDocs, type DocInfo } from '../../../src/shared/usage-docs/doc-topics';

const DOCS = path.resolve('ext', 'docs');
const EXTENSION = path.resolve('ext');

const TOPICS_TEXT = JSON.stringify({
  topics: [
    { topic: 'build', file: '01-构建.md', title: '构建', summary: '任务与构建规则' },
    { topic: 'syscall', file: 'sb2000/02-系统调用手册.md', title: '系统调用手册', summary: '参数与出错码' },
    { topic: 'api', file: 'targets/sb2000/api-reference.md', root: 'extension', title: 'C 接口参考', summary: '全部函数' }
  ]
});

/** 只含一项的登记表，字段可以用 item 覆盖。 */
function one(item: Record<string, unknown>): string {
  return JSON.stringify({ topics: [{ topic: 'a', file: 'a.md', title: 'A', summary: 'a', ...item }] });
}

describe('登记表的解析', () => {
  it('保持登记顺序，root 为 extension 的项带上标记', () => {
    const topics = parseTopics(TOPICS_TEXT, 'topics.json');
    expect(topics.map((item) => item.topic)).toEqual(['build', 'syscall', 'api']);
    expect(topics[2]?.root).toBe('extension');
    expect(topics[0]?.root).toBeUndefined();
  });

  it('缺字段、主题名不合法、主题名重复、root 不认识都报错', () => {
    expect(() => parseTopics(one({ title: '' }), 'f')).toThrow(/title/);
    expect(() => parseTopics(one({ topic: 'Bad Name' }), 'f')).toThrow(/小写字母/);
    expect(() => parseTopics(one({ root: 'docs' }), 'f')).toThrow(/root/);
    expect(() => parseTopics(JSON.stringify({ topics: [JSON.parse(one({})).topics[0], JSON.parse(one({})).topics[0]] }), 'f')).toThrow(/重复/);
    expect(() => parseTopics('{}', 'f')).toThrow(/topics/);
    expect(() => parseTopics('[', 'f')).toThrow(expect.objectContaining({ code: 'JSON_INVALID' }));
  });
});

describe('查找', () => {
  const docs: DocInfo[] = parseTopics(TOPICS_TEXT, 'topics.json').map((entry) => describeDoc(entry, DOCS, EXTENSION));

  it('docs 目录里的文档与扩展目录里的文档分别补全本机路径', () => {
    expect(docs[1]?.localPath).toBe(path.join(DOCS, 'sb2000', '02-系统调用手册.md'));
    expect(docs[2]?.localPath).toBe(path.join(EXTENSION, 'targets', 'sb2000', 'api-reference.md'));
  });

  it('主题名先于文件名先于关键词', () => {
    expect(queryDocs(docs, 'build').kind).toBe('topic');
    expect(queryDocs(docs, '01-构建').matches.map((doc) => doc.topic)).toEqual(['build']);
    expect(queryDocs(docs, '系统调用手册')).toMatchObject({ kind: 'file' });
    expect(queryDocs(docs, '出错码')).toMatchObject({ kind: 'text' });
    expect(queryDocs(docs, 'zzzz_not_exist').kind).toBe('none');
  });

  it('列表每份文档一行，主题名对齐', () => {
    expect(formatDocList(docs).split('\n')).toEqual(['  build    构建：任务与构建规则', '  syscall  系统调用手册：参数与出错码', '  api      C 接口参考：全部函数']);
  });
});
