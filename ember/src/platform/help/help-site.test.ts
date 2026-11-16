/*
 * ------------------------------------------------------------------------
 * 名称：help-site.test.ts
 * 说明：帮助文档站点文件服务的测试：地址到文件的映射（首页、目录、子页面）、不合格地址与跳出站点目录的地址被拒绝、读取文件的响应（类型、安全响应头、404）、站点内地址与外部链接的判断。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用临时目录做站点，不依赖真实的构建产物。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve, sep } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { HELP_START_URL, isHelpUrl, isOpenableExternalUrl, resolveHelpFile, respondToHelpRequest } from './help-site';

let root = '';

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'ember-help-'));
  mkdirSync(join(root, 'manual'));
  mkdirSync(join(root, 'assets'));
  writeFileSync(join(root, 'index.html'), '<h1>首页</h1>');
  writeFileSync(join(root, 'manual', 'start.html'), '<h1>快速开始</h1>');
  writeFileSync(join(root, 'manual', 'index.html'), '<h1>使用说明</h1>');
  writeFileSync(join(root, 'assets', 'app.js'), 'export {};');
  writeFileSync(join(root, 'assets', 'font.woff2'), 'x');
  writeFileSync(join(root, 'assets', 'data.bin'), 'x');
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('resolveHelpFile', () => {
  it('根地址与目录地址读该目录的 index.html，页面与资源按路径映射', () => {
    expect(resolveHelpFile(root, HELP_START_URL)).toBe(resolve(root, 'index.html'));
    expect(resolveHelpFile(root, 'ember-help://app/manual/')).toBe(resolve(root, 'manual', 'index.html'));
    expect(resolveHelpFile(root, 'ember-help://app/manual/start.html?x=1#top')).toBe(resolve(root, 'manual', 'start.html'));
    expect(resolveHelpFile(root, 'ember-help://app/assets/app.js')).toBe(resolve(root, 'assets', 'app.js'));
  });

  it('不是本站的协议或主机时返回 null', () => {
    expect(resolveHelpFile(root, 'https://app/index.html')).toBeNull();
    expect(resolveHelpFile(root, 'ember-help://other/index.html')).toBeNull();
    expect(resolveHelpFile(root, '不是地址')).toBeNull();
  });

  it('含编码的上级目录、反斜杠、冒号、空字符与无法解码的地址返回 null', () => {
    for (const path of ['/%2E%2E%2Fsecret', '/manual%2F..%2F..%2Fsecret', '/manual%5Cstart.html', '/C:/Windows/win.ini', '/a%00b', '/%E0%A4%A']) {
      expect(resolveHelpFile(root, `ember-help://app${path}`), path).toBeNull();
    }
  });

  it('地址里的点段会先被规整，结果要么是 null，要么还在站点目录之内', () => {
    for (const path of ['/../secret', '/manual/../../secret', '/%2e%2e/secret', '/%2e%2e', '//..//..']) {
      const file = resolveHelpFile(root, `ember-help://app${path}`);
      expect(file === null || file.startsWith(`${resolve(root)}${sep}`), path).toBe(true);
    }
  });
});

describe('respondToHelpRequest', () => {
  it('读出文件，按扩展名给类型，并带上安全响应头', async () => {
    const page = await respondToHelpRequest(root, HELP_START_URL);
    expect(page.status).toBe(200);
    expect(Buffer.from(page.body ?? []).toString('utf8')).toBe('<h1>首页</h1>');
    expect(page.headers['Content-Type']).toBe('text/html; charset=utf-8');
    expect(page.headers['X-Content-Type-Options']).toBe('nosniff');
    expect(page.headers['Content-Security-Policy']).toContain("default-src 'self'");
    expect((await respondToHelpRequest(root, 'ember-help://app/assets/app.js')).headers['Content-Type']).toBe('text/javascript; charset=utf-8');
    expect((await respondToHelpRequest(root, 'ember-help://app/assets/font.woff2')).headers['Content-Type']).toBe('font/woff2');
    expect((await respondToHelpRequest(root, 'ember-help://app/assets/data.bin')).headers['Content-Type']).toBe('application/octet-stream');
  });

  it('文件不存在、地址不合格、读目录时都是 404，且没有内容', async () => {
    for (const url of ['ember-help://app/missing.html', 'ember-help://app/%2E%2E%2Fsecret', 'ember-help://app/assets', 'https://example.com/']) {
      const reply = await respondToHelpRequest(root, url);
      expect(reply.status, url).toBe(404);
      expect(reply.body, url).toBeNull();
    }
  });
});

describe('isHelpUrl', () => {
  it('只有本站的协议与主机算站点内的地址', () => {
    expect(isHelpUrl('ember-help://app/manual/start.html')).toBe(true);
    expect(isHelpUrl('ember-help://other/')).toBe(false);
    expect(isHelpUrl('https://app/')).toBe(false);
    expect(isHelpUrl('不是地址')).toBe(false);
  });
});

describe('isOpenableExternalUrl', () => {
  it('只有 http 与 https 可以交给系统浏览器', () => {
    expect(isOpenableExternalUrl('https://example.com/a')).toBe(true);
    expect(isOpenableExternalUrl('http://example.com/')).toBe(true);
    for (const url of ['file:///C:/Windows/notepad.exe', 'javascript:alert(1)', 'ember-help://app/', 'mailto:a@b.c', '不是地址']) {
      expect(isOpenableExternalUrl(url), url).toBe(false);
    }
  });
});
