/*
 * ------------------------------------------------------------------------
 * 名称：media-module.tsx
 * 说明：介质模块在外壳里的登记项：底部面板的“设备”页签（系统栏左侧的“设备”按钮，图标里带软驱读写灯）、系统栏左侧的已载入软盘名与常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：控制器由 bootstrap/renderer/modules/media.ts 创建并传入，面板与常驻组件共用；规则见 docs/05-界面.md 第 4.3 节。
 * ------------------------------------------------------------------------
 */

import { iconDeviceFloppy } from '@rujianjs/ui-kit';
import type { ShellModule, ShellPageProps } from '../shell/shell-registry';
import type { MediaController } from './media-controller';
import { MediaHost } from './MediaHost';
import { MediaPanel } from './MediaPanel';
import { FloppyStatusItem } from './MediaStatusItems';

/** 底部面板“设备”页签的标识。 */
export const MEDIA_DEVICES_PANEL_ID = 'media.devices';

/** 软盘图标（iconDeviceFloppy）中间那个圆在图标里的位置：灯画在这个圆上。 */
const FLOPPY_LAMP_IN_ICON = { x: 50, y: 60 } as const;

/** 用户可见的文字。 */
const TEXT = {
  devices: '设备',
  floppyLamp: '软驱读写灯'
} as const;

/**
 * 创建介质模块的登记项。
 * @param controller 设备面板的控制器，面板与常驻组件共用。
 */
export function createMediaModule(controller: MediaController): ShellModule {
  return {
    name: 'media',
    bottomPanels: [
      {
        id: MEDIA_DEVICES_PANEL_ID,
        title: TEXT.devices,
        icon: iconDeviceFloppy,
        autoCollapse: true,
        Component: ({ finishAction }) => <MediaPanel controller={controller} onLoaded={finishAction} />,
        // 亮着 = 任一软驱的马达在转（真机的读写灯跟马达一起亮灭），没有软驱时不显示。
        lamp: () => {
          const { floppies } = controller.snapshot();
          return floppies.length === 0 ? undefined : { lit: floppies.some((floppy) => floppy.motor), label: TEXT.floppyLamp, inIcon: FLOPPY_LAMP_IN_ICON };
        }
      }
    ],
    statusItems: [{ id: 'media.floppy', align: 'start', Component: () => <FloppyStatusItem controller={controller} /> }],
    Host: (props: ShellPageProps) => <MediaHost {...props} controller={controller} />
  };
}
