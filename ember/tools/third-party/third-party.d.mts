/*
 * ------------------------------------------------------------------------
 * 名称：third-party.d.mts
 * 说明：third-party.mjs 的类型声明，供 Vite 配置、打包配置与测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：条目字段与组件库 tools/third-party.d.mts 一致。
 * ------------------------------------------------------------------------
 */

import type { Plugin } from 'vite';

/** 清单条目。 */
export interface ThirdPartyEntry {
  name: string;
  version: string;
  authors: string[];
  license: string;
  homepage: string;
  usage: string;
  bundled: boolean;
  licenseText: string;
}

export declare const VIRTUAL_MODULE: string;
export declare const NOTICES_FILE: string;
export declare const NOTICES_DIRECTORY: string;
export declare function collectClient(uiKitDirectory: string): Promise<{ entries: ThirdPartyEntry[] }>;
export declare function writeNotices(uiKitDirectory: string): Promise<string>;
export declare function thirdPartyPlugin(uiKitDirectory: string): Plugin;
