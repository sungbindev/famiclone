// ------------------------------------------------------------------------
// 名称：project-info.test.ts
// 说明：测试项目信息的解析：从 fcdev info --json 的输出里取目标与头文件目录，格式不合法时返回空。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：样例取自 fcdev info --json 的真实输出格式。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { parseProjectInfo } from '../../src/shared/project/project-info';

/** fcdev info --json 的输出样例。 */
const OUTPUT = JSON.stringify({
  root: 'D:\\proj',
  configFile: 'D:\\proj\\fcdev.json',
  target: { name: 'sb2000', title: '小霸王学习机 SB2000（SB-DOS）' },
  toolchain: { home: 'D:\\cc65', source: 'package', version: 'cc65 V2.19' },
  includeDirs: ['D:\\t\\include', 'D:\\cc65\\include'],
  asmIncludeDirs: ['D:\\t\\asminc', 'D:\\cc65\\asminc'],
  defines: { __CC65__: 1 },
  outDir: 'D:\\proj\\build'
});

describe('解析项目信息', () => {
  it('取出根目录、目标名与两类目录', () => {
    expect(parseProjectInfo(OUTPUT)).toEqual({
      root: 'D:\\proj',
      targetName: 'sb2000',
      includeDirs: ['D:\\t\\include', 'D:\\cc65\\include'],
      asmIncludeDirs: ['D:\\t\\asminc', 'D:\\cc65\\asminc']
    });
  });

  it('不是 JSON、不是对象、缺字段或类型不对时返回空', () => {
    expect(parseProjectInfo('错误：找不到 cc65 工具链')).toBeUndefined();
    expect(parseProjectInfo('[]')).toBeUndefined();
    expect(parseProjectInfo('null')).toBeUndefined();
    expect(parseProjectInfo(JSON.stringify({ root: 'x' }))).toBeUndefined();
    expect(parseProjectInfo(JSON.stringify({ root: 'x', target: { name: 'nes' }, includeDirs: [1], asmIncludeDirs: [] }))).toBeUndefined();
  });
});
