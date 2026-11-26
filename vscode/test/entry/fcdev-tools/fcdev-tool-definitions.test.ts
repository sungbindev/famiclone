// ------------------------------------------------------------------------
// 名称：fcdev-tool-definitions.test.ts
// 说明：测试新建项目、构建、运行、项目信息、C 接口与使用文档六个语言模型工具的执行：路径
//       规则、结果文字、错误说明，以及工具名与清单里的声明一一对应。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：各服务用替身；输入的结构检查由注册处按清单里的 inputSchema 做，
//       这里取清单里的结构核对示例输入。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it, vi } from 'vitest';
import type { BuildOutcome } from '../../../src/contracts/build/build-service';
import { createFcdevTools, describeFailure, resolveWorkspacePath, type FcdevToolServices } from '../../../src/entry/fcdev-tools/fcdev-tool-definitions';
import { parseHeaderFunctions } from '../../../src/shared/c-api/api-functions';
import { validateInput, type JsonSchema } from '../../../src/shared/disassembly/schema-validator';
import { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { describeDoc, parseTopics } from '../../../src/shared/usage-docs/doc-topics';

const ROOT = path.resolve('ws');

const manifest = JSON.parse(readFileSync(path.join(__dirname, '..', '..', '..', 'package.json'), 'utf8')) as {
  contributes: { languageModelTools: Array<{ name: string; inputSchema: JsonSchema }> };
};

const FUNCTIONS = parseHeaderFunctions('/**\n * 打开。\n * @syscall $7B/$09\n */\nunsigned char sb_file_open(void);\n', 'sb2000-file.h');
const DOCS = parseTopics(JSON.stringify({ topics: [{ topic: 'build', file: '01-构建.md', title: '构建', summary: '任务' }, { topic: 'create', file: '04.md', title: '新建', summary: '任务模板' }] }), 'f').map((entry) =>
  describeDoc(entry, path.join(ROOT, 'docs'), ROOT)
);

/** 构建一组替身服务。 */
function createServices(overrides: Partial<FcdevToolServices> = {}): FcdevToolServices {
  const outcome = {
    plan: { target: { name: 'nes' } },
    result: { programName: 'hello', outputFile: path.join(ROOT, 'build', 'nes', 'hello.nes'), size: 40976, compiled: 1, reused: 0 },
    disk: undefined
  } as BuildOutcome;
  return {
    build: {
      plan: vi.fn(),
      build: vi.fn(async (_input, reporter) => {
        reporter?.log('编译 hello.c');
        return outcome;
      }),
      buildDisk: vi.fn()
    },
    runner: {
      run: vi.fn(async (_input, reporter) => {
        reporter?.log('编译 hello.c');
        return { pid: 4242, description: 'hello.nes', debug: false, port: undefined, firmwareDisk: false };
      })
    },
    scaffold: {
      listTemplates: vi.fn(),
      create: vi.fn((input) => ({ directory: input.directory, template: { name: `${input.target}-${input.language}`, target: input.target, language: input.language, title: 't', programName: 'hello' }, files: ['fcdev.json', 'src/hello.c'] }))
    },
    describer: {
      describe: vi.fn(({ cwd }) => ({
        root: cwd, configFile: null, targetName: 'nes', targetTitle: '红白机', toolchainHome: 'tc', includeDirs: ['inc'], asmIncludeDirs: ['asm'], defines: {}, outDir: path.join(cwd, 'build')
      }))
    },
    api: { describe: vi.fn(() => ({ targetName: 'sb2000', targetTitle: 'SB2000', functions: FUNCTIONS })) },
    docs: {
      list: () => DOCS,
      query: (query) => ({ kind: 'text', matches: DOCS.filter((doc) => `${doc.topic}${doc.title}${doc.summary}`.includes(query)) }),
      read: (doc) => `正文 ${doc.topic}`
    },
    ...overrides
  };
}

/** 取一个工具。 */
function toolOf(name: string, services: FcdevToolServices = createServices()) {
  const tool = createFcdevTools(services).find((item) => item.name === name);
  if (tool === undefined) throw new Error(`没有工具 ${name}`);
  return tool;
}

/** 取清单里一个工具的输入结构。 */
function schemaOf(name: string): JsonSchema {
  return manifest.contributes.languageModelTools.find((tool) => tool.name === name)!.inputSchema;
}

/** 运行 C 接口工具。 */
function runApi(input: Record<string, unknown>): Promise<string> {
  return toolOf('fcdev_api').run(input, { roots: [ROOT] });
}

/** 运行使用文档工具。 */
function runDocs(input: Record<string, unknown>): Promise<string> {
  return toolOf('fcdev_docs').run(input, { roots: [] });
}

describe('与清单的对应', () => {
  it('工具名与清单里声明的 fcdev_ 工具一一对应，写操作的工具要确认', () => {
    const declared = manifest.contributes.languageModelTools.map((tool) => tool.name).filter((name) => name.startsWith('fcdev_')).toSorted();
    expect(createFcdevTools(createServices()).map((tool) => tool.name).toSorted()).toEqual(declared);
    expect(toolOf('fcdev_new_project').writes).toBe(true);
    expect(toolOf('fcdev_build').writes).toBe(true);
    expect(toolOf('fcdev_run').writes).toBe(true);
    for (const name of ['fcdev_info', 'fcdev_api', 'fcdev_docs']) expect(toolOf(name).writes).toBe(false);
  });

  it('清单里的输入结构能检查示例输入', () => {
    expect(validateInput(schemaOf('fcdev_new_project'), { directory: 'game', target: 'nes', language: 'c' })).toEqual([]);
    expect(validateInput(schemaOf('fcdev_new_project'), { directory: 'game', target: 'gba', language: 'c' })).not.toEqual([]);
    expect(validateInput(schemaOf('fcdev_build'), { path: 'src', target: 'nes' })).toEqual([]);
    expect(validateInput(schemaOf('fcdev_build'), { disk: true })).not.toEqual([]);
    expect(validateInput(schemaOf('fcdev_run'), { path: 'src', debug: true })).toEqual([]);
    expect(validateInput(schemaOf('fcdev_run'), { debug: 'yes' })).not.toEqual([]);
    expect(validateInput(schemaOf('fcdev_docs'), {})).toEqual([]);
  });
});

describe('路径规则', () => {
  it('相对路径按第一个工作区文件夹解析，必须在工作区里', () => {
    expect(resolveWorkspacePath(undefined, 'path', [ROOT])).toBe(ROOT);
    expect(resolveWorkspacePath('src', 'path', [ROOT])).toBe(path.join(ROOT, 'src'));
    expect(() => resolveWorkspacePath('..', 'path', [ROOT])).toThrow(/必须在工作区里/);
    expect(() => resolveWorkspacePath('src', 'path', [])).toThrow(/没有打开工作区/);
    expect(() => resolveWorkspacePath('', 'path', [ROOT])).toThrow(/必须是路径/);
  });

  it('可预期的错误带上建议，其他错误没有说明', () => {
    expect(describeFailure(new FcdevError('失败', { hint: '试试这样' }))).toBe('失败\n试试这样');
    expect(describeFailure(new Error('缺陷'))).toBeUndefined();
  });
});

describe('运行', () => {
  it('把路径、目标与 debug 交给运行器，返回构建进度与启动结果', async () => {
    const services = createServices();
    const text = await toolOf('fcdev_run', services).run({ path: 'src', target: 'nes', debug: true }, { roots: [ROOT] });
    expect(services.runner.run).toHaveBeenCalledWith({ build: { pathArg: 'src', cwd: ROOT, targetOverride: 'nes' }, debug: true }, expect.anything());
    expect(text).toBe('编译 hello.c\n已启动 Ember（进程 4242），加载 hello.nes');
  });

  it('可预期的错误写成“错误：…”并带建议', async () => {
    const services = createServices({
      runner: {
        run: vi.fn(async () => {
          throw new FcdevError('还没有设置 Ember 的位置', { code: 'EMULATOR_PATH_NOT_SET', hint: '填写 fcdev.emulator.path' });
        })
      }
    });
    expect(await toolOf('fcdev_run', services).run({}, { roots: [ROOT] })).toBe('错误：还没有设置 Ember 的位置\n填写 fcdev.emulator.path');
  });

  it('调用前要确认，说明会启动 Ember', () => {
    expect(toolOf('fcdev_run').confirm?.({ path: 'src', debug: true })).toContain('启动设置里的 Ember');
  });
});

describe('新建项目', () => {
  it('把目录换成绝对路径后调用新建服务，返回文件清单', async () => {
    const services = createServices();
    const text = await toolOf('fcdev_new_project', services).run({ directory: 'game', target: 'nes', language: 'c' }, { roots: [ROOT] });
    expect(services.scaffold.create).toHaveBeenCalledWith({ directory: path.join(ROOT, 'game'), target: 'nes', language: 'c' });
    expect(text).toContain('共 2 个文件');
    expect(text).toContain('src/hello.c');
  });

  it('语言不对时报错', async () => {
    await expect(toolOf('fcdev_new_project').run({ directory: 'game', target: 'nes', language: 'rust' }, { roots: [ROOT] })).rejects.toThrow(/language/);
  });
});

describe('构建', () => {
  it('返回进度与结果，没有生成磁盘镜像时不返回镜像行', async () => {
    const services = createServices();
    const text = await toolOf('fcdev_build', services).run({ path: 'src', target: 'nes' }, { roots: [ROOT] });
    expect(text).toContain('编译 hello.c');
    expect(text).toContain('40976 字节，目标 nes');
    expect(text).not.toContain('.img');
    expect(vi.mocked(services.build.build).mock.calls[0]?.[0]).toEqual({ pathArg: 'src', cwd: ROOT, targetOverride: 'nes' });
  });

  it('构建同时生成了磁盘镜像时，再返回镜像行', async () => {
    const outcome = {
      plan: { target: { name: 'sb2000' } },
      result: { programName: 'hello', outputFile: path.join(ROOT, 'build', 'sb2000', 'HELLO.EXE'), size: 420, compiled: 1, reused: 0 },
      disk: { imageFile: path.join(ROOT, 'build', 'sb2000', 'HELLO.img'), size: 1474560, files: ['HELLO.EXE', 'README.TXT'] }
    } as unknown as BuildOutcome;
    const services = createServices({ build: { plan: vi.fn(), buildDisk: vi.fn(), build: vi.fn(async () => outcome) } });
    const text = await toolOf('fcdev_build', services).run({ path: 'src' }, { roots: [ROOT] });
    expect(text).toContain('HELLO.img');
    expect(text).toContain('含 HELLO.EXE、README.TXT');
  });

  it('不给路径时构建工作区第一个文件夹', async () => {
    const services = createServices();
    await toolOf('fcdev_build', services).run({}, { roots: [ROOT] });
    expect(vi.mocked(services.build.build).mock.calls[0]?.[0]).toMatchObject({ pathArg: '.', cwd: ROOT });
  });

  it('构建失败时返回已有的输出与“错误：…”，不抛出', async () => {
    const services = createServices({
      build: {
        plan: vi.fn(),
        buildDisk: vi.fn(),
        build: async (_input, reporter) => {
          reporter?.write('bad.c(1): Error: Undefined symbol\n');
          throw new FcdevError('编译失败：bad.c', { hint: '检查源码' });
        }
      }
    });
    const text = await toolOf('fcdev_build', services).run({ path: 'src/bad.c' }, { roots: [ROOT] });
    expect(text).toBe('bad.c(1): Error: Undefined symbol\n错误：编译失败：bad.c\n检查源码');
  });
});

describe('项目信息、C 接口与使用文档', () => {
  it('项目信息：用 path 找项目', async () => {
    const text = await toolOf('fcdev_info').run({ path: 'game' }, { roots: [ROOT] });
    expect(text).toContain(`项目根目录：${path.join(ROOT, 'game')}`);
    expect(text).toContain('目标：nes（红白机）');
  });

  it('C 接口：不给查询词列出模块，给了就查询，没有结果时说明', async () => {
    expect(await runApi({})).toMatch(/1 个函数，1 个模块/);
    expect(await runApi({ query: 'sb_file_open' })).toContain('系统调用：$7B/$09');
    expect(await runApi({ query: 'zzz' })).toBe('没有找到“zzz”');
  });

  it('使用文档：列出主题；只匹配到一份返回全文，多份列摘要，没有时说明', async () => {
    expect(await runDocs({})).toMatch(/使用文档：2 份[\s\S]*build[\s\S]*create/);
    expect(await runDocs({ query: '构建' })).toBe('# 构建（主题 build）\n\n正文 build');
    expect(await runDocs({ query: '任务' })).toMatch(/包含“任务”的文档：2 份/);
    expect(await runDocs({ query: 'zzz' })).toMatch(/没有找到“zzz”/);
  });
});
