/*
 * ------------------------------------------------------------------------
 * 名称：MediaStatusItems.tsx
 * 说明：介质模块登记到系统栏左侧的项：已载入的软盘镜像名（在主机型号后面）；没有载入盘片时不显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：只有一个软驱时直接显示镜像名，有多个软驱时每项前面带驱动器名（如“A: 镜像名”）；有未保存的修改时在后面注明；悬停提示是“软驱 A 的盘片”。
 * ------------------------------------------------------------------------
 */

import { StatusBarItem } from '@rujianjs/ui-kit';
import { driveLabel, type MediaController } from './media-controller';
import { diskText } from './media-status';
import { useMediaState } from './use-media-state';

/** 用户可见的文字。 */
const TEXT = {
  disk: (drive: string) => `软驱 ${drive} 的盘片`
} as const;

/** 软盘状态项的属性。 */
export interface FloppyStatusItemProps {
  readonly controller: MediaController;
}

/** 已载入的软盘镜像名；没有载入盘片的软驱不显示。 */
export function FloppyStatusItem({ controller }: FloppyStatusItemProps) {
  const { floppies } = useMediaState(controller);
  const loaded = floppies.filter((floppy) => floppy.inserted);
  return (
    <>
      {loaded.map((floppy) => {
        const drive = driveLabel(floppy.drive);
        return <StatusBarItem key={floppy.drive} text={floppies.length > 1 ? `${drive}: ${diskText(floppy)}` : diskText(floppy)} title={TEXT.disk(drive)} />;
      })}
    </>
  );
}
