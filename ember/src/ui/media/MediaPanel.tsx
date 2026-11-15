/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.tsx
 * 说明：底部面板的“设备”内容：用组件库的紧凑列表行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称、盘片、状态、行尾的四个图标按钮（写保护开关、载入镜像、取出、保存镜像）；磁带机一行是名称、磁带、位置、状态与行尾的四个图标按钮（取出、播放、录音、停止）；打印机一行是名称与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：规则见 docs/05-界面.md 第 4.3 节；显示期间由控制器定时读取软驱、磁带机与打印机的状态，面板收起后停止。全部设备始终列出，与主机型号和卡带无关；软驱与磁带机是独立设备，盘片与磁带随时可以载入与取出，打印机属于主机（没有时是沉默设备）。
 *       磁带机与软驱的状态列宽度相同、都是行里最后一列，所以上下对齐。
 *       按钮只有图标，名称在悬停提示与可访问名称里；写保护是按下态的图标按钮（锁），不是开关；软盘里的文件管理不在本阶段。
 * ------------------------------------------------------------------------
 */

import { Button, iconDeviceFloppy, iconFolderOpen, iconLock, iconLockOpen, iconPlayerEject, iconPlayerPlay, iconPlayerRecord, iconPlayerStop, RowList, RowListItem } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import type { FloppyStatus, TapeStatus } from '../../contracts/media/generated/media';
import { driveLabel, type MediaController } from './media-controller';
import { diskText, floppyAvailability, floppyStateText, printerStateText, tapeAvailability, tapeNameText, tapePositionText, tapeStateText } from './media-status';
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
  tape: '磁带机',
  ejectTape: '取出',
  play: '播放',
  record: '录音',
  stop: '停止'
} as const;

/** 面板里至少列出的软驱数；软驱 A 一直存在，还没读到状态时也先列出，按钮置灰。 */
const MIN_FLOPPY_DRIVES = 1;

/** 状态列的宽度（像素），能放下“读写中（磁道 79）”；软驱与磁带机用同一个宽度、同为行里最后一列，状态才能对齐。 */
const STATE_WIDTH = 104;
/** 磁带位置列的宽度（像素）；能放下“28:09 / 28:09”。 */
const POSITION_WIDTH = 92;

/** 设备面板的属性。 */
export interface MediaPanelProps {
  readonly controller: MediaController;
}

/** 一个软驱的行；floppy 为 undefined 表示还没读到状态。 */
function FloppyRow({ controller, drive, floppy }: { readonly controller: MediaController; readonly drive: number; readonly floppy: FloppyStatus | undefined }) {
  const available = floppyAvailability(floppy);
  return (
    <RowListItem
      leading={TEXT.floppy(driveLabel(drive))}
      title={diskText(floppy)}
      cells={[{ key: 'state', content: floppyStateText(floppy), width: STATE_WIDTH, muted: true }]}
      actions={
        <>
          <Button
            variant="ghost"
            iconOnly
            compact
            icon={floppy?.writeProtected === true ? iconLock : iconLockOpen}
            pressed={floppy?.writeProtected === true}
            title={TEXT.writeProtect}
            disabled={!available.writeProtect}
            onClick={() => controller.setWriteProtected(drive, floppy?.writeProtected !== true)}
          >
            {TEXT.writeProtect}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.insert} disabled={!available.insert} onClick={() => controller.insertFloppy(drive)}>
            {TEXT.insert}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconPlayerEject} title={TEXT.eject} disabled={!available.eject} onClick={() => controller.ejectFloppy(drive)}>
            {TEXT.eject}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconDeviceFloppy} title={TEXT.save} disabled={!available.save} onClick={() => controller.saveFloppy(drive)}>
            {TEXT.save}
          </Button>
        </>
      }
    />
  );
}

/** 磁带机的行；没有接在通电的主机上时只能取出与选磁带（没有磁带时点“播放”），不能播放与录音。 */
function TapeRow({ controller, tape }: { readonly controller: MediaController; readonly tape: TapeStatus | undefined }) {
  const available = tapeAvailability(tape);
  return (
    <RowListItem
      leading={TEXT.tape}
      title={tapeNameText(tape)}
      cells={[
        { key: 'position', content: tapePositionText(tape), width: POSITION_WIDTH, muted: true },
        { key: 'state', content: tapeStateText(tape), width: STATE_WIDTH, muted: true }
      ]}
      actions={
        <>
          <Button variant="ghost" iconOnly compact icon={iconPlayerEject} title={TEXT.ejectTape} disabled={!available.eject} onClick={() => controller.ejectTape()}>
            {TEXT.ejectTape}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconPlayerPlay} title={TEXT.play} disabled={!available.play} onClick={() => controller.playTape()}>
            {TEXT.play}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconPlayerRecord} title={TEXT.record} disabled={!available.record} onClick={() => controller.recordTape()}>
            {TEXT.record}
          </Button>
          <Button variant="ghost" iconOnly compact icon={iconPlayerStop} title={TEXT.stop} disabled={!available.stop} onClick={() => controller.stopTape()}>
            {TEXT.stop}
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
  const { floppies, printer, tape } = useMediaState(controller);
  useEffect(() => controller.startPolling(), [controller]);

  const driveCount = Math.max(MIN_FLOPPY_DRIVES, floppies.length);
  return (
    <RowList className="ember-media-list" aria-label={TEXT.devices}>
      {Array.from({ length: driveCount }, (_, drive) => (
        <FloppyRow key={drive} controller={controller} drive={drive} floppy={floppies.find((item) => item.drive === drive)} />
      ))}
      <TapeRow controller={controller} tape={tape} />
      <RowListItem leading={TEXT.printer} title={printerStateText(printer)} />
    </RowList>
  );
}
