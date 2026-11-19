/*
 * ------------------------------------------------------------------------
 * 名称：toolbar-page.tsx
 * 说明：工具栏参考页：用 rujian-ui-kit 的真实 Toolbar 画出与 gallery 的 toolbar 页一样的内容；宽、窄两个页面共用，宽窄由页面宽度决定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：前导是没有文字的滑块，说明文字，右边三个无底色图标按钮；必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, iconPlayerEject, iconPlayerPlay, iconPlayerStop, Slider, Stack, Toolbar } from '@rujianjs/ui-kit';

/** 工具栏页。 */
export function ToolbarPage() {
  return (
    <Stack className="reference-page">
      <Toolbar
        leading={<Slider aria-label="进度" defaultValue={40} />}
        description="共 3 项"
        actions={
          <>
            <Button variant="ghost" iconOnly compact icon={iconPlayerPlay}>
              播放
            </Button>
            <Button variant="ghost" iconOnly compact icon={iconPlayerStop}>
              停止
            </Button>
            <Button variant="ghost" iconOnly compact icon={iconPlayerEject}>
              弹出
            </Button>
          </>
        }
      />
    </Stack>
  );
}
