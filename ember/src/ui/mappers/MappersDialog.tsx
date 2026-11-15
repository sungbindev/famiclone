/*
 * ------------------------------------------------------------------------
 * 名称：MappersDialog.tsx
 * 说明：“mapper 插件”对话框的内容：顶部是搜索框、来源筛选与“安装插件”按钮，下面是插件表格（名称、版本、匹配、主机型号、来源、状态），选中一行后在表格下方显示清单摘要与“停用/启用”“卸载”按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：预装插件只能停用，不能卸载（卸载按钮置灰）；卸载前用确认对话框询问；安装与卸载的结果、失败的原因用外壳通知，页面不显示操作结果。规则见 docs/05-界面.md 第 4.4 节与 docs/03 第 7 节。
 * ------------------------------------------------------------------------
 */

import { Actions, Button, Facts, iconPlus, iconTrash, Panel, ScrollArea, Select, Stack, State, StatusLabel, Table, TableMainCell, TextInput, Toolbar, useDialog, type TableColumn } from '@rujianjs/ui-kit';
import { useState } from 'react';
import type { MapperSummary } from '../../contracts/mappers/generated/mappers';
import { useShell } from '../shell/shell-context';
import type { ShellPageProps } from '../shell/shell-registry';
import { detailFacts, errorText, filterPlugins, hostsText, matchesText, sourceLabel, type SourceFilter } from './mappers-model';
import { useMappers } from './use-mappers';
import './ember-mappers.css';

/** 用户可见的文字。 */
const TEXT = {
  search: '搜索插件',
  searchPlaceholder: '搜索名称、标识或发布者',
  sourceFilter: '来源',
  sourceAll: '全部来源',
  install: '安装插件',
  uninstall: '卸载',
  uninstallTitle: '卸载插件',
  uninstallMessage: (name: string) => `卸载“${name}”？卸载后使用它的卡带将不能运行，可以重新安装插件包。`,
  bundledHint: '预装插件不能卸载，只能停用',
  enable: '启用',
  disable: '停用',
  table: 'mapper 插件',
  columns: { name: '名称', version: '版本', match: '匹配', host: '主机型号', source: '来源', state: '状态' },
  enabled: '已启用',
  disabled: '已停用',
  loading: '正在读取…',
  empty: '没有符合条件的插件',
  detailLoading: '正在读取清单…',
  installFailed: '安装插件失败',
  uninstallFailed: '卸载插件失败',
  toggleFailed: '修改插件的启用状态失败',
  installed: (plugin: MapperSummary) => `已安装插件：${plugin.name} ${plugin.version}`,
  uninstalled: (name: string) => `已卸载插件：${name}`
} as const;

/** 来源筛选的选项。 */
const SOURCE_OPTIONS = [
  { value: 'all', label: TEXT.sourceAll },
  { value: 'bundled', label: '预装' },
  { value: 'user', label: '用户安装' }
] as const;

/** 表格的列。 */
const COLUMNS: readonly TableColumn<MapperSummary>[] = [
  { title: TEXT.columns.name, render: (row) => <TableMainCell text={row.name} description={row.description} />, minWidth: 140 },
  { title: TEXT.columns.version, key: 'version', nowrap: true, muted: true },
  { title: TEXT.columns.match, render: (row) => matchesText(row.match), muted: true, tooltip: (row) => matchesText(row.match), minWidth: 120 },
  { title: TEXT.columns.host, render: (row) => hostsText(row.host), nowrap: true, muted: true },
  { title: TEXT.columns.source, render: (row) => sourceLabel(row.source), nowrap: true, muted: true },
  { title: TEXT.columns.state, render: (row) => <StatusLabel tone={row.enabled ? 'success' : 'stopped'}>{row.enabled ? TEXT.enabled : TEXT.disabled}</StatusLabel>, nowrap: true }
];

/**
 * “mapper 插件”对话框的内容。
 * @param props 外壳传入的属性。
 */
export function MappersDialog({ bridge }: ShellPageProps) {
  const { notify } = useShell();
  const dialog = useDialog();
  const data = useMappers(bridge);
  const [search, setSearch] = useState('');
  const [source, setSource] = useState<SourceFilter>('all');

  const install = async (): Promise<void> => {
    try {
      const plugin = await data.install();
      if (plugin !== undefined) notify(TEXT.installed(plugin), 'success');
    } catch (error) {
      notify(errorText(error, TEXT.installFailed), 'error');
    }
  };

  const toggle = async (plugin: MapperSummary): Promise<void> => {
    try {
      await data.setEnabled(plugin.id, !plugin.enabled);
    } catch (error) {
      notify(errorText(error, TEXT.toggleFailed), 'error');
    }
  };

  const uninstall = async (plugin: MapperSummary): Promise<void> => {
    const confirmed = await dialog.confirm({ title: TEXT.uninstallTitle, message: TEXT.uninstallMessage(plugin.name), confirmText: TEXT.uninstall, variant: 'danger' });
    if (!confirmed) return;
    try {
      await data.uninstall(plugin.id);
      notify(TEXT.uninstalled(plugin.name), 'success');
    } catch (error) {
      notify(errorText(error, TEXT.uninstallFailed), 'error');
    }
  };

  const plugins = data.state.kind === 'ready' ? data.state.plugins : [];
  const shown = filterPlugins(plugins, source, search);
  const selected = plugins.find((plugin) => plugin.id === data.selectedId);

  let status = null;
  if (data.state.kind === 'failed') status = <State kind="error" text={data.state.message} />;
  else if (data.state.kind === 'loading') status = <State kind="busy" text={TEXT.loading} />;
  else if (shown.length === 0) status = <State text={TEXT.empty} />;

  let detail = null;
  if (selected !== undefined) {
    let facts = <State kind="busy" text={TEXT.detailLoading} />;
    if (data.detail.kind === 'failed') facts = <State kind="error" text={data.detail.message} />;
    else if (data.detail.kind === 'ready') {
      facts = (
        <Facts
          aria-label={selected.name}
          items={detailFacts(data.detail.detail).map((fact) => ({
            label: fact.label,
            value: fact.lines.map((line, index) => <div key={index}>{line}</div>)
          }))}
        />
      );
    }
    detail = (
      <Panel
        fixed
        compactHead
        title={selected.name}
        aside={
          <Actions>
            <Button variant="secondary" compact onClick={() => void toggle(selected)}>
              {selected.enabled ? TEXT.disable : TEXT.enable}
            </Button>
            <Button variant="danger" compact icon={iconTrash} disabled={selected.source === 'bundled'} title={selected.source === 'bundled' ? TEXT.bundledHint : TEXT.uninstall} onClick={() => void uninstall(selected)}>
              {TEXT.uninstall}
            </Button>
          </Actions>
        }
      >
        {facts}
      </Panel>
    );
  }

  return (
    <ScrollArea bleed horizontal="hidden">
      <Stack>
        <Toolbar
          leading={
            <span className="ember-mappers__search">
              <TextInput type="search" aria-label={TEXT.search} placeholder={TEXT.searchPlaceholder} value={search} onChange={setSearch} />
            </span>
          }
          actions={
            <>
              <Select aria-label={TEXT.sourceFilter} options={SOURCE_OPTIONS} value={source} allowEmpty={false} onChange={(next) => setSource(next as SourceFilter)} />
              <Button variant="primary" icon={iconPlus} onClick={() => void install()}>
                {TEXT.install}
              </Button>
            </>
          }
        />
        {status}
        {shown.length === 0 ? null : (
          <Table
            compact
            flush
            aria-label={TEXT.table}
            columns={COLUMNS}
            rows={shown}
            rowKey={(row) => row.id}
            isRowSelected={(row) => row.id === data.selectedId}
            onRowSelect={(row) => data.select(row.id === data.selectedId ? undefined : row.id)}
          />
        )}
        {detail}
      </Stack>
    </ScrollArea>
  );
}
