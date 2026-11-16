/*
 * ------------------------------------------------------------------------
 * 名称：open-file-dialog.ts
 * 说明：文件选择窗口的 Electron 实现：弹出系统的“打开”窗口，只选一个文件，或只选一个文件夹；也能弹出系统的“保存”窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：platform 层不引用 contracts，参数与返回值按结构与契约的 FileDialog 一致，由组装根注入；
 *       有父窗口时以它为父窗口（模态），没有时单独弹出；用户取消时返回 undefined；
 *       给了目录记忆时，窗口从同一用途上次选定的目录打开，选定后记下新的目录（文件记它所在的目录，文件夹记它自己）；记下的目录不存在时系统退回默认位置。
 * ------------------------------------------------------------------------
 */

import { dirname } from 'node:path';
import { dialog, type BrowserWindow, type OpenDialogOptions, type SaveDialogOptions } from 'electron';

/** 选择文件的参数。 */
export interface OpenFileOptions {
  readonly title: string;
  readonly filters: readonly { readonly name: string; readonly extensions: readonly string[] }[];
  /** 用途标识：相同用途的窗口记住上次打开的目录。 */
  readonly purpose: string;
}

/** 选择文件夹的参数。 */
export interface OpenDirectoryOptions {
  readonly title: string;
  readonly purpose: string;
}

/** 文件选择窗口。 */
export interface OpenFileDialog {
  pickFile(options: OpenFileOptions): Promise<string | undefined>;
  pickDirectory(options: OpenDirectoryOptions): Promise<string | undefined>;
  pickSaveFile(options: OpenFileOptions): Promise<string | undefined>;
}

/** 按用途记住上次目录的存储（结构与契约的 LastDirectoriesService 一致）。 */
export interface DirectoryMemory {
  get(purpose: string): string | undefined;
  set(purpose: string, directory: string): void;
}

/**
 * 创建文件选择窗口。
 * @param getParent 取父窗口，没有窗口时返回 undefined。
 * @param memory 记住上次目录的存储；不给时不记。
 * @returns 文件选择窗口。
 */
export function createOpenFileDialog(getParent: () => BrowserWindow | undefined, memory?: DirectoryMemory): OpenFileDialog {
  /** 该用途上次选定的目录，作为窗口的起始位置；没记过时不带起始位置。 */
  const startIn = (purpose: string): { defaultPath?: string } => {
    const directory = memory?.get(purpose);
    return directory === undefined ? {} : { defaultPath: directory };
  };
  /** 弹出系统的“打开”窗口（有父窗口时模态），只取第一项；取消或没选中时为 undefined。 */
  const show = async (dialogOptions: OpenDialogOptions): Promise<string | undefined> => {
    const parent = getParent();
    const result = parent === undefined ? await dialog.showOpenDialog(dialogOptions) : await dialog.showOpenDialog(parent, dialogOptions);
    const [path] = result.filePaths;
    return result.canceled || path === undefined ? undefined : path;
  };
  return {
    pickFile: async (options) => {
      const path = await show({
        title: options.title,
        properties: ['openFile'],
        filters: options.filters.map((filter) => ({ name: filter.name, extensions: [...filter.extensions] })),
        ...startIn(options.purpose)
      });
      if (path !== undefined) memory?.set(options.purpose, dirname(path));
      return path;
    },
    pickDirectory: async (options) => {
      const path = await show({ title: options.title, properties: ['openDirectory', 'createDirectory'], ...startIn(options.purpose) });
      if (path !== undefined) memory?.set(options.purpose, path);
      return path;
    },
    pickSaveFile: async (options) => {
      const saveOptions: SaveDialogOptions = {
        title: options.title,
        filters: options.filters.map((filter) => ({ name: filter.name, extensions: [...filter.extensions] })),
        ...startIn(options.purpose)
      };
      const parent = getParent();
      const result = parent === undefined ? await dialog.showSaveDialog(saveOptions) : await dialog.showSaveDialog(parent, saveOptions);
      if (result.canceled || result.filePath === '') return undefined;
      memory?.set(options.purpose, dirname(result.filePath));
      return result.filePath;
    }
  };
}
