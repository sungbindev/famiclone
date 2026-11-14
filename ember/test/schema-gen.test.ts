/*
 * ------------------------------------------------------------------------
 * 名称：schema-gen.test.ts
 * 说明：Schema 代码生成的测试：对 system 模块生成的 TypeScript 与 C++ 代码、汇总文件做关键断言，并检查 mainOnly 与 debugOnly 请求不进入界面请求名单。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：生成在临时目录里进行，不改动仓库里的生成文件。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { cpSync, existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, describe, expect, it } from 'vitest';
import { generate } from '../tools/schema-gen/schema-gen.mjs';

const REPO_ROOT = fileURLToPath(new URL('..', import.meta.url));
const roots: string[] = [];

/** 创建临时项目：只含给定的 Schema 文件。 */
function createProject(schemas: Record<string, unknown>): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-schema-gen-'));
  roots.push(root);
  mkdirSync(join(root, 'schema'));
  for (const [name, schema] of Object.entries(schemas)) writeFileSync(join(root, 'schema', `${name}.schema.json`), JSON.stringify(schema), 'utf8');
  return root;
}

/** 读取项目里的文件。 */
function read(root: string, ...parts: string[]): string {
  return readFileSync(join(root, ...parts), 'utf8');
}

const EMPTY_OBJECT = { type: 'object', properties: {} };

afterEach(() => {
  for (const root of roots.splice(0)) rmSync(root, { recursive: true, force: true });
});

describe('generate：system 模块', () => {
  it('生成 TypeScript、C++ 与汇总文件', () => {
    const root = createProject({});
    cpSync(join(REPO_ROOT, 'schema', 'system.schema.json'), join(root, 'schema', 'system.schema.json'));
    const result = generate(root);
    expect(result.modules).toEqual(['system']);

    const ts = read(root, 'src', 'contracts', 'system', 'generated', 'system.ts');
    expect(ts).toContain("version: 'system.version'");
    expect(ts).toContain('export interface SystemVersionResult');
    expect(ts).toContain('appVersion: string;');
    expect(ts).toContain('coreVersion: string;');
    expect(ts).toContain('export const SYSTEM_SCHEMA: ModuleSchema');

    const header = read(root, 'core', 'src', 'contracts', 'system', 'generated', 'system.hpp');
    expect(header).toContain('namespace ember::contracts::system {');
    expect(header).toContain('SYSTEM_VERSION_REQUEST = "system.version"');
    expect(header).toContain('struct SystemVersionResult');
    expect(header).toContain('std::string appVersion{};');
    const source = read(root, 'core', 'src', 'contracts', 'system', 'generated', 'system.cpp');
    expect(source).toContain('namespace json_fields = ::ember::shared::json_fields;');
    expect(source).toContain('::ember::shared::SchemaError');

    const api = read(root, 'src', 'contracts', 'core-gateway', 'generated', 'core-api.ts');
    expect(api).toMatch(/CORE_REQUEST_NAMES: readonly string\[\] = \[\s*'system\.version'\s*\]/);
    expect(api).toContain('version: (params: CoreParams<\'system.version\'>) => Promise<CoreResult<\'system.version\'>>;');
    expect(api).toContain('CORE_SCHEMAS: readonly ModuleSchema[] = [SYSTEM_SCHEMA]');
  });

  it('不含插件 SDK 的输出，内容没变时不改写，模块删除后清理生成目录', () => {
    const root = createProject({ system: { module: 'system', types: {}, requests: { version: { params: EMPTY_OBJECT, result: EMPTY_OBJECT } }, events: {} } });
    expect(generate(root).written.length).toBeGreaterThan(0);
    expect(generate(root).written).toEqual([]);
    expect(existsSync(join(root, 'sdk'))).toBe(false);
    rmSync(join(root, 'schema', 'system.schema.json'));
    const removed = generate(root).removed;
    expect(removed.length).toBe(2);
    expect(existsSync(join(root, 'src', 'contracts', 'system', 'generated'))).toBe(false);
  });
});

describe('generate：mappers 模块', () => {
  it('install 只能由主进程发起，其他请求进入界面请求名单，事件与类型照常生成', () => {
    const root = createProject({});
    cpSync(join(REPO_ROOT, 'schema', 'mappers.schema.json'), join(root, 'schema', 'mappers.schema.json'));
    expect(generate(root).modules).toEqual(['mappers']);
    const api = read(root, 'src', 'contracts', 'core-gateway', 'generated', 'core-api.ts');
    expect(api).toMatch(/CORE_REQUEST_NAMES: readonly string\[\] = \[\s*'mappers\.list',\s*'mappers\.info',\s*'mappers\.uninstall',\s*'mappers\.setEnabled',\s*'mappers\.reload'\s*\]/);
    expect(api).not.toMatch(/\n\s+install: \(params/);
    expect(api).toContain("'mappers.changed'");
    const header = read(root, 'core', 'src', 'contracts', 'mappers', 'generated', 'mappers.hpp');
    expect(header).toContain('MAPPERS_INSTALL_REQUEST = "mappers.install"');
    expect(header).toContain('struct MappersInstallParams');
    expect(header).toContain('struct MapperDetail');
  });
});

/** 在临时目录里准备一个 SDK 目录（ember-debug-sdk 的位置）。 */
function createSdkDirectory(): string {
  const sdkRoot = mkdtempSync(join(tmpdir(), 'ember-debug-sdk-'));
  roots.push(sdkRoot);
  return sdkRoot;
}

describe('generate：调试 SDK', () => {
  const debug = { module: 'debug', types: {}, requests: {}, events: {} };
  const shots = {
    module: 'shots',
    types: { Frame: { type: 'object', properties: { n: { type: 'integer' } }, required: ['n'] } },
    requests: {
      take: {
        description: '截图。',
        debugOnly: true,
        params: { type: 'object', properties: { format: { type: 'string', enum: ['png', 'rgba'] } } },
        result: { type: 'object', properties: { width: { type: 'integer' }, data: { description: '图像数据（二进制）' }, frame: { $ref: '#/types/Frame' } }, required: ['width'] },
        errors: ['shots.noPicture']
      }
    },
    events: { taken: { description: '截图完成。', data: { type: 'object', properties: { width: { type: 'integer' } }, required: ['width'] } } }
  };

  it('给出 SDK 目录时，把 OpenRPC 文档与类型写到它的 generated', () => {
    const root = createProject({ debug, shots });
    const sdkRoot = createSdkDirectory();
    generate(root, { sdkDirectory: sdkRoot });

    const api = read(sdkRoot, 'generated', 'api.ts');
    expect(api).toContain('export interface ShotsFrame {');
    expect(api).toContain('export interface ShotsTakeParams {');
    expect(api).toContain('format?: "png" | "rgba";');
    expect(api).toContain('export interface ShotsTakeResult {');
    expect(api).toContain('frame?: ShotsFrame;');
    expect(api).toContain('"shots.take": { params: ShotsTakeParams; result: ShotsTakeResult };');
    expect(api).toContain('"shots.taken": ShotsTakenData;');
    expect(api).toContain('readonly shots: {');
    expect(api).toContain('take(params?: ShotsTakeParams): Promise<ShotsTakeResult>;');
    expect(api).toContain('export type RpcDiscoverResult = Record<string, unknown>;');
    expect(JSON.parse(read(sdkRoot, 'generated', 'openrpc.json')).methods.map((method: { name: string }) => method.name)).toContain('shots.take');
  });

  it('没有类型的顶层 data 属性是二进制：OpenRPC 里有 x-binary，SDK 类型是 Uint8Array', () => {
    const root = createProject({ debug, shots });
    const sdkRoot = createSdkDirectory();
    generate(root, { sdkDirectory: sdkRoot });
    const method = JSON.parse(read(sdkRoot, 'generated', 'openrpc.json')).methods.find((item: { name: string }) => item.name === 'shots.take');
    expect(method.result.schema.properties.data['x-binary']).toBe(true);
    expect(read(sdkRoot, 'generated', 'api.ts')).toMatch(/\/\*\* 图像数据（二进制） \*\/\n {2}data\?: Uint8Array;/);
  });

  it('没有给 SDK 目录、或目录不存在时不写', () => {
    const root = createProject({ debug, shots });
    const missing = join(createSdkDirectory(), 'not-there');
    const result = generate(root, { sdkDirectory: missing });
    expect(existsSync(missing)).toBe(false);
    expect(result.written.every((path) => !path.startsWith(missing))).toBe(true);
    expect(generate(createProject({ debug, shots })).written.some((path) => path.endsWith('api.ts') && path.includes('debug-sdk'))).toBe(false);
  });
});

describe('generate：请求使用范围', () => {
  const demo = {
    module: 'demo',
    types: {},
    requests: {
      open: { params: EMPTY_OBJECT, result: EMPTY_OBJECT },
      peek: { debugOnly: true, params: EMPTY_OBJECT, result: EMPTY_OBJECT },
      openPath: { mainOnly: true, params: EMPTY_OBJECT, result: EMPTY_OBJECT }
    },
    events: {}
  };

  it('debugOnly 与 mainOnly 的请求不进入界面请求名单与 CoreApi', () => {
    const root = createProject({ demo });
    generate(root);
    const api = read(root, 'src', 'contracts', 'core-gateway', 'generated', 'core-api.ts');
    expect(api).toMatch(/CORE_REQUEST_NAMES: readonly string\[\] = \[\s*'demo\.open'\s*\]/);
    expect(api).not.toContain('peek: (params');
    expect(api).not.toContain('openPath: (params');
    expect(api).toContain('open: (params');
  });

  it('它们仍生成请求名与类型，核心的请求名常量也照常生成', () => {
    const root = createProject({ demo });
    generate(root);
    const ts = read(root, 'src', 'contracts', 'demo', 'generated', 'demo.ts');
    expect(ts).toContain("peek: 'demo.peek'");
    expect(ts).toContain("'demo.peek': { params: DemoPeekParams; result: DemoPeekResult };");
    expect(ts).toContain('"debugOnly": true');
    expect(read(root, 'core', 'src', 'contracts', 'demo', 'generated', 'demo.hpp')).toContain('DEMO_PEEK_REQUEST = "demo.peek"');
  });

  /** 创建只有一个带指定标记的请求的项目。 */
  const projectWithFlags = (flags: Record<string, unknown>) => createProject({ demo: { ...demo, requests: { peek: { ...flags, params: EMPTY_OBJECT, result: EMPTY_OBJECT } } } });

  it('拒绝非法的标记', () => {
    expect(() => generate(projectWithFlags({ debugOnly: false }))).toThrow('debugOnly');
    expect(() => generate(projectWithFlags({ mainOnly: true, debugOnly: true }))).toThrow('不能同时标记');
  });

  it('不再支持 sdk 标记', () => {
    const root = createProject({ demo: { module: 'demo', types: { Item: { ...EMPTY_OBJECT, sdk: true } }, requests: {}, events: {} } });
    expect(() => generate(root)).toThrow('不支持的关键字：sdk');
  });
});
