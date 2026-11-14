/*
 * ------------------------------------------------------------------------
 * 名称：server.ts
 * 说明：MCP 服务的协议处理：initialize、ping、tools/list、tools/call；工具调用转给 McpBackend，结果转成 MCP 的内容块（文本，PNG 截图带图像块）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只实现工具（tools）能力；传输（标准输入输出的逐行 JSON）在 index.ts。
 * ------------------------------------------------------------------------
 */

import { EmberConnectionError, EmberRequestError } from '../shared/errors.js';
import { EVENTS_TOOL_NAME, type McpTool } from './tools.js';

/** 工具调用依赖的后端：连接 Ember、发请求、读事件。 */
export interface McpBackend {
  /** 发请求；没有连接时先连接。 */
  call(method: string, params: object): Promise<unknown>;
  /** 开始记录（追加）给定的事件，并取出已记录的事件。 */
  drainEvents(patterns: readonly string[] | undefined, max: number): Promise<{ events: ReadonlyArray<{ event: string; data: unknown }>; remaining: number }>;
}

/** 服务信息。 */
export interface McpServerInfo {
  readonly name: string;
  readonly version: string;
}

/** 收到的报文。 */
interface IncomingMessage {
  readonly jsonrpc?: string;
  readonly id?: number | string;
  readonly method?: string;
  readonly params?: Record<string, unknown>;
}

/** 本服务支持的协议版本，客户端要求其中之一时原样采用，否则用最新的。 */
const SUPPORTED_PROTOCOLS = ['2025-11-25', '2025-06-18', '2025-03-26', '2024-11-05'];
const DEFAULT_EVENT_COUNT = 100;
/** 给模型看的服务说明。 */
const INSTRUCTIONS =
  'Ember 是 FC/红白机与学习机的模拟器。这些工具经调试服务控制正在运行的 Ember（需要在首选项的“调试服务”分组开启，或用启动参数 --debug-port）。常用流程：ember_cartridges_load 装入卡带，ember_runtime_run 按帧运行（wait 为 true 时等运行结束），ember_output_screenshot 截图（PNG 会作为图像返回），ember_input_key 与 ember_input_pad 输入，ember_events 读事件。';

/** 对象的 JSON 文本。 */
function pretty(value: unknown): string {
  return JSON.stringify(value, (_key, child) => (child instanceof Uint8Array ? `<二进制 ${child.byteLength} 字节>` : child), 2);
}

/** 错误转成给模型看的说明。 */
function errorText(error: unknown): string {
  if (error instanceof EmberRequestError) return `请求失败 ${error.code}：${error.message}`;
  if (error instanceof EmberConnectionError) return `连接 Ember 失败（${error.kind}）：${error.message}`;
  return error instanceof Error ? error.message : String(error);
}

/** base64。 */
function base64(bytes: Uint8Array): string {
  return Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('base64');
}

/** MCP 服务。 */
export class McpServer {
  private readonly toolsByName: ReadonlyMap<string, McpTool>;

  /**
   * @param tools 工具清单。
   * @param backend 后端。
   * @param info 服务名称与版本。
   */
  constructor(
    private readonly tools: readonly McpTool[],
    private readonly backend: McpBackend,
    private readonly info: McpServerInfo
  ) {
    this.toolsByName = new Map(tools.map((tool) => [tool.name, tool]));
  }

  /**
   * 处理一条收到的报文。
   * @returns 要回复的报文；通知没有回复。
   */
  async handle(message: IncomingMessage): Promise<Record<string, unknown> | undefined> {
    if (message.id === undefined) return undefined;
    try {
      return { jsonrpc: '2.0', id: message.id, result: await this.dispatch(message.method ?? '', message.params ?? {}) };
    } catch (error) {
      if (error instanceof ProtocolError) return { jsonrpc: '2.0', id: message.id, error: { code: error.code, message: error.message } };
      return { jsonrpc: '2.0', id: message.id, error: { code: -32603, message: errorText(error) } };
    }
  }

  private async dispatch(method: string, params: Record<string, unknown>): Promise<unknown> {
    switch (method) {
      case 'initialize': {
        const requested = typeof params.protocolVersion === 'string' ? params.protocolVersion : '';
        return { protocolVersion: SUPPORTED_PROTOCOLS.includes(requested) ? requested : SUPPORTED_PROTOCOLS[0], capabilities: { tools: {} }, serverInfo: this.info, instructions: INSTRUCTIONS };
      }
      case 'ping':
        return {};
      case 'tools/list':
        return { tools: this.tools.map((tool) => ({ name: tool.name, description: tool.description, inputSchema: tool.inputSchema })) };
      case 'tools/call':
        return this.callTool(params);
      default:
        throw new ProtocolError(-32601, `不支持的方法：${method}`);
    }
  }

  private async callTool(params: Record<string, unknown>): Promise<unknown> {
    const name = typeof params.name === 'string' ? params.name : '';
    const tool = this.toolsByName.get(name);
    if (tool === undefined) throw new ProtocolError(-32602, `没有这个工具：${name}`);
    const args = params.arguments;
    if (args !== undefined && (args === null || typeof args !== 'object' || Array.isArray(args))) throw new ProtocolError(-32602, 'arguments 必须是对象');
    const input = (args ?? {}) as Record<string, unknown>;
    try {
      if (tool.name === EVENTS_TOOL_NAME) return await this.readEvents(input);
      return this.toContent(await this.backend.call(tool.method as string, input));
    } catch (error) {
      return { isError: true, content: [{ type: 'text', text: errorText(error) }] };
    }
  }

  private async readEvents(input: Record<string, unknown>): Promise<unknown> {
    const patterns = Array.isArray(input.patterns) ? input.patterns.filter((item): item is string => typeof item === 'string') : undefined;
    const max = typeof input.max === 'number' && Number.isInteger(input.max) && input.max > 0 ? input.max : DEFAULT_EVENT_COUNT;
    const drained = await this.backend.drainEvents(patterns, max);
    return { content: [{ type: 'text', text: pretty(drained) }] };
  }

  /** 请求结果转成内容块：文本，加上 PNG 截图的图像块。 */
  private toContent(result: unknown): unknown {
    const content: Array<Record<string, string>> = [{ type: 'text', text: pretty(result) }];
    const record = result as { format?: unknown; data?: unknown } | null;
    if (record?.data instanceof Uint8Array && record.format === 'png') content.push({ type: 'image', data: base64(record.data), mimeType: 'image/png' });
    return { content };
  }
}

/** 协议级错误：以 JSON-RPC 错误响应返回，不是工具执行失败。 */
class ProtocolError extends Error {
  constructor(
    readonly code: number,
    message: string
  ) {
    super(message);
  }
}
