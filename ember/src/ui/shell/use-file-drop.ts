/*
 * ------------------------------------------------------------------------
 * 名称：use-file-drop.ts
 * 说明：窗口上的文件拖放：把拖入窗口的文件交给主进程登记（预加载脚本取路径，页面只拿到编号与显示名），再按扩展名交给登记表里认领它的处理项；没有处理项认领的文件用通知说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：只响应带文件的拖动（拖选中的文字等不处理）；带文件的拖动一律阻止默认行为，否则浏览器会把窗口导航到被拖入的文件；
 *       拖放只在主窗口里生效，对话框是独立的窗口，不接收文件。
 * ------------------------------------------------------------------------
 */

import type { NotifyFunction } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellFileHandlerDefinition } from './shell-registry';

/** 用户可见的文字。 */
const TEXT = {
  unsupported: (name: string) => `不支持拖入的文件：${name}。可以拖入 NES 卡带、软盘镜像或声音文件`,
  unreadable: '无法读取拖入的文件，请从文件管理器里拖入磁盘上的文件'
} as const;

/**
 * 找到认领这个文件名的扩展名的处理项。
 * @param handlers 全部处理项。
 * @param name 文件名。
 * @returns 处理项；文件名没有扩展名或没有处理项认领时为 undefined。
 */
export function fileHandlerFor(handlers: readonly ShellFileHandlerDefinition[], name: string): ShellFileHandlerDefinition | undefined {
  const dot = name.lastIndexOf('.');
  if (dot < 0) return undefined;
  const extension = name.slice(dot + 1).toLowerCase();
  return handlers.find((handler) => handler.extensions.includes(extension));
}

/** 这次拖动是否带着文件。 */
function carriesFiles(event: DragEvent): boolean {
  return event.dataTransfer !== null && Array.from(event.dataTransfer.types).includes('Files');
}

/**
 * 接上窗口的文件拖放。
 * @param bridge 预加载脚本暴露的接口。
 * @param handlers 全部文件处理项。
 * @param notify 通知。
 */
export function useFileDrop(bridge: EmberBridge, handlers: readonly ShellFileHandlerDefinition[], notify: NotifyFunction): void {
  useEffect(() => {
    // 不阻止 dragover，浏览器就不会触发 drop；同时指明是复制，指针上不显示“禁止”。
    const handleDragOver = (event: DragEvent): void => {
      if (!carriesFiles(event)) return;
      event.preventDefault();
      if (event.dataTransfer !== null) event.dataTransfer.dropEffect = 'copy';
    };
    const handleDrop = (event: DragEvent): void => {
      if (!carriesFiles(event)) return;
      event.preventDefault();
      // 事件结束后文件清单会被清空，所以先取出来。
      const files = Array.from(event.dataTransfer?.files ?? []);
      if (files.length === 0) return;
      bridge.shell.registerDroppedFiles(files).then(
        (dropped) => {
          if (dropped.length === 0) notify(TEXT.unreadable, 'error');
          for (const file of dropped) {
            const handler = fileHandlerFor(handlers, file.name);
            if (handler === undefined) notify(TEXT.unsupported(file.name), 'error');
            else handler.open(file);
          }
        },
        () => notify(TEXT.unreadable, 'error')
      );
    };
    window.addEventListener('dragover', handleDragOver);
    window.addEventListener('drop', handleDrop);
    return () => {
      window.removeEventListener('dragover', handleDragOver);
      window.removeEventListener('drop', handleDrop);
    };
  }, [bridge, handlers, notify]);
}
