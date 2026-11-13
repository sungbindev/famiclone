/*
 * ------------------------------------------------------------------------
 * 名称：file-error-logger.test.ts
 * 说明：主进程错误日志的测试：按本地日期分目录、一行一条、字段齐全。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { FileErrorLogger, localDate } from './file-error-logger';

let root = '';

afterEach(() => {
  if (root) rmSync(root, { recursive: true, force: true });
});

describe('FileErrorLogger', () => {
  it('按本地日期追加 JSON Lines', async () => {
    root = mkdtempSync(join(tmpdir(), 'ember-log-'));
    const time = new Date(2026, 10, 3, 9, 30, 15, 482);
    const logger = new FileErrorLogger(root, () => time);
    logger.write({ process: 'main', code: 'shell.mainError', message: '一' });
    logger.write({ process: 'renderer', module: 'system', code: 'shell.rendererError', message: '二', stack: 'at x', context: { a: 1 } });
    await logger.flush();

    const lines = readFileSync(join(root, localDate(time), 'errors.jsonl'), 'utf8').trim().split('\n');
    expect(lines.map((line) => JSON.parse(line) as unknown)).toEqual([
      { time: time.toISOString(), process: 'main', module: null, code: 'shell.mainError', message: '一', stack: null, context: {} },
      { time: time.toISOString(), process: 'renderer', module: 'system', code: 'shell.rendererError', message: '二', stack: 'at x', context: { a: 1 } }
    ]);
    expect(localDate(time)).toBe('2026-11-03');
  });
});
