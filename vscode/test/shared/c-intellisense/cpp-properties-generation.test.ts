// ------------------------------------------------------------------------
// 名称：cpp-properties-generation.test.ts
// 说明：测试 C/C++ 配置的生成：合并进已有的 c_cpp_properties.json、
//       compile_flags.txt 的内容与归属标记。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：配置名的解析见 test/shared/cpp-properties.test.ts。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { buildCompileFlags, isGeneratedCompileFlags, mergeCppProperties } from '../../../src/shared/c-intellisense/cpp-properties';
import type { ProjectDescription } from '../../../src/shared/project/project-info';

const DESCRIPTION: ProjectDescription = {
  root: 'p',
  configFile: 'p/fcdev.json',
  targetName: 'sb2000',
  targetTitle: 'SB2000',
  toolchainHome: 'tc',
  includeDirs: ['a/include', 'tc/include'],
  asmIncludeDirs: [],
  defines: { __fastcall__: '', __SB2000__: 1 },
  outDir: 'p/build'
};

describe('合并 c_cpp_properties.json', () => {
  it('没有文件时新建，只含 FC Dev 配置', () => {
    const document = JSON.parse(mergeCppProperties(undefined, 'f', DESCRIPTION)) as { version: number; configurations: Array<Record<string, unknown>> };
    expect(document.version).toBe(4);
    expect(document.configurations).toEqual([
      { name: 'fcdev', includePath: ['a/include', 'tc/include'], defines: ['__fastcall__=', '__SB2000__=1'], cStandard: 'c89', intelliSenseMode: 'clang-x86' }
    ]);
  });

  it('已有 FC Dev 配置时替换，其他配置保留', () => {
    const existing = JSON.stringify({ version: 4, configurations: [{ name: 'Win32' }, { name: 'fcdev', includePath: ['旧'] }] });
    const document = JSON.parse(mergeCppProperties(existing, 'f', DESCRIPTION)) as { configurations: Array<{ name: string; includePath?: string[] }> };
    expect(document.configurations.map((item) => item.name)).toEqual(['Win32', 'fcdev']);
    expect(document.configurations[1]?.includePath).toEqual(['a/include', 'tc/include']);
  });

  it('没有 FC Dev 配置时追加', () => {
    const document = JSON.parse(mergeCppProperties(JSON.stringify({ configurations: [{ name: 'Win32' }] }), 'f', DESCRIPTION)) as { configurations: unknown[] };
    expect(document.configurations).toHaveLength(2);
  });

  it('已有文件不是合法的配置时报错', () => {
    expect(() => mergeCppProperties('{', 'f', DESCRIPTION)).toThrow(expect.objectContaining({ code: 'SYNC_FILE_INVALID' }));
    expect(() => mergeCppProperties('{}', 'f', DESCRIPTION)).toThrow(/configurations/);
  });
});

describe('compile_flags.txt', () => {
  it('第一行是生成标记，之后是 -I 与 -D 行', () => {
    expect(buildCompileFlags(DESCRIPTION)).toBe('-DFCDEV_GENERATED\n-Ia/include\n-Itc/include\n-D__fastcall__=\n-D__SB2000__=1\n');
  });

  it('只有第一行是标记的文件才算生成的', () => {
    expect(isGeneratedCompileFlags(buildCompileFlags(DESCRIPTION))).toBe(true);
    expect(isGeneratedCompileFlags('-Imine\n')).toBe(false);
  });
});
