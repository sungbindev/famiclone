/*
 * ------------------------------------------------------------------------
 * 名称：table-page.tsx
 * 说明：表格参考页：用 rujian-ui-kit 的真实 Table（紧凑、平铺）画出与 gallery 的 table 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：四列三行，第二行选中并带详情（高 24 的强调色块）；文字与列设置必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { type TableColumn, Stack, StatusLabel, Table, TableMainCell } from '@rujianjs/ui-kit';

/** 一行数据。 */
interface PluginRow {
  id: string;
  name: string;
  description: string;
  version: string;
  match: string;
  enabled: boolean;
}

/** 数据。 */
const ROWS: readonly PluginRow[] = [
  { id: 'a', name: '插件甲', description: '第一段说明', version: '1.0', match: 'NES 卡带', enabled: true },
  { id: 'b', name: '插件乙', description: '说明乙', version: '2.1.3', match: 'SB2000 软盘', enabled: false },
  { id: 'c', name: '插件丙', description: '', version: '0.9', match: '全部', enabled: true }
];

/** 列。 */
const COLUMNS: readonly TableColumn<PluginRow>[] = [
  { title: '名称', render: (row) => <TableMainCell text={row.name} description={row.description} />, minWidth: 140 },
  { title: '版本', key: 'version', nowrap: true, muted: true },
  { title: '匹配', key: 'match', muted: true, minWidth: 120 },
  { title: '状态', render: (row) => <StatusLabel tone={row.enabled ? 'success' : 'stopped'}>{row.enabled ? '已启用' : '已停用'}</StatusLabel>, nowrap: true }
];

/** 表格页。 */
export function TablePage() {
  return (
    <Stack className="reference-page">
      <Table
        compact
        flush
        aria-label="插件"
        columns={COLUMNS}
        rows={ROWS}
        rowKey={(row) => row.id}
        isRowSelected={(row) => row.id === 'b'}
        onRowSelect={() => undefined}
        renderRowDetail={(row) => (row.id === 'b' ? <div style={{ height: 24, background: 'var(--accent)' }} /> : null)}
      />
    </Stack>
  );
}
