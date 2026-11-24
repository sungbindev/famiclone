/*
 * ------------------------------------------------------------------------
 * 名称：shots.mjs
 * 说明：首页用到的截图清单：文件名（放在 site/assets/shots 下）、替代文字与图注。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：截图只裁到应用窗口本身，不带桌面背景；清单里的文件缺失时构建直接报错。
 * ------------------------------------------------------------------------
 */

/** 首页的大图：模拟器运行中的窗口。 */
export const HERO_SHOT = { file: 'ember-nes.png', alt: 'Ember 运行 NES 程序，画面上显示 HELLO, NES!', caption: 'Ember 运行 FC Dev 构建的 NES 程序' };

/** 模拟器部分的截图。 */
export const EMBER_SHOTS = [
  { file: 'ember-sb2000.png', alt: 'Ember 运行小霸王学习机，SB-DOS 里运行 hello 示例程序', caption: '小霸王 2000 开机，在 SB-DOS 里运行示例程序' },
  { file: 'ember-preferences.png', alt: 'Ember 的首选项对话框，显示页', caption: '首选项：画面比例、缩放方式与画面效果' },
  { file: 'ember-machines.png', alt: 'Ember 的机型与 ROM 对话框', caption: '机型与 ROM：按机型添加学习机的 ROM 文件' }
];

/** FC Dev 扩展部分的截图。 */
export const FCDEV_SHOTS = [
  { file: 'fcdev-editor.png', alt: 'VS Code 中打开 FC Dev 的 SB2000 示例程序 hello.c', caption: 'VS Code 里打开 FC Dev 的 SB2000 示例程序' },
  { file: 'fcdev-build.png', alt: 'FC Dev 构建任务的输出，生成 HELLO.EXE 与磁盘镜像 HELLO.img', caption: '“FCDEV：构建”输出 HELLO.EXE 与软盘镜像 HELLO.img' }
];
