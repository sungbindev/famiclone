/*
 * ------------------------------------------------------------------------
 * 名称：json-file-store.ts
 * 说明：本地 JSON 文件的读写：同步读取（文件不存在、读不了或不是合法 JSON 时得到 undefined，不抛异常），防抖写入，退出前可立即写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：写入先写同目录的临时文件再改名，写到一半断电不会留下半个文件；写失败只经 onError 报告，不影响界面；
 *       定时器不阻止进程退出（unref），退出前要调用 flush。
 * ------------------------------------------------------------------------
 */

import { mkdirSync, readFileSync, renameSync, writeFileSync } from 'node:fs';
import { dirname } from 'node:path';

/** 写入的防抖时间（毫秒）：连续修改（拖动调整高度、移动窗口）合并成一次写盘。 */
const DEFAULT_DEBOUNCE_MS = 500;

/** 文件存储的选项。 */
export interface JsonFileStoreOptions {
  /** 文件的绝对路径。 */
  readonly path: string;
  /** 写入的防抖时间（毫秒）。 */
  readonly debounceMs?: number;
  /** 读写出错时的报告。 */
  readonly onError?: (error: unknown) => void;
}

/** 本地 JSON 文件存储。 */
export class JsonFileStore {
  private readonly debounceMs: number;
  private pending: { readonly value: unknown } | undefined;
  private timer: ReturnType<typeof setTimeout> | undefined;

  constructor(private readonly options: JsonFileStoreOptions) {
    this.debounceMs = options.debounceMs ?? DEFAULT_DEBOUNCE_MS;
  }

  /**
   * 读取文件内容。
   * @returns 解析后的内容；文件不存在、读不了或内容不是合法 JSON 时为 undefined。
   */
  read(): unknown {
    let text: string;
    try {
      text = readFileSync(this.options.path, 'utf8');
    } catch (error) {
      // 文件不存在是正常情况（第一次运行），不报告。
      if ((error as NodeJS.ErrnoException).code !== 'ENOENT') this.options.onError?.(error);
      return undefined;
    }
    try {
      return JSON.parse(text) as unknown;
    } catch (error) {
      this.options.onError?.(error);
      return undefined;
    }
  }

  /**
   * 安排写入；防抖时间内再次调用时只写最后一次的内容。
   * @param value 要保存的内容。
   */
  write(value: unknown): void {
    this.pending = { value };
    if (this.timer !== undefined) clearTimeout(this.timer);
    this.timer = setTimeout(() => this.flush(), this.debounceMs);
    this.timer.unref();
  }

  /** 立即写入尚未写盘的内容；没有时什么都不做。 */
  flush(): void {
    if (this.timer !== undefined) {
      clearTimeout(this.timer);
      this.timer = undefined;
    }
    const pending = this.pending;
    if (pending === undefined) return;
    this.pending = undefined;
    const temporary = `${this.options.path}.tmp`;
    try {
      mkdirSync(dirname(this.options.path), { recursive: true });
      writeFileSync(temporary, JSON.stringify(pending.value, null, 2), 'utf8');
      renameSync(temporary, this.options.path);
    } catch (error) {
      this.options.onError?.(error);
    }
  }
}
