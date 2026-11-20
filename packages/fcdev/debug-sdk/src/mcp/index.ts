/*
 * ------------------------------------------------------------------------
 * 名称：index.ts
 * 说明：MCP 服务（fcdev mcp）的入口：经标准输入输出（逐行 JSON）与 MCP 客户端通信，工具调用转给运行中的 Ember。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：标准输出只写协议报文，日志写标准错误；连接 Ember 的地址与令牌取环境变量 EMBER_DEBUG_URL、EMBER_DEBUG_TOKEN、EMBER_DEBUG_ENDPOINT、EMBER_USER_DATA，
 *       没有时读端点文件；EMBER_MCP_TIMEOUT_MS 指定每个请求等响应的毫秒数（默认 120000）。
 * ------------------------------------------------------------------------
 */

import { createInterface } from 'node:readline';

import packageInfo from '../../../package.json' with { type: 'json' };
import { connectToEmber } from '../entry/connect-node.js';
import { OPENRPC_DOCUMENT } from '../entry/document.js';
import { EmberBackend } from './backend.js';
import { McpServer } from './server.js';
import { buildTools } from './tools.js';

const DEFAULT_TIMEOUT_MS = 120_000;
const CLIENT_NAME = 'ember-debug-mcp';

const timeout = Number(process.env.EMBER_MCP_TIMEOUT_MS);
const backend = new EmberBackend(() => connectToEmber({ clientName: CLIENT_NAME }), Number.isFinite(timeout) && timeout >= 0 && process.env.EMBER_MCP_TIMEOUT_MS !== undefined ? timeout : DEFAULT_TIMEOUT_MS);
const server = new McpServer(buildTools(OPENRPC_DOCUMENT), backend, { name: 'ember-debug-mcp', version: packageInfo.version });

const input = createInterface({ input: process.stdin });
input.on('line', (line) => {
  if (line.trim() === '') return;
  let message: unknown;
  try {
    message = JSON.parse(line);
  } catch {
    process.stdout.write(`${JSON.stringify({ jsonrpc: '2.0', id: null, error: { code: -32700, message: '报文不是合法的 JSON' } })}\n`);
    return;
  }
  void server.handle(message as Parameters<McpServer['handle']>[0]).then((reply) => {
    if (reply !== undefined) process.stdout.write(`${JSON.stringify(reply)}\n`);
  });
});
input.on('close', () => process.exit(0));
