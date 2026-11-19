/*
 * ------------------------------------------------------------------------
 * 名称：title-bar-page.tsx
 * 说明：标题栏参考页：用 rujian-ui-kit 的真实标题栏画出与 gallery 的 title-bar 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：样本依次是标题栏、菜单栏、操作区的按钮、三个折叠按钮；网页里没有系统窗口按钮，标题栏右侧不留空；
 *       样本顺序与内容必须与 gallery/pages.cpp 的 buildTitleBarPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, iconBell, iconEye, iconSettings, MenuBar, TitleBar } from '@rujianjs/ui-kit';

/** 应用图标的占位：主题色的圆角方块。 */
function Logo() {
  return <span style={{ display: 'block', background: 'var(--accent)', borderRadius: 4 }} />;
}

/** 标题栏页。 */
export function TitleBarPage() {
  return (
    <TitleBar
      logo={<Logo />}
      title="Ember"
      subtitle="模拟器"
      menuBar={
        <MenuBar
          menus={[
            {
              id: 'file',
              label: '文件',
              mnemonic: 'F',
              items: [
                { id: 'open', label: '打开' },
                { id: 'exit', label: '退出' },
                { id: 'about', label: '关于' }
              ]
            }
          ]}
        />
      }
      actions={<Button>更新</Button>}
      tools={[
        { id: 'side', label: '边栏', icon: iconSettings, expanded: true },
        { id: 'bottom', label: '底部面板', icon: iconEye, expanded: false },
        { id: 'right', label: '右边栏', icon: iconBell, expanded: false, disabled: true }
      ]}
    />
  );
}
