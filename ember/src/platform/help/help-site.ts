/*
 * ------------------------------------------------------------------------
 * 名称：help-site.ts
 * 说明：帮助文档站点的静态文件服务：把 ember-help://app/ 下的请求地址映射到站点目录里的文件，读出内容并给出类型与安全响应头；还判断哪些链接可以交给系统浏览器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：不引用 electron，便于测试；站点是只读的静态页面，请求地址一律不可信：解码失败、含反斜杠、冒号、空字符、上级目录的地址，
 *       以及解析后落在站点目录之外的地址都按“不存在”处理；目录地址（以 / 结尾）读该目录的 index.html。
 * ------------------------------------------------------------------------
 */

import { readFile } from 'node:fs/promises';
import { extname, resolve, sep } from 'node:path';

/** 自定义协议名。 */
export const HELP_SCHEME = 'ember-help';
/** 站点的主机名，协议加主机名就是站点的来源。 */
export const HELP_HOST = 'app';
/** 站点的首页。 */
export const HELP_START_URL = `${HELP_SCHEME}://${HELP_HOST}/`;

/** 可以直接打开的页面名；页面地址只在这里，请求方不能指定。 */
export type HelpPageName = 'changelog';

/** 各页面的地址：站点只能用 .html 结尾的链接。 */
export const HELP_PAGE_URLS: Readonly<Record<HelpPageName, string>> = {
  changelog: `${HELP_START_URL}manual/changelog.html`
};

/** 文件类型：站点里用到的扩展名。 */
const CONTENT_TYPES: Readonly<Record<string, string>> = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.ico': 'image/x-icon',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2'
};

/** 页面里的内联脚本与样式由站点生成器输出（暗色主题的初始化等），所以放行 unsafe-inline；其余一律只允许本站。 */
const CONTENT_SECURITY_POLICY = "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'";

/** 一次响应。 */
export interface HelpResponse {
  readonly status: number;
  readonly headers: Readonly<Record<string, string>>;
  readonly body: Uint8Array<ArrayBuffer> | null;
}

/**
 * 把请求地址映射到站点目录里的文件。
 * @param root 站点目录。
 * @param url 请求地址。
 * @returns 文件的绝对路径；不是本站地址、地址不合格或落在站点目录之外时返回 null。
 */
export function resolveHelpFile(root: string, url: string): string | null {
  let target: URL;
  try {
    target = new URL(url);
  } catch {
    return null;
  }
  if (target.protocol !== `${HELP_SCHEME}:` || target.host !== HELP_HOST) return null;
  let pathname: string;
  try {
    pathname = decodeURIComponent(target.pathname);
  } catch {
    return null;
  }
  if (/[\0\\:]/.test(pathname)) return null;
  const segments = pathname.split('/').filter((segment) => segment !== '');
  if (segments.some((segment) => segment === '.' || segment === '..')) return null;
  const base = resolve(root);
  const file = resolve(base, ...(pathname.endsWith('/') ? [...segments, 'index.html'] : segments));
  return file.startsWith(`${base}${sep}`) ? file : null;
}

/** 响应头：类型、禁止猜测类型与内容安全策略。 */
function headersOf(contentType: string): Readonly<Record<string, string>> {
  return { 'Content-Type': contentType, 'X-Content-Type-Options': 'nosniff', 'Content-Security-Policy': CONTENT_SECURITY_POLICY };
}

/** 没有内容的错误响应。 */
function failure(status: number): HelpResponse {
  return { status, headers: headersOf('text/plain; charset=utf-8'), body: null };
}

/**
 * 处理一次请求：读出站点里的文件。
 * @param root 站点目录。
 * @param url 请求地址。
 * @returns 响应；文件不存在或地址不合格时是 404，读取出错时是 500。
 */
export async function respondToHelpRequest(root: string, url: string): Promise<HelpResponse> {
  const file = resolveHelpFile(root, url);
  if (file === null) return failure(404);
  try {
    const body = new Uint8Array(await readFile(file));
    return { status: 200, headers: headersOf(CONTENT_TYPES[extname(file).toLowerCase()] ?? 'application/octet-stream'), body };
  } catch (error) {
    const code = (error as NodeJS.ErrnoException).code;
    return failure(code === 'ENOENT' || code === 'EISDIR' || code === 'ENOTDIR' ? 404 : 500);
  }
}

/** 站点内的地址（协议与主机都是本站）。 */
export function isHelpUrl(url: string): boolean {
  try {
    const target = new URL(url);
    return target.protocol === `${HELP_SCHEME}:` && target.host === HELP_HOST;
  } catch {
    return false;
  }
}

/** 可以交给系统浏览器打开的外部链接：只有 http 与 https。 */
export function isOpenableExternalUrl(url: string): boolean {
  try {
    const protocol = new URL(url).protocol;
    return protocol === 'http:' || protocol === 'https:';
  } catch {
    return false;
  }
}
