/*
 * ------------------------------------------------------------------------
 * 名称：open-file-dialog.ts
 * 说明：文件选择窗口的 Electron 实现：弹出系统的“打开”窗口，只选一个文件，或只选一个文件夹。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：platform 层不引用 contracts，参数与返回值按结构与契约的 FileDialog 一致，由组装根注入；
 *       有父窗口时以它为父窗口（模态），没有时单独弹出；用户取消时返回 undefined。
 * ------------------------------------------------------------------------
 */

import { dialog, type BrowserWindow, type OpenDialogOptions } from 'electron';

/** 选择文件的参数。 */
export interface OpenFileOptions {
  readonly title: string;
  readonly filters: readonly { readonly name: string; readonly extensions: readonly string[] }[];
}

/** 选择文件夹的参数。 */
export interface OpenDirectoryOptions {
  readonly title: string;
}

/** 文件选择窗口。 */
export interface OpenFileDialog {
  pickFile(options: OpenFileOptions): Promise<string | undefined>;
  pickDirectory(options: OpenDirectoryOptions): Promise<string | undefined>;
}

/**
 * 创建文件选择窗口。
 * @param getParent 取父窗口，没有窗口时返回 undefined。
 * @returns 文件选择窗口。
 */
export function createOpenFileDialog(getParent: () => BrowserWindow | undefined): OpenFileDialog {
  /** 弹出系统的“打开”窗口（有父窗口时模态），只取第一项；取消或没选中时为 undefined。 */
  const show = async (dialogOptions: OpenDialogOptions): Promise<string | undefined> => {
    const parent = getParent();
    const result = parent === undefined ? await dialog.showOpenDialog(dialogOptions) : await dialog.showOpenDialog(parent, dialogOptions);
    const [path] = result.filePaths;
    return result.canceled || path === undefined ? undefined : path;
  };
  return {
    pickFile: (options) =>
      show({
        title: options.title,
        properties: ['openFile'],
        filters: options.filters.map((filter) => ({ name: filter.name, extensions: [...filter.extensions] }))
      }),
    pickDirectory: (options) => show({ title: options.title, properties: ['openDirectory', 'createDirectory'] })
  };
}
