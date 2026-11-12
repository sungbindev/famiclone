// ------------------------------------------------------------------------
// 名称：helpers.mjs
// 说明：测试公共工具：临时目录、写文件、探测本机是否有 cc65 工具链。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：集成测试在找不到工具链时跳过，并在跳过说明里写明原因。
// ------------------------------------------------------------------------
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { locateToolchain } from '../src/toolchain/index.mjs';

/**
 * 创建临时目录。
 * @returns {{dir: string, cleanup: () => void}} 目录与清理函数
 */
export function createTempDir() {
  const dir = mkdtempSync(path.join(tmpdir(), 'fcdev-test-'));
  return { dir, cleanup: () => rmSync(dir, { recursive: true, force: true }) };
}

/**
 * 在目录下写文件，自动创建上级目录。
 * @param {string} root 根目录
 * @param {string} relativePath 相对路径
 * @param {string} content 内容
 * @returns {string} 文件的绝对路径
 */
export function writeTestFile(root, relativePath, content) {
  const full = path.join(root, relativePath);
  mkdirSync(path.dirname(full), { recursive: true });
  writeFileSync(full, content);
  return full;
}

/**
 * 探测本机的 cc65 工具链。
 * @returns {import('../src/toolchain/index.mjs').Toolchain|null} 工具链；找不到时为 null
 */
export function findTestToolchain() {
  try {
    return locateToolchain();
  } catch {
    return null;
  }
}

/** 集成测试用的跳过说明：没有工具链时为字符串，有工具链时为 false */
export const SKIP_WITHOUT_TOOLCHAIN = findTestToolchain() ? false : '本机没有 cc65 工具链（设置 FCDEV_CC65_HOME 后运行）';
