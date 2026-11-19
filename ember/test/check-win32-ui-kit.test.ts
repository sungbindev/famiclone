/*
 * ------------------------------------------------------------------------
 * 名称：check-win32-ui-kit.test.ts
 * 说明：Win32 控件库结构检查的测试：真实目录没有违规；在临时目录里逐条制造违规，检查都能发现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, describe, expect, it } from 'vitest';
import { checkUiKit } from '../tools/check/check-win32-ui-kit.mjs';

const REAL_ROOT = join(dirname(fileURLToPath(import.meta.url)), '..', 'win32', 'ui-kit');
const roots: string[] = [];

afterEach(() => {
  for (const root of roots.splice(0)) rmSync(root, { recursive: true, force: true });
});

/** 创建临时控件库：files 是相对路径到内容；CMakeLists.txt 默认登记所有 .cpp。 */
function createKit(files: Record<string, string>): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-ui-kit-check-'));
  roots.push(root);
  const cppFiles = Object.keys(files).filter((name) => name.endsWith('.cpp'));
  const all = { 'CMakeLists.txt': `add_library(x ${cppFiles.join(' ')})`, ...files };
  for (const [name, content] of Object.entries(all)) {
    const path = join(root, name);
    mkdirSync(dirname(path), { recursive: true });
    writeFileSync(path, content, 'utf8');
  }
  return root;
}

describe('Win32 控件库结构检查', () => {
  it('真实的控件库没有违规', () => {
    expect(checkUiKit(REAL_ROOT)).toEqual([]);
  });

  it('合规的结构没有违规', () => {
    const root = createKit({
      'include/rujian/ui/button.hpp': '#include <string>\n#include "rujian/ui/widget.hpp"\n',
      'src/base/graphics.hpp': '#include <d2d1.h>\n#include "rujian/ui/canvas.hpp"\n',
      'src/components/button/button.cpp': '#include "rujian/ui/button.hpp"\n#include "base/theme-tokens.hpp"\n#include "components/button/helper.hpp"\n'
    });
    expect(checkUiKit(root)).toEqual([]);
  });

  it('公共头文件引用 src 或系统图形头文件时报错', () => {
    const root = createKit({ 'include/rujian/ui/a.hpp': '#include "base/graphics.hpp"\n#include <d2d1.h>\n#include <windows.h>\n' });
    const problems = checkUiKit(root);
    expect(problems).toHaveLength(3);
    expect(problems.join('\n')).toContain('不能引用 base/graphics.hpp');
  });

  it('base 引用控件时报错', () => {
    const root = createKit({ 'src/base/x.cpp': '#include "components/button/button.hpp"\n' });
    expect(checkUiKit(root).join('\n')).toContain('base 不能引用控件');
  });

  it('控件引用别的控件、base 内部文件或系统图形头文件时报错', () => {
    const root = createKit({
      'src/components/button/button.cpp': '#include "components/input/input.hpp"\n#include "base/graphics.hpp"\n#include <d2d1.h>\n#include <wrl/client.h>\n'
    });
    const problems = checkUiKit(root);
    expect(problems).toHaveLength(4);
    expect(problems.join('\n')).toContain('不能引用 components/input/input.hpp');
    expect(problems.join('\n')).toContain('不能直接引用系统头文件 <d2d1.h>');
  });

  it('源文件没有登记到 CMakeLists.txt 时报错', () => {
    const root = createKit({ 'src/base/a.cpp': '', 'CMakeLists.txt': 'add_library(x)' });
    expect(checkUiKit(root)).toEqual(['src/base/a.cpp：没有在 CMakeLists.txt 登记']);
  });
});
