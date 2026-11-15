/*
 * ------------------------------------------------------------------------
 * 名称：AboutDialog.tsx
 * 说明：“关于 Ember”对话框的内容：标志与名称、应用版本、核心版本、许可证，以及“第三方依赖”小节（一行说明和一张表：名称与版本、作者、许可）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：第三方依赖的清单由构建脚本从各项目的声明生成，由组装根传入，这里不手写；版本经 system.version 读取，读不到时显示“读取失败”，不用通知。规则见 docs/05-界面.md 第 8 节。
 * ------------------------------------------------------------------------
 */

import { Facts, Heading, ScrollArea, Stack, Table, type TableColumn } from '@rujianjs/ui-kit';
import { useEffect, useState } from 'react';
import { EmberLogo } from '../shell/EmberLogo';
import type { ShellPageProps } from '../shell/shell-registry';
import './ember-about.css';

/** 一项第三方开源代码的声明。 */
export interface ThirdPartyNotice {
  readonly name: string;
  readonly version: string;
  readonly authors: readonly string[];
  readonly license: string;
  readonly homepage: string;
  readonly usage: string;
}

/** 关于对话框的属性。 */
export interface AboutDialogProps extends ShellPageProps {
  /** 第三方依赖清单。 */
  readonly notices: readonly ThirdPartyNotice[];
}

/** 用户可见的文字。 */
const TEXT = {
  appName: 'Ember',
  version: '版本',
  coreVersion: '核心版本',
  license: '许可证',
  licenseName: 'GNU 通用公共许可证第 3 版（GPL-3.0）',
  loading: '正在读取…',
  failed: '读取失败',
  thirdParty: '第三方依赖',
  thirdPartyHint: 'Ember 使用了下列开源项目，各项目的许可全文随安装包分发。',
  thirdPartyTable: '第三方依赖',
  columnName: '名称与版本',
  columnAuthors: '作者',
  columnLicense: '许可'
} as const;

/** 版本的读取状态。 */
type Versions = { readonly kind: 'loading' } | { readonly kind: 'failed' } | { readonly kind: 'ready'; readonly app: string; readonly core: string };

/** 表格的列。 */
const COLUMNS: readonly TableColumn<ThirdPartyNotice>[] = [
  { title: TEXT.columnName, render: (row) => `${row.name} ${row.version}`, tooltip: (row) => row.usage, minWidth: 160 },
  { title: TEXT.columnAuthors, render: (row) => row.authors.join('、'), muted: true, minWidth: 120 },
  { title: TEXT.columnLicense, key: 'license', nowrap: true, muted: true }
];

/** 版本在清单里显示的文字。 */
function versionText(versions: Versions, pick: (ready: { app: string; core: string }) => string): string {
  if (versions.kind === 'loading') return TEXT.loading;
  return versions.kind === 'failed' ? TEXT.failed : pick(versions);
}

/**
 * “关于 Ember”对话框的内容。
 * @param props 属性。
 */
export function AboutDialog({ bridge, notices }: AboutDialogProps) {
  const [versions, setVersions] = useState<Versions>({ kind: 'loading' });

  useEffect(() => {
    let alive = true;
    bridge.core.system.version({}).then(
      (result) => {
        if (alive) setVersions({ kind: 'ready', app: result.appVersion, core: result.coreVersion });
      },
      () => {
        if (alive) setVersions({ kind: 'failed' });
      }
    );
    return () => {
      alive = false;
    };
  }, [bridge]);

  return (
    <ScrollArea bleed horizontal="hidden">
      <Stack>
        <div className="ember-about__title">
          <span className="ember-about__logo">
            <EmberLogo />
          </span>
          <Heading>{TEXT.appName}</Heading>
        </div>
        <Facts
          items={[
            { label: TEXT.version, value: versionText(versions, (ready) => ready.app) },
            { label: TEXT.coreVersion, value: versionText(versions, (ready) => ready.core) },
            { label: TEXT.license, value: TEXT.licenseName }
          ]}
        />
        <Heading>{TEXT.thirdParty}</Heading>
        <p className="ember-about__hint">{TEXT.thirdPartyHint}</p>
        <Table compact flush aria-label={TEXT.thirdPartyTable} columns={COLUMNS} rows={notices} rowKey={(row) => row.name} />
      </Stack>
    </ScrollArea>
  );
}
