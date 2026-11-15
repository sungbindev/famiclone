/*
 * ------------------------------------------------------------------------
 * 名称：media-module.tsx
 * 说明：介质模块在外壳里的登记项：活动栏的“设备”按钮（只有边栏，没有页面）与常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：控制器由 bootstrap/renderer/modules/media.ts 创建并传入，边栏与常驻组件共用；规则见 docs/05-界面.md 第 4.3 节。
 * ------------------------------------------------------------------------
 */

import { iconDevices } from '@rujianjs/ui-kit';
import type { ShellModule, ShellPageProps } from '../shell/shell-registry';
import type { MediaController } from './media-controller';
import { MediaHost } from './MediaHost';
import { MediaSideBar } from './MediaSideBar';

/** 活动栏“设备”按钮的标识。 */
export const MEDIA_DEVICES_ACTIVITY_ID = 'media.devices';

/** 用户可见的文字。 */
const TEXT = {
  devices: '设备',
  floppyLamp: '软驱读写灯'
} as const;

/**
 * 创建介质模块的登记项。
 * @param controller 设备边栏的控制器，边栏与常驻组件共用。
 */
export function createMediaModule(controller: MediaController): ShellModule {
  return {
    name: 'media',
    activityItems: [
      {
        id: MEDIA_DEVICES_ACTIVITY_ID,
        label: TEXT.devices,
        icon: iconDevices,
        sideBar: { title: TEXT.devices, Component: () => <MediaSideBar controller={controller} /> },
        // 亮着 = 任一软驱的马达在转（真机的读写灯跟马达一起亮灭），没有软驱时不显示。
        lamp: () => {
          const { floppies } = controller.snapshot();
          return floppies.length === 0 ? undefined : { lit: floppies.some((floppy) => floppy.motor), label: TEXT.floppyLamp };
        }
      }
    ],
    Host: (props: ShellPageProps) => <MediaHost {...props} controller={controller} />
  };
}
