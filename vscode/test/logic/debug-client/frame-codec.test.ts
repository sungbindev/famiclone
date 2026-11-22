// ------------------------------------------------------------------------
// 名称：frame-codec.test.ts
// 说明：二进制帧解码的测试：响应的 data 放进 result，事件的 data 放进 params，长度前缀与 JSON 不合
//       格时报协议错误。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：从调试 SDK 的测试并入扩展，用假传输层，不连真实的 Ember。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';

import { decodeBinaryFrame } from '../../../src/logic/debug-client/frame-codec';
import { EmberConnectionError } from '../../../src/shared/debug-client/errors';
import { binaryFrame } from '../../support/fake-transport';

describe('decodeBinaryFrame', () => {
  it('把剩余字节放进响应的 result.data', () => {
    const message = decodeBinaryFrame(binaryFrame({ jsonrpc: '2.0', id: 7, result: { format: 'rgba', width: 2 } }, new Uint8Array([1, 2, 3])));
    expect(message).toMatchObject({ id: 7, result: { format: 'rgba', width: 2 } });
    expect((message as { result: { data: Uint8Array } }).result.data).toEqual(new Uint8Array([1, 2, 3]));
  });

  it('把剩余字节放进事件的 params.data', () => {
    const message = decodeBinaryFrame(binaryFrame({ jsonrpc: '2.0', method: 'runtime.frame', params: { frame: 3 } }, new Uint8Array([9])));
    expect((message as { params: { frame: number; data: Uint8Array } }).params.frame).toBe(3);
    expect((message as { params: { data: Uint8Array } }).params.data).toEqual(new Uint8Array([9]));
  });

  it('响应没有 result 时补一个对象放 data；data 可以为空', () => {
    const message = decodeBinaryFrame(binaryFrame({ jsonrpc: '2.0', id: 1 }, new Uint8Array()));
    expect((message as { result: { data: Uint8Array } }).result.data.byteLength).toBe(0);
  });

  it('长度前缀、JSON 或 data 的位置不合格时抛出协议错误', () => {
    expect(() => decodeBinaryFrame(new Uint8Array([0, 0]))).toThrow(EmberConnectionError);
    const tooLong = new Uint8Array([0, 0, 0, 200, 123, 125]);
    expect(() => decodeBinaryFrame(tooLong)).toThrow('超出');
    const notJson = new Uint8Array([0, 0, 0, 3, 120, 120, 120]);
    expect(() => decodeBinaryFrame(notJson)).toThrow('JSON');
    expect(() => decodeBinaryFrame(binaryFrame({ jsonrpc: '2.0', id: 1, result: 5 }, new Uint8Array([1])))).toThrow('对象');
  });
});
