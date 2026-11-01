// OpenAPI 契约解析与校验（v1.6 §7.1 / §7.4 / 附录 F G0）
//
// 只做本项目需要的那部分：路径前缀、认证声明、operation 级权限/范围/能力/许可/
// 审计/幂等元数据。契约是唯一事实来源，Mock 与 webd 都从这里读「路径 → 权限」，
// 所以这里必须严格——松一处的后果是运行时少一道授权。
import path from 'node:path';
import { PERMISSION_RE, Problems, readYaml, webPath } from './util.mjs';

const METHODS = ['get', 'post', 'put', 'patch', 'delete'];
const API_PREFIX = '/api/v1';

export function loadCoreOpenApi() {
  return readYaml(webPath('openapi/core.yaml'));
}

export function loadProductOpenApi(product) {
  const extension = product.product.openapi;
  if (!extension) return null;
  return readYaml(path.join(product.dir, extension));
}

/** 把 paths 展开成扁平 operation 列表。 */
function flatten(doc) {
  const ops = [];
  for (const [routePath, item] of Object.entries(doc?.paths ?? {})) {
    const shared = item?.parameters ?? [];
    for (const method of METHODS) {
      const op = item?.[method];
      if (!op) continue;
      ops.push({ method: method.toUpperCase(), path: routePath, op, shared });
    }
  }
  return ops;
}

export function buildOperations({ coreDoc, productDoc, catalog, product, problems }) {
  const operations = [];
  const dropped = [];
  const anonymousAllowlist = new Set(coreDoc?.['x-anonymous-allowlist'] ?? []);
  const declaredFeatures = new Set(Object.keys(product.product.features ?? {}));
  const enabledFeatures = new Set(product.enabledFeatures);
  const declaredCapabilities = new Set(product.product.capabilities ?? []);
  const declaredEntitlements = new Set(
    Object.keys(product.product.entitlements ?? {}),
  );

  for (const source of [
    { doc: coreDoc, origin: 'core' },
    { doc: productDoc, origin: 'pack' },
  ]) {
    if (!source.doc) continue;
    for (const { method, path: routePath, op } of flatten(source.doc)) {
      const where = `${method} ${routePath}`;

      if (!routePath.startsWith(API_PREFIX + '/')) {
        problems.add(`路径必须以 ${API_PREFIX}/ 开头`, where);
      }
      if (source.origin === 'pack' && !routePath.startsWith(`${API_PREFIX}/${product.id}/`)) {
        problems.add(`Pack 的 API 路径必须以 ${API_PREFIX}/${product.id}/ 为前缀`, where);
      }
      if (!Array.isArray(op.security)) {
        problems.add('缺少 security 声明（匿名必须显式写 security: []）', where);
      }

      const anonymous = Array.isArray(op.security) && op.security.length === 0;
      const allowlisted = anonymousAllowlist.has(where);
      if (anonymous && !allowlisted) {
        problems.add('匿名接口必须登记进 x-anonymous-allowlist', where);
      }
      if (!anonymous && allowlisted) {
        problems.add('已登记匿名 allowlist，但 operation 声明了 security', where);
      }

      const permissions = op['x-required-permissions'] ?? [];
      const permissionSource = op['x-permission-source'];
      if (!Array.isArray(permissions)) {
        problems.add('x-required-permissions 必须是数组', where);
      } else {
        for (const id of permissions) {
          if (!PERMISSION_RE.test(String(id))) {
            problems.add(`x-required-permissions 必须是精确权限 ID（禁止通配符）: ${id}`, where);
          } else if (!catalog.byId.has(id)) {
            problems.add(`引用了权限目录中不存在的权限: ${id}`, where);
          }
        }
      }
      // 只需登录、不需要具体权限的自助接口（登出 / me / 改密 / WS 握手）必须显式声明
      // x-auth-level: session —— 否则"忘了绑权限"和"故意不绑权限"看起来一模一样。
      const authLevel = op['x-auth-level'] ?? 'permission';
      if (authLevel === 'session' && anonymous) {
        problems.add('x-auth-level: session 只能用于已认证接口', where);
      }
      if (!anonymous && permissions.length === 0 && !permissionSource && authLevel !== 'session') {
        problems.add('非匿名接口必须绑定权限，或声明 x-permission-source / x-auth-level: session', where);
      }
      if (permissionSource && !['config-section', 'task-cancel-v1'].includes(permissionSource)) {
        problems.add(`未实现的权限来源策略: ${permissionSource}`, where);
      }
      if (!op['x-resource-scope']) {
        problems.add('缺少 x-resource-scope', where);
      }

      for (const cap of op['x-required-capabilities'] ?? []) {
        if (!declaredCapabilities.has(cap)) {
          problems.add(`引用了产品未声明的能力: ${cap}`, where);
        }
      }
      for (const ent of op['x-required-entitlements'] ?? []) {
        if (!declaredEntitlements.has(ent)) {
          problems.add(`引用了产品未声明的许可: ${ent}`, where);
        }
      }

      const feature = op['x-feature'];
      if (feature) {
        if (!declaredFeatures.has(feature)) {
          problems.add(`引用了 product.yaml 未声明的功能开关: ${feature}`, where);
        } else if (!enabledFeatures.has(feature)) {
          // 发布范围关闭 → 该 operation 不得出现在最终产物里。
          dropped.push({ operationId: op.operationId, method, path: routePath, feature });
          continue;
        }
      }

      operations.push({
        operationId: op.operationId ?? `${method.toLowerCase()}${routePath.replace(/[^a-zA-Z0-9]/g, '_')}`,
        method,
        path: routePath,
        origin: source.origin,
        anonymous,
        permissions: [...permissions].sort(),
        permissionSource: permissionSource ?? null,
        resourceScope: op['x-resource-scope'],
        capabilities: [...(op['x-required-capabilities'] ?? [])].sort(),
        entitlements: [...(op['x-required-entitlements'] ?? [])].sort(),
        auditLevel: op['x-audit-level'] ?? 'none',
        idempotent: op['x-idempotent'] === true,
        authLevel,
        authorizationPolicy: op['x-authorization-policy'] ?? null,
        riskClass: op['x-risk-class'] ?? null,
        deprecated: op['x-deprecated'] === true,
        feature: feature ?? null,
        summary: op.summary ?? '',
      });
    }
  }

  // 同一 operationId 出现两次 → 生成的路由/策略表会互相覆盖。
  const seen = new Set();
  for (const op of operations) {
    if (seen.has(op.operationId)) problems.add(`operationId 重复: ${op.operationId}`);
    seen.add(op.operationId);
  }
  // 同一 method+path 也不允许重复（Pack 覆盖 Core 必须走白名单，不能自然覆盖）。
  const routes = new Set();
  for (const op of operations) {
    const key = `${op.method} ${op.path}`;
    if (routes.has(key)) problems.add(`路由重复（禁止后注册者覆盖）: ${key}`);
    routes.add(key);
  }

  return { operations: operations.sort((a, b) => (a.path + a.method).localeCompare(b.path + b.method)), dropped };
}

/** 页面/Widget/角色引用必须能解析；接口未绑定的权限不能悬空。 */
export function checkReferences({ pages, catalog, problems }) {
  for (const page of pages) {
    const ids = [...(page.permissions?.allOf ?? []), ...(page.permissions?.anyOf ?? [])];
    if (ids.length === 0) problems.add('页面未声明权限', `pages/${page.id}`);
    for (const id of ids) {
      if (!catalog.byId.has(id)) problems.add(`页面引用了未声明的权限: ${id}`, `pages/${page.id}`);
    }
  }
}
