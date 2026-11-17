/*
 * ------------------------------------------------------------------------
 * 名称：allow-multiple-instances.test.ts
 * 说明：启动时读取“允许同时运行多个实例”的测试：开启时为真，文件缺失、损坏、版本不对、取值不是布尔值时都为假。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * ------------------------------------------------------------------------
 */

import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { ALLOW_MULTIPLE_INSTANCES_SETTING_ID, readAllowMultipleInstances } from './allow-multiple-instances';

let directory = '';
let file = '';

beforeEach(() => {
  directory = mkdtempSync(join(tmpdir(), 'ember-multi-instance-'));
  file = join(directory, 'preferences.json');
});

afterEach(() => rmSync(directory, { recursive: true, force: true }));

/** 按核心保存的格式写首选项文件。 */
function writePreferences(values: Record<string, unknown>, version = 1): void {
  writeFileSync(file, JSON.stringify({ version, values }));
}

describe('readAllowMultipleInstances', () => {
  it('设置项为 true 时允许', () => {
    writePreferences({ [ALLOW_MULTIPLE_INSTANCES_SETTING_ID]: true });
    expect(readAllowMultipleInstances(file)).toBe(true);
  });

  it('设置项为 false、没有这一项时不允许', () => {
    writePreferences({ [ALLOW_MULTIPLE_INSTANCES_SETTING_ID]: false });
    expect(readAllowMultipleInstances(file)).toBe(false);
    writePreferences({ 'update.autoCheck': true });
    expect(readAllowMultipleInstances(file)).toBe(false);
  });

  it('取值不是布尔值时不允许', () => {
    for (const bad of ['true', 1, null, [true]]) {
      writePreferences({ [ALLOW_MULTIPLE_INSTANCES_SETTING_ID]: bad });
      expect(readAllowMultipleInstances(file)).toBe(false);
    }
  });

  it('文件不存在、内容损坏、格式版本不对或结构不对时不允许，不抛异常', () => {
    expect(readAllowMultipleInstances(file)).toBe(false);
    writeFileSync(file, '{坏的');
    expect(readAllowMultipleInstances(file)).toBe(false);
    writePreferences({ [ALLOW_MULTIPLE_INSTANCES_SETTING_ID]: true }, 2);
    expect(readAllowMultipleInstances(file)).toBe(false);
    for (const text of ['null', '5', '[]', '{"version":1}', '{"version":1,"values":null}']) {
      writeFileSync(file, text);
      expect(readAllowMultipleInstances(file)).toBe(false);
    }
  });
});
