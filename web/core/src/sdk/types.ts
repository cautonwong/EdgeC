/**
 * Core SDK 类型：构建期产物（Registry）与运行期授权上下文的契约。
 *
 * 这一层**不含任何框架依赖**（没有 vue / pinia / axios），因此可以在 node 里直接
 * 单测，也保证 Pack 引用 SDK 时不会把框架版本绑死。依赖方向由
 * `tooling boundaries` 强制。
 */

export type PermissionId = string;

export interface PermissionDef {
  id: PermissionId;
  description: string;
  sensitivity: 'low' | 'medium' | 'high';
  dependencies: PermissionId[];
}

export interface RoleDef {
  id: string;
  name: string;
  builtin: boolean;
  description: string;
  defaultScope: { type: string; id?: string; ids?: string[] };
  /** 构建期已展开为具体权限 ID（通配符不出现在运行时）。 */
  permissions: PermissionId[];
}

export interface PagePermissions {
  allOf: PermissionId[];
  anyOf?: PermissionId[];
}

export interface PageRequires {
  capabilities: string[];
  entitlements: string[];
}

export interface PageDef {
  id: string;
  path: string;
  component: string;
  source: 'core' | 'pack';
  title: string;
  menu: { group: string; order: number; icon: string };
  permissions: PagePermissions;
  requires: PageRequires;
  feature: string | null;
}

export interface WidgetDef {
  id: string;
  component: string;
  size: { w: number; h: number };
  permissions: PagePermissions;
  requires: { capabilities: string[] };
  feature: string | null;
}

export interface OperationDef {
  operationId: string;
  method: string;
  path: string;
  origin: 'core' | 'pack';
  anonymous: boolean;
  permissions: PermissionId[];
  permissionSource: string | null;
  resourceScope: string;
  capabilities: string[];
  entitlements: string[];
  auditLevel: 'none' | 'low' | 'high';
  idempotent: boolean;
  authorizationPolicy: string | null;
  riskClass: string | null;
  deprecated: boolean;
  feature: string | null;
  summary: string;
}

export interface Registry {
  schemaVersion: 1;
  productId: string;
  productName: string;
  productVersion: string;
  coreSdk: string;
  theme: { brandColor: string; logo: string | null };
  groups: { id: string; title: string }[];
  pages: PageDef[];
  widgets: WidgetDef[];
  permissions: PermissionDef[];
  roles: RoleDef[];
  capabilities: { declared: string[]; entitlements: string[] };
  api: OperationDef[];
  releaseScope: {
    releaseId: string;
    enabled: string[];
    disabled: string[];
    approvals: Record<string, string>;
  };
  digests: { registry: string; releaseScope: string; openapi: string };
  warnings: { unusedPermissions: PermissionId[]; droppedOperations: { operationId: string; method: string; path: string; feature: string }[] };
}

/** `GET /api/v1/system/info` 的匿名 allowlist 字段，多一个都不返回。 */
export interface SystemInfo {
  productId: string;
  displayName: string;
  language: string;
  themeVersion: string;
  loginHint: string;
}

/** `GET /api/v1/auth/me` */
export interface AuthMe {
  userId: string;
  username: string;
  displayName: string;
  roles: { roleId: string; scope: { type: string; id?: string; ids?: string[] } }[];
  permissions: PermissionId[];
  scopes: string[];
  mustChangePassword: boolean;
  policyRevision: number;
  csrfToken: string;
  sessionExpiresAt: string;
}

/** `GET /api/v1/system/capabilities` */
export interface CapabilitySnapshot {
  productId: string;
  model: string;
  firmware: string;
  capabilityRevision: number;
  schemaVersion: number;
  hardware: Record<string, boolean | number>;
  software: Record<string, boolean | number>;
  entitlements: Record<string, { state: 'active' | 'expired' | 'absent' | 'invalid'; expiresAt: string | null }>;
  runtimeHealth: Record<string, 'healthy' | 'degraded' | 'down' | 'unknown'>;
}

export interface ApiError {
  code: string;
  message: string;
  details?: Record<string, unknown>;
  requestId: string;
}

export type PageState = 'visible' | 'forbidden' | 'unsupported' | 'unlicensed' | 'not-built';
