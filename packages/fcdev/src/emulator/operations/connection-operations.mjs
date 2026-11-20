// ------------------------------------------------------------------------
// 名称：connection-operations.mjs
// 说明：连接类操作：connect（连接已运行的 Ember）、launch（启动 Ember 并连接）、status（会话与运行状态，可关闭会话）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：launch 只启动调试模式的 Ember，不载入任何东西，载入用 load；关闭会话时只结束自己启动的进程。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { loadDebugSdk } from '../debug-sdk-loader.mjs';
import { DEFAULT_DEBUG_PORT, debugUserDataDirectory, launchEmulator } from '../emulator-launcher.mjs';
import { locateEmulator } from '../emulator-locator.mjs';
import { optionalBoolean, optionalInteger, optionalString } from './params.mjs';
import { closeSession, currentSession, hasSession, openSession } from './session.mjs';

/** 连接调试服务的超时，单位毫秒 */
const CONNECT_TIMEOUT_MS = 3000;

/** 调试客户端的名字，显示在 Ember 的连接列表里 */
const CLIENT_NAME = 'fcdev';

/**
 * 描述会话：握手信息与运行状态。
 * @param {import('./session.mjs').Session} session 会话
 * @returns {Promise<object>} 描述
 */
export async function describeSession(session) {
  const runtime = await session.client.request('runtime.status', {});
  return { port: session.port ?? null, launched: session.pid !== undefined, pid: session.pid ?? null, ember: session.client.info ?? null, runtime };
}

/**
 * 连接已运行且开启了调试服务的 Ember。
 * @param {Record<string, unknown>} params 可选 port（默认 5699，按它找 fcdev 启动的 Ember 的端点文件）、userDataDirectory、url 与 token
 * @returns {Promise<object>} 连接结果与运行状态
 * @throws {FcdevError} 调试 SDK 没有构建，或连不上
 */
export async function connect(params) {
  const sdk = await loadDebugSdk();
  const port = optionalInteger(params, 'port', { min: 1, max: 65535 }) ?? DEFAULT_DEBUG_PORT;
  const url = optionalString(params, 'url');
  const token = optionalString(params, 'token');
  const userDataDirectory = optionalString(params, 'userDataDirectory') ?? debugUserDataDirectory(port);
  let client;
  try {
    client = await sdk.connectToEmber({ ...(url === undefined ? { userDataDirectory } : { url, token }), clientName: CLIENT_NAME, connectTimeoutMs: CONNECT_TIMEOUT_MS });
  } catch (error) {
    throw new FcdevError(`连不上 Ember：${error.message}`, {
      code: 'EMULATOR_CONNECT_FAILED',
      hint: `确认 Ember 已启动并开启了调试服务（fcdev run --debug 或 emu_launch 启动的 Ember 端口是 ${port}）；没有运行的 Ember 时用 emu_launch 启动一个`,
      cause: error,
    });
  }
  return { connected: true, ...(await describeSession(openSession({ client, port }))) };
}

/**
 * 启动 Ember 并连接：调试模式、独立的用户数据目录，不载入任何东西。
 * @param {Record<string, unknown>} params 可选 port（默认 5699）、executable（Ember 的路径，缺省按环境变量与随包的 Ember 定位）
 * @param {{env?: NodeJS.ProcessEnv}} [options] 运行环境
 * @returns {Promise<object>} 进程号、端口与运行状态
 * @throws {FcdevError} 找不到 Ember、端口被占用，或启动失败
 */
export async function launch(params, { env = process.env } = {}) {
  const port = optionalInteger(params, 'port', { min: 1, max: 65535 }) ?? DEFAULT_DEBUG_PORT;
  const { executable, source } = locateEmulator({ configured: optionalString(params, 'executable'), env });
  const { pid, client } = await launchEmulator({ executable, debug: true, port });
  const session = openSession({ client, port, pid });
  return { launched: true, executable, source, ...(await describeSession(session)) };
}

/**
 * 会话与运行状态；close 为 true 时关闭会话。
 * @param {Record<string, unknown>} params 可选 close、terminate（关闭时结束自己启动的 Ember）
 * @returns {Promise<object>} 状态，或关闭的结果
 */
export async function status(params) {
  if (optionalBoolean(params, 'close', false)) return closeSession({ terminate: optionalBoolean(params, 'terminate', false) });
  if (!hasSession()) return { connected: false };
  const session = currentSession();
  return {
    connected: true,
    ...(await describeSession(session)),
    records: [...session.records].map(([id, record]) => ({ id, kind: record.kind })),
    cdl: session.flags.cdl,
  };
}
