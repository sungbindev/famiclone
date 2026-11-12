// ------------------------------------------------------------------------
// 名称：toolchain.test.mjs
// 说明：测试工具链定位：配置项、环境变量的优先级，目录不完整时报错，包名规则，以及真实工具可运行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用只含空文件的假目录测试定位逻辑，不运行假目录里的程序。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { getToolchainVersion, locateToolchain, TOOLCHAIN_ENV_NAME, TOOLCHAIN_PACKAGE_NAME } from '../src/toolchain/index.mjs';
import { createTempDir, findTestToolchain, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 在目录下创建一个完整的假工具链 */
function createFakeToolchain(root, name) {
  for (const tool of ['cc65', 'ca65', 'ld65']) writeTestFile(root, `${name}/bin/${tool}.exe`, '');
  return path.join(root, name);
}

describe('工具链定位', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('工具链包的包名在 @rujianjs 作用域下', () => {
    assert.equal(TOOLCHAIN_PACKAGE_NAME, '@rujianjs/fcdev-cc65');
  });

  it('配置项优先于环境变量', () => {
    const configured = createFakeToolchain(temp.dir, 'configured');
    const fromEnv = createFakeToolchain(temp.dir, 'from-env');
    const toolchain = locateToolchain({ configured, env: { [TOOLCHAIN_ENV_NAME]: fromEnv } });
    assert.deepEqual(toolchain, { home: configured, source: 'config' });
  });

  it('没有配置项时使用环境变量', () => {
    const fromEnv = createFakeToolchain(temp.dir, 'from-env');
    assert.deepEqual(locateToolchain({ env: { [TOOLCHAIN_ENV_NAME]: fromEnv } }), { home: fromEnv, source: 'env' });
  });

  it('指定的目录缺少工具时报错，不退回到其他来源', () => {
    writeTestFile(temp.dir, 'broken/readme.txt', '');
    assert.throws(
      () => locateToolchain({ configured: path.join(temp.dir, 'broken'), env: {} }),
      (error) => error.code === 'TOOLCHAIN_INCOMPLETE' && error.message.includes('cc65'),
    );
  });
});

describe('真实工具链', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  it('能运行 cc65 并读到版本', async () => {
    assert.match(await getToolchainVersion(findTestToolchain()), /^cc65 V\d+\.\d+/);
  });
});
