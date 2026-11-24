// ------------------------------------------------------------------------
// 名称：cpp-properties.test.ts
// 说明：测试 c_cpp_properties.json 的解析：取出配置名，格式不符时不抛错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：只测纯函数。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { FCDEV_CONFIGURATION_NAME, parseConfigurationNames } from '../../src/shared/c-intellisense/cpp-properties';

describe('parseConfigurationNames', () => {
  it('按文件里的顺序返回每个配置的名字', () => {
    const text = JSON.stringify({ version: 4, configurations: [{ name: 'Win32' }, { name: 'fcdev', includePath: [] }] });
    expect(parseConfigurationNames(text)).toEqual(['Win32', 'fcdev']);
  });

  it('没有名字的配置被跳过', () => {
    expect(parseConfigurationNames(JSON.stringify({ configurations: [{}, { name: 1 }, null, { name: 'mine' }] }))).toEqual(['mine']);
  });

  it('不是合法 JSON、没有 configurations 数组或根不是对象时返回空数组', () => {
    expect(parseConfigurationNames('不是 JSON')).toEqual([]);
    expect(parseConfigurationNames('{}')).toEqual([]);
    expect(parseConfigurationNames('{"configurations": 1}')).toEqual([]);
    expect(parseConfigurationNames('null')).toEqual([]);
  });

  it('FC Dev 配置名与命令行写入的一致', () => {
    expect(FCDEV_CONFIGURATION_NAME).toBe('fcdev');
  });
});
