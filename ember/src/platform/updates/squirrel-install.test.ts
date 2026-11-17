/*
 * ------------------------------------------------------------------------
 * 名称：squirrel-install.test.ts
 * 说明：Squirrel 安装判断的测试：安装目录里有 Update.exe 与启动器、程序在 app-<版本> 目录里时返回启动器；开发版、非 Windows、免安装目录、缺 Update.exe 或缺启动器时返回 undefined。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：文件是否存在用假函数。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { findSquirrelLauncher } from './squirrel-install';

const ROOT = 'C:\\Users\\sun\\AppData\\Local\\ember';
const EXEC_PATH = `${ROOT}\\app-0.1.0\\ember.exe`;

function probe(overrides: Partial<Parameters<typeof findSquirrelLauncher>[0]> = {}, existing: readonly string[] = [`${ROOT}\\Update.exe`, `${ROOT}\\ember.exe`]) {
  return findSquirrelLauncher({ platform: 'win32', isPackaged: true, execPath: EXEC_PATH, exists: (path) => existing.includes(path), ...overrides });
}

describe('findSquirrelLauncher', () => {
  it('Squirrel 安装的正式版返回安装根目录里的启动器', () => {
    expect(probe()).toBe(`${ROOT}\\ember.exe`);
  });

  it('开发版（没有打包）不是', () => {
    expect(probe({ isPackaged: false })).toBeUndefined();
  });

  it('不是 Windows 不是', () => {
    expect(probe({ platform: 'darwin' })).toBeUndefined();
  });

  it('免安装的打包目录（程序不在 app-<版本> 目录里）不是', () => {
    expect(probe({ execPath: 'D:\\build\\Ember-win32-x64\\ember.exe' }, ['D:\\build\\Update.exe', 'D:\\build\\ember.exe'])).toBeUndefined();
  });

  it('缺 Update.exe 或缺启动器时不是', () => {
    expect(probe({}, [`${ROOT}\\ember.exe`])).toBeUndefined();
    expect(probe({}, [`${ROOT}\\Update.exe`])).toBeUndefined();
  });
});
