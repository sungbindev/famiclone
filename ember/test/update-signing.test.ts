/*
 * ------------------------------------------------------------------------
 * 名称：update-signing.test.ts
 * 说明：升级签名与发布脚本的测试：被签名的文字格式、清单的生成与签名能用公钥校验、改动任何一项签名都对不上、复制顺序（程序文件在前、清单在最后）、公钥头文件的生成与比对、不合格的版本号被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用测试里现生成的小密钥（2048 位），不读用户目录里的私钥，不访问网络。
 * ------------------------------------------------------------------------
 */

import { createHash, createPublicKey, createVerify, generateKeyPairSync } from 'node:crypto';
import { describe, expect, it } from 'vitest';
import { buildLatest, planFeedFiles, publicKeyMatches } from '../tools/release/publish-update.mjs';
import { buildManifest, digestOf, formatPublicKeyHeader, publicKeyParts, signedMessage } from '../tools/release/update-signing.mjs';

const { privateKey } = generateKeyPairSync('rsa', { modulusLength: 2048 });
const CONTENT = Buffer.from('MZ-fake-ember-exe');

describe('升级签名', () => {
  it('被签名的文字是五行，换行分隔，结尾没有换行', () => {
    expect(signedMessage({ version: '1.2.3', file: 'Ember-1.2.3.exe', size: 17, sha256: 'ab' })).toBe('ember-update\n1.2.3\nEmber-1.2.3.exe\n17\nab');
  });

  it('清单里的大小与 SHA-256 取自程序文件，签名能用公钥校验', () => {
    const manifest = buildManifest(privateKey, { version: '0.3.0', content: CONTENT });
    expect(manifest.file).toBe('Ember-0.3.0.exe');
    expect(manifest.size).toBe(CONTENT.length);
    expect(manifest.sha256).toBe(createHash('sha256').update(CONTENT).digest('hex'));
    expect(digestOf(CONTENT)).toEqual({ sha256: manifest.sha256, size: CONTENT.length });
    const verify = (fields: typeof manifest): boolean => createVerify('sha256').update(signedMessage(fields)).verify(createPublicKey(privateKey), Buffer.from(manifest.signature, 'base64'));
    expect(verify(manifest)).toBe(true);
    expect(verify({ ...manifest, version: '0.3.1' })).toBe(false);
    expect(verify({ ...manifest, size: manifest.size + 1 })).toBe(false);
    expect(verify({ ...manifest, sha256: '0'.repeat(64) })).toBe(false);
    expect(verify({ ...manifest, file: 'Ember-9.9.9.exe' })).toBe(false);
  });

  it('latest.json 是带换行结尾的 JSON，字段齐全', () => {
    const text = buildLatest(privateKey, '0.3.0', CONTENT);
    expect(text.endsWith('\n')).toBe(true);
    const json = JSON.parse(text) as Record<string, unknown>;
    expect(Object.keys(json).toSorted()).toEqual(['file', 'sha256', 'signature', 'size', 'version']);
  });

  it('版本号必须是主.次.修订', () => {
    for (const version of ['1.2', '1.2.3.4', 'v1.2.3', '1.2.x', '']) expect(() => buildLatest(privateKey, version, CONTENT)).toThrow('版本号');
  });

  it('先复制程序文件、最后复制清单', () => {
    expect(planFeedFiles('0.3.0')).toEqual(['Ember-0.3.0.exe', 'Ember-0.3.0.zip', 'latest.json']);
  });

  it('公钥头文件与私钥一致才算对得上', () => {
    const header = formatPublicKeyHeader(publicKeyParts(privateKey));
    expect(header).toContain('kUpdatePublicModulus');
    expect(header).toContain('kUpdatePublicExponent');
    expect(header).toContain('RSA 2048 位');
    expect(publicKeyMatches(privateKey, header)).toBe(true);
    expect(publicKeyMatches(privateKey, header.replaceAll('\n', '\r\n'))).toBe(true);
    const other = generateKeyPairSync('rsa', { modulusLength: 2048 }).privateKey;
    expect(publicKeyMatches(other, header)).toBe(false);
  });

  it('公钥的指数是 0x010001', () => {
    expect([...publicKeyParts(privateKey).exponent]).toEqual([1, 0, 1]);
  });
});
