/*
 * ------------------------------------------------------------------------
 * 名称：system.ts
 * 说明：系统模块在渲染进程中的登记项：“帮助”菜单与“关于 Ember”对话框，第三方依赖清单与编译时间取自构建脚本生成的虚拟模块。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记的内容在 ui/system/system-module.tsx。
 * ------------------------------------------------------------------------
 */

import buildTime from 'virtual:ember-build-info';
import notices from 'virtual:ember-third-party';
import type { ShellModule } from '../../../ui/shell/shell-registry';
import { createSystemModule } from '../../../ui/system/system-module';

/** 系统模块。 */
export const SYSTEM_RENDERER_MODULE: ShellModule = createSystemModule(notices, buildTime);
