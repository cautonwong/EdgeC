// 构建输入校验：core/modules.manifest.yaml、products/<id>/product.yaml、release-scope.yaml
//
// 设计依据 v1.6 §4.2 / §4.4 / §14.1。这里的每一条检查都对应一条"构建期必须失败"
// 的规则——校验器不是文档的装饰，它决定产物能不能生成。
import { existsSync, readdirSync } from 'node:fs';
import path from 'node:path';
import { ID_RE, PERMISSION_RE, PERMISSION_WILDCARD_RE, Problems, readYaml, rel, webPath } from './util.mjs';

export const MENU_GROUPS = ['overview', 'data', 'control', 'network', 'system', 'diagnostics'];
const SEMVER_RE = /^\d+\.\d+\.\d+$/;
const RANGE_RE = /^[\^~]?\d+\.\d+\.\d+$/;
const HEX_RE = /^#[0-9a-fA-F]{6}$/;
const COMPONENT_RE = /^[A-Za-z0-9_./-]+\.vue$/;

export function loadCoreManifest() {
  const file = webPath('core/modules.manifest.yaml');
  const doc = readYaml(file);
  const problems = new Problems('core/modules.manifest.yaml');

  if (doc?.schemaVersion !== 1) problems.add('schemaVersion 必须为 1');
  if (!doc?.coreSdk || !SEMVER_RE.test(String(doc.coreSdk))) problems.add('coreSdk 必须是 x.y.z');

  const groups = new Map();
  for (const group of doc?.groups ?? []) {
    if (!MENU_GROUPS.includes(group.id)) problems.add(`未登记的一级分组 ${group.id}`, 'groups');
    if (!group.title) problems.add(`${group.id} 缺少 title`, 'groups');
    groups.set(group.id, group);
  }
  for (const id of MENU_GROUPS) {
    if (!groups.has(id)) problems.add(`缺少固定分组 ${id}`, 'groups');
  }

  const modules = new Map();
  const pages = [];
  const seenPageIds = new Set();
  for (const mod of doc?.modules ?? []) {
    if (!/^core:[a-z0-9-]+$/.test(mod.id ?? '')) problems.add(`模块 id 必须是 core:<name>，得到 ${mod.id}`, 'modules');
    if (modules.has(mod.id)) problems.add(`模块 id 重复 ${mod.id}`, 'modules');
    modules.set(mod.id, mod);

    for (const page of mod.pages ?? []) {
      const where = `modules/${mod.id}/pages/${page.id}`;
      if (!ID_RE.test(page.id ?? '')) problems.add(`页面 id 不符合 domain.resource 形式: ${page.id}`, where);
      if (seenPageIds.has(page.id)) problems.add(`页面 id 重复: ${page.id}`, where);
      seenPageIds.add(page.id);
      if (!String(page.path ?? '').startsWith('/')) problems.add('path 必须以 / 开头', where);
      if (!COMPONENT_RE.test(page.component ?? '')) problems.add(`component 必须是 core/src/modules 下的 .vue 相对路径: ${page.component}`, where);
      else if (!existsSync(webPath('core/src/modules', page.component))) {
        problems.add(`component 文件不存在: core/src/modules/${page.component}`, where);
      }
      if (!page.menu?.group) problems.add('缺少 menu.group', where);
      else if (!groups.has(page.menu.group)) problems.add(`menu.group 未登记: ${page.menu.group}`, where);
      if (!Number.isFinite(page.menu?.order)) problems.add('menu.order 必须是数字', where);
      validatePermissionBlock(page.permissions, where, problems, { allowWildcard: false });
      pages.push({ ...page, source: 'core', module: mod.id });
    }
  }

  problems.throwIfAny();
  return { doc, groups: [...groups.values()], modules, pages, coreSdk: String(doc.coreSdk) };
}

function validatePermissionBlock(block, where, problems, { allowWildcard }) {
  if (!block) {
    problems.add('缺少 permissions 声明（页面/接口必须绑定精确权限）', where);
    return;
  }
  const lists = [['allOf', block.allOf], ['anyOf', block.anyOf]].filter(([, v]) => v !== undefined);
  if (lists.length === 0) problems.add('permissions 必须声明 allOf 或 anyOf', where);
  for (const [key, list] of lists) {
    if (!Array.isArray(list)) {
      problems.add(`permissions.${key} 必须是数组`, where);
      continue;
    }
    for (const id of list) {
      const re = allowWildcard ? PERMISSION_WILDCARD_RE : PERMISSION_RE;
      if (!re.test(String(id))) problems.add(`权限 ID 不合法: ${id}`, `${where}.${key}`);
    }
  }
}

export function loadProduct(productId) {
  const dir = webPath('products', productId);
  const manifestFile = path.join(dir, 'product.yaml');
  const scopeFile = path.join(dir, 'release-scope.yaml');
  const problems = new Problems(`products/${productId}`);

  const product = readYaml(manifestFile);
  const scope = readYaml(scopeFile);

  // ---- product.yaml ----
  if (product?.schemaVersion !== 1) problems.add('schemaVersion 必须为 1', 'product.yaml');
  if (product?.id !== productId) problems.add(`id 必须等于目录名 ${productId}`, 'product.yaml');
  if (!SEMVER_RE.test(String(product?.version))) problems.add('version 必须是 x.y.z', 'product.yaml');
  if (!RANGE_RE.test(String(product?.coreSdk))) problems.add('coreSdk 必须是 semver 范围（^x.y.z / ~x.y.z）', 'product.yaml');
  if (product?.theme?.brandColor && !HEX_RE.test(product.theme.brandColor)) {
    problems.add('theme.brandColor 必须是 #RRGGBB', 'product.yaml');
  }

  const declaredFeatures = Object.keys(product?.features ?? {});
  for (const modId of Object.keys(product?.modules ?? {})) {
    if (!/^core:[a-z0-9-]+$/.test(modId)) problems.add(`modules 键必须是 core:<name>: ${modId}`, 'product.yaml');
  }

  const pages = [];
  const seenIds = new Set();
  const seenPaths = new Set();
  for (const page of product?.pages ?? []) {
    const where = `pages/${page.id}`;
    if (!ID_RE.test(page.id ?? '')) problems.add(`页面 id 不合法: ${page.id}`, where);
    if (seenIds.has(page.id)) problems.add(`页面 id 重复: ${page.id}`, where);
    seenIds.add(page.id);
    if (!String(page.path ?? '').startsWith('/')) problems.add('path 必须以 / 开头', where);
    if (seenPaths.has(page.path)) problems.add(`path 重复: ${page.path}`, where);
    seenPaths.add(page.path);
    if (!MENU_GROUPS.includes(page.menu?.group)) problems.add(`menu.group 必须是固定分组之一: ${page.menu?.group}`, where);
    if (!Number.isFinite(page.menu?.order)) problems.add('menu.order 必须是数字', where);
    if (!COMPONENT_RE.test(page.component ?? '')) problems.add(`component 必须是 Pack 内 .vue 相对路径: ${page.component}`, where);
    else if (!existsSync(path.join(dir, page.component))) problems.add(`component 文件不存在: products/${productId}/${page.component}`, where);
    validatePermissionBlock(page.permissions, where, problems, { allowWildcard: false });
    if (page.feature && !declaredFeatures.includes(page.feature)) {
      problems.add(`引用了 product.yaml 未声明的功能开关: ${page.feature}`, where);
    }
    pages.push({ ...page, source: 'pack', product: productId });
  }

  for (const widget of product?.widgets ?? []) {
    if (!ID_RE.test(widget.id ?? '')) problems.add(`widget id 不合法: ${widget.id}`, 'widgets');
    validatePermissionBlock(widget.permissions, `widgets/${widget.id}`, problems, { allowWildcard: false });
  }

  for (const perm of product?.permissions ?? []) {
    if (!PERMISSION_RE.test(String(perm.id))) problems.add(`权限 ID 不合法: ${perm.id}`, 'permissions');
  }

  const extension = product?.openapi;
  if (extension && !existsSync(path.join(dir, extension))) {
    problems.add(`openapi 扩展文件不存在: ${extension}`, 'product.yaml');
  }

  // ---- release-scope.yaml ----
  if (scope?.schemaVersion !== 1) problems.add('schemaVersion 必须为 1', 'release-scope.yaml');
  if (scope?.productId !== productId) problems.add(`productId 必须等于 ${productId}`, 'release-scope.yaml');
  if (String(scope?.releaseId) !== String(product?.version)) {
    problems.add(`releaseId(${scope?.releaseId}) 必须等于 product.yaml 的 version(${product?.version})`, 'release-scope.yaml');
  }
  for (const [name, feature] of Object.entries(scope?.features ?? {})) {
    if (typeof feature?.enabled !== 'boolean') problems.add(`features.${name}.enabled 必须是布尔`, 'release-scope.yaml');
    if (!declaredFeatures.includes(name)) problems.add(`features.${name} 未在 product.yaml 声明`, 'release-scope.yaml');
    if (feature.enabled && (!feature.tasks?.length || !feature.requirements?.length)) {
      problems.add(`启用功能 ${name} 必须关联 tasks 与 requirements（未验证不得标记为已启用）`, 'release-scope.yaml');
    }
  }
  for (const name of declaredFeatures) {
    if (!(name in (scope?.features ?? {}))) {
      problems.add(`product.yaml 声明的功能 ${name} 未在 release-scope.yaml 给出启用结论`, 'release-scope.yaml');
    }
  }
  for (const key of ['productOwner', 'platformOwner', 'securityOwner']) {
    if (!scope?.approvals?.[key]) problems.add(`approvals.${key} 不能为空`, 'release-scope.yaml');
  }

  problems.throwIfAny();

  return {
    id: productId,
    dir,
    product,
    scope,
    pages,
    enabledFeatures: Object.entries(scope.features ?? {})
      .filter(([, f]) => f.enabled)
      .map(([name]) => name)
      .sort(),
    disabledFeatures: Object.entries(scope.features ?? {})
      .filter(([, f]) => !f.enabled)
      .map(([name]) => name)
      .sort(),
    manifestFile: rel(manifestFile),
    scopeFile: rel(scopeFile),
  };
}

export function loadCatalog() {
  const doc = readYaml(webPath('permissions/catalog.yaml'));
  const problems = new Problems('permissions/catalog.yaml');
  if (doc?.schemaVersion !== 1) problems.add('schemaVersion 必须为 1');
  const byId = new Map();
  for (const perm of doc?.permissions ?? []) {
    if (!PERMISSION_RE.test(String(perm.id))) problems.add(`权限 ID 不合法: ${perm.id}`);
    if (byId.has(perm.id)) problems.add(`权限 ID 重复: ${perm.id}`);
    if (!perm.description) problems.add(`${perm.id} 缺少 description`);
    for (const dep of perm.dependencies ?? []) {
      if (!PERMISSION_RE.test(String(dep))) problems.add(`${perm.id} 的依赖不是精确权限 ID: ${dep}`);
    }
    byId.set(perm.id, perm);
  }
  for (const perm of byId.values()) {
    for (const dep of perm.dependencies ?? []) {
      if (!byId.has(dep)) problems.add(`${perm.id} 依赖了未声明的权限: ${dep}`);
    }
  }
  problems.throwIfAny();
  return { list: [...byId.values()], byId };
}

export function loadRolePresets(catalog) {
  const doc = readYaml(webPath('permissions/role-presets.yaml'));
  const problems = new Problems('permissions/role-presets.yaml');
  if (doc?.schemaVersion !== 1) problems.add('schemaVersion 必须为 1');
  const roles = [];
  for (const role of doc?.roles ?? []) {
    if (!/^[a-z][a-z0-9-]*$/.test(role.id ?? '')) problems.add(`角色 id 不合法: ${role.id}`);
    if (!Array.isArray(role.permissions) || role.permissions.length === 0) {
      problems.add(`角色 ${role.id} 必须声明权限集合`);
    }
    roles.push({ ...role, permissions: expandPermissions(role.permissions ?? [], catalog, problems, `roles/${role.id}`) });
  }
  problems.throwIfAny();
  return roles;
}

/** 通配符只在构建输入展开；展开结果写进 Registry，运行时只见具体 ID。 */
export function expandPermissions(list, catalog, problems, where) {
  const out = new Set();
  for (const raw of list) {
    const id = String(raw);
    if (!id.includes('*')) {
      if (!catalog.byId.has(id)) problems?.add(`引用未声明的权限: ${id}`, where);
      out.add(id);
      continue;
    }
    if (!PERMISSION_WILDCARD_RE.test(id)) {
      problems?.add(`通配符写法不合法: ${id}`, where);
      continue;
    }
    const [domain, rest] = id.split('.');
    const [resourcePattern, actionPattern] = rest.split(':');
    let matched = 0;
    for (const perm of catalog.list) {
      const [pDomain, pRest] = perm.id.split('.');
      const [pResource, pAction] = pRest.split(':');
      const domainOk = domain === '*' || domain === pDomain;
      const resourceOk = resourcePattern === '*' || resourcePattern === pResource;
      const actionOk = actionPattern === '*' || actionPattern === pAction;
      if (domainOk && resourceOk && actionOk) {
        out.add(perm.id);
        matched += 1;
      }
    }
    if (matched === 0) problems?.add(`通配符没有匹配到任何权限: ${id}`, where);
  }
  return [...out].sort();
}

/** 权限闭包：显式声明的依赖一并授予（write 不含 read，但可以显式要求 read）。 */
export function withDependencies(ids, catalog) {
  const out = new Set();
  const visit = (id) => {
    if (out.has(id)) return;
    out.add(id);
    for (const dep of catalog.byId.get(id)?.dependencies ?? []) visit(dep);
  };
  for (const id of ids) visit(id);
  return [...out].sort();
}

export function listProducts() {
  const dir = webPath('products');
  return readdirSync(dir, { withFileTypes: true })
    .filter((e) => e.isDirectory() && existsSync(path.join(dir, e.name, 'product.yaml')))
    .map((e) => e.name)
    .sort();
}
