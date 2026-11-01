// 静态 Registry 生成（v1.6 §4.4 / §7.4 / WBUI-013）
//
// 产物是**确定性**的：同样的输入必然得到同样的字节（数组按 id 排序、JSON 键排序），
// 因此 `digests.registry` 可以随固件一起校验。任何"运行期动态注册"都不在这条路径上。
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import {
  Problems,
  canonicalJson,
  digestOf,
  rel,
  sha256,
  webPath,
} from './util.mjs';
import {
  expandPermissions,
  loadCatalog,
  loadCoreManifest,
  loadProduct,
  loadRolePresets,
  withDependencies,
} from './manifest.mjs';
import { buildOperations, checkReferences, loadCoreOpenApi, loadProductOpenApi } from './openapi.mjs';

function satisfiesCoreSdk(range, version) {
  const wanted = String(range).replace(/^[\^~]/, '');
  const [wMajor, wMinor] = wanted.split('.').map(Number);
  const [vMajor, vMinor] = String(version).split('.').map(Number);
  if (String(range).startsWith('^')) return vMajor === wMajor && vMinor >= wMinor;
  if (String(range).startsWith('~')) return vMajor === wMajor && vMinor === wMinor;
  return vMajor === wMajor && vMinor === wMinor;
}

export function buildRegistry(productId, { write = true } = {}) {
  const problems = new Problems(`registry(${productId})`);
  const core = loadCoreManifest();
  const catalog = loadCatalog();
  const coreRoles = loadRolePresets(catalog);
  const product = loadProduct(productId);

  if (!satisfiesCoreSdk(product.product.coreSdk, core.coreSdk)) {
    problems.add(`coreSdk 范围不满足：Pack 要求 ${product.product.coreSdk}，Core 为 ${core.coreSdk}`);
  }

  // Pack 声明的权限必须已进入目录（目录是唯一事实来源，Pack 不能私造）。
  for (const perm of product.product.permissions ?? []) {
    if (!catalog.byId.has(perm.id)) {
      problems.add(`Pack 声明了目录中不存在的权限: ${perm.id}`, 'product.yaml/permissions');
    }
  }

  // ---- 角色：Core 预置 + Pack 追加 ----
  const rolesById = new Map(coreRoles.map((role) => [role.id, { ...role, permissions: [...role.permissions] }]));
  for (const [roleId, extra] of Object.entries(product.product.rolePresets ?? {})) {
    const base = rolesById.get(roleId);
    if (!base) {
      problems.add(`Pack 追加了未定义的预置角色: ${roleId}`, 'product.yaml/rolePresets');
      continue;
    }
    base.permissions = [
      ...new Set([...base.permissions, ...expandPermissions(extra, catalog, problems, `rolePresets/${roleId}`)]),
    ].sort();
  }
  const roles = [...rolesById.values()]
    .map((role) => ({
      id: role.id,
      name: role.name,
      builtin: role.builtin !== false,
      description: role.description ?? '',
      defaultScope: role.defaultScope ?? { type: 'gateway', id: 'self' },
      permissions: withDependencies(role.permissions, catalog),
    }))
    .sort((a, b) => a.id.localeCompare(b.id));

  // ---- 页面：构建清单 ∩ release-scope ----
  const enabledModules = new Set(
    Object.entries(product.product.modules ?? {})
      .filter(([, cfg]) => cfg?.enabled !== false)
      .map(([id]) => id),
  );
  const corePages = core.pages
    .filter((page) => enabledModules.has(page.module))
    .filter((page) => !page.feature || product.enabledFeatures.includes(page.feature));
  const packPages = product.pages.filter((page) => !page.feature || product.enabledFeatures.includes(page.feature));
  const pages = [...corePages, ...packPages]
    .map((page) => ({
      id: page.id,
      path: page.path,
      component: page.component,
      source: page.source,
      title: page.title,
      menu: {
        group: page.menu.group,
        order: page.menu.order,
        icon: page.menu.icon ?? 'dot',
      },
      permissions: {
        allOf: [...(page.permissions?.allOf ?? [])].sort(),
        ...(page.permissions?.anyOf ? { anyOf: [...page.permissions.anyOf].sort() } : {}),
      },
      requires: {
        capabilities: [...(page.requires?.capabilities ?? [])].sort(),
        entitlements: [...(page.requires?.entitlements ?? [])].sort(),
      },
      feature: page.feature ?? null,
    }))
    .sort((a, b) => (a.menu.group + a.menu.order).localeCompare(b.menu.group + b.menu.order) || a.id.localeCompare(b.id));

  const pageIds = new Set();
  const pagePaths = new Set();
  for (const page of pages) {
    if (pageIds.has(page.id)) problems.add(`页面 id 冲突（Core 与 Pack 不得同名）: ${page.id}`);
    if (pagePaths.has(page.path)) problems.add(`页面路径冲突: ${page.path}`);
    pageIds.add(page.id);
    pagePaths.add(page.path);
  }

  const widgets = (product.product.widgets ?? [])
    .map((widget) => ({
      id: widget.id,
      component: widget.component,
      size: widget.size ?? { w: 4, h: 3 },
      permissions: { allOf: [...(widget.permissions?.allOf ?? [])].sort() },
      requires: { capabilities: [...(widget.requires?.capabilities ?? [])].sort() },
      feature: widget.feature ?? null,
    }))
    .sort((a, b) => a.id.localeCompare(b.id));

  // ---- API：契约 → 静态路由/策略表 ----
  const coreDoc = loadCoreOpenApi();
  const productDoc = loadProductOpenApi(product);
  const { operations, dropped } = buildOperations({ coreDoc, productDoc, catalog, product, problems });

  checkReferences({ pages, catalog, problems });

  // 未使用的权限：只告警不失败（目录可能先于实现落地），但要能在报告里看到。
  const used = new Set();
  for (const page of pages) {
    for (const id of page.permissions.allOf) used.add(id);
    for (const id of page.permissions.anyOf ?? []) used.add(id);
  }
  for (const op of operations) for (const id of op.permissions) used.add(id);
  for (const role of roles) for (const id of role.permissions) used.add(id);
  // 只统计本产品相关的域：目录是全局的，把别的产品的权限算成"未使用"是噪音。
  const relevantDomains = new Set(['core', 'network', 'system', 'user', 'debug', product.id]);
  const unusedPermissions = catalog.list
    .map((p) => p.id)
    .filter((id) => relevantDomains.has(id.split('.')[0]))
    .filter((id) => !used.has(id))
    .sort();

  problems.throwIfAny();

  const releaseScope = {
    releaseId: String(product.scope.releaseId),
    enabled: product.enabledFeatures,
    disabled: product.disabledFeatures,
    approvals: product.scope.approvals,
  };

  const body = {
    schemaVersion: 1,
    productId: product.id,
    productName: product.product.name,
    productVersion: String(product.product.version),
    coreSdk: core.coreSdk,
    theme: {
      brandColor: product.product.theme?.brandColor ?? '#0B6BCB',
      logo: product.product.theme?.logo ?? null,
    },
    groups: core.groups.map((g) => ({ id: g.id, title: g.title })).sort((a, b) => a.id.localeCompare(b.id)),
    pages,
    widgets,
    permissions: catalog.list
      .map((p) => ({ id: p.id, description: p.description, sensitivity: p.sensitivity ?? 'low', dependencies: [...(p.dependencies ?? [])].sort() }))
      .sort((a, b) => a.id.localeCompare(b.id)),
    roles,
    capabilities: {
      declared: [...(product.product.capabilities ?? [])].sort(),
      entitlements: Object.keys(product.product.entitlements ?? {}).sort(),
    },
    api: operations,
    releaseScope,
    digests: {
      releaseScope: digestOf(releaseScope),
      openapi: digestOf({ core: coreDoc, pack: productDoc ?? null }),
    },
    warnings: { unusedPermissions, droppedOperations: dropped },
  };
  body.digests.registry = digestOf(body);

  const outFile = path.join(product.dir, 'src/generated/registry.json');
  if (write) {
    mkdirSync(path.dirname(outFile), { recursive: true });
    writeFileSync(outFile, JSON.stringify(body, null, 2) + '\n', 'utf8');
    writeFileSync(
      path.join(product.dir, 'src/generated/build-report.json'),
      JSON.stringify(
        {
          productId: body.productId,
          version: body.productVersion,
          pages: pages.length,
          operations: operations.length,
          droppedOperations: dropped,
          unusedPermissions,
          digests: body.digests,
          inputs: {
            manifest: product.manifestFile,
            releaseScope: product.scopeFile,
            manifestDigest: `sha256:${sha256(canonicalJson(product.product))}`,
            scopeDigest: `sha256:${sha256(canonicalJson(product.scope))}`,
          },
        },
        null,
        2,
      ) + '\n',
      'utf8',
    );
  }

  return { registry: body, outFile: rel(outFile), problems };
}
