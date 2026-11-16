/*
 * ------------------------------------------------------------------------
 * 名称：main-module.ts
 * 说明：主进程模块登记项的类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：模块有主进程自己处理的请求（Schema 的 mainRequests）时，提供 createMainHandlers 创建处理函数。
 * ------------------------------------------------------------------------
 */

import type { RecentCartridgesService } from '../../contracts/cartridges/recent-cartridges-service';
import type { DroppedFilesService } from '../../contracts/shell/dropped-files';
import type { FileDialog } from '../../contracts/shell/file-dialog';
import type { HelpWindow } from '../../contracts/shell/help-window';
import type { LogDirectory } from '../../contracts/shell/log-directory';
import type { CoreRequestHandling } from '../../entry/ipc/core/core-ipc';
import type { MainRequestHandler } from '../../entry/ipc/core/main-request-ipc';
import type { ModuleSchema } from '../../shared/schema/schema-types';

/** 创建主进程请求的处理函数时可用的能力。 */
export interface MainHandlerContext {
  /** 文件选择窗口。 */
  readonly fileDialog: FileDialog;
  /** 拖入窗口的文件。 */
  readonly droppedFiles: DroppedFilesService;
  /** 日志目录。 */
  readonly logDirectory: LogDirectory;
  /** 帮助文档窗口。 */
  readonly helpWindow: HelpWindow;
  /** 以主进程的身份向核心发起请求所需的依赖。 */
  readonly core: CoreRequestHandling;
  /** 最近打开的卡带。 */
  readonly recentCartridges: RecentCartridgesService;
}

/** 一个模块在主进程中的登记项。 */
export interface MainModule {
  /** 模块的 Schema：入口层据此接受请求名并校验参数。 */
  readonly schema: ModuleSchema;
  /** 创建主进程自己处理的请求的处理函数，键为请求名“模块.操作”。 */
  readonly createMainHandlers?: (context: MainHandlerContext) => Readonly<Record<string, MainRequestHandler>>;
}
