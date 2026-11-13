/*
 * ------------------------------------------------------------------------
 * 名称：forge.config.mts
 * 说明：Electron Forge 配置：应用标识、Vite 构建入口（主进程、预加载脚本、核心宿主、界面）、随安装包分发的原生模块与第三方开源代码的许可全文、
 *       Windows 安装程序（Squirrel），以及打包时关闭的 Electron 危险开关（Fuses）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：原生模块由 npm run core:build 编译到 core/build/Release，打包时作为额外资源复制到应用的 resources 目录；
 *       预装的官方 mapper 插件包（build/plugins 下的 .emap，同样由 core:build 打包）以目录为单位复制成 resources/plugins，主进程把它交给核心作为预装目录；
 *       应用图标做好后在 packagerConfig 与安装程序里补上；macOS 安装包格式以后再定。
 * ------------------------------------------------------------------------
 */

import type { ForgeConfig } from '@electron-forge/shared-types';
import { MakerSquirrel } from '@electron-forge/maker-squirrel';
import { FusesPlugin } from '@electron-forge/plugin-fuses';
import { VitePlugin } from '@electron-forge/plugin-vite';
import { FuseV1Options, FuseVersion } from '@electron/fuses';
import { NOTICES_DIRECTORY, NOTICES_FILE, writeNotices } from './tools/third-party/third-party.mjs';
import { uiKitDirectory } from './ui-kit-source.mts';

/** 应用标识：Windows 的应用用户模型 ID、macOS 的 Bundle ID；发布后不再修改。 */
const APP_ID = 'cn.3578.ember';
/** 编译好的原生模块。 */
const NATIVE_MODULE = './core/build/Release/ember_core.node';
/** 打好的预装插件目录：目录名 plugins 保留，成为 resources/plugins。 */
const BUNDLED_PLUGINS = './build/plugins';
/** 可执行文件名。 */
const EXECUTABLE_NAME = 'ember';

const config: ForgeConfig = {
  packagerConfig: {
    asar: true,
    appBundleId: APP_ID,
    executableName: EXECUTABLE_NAME,
    extraResource: [NATIVE_MODULE, BUNDLED_PLUGINS, `${NOTICES_DIRECTORY}/${NOTICES_FILE}`]
  },
  hooks: {
    // 第三方开源代码的许可全文在打包前生成，与关于对话框的清单同源。
    generateAssets: async () => {
      await writeNotices(uiKitDirectory());
    }
  },
  rebuildConfig: {},
  makers: [
    new MakerSquirrel(
      {
        name: EXECUTABLE_NAME,
        exe: `${EXECUTABLE_NAME}.exe`,
        setupExe: 'ember-setup.exe',
        authors: 'sungbin',
        description: 'Ember'
      },
      ['win32']
    )
  ],
  plugins: [
    new VitePlugin({
      build: [
        { entry: 'src/bootstrap/main/main.ts', config: 'vite.main.config.mts', target: 'main' },
        { entry: 'src/core-host/core-host.ts', config: 'vite.main.config.mts', target: 'main' },
        { entry: 'src/entry/preload/preload.ts', config: 'vite.preload.config.mts', target: 'preload' }
      ],
      renderer: [{ name: 'main_window', config: 'vite.renderer.config.mts' }]
    }),
    new FusesPlugin({
      version: FuseVersion.V1,
      [FuseV1Options.RunAsNode]: false,
      [FuseV1Options.EnableCookieEncryption]: true,
      [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false,
      [FuseV1Options.EnableNodeCliInspectArguments]: false,
      [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: true,
      [FuseV1Options.OnlyLoadAppFromAsar]: true
    })
  ]
};

export default config;
