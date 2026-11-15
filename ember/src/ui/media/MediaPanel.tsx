/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.tsx
 * 说明：底部面板的“设备”内容：用组件库的紧凑列表行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称、盘片、状态、行尾的四个图标按钮（写保护开关、载入镜像、取出、保存镜像）；打印机一行是名称与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节；显示期间由控制器定时读取软驱与打印机的状态，面板收起后停止。没有开机或主机没有软驱时不显示设备，只给一句提示。
 *       按钮只有图标，名称在悬停提示与可访问名称里；写保护是按下态的图标按钮（锁），不是开关；磁带机以后再做；软盘文件管理也不在本阶段。
 * ------------------------------------------------------------------------
 */

import { Button, iconDeviceFloppy, iconFolderOpen, iconLock, iconLockOpen, iconPlayerEject, RowList, RowListItem, State } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import type { FloppyStatus } from '../../contracts/media/generated/media';
import { driveLabel, type MediaController } from './media-controller';
import { diskText, floppyAvailability, floppyStateText, printerStateText } from './media-status';
import { useMediaState } from './use-media-state';
import './ember-media.css';

/** 用户可见的文字。 */
const TEXT = {
  devices: '设备',
  floppy: (drive: string) => `软驱 ${drive}`,
  printer: '打印机',
  writeProtect: '写保护',
  insert: '载入镜像',
  eject: '取出',
  save: '保存镜像',
  noDevices: '没有设备：整机机型开机后才有软驱与打印机。'
} as const;

/** 状态列的宽度（像素），各行相同才能对齐。 */
const STATE_WIDTH = 128;

/** 设备面板的属性。 */
export interface MediaPanelProps {
  readonly controller: MediaController;
}

/** 一个软驱的行。 */
function FloppyRow({ controller, floppy }: { readonly controller: MediaController; readonly floppy: FloppyStatus }) {
  const available = floppyAvailability(floppy);
  return (
    <RowListItem
      leading={TEXT.floppy(driveLabel(floppy.drive))}
      title={diskText(floppy)}
      cells={[{ key: 'state', content: floppyStateText(floppy), width: STATE_WIDTH, muted: true }]}
      actions={
        <>
          <Button
            variant="ghost"
            iconOnly
            compact
            icon={floppy.writeProtected ? iconLock : iconLockOpen}
            pressed={floppy.writeProtected}
            title={TEXT.writeProtect}
            disabled={!available.writeProtect}
            onClick={() => controller.setWriteProtected(floppy.drive, !floppy.writeProtected)}
          >
            {TEXT.writeProtect}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.insert} disabled={!available.insert} onClick={() => controller.insertFloppy(floppy.drive)}>
            {TEXT.insert}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconPlayerEject} title={TEXT.eject} disabled={!available.eject} onClick={() => controller.ejectFloppy(floppy.drive)}>
            {TEXT.eject}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconDeviceFloppy} title={TEXT.save} disabled={!available.save} onClick={() => controller.saveFloppy(floppy.drive)}>
            {TEXT.save}
          </Button>
        </>
      }
    />
  );
}

/**
 * 底部面板的“设备”内容。
 * @param props 属性。
 */
export function MediaPanel({ controller }: MediaPanelProps) {
  const { floppies, printer } = useMediaState(controller);
  useEffect(() => controller.startPolling(), [controller]);

  if (floppies.length === 0) return <State text={TEXT.noDevices} />;
  return (
    <RowList className="ember-media-list" aria-label={TEXT.devices}>
      {floppies.map((floppy) => (
        <FloppyRow key={floppy.drive} controller={controller} floppy={floppy} />
      ))}
      <RowListItem leading={TEXT.printer} title={printerStateText(printer)} />
    </RowList>
  );
}
