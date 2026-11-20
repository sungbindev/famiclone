// ------------------------------------------------------------------------
// 名称：emulator-install.test.mjs
// 说明：测试随包的 Ember：首次安装到用户级目录、包里版本更新时替换、被 Ember 升级过的副本不动、写不进用户级目录时直接用包里的程序、非 Windows 与没有模拟器包的情况，以及版本号比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：模拟器包与用户级目录都用临时目录（FCDEV_EMBER_PACKAGE、FCDEV_EMBER_HOME），不碰真实的 %LOCALAPPDATA%；程序内容用文本代替。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { compareVersions, installBundledEmber, userEmberDirectory } from '../src/emulator/index.mjs';
import { createTempDir, writeTestFile } from './helpers.mjs';

/** 文本的 SHA-256 */
function sha256Of(text) {
  return createHash('sha256').update(text).digest('hex');
}

/** 在 root 下建一个假的模拟器包，返回包目录 */
function makePackage(root, version, content) {
  const dir = path.join(root, 'package');
  writeTestFile(dir, 'bin/Ember.exe', content);
  writeTestFile(dir, 'manifest.json', JSON.stringify({ emberVersion: version, files: { 'bin/Ember.exe': sha256Of(content) } }));
  return dir;
}

describe('随包的 Ember', () => {
  let temp;
  let home;

  beforeEach(() => {
    temp = createTempDir();
    home = path.join(temp.dir, 'home');
  });

  afterEach(() => temp.cleanup());

  /** 安装一次，环境变量指向临时目录 */
  function install(packageDir, platform = 'win32') {
    return installBundledEmber({ env: { FCDEV_EMBER_PACKAGE: packageDir, FCDEV_EMBER_HOME: home }, platform });
  }

  it('第一次使用时把包里的程序复制到用户级目录，并写下版本与校验值', () => {
    const result = install(makePackage(temp.dir, '1.0.0', 'ember-1'));
    assert.deepEqual(result, { executable: path.join(home, 'Ember.exe'), source: 'installed', emberVersion: '1.0.0' });
    assert.equal(readFileSync(result.executable, 'utf8'), 'ember-1');
    assert.deepEqual(JSON.parse(readFileSync(path.join(home, 'installed.json'), 'utf8')), { emberVersion: '1.0.0', sha256: sha256Of('ember-1') });
  });

  it('副本没有被动过、包里的版本更新时替换副本', () => {
    install(makePackage(temp.dir, '1.0.0', 'ember-1'));
    const result = install(makePackage(temp.dir, '1.1.0', 'ember-2'));
    assert.equal(readFileSync(result.executable, 'utf8'), 'ember-2');
    assert.equal(JSON.parse(readFileSync(path.join(home, 'installed.json'), 'utf8')).emberVersion, '1.1.0');
  });

  it('包里的版本相同或更旧时不替换', () => {
    install(makePackage(temp.dir, '1.1.0', 'ember-2'));
    const result = install(makePackage(temp.dir, '1.0.0', 'ember-1'));
    assert.equal(readFileSync(result.executable, 'utf8'), 'ember-2');
  });

  it('副本被 Ember 升级过（内容与记录不同）时不动它，即使包里的版本更新', () => {
    const result = install(makePackage(temp.dir, '1.0.0', 'ember-1'));
    writeFileSync(result.executable, 'ember-upgraded-by-itself');
    const again = install(makePackage(temp.dir, '2.0.0', 'ember-3'));
    assert.equal(readFileSync(again.executable, 'utf8'), 'ember-upgraded-by-itself');
  });

  it('用户级目录里已有程序但没有记录时不动它', () => {
    writeTestFile(home, 'Ember.exe', 'placed-by-user');
    const result = install(makePackage(temp.dir, '1.0.0', 'ember-1'));
    assert.equal(readFileSync(result.executable, 'utf8'), 'placed-by-user');
    assert.equal(existsSync(path.join(home, 'installed.json')), false);
  });

  it('用户级目录写不进去（路径被文件占着）时直接用包里的程序', () => {
    writeTestFile(temp.dir, 'home', 'a file, not a directory');
    const packageDir = makePackage(temp.dir, '1.0.0', 'ember-1');
    const result = install(packageDir);
    assert.deepEqual(result, { executable: path.join(packageDir, 'bin', 'Ember.exe'), source: 'package', emberVersion: '1.0.0' });
  });

  it('没有模拟器包（或包不完整）时返回 null', () => {
    assert.equal(install(path.join(temp.dir, 'none')), null);
    const incomplete = path.join(temp.dir, 'incomplete');
    writeTestFile(incomplete, 'manifest.json', JSON.stringify({ emberVersion: '1.0.0', files: {} }));
    assert.equal(install(incomplete), null);
  });

  it('非 Windows 且没有指定用户级目录时没有随包的 Ember', () => {
    const packageDir = makePackage(temp.dir, '1.0.0', 'ember-1');
    assert.equal(installBundledEmber({ env: { FCDEV_EMBER_PACKAGE: packageDir }, platform: 'linux' }), null);
    assert.equal(userEmberDirectory({}, 'linux'), null);
  });

  it('用户级目录默认在 %LOCALAPPDATA%\\fcdev\\ember，FCDEV_EMBER_HOME 可以覆盖', () => {
    assert.equal(userEmberDirectory({ LOCALAPPDATA: path.join('C:', 'Users', 'me', 'AppData', 'Local') }, 'win32'), path.join('C:', 'Users', 'me', 'AppData', 'Local', 'fcdev', 'ember'));
    assert.equal(userEmberDirectory({ FCDEV_EMBER_HOME: home, LOCALAPPDATA: 'x' }, 'win32'), path.resolve(home));
  });
});

describe('版本号比较', () => {
  it('按主、次、修订逐段比较数字', () => {
    assert.ok(compareVersions('1.0.1', '1.0.0') > 0);
    assert.ok(compareVersions('1.10.0', '1.9.0') > 0);
    assert.ok(compareVersions('0.3.1', '0.3.10') < 0);
    assert.equal(compareVersions('2.0.0', '2.0.0'), 0);
  });
});
