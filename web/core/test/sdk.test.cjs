// Core SDK 纯逻辑单测（node:test，无需浏览器）。
// 覆盖 v1.6 §5.2（不隐含继承）、§5.4（可见性算法）、§4.4（Registry 自检）。
const test = require('node:test');
const assert = require('node:assert/strict');

const { expandWildcard, withDependencies, composePermissions, isPermitted, missingPermissions } = require('../test-build/permission.js');
const { explainPage, buildMenu, pageVisible, capabilityValue } = require('../test-build/visibility.js');
const { parseRegistry } = require('../test-build/registry.js');

const CATALOG = [
  { id: 'core.dashboard:read', dependencies: [] },
  { id: 'core.telemetry:read', dependencies: [] },
  { id: 'core.alarm:read', dependencies: [] },
  { id: 'core.alarm:write', dependencies: ['core.alarm:read'] },
  { id: 'iems.strategy:read', dependencies: [] },
  { id: 'iems.strategy:write', dependencies: ['iems.strategy:read'] },
  { id: 'iems.cloud:read', dependencies: [] },
];

const setOf = (...ids) => new Set(ids);

test('通配符只在构建期展开为具体 ID', () => {
  assert.deepEqual(expandWildcard('iems.strategy:*', CATALOG.map((p) => p.id)), ['iems.strategy:read', 'iems.strategy:write']);
  // 展开结果保持目录顺序，便于人工比对
  assert.deepEqual(expandWildcard('core.*:read', CATALOG.map((p) => p.id)), ['core.dashboard:read', 'core.telemetry:read', 'core.alarm:read']);
  assert.deepEqual(expandWildcard('iems.nothing:*', CATALOG.map((p) => p.id)), []);
});

test('write 不隐含 read，但目录显式依赖会展开', () => {
  assert.deepEqual(withDependencies(['iems.strategy:write'], CATALOG), ['iems.strategy:read', 'iems.strategy:write']);
  assert.deepEqual(withDependencies(['iems.cloud:read'], CATALOG), ['iems.cloud:read']);
  // 没有声明依赖的权限不会被"顺带"授予
  assert.ok(!withDependencies(['iems.cloud:read'], CATALOG).includes('iems.cloud:write'));
});

test('多角色授权按并集合成', () => {
  const granted = composePermissions(
    [{ permissions: ['core.dashboard:read'] }, { permissions: ['iems.strategy:write'] }],
    ['core.telemetry:read'],
    CATALOG,
  );
  assert.ok(granted.has('core.dashboard:read'));
  assert.ok(granted.has('core.telemetry:read'));
  assert.ok(granted.has('iems.strategy:read'), '依赖闭包应包含 read');
});

test('allOf 全满足 / anyOf 至少一条 / 空声明视为拒绝', () => {
  const granted = setOf('core.dashboard:read', 'core.telemetry:read');
  assert.equal(isPermitted({ allOf: ['core.dashboard:read'] }, granted), true);
  assert.equal(isPermitted({ allOf: ['core.dashboard:read', 'core.alarm:read'] }, granted), false);
  assert.equal(isPermitted({ allOf: [], anyOf: ['core.alarm:read', 'core.telemetry:read'] }, granted), true);
  assert.equal(isPermitted({ allOf: [], anyOf: ['core.alarm:read'] }, granted), false);
  assert.equal(isPermitted(undefined, granted), false, '未声明权限必须拒绝');
  assert.deepEqual(missingPermissions({ permissions: { allOf: ['core.dashboard:read', 'core.alarm:read'] } }, granted), ['core.alarm:read']);
});

const PAGE = {
  id: 'iems.strategy',
  path: '/strategy',
  component: 'pages/Strategy.vue',
  source: 'pack',
  title: '策略',
  menu: { group: 'control', order: 10, icon: 'bolt' },
  permissions: { allOf: ['iems.strategy:read'] },
  requires: { capabilities: ['software.collector.modbus'], entitlements: ['iems.peak-shaving'] },
  feature: null,
};

const CAPS = {
  productId: 'iems',
  model: 'X',
  firmware: '1.0.0',
  capabilityRevision: 1,
  schemaVersion: 1,
  hardware: { 'network.cellular': false, 'io.rs485.ports': 2 },
  software: { 'collector.modbus': true },
  entitlements: { 'iems.peak-shaving': { state: 'active', expiresAt: null } },
  runtimeHealth: {},
};

test('页面可见性区分 403 / 不支持 / 未授权', () => {
  const base = { permissions: setOf('iems.strategy:read'), capabilities: CAPS };
  assert.equal(explainPage(PAGE, base), 'visible');

  assert.equal(explainPage(PAGE, { ...base, permissions: setOf() }), 'forbidden');
  assert.equal(
    explainPage(PAGE, { ...base, capabilities: { ...CAPS, software: { 'collector.modbus': false } } }),
    'unsupported',
  );
  assert.equal(
    explainPage(PAGE, { ...base, capabilities: { ...CAPS, entitlements: { 'iems.peak-shaving': { state: 'absent', expiresAt: null } } } }),
    'unlicensed',
  );
  assert.equal(
    explainPage(PAGE, { ...base, capabilities: { ...CAPS, entitlements: { 'iems.peak-shaving': { state: 'active', expiresAt: '2020-01-01T00:00:00Z' } } } }),
    'unlicensed',
    '过期许可不能算有效',
  );
});

test('能力未知时按不支持处理（fail-closed）', () => {
  assert.equal(pageVisible(PAGE, { permissions: setOf('iems.strategy:read'), capabilities: null, capabilityUnknown: true }), false);
  assert.equal(capabilityValue(CAPS, 'hardware.network.cellular'), false);
  assert.equal(capabilityValue(CAPS, 'hardware.io.rs485.ports'), 2);
  assert.equal(capabilityValue(CAPS, 'hardware.absent.key'), undefined);
});

test('分组内一条都不可见时整组隐藏', () => {
  const registry = {
    groups: [
      { id: 'control', title: '控制与策略' },
      { id: 'system', title: '系统' },
    ],
    pages: [PAGE],
  };
  const menu = buildMenu(registry, { permissions: setOf('iems.strategy:read'), capabilities: CAPS });
  assert.equal(menu.length, 1);
  assert.equal(menu[0].id, 'control');
  assert.equal(menu[0].items.length, 1);

  const empty = buildMenu(registry, { permissions: setOf(), capabilities: CAPS });
  assert.deepEqual(empty, [], '无权限时不得留下空分组');
});

test('Registry 自检拒绝产品不匹配与缺失摘要', () => {
  const base = {
    schemaVersion: 1,
    productId: 'iems',
    pages: [],
    api: [],
    roles: [],
    coreSdk: '1.6.0',
    digests: { registry: 'sha256:abc' },
  };
  assert.equal(parseRegistry(base, 'iems').productId, 'iems');
  assert.throws(() => parseRegistry(base, 'dcu'), /产品不匹配/);
  assert.throws(() => parseRegistry({ ...base, digests: {} }, 'iems'), /缺少摘要/);
  assert.throws(() => parseRegistry({ ...base, coreSdk: '2.0.0' }, 'iems'), /主版本不兼容/);
});
