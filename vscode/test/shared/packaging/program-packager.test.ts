// ------------------------------------------------------------------------
// 名称：program-packager.test.ts
// 说明：测试打包器：文件头模式、flat-exe 的文件头与大小校验、iNES 校验、磁盘打包器。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { buildFat12Image, listFat12 } from '../../../src/shared/packaging/fat12-image';
import { packageDisk, packageProgram, parseHeaderPattern } from '../../../src/shared/packaging/program-packager';

describe('打包器', () => {
  it('未知打包器报错', () => {
    expect(() => packageProgram('nope', new Uint8Array(1), {})).toThrow(expect.objectContaining({ code: 'PACKAGER_UNKNOWN' }));
    expect(() => packageDisk('nope', [])).toThrow(expect.objectContaining({ code: 'PACKAGER_UNKNOWN' }));
  });

  it('文件头模式里 ?? 是通配', () => {
    expect(parseHeaderPattern('4C????AA55')).toEqual([0x4c, null, null, 0xaa, 0x55]);
    expect(() => parseHeaderPattern('4G')).toThrow(expect.objectContaining({ code: 'PACKAGER_OPTION_INVALID' }));
  });

  it('flat-exe 校验文件头后原样写出', () => {
    const image = Uint8Array.from([0x4c, 0x05, 0x80, 0xaa, 0x55, 0x00, 0x41, 0x42, 0x60]);
    expect(packageProgram('flat-exe', image, { header: '4C????AA55', maxSize: 16 })).toEqual(image);
  });

  it('flat-exe 文件头不符时报错并给出实际内容', () => {
    expect(() => packageProgram('flat-exe', Uint8Array.from([0x60, 0, 0, 0, 0]), { header: '4C????AA55' })).toThrow(
      expect.objectContaining({ code: 'PACKAGE_HEADER_MISMATCH', message: expect.stringContaining('6000000000') })
    );
  });

  it('flat-exe 超过大小上限时报错', () => {
    expect(() => packageProgram('flat-exe', new Uint8Array(17), { maxSize: 16 })).toThrow(expect.objectContaining({ code: 'PACKAGE_TOO_LARGE' }));
  });

  it('flat-exe 的文件头可以在指定偏移处，偏移处不符时报错并给出偏移与实际内容', () => {
    const image = new Uint8Array(16);
    image.set([0xaa, 0x55], 8);
    expect(packageProgram('flat-exe', image, { header: 'AA55', headerOffset: 8 })).toEqual(image);
    expect(() => packageProgram('flat-exe', image, { header: 'AA55', headerOffset: 7 })).toThrow(
      expect.objectContaining({ code: 'PACKAGE_HEADER_MISMATCH', message: expect.stringContaining('偏移 7') })
    );
    expect(() => packageProgram('flat-exe', new Uint8Array(9), { header: 'AA55', headerOffset: 8 })).toThrow(expect.objectContaining({ code: 'PACKAGE_HEADER_MISMATCH' }));
    expect(() => packageProgram('flat-exe', image, { header: 'AA55', headerOffset: -1 })).toThrow(expect.objectContaining({ code: 'PACKAGER_OPTION_INVALID' }));
  });

  it('flat-exe 的 size 要求映像大小正好相等', () => {
    expect(packageProgram('flat-exe', new Uint8Array(16), { size: 16 }).length).toBe(16);
    expect(() => packageProgram('flat-exe', new Uint8Array(15), { size: 16 })).toThrow(expect.objectContaining({ code: 'PACKAGE_SIZE_MISMATCH' }));
    expect(() => packageProgram('flat-exe', new Uint8Array(17), { size: 16 })).toThrow(expect.objectContaining({ code: 'PACKAGE_SIZE_MISMATCH' }));
  });

  it('ines 接受文件头与长度一致的文件', () => {
    const header = [0x4e, 0x45, 0x53, 0x1a, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    const image = Uint8Array.from([...header, ...Array.from({ length: 0x6000 }, () => 0)]);
    expect(packageProgram('ines', image, {}).length).toBe(16 + 0x6000);
  });

  it('ines 拒绝缺少 NES 文件头或长度不符的文件', () => {
    expect(() => packageProgram('ines', new Uint8Array(32), {})).toThrow(expect.objectContaining({ code: 'PACKAGE_HEADER_MISMATCH' }));
    const wrongSize = Uint8Array.from([0x4e, 0x45, 0x53, 0x1a, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
    expect(() => packageProgram('ines', wrongSize, {})).toThrow(expect.objectContaining({ code: 'PACKAGE_SIZE_MISMATCH' }));
  });

  describe('rom-overlay', () => {
    /** 底固件：8 字节，每字节等于它的下标。 */
    const base = Uint8Array.from([0, 1, 2, 3, 4, 5, 6, 7]);
    const context = (targetOptions: Record<string, unknown>, withBase = true) => ({ targetOptions, readBase: () => (withBase ? base : undefined) });

    it('把链接输出覆盖到底固件的 offset 处，不改动底固件本身', () => {
      const result = packageProgram('rom-overlay', Uint8Array.from([0xaa, 0xbb]), {}, context({ offset: 6, size: 8 }));
      expect(Array.from(result)).toEqual([0, 1, 2, 3, 4, 5, 0xaa, 0xbb]);
      expect(Array.from(base)).toEqual([0, 1, 2, 3, 4, 5, 6, 7]);
    });

    it('没有底固件且 offset 为 0 时，链接输出就是整块固件，size 要求大小正好相等', () => {
      expect(packageProgram('rom-overlay', new Uint8Array(4), {}, context({ size: 4 }, false)).length).toBe(4);
      expect(() => packageProgram('rom-overlay', new Uint8Array(3), {}, context({ size: 4 }, false))).toThrow(expect.objectContaining({ code: 'PACKAGE_SIZE_MISMATCH' }));
    });

    it('offset 不为 0 而没有底固件时报错并提示配置 firmware', () => {
      expect(() => packageProgram('rom-overlay', new Uint8Array(2), {}, context({ offset: 4 }, false))).toThrow(
        expect.objectContaining({ code: 'PACKAGE_BASE_MISSING', hint: expect.stringContaining('firmware') })
      );
    });

    it('底固件大小与 size 不符，或链接输出放不进底固件时报错', () => {
      expect(() => packageProgram('rom-overlay', new Uint8Array(2), {}, context({ offset: 0, size: 16 }))).toThrow(expect.objectContaining({ code: 'PACKAGE_SIZE_MISMATCH' }));
      expect(() => packageProgram('rom-overlay', new Uint8Array(3), {}, context({ offset: 6 }))).toThrow(expect.objectContaining({ code: 'PACKAGE_TOO_LARGE' }));
    });

    it('offset 与 size 必须是不小于 0 的整数', () => {
      expect(() => packageProgram('rom-overlay', new Uint8Array(1), {}, context({ offset: -1 }))).toThrow(expect.objectContaining({ code: 'PACKAGER_OPTION_INVALID' }));
      expect(() => packageProgram('rom-overlay', new Uint8Array(1), {}, context({ size: 1.5 }))).toThrow(expect.objectContaining({ code: 'PACKAGER_OPTION_INVALID' }));
    });
  });

  it('fat-floppy 把文件放进镜像根目录', () => {
    const image = packageDisk('fat-floppy', [{ name: 'a.exe', data: Uint8Array.from([1, 2, 3]) }], { label: 'FCDEV' });
    expect(listFat12(image).map((entry) => entry.name)).toEqual(['A.EXE']);
  });

  it('fat-floppy-overlay 在底盘镜像上添加文件，没有底盘镜像时报错并提示 diskImage', () => {
    const base = buildFat12Image([{ name: 'system.sys', data: Uint8Array.from([9]) }]);
    const image = packageDisk('fat-floppy-overlay', [{ name: 'a.cmd', data: Uint8Array.from([1, 2, 3]) }], { base });
    expect(listFat12(image).map((entry) => entry.name)).toEqual(['SYSTEM.SYS', 'A.CMD']);
    expect(() => packageDisk('fat-floppy-overlay', [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_MISSING', hint: expect.stringContaining('diskImage') }));
  });
});
