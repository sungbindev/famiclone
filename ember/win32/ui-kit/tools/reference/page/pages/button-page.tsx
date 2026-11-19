/*
 * ------------------------------------------------------------------------
 * 名称：button-page.tsx
 * 说明：按钮参考页：用 rujian-ui-kit 的真实按钮画出与 gallery 的 button 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：data-sample 的顺序必须与 gallery/pages.cpp 的 buildButtonPage 里 addButton 的顺序一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, iconPlayerPlay, iconPlayerStop, iconTrash, iconVolume, Row, Stack } from '@rujianjs/ui-kit';

/** 按钮页。 */
export function ButtonPage() {
  return (
    <Stack className="reference-page">
      <Row>
        <Button data-sample>取消</Button>
        <Button data-sample variant="primary">
          确定
        </Button>
        <Button data-sample>保存</Button>
        <Button data-sample variant="danger">
          删除
        </Button>
        <Button data-sample icon={false} disabled>
          禁用
        </Button>
      </Row>
      <Row>
        <Button data-sample variant="ghost" iconOnly compact icon={iconPlayerPlay}>
          播放
        </Button>
        <Button data-sample variant="ghost" iconOnly compact icon={iconPlayerStop} disabled>
          停止
        </Button>
        <Button data-sample variant="ghost" iconOnly compact icon={iconVolume} pressed>
          静音
        </Button>
        <Button data-sample icon={false} pressed>
          静音
        </Button>
      </Row>
      <Row>
        <Button data-sample variant="danger" compact icon={iconTrash}>
          卸载
        </Button>
        <Button data-sample>下一步</Button>
        <Button data-sample variant="ghost" icon={false}>
          更多
        </Button>
        <Button data-sample variant="link">
          选择目录
        </Button>
      </Row>
    </Stack>
  );
}
