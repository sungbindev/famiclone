// ------------------------------------------------------------------------
// 名称：ember-dump.mjs
// 说明：经 Ember 的调试服务读出一段内存并写成二进制文件，用来把运行时的内存与映像文件逐字节比对。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：用法 node ember-dump.mjs <端口> <空间> <起始地址> <长度> <输出文件>；端点文件在
//       %TEMP%/fcdev-ember/<端口>/run/debug-endpoint.json（emu_launch 启动的 Ember）。只读内存，不改机器状态。
// ------------------------------------------------------------------------

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const [portText, space, addressText, lengthText, outFile] = process.argv.slice(2);
if (!outFile) {
  console.error('用法：node ember-dump.mjs <端口> <空间> <起始地址> <长度> <输出文件>');
  process.exit(2);
}
const CHUNK = 4096;
const endpointFile = path.join(os.tmpdir(), 'fcdev-ember', portText, 'run', 'debug-endpoint.json');
const endpoint = JSON.parse(fs.readFileSync(endpointFile, 'utf8'));
const start = Number(addressText);
const total = Number(lengthText);

const socket = new WebSocket(endpoint.url, ['ember-debug.v1']);
socket.binaryType = 'arraybuffer';
const pending = new Map();
let nextId = 1;

socket.addEventListener('message', (event) => {
  let message;
  let data;
  if (typeof event.data === 'string') {
    message = JSON.parse(event.data);
  } else {
    const bytes = new Uint8Array(event.data);
    const jsonLength = new DataView(event.data).getUint32(0, false);
    message = JSON.parse(new TextDecoder().decode(bytes.subarray(4, 4 + jsonLength)));
    data = bytes.slice(4 + jsonLength);
  }
  const entry = pending.get(message.id);
  if (!entry) return;
  pending.delete(message.id);
  if (message.error) entry.reject(new Error(JSON.stringify(message.error)));
  else entry.resolve({ result: message.result, data });
});

function request(method, params) {
  const id = nextId++;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject });
    socket.send(JSON.stringify({ jsonrpc: '2.0', id, method, params }));
  });
}

socket.addEventListener('open', async () => {
  try {
    await request('debug.hello', { client: 'bbg-dump', token: endpoint.token, protocol: 1 });
    const parts = [];
    for (let done = 0; done < total; done += CHUNK) {
      const { data } = await request('inspector.readMemory', { space, address: start + done, length: Math.min(CHUNK, total - done) });
      parts.push(Buffer.from(data));
    }
    fs.mkdirSync(path.dirname(outFile), { recursive: true });
    fs.writeFileSync(outFile, Buffer.concat(parts));
    console.log(`已写出 ${outFile}，${total} 字节`);
  } catch (error) {
    console.error(error.message);
    process.exitCode = 1;
  } finally {
    socket.close();
  }
});
socket.addEventListener('error', () => {
  console.error('连接失败');
  process.exitCode = 1;
});
