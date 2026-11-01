/**
 * 页面可见性与菜单生成（纯函数，v1.6 §5.4 / §6.2）。
 *
 * 可见页面 = 构建产物包含该页面
 *          AND requires.capabilities 全部满足
 *          AND requires.entitlements 全部有效
 *          AND permissions.allOf 全部满足（或 anyOf 满足）
 *
 * 注意：这里算出来的只是**呈现**结果，不构成授权。服务端对每个 API 独立判定
 * （见 mock-server / webd），前端隐藏按钮永远不是安全边界。
 */
import { isPermitted, missingPermissions } from './permission';
import type {
  CapabilitySnapshot,
  PageDef,
  PageState,
  PermissionId,
  Registry,
} from './types';

export interface VisibilityContext {
  permissions: ReadonlySet<PermissionId>;
  capabilities: CapabilitySnapshot | null;
  /** 能力快照不可用时的策略：默认 fail-closed（不能推断"默认支持"）。 */
  capabilityUnknown?: boolean;
}

export type CapabilityValue = boolean | number | undefined;

export function capabilityValue(snapshot: CapabilitySnapshot | null, key: string): CapabilityValue {
  if (!snapshot) return undefined;
  const [scope, ...rest] = key.split('.');
  const path = rest.join('.');
  const bag = (snapshot as unknown as Record<string, Record<string, CapabilityValue>>)[scope];
  if (!bag) return undefined;
  const value = bag[path];
  if (value !== undefined) return value;
  // 支持 `hardware.network.ethernet.ports` 这种点分嵌套写法
  return rest.reduce<CapabilityValue>((acc, part) => {
    if (acc && typeof acc === 'object') return (acc as unknown as Record<string, CapabilityValue>)[part];
    return undefined;
  }, bag as unknown as CapabilityValue);
}

function capabilitySatisfied(ctx: VisibilityContext, key: string): boolean {
  if (ctx.capabilityUnknown || !ctx.capabilities) return false;
  const value = capabilityValue(ctx.capabilities, key);
  if (value === undefined || value === false) return false;
  if (typeof value === 'number') return value > 0;
  return value === true;
}

function entitlementSatisfied(ctx: VisibilityContext, key: string): boolean {
  if (ctx.capabilityUnknown || !ctx.capabilities) return false;
  const entry = ctx.capabilities.entitlements?.[key];
  if (!entry) return false;
  if (entry.state !== 'active') return false;
  if (entry.expiresAt && Date.parse(entry.expiresAt) <= Date.now()) return false;
  return true;
}

/** 给出精确原因，让 UI 能区分"没权限"和"这台设备不支持"。 */
export function explainPage(page: PageDef, ctx: VisibilityContext): PageState {
  if (!isPermitted(page.permissions, ctx.permissions)) return 'forbidden';
  for (const key of page.requires.capabilities) {
    if (!capabilitySatisfied(ctx, key)) return 'unsupported';
  }
  for (const key of page.requires.entitlements) {
    if (!entitlementSatisfied(ctx, key)) return 'unlicensed';
  }
  return 'visible';
}

export function pageVisible(page: PageDef, ctx: VisibilityContext): boolean {
  return explainPage(page, ctx) === 'visible';
}

export interface MenuItem {
  pageId: string;
  path: string;
  title: string;
  icon: string;
  order: number;
}

export interface MenuGroup {
  id: string;
  title: string;
  items: MenuItem[];
}

/** 分组内所有子页面都不可见时，分组整体隐藏。 */
export function buildMenu(registry: Registry, ctx: VisibilityContext): MenuGroup[] {
  const groups = new Map<string, MenuGroup>();
  for (const group of registry.groups) groups.set(group.id, { id: group.id, title: group.title, items: [] });

  for (const page of registry.pages) {
    if (!pageVisible(page, ctx)) continue;
    const group = groups.get(page.menu.group);
    if (!group) continue; // 未登记分组不得出现在菜单（构建期已拦，这里再兜一层）
    group.items.push({
      pageId: page.id,
      path: page.path,
      title: page.title,
      icon: page.menu.icon,
      order: page.menu.order,
    });
  }

  return [...groups.values()]
    .map((group) => ({ ...group, items: group.items.sort((a, b) => a.order - b.order || a.title.localeCompare(b.title)) }))
    .filter((group) => group.items.length > 0);
}

export function visiblePages(registry: Registry, ctx: VisibilityContext): PageDef[] {
  return registry.pages.filter((page) => pageVisible(page, ctx));
}

export function findPageByPath(registry: Registry, path: string): PageDef | undefined {
  const normalized = path.replace(/\/+$/, '') || '/';
  return registry.pages.find((page) => page.path === normalized);
}

/** 403 页面要能说清"缺哪几条权限"。 */
export function pageDenial(page: PageDef, ctx: VisibilityContext): {
  state: PageState;
  missing: PermissionId[];
  missingCapabilities: string[];
} {
  return {
    state: explainPage(page, ctx),
    missing: missingPermissions(page, ctx.permissions),
    missingCapabilities: page.requires.capabilities.filter((key) => !capabilitySatisfied(ctx, key)),
  };
}
