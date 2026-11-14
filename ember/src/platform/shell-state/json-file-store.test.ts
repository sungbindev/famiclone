/*
 * ------------------------------------------------------------------------
 * 名称：json-file-store.test.ts
 * 说明：本地 JSON 文件存储的测试：文件不存在、损坏、不可读时读到 undefined 且不抛异常；防抖合并写入；立即写盘；写失败只报告；写入留下的是完整文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { JsonFileStore } from './json-file-store';

let directory: string;

beforeEach(() => {
  directory = mkdtempSync(join(tmpdir(), 'ember-json-store-'));
});

afterEach(() => {
  vi.useRealTimers();
  rmSync(directory, { recursive: true, force: true });
});

describe('JsonFileStore.read', () => {
  it('文件不存在时返回 undefined，也不报告错误', () => {
    const onError = vi.fn();
    expect(new JsonFileStore({ path: join(directory, 'none.json'), onError }).read()).toBeUndefined();
    expect(onError).not.toHaveBeenCalled();
  });

  it('文件损坏时返回 undefined，并报告', () => {
    const path = join(directory, 'broken.json');
    writeFileSync(path, '{"layout": ', 'utf8');
    const onError = vi.fn();
    expect(new JsonFileStore({ path, onError }).read()).toBeUndefined();
    expect(onError).toHaveBeenCalledTimes(1);
  });

  it('路径是目录而读不了时返回 undefined，不抛异常', () => {
    const path = join(directory, 'dir');
    mkdirSync(path);
    const onError = vi.fn();
    expect(new JsonFileStore({ path, onError }).read()).toBeUndefined();
    expect(onError).toHaveBeenCalledTimes(1);
  });

  it('合法的内容原样读出', () => {
    const path = join(directory, 'ok.json');
    writeFileSync(path, '{"a": [1, 2]}', 'utf8');
    expect(new JsonFileStore({ path }).read()).toEqual({ a: [1, 2] });
  });
});

describe('JsonFileStore.write', () => {
  it('防抖时间内连续写入，只在最后写一次最后的内容', () => {
    vi.useFakeTimers();
    const path = join(directory, 'state.json');
    const store = new JsonFileStore({ path, debounceMs: 300 });
    store.write({ n: 1 });
    vi.advanceTimersByTime(200);
    store.write({ n: 2 });
    vi.advanceTimersByTime(200);
    expect(existsSync(path)).toBe(false);
    vi.advanceTimersByTime(100);
    expect(JSON.parse(readFileSync(path, 'utf8'))).toEqual({ n: 2 });
  });

  it('flush 立即写盘，之后不再重复写', () => {
    vi.useFakeTimers();
    const path = join(directory, 'nested', 'state.json');
    const store = new JsonFileStore({ path, debounceMs: 300 });
    store.write({ n: 3 });
    store.flush();
    expect(JSON.parse(readFileSync(path, 'utf8'))).toEqual({ n: 3 });
    writeFileSync(path, '{"n": 9}', 'utf8');
    vi.advanceTimersByTime(1000);
    expect(JSON.parse(readFileSync(path, 'utf8'))).toEqual({ n: 9 });
  });

  it('没有待写内容时 flush 什么都不做', () => {
    const path = join(directory, 'none.json');
    new JsonFileStore({ path }).flush();
    expect(existsSync(path)).toBe(false);
  });

  it('写入后目录里只有目标文件，没有临时文件', () => {
    const path = join(directory, 'state.json');
    const store = new JsonFileStore({ path });
    store.write({ a: 1 });
    store.flush();
    expect(existsSync(`${path}.tmp`)).toBe(false);
  });

  it('写入失败时只报告，不抛异常', () => {
    // 目标路径的父级是一个文件，建目录会失败。
    const blocker = join(directory, 'blocker');
    writeFileSync(blocker, 'x', 'utf8');
    const onError = vi.fn();
    const store = new JsonFileStore({ path: join(blocker, 'state.json'), onError });
    store.write({ a: 1 });
    expect(() => store.flush()).not.toThrow();
    expect(onError).toHaveBeenCalledTimes(1);
  });
});
