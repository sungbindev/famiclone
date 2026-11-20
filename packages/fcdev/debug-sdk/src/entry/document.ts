/*
 * ------------------------------------------------------------------------
 * 名称：document.ts
 * 说明：随 SDK 打包的 OpenRPC 接口描述文档（由 Ember 的 npm run schema 生成到 generated/openrpc.json）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import document from '../../generated/openrpc.json' with { type: 'json' };
import type { OpenRpcDocument } from '../logic/method-catalog.js';

/** 接口描述文档：全部请求、事件与数据类型。 */
export const OPENRPC_DOCUMENT = document as unknown as OpenRpcDocument;
