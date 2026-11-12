// ------------------------------------------------------------------------
// 名称：structure.test.mjs
// 说明：测试分层依赖检查：合规、越层引用、绕过 index.mjs、未登记目录都要被识别。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用临时目录构造小型 src 树，不依赖主包的真实代码。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { checkImports } from '../tools/check-structure.mjs';

describe('分层依赖检查', () => {
  let root;

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'fcdev-structure-'));
  });

  afterEach(() => {
    rmSync(root, { recursive: true, force: true });
  });

  /** 写一个文件，自动创建目录 */
  function write(relativePath, content) {
    const full = path.join(root, relativePath);
    mkdirSync(path.dirname(full), { recursive: true });
    writeFileSync(full, content);
  }

  it('合规的引用没有违规', () => {
    write('shared/index.mjs', 'export const a = 1;\n');
    write('config/index.mjs', "import { a } from '../shared/index.mjs';\nexport const b = a;\n");
    assert.deepEqual(checkImports(root), []);
  });

  it('引用不在依赖表里的目录要报违规', () => {
    write('shared/index.mjs', "import { b } from '../config/index.mjs';\nexport const a = b;\n");
    write('config/index.mjs', 'export const b = 1;\n');
    const violations = checkImports(root);
    assert.equal(violations.length, 1);
    assert.match(violations[0], /shared\/index\.mjs：不允许引用 config/);
  });

  it('绕过 index.mjs 直接引用其他目录的内部文件要报违规', () => {
    write('shared/inner.mjs', 'export const a = 1;\n');
    write('config/index.mjs', "import { a } from '../shared/inner.mjs';\nexport const b = a;\n");
    const violations = checkImports(root);
    assert.equal(violations.length, 1);
    assert.match(violations[0], /必须经 shared\/index\.mjs 引用/);
  });

  it('同一目录内部的引用不受限制', () => {
    write('shared/index.mjs', "export { a } from './inner.mjs';\n");
    write('shared/inner.mjs', 'export const a = 1;\n');
    assert.deepEqual(checkImports(root), []);
  });

  it('没有登记的目录要报违规', () => {
    write('unknown/index.mjs', 'export const a = 1;\n');
    const violations = checkImports(root);
    assert.deepEqual(violations, ['unknown：目录没有在依赖表中登记']);
  });
});
