// ------------------------------------------------------------------------
// 名称：cc65-tools.test.mjs
// 说明：测试 cc65 源码目录与已构建的工具链包：必要路径、工具链包文件的校验值、包的版本。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：工具链包校验在工具链包已构建时才运行。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it } from 'node:test';
import { CC65_COMMIT, TOOLCHAIN_PACKAGE_DIR_NAME } from '../tools/build-cc65.mjs';

const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

describe('入库的 cc65 源码', () => {
  it('构建工具与运行库需要的路径都在 cc65/ 里', () => {
    const required = ['LICENSE', 'README-FCDEV.txt', 'src/cc65', 'src/ca65', 'src/ld65', 'src/ar65', 'src/da65', 'libsrc/none', 'libsrc/nes', 'include', 'asminc', 'cfg/none.cfg', 'cfg/nes.cfg', 'cfg/module.cfg'];
    for (const relative of required) assert.ok(existsSync(path.join(REPO_ROOT, 'cc65', relative)), `缺少 ${relative}`);
  });

  it('说明文件写明是删减版并给出导入的提交', () => {
    const notice = readFileSync(path.join(REPO_ROOT, 'cc65', 'README-FCDEV.txt'), 'utf8');
    assert.match(notice, /删减版/);
    assert.ok(notice.includes(CC65_COMMIT));
  });
});

const packageDir = path.join(REPO_ROOT, 'packages', TOOLCHAIN_PACKAGE_DIR_NAME);

describe('cc65 工具链包', { skip: existsSync(path.join(packageDir, 'manifest.json')) ? false : '工具链包尚未构建（运行 node tools/build-cc65.mjs）' }, () => {
  it('manifest.json 记录的每个文件都存在且校验值一致', () => {
    const manifest = JSON.parse(readFileSync(path.join(packageDir, 'manifest.json'), 'utf8'));
    assert.equal(manifest.commit, CC65_COMMIT);
    for (const [relative, expected] of Object.entries(manifest.files)) {
      const actual = createHash('sha256').update(readFileSync(path.join(packageDir, relative))).digest('hex');
      assert.equal(actual, expected, relative);
    }
  });

  it('包含 none 与 nes 的运行库、两个链接配置与许可文件', () => {
    for (const relative of ['lib/none.lib', 'lib/nes.lib', 'cfg/none.cfg', 'cfg/nes.cfg', 'cfg/module.cfg', 'LICENSE']) {
      assert.ok(existsSync(path.join(packageDir, relative)), relative);
    }
  });

  it('package.json 的版本与主包一致，主包依赖了它', () => {
    const document = JSON.parse(readFileSync(path.join(packageDir, 'package.json'), 'utf8'));
    const main = JSON.parse(readFileSync(path.join(REPO_ROOT, 'packages', 'fcdev', 'package.json'), 'utf8'));
    assert.equal(document.version, main.version);
    assert.equal(main.optionalDependencies[document.name], main.version);
  });
});
