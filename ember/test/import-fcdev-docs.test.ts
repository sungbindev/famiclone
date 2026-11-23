/*
 * ------------------------------------------------------------------------
 * 名称：import-fcdev-docs.test.ts
 * 说明：fcdev 手册导入脚本的测试：文档之间的链接改成站内页面、指向文档目录之外的链接变成普通文字、尖括号与双花括号只在代码之外转义、只写有变化的页面并删除不再登记的页面、侧边栏分组，以及真实手册导入后没有失效的页面链接。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：不运行 VitePress；真实手册的导入写到临时目录，不动 help/fcdev。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { convertDocument, DOC_TOPICS, groupTopics, importFcdevDocs, sidebarGroups, type FcdevDocTopic } from '../tools/build/import-fcdev-docs.mjs';

let temp = '';

beforeEach(() => {
  temp = mkdtempSync(join(tmpdir(), 'ember-fcdev-docs-'));
});

afterEach(() => {
  rmSync(temp, { recursive: true, force: true });
});

/** 在临时目录里造一个小的扩展目录：docs 里两篇、目标目录里一篇。 */
function makeFcdev(): { root: string; topics: FcdevDocTopic[] } {
  const root = join(temp, 'fcdev');
  mkdirSync(join(root, 'docs', 'sb2000'), { recursive: true });
  mkdirSync(join(root, 'targets', 'sb2000'), { recursive: true });
  writeFileSync(join(root, 'docs', '01-构建.md'), '# 构建\r\n\r\n见 [系统调用](sb2000/02-系统调用.md#第五节) 与 [参考](../targets/sb2000/api-reference.md)。\r\n');
  writeFileSync(join(root, 'docs', 'sb2000', '02-系统调用.md'), '# 系统调用\n\n回到 [构建](../01-构建.md)，头文件 [sb2000.h](../../targets/sb2000/include/sb2000.h)，外部 [站点](https://example.com/a.md)。\n');
  writeFileSync(join(root, 'targets', 'sb2000', 'api-reference.md'), '# 参考\n\n<a id="x"></a>\n## x\n');
  const topics: FcdevDocTopic[] = [
    { topic: 'build', file: '01-构建.md', title: '构建', summary: '构建说明' },
    { topic: 'syscall', file: 'sb2000/02-系统调用.md', title: '系统调用', summary: '调用说明' },
    { topic: 'api', file: 'targets/sb2000/api-reference.md', root: 'extension', title: '接口参考', summary: '函数 | 签名' }
  ];
  return { root, topics };
}

describe('convertDocument', () => {
  const sourceFile = resolve('/x/docs/a.md');
  const pageByFile = new Map([[resolve('/x/docs/b.md'), 'other']]);
  const convert = (text: string) => convertDocument({ text, sourceFile, relativeFile: 'docs/a.md', pageByFile });

  it('指向其他登记文档的链接改成站内页面并保留锚点，目录之外的链接变成普通文字，锚点与外部链接不变', () => {
    const page = convert('[乙](b.md#节) [头文件](../targets/x.h) [本页](#节) [外](https://example.com/a.md)\n');
    expect(page).toContain('[乙](other.md#节) 头文件 [本页](#节) [外](https://example.com/a.md)');
  });

  it('尖括号与双花括号只在代码之外转义，行内代码、代码块与锚点标签原样保留', () => {
    const page = convert('正文 <程序名> 与 {{x}}，代码 `<程序名> {{x}}`\n<a id="s01"></a>\n```c\n<a> {{b}}\n```\n');
    expect(page).toContain('正文 &lt;程序名> 与 &#123;&#123;x}}，代码 `<程序名> {{x}}`');
    expect(page).toContain('\n<a id="s01"></a>\n');
    expect(page).toContain('```c\n<a> {{b}}\n```');
  });

  it('去掉单独成节的“## 目录”，保留它后面的锚点与标题；代码块里的同名行与正文里的列表不动', () => {
    const page = convert('# 标题\n\n简介\n\n## 目录\n\n- [甲](#a)\n- [乙](#b)\n\n<a id="a"></a>\n## 甲\n\n- 正文列表\n\n```text\n## 目录\n- 代码里的\n```\n');
    expect(page).toContain('简介\n\n<a id="a"></a>\n## 甲\n\n- 正文列表');
    expect(page).not.toContain('[甲](#a)');
    expect(page).toContain('```text\n## 目录\n- 代码里的\n```');
    expect(page.match(/## 目录/g)).toHaveLength(1);
  });

  it('开头有“由导入生成”的标记，换行统一为 LF，结尾只有一个换行', () => {
    const page = convert('# 标题\r\n\r\n正文\r\n\r\n');
    expect(page.startsWith('<!-- 本文件由 npm run help:build 从 vscode/docs/a.md 导入，不要手改 -->\n\n# 标题\n')).toBe(true);
    expect(page).not.toContain('\r');
    expect(page.endsWith('正文\n')).toBe(true);
  });
});

describe('importFcdevDocs', () => {
  it('每个登记的主题一页加一个概述页；文档之间的链接指向站内页面', () => {
    const { root, topics } = makeFcdev();
    const output = join(temp, 'out');
    expect(importFcdevDocs({ topics, extensionRoot: root, outputDirectory: output })).toEqual({ pages: 4, written: 4 });
    expect(readdirSync(output).toSorted()).toEqual(['api.md', 'build.md', 'index.md', 'syscall.md']);
    const build = readFileSync(join(output, 'build.md'), 'utf8');
    expect(build).toContain('[系统调用](syscall.md#第五节)');
    expect(build).toContain('[参考](api.md)');
    expect(readFileSync(join(output, 'syscall.md'), 'utf8')).toContain('回到 [构建](build.md)，头文件 sb2000.h，外部 [站点](https://example.com/a.md)');
  });

  it('概述页按分组列出手册，表格里的竖线被转义', () => {
    const { root, topics } = makeFcdev();
    const output = join(temp, 'out');
    importFcdevDocs({ topics, extensionRoot: root, outputDirectory: output });
    const index = readFileSync(join(output, 'index.md'), 'utf8');
    expect(index).toContain('# fcdev 开发手册');
    expect(index).toContain('## 通用\n');
    expect(index).toContain('## SB2000（小霸王学习机）\n');
    expect(index).toContain('| [构建](build.md) | 构建说明 |');
    expect(index).toContain('| [接口参考](api.md) | 函数 \\| 签名 |');
  });

  it('再导入一次没有变化时不写文件；删除不再登记的页面', () => {
    const { root, topics } = makeFcdev();
    const output = join(temp, 'out');
    importFcdevDocs({ topics, extensionRoot: root, outputDirectory: output });
    writeFileSync(join(output, 'old.md'), '旧页面');
    expect(importFcdevDocs({ topics, extensionRoot: root, outputDirectory: output })).toEqual({ pages: 4, written: 0 });
    expect(existsSync(join(output, 'old.md'))).toBe(false);
  });

  it('登记的文档不存在时报错并写出路径', () => {
    const { root, topics } = makeFcdev();
    rmSync(join(root, 'docs', '01-构建.md'));
    expect(() => importFcdevDocs({ topics, extensionRoot: root, outputDirectory: join(temp, 'out') })).toThrow('找不到 fcdev 的文档');
  });
});

describe('分组与侧边栏', () => {
  it('分组顺序固定为通用、NES、SB2000、SB97、裕兴、步步高，空分组不出现，组内保持登记顺序', () => {
    const groups = groupTopics(DOC_TOPICS);
    expect(groups.map((group) => group.title)).toEqual(['通用', 'NES', 'SB2000（小霸王学习机）', 'SB97（小霸王 97）', '裕兴（学习机）', '步步高（学生电脑）']);
    expect(groups[0]?.topics.map((topic) => topic.topic)).toEqual(['install', 'build', 'emulator', 'disasm', 'create', 'rom']);
    expect(groupTopics([{ file: 'nes/01.md' }]).map((group) => group.title)).toEqual(['NES']);
  });

  it('侧边栏第一组是概述，其余每个登记的主题恰好出现一次，链接是 /fcdev/<主题>', () => {
    const sidebar = sidebarGroups();
    expect(sidebar[0]).toEqual({ text: 'fcdev 开发手册', items: [{ text: '概述', link: '/fcdev/' }] });
    const links = sidebar.slice(1).flatMap((group) => group.items.map((item) => item.link));
    expect(links.toSorted()).toEqual(DOC_TOPICS.map((topic) => `/fcdev/${topic.topic}`).toSorted());
  });
});

describe('真实的 fcdev 手册', () => {
  it('导入后每个页面链接都指向存在的页面，没有留下指向文档目录之外的相对链接', () => {
    const output = join(temp, 'real');
    importFcdevDocs({ outputDirectory: output });
    expect(readdirSync(output).toSorted()).toEqual(['index.md', ...DOC_TOPICS.map((topic) => `${topic.topic}.md`)].toSorted());
    for (const name of readdirSync(output)) {
      const text = readFileSync(join(output, name), 'utf8');
      expect(text, `${name} 里不应留下“## 目录”`).not.toContain('\n## 目录\n');
      for (const match of text.matchAll(/\]\(([^)\s#]+)(?:#[^)\s]*)?\)/g)) {
        const target = match[1] ?? '';
        if (/^[a-z][a-z0-9+.-]*:/i.test(target)) continue;
        expect(target, `${name} 里的链接 ${target}`).toMatch(/^[a-z0-9-]+\.md$/);
        expect(existsSync(join(output, target)), `${name} 里的链接 ${target} 指向的页面不存在`).toBe(true);
      }
    }
  });
});
