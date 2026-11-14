/*
 * ------------------------------------------------------------------------
 * 名称：node.ts
 * 说明：@rujianjs/ember-debug-sdk/node 的入口：Node 环境下自动找到正在运行的 Ember（connectToEmber）与端点文件的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

export { connectToEmber, type ConnectToEmberOptions } from './entry/connect-node.js';
export { defaultUserDataDirectory, endpointFilePath, readEndpoint, resolveTarget, type Endpoint, type EndpointDeps, type Target, type TargetOptions } from './infra/endpoint-file.js';
