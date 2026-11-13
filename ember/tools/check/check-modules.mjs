/*
 * ------------------------------------------------------------------------
 * 名称：check-modules.mjs
 * 说明：分层与模块检查：TypeScript 各层的 import 规则与文件循环引用、C++ 各层的 #include 规则、硬件契约与硬件模块的规则（模块之间互不引用）；
 *       从 TypeScript 的 import、C++ 的 #include 与 Schema 的 $ref 分析模块之间的依赖，发现引用其他模块契约以外的目录与模块循环依赖；
 *       检查模块目录都在模块清单中登记；检查官方插件目录 plugins/<id>/：目录名等于清单里的 id，只含 manifest.json、main.js、icon.png。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：规则来自 docs/01-软件架构设计.md 第 4、5 节与 docs/06-构建与模块登记.md；测试文件不参与检查。
 *       用法：node tools/check/check-modules.mjs [--dependents <模块>]，后者列出直接与间接依赖该模块的全部模块；
 *       checkProject(root) 可被测试引用，对任意目录做同样的检查。
 * ------------------------------------------------------------------------
 */

import { existsSync, readdirSync, readFileSync, statSync } from 'node:fs';
import { builtinModules } from 'node:module';
import { dirname, join, posix, relative, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { loadSchemas } from '../schema-gen/schema-gen.mjs';

/** 不属于任何业务模块的契约目录（核心网关、外壳、日志、画面与输入的消息端口）。 */
const FRAMEWORK_CONTRACTS = new Set(['core-gateway', 'shell', 'logging', 'stream-ports']);
/** 不属于任何业务模块的入口目录（通用的核心请求转发、外壳能力、消息端口）。 */
const FRAMEWORK_ENTRY = new Set(['core', 'shell', 'streams']);
/** 不属于任何业务模块的界面目录（外壳）。 */
const FRAMEWORK_UI = new Set(['shell']);

/** TypeScript 各层可以引用的层。 */
const TS_ALLOWED = {
  contracts: ['contracts', 'shared'],
  entry: ['entry', 'contracts', 'shared'],
  infra: ['infra', 'contracts', 'shared', 'platform'],
  ui: ['ui', 'contracts'],
  platform: ['platform'],
  shared: ['shared'],
  'core-host': ['core-host', 'contracts', 'shared'],
  bootstrap: ['bootstrap', 'contracts', 'entry', 'infra', 'ui', 'platform', 'shared', 'core-host']
};
/** 可以引入 electron 的层。 */
const ELECTRON_LAYERS = new Set(['platform', 'entry', 'core-host', 'bootstrap']);
/** 不能使用 Node 内置模块的层。 */
const NO_NODE_LAYERS = new Set(['ui', 'shared', 'contracts']);
/** 界面库与可以使用它们的层。 */
const UI_PACKAGES = new Set(['react', 'react-dom', '@rujianjs/ui-kit']);
const UI_PACKAGE_LAYERS = new Set(['ui', 'bootstrap']);
/** 可以使用其他第三方包的层。 */
const PACKAGE_LAYERS = new Set(['platform', 'bootstrap']);
/** Node 内置模块名。 */
const NODE_BUILTINS = new Set(builtinModules);

/** C++ 按模块分目录的层。 */
const CPP_MODULE_LAYERS = new Set(['contracts', 'logic', 'infra', 'requests']);
/** C++ 各层可以包含的层。 */
const CPP_ALLOWED = {
  contracts: ['contracts', 'shared'],
  logic: ['logic', 'contracts', 'shared', 'platform'],
  infra: ['infra', 'contracts', 'shared', 'platform'],
  requests: ['requests', 'contracts', 'shared', 'platform'],
  platform: ['platform', 'shared'],
  shared: ['shared'],
  binding: ['bootstrap', 'shared'],
  bootstrap: ['contracts', 'logic', 'infra', 'requests', 'platform', 'shared', 'bootstrap']
};
/** logic 层只能包含的 platform 抽象：事件、取消、错误日志、帧输出。 */
const PLATFORM_ABSTRACTIONS = new Set(['platform/events/event-sink.hpp', 'platform/requests/cancellation.hpp', 'platform/logging/error-log.hpp', 'platform/streams/frame-output.hpp']);
/** 硬件模块只能包含的 platform 抽象：日志与画面输出不属于硬件。 */
const HARDWARE_PLATFORM_ABSTRACTIONS = new Set(['platform/events/event-sink.hpp', 'platform/requests/cancellation.hpp']);
/** binding 只能包含的核心入口。 */
const BINDING_ENTRY = 'bootstrap/core.hpp';
/**
 * 硬件目录：contracts/hardware 是硬件契约，logic/hardware/<模块> 是硬件模块（一个型号一个目录）。
 * 硬件不是业务模块，没有请求，所以不要求登记在模块清单中；硬件模块之间互不引用，只经 contracts/hardware。
 */
const HARDWARE_DIRECTORY = 'hardware';
/** 硬件契约的目录前缀（#include 路径与相对 core/src 的文件路径都以它开头）。 */
const HARDWARE_CONTRACTS_PREFIX = 'contracts/hardware/';
/** 硬件模块的目录前缀。 */
const HARDWARE_LOGIC_PREFIX = 'logic/hardware/';
/** 标准库头文件：没有扩展名与目录的名字（如 <cstdint>、<span>）。 */
const STANDARD_HEADER = /^[a-z][a-z0-9_]*$/;

/** 递归列出文件。 */
function listFiles(directory, pattern) {
  if (!existsSync(directory)) return [];
  const result = [];
  for (const name of readdirSync(directory)) {
    const path = join(directory, name);
    if (statSync(path).isDirectory()) result.push(...listFiles(path, pattern));
    else if (pattern.test(name)) result.push(path);
  }
  return result;
}

/** 列出子目录名。 */
function listDirectories(directory) {
  if (!existsSync(directory)) return [];
  return readdirSync(directory).filter((name) => statSync(join(directory, name)).isDirectory());
}

/** 统一为以 / 分隔的相对路径。 */
function toPosix(path) {
  return path.split('\\').join('/');
}

/** 找出有向图中的环。 */
function findCycles(graph) {
  const cycles = [];
  const state = new Map();
  const stack = [];
  const visit = (node) => {
    state.set(node, 'visiting');
    stack.push(node);
    for (const next of graph.get(node) ?? []) {
      if (state.get(next) === 'visiting') cycles.push([...stack.slice(stack.indexOf(next)), next].join(' → '));
      else if (!state.has(next)) visit(next);
    }
    stack.pop();
    state.set(node, 'done');
  };
  for (const node of graph.keys()) if (!state.has(node)) visit(node);
  return cycles;
}

/** 由边列表建图。 */
function toGraph(edges) {
  const graph = new Map();
  for (const edge of edges) {
    if (!graph.has(edge.from)) graph.set(edge.from, new Set());
    graph.get(edge.from).add(edge.to);
  }
  return graph;
}

// ---------------------------------------------------------------- C++

/**
 * C++ 文件所在的层与模块：硬件契约属于模块 hardware，每个硬件模块自成一个模块 hardware/<名称>，
 * 这样硬件模块之间的引用与业务模块之间的引用走同一套“只能引用对方契约”的规则。
 */
function cppLocation(path) {
  const parts = path.split('/');
  const layer = parts[0];
  if (CPP_MODULE_LAYERS.has(layer) && parts.length > 2) {
    if (layer === 'logic' && parts[1] === HARDWARE_DIRECTORY && parts.length > 3) return { layer, module: `${HARDWARE_DIRECTORY}/${parts[2]}` };
    return { layer, module: parts[1] };
  }
  if (layer === 'bootstrap' && parts[1] === 'modules' && parts.length === 3) return { layer, module: parts[2].replace(/\.(cpp|hpp)$/, '') };
  return { layer, module: undefined };
}

/** 第三方头文件可以出现的位置。 */
function thirdPartyAllowed(include, path, location) {
  if (include.startsWith('nlohmann/')) {
    if (location.layer === 'contracts') return path.includes('/generated/');
    if (location.layer === 'shared') return path.startsWith('shared/json/');
    return ['platform', 'infra', 'requests', 'binding', 'bootstrap'].includes(location.layer);
  }
  if (include === 'napi.h' || include === 'node_api.h') return location.layer === 'binding';
  if (include === 'sqlite3.h') return path.startsWith('platform/database/');
  if (include === 'quickjs.h') return path.startsWith('infra/mappers/');
  if (include === 'miniz.h') return location.layer === 'infra';
  if (include === 'miniaudio.h') return path.startsWith('platform/audio/');
  return true;
}

/** 检查 C++ 的 #include，并收集模块之间的依赖。 */
function checkCpp(root, errors, edges) {
  const base = join(root, 'core', 'src');
  for (const file of listFiles(base, /\.(cpp|hpp)$/)) {
    const path = toPosix(relative(base, file));
    const from = cppLocation(path);
    const inHardwareContracts = path.startsWith(HARDWARE_CONTRACTS_PREFIX);
    const inHardwareLogic = path.startsWith(HARDWARE_LOGIC_PREFIX);
    if (inHardwareLogic && path.split('/').length < 4) errors.push(`core/src/${path}：硬件模块的文件必须放在 logic/hardware/<模块>/ 目录下`);
    const text = readFileSync(file, 'utf8');
    for (const match of text.matchAll(/^\s*#include\s+([<"])([^>"]+)[>"]/gm)) {
      const include = match[2];
      if (match[1] === '<') {
        if (inHardwareContracts) {
          if (!STANDARD_HEADER.test(include)) errors.push(`core/src/${path}：contracts/hardware 只能包含标准库头文件，不能包含 <${include}>`);
          continue;
        }
        if (!thirdPartyAllowed(include, path, from)) errors.push(`core/src/${path}：不能包含 <${include}>`);
        continue;
      }
      if (inHardwareContracts) {
        if (!include.startsWith(HARDWARE_CONTRACTS_PREFIX) && !include.startsWith('shared/')) errors.push(`core/src/${path}：contracts/hardware 只能包含 contracts/hardware 与 shared，不能包含 ${include}`);
        continue;
      }
      const to = cppLocation(include);
      const allowed = CPP_ALLOWED[from.layer];
      if (allowed !== undefined && !allowed.includes(to.layer)) errors.push(`core/src/${path}：${from.layer} 层不能包含 ${to.layer} 层的 ${include}`);
      if (from.layer === 'logic' && to.layer === 'platform' && !(inHardwareLogic ? HARDWARE_PLATFORM_ABSTRACTIONS : PLATFORM_ABSTRACTIONS).has(include)) {
        errors.push(`core/src/${path}：logic 层只能包含 platform 的抽象，不能包含 ${include}`);
      }
      if (from.layer === 'binding' && to.layer === 'bootstrap' && include !== BINDING_ENTRY) errors.push(`core/src/${path}：binding 只能经 ${BINDING_ENTRY} 使用核心`);
      if (inHardwareLogic && to.layer === 'contracts' && !include.startsWith(HARDWARE_CONTRACTS_PREFIX)) errors.push(`core/src/${path}：硬件模块只能包含 contracts/hardware 的契约，不能包含 ${include}`);
      if (from.module !== undefined && to.module !== undefined && from.module !== to.module) {
        if (to.layer !== 'contracts') errors.push(`core/src/${path}：模块 ${from.module} 只能引用模块 ${to.module} 的契约，不能包含 ${include}`);
        edges.push({ from: from.module, to: to.module });
      }
    }
  }
}

// ---------------------------------------------------------------- TypeScript

/** TypeScript 文件所在的层与模块。 */
function tsLocation(path) {
  const parts = path.split('/');
  const layer = parts[0];
  if (layer === 'contracts' && parts.length > 2 && !FRAMEWORK_CONTRACTS.has(parts[1])) return { layer, module: parts[1] };
  if (layer === 'ui' && parts.length > 2 && !FRAMEWORK_UI.has(parts[1])) return { layer, module: parts[1] };
  if (layer === 'entry' && parts[1] === 'ipc' && parts.length > 3 && !FRAMEWORK_ENTRY.has(parts[2])) return { layer, module: parts[2] };
  if (layer === 'bootstrap' && parts[2] === 'modules' && parts.length === 4) return { layer, module: parts[3].replace(/\.tsx?$/, ''), registration: true };
  return { layer, module: undefined };
}

/** 读出文件中的引用：说明符与是否只引用类型。 */
function readImports(text) {
  const imports = [];
  for (const match of text.matchAll(/(?:^|[\n;])\s*(import|export)\s+(type\s+)?([^'";]*?\bfrom\s+)?['"]([^'"]+)['"]/g)) {
    const clause = match[3] ?? '';
    if (match[1] === 'export' && clause === '') continue;
    const braces = /^\{([^}]*)\}\s*from\s+$/.exec(clause.trim() + ' ');
    const allTypes = braces !== null && braces[1].split(',').map((item) => item.trim()).filter(Boolean).every((item) => item.startsWith('type '));
    imports.push({ specifier: match[4], typeOnly: match[2] !== undefined || allTypes });
  }
  for (const match of text.matchAll(/\bimport\(\s*['"]([^'"]+)['"]\s*\)/g)) imports.push({ specifier: match[1], typeOnly: false });
  return imports;
}

/** 把相对引用解析为 src 下的文件。 */
function resolveTsFile(fromPath, specifier, files) {
  const target = posix.normalize(posix.join(posix.dirname(fromPath), specifier));
  for (const candidate of [target, `${target}.ts`, `${target}.tsx`, `${target}/index.ts`, `${target}/index.tsx`]) if (files.has(candidate)) return candidate;
  return target;
}

/** 包名（去掉子路径）。 */
function packageName(specifier) {
  const parts = specifier.split('/');
  return specifier.startsWith('@') ? parts.slice(0, 2).join('/') : parts[0];
}

/** 检查 TypeScript 的 import，收集模块之间的依赖。 */
function checkTs(root, errors, edges) {
  const base = join(root, 'src');
  const all = listFiles(base, /\.(ts|tsx)$/).map((file) => toPosix(relative(base, file)));
  const sources = all.filter((path) => !/\.test\.tsx?$/.test(path) && !path.endsWith('.d.ts'));
  const files = new Set(all);
  const fileEdges = [];
  for (const path of sources) {
    const from = tsLocation(path);
    const text = readFileSync(join(base, path), 'utf8');
    for (const { specifier, typeOnly } of readImports(text)) {
      if (specifier.startsWith('.')) {
        const target = resolveTsFile(path, specifier, files);
        const to = tsLocation(target);
        const allowed = TS_ALLOWED[from.layer];
        if (allowed !== undefined && !allowed.includes(to.layer)) errors.push(`src/${path}：${from.layer} 层不能引用 ${to.layer} 层的 ${specifier}`);
        if (from.layer === 'ui' && to.layer === 'contracts' && !typeOnly) errors.push(`src/${path}：ui 层只能引用 contracts 中的类型，${specifier} 应使用 import type`);
        if (!typeOnly) fileEdges.push({ from: path, to: target });
        if (from.module !== undefined && !from.registration && to.module !== undefined && to.module !== from.module) {
          if (to.layer !== 'contracts') errors.push(`src/${path}：模块 ${from.module} 只能引用模块 ${to.module} 的契约，不能引用 ${specifier}`);
          edges.push({ from: from.module, to: to.module });
        }
        continue;
      }
      const name = packageName(specifier);
      if (name === 'electron') {
        if (!ELECTRON_LAYERS.has(from.layer)) errors.push(`src/${path}：${from.layer} 层不能引入 electron`);
      } else if (specifier.startsWith('node:') || NODE_BUILTINS.has(name)) {
        if (NO_NODE_LAYERS.has(from.layer)) errors.push(`src/${path}：${from.layer} 层不能使用 Node 内置模块 ${specifier}`);
      } else if (UI_PACKAGES.has(name)) {
        if (!UI_PACKAGE_LAYERS.has(from.layer)) errors.push(`src/${path}：${from.layer} 层不能引用 ${name}`);
      } else if (!PACKAGE_LAYERS.has(from.layer)) {
        errors.push(`src/${path}：${from.layer} 层不能引用第三方包 ${name}`);
      }
    }
  }
  for (const cycle of findCycles(toGraph(fileEdges))) errors.push(`文件之间有循环引用：${cycle}`);
}

/** 直接与间接依赖 target 的模块。 */
function dependentsOf(target, edges) {
  const result = new Set();
  const queue = [target];
  while (queue.length > 0) {
    const current = queue.shift();
    for (const edge of edges) {
      if (edge.to === current && !result.has(edge.from) && edge.from !== target) {
        result.add(edge.from);
        queue.push(edge.from);
      }
    }
  }
  return [...result].toSorted();
}

// ---------------------------------------------------------------- 登记

/** 模块清单文件引用的 ./modules/<模块>。 */
function importsOf(file) {
  const text = existsSync(file) ? readFileSync(file, 'utf8') : '';
  return new Set([...text.matchAll(/from\s+['"]\.\/modules\/([a-z0-9-]+)['"]/g)].map((match) => match[1]));
}

/** 检查模块目录都在模块清单中登记。 */
function checkRegistration(root, errors, schemaModules) {
  const cppList = join(root, 'core', 'src', 'bootstrap', 'module-list.cpp');
  const cppText = existsSync(cppList) ? readFileSync(cppList, 'utf8') : '';
  const cppModules = new Set([...cppText.matchAll(/\{\s*"([a-z0-9-]+)"\s*,/g)].map((match) => match[1]));
  const mainModules = importsOf(join(root, 'src', 'bootstrap', 'main', 'module-list.ts'));
  const rendererModules = importsOf(join(root, 'src', 'bootstrap', 'renderer', 'module-list.ts'));

  const requireIn = (set, module, listName, reason) => {
    if (!set.has(module)) errors.push(`模块 ${module}${reason}，但没有登记在 ${listName}`);
  };
  for (const module of schemaModules) {
    requireIn(cppModules, module, 'core/src/bootstrap/module-list.cpp', ' 有 Schema');
    requireIn(mainModules, module, 'src/bootstrap/main/module-list.ts', ' 有 Schema');
  }
  for (const layer of CPP_MODULE_LAYERS) {
    for (const module of listDirectories(join(root, 'core', 'src', layer))) {
      if (module === HARDWARE_DIRECTORY) continue;
      requireIn(cppModules, module, 'core/src/bootstrap/module-list.cpp', ` 有 core/src/${layer} 目录`);
    }
  }
  for (const module of cppModules) {
    if (!existsSync(join(root, 'core', 'src', 'bootstrap', 'modules', `${module}.cpp`))) errors.push(`模块 ${module} 登记在 C++ 模块清单中，但没有 core/src/bootstrap/modules/${module}.cpp`);
  }
  for (const module of listDirectories(join(root, 'src', 'entry', 'ipc'))) {
    if (!FRAMEWORK_ENTRY.has(module)) requireIn(mainModules, module, 'src/bootstrap/main/module-list.ts', ' 有 src/entry/ipc 目录');
  }
  for (const module of listDirectories(join(root, 'src', 'ui'))) {
    if (!FRAMEWORK_UI.has(module)) requireIn(rendererModules, module, 'src/bootstrap/renderer/module-list.ts', ' 有 src/ui 目录');
  }
  for (const module of listDirectories(join(root, 'src', 'contracts'))) {
    if (!FRAMEWORK_CONTRACTS.has(module) && !schemaModules.includes(module) && !rendererModules.has(module)) {
      errors.push(`src/contracts/${module} 既没有 Schema，也没有登记在渲染进程的模块清单中`);
    }
  }
  for (const [list, directory] of [
    [mainModules, join(root, 'src', 'bootstrap', 'main', 'modules')],
    [rendererModules, join(root, 'src', 'bootstrap', 'renderer', 'modules')]
  ]) {
    for (const file of listFiles(directory, /\.tsx?$/)) {
      const module = file.replace(/^.*[\\/]/, '').replace(/\.tsx?$/, '');
      if (!list.has(module)) errors.push(`${toPosix(relative(root, file))} 没有登记在上一级目录的 module-list.ts`);
    }
  }
}

// ---------------------------------------------------------------- 官方插件

/** 插件目录里允许的文件，与插件包的格式一致（docs/03-mapper插件规范.md 第 3 节）。 */
const PLUGIN_FILES = new Set(['manifest.json', 'main.js', 'icon.png']);

/** 检查官方插件目录：只含允许的文件、有清单、清单里的 id 等于目录名。 */
function checkPlugins(root, errors) {
  const pluginsDir = join(root, 'plugins');
  for (const name of listDirectories(pluginsDir)) {
    const directory = join(pluginsDir, name);
    for (const entry of readdirSync(directory)) {
      if (!PLUGIN_FILES.has(entry) || statSync(join(directory, entry)).isDirectory()) errors.push(`plugins/${name}：不允许出现 ${entry}（只允许 ${[...PLUGIN_FILES].join('、')}）`);
    }
    const manifestFile = join(directory, 'manifest.json');
    if (!existsSync(manifestFile)) {
      errors.push(`plugins/${name}：缺少 manifest.json`);
      continue;
    }
    let manifest;
    try {
      manifest = JSON.parse(readFileSync(manifestFile, 'utf8'));
    } catch {
      errors.push(`plugins/${name}/manifest.json 不是合法的 JSON`);
      continue;
    }
    if (manifest?.id !== name) errors.push(`plugins/${name}：manifest.json 的 id 必须等于目录名`);
  }
}

// ---------------------------------------------------------------- 入口

/**
 * 检查一个项目目录。
 * @param {string} root 项目根目录。
 * @returns {{ errors: string[], edges: Array<{ from: string, to: string }>, schemaCount: number }} 错误（已去重）、模块之间的依赖与 Schema 模块数。
 */
export function checkProject(root) {
  const schemas = loadSchemas(root);
  const errors = [];
  const edges = schemas.flatMap((schema) => schema.refs.filter((ref) => ref.module !== schema.name).map((ref) => ({ from: schema.name, to: ref.module })));
  checkCpp(root, errors, edges);
  checkTs(root, errors, edges);
  checkPlugins(root, errors);
  for (const cycle of findCycles(toGraph(edges))) errors.push(`模块之间有循环依赖：${cycle}`);
  checkRegistration(
    root,
    errors,
    schemas.map((schema) => schema.name)
  );
  return { errors: [...new Set(errors)], edges, schemaCount: schemas.length };
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
  try {
    const { errors, edges, schemaCount } = checkProject(root);
    const index = process.argv.indexOf('--dependents');
    if (index >= 0) {
      const target = process.argv[index + 1];
      const dependents = dependentsOf(target, edges);
      console.log(dependents.length > 0 ? `依赖 ${target} 的模块：${dependents.join('、')}` : `没有模块依赖 ${target}`);
    }
    if (errors.length > 0) {
      for (const error of errors) console.error(error);
      process.exit(1);
    }
    console.log(`分层与模块检查通过：${schemaCount} 个 Schema 模块。`);
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
