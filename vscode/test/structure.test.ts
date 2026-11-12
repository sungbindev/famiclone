// ------------------------------------------------------------------------
// 名称：structure.test.ts
// 说明：结构检查：各层的依赖规则、同层功能目录之间不互相引用、源文件都有文件头、
//       注释行宽不超过 100 列，以及扩展清单与代码里的标识保持一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只读源码文本，不加载被检查的模块；违反规则时测试失败，失败信息写明文件与引用。
// ------------------------------------------------------------------------

import { readdirSync, readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it } from 'vitest';

const ROOT = path.resolve(__dirname, '..');
const SRC = path.join(ROOT, 'src');

/** 每层允许引用的其他层。 */
const ALLOWED_LAYERS: Record<string, readonly string[]> = {
  contracts: ['contracts', 'shared'],
  shared: ['shared'],
  logic: ['contracts', 'shared', 'platform'],
  infra: ['contracts', 'shared', 'platform'],
  entry: ['contracts', 'shared', 'platform'],
  platform: ['platform'],
  bootstrap: ['contracts', 'shared', 'logic', 'infra', 'entry', 'platform']
};

/** 每层允许引用的外部包（前缀匹配）；没有列出的层不能引用任何外部包。 */
const ALLOWED_EXTERNALS: Record<string, readonly string[]> = {
  infra: ['vscode', 'node:'],
  entry: ['vscode', 'node:'],
  platform: ['vscode', 'node:'],
  bootstrap: ['vscode', 'node:']
};

/** 逻辑层与入口层只能引用平台层的抽象，即不依赖 vscode 的文件。 */
const ABSTRACT_ONLY_LAYERS = ['logic', 'entry'];

/** 同层内，一个功能目录不能引用另一个功能目录的层。 */
const FEATURE_ISOLATED_LAYERS = ['logic', 'infra', 'entry'];

/** 注释行的最大显示宽度：中文字符按 2 列计。 */
const MAX_COMMENT_WIDTH = 100;

/** 递归列出目录下符合扩展名的文件。 */
function listFiles(directory: string, extensions: readonly string[]): string[] {
  return readdirSync(directory).flatMap((name) => {
    const full = path.join(directory, name);
    if (statSync(full).isDirectory()) return listFiles(full, extensions);
    return extensions.includes(path.extname(name)) ? [full] : [];
  });
}

/** 相对扩展根目录的路径，统一用“/”。 */
function relative(file: string): string {
  return path.relative(ROOT, file).split(path.sep).join('/');
}

/** 取出源码里的全部引用路径。 */
function importsOf(source: string): string[] {
  const pattern = /(?:import|export)\s+(?:type\s+)?(?:[^'";]*?\s+from\s+)?['"]([^'"]+)['"]/g;
  return [...source.matchAll(pattern)].map((match) => match[1] ?? '');
}

/** 源文件所在的层与功能目录。 */
function locate(file: string): { layer: string; feature: string } {
  const [layer = '', feature = ''] = path.relative(SRC, file).split(path.sep);
  return { layer, feature };
}

/** 文字的显示宽度：中文、全角字符按 2 列，其他按 1 列。 */
function displayWidth(text: string): number {
  let width = 0;
  for (const char of text) {
    const code = char.codePointAt(0) ?? 0;
    const wide =
      (code >= 0x2e80 && code <= 0xa4cf) ||
      (code >= 0xac00 && code <= 0xd7a3) ||
      (code >= 0xf900 && code <= 0xfaff) ||
      (code >= 0xff00 && code <= 0xff60) ||
      (code >= 0xffe0 && code <= 0xffe6) ||
      (code >= 0x3000 && code <= 0x303f) ||
      (code >= 0x2018 && code <= 0x201d) ||
      code === 0x2026 ||
      code === 0x2014;
    width += wide ? 2 : 1;
  }
  return width;
}

/** 取出源文件里某个常量的字符串值。 */
function constantOf(file: string, name: string): string {
  const match = new RegExp(`const ${name} = '([^']+)'`).exec(readFileSync(path.join(SRC, file), 'utf8'));
  return match?.[1] ?? '';
}

const sourceFiles = listFiles(SRC, ['.ts']);
const testFiles = listFiles(path.join(ROOT, 'test'), ['.ts']);

describe('依赖规则', () => {
  const problems: string[] = [];
  for (const file of sourceFiles) {
    const { layer, feature } = locate(file);
    const allowed = ALLOWED_LAYERS[layer];
    if (allowed === undefined) {
      problems.push(`${relative(file)}：不在已登记的层里`);
      continue;
    }
    for (const specifier of importsOf(readFileSync(file, 'utf8'))) {
      if (specifier.startsWith('.')) {
        const target = path.resolve(path.dirname(file), specifier);
        const where = locate(target + '.ts');
        if (!target.startsWith(SRC)) {
          problems.push(`${relative(file)}：引用了源码目录之外的 ${specifier}`);
          continue;
        }
        if (!allowed.includes(where.layer) && where.layer !== layer) {
          problems.push(`${relative(file)}：${layer} 层不能引用 ${where.layer} 层（${specifier}）`);
        }
        if (where.layer === layer && FEATURE_ISOLATED_LAYERS.includes(layer) && where.feature !== feature) {
          problems.push(`${relative(file)}：${layer} 层的 ${feature} 不能引用 ${where.feature}（${specifier}）`);
        }
        if (ABSTRACT_ONLY_LAYERS.includes(layer) && where.layer === 'platform') {
          const concrete = `${target}.ts`;
          if (/from\s+['"]vscode['"]/.test(readFileSync(concrete, 'utf8'))) {
            problems.push(`${relative(file)}：${layer} 层只能引用平台层的抽象，不能引用依赖 vscode 的实现（${specifier}）`);
          }
        }
      } else {
        const externals = ALLOWED_EXTERNALS[layer] ?? [];
        if (!externals.some((prefix) => specifier === prefix || specifier.startsWith(prefix.endsWith(':') ? prefix : `${prefix}/`))) {
          problems.push(`${relative(file)}：${layer} 层不能引用外部包 ${specifier}`);
        }
      }
    }
  }

  it('没有违反层间依赖、功能目录隔离与外部包限制', () => {
    expect(problems).toEqual([]);
  });
});

describe('文件头', () => {
  const required = [/名称：/, /说明：/, /作者：sungbin/, /邮箱：sungbin@3578\.cn/, /日期：\d{4}-\d{2}-\d{2}/, /备注：/];

  it('每个源文件与测试文件顶部都有完整的文件头', () => {
    const missing = [...sourceFiles, ...testFiles].filter((file) => {
      const head = readFileSync(file, 'utf8').split(/\r?\n/).slice(0, 14).join('\n');
      return required.some((pattern) => !pattern.test(head));
    });
    expect(missing.map(relative)).toEqual([]);
  });
});

describe('注释行宽', () => {
  it(`每行注释的显示宽度不超过 ${MAX_COMMENT_WIDTH} 列`, () => {
    const tooWide: string[] = [];
    for (const file of [...sourceFiles, ...testFiles]) {
      readFileSync(file, 'utf8')
        .split(/\r?\n/)
        .forEach((line, index) => {
          const trimmed = line.trim();
          const isComment = trimmed.startsWith('//') || trimmed.startsWith('/*') || trimmed.startsWith('*');
          if (isComment && displayWidth(line) > MAX_COMMENT_WIDTH) tooWide.push(`${relative(file)}:${index + 1}（${displayWidth(line)} 列）`);
        });
    }
    expect(tooWide).toEqual([]);
  });
});

describe('扩展清单与代码一致', () => {
  const manifest = JSON.parse(readFileSync(path.join(ROOT, 'package.json'), 'utf8')) as {
    main: string;
    contributes: {
      languages: Array<{ id: string; extensions: string[]; configuration: string }>;
      grammars: Array<{ language: string; scopeName: string; path: string }>;
      commands: Array<{ command: string }>;
      configuration: { properties: Record<string, { type: string; default: unknown }> };
    };
  };

  it('汇编语言的标识一致，语言配置文件存在', () => {
    const id = constantOf('entry/assembly/assembly-conversion.ts', 'ASSEMBLY_LANGUAGE_ID');
    const language = manifest.contributes.languages.find((item) => item.id === id);
    expect(language).toBeDefined();
    expect(statSync(path.join(ROOT, language?.configuration ?? 'missing')).isFile()).toBe(true);
  });

  it('语法文件的语言与作用域名一致，文件存在', () => {
    const id = constantOf('entry/assembly/assembly-conversion.ts', 'ASSEMBLY_LANGUAGE_ID');
    const grammar = manifest.contributes.grammars.find((item) => item.language === id);
    expect(grammar).toBeDefined();
    const file = path.join(ROOT, grammar?.path ?? 'missing');
    expect((JSON.parse(readFileSync(file, 'utf8')) as { scopeName: string }).scopeName).toBe(grammar?.scopeName);
  });

  it('代码注册的命令在清单里声明', () => {
    const declared = manifest.contributes.commands.map((item) => item.command);
    expect(declared).toContain(constantOf('entry/c-intellisense/c-intellisense-commands.ts', 'SYNC_COMMAND'));
  });

  it('自动同步设置在清单里声明，类型为布尔，默认开启', () => {
    const section = constantOf('entry/c-intellisense/c-intellisense-commands.ts', 'SETTINGS_SECTION');
    const key = constantOf('entry/c-intellisense/c-intellisense-commands.ts', 'AUTO_SYNC_KEY');
    expect(manifest.contributes.configuration.properties[`${section}.${key}`]).toMatchObject({ type: 'boolean', default: true });
  });

  it('提示选择配置的设置在清单里声明，类型为布尔，默认开启', () => {
    const section = constantOf('entry/c-intellisense/c-intellisense-commands.ts', 'SETTINGS_SECTION');
    const key = constantOf('entry/c-intellisense/c-intellisense-commands.ts', 'PROMPT_KEY');
    expect(manifest.contributes.configuration.properties[`${section}.${key}`]).toMatchObject({ type: 'boolean', default: true });
  });

  it('扩展入口指向编译产物，且组装根导出 activate', () => {
    expect(manifest.main).toBe('./out/bootstrap/extension.js');
    expect(readFileSync(path.join(SRC, 'bootstrap', 'extension.ts'), 'utf8')).toMatch(/export function activate\(/);
  });
});
