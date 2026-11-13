/*
 * ------------------------------------------------------------------------
 * 名称：forge-env.d.ts
 * 说明：Electron Forge 的 Vite 插件在构建主进程时注入的常量。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

/** 开发服务器地址；正式包中为 undefined。 */
declare const MAIN_WINDOW_VITE_DEV_SERVER_URL: string | undefined;
/** 界面构建目录名。 */
declare const MAIN_WINDOW_VITE_NAME: string;
