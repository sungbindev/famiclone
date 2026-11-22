// ------------------------------------------------------------------------
// 名称：call-arguments.test.ts
// 说明：调用调试请求时的参数解析：JSON 对象、key=value 列表、十六进制整数，以及结果里二进制数据的显
//       示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：从调试 SDK 的终端客户端测试并入扩展。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { CallArgumentError, parseCallParams, printable } from '../../../src/shared/debug-client/call-arguments';

describe('parseCallParams', () => {
  it('没有参数是空对象；key=value 的值按 JSON 解析，不是 JSON 就当字符串', () => {
    expect(parseCallParams([])).toEqual({});
    expect(parseCallParams(['frames=60', 'wait=true', 'path=D:/roms/a b.nes', 'name="x"', 'list=[1,2]'])).toEqual({ frames: 60, wait: true, path: 'D:/roms/a b.nes', name: 'x', list: [1, 2] });
    // 十六进制整数：0x 或 $ 开头；加引号当字符串
    expect(parseCallParams(['address=0x8000', 'start=$C000', 'last=0XFF', 'file="0x10"', 'name=0xZZ'])).toEqual({ address: 0x8000, start: 0xc000, last: 0xff, file: '0x10', name: '0xZZ' });
  });

  it('一个 JSON 对象也可以', () => {
    expect(parseCallParams(['{"frames":3}'])).toEqual({ frames: 3 });
  });

  it('写法不对时抛出 CallArgumentError', () => {
    expect(() => parseCallParams(['frames'])).toThrow(CallArgumentError);
    expect(() => parseCallParams(['=1'])).toThrow(CallArgumentError);
    expect(() => parseCallParams(['{bad'])).toThrow('JSON');
    expect(() => parseCallParams(['{"a":1}', 'b=2'])).toThrow('一个参数');
  });
});

describe('printable', () => {
  it('把二进制换成说明文字', () => {
    expect(printable({ a: [new Uint8Array(3)], b: 1 })).toEqual({ a: ['<二进制 3 字节>'], b: 1 });
  });
});
