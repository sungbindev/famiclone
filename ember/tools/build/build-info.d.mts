/*
 * ------------------------------------------------------------------------
 * 名称：build-info.d.mts
 * 说明：build-info.mjs 的类型声明，供 Vite 配置与测试配置引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：无。
 * ------------------------------------------------------------------------
 */

import type { Plugin } from 'vite';

export declare const BUILD_INFO_MODULE: string;
export declare function formatBuildTime(date: Date): string;
export declare function buildInfoPlugin(): Plugin;
