/*
 * ------------------------------------------------------------------------
 * 名称：core-process.ts
 * 说明：核心进程的抽象与 Electron utilityProcess 实现：启动核心宿主脚本、收发消息、监听退出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：本模块不认识消息的内容，消息格式由核心网关的实现负责。
 * ------------------------------------------------------------------------
 */

import { utilityProcess, type MessagePortMain } from 'electron';

/** 一个核心进程。 */
export interface CoreProcess {
  /** 发送消息；transfer 里的消息端口随消息转移给核心进程，转移后本进程不再能使用它们。 */
  postMessage(message: unknown, transfer?: readonly object[]): void;
  /** 收到消息。 */
  onMessage(listener: (message: unknown) => void): void;
  /** 进程退出，参数为退出码。 */
  onExit(listener: (code: number) => void): void;
  /** 结束进程。 */
  kill(): void;
}

/** 核心进程的服务名，显示在任务管理器中。 */
const SERVICE_NAME = 'Ember核心';

/**
 * 启动核心进程。
 * @param hostScript 核心宿主脚本的路径。
 * @returns 核心进程。
 */
export function startElectronCoreProcess(hostScript: string): CoreProcess {
  const child = utilityProcess.fork(hostScript, [], { serviceName: SERVICE_NAME, stdio: 'inherit' });
  return {
    postMessage: (message, transfer) => child.postMessage(message, transfer === undefined ? undefined : (transfer as MessagePortMain[])),
    onMessage: (listener) => {
      child.on('message', listener);
    },
    onExit: (listener) => {
      child.on('exit', listener);
    },
    kill: () => {
      child.kill();
    }
  };
}
