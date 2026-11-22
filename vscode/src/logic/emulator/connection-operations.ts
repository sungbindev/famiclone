// ------------------------------------------------------------------------
// 名称：connection-operations.ts
// 说明：连接类操作：connect（连接已运行的 Ember）、launch（启动 Ember 并连接）、status（会话与运行
//       状态，可关闭会话）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：launch 只启动调试模式的 Ember，不载入任何东西，载入用 load；Ember 的位置来自设置；关闭会话
//       时只结束自己启动的进程。
// ------------------------------------------------------------------------

import { FcdevError } from '../../shared/errors/fcdev-error';
import type { EmulatorSession } from './emulator-session';
import { CLIENT_NAME, launchEmulator } from './emulator-launcher';
import { requireEmulatorExecutable, type OperationContext } from './operation-context';
import { optionalBoolean, optionalInteger, optionalString } from './params';

/** 连接调试服务的超时，单位毫秒。 */
const CONNECT_TIMEOUT_MS = 3000;

/**
 * 描述会话：握手信息与运行状态。
 * @param session 会话。
 * @returns 描述。
 */
export async function describeSession(session: EmulatorSession): Promise<Record<string, unknown>> {
  const runtime = await session.client.request('runtime.status', {});
  return { port: session.port ?? null, launched: session.pid !== undefined, pid: session.pid ?? null, ember: session.client.info, runtime };
}

/**
 * 连接已运行且开启了调试服务的 Ember。
 * @param context 上下文。
 * @param params 可选 port（默认取设置里的调试端口，按它找 fcdev 启动的 Ember 的端点文件）、
 *   userDataDirectory、url 与 token。
 * @returns 连接结果与运行状态。
 * @throws FcdevError 连不上。
 */
export async function connect(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { env, store } = context;
  const port = optionalInteger(params, 'port', { min: 1, max: 65_535 }) ?? env.settings.debugPort();
  const url = optionalString(params, 'url');
  const token = optionalString(params, 'token');
  const userDataDirectory = optionalString(params, 'userDataDirectory') ?? env.processes.userDataDirectory(port);
  let client;
  try {
    client = await env.connector.connectToEmber({
      ...(url === undefined ? {} : { url }),
      ...(token === undefined ? {} : { token }),
      userDataDirectory,
      clientName: CLIENT_NAME,
      connectTimeoutMs: CONNECT_TIMEOUT_MS
    });
  } catch (error) {
    throw new FcdevError(`连不上 Ember：${error instanceof Error ? error.message : String(error)}`, {
      code: 'EMULATOR_CONNECT_FAILED',
      hint: `确认 Ember 已启动并开启了调试服务（“调试运行”或 emu_launch 启动的 Ember 端口是 ${port}）；没有运行的 Ember 时用 emu_launch 启动一个`,
      cause: error
    });
  }
  return { connected: true, ...(await describeSession(store.openSession({ client, port }))) };
}

/**
 * 启动 Ember 并连接：调试模式、独立的用户数据目录，不载入任何东西。
 * @param context 上下文。
 * @param params 可选 port（默认取设置里的调试端口）。
 * @returns 进程号、端口与运行状态。
 * @throws FcdevError 没有设置 Ember 的位置、端口被占用，或启动失败。
 */
export async function launch(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { env, store } = context;
  const port = optionalInteger(params, 'port', { min: 1, max: 65_535 }) ?? env.settings.debugPort();
  const executable = requireEmulatorExecutable(env);
  const { pid, client } = await launchEmulator(env, { executable, debug: true, port });
  if (client === null) throw new FcdevError('启动 Ember 后没有得到调试连接', { code: 'EMULATOR_NOT_READY' });
  const session = store.openSession({ client, port, pid });
  return { launched: true, executable, ...(await describeSession(session)) };
}

/**
 * 会话与运行状态；close 为 true 时关闭会话。
 * @param context 上下文。
 * @param params 可选 close、terminate（关闭时结束自己启动的 Ember）。
 * @returns 状态，或关闭的结果。
 */
export async function status(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { store } = context;
  if (optionalBoolean(params, 'close', false)) return { ...store.closeSession({ terminate: optionalBoolean(params, 'terminate', false) }) };
  if (!store.hasSession()) return { connected: false };
  const session = store.currentSession();
  return {
    connected: true,
    ...(await describeSession(session)),
    records: [...session.records].map(([id, record]) => ({ id, kind: record.kind })),
    cdl: session.flags.cdl
  };
}
