/*
 * ------------------------------------------------------------------------
 * 名称：log-directory.test.ts
 * 说明：日志目录的测试：目录不存在时先创建再打开、打开函数返回错误说明时抛出带该说明的异常、目录已存在时直接打开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用真实的临时目录与假的打开函数，不打开系统的文件管理器。
 * ------------------------------------------------------------------------
 */

import { existsSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { createLogDirectory } from './log-directory';

let root = '';

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'ember-log-directory-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('createLogDirectory', () => {
  it('目录不存在时先创建，再用打开函数打开它', async () => {
    const path = join(root, 'logs');
    const openPath = vi.fn().mockImplementation(() => Promise.resolve(existsSync(path) ? '' : '目录还没有创建'));
    await createLogDirectory(path, openPath).open();
    expect(openPath).toHaveBeenCalledWith(path);
    expect(existsSync(path)).toBe(true);
  });

  it('目录已经存在时直接打开', async () => {
    const openPath = vi.fn().mockResolvedValue('');
    await createLogDirectory(root, openPath).open();
    expect(openPath).toHaveBeenCalledWith(root);
  });

  it('打开函数返回错误说明时抛出带该说明的异常', async () => {
    const open = createLogDirectory(join(root, 'logs'), vi.fn().mockResolvedValue('找不到文件管理器'));
    await expect(open.open()).rejects.toThrow('找不到文件管理器');
  });
});
