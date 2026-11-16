/*
 * ------------------------------------------------------------------------
 * 名称：help-protocol.ts
 * 说明：帮助文档的自定义协议 ember-help://：声明协议的特权（标准、安全、支持 fetch），并把请求交给站点文件服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：声明协议要在应用就绪之前，登记处理函数要在应用就绪之后；只接受 GET 与 HEAD。
 * ------------------------------------------------------------------------
 */

import { protocol } from 'electron';
import { HELP_SCHEME, respondToHelpRequest } from './help-site';

/** 声明自定义协议的特权；必须在应用就绪之前调用。 */
export function registerHelpScheme(): void {
  protocol.registerSchemesAsPrivileged([{ scheme: HELP_SCHEME, privileges: { standard: true, secure: true, supportFetchAPI: true } }]);
}

/**
 * 登记自定义协议的处理函数；必须在应用就绪之后调用。
 * @param root 站点目录。
 */
export function registerHelpProtocol(root: string): void {
  protocol.handle(HELP_SCHEME, async (request) => {
    if (request.method !== 'GET' && request.method !== 'HEAD') return new Response(null, { status: 405 });
    const result = await respondToHelpRequest(root, request.url);
    return new Response(request.method === 'HEAD' ? null : result.body, { status: result.status, headers: { ...result.headers } });
  });
}
