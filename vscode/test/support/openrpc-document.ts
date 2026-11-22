// ------------------------------------------------------------------------
// 名称：openrpc-document.ts
// 说明：测试用的接口描述文档：读扩展自带的 debug/openrpc.json。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：文档由 Ember 的 npm run schema 生成。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import type { OpenRpcDocument } from '../../src/shared/debug-client/method-catalog';

/** 接口描述文档：全部请求、事件与数据类型。 */
export const OPENRPC_DOCUMENT = JSON.parse(readFileSync(path.resolve(__dirname, '..', '..', 'debug', 'openrpc.json'), 'utf8')) as OpenRpcDocument;
