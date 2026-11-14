/*
 * ------------------------------------------------------------------------
 * 名称：media.ts
 * 说明：介质模块在渲染进程中的登记项：创建设备边栏的控制器，登记“设备”活动栏按钮与常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记的内容在 ui/media/media-module.tsx；控制器在这里创建，保证整个界面只有一个。
 * ------------------------------------------------------------------------
 */

import { MediaController } from '../../../ui/media/media-controller';
import { createMediaModule } from '../../../ui/media/media-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 介质模块。 */
export const MEDIA_RENDERER_MODULE: ShellModule = createMediaModule(new MediaController());
