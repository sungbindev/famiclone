// ------------------------------------------------------------------------
// 名称：api-functions.test.ts
// 说明：测试 C 接口文档：头文件文档注释的解析、系统调用标签的匹配规则、查询优先级与格式化。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用内存里的头文件文本测试，真实 sb2000 头文件的查询见 test/logic/c-api。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import {
  formatFunction,
  formatModules,
  formatQueryResult,
  groupByModule,
  parseHeaderFunctions,
  queryApi,
  syscallMatches
} from '../../../src/shared/c-api/api-functions';

const SAMPLE_HEADER = [
  '/**\n * 读。\n * @syscall $20\n */\nvoid sb_a_read(void);',
  '/**\n * 写。\n * @syscall $21\n */\nvoid sb_a_write(void);',
  ''
].join('\n');

describe('解析文档注释', () => {
  it('说明全文、参数与返回值的续行、签名', () => {
    const [fn] = parseHeaderFunctions(
      '/**\n * 第一段。\n * 第二行。\n *\n * @syscall $10\n * @param a 参数 a，\n *        续行。\n * @return 返回值。\n */\nunsigned char sb_x(unsigned char a);\n',
      'sb2000-x.h'
    );
    expect(fn?.description).toBe('第一段。\n第二行。');
    expect(fn?.paramText['a']).toBe('参数 a， 续行。');
    expect(fn?.returnText).toBe('返回值。');
    expect(fn?.signature).toBe('unsigned char sb_x(unsigned char a)');
    expect(fn?.syscalls).toEqual(['$10']);
  });

  it('没有文档注释的函数不收录', () => {
    expect(parseHeaderFunctions('void sb_plain(void);\n', 'sb2000-x.h')).toEqual([]);
  });
});

describe('系统调用标签', () => {
  it('单个、范围、多个并列的匹配规则', () => {
    expect(syscallMatches('$7B/$09', '7b')).toBe(true);
    expect(syscallMatches('$7B/$09', '7b', '09')).toBe(true);
    expect(syscallMatches('$7B/$09', '7b', '0a')).toBe(false);
    expect(syscallMatches('$30/$04-$07', '30', '06')).toBe(true);
    expect(syscallMatches('$30/$04-$07', '30', '08')).toBe(false);
    expect(syscallMatches('$3C、$3D、$3E、$3F', '3e')).toBe(true);
    expect(syscallMatches('$7B/$01、$7B/$06', '7b', '06')).toBe(true);
    expect(syscallMatches('直接写 $0696-$0699', '06')).toBe(false);
  });
});

describe('查询', () => {
  const functions = parseHeaderFunctions(SAMPLE_HEADER, 'sb2000-a.h');

  it('优先级：函数名先于模块名先于系统调用号先于关键词', () => {
    expect(queryApi(functions, 'sb_a_read').kind).toBe('function');
    expect(queryApi(functions, 'a').kind).toBe('module');
    expect(queryApi(functions, '$21').kind).toBe('syscall');
    expect(queryApi(functions, '写').kind).toBe('text');
    expect(queryApi(functions, '没有').kind).toBe('none');
  });

  it('按模块分组，保持头文件顺序', () => {
    const modules = groupByModule([...functions, ...parseHeaderFunctions('/**\n * 十。\n */\nvoid sb_b_x(void);\n', 'sb2000-b.h')]);
    expect(modules.map((item) => [item.name, item.header, item.functions.length])).toEqual([
      ['a', 'sb2000-a.h', 2],
      ['b', 'sb2000-b.h', 1]
    ]);
  });
});

describe('格式化', () => {
  const functions = parseHeaderFunctions('/**\n * 打开。\n * @syscall $7B/$09\n * @param path 路径\n * @return 0 成功\n */\nunsigned char sb_file_open(const char *path);\n', 'sb2000-file.h');

  it('一个函数的完整文档含签名、所在、系统调用、参数与返回', () => {
    const text = formatFunction(functions[0]!);
    expect(text).toContain('unsigned char sb_file_open(const char *path);');
    expect(text).toContain('所在：sb2000-file.h');
    expect(text).toContain('系统调用：$7B/$09');
    expect(text).toContain('path  路径');
    expect(text).toContain('返回：0 成功');
  });

  it('模块列表与查询结果', () => {
    expect(formatModules('SB2000', groupByModule(functions))).toMatch(/1 个函数，1 个模块[\s\S]*file/);
    expect(formatQueryResult('file', queryApi(functions, 'file'))).toMatch(/^模块 file：1 个函数\n {2}sb_file_open {2}打开。/);
  });
});
