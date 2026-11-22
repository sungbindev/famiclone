// ------------------------------------------------------------------------
// 名称：source-charset.test.ts
// 说明：测试源文件转码：UTF-8 转 GB2312 的规则与出错提示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用真实 cc65 构建含汉字的学习机程序，见 test/logic/build/build-integration.test.ts。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { convertToGb2312 } from '../../../src/shared/build/source-charset';

/** “你好”的 GB2312 内码。 */
const NI_HAO = Buffer.from([0xc4, 0xe3, 0xba, 0xc3]);

describe('convertToGb2312', () => {
  it('汉字与全角标点转成两个字节，ASCII 不变，注释里的汉字一并转换', () => {
    expect([...convertToGb2312(Buffer.from('你好，小霸王！', 'utf8'), 'a.c')]).toEqual([
      0xc4, 0xe3, 0xba, 0xc3, 0xa3, 0xac, 0xd0, 0xa1, 0xb0, 0xd4, 0xcd, 0xf5, 0xa3, 0xa1
    ]);
    const source = '/* 说明 */ "a\\n"\r\n';
    expect(convertToGb2312(Buffer.from(source, 'utf8'), 'a.c').length).toBe(source.length + 2);
  });

  it('开头的 BOM 被去掉', () => {
    expect([...convertToGb2312(Buffer.from('\uFEFFint x;', 'utf8'), 'a.c')]).toEqual([...Buffer.from('int x;')]);
  });

  it('不是合法 UTF-8 的内容视为已经是 GB2312，原样返回', () => {
    const gb = Buffer.concat([Buffer.from('s = "'), NI_HAO, Buffer.from('";')]);
    expect(convertToGb2312(gb, 'a.c')).toBe(gb);
  });

  it('GB2312 里没有的字符报错，写出文件、行、列与字符', () => {
    expect(() => convertToGb2312(Buffer.from('int a;\n  s = "好😀";\n', 'utf8'), 'x/a.c')).toThrow(
      expect.objectContaining({
        code: 'SOURCE_CHAR_UNSUPPORTED',
        message: expect.stringMatching(/^x\/a\.c:2:9：.*U\+1F600/u)
      })
    );
  });
});
