/*
 * ------------------------------------------------------------------------
 * 名称：MediaSideBar.tsx
 * 说明：设备边栏：每个设备一张卡片。软驱卡片显示盘片、状态与写保护开关，按钮有载入镜像、取出、保存镜像；打印机卡片只显示状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节；显示期间由控制器定时读取软驱与打印机的状态，隐藏后停止。没有开机或主机没有软驱时不显示卡片，只给一句提示。
 *       磁带机以后再做；软驱指示灯与软盘文件（右边栏）也不在本阶段。
 * ------------------------------------------------------------------------
 */

import { Button, Facts, iconDeviceFloppy, iconFolderOpen, iconPlayerEject, Panel, Stack, State, Switch } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import type { FloppyStatus } from '../../contracts/media/generated/media';
import { driveLabel, type MediaController } from './media-controller';
import { diskText, floppyAvailability, floppyStateText, printerStateText } from './media-status';
import { useMediaState } from './use-media-state';
import './ember-media.css';

/** 用户可见的文字。 */
const TEXT = {
  floppy: (drive: string) => `软驱 ${drive}`,
  printer: '打印机',
  disk: '盘片',
  state: '状态',
  writeProtect: '写保护',
  insert: '载入镜像',
  eject: '取出',
  save: '保存镜像',
  noDevices: '没有设备：整机机型开机后才有软驱与打印机。'
} as const;

/** 设备边栏的属性。 */
export interface MediaSideBarProps {
  readonly controller: MediaController;
}

/** 一个软驱的卡片。 */
function FloppyCard({ controller, floppy }: { readonly controller: MediaController; readonly floppy: FloppyStatus }) {
  const available = floppyAvailability(floppy);
  const label = driveLabel(floppy.drive);
  return (
    <Panel title={TEXT.floppy(label)} compactHead>
      <div className="ember-media-card__body">
        <Facts
          aria-label={TEXT.floppy(label)}
          items={[
            { label: TEXT.disk, value: diskText(floppy) },
            { label: TEXT.state, value: floppyStateText(floppy) }
          ]}
        />
        <Switch label={TEXT.writeProtect} checked={floppy.writeProtected} disabled={!available.writeProtect} onChange={(checked) => controller.setWriteProtected(floppy.drive, checked)} />
        <div className="ember-media-card__buttons">
          <Button icon={iconFolderOpen} disabled={!available.insert} onClick={() => controller.insertFloppy(floppy.drive)}>
            {TEXT.insert}
          </Button>
          <Button icon={iconPlayerEject} disabled={!available.eject} onClick={() => controller.ejectFloppy(floppy.drive)}>
            {TEXT.eject}
          </Button>
          <Button icon={iconDeviceFloppy} disabled={!available.save} onClick={() => controller.saveFloppy(floppy.drive)}>
            {TEXT.save}
          </Button>
        </div>
      </div>
    </Panel>
  );
}

/**
 * 设备边栏。
 * @param props 属性。
 */
export function MediaSideBar({ controller }: MediaSideBarProps) {
  const { floppies, printer } = useMediaState(controller);
  useEffect(() => controller.startPolling(), [controller]);

  if (floppies.length === 0) return <State text={TEXT.noDevices} />;
  return (
    <Stack className="ember-media-side">
      {floppies.map((floppy) => (
        <FloppyCard key={floppy.drive} controller={controller} floppy={floppy} />
      ))}
      <Panel title={TEXT.printer} compactHead>
        <div className="ember-media-card__body">
          <Facts aria-label={TEXT.printer} items={[{ label: TEXT.state, value: printerStateText(printer) }]} />
        </div>
      </Panel>
    </Stack>
  );
}
