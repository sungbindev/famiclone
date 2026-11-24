// ------------------------------------------------------------------------
// 名称：file-scaffold-service.test.ts
// 说明：测试新建项目：七个模板、生成的文件、目录检查，以及用真实 cc65 构建七个模板。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：构建部分在本机没有组装工具链时跳过。
// ------------------------------------------------------------------------

import { existsSync, mkdirSync, readFileSync, statSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FileScaffoldService } from '../../../src/logic/scaffold/file-scaffold-service';
import { HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, type TempDir } from '../../support/build-fixtures';

/** 真实构建较慢，用例的超时时间。 */
const TIMEOUT_MS = 120_000;

const { service: buildService, fileSystem, resources } = createRealBuildEnvironment();
const scaffold = new FileScaffoldService(fileSystem, resources);

/** 读出项目 tasks.json 里各任务的命令。 */
function commandsOf(directory: string): unknown[] {
  return (JSON.parse(readFileSync(path.join(directory, '.vscode', 'tasks.json'), 'utf8')) as { tasks: Array<{ command: string }> }).tasks.map((task) => task.command);
}

describe('模板', () => {
  it('有七个模板：nes 与 sb2000 各有 c 和 asm，sb97、yuxing 与 bbg 只有 asm', () => {
    expect(scaffold.listTemplates().map((item) => item.name)).toEqual(['bbg-asm', 'nes-asm', 'nes-c', 'sb2000-asm', 'sb2000-c', 'sb97-asm', 'yuxing-asm']);
  });

  it('每个模板都有示例源码与说明', () => {
    for (const template of scaffold.listTemplates()) {
      const source = template.language === 'c' ? 'src/hello.c' : 'src/hello.s';
      expect(existsSync(path.join(resources.templatesDirectory, template.name, source)), `${template.name} 缺少 ${source}`).toBe(true);
      expect(existsSync(path.join(resources.templatesDirectory, template.name, 'README.md')), `${template.name} 缺少 README.md`).toBe(true);
    }
  });
});

describe('生成项目', () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('生成源码、配置、任务、README 与 .gitignore，不复制 template.json，学习机模板带 disk/README.TXT', () => {
    const directory = path.join(temp.dir, 'my-game');
    const created = scaffold.create({ directory, target: 'sb2000', language: 'c' });
    expect(created.files).toEqual(['.gitignore', '.vscode/tasks.json', 'README.md', 'disk/README.TXT', 'fcdev.json', 'src/hello.c']);
    expect(readFileSync(path.join(directory, 'README.md'), 'utf8')).toMatch(/^# my-game\r?\n/);
    expect(readFileSync(path.join(directory, '.gitignore'), 'utf8')).toMatch(/compile_flags\.txt/);
    expect(JSON.parse(readFileSync(path.join(directory, 'fcdev.json'), 'utf8'))).toEqual({ target: 'sb2000', name: 'hello' });
  });

  it('模板里的 disk/README.TXT 是 UTF-8 带 BOM、CRLF 换行，按字节原样复制', () => {
    for (const [name, target] of [['a', 'sb2000'], ['b', 'sb97'], ['c', 'yuxing']] as const) {
      const directory = path.join(temp.dir, name);
      scaffold.create({ directory, target, language: 'asm' });
      const bytes = readFileSync(path.join(directory, 'disk', 'README.TXT'));
      expect([...bytes.subarray(0, 3)]).toEqual([0xef, 0xbb, 0xbf]);
      expect(bytes.toString('utf8')).toMatch(/\r\n/);
      expect(bytes.toString('utf8')).not.toMatch(/[^\r]\n/);
    }
  });

  it('bbg 模板没有 disk 目录：磁盘镜像建在用户指定的系统盘上，空间有限', () => {
    const directory = path.join(temp.dir, 'bbg');
    const { files } = scaffold.create({ directory, target: 'bbg', language: 'asm' });
    expect(files).toEqual(['.gitignore', '.vscode/tasks.json', 'README.md', 'fcdev.json', 'src/hello.s']);
    expect(JSON.parse(readFileSync(path.join(directory, 'fcdev.json'), 'utf8'))).toEqual({ target: 'bbg', name: 'hello' });
  });

  it('所有模板的任务都是构建与运行', () => {
    scaffold.create({ directory: path.join(temp.dir, 'a'), target: 'sb2000', language: 'asm' });
    scaffold.create({ directory: path.join(temp.dir, 'b'), target: 'nes', language: 'c' });
    expect(commandsOf(path.join(temp.dir, 'a'))).toEqual(['build', 'run']);
    expect(commandsOf(path.join(temp.dir, 'b'))).toEqual(['build', 'run']);
  });

  it('目录不存在时创建，已有 .git 的空目录可以用', () => {
    const directory = path.join(temp.dir, 'with-git');
    mkdirSync(path.join(directory, '.git'), { recursive: true });
    const { files } = scaffold.create({ directory, target: 'nes', language: 'asm' });
    expect(files).toContain('src/hello.s');
    expect(files.some((file) => file.startsWith('.git/'))).toBe(false);
  });

  it('目录里已有其他文件时报错，并且不动已有文件', () => {
    const directory = path.join(temp.dir, 'busy');
    mkdirSync(directory);
    writeFileSync(path.join(directory, 'notes.txt'), 'keep');
    expect(() => scaffold.create({ directory, target: 'nes', language: 'c' })).toThrow(
      expect.objectContaining({ code: 'DIRECTORY_NOT_EMPTY', hint: expect.stringContaining('notes.txt') })
    );
    expect(readFileSync(path.join(directory, 'notes.txt'), 'utf8')).toBe('keep');
    expect(existsSync(path.join(directory, 'fcdev.json'))).toBe(false);
  });

  it('没有这个目标与语言的模板时报错，列出可用的目标与语言', () => {
    expect(() => scaffold.create({ directory: path.join(temp.dir, 'x'), target: 'nope', language: 'c' })).toThrow(
      expect.objectContaining({ code: 'TEMPLATE_NOT_FOUND', hint: expect.stringMatching(/sb2000.*sb97.*yuxing.*asm/) })
    );
  });
});

describe.skipIf(!HAS_TOOLCHAIN)('七个模板都能构建', { timeout: TIMEOUT_MS }, () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it.each(scaffold.listTemplates().map((item) => item.name))('%s', async (name) => {
    const [target, language] = name.split('-') as [string, 'c' | 'asm'];
    const directory = path.join(temp.dir, name);
    scaffold.create({ directory, target, language });
    const { result, disk } = await buildService.build({ pathArg: 'src', cwd: directory });
    expect(statSync(result.outputFile).size).toBeGreaterThan(0);
    // 模板的 fcdev.json 没有配置 diskName，构建不生成镜像
    expect(disk).toBeUndefined();
  });
});
