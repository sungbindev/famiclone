// ------------------------------------------------------------------------
// 名称：project-config.test.ts
// 说明：测试项目配置：默认值、相对路径解析、未知项与类型错误报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：不再接受 toolchain 与 emulator 配置项，位置分别由扩展自带的工具链与扩展设置给出。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { normalizeConfig } from '../../../src/shared/project/project-config';

const ROOT = path.resolve('project-root');

describe('项目配置', () => {
  it('补齐默认值并把相对路径换成绝对路径', () => {
    const file = path.join(ROOT, 'fcdev.json');
    const config = normalizeConfig({ target: 'nes', include: ['inc'], out: 'dist', firmware: 'rom/bios.nes' }, file, ROOT);
    expect(config.target).toBe('nes');
    expect(config.optimize).toBe(true);
    expect(config.disk).toBe(true);
    expect(config.include).toEqual([path.join(ROOT, 'inc')]);
    expect(config.outDir).toBe(path.join(ROOT, 'dist'));
    expect(config.firmware).toBe(path.join(ROOT, 'rom', 'bios.nes'));
  });

  it('linkConfig 换成绝对路径，不写时为 undefined，类型不对报错', () => {
    const file = path.join(ROOT, 'fcdev.json');
    expect(normalizeConfig({ linkConfig: 'cfg/image.cfg' }, file, ROOT).linkConfig).toBe(path.join(ROOT, 'cfg', 'image.cfg'));
    expect(normalizeConfig({}, file, ROOT).linkConfig).toBeUndefined();
    expect(() => normalizeConfig({ linkConfig: 1 }, file, ROOT)).toThrow(/类型不对/);
  });

  it('diskImage 的相对路径以配置文件所在目录为基准，类型不对报错', () => {
    const file = path.join(ROOT, 'fcdev.json');
    expect(normalizeConfig({ diskImage: 'disk/dos.img' }, file, ROOT).diskImage).toBe(path.join(ROOT, 'disk', 'dos.img'));
    expect(normalizeConfig({}, file, ROOT).diskImage).toBeUndefined();
    expect(() => normalizeConfig({ diskImage: 1 }, file, ROOT)).toThrow(/类型不对/);
  });

  it('disk 为 false 时不生成磁盘镜像，类型不对报错', () => {
    expect(normalizeConfig({ disk: false }, null, ROOT).disk).toBe(false);
    expect(() => normalizeConfig({ disk: 'no' }, null, ROOT)).toThrow(/类型不对/);
  });

  it('没有配置文件时以给定目录为项目根', () => {
    const config = normalizeConfig({}, null, ROOT);
    expect(config.file).toBeNull();
    expect(config.root).toBe(ROOT);
    expect(config.outDir).toBe(path.join(ROOT, 'build'));
  });

  it('未知的配置项报错并列出可用项', () => {
    expect(() => normalizeConfig({ targt: 'nes' }, null, ROOT)).toThrow(
      expect.objectContaining({ code: 'CONFIG_UNKNOWN_KEY', hint: expect.stringContaining('target') })
    );
  });

  it('toolchain 与 emulator 不再是配置项', () => {
    for (const key of ['toolchain', 'emulator']) {
      expect(() => normalizeConfig({ [key]: 'x' }, null, ROOT)).toThrow(expect.objectContaining({ code: 'CONFIG_UNKNOWN_KEY' }));
    }
  });

  it('类型不对报错', () => {
    expect(() => normalizeConfig({ optimize: 'yes' }, null, ROOT)).toThrow(/类型不对/);
    expect(() => normalizeConfig({ include: [1] }, null, ROOT)).toThrow(/类型不对/);
  });

  it('define 的值只能是字符串或数字', () => {
    expect(() => normalizeConfig({ define: { A: true } }, null, ROOT)).toThrow(/define\.A/);
  });

  it('内容不是对象报错', () => {
    expect(() => normalizeConfig([], null, ROOT)).toThrow(/必须是对象/);
  });
});
