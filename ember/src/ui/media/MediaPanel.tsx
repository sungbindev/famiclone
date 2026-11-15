/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.tsx
 * 说明：底部面板的“设备”内容：用组件库的紧凑列表行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称、盘片、状态、行尾的四个图标按钮（写保护开关、载入镜像、取出、保存镜像）；磁带机一行是名称、磁带、状态、位置与行尾的五个图标按钮（载入、取出、播放、录音、停止）；打印机一行是名称与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节；显示期间由控制器定时读取软驱、磁带机与打印机的状态，面板收起后停止。主机没有任何设备时不显示，只给一句提示。
 *       按钮只有图标，名称在悬停提示与可访问名称里；写保护是按下态的图标按钮（锁），不是开关；软盘里的文件管理不在本阶段。
 *       载入镜像与载入磁带成功后调用 onLoaded（外壳的“自动收缩”开着就收起面板），取消选择与载入失败时不调用。
 * ------------------------------------------------------------------------
 */

import { Button, iconDeviceFloppy, iconFolderOpen, iconLock, iconLockOpen, iconPlayerEject, iconPlayerPlay, iconPlayerRecord, iconPlayerStop, RowList, RowListItem, State } from '@rujianjs/ui-kit';
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
  stop: '停止',
  noDevices: '没有设备：打开卡带或用整机机型开机后才有设备。'
} as const;

/** 软驱状态列的宽度（像素），能放下“读写中（磁道 79）”；软驱的各行相同才能对齐。 */
const STATE_WIDTH = 104;
/** 磁带机状态列的宽度（像素），能放下“播放中”“录音中”；让磁带名在最小窗口下也能显示一部分。 */
const TAPE_STATE_WIDTH = 56;
/** 磁带位置列的宽度（像素）；能放下“28:09 / 28:09”。 */
const POSITION_WIDTH = 92;

/** 设备面板的属性。 */
export interface MediaPanelProps {
  readonly controller: MediaController;
  /** 镜像载入成功后调用。 */
  readonly onLoaded?: () => void;
}

/** 一个软驱的行。 */
function FloppyRow({ controller, floppy, onLoaded }: { readonly controller: MediaController; readonly floppy: FloppyStatus; readonly onLoaded: (() => void) | undefined }) {
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
          <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.insert} disabled={!available.insert} onClick={() => controller.insertFloppy(floppy.drive, onLoaded)}>
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

/** 磁带机的行。 */
function TapeRow({ controller, tape, onLoaded }: { readonly controller: MediaController; readonly tape: TapeStatus; readonly onLoaded: (() => void) | undefined }) {
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
          <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.loadTape} disabled={!available.load} onClick={() => controller.loadTape(onLoaded)}>
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
export function MediaPanel({ controller, onLoaded }: MediaPanelProps) {
  const { floppies, printer, tape } = useMediaState(controller);
  useEffect(() => controller.startPolling(), [controller]);

  const hasTape = tape?.available === true;
  if (floppies.length === 0 && !hasTape) return <State text={TEXT.noDevices} />;
  return (
    <RowList className="ember-media-list" aria-label={TEXT.devices}>
      {floppies.map((floppy) => (
        <FloppyRow key={floppy.drive} controller={controller} floppy={floppy} onLoaded={onLoaded} />
      ))}
      {hasTape ? <TapeRow controller={controller} tape={tape} onLoaded={onLoaded} /> : null}
      {floppies.length > 0 ? <RowListItem leading={TEXT.printer} title={printerStateText(printer)} /> : null}
    </RowList>
  );
}
