/*
 * ------------------------------------------------------------------------
 * 名称：HomePage.tsx
 * 说明：首页：开头介绍，然后是 Ember 模拟器、fcdev 扩展两部分（各有功能卡片与截图），最后是帮助文档的入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：功能卡片用组件库的 Panel，标题用 Title；截图点开看原图（新窗口）。
 * ------------------------------------------------------------------------
 */

import { Panel, Title } from '@rujianjs/ui-kit';
import type { LandingView, PageView, ShotView } from './types';

/** 模拟器的功能卡片：标题与说明。 */
const EMBER_FEATURES = [
  { title: '运行 NES 卡带', text: '打开 .nes 文件或直接拖进窗口，按清单匹配 mapper 插件；键位与手柄绑定在“设置→手柄设置”里修改。' },
  { title: '学习机整机', text: '“机型与 ROM”里列出小霸王、步步高、金字塔、裕兴等学习机，添加 ROM 文件后选择机型即可开机；支持软盘镜像、磁带与打印机。' },
  { title: '调试服务', text: '开启后用 WebSocket JSON-RPC 读写内存与寄存器、设置断点与单步、记录指令跟踪、截图。fcdev 与 Copilot 工具都经它调试。' },
  { title: 'mapper 插件', text: '用清单与脚本描述卡带的存储区与寄存器，插件包安装即用，手册里有测试与排错的方法。' },
  { title: '画面与声音', text: '画面用 Direct3D 11 显示，没有显卡时回退到软件渲染；声音直接输出到系统，支持全屏与窗口置顶。' },
  { title: 'Windows 原生', text: '原生外壳支持 Windows 7 SP1 到 Windows 11（64 位），下载的压缩包解压即用，没有安装步骤。' }
];

/** fcdev 的功能卡片：标题与说明。 */
const FCDEV_FEATURES = [
  { title: '新建与构建', text: '内置模板新建 NES 或 SB2000 项目，fcdev.json 配置目标，一键构建；学习机程序自动生成软盘镜像。' },
  { title: '运行与调试', text: '一键在 Ember 里运行，也可以调试运行：设断点、单步、看内存与寄存器，程序停在哪一步都能查。' },
  { title: 'C 与汇编', text: 'NES 支持 C 与 ca65 汇编；SB2000 提供 C 接口、系统调用、符号与硬件参考，示例源码可以直接对照。' },
  { title: '反汇编', text: '把 NES 卡带或 SB-DOS 程序反汇编回 ca65 工程，重新汇编后与原文件逐字节一致。' },
  { title: 'Copilot 工具', text: '构建、运行、反汇编与模拟器调试都注册成 Copilot 工具，可以在对话里直接让 AI 调用。' },
  { title: '手册与参考', text: '从安装、构建到系统调用、硬件说明与示例程序，全部手册随扩展发布，也在本站可查。' }
];

/**
 * 截图卡片。
 * @param props 截图。
 */
function Shot({ shot, className }: { shot: ShotView; className?: string }) {
  return (
    <figure className={className ? `shot ${className}` : 'shot'}>
      <a href={shot.src} target="_blank" rel="noopener">
        <img src={shot.src} alt={shot.alt} loading="lazy" />
      </a>
      <figcaption>{shot.caption}</figcaption>
    </figure>
  );
}

/**
 * 首页。
 * @param props 视图与首页内容。
 */
export function HomePage({ view, landing }: { view: PageView; landing: LandingView }) {
  const { links, counts } = landing;
  const total = counts.manual + counts.plugin + counts.debug + counts.fcdev;
  return (
    <div className="home">
      <section className="hero">
        <div className="hero__text">
          <p className="kicker">FC · 学习机 · 6502</p>
          <h1 className="hero__title">Ember 模拟器与 fcdev 开发工具</h1>
          <p className="hero__lead">
            Ember 运行 NES 卡带与学习机整机，带有调试服务、断点、单步与截图。fcdev 是配套的 VS Code 扩展，用 cc65 把 C 或汇编构建成程序，并在 Ember 里运行与调试。
          </p>
          <div className="hero__actions">
            <a className="ui-button ui-button--primary" href={links.manualStart}>
              <span>快速开始</span>
            </a>
            <a className="ui-button ui-button--secondary" href={links.fcdevStart}>
              <span>fcdev 手册</span>
            </a>
          </div>
          <dl className="hero__stats">
            <div>
              <dt>帮助文档</dt>
              <dd>{total} 页</dd>
            </div>
            <div>
              <dt>发布方式</dt>
              <dd>解压即用</dd>
            </div>
            <div>
              <dt>适用系统</dt>
              <dd>Windows 7 SP1 至 11（64 位）</dd>
            </div>
          </dl>
        </div>
        <Shot shot={landing.hero} className="shot--hero" />
      </section>

      <section className="landing-section" id="emulator">
        <header className="section-head">
          <p className="kicker">模拟器</p>
          <Title as="h2">Ember 模拟器</Title>
          <p className="section-lead">
            Ember 是 FC 与学习机的模拟器。NES 卡带靠 mapper 插件描述映射，学习机按整机机型运行，两者共用同一套调试服务。
          </p>
        </header>
        <div className="feature-grid">
          {EMBER_FEATURES.map((feature) => (
            <Panel key={feature.title} as="div" title={feature.title} className="feature">
              <p>{feature.text}</p>
            </Panel>
          ))}
        </div>
        <div className="shot-grid">
          {landing.emberShots.map((shot) => (
            <Shot key={shot.src} shot={shot} />
          ))}
        </div>
        <p className="section-links">
          <a href={links.manualIndex}>使用说明</a>
          <a href={links.pluginIndex}>插件开发手册</a>
          <a href={links.debugIndex}>调试接口手册</a>
        </p>
      </section>

      <section className="landing-section" id="fcdev">
        <header className="section-head">
          <p className="kicker">开发者工具</p>
          <Title as="h2">fcdev：VS Code 扩展</Title>
          <p className="section-lead">
            fcdev 是面向开发者的 VS Code 扩展，支持 NES 与 SB2000（小霸王学习机）程序的开发。它与 Ember 直接连通，构建完的程序一键就能在模拟器里运行与调试。
          </p>
        </header>
        <div className="feature-grid">
          {FCDEV_FEATURES.map((feature) => (
            <Panel key={feature.title} as="div" title={feature.title} className="feature">
              <p>{feature.text}</p>
            </Panel>
          ))}
        </div>
        <div className="shot-grid">
          {landing.fcdevShots.map((shot) => (
            <Shot key={shot.src} shot={shot} />
          ))}
        </div>
        <p className="section-links">
          <a href={links.fcdevStart}>安装与入门</a>
          <a href={links.fcdevBuild}>构建</a>
          <a href={links.fcdevEmulator}>模拟器与调试</a>
          <a href={links.fcdevNes}>NES 开发指南</a>
          <a href={links.fcdevSb2000}>SB2000 开发指南</a>
          <a href={links.fcdevSyscall}>系统调用手册</a>
        </p>
      </section>

      <section className="landing-section" id="docs">
        <header className="section-head">
          <p className="kicker">帮助文档</p>
          <Title as="h2">整套帮助文档</Title>
          <p className="section-lead">左侧导航可以直接跳到任何一页，版本变化见使用说明里的更新记录。</p>
        </header>
        <div className="doc-cards">
          <a className="doc-card" href={links.manualIndex}>
            <strong>使用说明</strong>
            <span>{counts.manual} 页</span>
            <p>快速开始、窗口与快捷键、机型与 ROM、卡带与软盘、键位与手柄、画面声音、首选项、常见问题与更新记录。</p>
          </a>
          <a className="doc-card" href={links.pluginIndex}>
            <strong>插件开发手册</strong>
            <span>{counts.plugin} 页</span>
            <p>为 NES 卡带编写 mapper 插件：清单字段、存储区与窗口、寄存器、脚本接口、常见写法与测试排错。</p>
          </a>
          <a className="doc-card" href={links.debugIndex}>
            <strong>调试接口手册</strong>
            <span>{counts.debug} 页</span>
            <p>调试服务的连接与报文、运行控制流程、客户端工具，以及每个请求与事件的参数和返回值。</p>
          </a>
          <a className="doc-card" href={links.fcdevIndex}>
            <strong>fcdev 开发手册</strong>
            <span>{counts.fcdev} 页</span>
            <p>构建、模拟器与调试、反汇编、NES 与 SB2000 开发指南、系统调用、硬件说明、C 接口与符号参考、示例源码。</p>
          </a>
        </div>
      </section>
    </div>
  );
}
