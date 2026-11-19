/*
 * ------------------------------------------------------------------------
 * 名称：check-modules.test.ts
 * 说明：分层与模块检查的测试：本仓库通过检查；在临时项目里构造违规，检查硬件契约与硬件模块的包含规则、硬件模块之间的引用、模块只能引用对方契约、层间依赖与模块登记都能被发现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, describe, expect, it } from 'vitest';
import { checkProject } from '../tools/check/check-modules.mjs';

const REPO_ROOT = fileURLToPath(new URL('..', import.meta.url));
const roots: string[] = [];

/** 创建临时项目：文件按相对路径给出内容。 */
function createProject(files: Record<string, string>): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-check-'));
  roots.push(root);
  for (const [path, content] of Object.entries(files)) {
    mkdirSync(dirname(join(root, path)), { recursive: true });
    writeFileSync(join(root, path), content, 'utf8');
  }
  return root;
}

afterEach(() => {
  for (const root of roots.splice(0)) rmSync(root, { recursive: true, force: true });
});

describe('checkProject', () => {
  it('本仓库通过检查', () => {
    expect(checkProject(REPO_ROOT).errors).toEqual([]);
  });

  it('硬件模块经 contracts/hardware 协作且不要求登记', () => {
    const root = createProject({
      'core/src/contracts/hardware/bus.hpp': '#pragma once\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#include "contracts/hardware/bus.hpp"\n',
      'core/src/logic/hardware/ppu-2c02/ppu.hpp': '#include "contracts/hardware/bus.hpp"\n'
    });
    expect(checkProject(root).errors).toEqual([]);
  });

  it('硬件模块之间不能互相引用，同族模块也一样', () => {
    const root = createProject({
      'core/src/logic/hardware/cpu-um6576/cpu.hpp': '#include "logic/hardware/ppu-um6576/ppu.hpp"\n',
      'core/src/logic/hardware/ppu-um6576/ppu.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/cpu-um6576/cpu.hpp：模块 hardware/cpu-um6576 只能引用模块 hardware/ppu-um6576 的契约，不能包含 logic/hardware/ppu-um6576/ppu.hpp']);
  });

  it('业务模块不能直接引用硬件模块的实现', () => {
    const root = createProject({
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#pragma once\n',
      'core/src/logic/runtime/runtime.hpp': '#include "logic/hardware/cpu-6502/cpu.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/runtime/runtime.hpp：模块 runtime 只能引用模块 hardware/cpu-6502 的契约，不能包含 logic/hardware/cpu-6502/cpu.hpp']);
  });

  it('硬件模块可以包含自己目录里的文件、shared 与 platform 的抽象', () => {
    const root = createProject({
      'core/src/contracts/hardware/bus.hpp': '#pragma once\n',
      'core/src/shared/errors/core-error.hpp': '#pragma once\n',
      'core/src/platform/events/event-sink.hpp': '#pragma once\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#include "logic/hardware/cpu-6502/table.hpp"\n#include "contracts/hardware/bus.hpp"\n#include "shared/errors/core-error.hpp"\n#include "platform/events/event-sink.hpp"\n#include <cstdint>\n',
      'core/src/logic/hardware/cpu-6502/table.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual([]);
  });

  it('业务 logic 模块可以包含 platform 的帧输出与错误日志抽象，硬件模块不能', () => {
    const root = createProject({
      'core/src/contracts/hardware/bus.hpp': '#pragma once\n',
      'core/src/platform/streams/frame-output.hpp': '#pragma once\n',
      'core/src/platform/logging/error-log.hpp': '#pragma once\n',
      'core/src/logic/runtime/runtime.hpp': '#include "platform/streams/frame-output.hpp"\n#include "platform/logging/error-log.hpp"\n',
      'core/src/logic/hardware/ppu-2c02/ppu.hpp': '#include "contracts/hardware/bus.hpp"\n#include "platform/streams/frame-output.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/ppu-2c02/ppu.hpp：logic 层只能包含 platform 的抽象，不能包含 platform/streams/frame-output.hpp']);
  });

  it('硬件模块只能包含 contracts/hardware 的契约，不能包含其他模块的契约', () => {
    const root = createProject({
      'core/src/contracts/system/system-service.hpp': '#pragma once\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#include "contracts/system/system-service.hpp"\n#include "platform/time/clock.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"system", registerSystem},\n',
      'core/src/bootstrap/modules/system.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual([
      'core/src/logic/hardware/cpu-6502/cpu.hpp：硬件模块只能包含 contracts/hardware 的契约，不能包含 contracts/system/system-service.hpp',
      'core/src/logic/hardware/cpu-6502/cpu.hpp：logic 层只能包含 platform 的抽象，不能包含 platform/time/clock.hpp'
    ]);
  });

  it('硬件模块的文件必须放在 logic/hardware/<模块>/ 目录下', () => {
    const root = createProject({
      'core/src/logic/hardware/loose.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/loose.hpp：硬件模块的文件必须放在 logic/hardware/<模块>/ 目录下']);
  });

  it('mapper 芯片在 logic/hardware/mapper-chips/<芯片>/ 目录，不要求登记，经 contracts/hardware 协作，芯片之间与对其他硬件模块都不能互相引用', () => {
    const root = createProject({
      'core/src/contracts/hardware/irq-line.hpp': '#pragma once\n',
      'core/src/logic/hardware/mapper-chips/mmc3/mmc3.hpp': '#include "contracts/hardware/irq-line.hpp"\n',
      'core/src/logic/hardware/mapper-chips/vrc6/vrc6.hpp': '#include "contracts/hardware/irq-line.hpp"\n'
    });
    expect(checkProject(root).errors).toEqual([]);
    const crossing = createProject({
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#pragma once\n',
      'core/src/logic/hardware/mapper-chips/mmc3/mmc3.hpp': '#include "logic/hardware/cpu-6502/cpu.hpp"\n#include "logic/hardware/mapper-chips/vrc6/vrc6.hpp"\n',
      'core/src/logic/hardware/mapper-chips/vrc6/vrc6.hpp': '#pragma once\n'
    });
    expect(checkProject(crossing).errors).toEqual([
      'core/src/logic/hardware/mapper-chips/mmc3/mmc3.hpp：模块 hardware/mapper-chips/mmc3 只能引用模块 hardware/cpu-6502 的契约，不能包含 logic/hardware/cpu-6502/cpu.hpp',
      'core/src/logic/hardware/mapper-chips/mmc3/mmc3.hpp：模块 hardware/mapper-chips/mmc3 只能引用模块 hardware/mapper-chips/vrc6 的契约，不能包含 logic/hardware/mapper-chips/vrc6/vrc6.hpp'
    ]);
  });

  it('mapper 芯片的文件不能直接放在 logic/hardware/mapper-chips/ 下', () => {
    const root = createProject({
      'core/src/logic/hardware/mapper-chips/loose.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/mapper-chips/loose.hpp：mapper 芯片的文件必须放在 logic/hardware/mapper-chips/<芯片>/ 目录下']);
  });

  it('机型的装配按 bootstrap/machines/<机型>/ 分目录，只能包含硬件契约、硬件模块、shared 与自己目录和 common 里的文件', () => {
    const root = createProject({
      'core/src/contracts/hardware/irq-line.hpp': '#pragma once\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#pragma once\n',
      'core/src/shared/errors/core-error.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/common/pad-port.hpp': '#include "contracts/hardware/irq-line.hpp"\n',
      'core/src/bootstrap/machines/fc/fc.hpp': '#include "bootstrap/machines/common/pad-port.hpp"\n#include "bootstrap/machines/fc/fc-port.hpp"\n#include "logic/hardware/cpu-6502/cpu.hpp"\n#include "shared/errors/core-error.hpp"\n',
      'core/src/bootstrap/machines/fc/fc-port.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual([]);
  });

  it('机型不能包含其他机型、machine-catalog 与业务模块，common 不能包含任何机型', () => {
    const root = createProject({
      'core/src/contracts/runtime/runtime-service.hpp': '#pragma once\n',
      'core/src/logic/runtime/runtime.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/machine-catalog.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/sb2000/sb2000.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/fc/fc.hpp': '#include "bootstrap/machines/sb2000/sb2000.hpp"\n#include "bootstrap/machines/machine-catalog.hpp"\n#include "logic/runtime/runtime.hpp"\n',
      'core/src/bootstrap/machines/common/relay.hpp': '#include "bootstrap/machines/fc/fc.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual([
      'core/src/bootstrap/machines/common/relay.hpp：机型 common 只能包含自己目录与 common 里的文件，不能包含 bootstrap/machines/fc/fc.hpp',
      'core/src/bootstrap/machines/fc/fc.hpp：机型 fc 只能包含自己目录与 common 里的文件，不能包含 bootstrap/machines/sb2000/sb2000.hpp',
      'core/src/bootstrap/machines/fc/fc.hpp：机型 fc 只能包含自己目录与 common 里的文件，不能包含 bootstrap/machines/machine-catalog.hpp',
      'core/src/bootstrap/machines/fc/fc.hpp：机型 fc 只能包含硬件契约、硬件模块、shared 与 platform 的抽象，不能包含 logic/runtime/runtime.hpp'
    ]);
  });

  it('machine-catalog 可以包含各机型，其他机型文件不能直接放在 bootstrap/machines/ 下', () => {
    const root = createProject({
      'core/src/contracts/runtime/machine-factory.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/fc/fc.hpp': '#pragma once\n',
      'core/src/bootstrap/machines/machine-catalog.cpp': '#include "bootstrap/machines/fc/fc.hpp"\n#include "contracts/runtime/machine-factory.hpp"\n',
      'core/src/bootstrap/machines/loose.hpp': '#pragma once\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/bootstrap/machines/loose.hpp：机型的文件必须放在 bootstrap/machines/<机型>/ 或 bootstrap/machines/common/ 目录下']);
  });

  it('contracts/hardware 不能引用 logic 与第三方库，只能包含自己、shared 与标准库', () => {
    const root = createProject({
      'core/src/contracts/hardware/ok.hpp': '#pragma once\n#include <cstdint>\n#include <span>\n#include "contracts/hardware/other.hpp"\n#include "shared/errors/core-error.hpp"\n',
      'core/src/contracts/hardware/other.hpp': '#pragma once\n',
      'core/src/shared/errors/core-error.hpp': '#pragma once\n',
      'core/src/contracts/hardware/bad.hpp': '#include "logic/hardware/cpu-6502/cpu.hpp"\n#include "contracts/system/system-service.hpp"\n#include "platform/events/event-sink.hpp"\n#include <nlohmann/json.hpp>\n#include <quickjs.h>\n#include <stdint.h>\n',
      'core/src/logic/hardware/cpu-6502/cpu.hpp': '#pragma once\n'
    });
    expect(checkProject(root).errors).toEqual([
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含 contracts/hardware 与 shared，不能包含 logic/hardware/cpu-6502/cpu.hpp',
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含 contracts/hardware 与 shared，不能包含 contracts/system/system-service.hpp',
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含 contracts/hardware 与 shared，不能包含 platform/events/event-sink.hpp',
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含标准库头文件，不能包含 <nlohmann/json.hpp>',
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含标准库头文件，不能包含 <quickjs.h>',
      'core/src/contracts/hardware/bad.hpp：contracts/hardware 只能包含标准库头文件，不能包含 <stdint.h>'
    ]);
  });

  it('C++ 各层与第三方头文件的位置受限', () => {
    const root = createProject({
      'core/src/logic/system/a.hpp': '#include "infra/mappers/b.hpp"\n#include <quickjs.h>\n',
      'core/src/shared/c.hpp': '#include "platform/requests/cancellation.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"system", registerSystem},\n',
      'core/src/bootstrap/modules/system.cpp': '\n'
    });
    const { errors } = checkProject(root);
    expect(errors).toContain('core/src/logic/system/a.hpp：logic 层不能包含 infra 层的 infra/mappers/b.hpp');
    expect(errors).toContain('core/src/logic/system/a.hpp：不能包含 <quickjs.h>');
    expect(errors).toContain('core/src/shared/c.hpp：shared 层不能包含 platform 层的 platform/requests/cancellation.hpp');
  });

  it('原生应用（win32/app）符合分层时通过检查', () => {
    const root = createProject({
      'win32/app/src/contracts/shell/dialog.hpp': '#include <string>\n',
      'win32/app/src/shared/text/rules.hpp': '#include <nlohmann/json.hpp>\n',
      'win32/app/src/platform/window/frame.hpp': '#include <windows.h>\n#include "shared/text/rules.hpp"\n',
      'win32/app/src/infra/core-client/gateway.cpp': '#include "bootstrap/core.hpp"\n#include "platform/streams/frame-output.hpp"\n#include "contracts/shell/dialog.hpp"\n#include "platform/window/frame.hpp"\n',
      'win32/app/src/entry/main-requests/router.hpp': '#include "contracts/shell/dialog.hpp"\n',
      'win32/app/src/entry/cartridges/handlers.cpp': '#include "entry/main-requests/router.hpp"\n#include "contracts/shell/dialog.hpp"\n',
      'win32/app/src/ui/shell/view.cpp': '#include "rujian/ui/widget.hpp"\n#include "contracts/shell/dialog.hpp"\n',
      'win32/app/src/bootstrap/application.cpp': '#include <windows.h>\n#include "infra/core-client/gateway.cpp"\n#include "ui/shell/view.cpp"\n'
    });
    expect(checkProject(root).errors).toEqual([]);
  });

  it('原生应用的层间依赖、系统头文件、核心头文件与控件库头文件受限', () => {
    const root = createProject({
      'win32/app/src/shared/a.hpp': '#include "platform/window/frame.hpp"\n',
      'win32/app/src/platform/window/frame.hpp': '#include "contracts/shell/dialog.hpp"\n',
      'win32/app/src/contracts/shell/dialog.hpp': '#include <windows.h>\n#include "bootstrap/core.hpp"\n',
      'win32/app/src/entry/cartridges/handlers.cpp': '#include "infra/core-client/gateway.hpp"\n#include "rujian/ui/widget.hpp"\n',
      'win32/app/src/infra/core-client/gateway.hpp': '#include "platform/other.hpp"\n#include "somewhere/not-there.hpp"\n',
      'win32/app/src/ui/shell/view.cpp': '#include "entry/main-requests/router.hpp"\n',
      'win32/app/src/entry/main-requests/router.hpp': '\n'
    });
    const { errors } = checkProject(root);
    expect(errors).toContain('win32/app/src/shared/a.hpp：shared 层不能包含 platform 层的 platform/window/frame.hpp');
    expect(errors).toContain('win32/app/src/platform/window/frame.hpp：platform 层不能包含 contracts 层的 contracts/shell/dialog.hpp');
    expect(errors).toContain('win32/app/src/contracts/shell/dialog.hpp：contracts 层不能包含系统头文件 <windows.h>');
    expect(errors).toContain('win32/app/src/contracts/shell/dialog.hpp：contracts 层不能包含核心的 bootstrap/core.hpp');
    expect(errors).toContain('win32/app/src/entry/cartridges/handlers.cpp：entry 层不能包含 infra 层的 infra/core-client/gateway.hpp');
    expect(errors).toContain('win32/app/src/entry/cartridges/handlers.cpp：entry 层不能包含控件库的 rujian/ui/widget.hpp');
    expect(errors).toContain('win32/app/src/infra/core-client/gateway.hpp：找不到 somewhere/not-there.hpp，核心的头文件只能包含 bootstrap/core.hpp、platform/streams/frame-output.hpp');
    expect(errors).toContain('win32/app/src/ui/shell/view.cpp：ui 层不能包含 entry 层的 entry/main-requests/router.hpp');
  });

  it('原生应用的模块之间只能引用对方的契约，且不能循环依赖', () => {
    const root = createProject({
      'win32/app/src/infra/dialogs/a.hpp': '#include "infra/directories/b.hpp"\n',
      'win32/app/src/infra/directories/b.hpp': '#include "infra/dialogs/a.hpp"\n',
      'win32/app/src/infra/dialogs/own.hpp': '#include "infra/dialogs/a.hpp"\n',
      'win32/app/src/loose.hpp': '\n',
      'win32/app/src/infra/direct.hpp': '\n',
      'win32/app/src/other/x.hpp': '\n'
    });
    const { errors } = checkProject(root);
    expect(errors).toContain('win32/app/src/infra/dialogs/a.hpp：模块 dialogs 只能引用模块 directories 的契约，不能包含 infra/directories/b.hpp');
    expect(errors).toContain('win32/app/src/infra/directories/b.hpp：模块 directories 只能引用模块 dialogs 的契约，不能包含 infra/dialogs/a.hpp');
    expect(errors.some((error) => error.startsWith('win32/app：模块之间有循环依赖'))).toBe(true);
    expect(errors).toContain('win32/app/src/infra/direct.hpp：infra 层的文件必须放在 infra/<模块>/ 目录下');
    expect(errors.some((error) => error.startsWith('win32/app/src/other：不是八层之一'))).toBe(true);
  });

  it('发现没有登记的模块', () => {
    const root = createProject({
      'schema/demo.schema.json': JSON.stringify({ module: 'demo', types: {}, requests: {}, events: {} }),
      'core/src/logic/other/service.hpp': '#pragma once\n',
      'core/src/bootstrap/module-list.cpp': '{"stale", registerStale},\n'
    });
    expect(checkProject(root).errors).toEqual([
      '模块 demo 有 Schema，但没有登记在 core/src/bootstrap/module-list.cpp',
      '模块 other 有 core/src/logic 目录，但没有登记在 core/src/bootstrap/module-list.cpp',
      '模块 stale 登记在 C++ 模块清单中，但没有 core/src/bootstrap/modules/stale.cpp'
    ]);
  });
  it('官方插件目录：目录名等于清单里的 id，只含 manifest.json、main.js、icon.png', () => {
    const root = createProject({
      'plugins/good/manifest.json': JSON.stringify({ id: 'good' }),
      'plugins/good/main.js': '',
      'plugins/wrong/manifest.json': JSON.stringify({ id: 'other' }),
      'plugins/extra/manifest.json': JSON.stringify({ id: 'extra' }),
      'plugins/extra/readme.md': 'x',
      'plugins/nomanifest/main.js': '',
      'plugins/broken/manifest.json': '{'
    });
    expect(checkProject(root).errors).toEqual([
      'plugins/broken/manifest.json 不是合法的 JSON',
      'plugins/extra：不允许出现 readme.md（只允许 manifest.json、main.js、icon.png）',
      'plugins/nomanifest：缺少 manifest.json',
      'plugins/wrong：manifest.json 的 id 必须等于目录名'
    ]);
  });

  it('mapper 插件宿主（infra/mappers）可以用 QuickJS 与 miniz，logic 与 contracts 不行', () => {
    const root = createProject({
      'core/src/infra/mappers/host.cpp': '#include <quickjs.h>\n#include <miniz.h>\n#include "contracts/mappers/api.hpp"\n',
      'core/src/contracts/mappers/api.hpp': '#pragma once\n',
      'core/src/logic/cartridges/loader.cpp': '#include <quickjs.h>\n#include "infra/mappers/host.hpp"\n#include "contracts/mappers/api.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"mappers", registerMappers},\n{"cartridges", registerCartridges},\n',
      'core/src/bootstrap/modules/mappers.cpp': '\n',
      'core/src/bootstrap/modules/cartridges.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual([
      'core/src/logic/cartridges/loader.cpp：不能包含 <quickjs.h>',
      'core/src/logic/cartridges/loader.cpp：logic 层不能包含 infra 层的 infra/mappers/host.hpp',
      'core/src/logic/cartridges/loader.cpp：模块 cartridges 只能引用模块 mappers 的契约，不能包含 infra/mappers/host.hpp'
    ]);
  });

  it('业务 logic 模块可以包含 platform 的声音输出抽象，硬件模块不能', () => {
    const root = createProject({
      'core/src/contracts/hardware/bus.hpp': '#pragma once\n',
      'core/src/platform/audio/audio-output.hpp': '#pragma once\n',
      'core/src/logic/runtime/runtime.hpp': '#include "platform/audio/audio-output.hpp"\n',
      'core/src/logic/hardware/apu-2a03/apu.hpp': '#include "contracts/hardware/bus.hpp"\n#include "platform/audio/audio-output.hpp"\n',
      'core/src/bootstrap/module-list.cpp': '{"runtime", registerRuntime},\n',
      'core/src/bootstrap/modules/runtime.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/logic/hardware/apu-2a03/apu.hpp：logic 层只能包含 platform 的抽象，不能包含 platform/audio/audio-output.hpp']);
  });

  it('miniaudio 的头文件只能出现在 platform/audio', () => {
    const root = createProject({
      'core/src/platform/audio/miniaudio-output.cpp': '#include <miniaudio.h>\n',
      'core/src/infra/devices/device.cpp': '#include <miniaudio.h>\n',
      'core/src/bootstrap/module-list.cpp': '{"devices", registerDevices},\n',
      'core/src/bootstrap/modules/devices.cpp': '\n'
    });
    expect(checkProject(root).errors).toEqual(['core/src/infra/devices/device.cpp：不能包含 <miniaudio.h>']);
  });

  it('发现模块之间的循环依赖', () => {
    const root = createProject({
      'schema/a.schema.json': JSON.stringify({ module: 'a', types: { X: { type: 'object', properties: { b: { $ref: 'b#/types/Y' } } } }, requests: {}, events: {} }),
      'schema/b.schema.json': JSON.stringify({ module: 'b', types: { Y: { type: 'object', properties: { a: { $ref: 'a#/types/X' } } } }, requests: {}, events: {} })
    });
    expect(checkProject(root).errors).toContain('模块之间有循环依赖：a → b → a');
  });
});
