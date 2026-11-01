/**
 * 权限计算（纯函数，可在 node 中单测）。
 *
 * 三条硬规则（v1.6 §5.2）：
 *   1. 动作之间**不隐含继承**：write 不含 read，execute 不含 read，admin 不含全部。
 *      需要的搭配在权限目录的 `dependencies` 里显式声明，这里只做闭包展开。
 *   2. 通配符只存在于构建输入；运行时拿到的都是具体 ID。`matchesWildcard` 仅用于
 *      构建期与测试，不参与运行期判定。
 *   3. 授权是并集：多角色 + 直接授权的并集；缺少任一必需条件即拒绝（无隐式 deny 优先级）。
 */
import type { PageDef, PermissionId, PermissionDef, RoleDef } from './types';

/** 构建期用：把 `iems.strategy:*` / `core.*:read` 展开为目录中的具体 ID。 */
export function expandWildcard(pattern: string, catalog: PermissionId[]): PermissionId[] {
  if (!pattern.includes('*')) return catalog.includes(pattern) ? [pattern] : [];
  const [domain, rest] = pattern.split('.');
  if (!rest) return [];
  const [resource, action] = rest.split(':');
  return catalog.filter((id) => {
    const [pDomain, pRest] = id.split('.');
    if (!pRest) return false;
    const [pResource, pAction] = pRest.split(':');
    const okDomain = domain === '*' || domain === pDomain;
    const okResource = resource === '*' || resource === pResource;
    const okAction = action === '*' || action === pAction;
    return okDomain && okResource && okAction;
  });
}

/** 依赖闭包：显式声明的依赖一并计入，不改变"不隐含继承"的语义。 */
export function withDependencies(ids: Iterable<PermissionId>, catalog: Pick<PermissionDef, 'id' | 'dependencies'>[]): PermissionId[] {
  const byId = new Map(catalog.map((p) => [p.id, p]));
  const out = new Set<PermissionId>();
  const visit = (id: PermissionId) => {
    if (out.has(id)) return;
    out.add(id);
    for (const dep of byId.get(id)?.dependencies ?? []) visit(dep);
  };
  for (const id of ids) visit(id);
  return [...out].sort();
}

/** 多角色 + 直接授权 → 主体权限集合（并集）。 */
export function composePermissions(
  roles: Pick<RoleDef, 'permissions'>[],
  directGrants: PermissionId[] = [],
  catalog: Pick<PermissionDef, 'id' | 'dependencies'>[] = [],
): Set<PermissionId> {
  const union = new Set<PermissionId>();
  for (const role of roles) for (const id of role.permissions) union.add(id);
  for (const id of directGrants) union.add(id);
  return new Set(withDependencies(union, catalog));
}

export function hasPermission(granted: ReadonlySet<PermissionId>, required: PermissionId): boolean {
  return granted.has(required);
}

/** allOf 全满足；anyOf 至少一条满足（同时存在时两者都要满足）。 */
export function isPermitted(
  block: { allOf?: PermissionId[]; anyOf?: PermissionId[] } | undefined,
  granted: ReadonlySet<PermissionId>,
): boolean {
  if (!block) return false;
  const allOf = block.allOf ?? [];
  const anyOf = block.anyOf;
  if (allOf.length === 0 && (!anyOf || anyOf.length === 0)) return false;
  if (!allOf.every((id) => granted.has(id))) return false;
  if (anyOf && anyOf.length > 0 && !anyOf.some((id) => granted.has(id))) return false;
  return true;
}

/** 缺失的权限清单，用于 403 页面给出可执行信息（而不是只说"无权限"）。 */
export function missingPermissions(page: Pick<PageDef, 'permissions'>, granted: ReadonlySet<PermissionId>): PermissionId[] {
  return (page.permissions.allOf ?? []).filter((id) => !granted.has(id));
}
