/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.tsx
 * 说明：底部面板的“设备”内容：用组件库的紧凑列表行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称、盘片、状态、行尾的四个图标按钮（写保护开关、载入镜像、取出、保存镜像）；磁带机一行是名称、磁带、状态、位置与行尾的五个图标按钮（载入、取出、播放、录音、停止）；打印机一行是名称与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节；显示期间由控制器定时读取软驱、磁带机与打印机的状态，面板收起后停止。全部设备始终列出，与主机型号和卡带无关；当前主机没有的设备是沉默设备，内容为空、按钮置灰。
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
  loadTape: '载入磁带',
  ejectTape: '取出',
  play: '播放',
  record: '录音',
  stop: '停止'
} as const;

/** 面板里至少列出的软驱数；当前主机没有的软驱照样列出，按钮置灰。 */
const MIN_FLOPPY_DRIVES = 1;

/** 软驱状态列的宽度（像素），能放下“读写中（磁道 79）”；软驱的各行相同才能对齐。 */
const STATE_WIDTH = 104;
/** 磁带机状态列的宽度（像素），能放下“播放中”“录音中”；让磁带名在最小窗口下也能显示一部分。 */
const TAPE_STATE_WIDTH = 56;
/** 磁带位置列的宽度（像素）；能放下“28:09 / 28:09”。 */
const POSITION_WIDTH = 92;

/** 设备面板的属性。 */
export interface MediaPanelProps {
  readonly controller: MediaController;
}

/** 一个软驱的行；floppy 为 undefined 表示当前主机没有这个软驱（沉默设备）。 */
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

/** 磁带机的行；tape 为 undefined 或 available 为 false 表示当前主机没有磁带机（沉默设备）。 */
function TapeRow({ controller, tape }: { readonly controller: MediaController; readonly tape: TapeStatus | undefined }) {
  const available = tapeAvailability(tape);
  return (
    <RowListItem
      leading={TEXT.tape}
      title={tapeNameText(tape)}
      cells={[
        { key: 'state', content: tapeStateText(tape), width: TAPE_STATE_WIDTH, muted: true },
        { key: 'position', content: tapePositionText(tape), width: POSITION_WIDTH, muted: true }
      ]}
      actions={
        <>
          <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.loadTape} disabled={!available.load} onClick={() => controller.loadTape()}>
            {TEXT.loadTape}
          </Button>
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
