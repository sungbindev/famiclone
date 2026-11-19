/*
 * ------------------------------------------------------------------------
 * 名称：row-list-page.tsx
 * 说明：信息行列表参考页：用 rujian-ui-kit 的真实 RowList、RowListItem 画出与 gallery 的 row-list 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：三行：前导加信息列加三个操作、两列（定宽与自然宽）加两个操作、只有前导与标题；必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, iconFolderOpen, iconLock, iconPlayerEject, iconPlayerPlay, iconPlayerStop, RowList, RowListItem, Stack } from '@rujianjs/ui-kit';

/** 信息行列表页。 */
export function RowListPage() {
  return (
    <Stack className="reference-page">
      <RowList>
        <RowListItem
          leading="软驱 A"
          title="无软盘"
          cells={[{ key: 'state', content: '未就绪', muted: true }]}
          actions={
            <>
              <Button variant="ghost" iconOnly compact icon={iconLock}>
                写保护
              </Button>
              <Button variant="ghost" iconOnly compact icon={iconFolderOpen}>
                插入
              </Button>
              <Button variant="ghost" iconOnly compact icon={iconPlayerEject}>
                弹出
              </Button>
            </>
          }
        />
        <RowListItem
          leading="磁带"
          title="无磁带"
          cells={[
            { key: 'position', content: '00:00', width: 64, muted: true },
            { key: 'state', content: '已停止', muted: true }
          ]}
          actions={
            <>
              <Button variant="ghost" iconOnly compact icon={iconPlayerPlay}>
                播放
              </Button>
              <Button variant="ghost" iconOnly compact icon={iconPlayerStop}>
                停止
              </Button>
            </>
          }
        />
        <RowListItem leading="打印机" title="空闲" />
      </RowList>
    </Stack>
  );
}
