// ------------------------------------------------------------------------
// 名称：frame-codec.ts
// 说明：二进制帧的解码：前 4 字节是大端的 JSON 报文长度，随后是 JSON 报文，其余字节是顶层 data 的原
//       始内容。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：规则见 Ember 的 docs/04-调试服务.md 第 2.3 节。
// ------------------------------------------------------------------------

import { EmberConnectionError } from '../../shared/debug-client/errors';
import type { JsonRpcMessage } from '../../shared/debug-client/rpc-message';

/** 长度前缀的字节数。 */
const LENGTH_PREFIX_SIZE = 4;

/** 取出对象；不是对象时抛出协议错误。 */
function asObject(value: unknown): Record<string, unknown> {
  if (value === null || typeof value !== 'object' || Array.isArray(value)) throw new EmberConnectionError('protocol', '二进制帧里的 data 没有挂在对象上');
  return value as Record<string, unknown>;
}

/**
 * 解码一个二进制帧：把剩余字节放回报文的 data 字段（响应在 result.data，事件在 params.data）。
 * @param bytes 帧的全部内容。
 * @throws EmberConnectionError 长度前缀、JSON 或 data 的位置不符合协议。
 */
export function decodeBinaryFrame(bytes: Uint8Array): JsonRpcMessage {
  if (bytes.byteLength < LENGTH_PREFIX_SIZE) throw new EmberConnectionError('protocol', '二进制帧太短，没有长度前缀');
  const jsonLength = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(0, false);
  if (LENGTH_PREFIX_SIZE + jsonLength > bytes.byteLength) throw new EmberConnectionError('protocol', '二进制帧的 JSON 长度超出了帧的大小');
  let message: JsonRpcMessage;
  try {
    message = JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes.subarray(LENGTH_PREFIX_SIZE, LENGTH_PREFIX_SIZE + jsonLength))) as JsonRpcMessage;
  } catch {
    throw new EmberConnectionError('protocol', '二进制帧里的报文不是合法的 JSON');
  }
  const data = bytes.slice(LENGTH_PREFIX_SIZE + jsonLength);
  if ('method' in message) {
    asObject((message.params ??= {})).data = data;
  } else {
    asObject((message.result ??= {})).data = data;
  }
  return message;
}
