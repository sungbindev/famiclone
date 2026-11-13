// ------------------------------------------------------------------------
// 名称：debug-client.mjs
// 说明：模拟器调试服务的客户端：连接本机端口，发送一行命令，读取以单独的“.”行结束的应答。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：协议见 virtuanes/README.md；服务只监听 127.0.0.1，一次只处理一条命令。
// ------------------------------------------------------------------------
import net from 'node:net';
import { FcdevError } from '../shared/index.mjs';

/** fcdev 的调试服务默认端口 */
export const DEFAULT_DEBUG_PORT = 5699;

/** 连接调试服务的超时，单位毫秒 */
const CONNECT_TIMEOUT_MS = 5_000;

/** 一条命令默认的应答超时，单位毫秒 */
const COMMAND_TIMEOUT_MS = 30_000;

/**
 * @typedef {object} DebugReply
 * @property {boolean} ok 应答首行以 OK 开头
 * @property {string} head 应答首行
 * @property {string[]} lines 首行之后的各行
 */

/**
 * 解析不含结尾“.”行的应答文本。
 * @param {string} text 应答文本
 * @returns {DebugReply} 解析结果
 */
export function parseReply(text) {
  const lines = text.split('\n').filter((line) => line.length > 0);
  const head = lines[0] ?? '';
  return { ok: head.startsWith('OK'), head, lines: lines.slice(1) };
}

/** 调试服务客户端 */
export class DebugClient {
  #socket;
  #buffer = '';
  #waiter = null;

  /**
   * @param {net.Socket} socket 已连接的套接字
   */
  constructor(socket) {
    this.#socket = socket;
    socket.setEncoding('utf8');
    socket.on('data', (chunk) => {
      this.#buffer += chunk;
      this.#deliver();
    });
    socket.on('close', () => {
      this.#waiter?.reject(new FcdevError('调试连接已关闭', { code: 'DEBUG_CONNECTION_CLOSED' }));
      this.#waiter = null;
    });
  }

  /**
   * 连接本机调试服务。
   * @param {{port?: number, timeoutMs?: number}} [options] 端口与连接超时
   * @returns {Promise<DebugClient>} 客户端
   * @throws {FcdevError} 连接失败或超时
   */
  static connect({ port = DEFAULT_DEBUG_PORT, timeoutMs = CONNECT_TIMEOUT_MS } = {}) {
    return new Promise((resolve, reject) => {
      const socket = net.connect({ host: '127.0.0.1', port });
      const timer = setTimeout(() => {
        socket.destroy();
        reject(new FcdevError(`连接调试服务（端口 ${port}）超时`, { code: 'DEBUG_CONNECT_FAILED' }));
      }, timeoutMs);
      socket.once('connect', () => {
        clearTimeout(timer);
        resolve(new DebugClient(socket));
      });
      socket.once('error', (error) => {
        clearTimeout(timer);
        reject(new FcdevError(`无法连接调试服务（端口 ${port}）`, {
          code: 'DEBUG_CONNECT_FAILED',
          hint: '先用 npx fcdev run --debug 启动模拟器，或检查端口是否一致',
          cause: error,
        }));
      });
    });
  }

  /** 如果缓冲区里已经有完整应答，交给等待者 */
  #deliver() {
    if (!this.#waiter) return;
    const end = this.#buffer.indexOf('\n.\n');
    if (end < 0) return;
    const reply = parseReply(this.#buffer.slice(0, end + 1));
    this.#buffer = this.#buffer.slice(end + 3);
    const { resolve, timer } = this.#waiter;
    clearTimeout(timer);
    this.#waiter = null;
    resolve(reply);
  }

  /**
   * 发送一条命令并等待应答；应答为 ERR 时不抛错，由调用者看 ok。
   * @param {string} command 命令文本，不含换行
   * @param {{timeoutMs?: number}} [options] 应答超时
   * @returns {Promise<DebugReply>} 应答
   * @throws {FcdevError} 上一条命令未返回、连接关闭或超时
   */
  send(command, { timeoutMs = COMMAND_TIMEOUT_MS } = {}) {
    if (this.#waiter) throw new FcdevError('上一条调试命令尚未返回', { code: 'DEBUG_BUSY' });
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.#waiter = null;
        reject(new FcdevError(`调试命令“${command}”超过 ${timeoutMs} 毫秒没有应答`, { code: 'DEBUG_TIMEOUT' }));
      }, timeoutMs);
      this.#waiter = { resolve, reject, timer };
      this.#socket.write(`${command}\n`);
    });
  }

  /** 关闭连接 */
  close() {
    this.#socket.end();
  }
}
