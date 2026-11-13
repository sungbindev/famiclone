/*
 * ------------------------------------------------------------------------
 * 名称：file-error-logger.ts
 * 说明：主进程的系统错误日志：异步追加到 <日志目录>/<本地日期>/errors.jsonl，一行一条。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：字段与核心的 file-error-log.cpp 一致；按顺序写入，写入失败时丢弃该条，不再写日志。
 * ------------------------------------------------------------------------
 */

import { appendFile, mkdir } from 'node:fs/promises';
import { join } from 'node:path';

/** 一条系统错误。 */
export interface FileErrorRecord {
  readonly process: 'main' | 'renderer';
  readonly module?: string;
  readonly code: string;
  readonly message: string;
  readonly stack?: string;
  readonly context?: Readonly<Record<string, unknown>>;
}

/** 两位数字。 */
function pad(value: number): string {
  return String(value).padStart(2, '0');
}

/** 本地日期，如 2026-11-03。 */
export function localDate(time: Date): string {
  return `${time.getFullYear()}-${pad(time.getMonth() + 1)}-${pad(time.getDate())}`;
}

/** 文件错误日志。 */
export class FileErrorLogger {
  private queue: Promise<void> = Promise.resolve();

  /**
   * @param logsRoot 日志根目录（<用户数据目录>/logs）。
   * @param now 取当前时间，测试可替换。
   */
  constructor(
    private readonly logsRoot: string,
    private readonly now: () => Date = () => new Date()
  ) {}

  /** 写入一条错误；不抛出异常。 */
  write(record: FileErrorRecord): void {
    try {
      const time = this.now();
      const line = JSON.stringify({
        time: time.toISOString(),
        process: record.process,
        module: record.module ?? null,
        code: record.code,
        message: record.message,
        stack: record.stack ?? null,
        context: record.context ?? {}
      });
      const directory = join(this.logsRoot, localDate(time));
      this.queue = this.queue
        .then(async () => {
          await mkdir(directory, { recursive: true });
          await appendFile(join(directory, 'errors.jsonl'), `${line}\n`, 'utf8');
        })
        .catch(() => undefined);
    } catch {
      // 写日志失败不影响业务，也不再写日志。
    }
  }

  /** 等待已排队的错误写完。 */
  flush(): Promise<void> {
    return this.queue;
  }
}
