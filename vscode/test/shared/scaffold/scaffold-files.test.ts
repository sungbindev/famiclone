// ------------------------------------------------------------------------
// 名称：scaffold-files.test.ts
// 说明：测试新建项目生成的内容：模板清单的检查、fcdev.json、.vscode/tasks.json、README 的占位替换。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import {
  buildProjectConfigText,
  buildTasksText,
  expandReadme,
  parseTemplateManifest
} from '../../../src/shared/scaffold/scaffold-files';

describe('模板清单', () => {
  it('目录名必须等于“目标-语言”', () => {
    expect(parseTemplateManifest({ target: 'nes', language: 'c', title: '红白机' }, 'nes-c')).toEqual({
      name: 'nes-c',
      target: 'nes',
      language: 'c',
      title: '红白机'
    });
    expect(() => parseTemplateManifest({ target: 'nes', language: 'c', title: 'x' }, 'nes-asm')).toThrow(expect.objectContaining({ code: 'TEMPLATE_INVALID' }));
    expect(() => parseTemplateManifest({ target: 'nes', language: 'rust', title: 'x' }, 'nes-rust')).toThrow(/不合法/);
    expect(() => parseTemplateManifest([], 'nes-c')).toThrow(/不合法/);
  });
});

describe('生成的文件内容', () => {
  it('fcdev.json 写目标与程序名 hello', () => {
    expect(JSON.parse(buildProjectConfigText('nes'))).toEqual({ target: 'nes', name: 'hello' });
  });

  it('tasks.json 有默认构建任务与运行任务', () => {
    const { tasks } = JSON.parse(buildTasksText()) as { tasks: Array<Record<string, unknown>> };
    expect(tasks.map((task) => task.command)).toEqual(['build', 'run']);
    expect(tasks[0]).toMatchObject({ type: 'fcdev', path: 'src', problemMatcher: '$fcdev', group: { kind: 'build', isDefault: true } });
  });

  it('README 里的 {{name}} 换成项目名', () => {
    expect(expandReadme('# {{name}}\n用 {{name}} 构建', 'game')).toBe('# game\n用 game 构建');
  });
});
