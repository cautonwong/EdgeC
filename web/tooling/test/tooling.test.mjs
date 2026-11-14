// 构建期工具与生成产物的测试（node:test）。
// 这些用例的价值在于：把"文档里的规则"变成会红的断言。
import test from 'node:test';
import assert from 'node:assert/strict';

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

import { canonicalJson, digestOf, listFiles, sha256 } from '../lib/util.mjs';
import { buildRegistry } from '../lib/registry.mjs';
import { loadCatalog, loadCoreManifest, loadProduct, expandPermissions } from '../lib/manifest.mjs';
import { checkBoundaries } from '../lib/boundaries.mjs';
import { checkContrast } from '../lib/contrast.mjs';
import { checkSize } from '../lib/size.mjs';

test('规范化 JSON 与摘要与键序无关', () => {
  const a = { b: 1, a: { d: 2, c: [3, 1] } };
  const b = { a: { c: [3, 1], d: 2 }, b: 1 };
  assert.equal(canonicalJson(a), canonicalJson(b));
  assert.equal(digestOf(a), digestOf(b));
  assert.match(digestOf(a), /^sha256:[0-9a-f]{64}$/);
  assert.notEqual(digestOf(a), digestOf({ ...a, b: 2 }));
  assert.equal(sha256('x'), sha256('x'));
});

test('Registry 生成是确定性的（同输入同字节）', () => {
  const first = buildRegistry('iems', { write: false }).registry;
  const second = buildRegistry('iems', { write: false }).registry;
  assert.equal(first.digests.registry, second.digests.registry);
  assert.deepEqual(first.pages, second.pages);
  assert.deepEqual(first.api, second.api);
});

test('release-scope 关闭的功能不得残留在产物里', () => {
  const iems = buildRegistry('iems', { write: false }).registry;
  assert.ok(
    !iems.api.some((op) => op.feature === 'firmwareUpgrade'),
    'iEMS 关闭了 firmwareUpgrade，升级接口不得出现在 Registry',
  );
  assert.ok(iems.releaseScope.disabled.includes('firmwareUpgrade'));

  const dcu = buildRegistry('dcu', { write: false }).registry;
  assert.ok(
    dcu.api.some((op) => op.feature === 'firmwareUpgrade'),
    'DCU 启用了 firmwareUpgrade，升级接口应当生成',
  );
});

test('通配符不会漏进运行时 Registry', () => {
  const { registry } = buildRegistry('iems', { write: false });
  const all = registry.roles.flatMap((role) => role.permissions);
  assert.ok(all.length > 0);
  for (const id of all) assert.ok(!id.includes('*'), `角色权限里出现了通配符: ${id}`);
  for (const page of registry.pages) {
    for (const id of [...page.permissions.allOf, ...(page.permissions.anyOf ?? [])]) {
      assert.ok(!id.includes('*'), `页面权限里出现了通配符: ${id}`);
    }
  }
});

test('每个页面/接口引用的权限都能在目录中解析', () => {
  const catalog = loadCatalog();
  const { registry } = buildRegistry('iems', { write: false });
  for (const page of registry.pages) {
    for (const id of [...page.permissions.allOf, ...(page.permissions.anyOf ?? [])]) {
      assert.ok(catalog.byId.has(id), `页面 ${page.id} 引用了未声明权限 ${id}`);
    }
  }
  for (const op of registry.api) {
    for (const id of op.permissions) assert.ok(catalog.byId.has(id), `接口 ${op.operationId} 引用了未声明权限 ${id}`);
  }
});

test('匿名接口只允许出现在 allowlist 里', () => {
  const { registry } = buildRegistry('iems', { write: false });
  const anonymous = registry.api.filter((op) => op.anonymous).map((op) => `${op.method} ${op.path}`).sort();
  assert.deepEqual(anonymous, ['GET /api/v1/system/info', 'POST /api/v1/auth/login']);
});

test('所有接口都带 /api/v1 前缀，Pack 接口带产品前缀', () => {
  const { registry } = buildRegistry('iems', { write: false });
  for (const op of registry.api) {
    assert.ok(op.path.startsWith('/api/v1/'), `${op.operationId} 路径缺少 /api/v1: ${op.path}`);
    if (op.origin === 'pack') assert.ok(op.path.startsWith('/api/v1/iems/'), `${op.operationId} 未按 Pack 前缀命名: ${op.path}`);
  }
});

test('页面可见性依赖的页面声明了 requires 能力', () => {
  const { registry } = buildRegistry('iems', { write: false });
  const cellular = registry.pages.find((page) => page.id === 'core.network.cellular');
  assert.ok(cellular, '蜂窝页面应当存在');
  assert.deepEqual(cellular.requires.capabilities, ['hardware.network.cellular']);
});

test('依赖边界没有违规', async () => {
  const violations = await checkBoundaries();
  assert.deepEqual(violations, []);
});

test('设计 Token 的语义配对达到对比度要求', () => {
  const { results, violations } = checkContrast();
  assert.deepEqual(violations, []);
  assert.ok(results.length >= 15, `配对数量偏少: ${results.length}`);
});

test('体积预算：未构建时跳过而不是误报通过', () => {
  const result = checkSize('dcu');
  assert.ok(result.skipped === true || Array.isArray(result.rows));
});

test('Core 模块清单与产品清单可加载且互相一致', () => {
  const core = loadCoreManifest();
  const product = loadProduct('iems');
  assert.ok(core.modules.size >= 6);
  for (const modId of Object.keys(product.product.modules)) {
    assert.ok(core.modules.has(modId), `product.yaml 引用了不存在的 Core 模块: ${modId}`);
  }
});

test('通配符展开对未知域返回空集而不是抛错', () => {
  const catalog = loadCatalog();
  assert.deepEqual(expandPermissions(['unknown.*:*'], catalog, null, 'test'), []);
});

test('源码树遍历跳过 node_modules、dist 与 generated', async () => {
  const root = mkdtempSync(join(tmpdir(), 'edge-listfiles-'));
  try {
    mkdirSync(join(root, 'src', 'generated'), { recursive: true });
    mkdirSync(join(root, 'dist'), { recursive: true });
    mkdirSync(join(root, 'node_modules'), { recursive: true });
    writeFileSync(join(root, 'src', 'page.ts'), 'export {};\n');
    writeFileSync(join(root, 'src', 'generated', 'registry.json'), '{}\n');
    writeFileSync(join(root, 'dist', 'app.js'), 'console.log(1);\n');
    writeFileSync(join(root, 'node_modules', 'dep.ts'), 'export {};\n');

    const files = await listFiles(root, (file) => /\.(ts|js|json)$/.test(file));
    assert.deepEqual(files, [join(root, 'src', 'page.ts')]);
  } finally {
    rmSync(root, { recursive: true, force: true });
  }
});
