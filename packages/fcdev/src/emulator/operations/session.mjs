// ------------------------------------------------------------------------
// 名称：session.mjs
// 说明：模拟器操作的当前会话：一个已连接的调试客户端，加上启动它的进程号与正在进行的记录；整个进程里只有一个会话。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：会话放在模块里，所以多次调用之间保持连接；连接断开时自动清除；只结束自己启动的进程，不碰用户自己打开的 Ember。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';

/**
 * @typedef {object} Session
 * @property {{request: Function, close: Function, closed?: Promise<unknown>, info?: object}} client 已连接的调试客户端
 * @property {number|undefined} port 调试端口；不是由这里启动或不知道时为 undefined
 * @property {number|undefined} pid 这里启动的 Ember 的进程号；连接别人的 Ember 时为 undefined
 * @property {Map<number, {kind: string}>} records 正在进行的跟踪与调用记录：编号到种类
 * @property {{cdl: boolean}} flags 正在进行的 CDL 标记
 */

/** @type {Session|null} */
let current = null;

/**
 * 取当前会话。
 * @returns {Session} 会话
 * @throws {FcdevError} 还没有连接
 */
export function currentSession() {
  if (current === null) {
    throw new FcdevError('还没有连接 Ember', { code: 'EMULATOR_NOT_CONNECTED', hint: '先用 emu_connect 连接已运行且开启了调试服务的 Ember，或用 emu_launch 启动一个' });
  }
  return current;
}

/**
 * 是否已有会话。
 * @returns {boolean} 有会话时为 true
 */
export function hasSession() {
  return current !== null;
}

/**
 * 建立新会话；已有会话先关闭（只关闭连接，不结束进程）。
 * @param {{client: Session['client'], port?: number, pid?: number}} input 客户端与来源
 * @returns {Session} 新会话
 */
export function openSession({ client, port, pid }) {
  closeSession();
  const session = { client, port, pid, records: new Map(), flags: { cdl: false } };
  current = session;
  client.closed?.then(() => {
    if (current === session) current = null;
  }).catch(() => {});
  return session;
}

/**
 * 关闭当前会话。
 * @param {{terminate?: boolean}} [options] terminate 为 true 时结束自己启动的 Ember 进程
 * @returns {{closed: boolean, terminated: boolean}} 是否关闭了会话、是否结束了进程
 */
export function closeSession({ terminate = false } = {}) {
  const session = current;
  if (session === null) return { closed: false, terminated: false };
  current = null;
  try {
    session.client.close();
  } catch {
    // 连接已经断开
  }
  let terminated = false;
  if (terminate && session.pid !== undefined) {
    try {
      process.kill(session.pid);
      terminated = true;
    } catch {
      // 进程已经退出
    }
  }
  return { closed: true, terminated };
}
