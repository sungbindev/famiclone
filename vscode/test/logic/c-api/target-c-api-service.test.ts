// ------------------------------------------------------------------------
// 名称：target-c-api-service.test.ts
// 说明：测试目标 C 接口服务：用内置 sb2000 目标的头文件列出模块、查函数、按系统调用号与关键词查询，
//       以及没有指定目标、目标没有文档时的错误。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：不需要 cc65 与模拟器。
// ------------------------------------------------------------------------

import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../../src/infra/resources/extension-resources-factory';
import { TargetCApiService } from '../../../src/logic/c-api/target-c-api-service';
import { FileProjectConfigReader } from '../../../src/logic/project/file-project-config-reader';
import { FileTargetRepository } from '../../../src/logic/targets/file-target-repository';
import { formatFunction, formatModules, groupByModule, queryApi } from '../../../src/shared/c-api/api-functions';
import { EXTENSION_DIRECTORY, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

describe('目标 C 接口', () => {
  const fileSystem = new NodeFileSystem();
  const service = new TargetCApiService(
    fileSystem,
    new FileProjectConfigReader(fileSystem),
    new FileTargetRepository(fileSystem, createExtensionResources(EXTENSION_DIRECTORY))
  );
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('列出 sb2000 的全部模块与函数个数', () => {
    const api = service.describe({ cwd: temp.dir, target: 'sb2000' });
    const text = formatModules(api.targetTitle, groupByModule(api.functions));
    expect(text).toMatch(/个函数，\d+ 个模块/);
    for (const name of ['file', 'disk', 'display', 'math', 'sound', 'printer', 'ime']) expect(text).toMatch(new RegExp(`\\b${name}\\b`));
  });

  it('按函数名显示完整文档：签名、说明、参数、返回、系统调用', () => {
    const { functions } = service.describe({ cwd: temp.dir, target: 'sb2000' });
    const text = queryApi(functions, 'sb_file_open').matches.map(formatFunction).join('\n');
    expect(text).toMatch(/unsigned char sb_file_open\(const char \*path, unsigned char mode, unsigned char \*handle\);/);
    expect(text).toMatch(/系统调用：\$7B\/\$09/);
    expect(text).toMatch(/handle {2}返回句柄号/);
    expect(text).toMatch(/返回：0 成功/);
  });

  it('按模块名、头文件名、系统调用号与关键词查询', () => {
    const { functions } = service.describe({ cwd: temp.dir, target: 'sb2000' });
    const names = (query: string): string[] => queryApi(functions, query).matches.map((fn) => fn.name);
    expect(names('disk')).toContain('sb_disk_read_sector');
    expect(names('sb2000-disk.h')).toContain('sb_disk_format_track');
    expect(names('$80/$02')).toEqual(['sb_disk_read_sector']);
    expect(names('$7b')).toContain('sb_file_open');
    expect(names('格式化')).toContain('sb_disk_format_track');
  });

  it('函数的参数与系统调用来自头文件', () => {
    const { functions } = service.describe({ cwd: temp.dir, target: 'sb2000' });
    const [fn] = queryApi(functions, 'sb_clock_get_time').matches;
    expect(fn?.params).toEqual(['time']);
    expect(fn?.syscalls).toEqual(['$7F/$04']);
  });

  it('目标取自 fcdev.json；没有指定目标、目标没有文档时报错', () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000' }));
    expect(service.describe({ cwd: temp.dir }).targetName).toBe('sb2000');
    const other = createTempDir();
    try {
      expect(() => service.describe({ cwd: other.dir })).toThrow(expect.objectContaining({ code: 'TARGET_MISSING' }));
    } finally {
      other.cleanup();
    }
    expect(() => service.describe({ cwd: temp.dir, target: 'nes' })).toThrow(expect.objectContaining({ code: 'API_MISSING' }));
  });
});
