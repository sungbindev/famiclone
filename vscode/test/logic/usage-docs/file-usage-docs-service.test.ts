// ------------------------------------------------------------------------
// 名称：file-usage-docs-service.test.ts
// 说明：测试使用文档服务与文档登记：列出主题、按主题名与关键词查询、
//       读取正文，以及登记表与 docs 目录、架构文档清单一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：读取扩展目录里真实的文档，不需要 cc65 与模拟器。
// ------------------------------------------------------------------------

import { existsSync, readFileSync, readdirSync, statSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../../src/infra/resources/extension-resources-factory';
import { FileUsageDocsService } from '../../../src/logic/usage-docs/file-usage-docs-service';
import { EXTENSION_DIRECTORY, createTempDir, fakeResources, type TempDir } from '../../support/build-fixtures';

const resources = createExtensionResources(EXTENSION_DIRECTORY);
const service = new FileUsageDocsService(new NodeFileSystem(), resources);

/**
 * 列出目录下全部 Markdown 文件，路径相对 root，用 / 分隔。
 * @param root 根目录。
 * @param directory 当前目录。
 * @returns 文件路径。
 */
function listMarkdown(root: string, directory: string = root): string[] {
  return readdirSync(directory).flatMap((name) => {
    const full = path.join(directory, name);
    if (statSync(full).isDirectory()) return listMarkdown(root, full);
    return name.endsWith('.md') ? [path.relative(root, full).split(path.sep).join('/')] : [];
  });
}

describe('使用文档服务', () => {
  it('列出全部主题，本机路径都存在', () => {
    const docs = service.list();
    expect(docs.length).toBeGreaterThan(5);
    for (const doc of docs) expect(existsSync(doc.localPath), doc.localPath).toBe(true);
  });

  it('按主题名查询，读出正文；api 主题指向目标目录里的接口参考', () => {
    const { matches } = service.query('syscall');
    expect(matches).toHaveLength(1);
    expect(matches[0]?.localPath).toBe(path.join(resources.docsDirectory, 'sb2000', '02-系统调用手册.md'));
    expect(service.read(matches[0]!)).toMatch(/^# /);
    expect(service.query('api').matches[0]?.localPath).toBe(path.join(resources.targetsDirectory, 'sb2000', 'api-reference.md'));
  });

  it('关键词匹配到多份文档，找不到时为空', () => {
    expect(service.query('运行').matches.length).toBeGreaterThan(1);
    expect(service.query('zzzz_not_exist').matches).toEqual([]);
  });
});

describe('文档主题登记', () => {
  const docs = service.list();

  it('docs 目录里的登记项与目录里的 Markdown 文件一一对应', () => {
    const registered = docs.filter((doc) => doc.root !== 'extension').map((doc) => doc.file).toSorted();
    expect(registered).toEqual(listMarkdown(resources.docsDirectory).toSorted());
  });

  it('架构文档的使用文档清单列出了每个主题与文件', () => {
    const text = readFileSync(path.join(EXTENSION_DIRECTORY, '..', 'docs', '01-软件架构设计.md'), 'utf8');
    for (const { topic, file } of docs) {
      expect(text, `文档清单缺少主题 ${topic}`).toContain(`| \`${topic}\` |`);
      expect(text, `文档清单缺少 ${file}`).toContain(path.posix.basename(file, '.md'));
    }
  });
});

describe('文档缺失', () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('没有登记表时报错并提示重新安装', () => {
    const empty = new FileUsageDocsService(new NodeFileSystem(), fakeResources(temp.dir, { docsDirectory: temp.dir }));
    expect(() => empty.list()).toThrow(expect.objectContaining({ code: 'DOCS_MISSING' }));
  });
});
