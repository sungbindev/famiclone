/*
 * ------------------------------------------------------------------------
 * 名称：menu-page.tsx
 * 说明：菜单参考页：用 rujian-ui-kit 的真实菜单栏画出与 gallery 的 menu 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：每个菜单栏只有一个菜单；样本是菜单栏外层元素，展开后弹层与弹层里的各个菜单项依次排在所有样本之后（与 gallery 输出的位置一致）；
 *       菜单内容与样本顺序必须与 gallery/pages.cpp 的 buildMenuPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { iconDeviceFloppy, iconFolderOpen, iconPlus, iconPower, MenuBar, type MenuBarMenu, Row } from '@rujianjs/ui-kit';

const FILE_MENU: MenuBarMenu = {
  id: 'file',
  label: '文件',
  mnemonic: 'F',
  items: [
    { id: 'new', label: '新建', icon: iconPlus, shortcut: 'Ctrl+N' },
    { id: 'open', label: '打开', icon: iconFolderOpen, shortcut: 'Ctrl+O', disabled: true },
    { id: 'save', label: '保存', icon: iconDeviceFloppy, shortcut: 'Ctrl+S' },
    { id: 'exit', label: '退出', icon: iconPower, mnemonic: 'x', separatorBefore: true }
  ]
};

const VIEW_MENU: MenuBarMenu = {
  id: 'view',
  label: '视图',
  mnemonic: 'V',
  items: [
    { id: 'full', label: '全屏', checked: true, shortcut: 'F11' },
    { id: 'dark', label: '深色', checked: true, radio: true },
    { id: 'light', label: '浅色', checked: false, radio: true },
    {
      id: 'zoom',
      label: '缩放',
      separatorBefore: true,
      items: [
        { id: 'zoom-in', label: '放大' },
        { id: 'zoom-out', label: '缩小' }
      ]
    },
    { id: 'replay', label: '回放', hint: '暂未支持', disabled: true }
  ]
};

const HELP_MENU: MenuBarMenu = {
  id: 'help',
  label: '帮助',
  mnemonic: 'H',
  items: [
    { id: 'manual', label: '手册' },
    { id: 'update', label: '检查更新', hint: '暂未支持' },
    { id: 'about', label: '关于', separatorBefore: true }
  ]
};

/** 菜单页。 */
export function MenuPage() {
  return (
    <Row className="reference-page reference-chrome">
      <MenuBar menus={[FILE_MENU]} />
      <MenuBar menus={[VIEW_MENU]} />
      <MenuBar menus={[HELP_MENU]} />
    </Row>
  );
}
