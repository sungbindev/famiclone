/*
 * ------------------------------------------------------------------------
 * 名称：check-modules.test.ts
 * 说明：分层与模块检查的测试：本仓库通过检查；在临时项目里构造违规，检查硬件模块之间的引用、模块只能引用对方契约、层间依赖与模块登记都能被发现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, describe, expect, it } from 'vitest';
import { checkProject } from '../tools/check/check-modules.mjs';

const REPO_ROOT = fileURLToPath(new URL('..', import.meta.url));
const roots: string[] = [];

/** 创建临时项目：文件按相对路径给出内容。 */
function createProject(files: Record<string, string>): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-check-'));
  roots.push(root);
  for (const [path, content] of Object.entries(files)) {
    mkdirSync(dirname(join(root, path)), { recursive: true });
    writeFileSync(join(root, path), content, 'utf8');
  }
  return root;
}

afterEach(() => {
  for (const root of roots.splice(0)) rmSync(root, { recursive: true, force: true });
});

describe('checkProject', () => {
  it('本仓库通过检查', () => {
    expect(checkProject(REPO_ROOT).errors).toEqual([]);
  });

  it('硬件模块经 contracts/hardware 协作且不要求登记', () => {
    const root = createProject({
      'core/src/contracts/hardware/bus.hpp': '#pragma once\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#include "contracts/hardware/bus.hpp"\n',
      'core/src/logic/hardware/ppu-2c02/ppu.hpp': '#include "contracts/hardware/bus.hpp"\n'
    });
    expect(checkProject(root).errors).toEqual([]);
  });

  it('硬件模块之间不能互相引用，同族模块也一样', () => {
    const root = createProject({
      'core/src/logic/hardware/cpu-um6576/cpu.hpp': '#include "logic/hardware/ppu-um6576/ppu.hpp"\n',
      'core/src/logic/hardware/ppu-um6576/ppu.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/cpu-um6576/cpu.hpp：模块 hardware/cpu-um6576 只能引用模块 hardware/ppu-um6576 的契约，不能包含 logic/hardware/ppu-um6576/ppu.hpp']);
  });

  it('业务模块不能直接引用硬件模块的实现', () => {
    const root = createProject({
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#pragma once\n',
      'core/src/logic/runtime/runtime.hpp': '#include "logic/hardware/cpu-6502/cpu.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/runtime/runtime.hpp：模块 runtime 只能引用模块 hardware/cpu-6502 的契约，不能包含 logic/hardware/cpu-6502/cpu.hpp']);
  });

  it('C++ 各层与第三方头文件的位置受限', () => {
    const root = createProject({
      'core/src/logic/system/a.hpp': '#include "infra/mappers/b.hpp"\n#include <sqlite3.h>\n',
      'core/src/shared/c.hpp': '#include "platform/requests/cancellation.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"system", registerSystem},\n',
      'core/src/bootstrap/modules/system.cpp': '\n'
    });
    const { errors } = checkProject(root);
    expect(errors).toContain('core/src/logic/system/a.hpp：logic 层不能包含 infra 层的 infra/mappers/b.hpp');
    expect(errors).toContain('core/src/logic/system/a.hpp：不能包含 <sqlite3.h>');
    expect(errors).toContain('core/src/shared/c.hpp：shared 层不能包含 platform 层的 platform/requests/cancellation.hpp');
  });

  it('TypeScript 的层间依赖、electron 与第三方包受限', () => {
    const root = createProject({
      'src/ui/shell/Shell.tsx': "import { app } from 'electron';\nimport { run } from '../../infra/core-client/run';\nimport { readFileSync } from 'node:fs';\nimport lodash from 'lodash';\n",
      'src/infra/core-client/run.ts': 'export const run = 1;\n'
    });
    expect(checkProject(root).errors).toEqual([
      'src/ui/shell/Shell.tsx：ui 层不能引入 electron',
      'src/ui/shell/Shell.tsx：ui 层不能引用 infra 层的 ../../infra/core-client/run',
      'src/ui/shell/Shell.tsx：ui 层不能使用 Node 内置模块 node:fs',
      'src/ui/shell/Shell.tsx：ui 层不能引用第三方包 lodash'
    ]);
  });

  it('发现没有登记的模块', () => {
    const root = createProject({
      'schema/demo.schema.json': JSON.stringify({ module: 'demo', types: {}, requests: {}, events: {} }),
      'src/ui/demo/Page.tsx': 'export const Page = 1;\n',
      'src/bootstrap/renderer/modules/demo.ts': 'export const x = 1;\n',
      'src/bootstrap/renderer/module-list.ts': 'export const list = [];\n'
    });
    expect(checkProject(root).errors).toEqual([
      '模块 demo 有 Schema，但没有登记在 core/src/bootstrap/module-list.cpp',
      '模块 demo 有 Schema，但没有登记在 src/bootstrap/main/module-list.ts',
      '模块 demo 有 src/ui 目录，但没有登记在 src/bootstrap/renderer/module-list.ts',
      'src/bootstrap/renderer/modules/demo.ts 没有登记在上一级目录的 module-list.ts'
    ]);
  });

  it('发现模块之间的循环依赖', () => {
    const root = createProject({
      'schema/a.schema.json': JSON.stringify({ module: 'a', types: { X: { type: 'object', properties: { b: { $ref: 'b#/types/Y' } } } }, requests: {}, events: {} }),
      'schema/b.schema.json': JSON.stringify({ module: 'b', types: { Y: { type: 'object', properties: { a: { $ref: 'a#/types/X' } } } }, requests: {}, events: {} })
    });
    expect(checkProject(root).errors).toContain('模块之间有循环依赖：a → b → a');
  });
});
