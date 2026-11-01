#!/usr/bin/env node
/**
 * WebUI Mock Server —— 设备侧 webd 的**行为替身**（v1.6 §7.1–§7.5）。
 *
 * 它存在的理由不是"让前端跑起来"，而是让 E2E 能验证**服务端判定**：
 *   - 每个请求的权限/能力/许可/范围判定，来自构建期生成的 registry.json 与 OpenAPI
 *     元数据，而不是 handler 里手写的 if；
 *   - 会话、CSRF、Host/Origin、匿名 allowlist、错误模型按文档基线实现；
 *   - 配置事务状态机、幂等、deadline 与回滚是**真的**状态机，不是假绿。
 *
 * 明确不实现（留给 webd）：IPC、TLS、审计持久化、任务执行器、许可证签名校验。
 * 这些缺失必须在报告里如实体现，不能拿 mock 的绿灯冒充设备验证。
 *
 * 用法：
 *   node mock-server/server.mjs --product iems --port 8081 [--static products/iems/dist]
 */
import { createHash, randomBytes, randomUUID, scryptSync, timingSafeEqual } from 'node:crypto';
import { createReadStream, existsSync, readdirSync, readFileSync, statSync } from 'node:fs';
import http from 'node:http';
import { createHash as createHashForWs } from 'node:crypto';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB_ROOT = path.dirname(HERE);

const args = parseArgs(process.argv.slice(2));
const PRODUCT = args.product ?? 'iems';
const PORT = Number(args.port ?? 8081);
const HOST = args.host ?? '127.0.0.1';
const STATIC_DIR = args.static ? path.resolve(WEB_ROOT, args.static) : null;
const IDLE_TIMEOUT_MS = Number(process.env.EDGEOS_SESSION_IDLE_MS ?? 15 * 60 * 1000);
const ABSOLUTE_TIMEOUT_MS = Number(process.env.EDGEOS_SESSION_ABSOLUTE_MS ?? 8 * 60 * 60 * 1000);
const CONFIRM_WINDOW_MS = Number(process.env.EDGEOS_CONFIRM_WINDOW_MS ?? 60 * 1000);
const HOST_ALLOWLIST = new Set(['127.0.0.1', 'localhost', '[::1]']);

function parseArgs(argv) {
  const out = {};
  for (let i = 0; i < argv.length; i += 1) {
    if (!argv[i].startsWith('--')) continue;
    const key = argv[i].slice(2);
    const next = argv[i + 1];
    if (!next || next.startsWith('--')) out[key] = true;
    else {
      out[key] = next;
      i += 1;
    }
  }
  return out;
}

// ---------------------------------------------------------------- registry 装载
const registryFile = path.join(WEB_ROOT, 'products', PRODUCT, 'src/generated/registry.json');
if (!existsSync(registryFile)) {
  console.error(`缺少 registry：${registryFile}\n先跑 node tooling/cli.mjs registry --product ${PRODUCT}`);
  process.exit(1);
}
const registry = JSON.parse(readFileSync(registryFile, 'utf8'));
const catalog = yaml.load(readFileSync(path.join(WEB_ROOT, 'permissions/catalog.yaml'), 'utf8'));

const permissionDeps = new Map(catalog.permissions.map((p) => [p.id, p.dependencies ?? []]));

/**
 * 配置 Schema：构建期已校验过，这里只负责运行期使用。
 * 风险等级 → 确认窗口、字段校验、写权限，全部来自这一份文件，避免"前端一套、后端一套"。
 */
const schemasDir = path.join(WEB_ROOT, 'schemas');
const configSchemas = new Map();
if (existsSync(schemasDir)) {
  for (const file of readdirSync(schemasDir).filter((name) => name.endsWith('.json'))) {
    const doc = JSON.parse(readFileSync(path.join(schemasDir, file), 'utf8'));
    configSchemas.set(doc.id, doc);
  }
}

/** sectionId → 规则。未知分区一律拒绝（fail-closed），不允许"没登记就放行"。 */
function sectionRule(sectionId) {
  const doc = configSchemas.get(sectionId);
  if (!doc) return null;
  return {
    writePermission: doc.writePermission,
    readPermission: doc.readPermission ?? null,
    riskClass: doc.riskClass,
    confirmWindowSeconds: doc.confirmWindowSeconds ?? 0,
    // 台架可以用 EDGEOS_CONFIRM_WINDOW_MS 把窗口压短，验证"超时自动回滚"而不用等 60 秒
    effectiveWindowMs: Math.min(doc.confirmWindowSeconds ?? 0, CONFIRM_WINDOW_MS / 1000) * 1000,
    schema: doc,
  };
}

// ---------------------------------------------------------------- 设备与账号状态
const now = () => Date.now();

function hashPassword(password, salt = randomBytes(16).toString('hex')) {
  return `${salt}:${scryptSync(password, salt, 32).toString('hex')}`;
}

function verifyPassword(password, stored) {
  const [salt, digest] = stored.split(':');
  const candidate = scryptSync(password, salt, 32);
  const expected = Buffer.from(digest, 'hex');
  return candidate.length === expected.length && timingSafeEqual(candidate, expected);
}

/** 设备端账号（内存态）。初始口令只用于首次引导，登录后强制改密（§5.6）。 */
const users = [
  {
    id: 'u_1000',
    username: 'admin',
    displayName: '设备管理员',
    roleIds: ['admin'],
    status: 'active',
    mustChangePassword: true,
    password: hashPassword('Initial-EdgeOS-2026'),
    lastLogin: null,
  },
  {
    id: 'u_1001',
    username: 'engineer',
    displayName: '现场工程师',
    roleIds: ['engineer'],
    status: 'active',
    mustChangePassword: false,
    password: hashPassword('Engineer-Pass-2026'),
    lastLogin: null,
  },
  {
    id: 'u_1003',
    username: 'operator',
    displayName: '运维值班',
    roleIds: ['operator'],
    status: 'active',
    mustChangePassword: false,
    password: hashPassword('Operator-Pass-2026'),
    lastLogin: null,
  },
  {
    id: 'u_1002',
    username: 'viewer',
    displayName: '只读巡检',
    roleIds: ['viewer'],
    status: 'active',
    mustChangePassword: false,
    password: hashPassword('Viewer-Pass-2026'),
    lastLogin: null,
  },
];

const sessions = new Map();
const loginFailures = new Map(); // `${username}|${ip}` → {count, lockedUntil}
const idempotency = new Map(); // key → {digest, response}
const transactions = new Map();
const tasks = new Map();
const auditLog = [];
let policyRevision = 1;
let configRevision = 7;

/**
 * 台架上的"设备状态"。配置事务 apply 之后要真的改到这里 —— 否则 E2E 只能验
 * "接口返回 200"，验不出"配置到底生效没有"。
 */
const interfacesState = [
  { id: 'eth0', name: 'eth0', mac: '02:00:00:00:00:01', addresses: ['192.168.10.21/24'], gateway: '192.168.10.1', mtu: 1500, state: 'up', speedMbps: 1000 },
  { id: 'eth1', name: 'eth1', mac: '02:00:00:00:00:02', addresses: [], gateway: '', mtu: 1500, state: 'down', speedMbps: null },
];

/**
 * 告警（§8.3）。台架上用"确定性的产生源"而不是定时器：
 * 配置回滚、任务取消这类动作会真的产生告警 —— 这样 E2E 能验"事件真的推到了页面"，
 * 而不是等一个随机闹钟。
 */
const alarms = [
  {
    id: 'al_0001',
    ts: new Date(now() - 120000).toISOString(),
    level: 'major',
    source: 'northbound',
    code: 'NB-MQTT-DISCONNECT',
    message: '北向 MQTT 断开 12s 后重连成功',
    state: 'active',
    ackBy: null,
    ackAt: null,
  },
  {
    id: 'al_0002',
    ts: new Date(now() - 600000).toISOString(),
    level: 'minor',
    source: 'collector',
    code: 'COL-SLAVE-TIMEOUT',
    message: '2# 表抄读超时（连续 3 次）',
    state: 'active',
    ackBy: null,
    ackAt: null,
  },
  {
    id: 'al_0003',
    ts: new Date(now() - 3600000).toISOString(),
    level: 'info',
    source: 'system',
    code: 'SYS-BOOT',
    message: '设备重启完成，自检通过',
    state: 'cleared',
    ackBy: null,
    ackAt: null,
  },
];
let alarmSeq = alarms.length;

const LEVEL_ORDER = { critical: 0, major: 1, minor: 2, warning: 3, info: 4 };

function raiseAlarm({ level, source, code, message }) {
  alarmSeq += 1;
  const alarm = {
    id: `al_${String(alarmSeq).padStart(4, '0')}`,
    ts: new Date().toISOString(),
    level,
    source,
    code,
    message,
    state: 'active',
    ackBy: null,
    ackAt: null,
  };
  alarms.unshift(alarm);
  broadcast('alarm', 'alarm', alarm);
  return alarm;
}

/** 各配置分区"设备上的当前值"。网络接口从 interfacesState 派生，其余在这里维护。 */
const configSectionsState = {
  'system.time': { ntpEnabled: true, servers: ['ntp.aliyun.com'], manualTime: '' },
  'network.firewall': { enabled: true, defaultPolicy: 'deny', allowSources: ['192.168.10.0/24'] },
};

function sectionValues(sectionId) {
  if (sectionId === 'network.interfaces') {
    const target = interfacesState[0];
    return {
      interfaceId: target.id,
      addresses: [...target.addresses],
      gateway: target.gateway ?? '',
      mtu: target.mtu ?? 1500,
    };
  }
  return { ...(configSectionsState[sectionId] ?? {}) };
}

const capabilities = {
  iems: {
    productId: 'iems',
    model: 'IEMS-300-4G',
    firmware: registry.productVersion,
    capabilityRevision: 17,
    schemaVersion: 1,
    hardware: {
      'network.ethernet.ports': 2,
      'io.rs485.ports': 2,
      'storage.sdcard': true,
      // 台架型号没有蜂窝模块：用来验证"能力缺失 → 菜单不出现 + API 拒绝"
      'network.cellular': false,
    },
    software: { 'collector.modbus': true, 'collector.iec104': true },
    entitlements: {
      'iems.peak-shaving': { state: 'active', expiresAt: null },
      'iems.cloud-advanced': { state: 'absent', expiresAt: null },
    },
    runtimeHealth: { 'collector.modbus': 'healthy' },
  },
  dcu: {
    productId: 'dcu',
    model: 'DCU-200',
    firmware: registry.productVersion,
    capabilityRevision: 9,
    schemaVersion: 1,
    hardware: { 'network.ethernet.ports': 1, 'io.rs485.ports': 4, 'network.cellular': true },
    software: { 'collector.modbus': true, 'collector.dlt698': true },
    entitlements: {},
    runtimeHealth: { 'collector.modbus': 'healthy' },
  },
};

function capabilitySnapshot() {
  return capabilities[PRODUCT] ?? capabilities.iems;
}

// ---------------------------------------------------------------- 授权引擎
function roleById(id) {
  return registry.roles.find((role) => role.id === id);
}

/** 权限闭包：显式依赖一并计入（write 不隐含 read，但目录可以声明依赖）。 */
function effectivePermissions(user) {
  const out = new Set();
  const visit = (id) => {
    if (out.has(id)) return;
    out.add(id);
    for (const dep of permissionDeps.get(id) ?? []) visit(dep);
  };
  for (const roleId of user.roleIds) {
    for (const perm of roleById(roleId)?.permissions ?? []) visit(perm);
  }
  return out;
}

function mePayload(user, session) {
  return {
    userId: user.id,
    username: user.username,
    displayName: user.displayName,
    roles: user.roleIds.map((roleId) => ({ roleId, scope: { type: 'gateway', id: 'self' } })),
    permissions: [...effectivePermissions(user)].sort(),
    scopes: ['gateway:self'],
    mustChangePassword: user.mustChangePassword,
    policyRevision,
    csrfToken: session.csrf,
    sessionExpiresAt: new Date(session.absoluteExpiresAt).toISOString(),
  };
}

function capabilityAvailable(key) {
  const snapshot = capabilitySnapshot();
  const [scope, ...rest] = key.split('.');
  const value = (snapshot[scope] ?? {})[rest.join('.')];
  if (value === undefined) return false;
  if (typeof value === 'number') return value > 0;
  return value === true;
}

function entitlementActive(key) {
  const entry = capabilitySnapshot().entitlements?.[key];
  if (!entry) return false;
  if (entry.state !== 'active') return false;
  if (entry.expiresAt && Date.parse(entry.expiresAt) <= now()) return false;
  return true;
}

/** 与前端 buildMenu 同一套语义，但**服务端独立计算**——前端结果不参与判定。 */
function authorize(ctx, op, session) {
  if (op.anonymous) return null;
  if (!session) return { status: 401, code: 'unauthenticated', message: '需要登录' };
  const user = users.find((candidate) => candidate.id === session.userId);
  if (!user || user.status !== 'active') return { status: 401, code: 'session_revoked', message: '会话已失效' };

  const granted = effectivePermissions(user);
  for (const perm of op.permissions) {
    if (!granted.has(perm)) {
      return { status: 403, code: 'permission_denied', message: `缺少权限 ${perm}`, details: { permission: perm } };
    }
  }
  for (const cap of op.capabilities) {
    if (!capabilityAvailable(cap)) {
      return { status: 403, code: 'capability_unavailable', message: `当前设备不具备能力 ${cap}`, details: { capability: cap } };
    }
  }
  for (const ent of op.entitlements) {
    if (!entitlementActive(ent)) {
      return { status: 403, code: 'entitlement_inactive', message: `功能授权未生效 ${ent}`, details: { entitlement: ent } };
    }
  }
  // 配置分区权限由 sectionId 解析（§7.2 配置 API 权限解析）
  if (op.permissionSource === 'config-section') {
    const sectionId = ctx.body?.sectionId ?? ctx.params?.sectionId;
    if (sectionId) {
      const rule = sectionRule(sectionId);
      if (!rule) return { status: 403, code: 'unknown_section', message: `未知配置分区 ${sectionId}` };
      // 读分区用 readPermission、写用 writePermission：两者不等价（例如 engineer 能读
      // 防火墙策略但不能改）。缺哪个就报哪个，前端能给出可执行提示。
      const required = ctx.req.method === 'GET' ? rule.readPermission ?? rule.writePermission : rule.writePermission;
      if (!granted.has(required)) {
        return {
          status: 403,
          code: 'permission_denied',
          message: `缺少权限 ${required}`,
          details: { permission: required, sectionId },
        };
      }
    }
  }
  if (op.authorizationPolicy === 'task-cancel-v1') {
    const task = tasks.get(ctx.params?.id);
    if (!task) return { status: 404, code: 'not_found', message: '任务不存在' };
    const isOwner = task.createdBy === user.id;
    if (isOwner) {
      if (!task.ownerCancel) return { status: 403, code: 'cancel_not_allowed', message: '该任务类型不允许创建者取消' };
      if (!granted.has(task.requiredPermission)) {
        return { status: 403, code: 'permission_denied', message: '已失去执行该任务的权限' };
      }
    } else if (!granted.has('core.task:cancel')) {
      return { status: 403, code: 'permission_denied', message: '跨用户取消需要 core.task:cancel' };
    }
    if (!task.cancelableStages.includes(task.stage)) {
      return { status: 409, code: 'stage_not_cancelable', message: `阶段 ${task.stage} 不可安全取消` };
    }
  }
  return null;
}

// ---------------------------------------------------------------- 路由表
/** 把 `/api/v1/users/{id}` 编译成正则；同一份表既用于匹配也用于授权。 */
const routes = registry.api.map((op) => {
  const pattern = new RegExp('^' + compilePath(op.path) + '$');
  const names = [...op.path.matchAll(/\{([^}]+)\}/g)].map((m) => m[1]);
  return { op, pattern, names };
});

/** 把 `/api/v1/users/{id}` 编译成正则片段：先按占位符切分，再逐段转义，避免把 `{}` 转义坏。 */
function compilePath(routePath) {
  return routePath
    .split(/(\{[^}]+\})/)
    .map((segment) => (/^\{[^}]+\}$/.test(segment) ? '([^/]+)' : segment.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')))
    .join('');
}

const handlers = {
  'GET /api/v1/system/info': (ctx) => ({
    productId: registry.productId,
    displayName: registry.productName,
    language: 'zh-CN',
    themeVersion: `${registry.productVersion}-theme1`,
    loginHint: '首次登录需修改初始口令',
  }),

  'POST /api/v1/auth/login': (ctx) => {
    const { username, password } = ctx.body ?? {};
    const key = `${username}|${ctx.ip}`;
    const failure = loginFailures.get(key);
    if (failure?.lockedUntil && failure.lockedUntil > now()) {
      throw httpError(429, 'too_many_attempts', '登录尝试过于频繁，请稍后再试');
    }
    const user = users.find((candidate) => candidate.username === username);
    if (!user || !verifyPassword(String(password ?? ''), user.password)) {
      const next = { count: (failure?.count ?? 0) + 1, lockedUntil: 0 };
      if (next.count >= 5) next.lockedUntil = now() + 30_000;
      loginFailures.set(key, next);
      audit(ctx, 'auth.login.failed', { username });
      // 统一文案，避免账号枚举
      throw httpError(401, 'invalid_credentials', '用户名或口令错误');
    }
    if (user.status !== 'active') throw httpError(403, 'account_disabled', '账号已禁用');
    loginFailures.delete(key);
    const session = createSession(user);
    user.lastLogin = new Date().toISOString();
    audit(ctx, 'auth.login.success', { userId: user.id });
    ctx.setSession(session);
    return { user: mePayload(user, session) };
  },

  'POST /api/v1/auth/logout': (ctx) => {
    sessions.delete(ctx.session.sid);
    ctx.clearSession();
    return null;
  },

  'GET /api/v1/auth/me': (ctx) => {
    const user = users.find((candidate) => candidate.id === ctx.session.userId);
    return mePayload(user, ctx.session);
  },

  'POST /api/v1/auth/password': (ctx) => {
    const user = users.find((candidate) => candidate.id === ctx.session.userId);
    const { currentPassword, newPassword } = ctx.body ?? {};
    if (!verifyPassword(String(currentPassword ?? ''), user.password)) {
      throw httpError(400, 'invalid_current_password', '当前口令不正确');
    }
    if (String(newPassword ?? '').length < 12) {
      throw httpError(400, 'weak_password', '口令长度不足 12 位');
    }
    if (newPassword === user.username) throw httpError(400, 'weak_password', '口令不能与用户名相同');
    user.password = hashPassword(newPassword);
    user.mustChangePassword = false;
    // 改密后撤销其他会话，并轮换当前会话（§5.6）
    for (const [sid, session] of sessions) if (session.userId === user.id && sid !== ctx.session.sid) sessions.delete(sid);
    rotateSession(ctx.session);
    audit(ctx, 'auth.password.changed', { userId: user.id });
    return null;
  },

  'GET /api/v1/system/capabilities': () => capabilitySnapshot(),

  'GET /api/v1/users': () => ({
    items: users.map((user) => ({
      id: user.id,
      username: user.username,
      displayName: user.displayName,
      roles: user.roleIds,
      status: user.status,
      mustChangePassword: user.mustChangePassword,
      lastLogin: user.lastLogin,
    })),
  }),

  'POST /api/v1/users': (ctx) => {
    const { username, roleIds } = ctx.body ?? {};
    if (!username || users.some((user) => user.username === username)) {
      throw httpError(409, 'username_taken', '用户名已存在');
    }
    const user = {
      id: `u_${1000 + users.length + 1}`,
      username,
      displayName: username,
      roleIds: Array.isArray(roleIds) && roleIds.length ? roleIds : ['viewer'],
      status: 'active',
      mustChangePassword: true,
      password: hashPassword(randomBytes(12).toString('base64url')),
      lastLogin: null,
    };
    users.push(user);
    audit(ctx, 'user.created', { userId: user.id, roleIds: user.roleIds });
    return { id: user.id, username: user.username, mustChangePassword: true };
  },

  'GET /api/v1/users/{id}': (ctx) => {
    const user = users.find((candidate) => candidate.id === ctx.params.id);
    if (!user) throw httpError(404, 'not_found', '用户不存在');
    return { id: user.id, username: user.username, roles: user.roleIds, status: user.status };
  },

  'PATCH /api/v1/users/{id}': (ctx) => {
    const user = users.find((candidate) => candidate.id === ctx.params.id);
    if (!user) throw httpError(404, 'not_found', '用户不存在');
    if (ctx.body?.status === 'disabled') {
      const activeAdmins = users.filter((candidate) => candidate.status === 'active' && candidate.roleIds.includes('admin'));
      if (user.roleIds.includes('admin') && activeAdmins.length <= 1) {
        throw httpError(409, 'last_admin', '不能禁用最后一个可用管理员');
      }
      user.status = 'disabled';
    }
    if (Array.isArray(ctx.body?.roleIds)) user.roleIds = ctx.body.roleIds;
    audit(ctx, 'user.updated', { userId: user.id });
    return { id: user.id, status: user.status, roles: user.roleIds };
  },

  'DELETE /api/v1/users/{id}': (ctx) => {
    const index = users.findIndex((candidate) => candidate.id === ctx.params.id);
    if (index < 0) throw httpError(404, 'not_found', '用户不存在');
    const user = users[index];
    const activeAdmins = users.filter((candidate) => candidate.status === 'active' && candidate.roleIds.includes('admin'));
    if (user.roleIds.includes('admin') && activeAdmins.length <= 1) {
      throw httpError(409, 'last_admin', '不能删除最后一个可用管理员');
    }
    users.splice(index, 1);
    audit(ctx, 'user.deleted', { userId: user.id });
    return null;
  },

  'GET /api/v1/roles': () => ({
    items: registry.roles.map((role) => ({
      id: role.id,
      name: role.name,
      builtin: role.builtin,
      permissions: role.permissions,
    })),
  }),

  'POST /api/v1/roles': (ctx) => {
    const { id, name, permissions } = ctx.body ?? {};
    if (!id) throw httpError(400, 'invalid_request', '缺少角色 id');
    if (registry.roles.some((role) => role.id === id)) throw httpError(409, 'role_exists', '角色已存在');
    const actor = users.find((candidate) => candidate.id === ctx.session.userId);
    const granted = effectivePermissions(actor);
    for (const perm of permissions ?? []) {
      if (!granted.has(perm)) {
        throw httpError(403, 'grant_escalation', `不能授予自己没有的权限 ${perm}`);
      }
    }
    registry.roles.push({ id, name: name ?? id, builtin: false, permissions: [...(permissions ?? [])].sort() });
    return { id };
  },

  'GET /api/v1/network/interfaces': () => ({ items: interfacesState.map((item) => ({ ...item, addresses: [...item.addresses] })) }),

  'PATCH /api/v1/network/interfaces/{id}': (ctx) => {
    const transaction = createTransaction(ctx, 'network.interfaces', ctx.body ?? {});
    return { transactionId: transaction.id, state: transaction.state };
  },

  'GET /api/v1/network/cellular': () => ({
    state: 'connected',
    operator: 'CHN-UNICOM',
    technology: 'LTE Cat.4',
    signalDbm: -71,
    rxBytes: 734003200,
    txBytes: 41943040,
    updatedAt: new Date(now() - 8000).toISOString(),
  }),

  'GET /api/v1/system/services': () => ({
    items: [
      { id: 'collector', name: '采集服务', state: 'running', since: new Date(now() - 86400000).toISOString() },
      { id: 'northbound', name: '北向服务', state: 'running', since: new Date(now() - 86400000).toISOString() },
      { id: 'webd', name: 'Web 网关', state: 'running', since: new Date(now() - 3600000).toISOString() },
    ],
  }),

  'POST /api/v1/system/services/{id}/actions/{action}': (ctx) => {
    const allowed = ['start', 'stop', 'restart'];
    if (!allowed.includes(ctx.params.action)) throw httpError(400, 'invalid_action', '不支持的 action');
    const task = createTask(ctx, {
      type: `service.${ctx.params.action}`,
      target: ctx.params.id,
      requiredPermission: 'system.service:execute',
      ownerCancel: true,
      cancelableStages: ['QUEUED', 'RUNNING'],
    });
    return { taskId: task.id, state: task.state };
  },

  'POST /api/v1/system/reboot': (ctx) => {
    const task = createTask(ctx, { type: 'system.reboot', target: 'gateway', requiredPermission: 'system.reboot:execute', ownerCancel: false, cancelableStages: [] });
    return { taskId: task.id, state: task.state };
  },

  // 只有 release-scope 启用了 firmwareUpgrade 的产品才会注册这条路由（iEMS 上它根本不存在）
  'POST /api/v1/system/upgrade': (ctx) => {
    const task = createTask(ctx, {
      type: 'system.upgrade',
      target: 'firmware',
      requiredPermission: 'system.upgrade:execute',
      ownerCancel: false,
      cancelableStages: [],
    });
    return { taskId: task.id, state: task.state };
  },

  'GET /api/v1/alarms': (ctx) => {
    const level = ctx.query.level;
    const state = ctx.query.state;
    const limit = Number(ctx.query.limit ?? 50);
    const items = alarms
      .filter((alarm) => !level || alarm.level === level)
      .filter((alarm) => !state || alarm.state === state)
      .sort((a, b) => (LEVEL_ORDER[a.level] ?? 9) - (LEVEL_ORDER[b.level] ?? 9) || (a.ts < b.ts ? 1 : -1))
      .slice(0, Number.isFinite(limit) ? limit : 50);
    return { items, total: items.length };
  },

  'POST /api/v1/alarms/{id}/ack': (ctx) => {
    const alarm = alarms.find((item) => item.id === ctx.params.id);
    if (!alarm) throw httpError(404, 'not_found', '告警不存在');
    if (alarm.state === 'acked') return { ...alarm, idempotent: true };
    alarm.state = 'acked';
    alarm.ackBy = ctx.session.userId;
    alarm.ackAt = new Date().toISOString();
    audit(ctx, 'alarm.acked', { alarmId: alarm.id });
    broadcast('alarm', 'alarm', alarm);
    return { ...alarm };
  },

  'GET /api/v1/logs/system': () => ({
    items: [
      { ts: new Date(now() - 5000).toISOString(), level: 'info', service: 'collector', message: 'modbus poll ok (2 slaves)' },
      { ts: new Date(now() - 12000).toISOString(), level: 'warn', service: 'northbound', message: 'mqtt reconnect in 2s' },
    ],
    nextCursor: null,
  }),

  'GET /api/v1/logs/audit': () => ({ items: auditLog.slice(-50).reverse(), nextCursor: null }),

  'GET /api/v1/config/transactions': (ctx) => {
    const sectionId = ctx.query.sectionId;
    const state = ctx.query.state;
    const items = [...transactions.values()]
      .filter((tx) => tx.createdBy === ctx.session.userId)
      .filter((tx) => !sectionId || tx.sectionId === sectionId)
      .filter((tx) => !state || tx.state === state)
      .sort((a, b) => (a.createdAt < b.createdAt ? 1 : -1))
      .map(publicTransactionFields);
    return { items };
  },

  'POST /api/v1/config/transactions': (ctx) => {
    const sectionId = ctx.body?.sectionId;
    if (!sectionId) throw httpError(400, 'invalid_request', '缺少 sectionId');
    const rule = sectionRule(sectionId);
    if (!rule) throw httpError(403, 'unknown_section', `未知配置分区 ${sectionId}`);
    const values = ctx.body?.values ?? {};
    assertSchemaValues(rule.schema, values);
    const transaction = createTransaction(ctx, sectionId, values);
    return { transactionId: transaction.id, state: transaction.state, baseRevision: transaction.baseRevision };
  },

  'GET /api/v1/config/sections/{sectionId}': (ctx) => ({
    sectionId: ctx.params.sectionId,
    revision: configRevision,
    values: sectionValues(ctx.params.sectionId),
  }),

  'GET /api/v1/config/transactions/{id}': (ctx) => publicTransaction(ctx.params.id),

  'PUT /api/v1/config/transactions/{id}/sections/{sectionId}': (ctx) => {
    const transaction = transactions.get(ctx.params.id);
    if (!transaction) throw httpError(404, 'not_found', '事务不存在');
    if (transaction.state !== 'DRAFT') throw httpError(409, 'invalid_state', `事务状态 ${transaction.state} 不可写入`);
    const baseRevision = ctx.body?.baseRevision;
    if (baseRevision !== undefined && baseRevision !== transaction.baseRevision) {
      throw httpError(409, 'revision_conflict', '基线版本已变化，请重新加载后编辑');
    }
    const rule = sectionRule(transaction.sectionId);
    const nextValues = { ...transaction.values, ...(ctx.body?.values ?? {}) };
    if (rule) assertSchemaValues(rule.schema, nextValues);
    transaction.values = nextValues;
    return { state: transaction.state, revision: transaction.revision };
  },

  'POST /api/v1/config/transactions/{id}/apply': (ctx) => {
    const transaction = transactions.get(ctx.params.id);
    if (!transaction) throw httpError(404, 'not_found', '事务不存在');
    if (transaction.state === 'PENDING_CONFIRM' || transaction.state === 'CONFIRMED') {
      return { transactionId: transaction.id, state: transaction.state, idempotent: true };
    }
    if (transaction.state !== 'DRAFT' && transaction.state !== 'VALIDATING') {
      throw httpError(409, 'invalid_state', `事务状态 ${transaction.state} 不可应用`);
    }
    transaction.state = 'VALIDATING';
    transaction.state = 'APPLYING';
    configRevision += 1;
    transaction.appliedRevision = configRevision;
    transaction.appliedAt = new Date().toISOString();
    // 风险等级决定要不要确认窗口：R0/R1 直接生效，R2 起必须"应用后 N 秒内确认，否则回滚"。
    // deadline 由设备侧 watchdog 负责，浏览器计时器只是显示。
    const rule = sectionRule(transaction.sectionId);
    const windowMs = rule?.effectiveWindowMs ?? 0;
    if (windowMs > 0) {
      transaction.state = 'PENDING_CONFIRM';
      transaction.deadline = new Date(now() + windowMs).toISOString();
    } else {
      transaction.state = 'CONFIRMED';
      transaction.confirmedAt = new Date().toISOString();
    }
    applyToDeviceState(transaction);
    audit(ctx, 'config.applied', { transactionId: transaction.id, sectionId: transaction.sectionId });
    broadcast('config-transaction', 'config-transaction', publicTransactionFields(transaction));
    return { transactionId: transaction.id, state: transaction.state, deadline: transaction.deadline };
  },

  'POST /api/v1/config/transactions/{id}/confirm': (ctx) => {
    const transaction = transactions.get(ctx.params.id);
    if (!transaction) throw httpError(404, 'not_found', '事务不存在');
    if (transaction.state === 'CONFIRMED') return { transactionId: transaction.id, state: 'CONFIRMED', idempotent: true };
    if (transaction.state !== 'PENDING_CONFIRM') throw httpError(409, 'invalid_state', `事务状态 ${transaction.state} 不可确认`);
    if (transaction.deadline && Date.parse(transaction.deadline) < now()) {
      transaction.state = 'ROLLING_BACK';
      transaction.state = 'ROLLED_BACK';
      throw httpError(409, 'deadline_expired', '确认窗口已过期，已回滚');
    }
    transaction.state = 'CONFIRMED';
    transaction.confirmedAt = new Date().toISOString();
    audit(ctx, 'config.confirmed', { transactionId: transaction.id });
    broadcast('config-transaction', 'config-transaction', publicTransactionFields(transaction));
    return { transactionId: transaction.id, state: transaction.state };
  },

  'POST /api/v1/config/transactions/{id}/rollback': (ctx) => {
    const transaction = transactions.get(ctx.params.id);
    if (!transaction) throw httpError(404, 'not_found', '事务不存在');
    if (transaction.state === 'ROLLED_BACK') return { transactionId: transaction.id, state: 'ROLLED_BACK', idempotent: true };
    if (!['PENDING_CONFIRM', 'APPLYING', 'CONFIRMED'].includes(transaction.state)) {
      throw httpError(409, 'invalid_state', `事务状态 ${transaction.state} 不可回滚`);
    }
    transaction.state = 'ROLLING_BACK';
    if (transaction.snapshot) {
      interfacesState.splice(0, interfacesState.length, ...structuredClone(transaction.snapshot));
    }
    if (transaction.sectionSnapshot) {
      configSectionsState[transaction.sectionId] = structuredClone(transaction.sectionSnapshot);
    }
    transaction.state = 'ROLLED_BACK';
    transaction.rolledBackAt = new Date().toISOString();
    audit(ctx, 'config.rolledback', { transactionId: transaction.id });
    broadcast('config-transaction', 'config-transaction', publicTransactionFields(transaction));
    // 回滚是"需要人知道"的事件：配置没生效，现场必须看到
    raiseAlarm({
      level: 'major',
      source: 'config',
      code: 'CFG-ROLLED-BACK',
      message: `配置分区 ${transaction.sectionId} 已回滚（未在确认窗口内确认或主动取消）`,
    });
    return { transactionId: transaction.id, state: transaction.state };
  },

  'GET /api/v1/tasks': () => ({ items: [...tasks.values()].map(publicTask) }),

  'GET /api/v1/tasks/{id}': (ctx) => {
    const task = tasks.get(ctx.params.id);
    if (!task) throw httpError(404, 'not_found', '任务不存在');
    return publicTask(task);
  },

  'POST /api/v1/tasks/{id}/cancel': (ctx) => {
    const task = tasks.get(ctx.params.id);
    task.state = 'CANCELING';
    task.state = 'CANCELED';
    task.updatedAt = new Date().toISOString();
    audit(ctx, 'task.canceled', { taskId: task.id });
    broadcast('task', 'task', publicTask(task));
    raiseAlarm({ level: 'info', source: 'task', code: 'TASK-CANCELED', message: `任务 ${task.type} 已取消` });
    return { taskId: task.id, state: task.state };
  },

  'GET /api/v1/ws': () => {
    throw httpError(426, 'upgrade_required', '该接口只接受 WebSocket 升级');
  },

  // ---- 迁移期的取数面（旧页面仍在使用，逐步收敛到 Pack API） ----
  'GET /api/v1/contracts': () => legacyContract(),

  'GET /api/v1/data/latest/{tag}': (ctx) => ({
    id: ctx.params.tag,
    value: Number((220 + Math.sin(now() / 5000) * 3).toFixed(2)),
    unit: 'V',
    quality: 'good',
    ts: new Date().toISOString(),
  }),

  // ---- Pack 扩展接口 ----
  'GET /api/v1/iems/telemetry': () => ({
    soc: 68.4,
    pvPowerKw: 42.1,
    gridPowerKw: -12.6,
    loadPowerKw: 29.5,
    updatedAt: new Date(now() - 3000).toISOString(),
  }),

  'GET /api/v1/iems/strategy': () => ({
    items: [
      { id: 'peak-shaving', name: '削峰填谷', enabled: true, window: '08:00-11:30 / 18:00-21:00' },
      { id: 'demand-control', name: '需量控制', enabled: false, window: '—' },
    ],
  }),

  'GET /api/v1/dcu/meters': () => ({
    items: [
      { id: 'm_0001', address: '00000001', name: '1# 进线表', protocol: 'DLT698.45', online: true, lastReadingAt: new Date(now() - 60000).toISOString() },
      { id: 'm_0002', address: '00000002', name: '2# 馈线表', protocol: 'DLT645-2007', online: false, lastReadingAt: new Date(now() - 3600000).toISOString() },
    ],
  }),

  'GET /api/v1/dcu/frames': () => ({
    items: [
      { ts: new Date(now() - 2000).toISOString(), direction: 'rx', channel: 'rs485-1', hex: '68 12 34 56 78 90 12 68 11 04 33 33 34 33 78 16' },
    ],
  }),
};

// ---------------------------------------------------------------- 配置 Schema 校验
const IPV4_RE = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

function checkFormat(format, value) {
  if (format === 'ipv4') {
    const match = IPV4_RE.exec(value);
    return !!match && match.slice(1).every((part) => Number(part) <= 255);
  }
  if (format === 'cidr') {
    const [address, prefix] = String(value).split('/');
    if (!IPV4_RE.test(address ?? '')) return false;
    const bits = Number(prefix);
    return Number.isInteger(bits) && bits >= 0 && bits <= 32;
  }
  if (format === 'hostname') return /^[A-Za-z0-9]([A-Za-z0-9.-]{0,253})$/.test(String(value));
  if (format === 'rfc3339') return !Number.isNaN(Date.parse(String(value)));
  return true;
}

/**
 * 服务端校验：前端已经校验过一次，但**这里才是结论**。
 * 错误按字段返回，前端能把红字挂在对应输入框上（而不是一句"参数错误"）。
 */
function assertSchemaValues(schema, values) {
  const errors = {};
  const properties = schema.properties ?? {};
  for (const name of schema.required ?? []) {
    if (values[name] === undefined || values[name] === null || values[name] === '') {
      errors[name] = '必填';
    }
  }
  for (const [name, value] of Object.entries(values)) {
    const field = properties[name];
    if (!field) {
      errors[name] = '未声明的字段（拒绝写入未知字段）';
      continue;
    }
    // 空串按"未填"处理（required 已单独判过）：与前端 validateField 的语义保持一致，
    // 否则会出现"前端放过、后端 422"这种最让人困惑的组合。
    if (value === null || value === undefined || value === '') continue;
    if (field.enum && !field.enum.includes(value)) {
      errors[name] = `取值必须是 ${field.enum.join(' / ')}`;
      continue;
    }
    if (field.type === 'string') {
      if (typeof value !== 'string') errors[name] = '必须是字符串';
      else if (field.format && !checkFormat(field.format, value)) errors[name] = `格式不符合 ${field.format}`;
    } else if (field.type === 'integer') {
      if (!Number.isInteger(value)) errors[name] = '必须是整数';
      else if (field.minimum !== undefined && value < field.minimum) errors[name] = `不得小于 ${field.minimum}`;
      else if (field.maximum !== undefined && value > field.maximum) errors[name] = `不得大于 ${field.maximum}`;
    } else if (field.type === 'boolean') {
      if (typeof value !== 'boolean') errors[name] = '必须是布尔值';
    } else if (field.type === 'array') {
      if (!Array.isArray(value)) errors[name] = '必须是数组';
      else if (field.minItems !== undefined && value.length < field.minItems) errors[name] = `至少 ${field.minItems} 项`;
      else if (field.maxItems !== undefined && value.length > field.maxItems) errors[name] = `最多 ${field.maxItems} 项`;
      else {
        const badIndex = value.findIndex((item) => typeof item !== 'string' || (field.items?.format && !checkFormat(field.items.format, item)));
        if (badIndex >= 0) errors[name] = `第 ${badIndex + 1} 项格式不符合 ${field.items?.format ?? 'string'}`;
      }
    }
  }
  if (Object.keys(errors).length > 0) {
    throw httpError(422, 'schema_validation_failed', '配置项校验未通过', { fieldErrors: errors });
  }
}

/** 应用即落设备状态：网络接口是唯一实现了"真写"的分区，其余分区仍是台架占位。 */
function applyToDeviceState(transaction) {
  if (transaction.sectionId === 'network.interfaces') {
    applyNetworkInterfaces(transaction);
    return;
  }
  // 其余分区：整段覆盖（台架语义，够用于验证"读回来的确实是新值"）
  configSectionsState[transaction.sectionId] = { ...transaction.values };
}

function applyNetworkInterfaces(transaction) {
  const values = transaction.values ?? {};
  const target = interfacesState.find((item) => item.id === values.interfaceId);
  if (!target) return;
  if (Array.isArray(values.addresses)) target.addresses = [...values.addresses];
  if (typeof values.gateway === 'string') target.gateway = values.gateway;
  if (Number.isInteger(values.mtu)) target.mtu = values.mtu;
  target.state = target.addresses.length > 0 ? 'up' : 'down';
}

function publicTransactionFields(transaction) {
  const rule = sectionRule(transaction.sectionId);
  return {
    transactionId: transaction.id,
    sectionId: transaction.sectionId,
    state: transaction.state,
    riskClass: rule?.riskClass ?? 'R0',
    values: transaction.values,
    baseRevision: transaction.baseRevision,
    appliedRevision: transaction.appliedRevision,
    createdAt: transaction.createdAt,
    appliedAt: transaction.appliedAt,
    confirmedAt: transaction.confirmedAt,
    rolledBackAt: transaction.rolledBackAt,
    deadline: transaction.deadline,
    confirmWindowSeconds: rule?.confirmWindowSeconds ?? 0,
  };
}

// ---------------------------------------------------------------- 事务/任务
function createTransaction(ctx, sectionId, values) {
  const id = `tx_${randomUUID().slice(0, 8)}`;
  const transaction = {
    id,
    sectionId,
    values,
    snapshot: sectionId === 'network.interfaces' ? structuredClone(interfacesState) : null,
    sectionSnapshot: sectionId === 'network.interfaces' ? null : structuredClone(configSectionsState[sectionId] ?? {}),
    baseRevision: configRevision,
    revision: configRevision + 1,
    state: 'DRAFT',
    createdBy: ctx.session.userId,
    createdAt: new Date().toISOString(),
    deadline: null,
    appliedRevision: null,
    appliedAt: null,
    confirmedAt: null,
    rolledBackAt: null,
  };
  transactions.set(id, transaction);
  audit(ctx, 'config.draft.created', { transactionId: id, sectionId });
  return transaction;
}

function publicTransaction(id) {
  const transaction = transactions.get(id);
  if (!transaction) throw httpError(404, 'not_found', '事务不存在');
  return publicTransactionFields(transaction);
}

function createTask(ctx, { type, target, requiredPermission, ownerCancel, cancelableStages }) {
  const id = `task_${randomUUID().slice(0, 8)}`;
  const task = {
    id,
    type,
    target,
    state: 'QUEUED',
    stage: 'QUEUED',
    progress: 0,
    createdBy: ctx.session.userId,
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString(),
    requiredPermission,
    ownerCancel,
    cancelableStages,
    result: null,
  };
  tasks.set(id, task);
  audit(ctx, 'task.created', { taskId: id, type });
  broadcast('task', 'task', publicTask(task));
  return task;
}

function publicTask(task) {
  const { requiredPermission, ownerCancel, cancelableStages, ...rest } = task;
  return rest;
}

// ---------------------------------------------------------------- 会话
function createSession(user) {
  const sid = randomBytes(24).toString('base64url');
  const session = {
    sid,
    userId: user.id,
    csrf: randomBytes(18).toString('base64url'),
    createdAt: now(),
    lastSeenAt: now(),
    absoluteExpiresAt: now() + ABSOLUTE_TIMEOUT_MS,
  };
  sessions.set(sid, session);
  return session;
}

function rotateSession(session) {
  session.csrf = randomBytes(18).toString('base64url');
  session.lastSeenAt = now();
}

function loadSession(req) {
  const sid = parseCookies(req.headers.cookie ?? '')['edgeos_sid'];
  if (!sid) return null;
  const session = sessions.get(sid);
  if (!session) return null;
  if (now() - session.lastSeenAt > IDLE_TIMEOUT_MS || now() > session.absoluteExpiresAt) {
    sessions.delete(sid);
    return null;
  }
  session.lastSeenAt = now();
  return session;
}

function parseCookies(header) {
  const out = {};
  for (const part of header.split(';')) {
    const index = part.indexOf('=');
    if (index < 0) continue;
    out[part.slice(0, index).trim()] = decodeURIComponent(part.slice(index + 1).trim());
  }
  return out;
}

// ---------------------------------------------------------------- 审计（内存）
function audit(ctx, action, detail) {
  auditLog.push({
    ts: new Date().toISOString(),
    action,
    actor: ctx.session?.userId ?? 'anonymous',
    requestId: ctx.requestId,
    detail,
  });
  if (auditLog.length > 500) auditLog.shift();
}

// ---------------------------------------------------------------- HTTP
function httpError(status, code, message, details) {
  const error = new Error(message);
  error.status = status;
  error.code = code;
  error.details = details;
  return error;
}

function sendJson(res, status, payload, extraHeaders = {}) {
  const body = payload === null ? '' : JSON.stringify(payload);
  res.writeHead(status, {
    'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store',
    'X-Content-Type-Options': 'nosniff',
    ...extraHeaders,
  });
  res.end(body);
}

function sendError(res, status, code, message, requestId, details) {
  sendJson(res, status, { code, message, details, requestId });
}

async function readBody(req) {
  const chunks = [];
  let size = 0;
  for await (const chunk of req) {
    size += chunk.length;
    if (size > 1024 * 1024) throw httpError(413, 'payload_too_large', '请求体过大');
    chunks.push(chunk);
  }
  if (chunks.length === 0) return null;
  const text = Buffer.concat(chunks).toString('utf8');
  try {
    return JSON.parse(text);
  } catch {
    throw httpError(400, 'invalid_json', '请求体不是合法 JSON');
  }
}

function checkHostAndOrigin(req) {
  const host = (req.headers.host ?? '').split(':')[0];
  if (!HOST_ALLOWLIST.has(host)) throw httpError(421, 'host_not_allowed', `Host 不在 allowlist: ${host}`);

  const method = req.method ?? 'GET';
  if (!['POST', 'PUT', 'PATCH', 'DELETE'].includes(method)) return;
  const origin = req.headers.origin;
  if (!origin) throw httpError(403, 'origin_required', '状态变更请求必须带 Origin');
  const originHost = new URL(origin).hostname;
  if (!HOST_ALLOWLIST.has(originHost)) throw httpError(403, 'origin_not_allowed', `Origin 不在 allowlist: ${originHost}`);
}

function checkCsrf(req, session) {
  const method = req.method ?? 'GET';
  if (!['POST', 'PUT', 'PATCH', 'DELETE'].includes(method)) return;
  if (!session) return; // 登录接口无会话，靠 Origin + 限流
  const token = req.headers['x-csrf-token'];
  if (token !== session.csrf) throw httpError(403, 'csrf_failed', 'CSRF token 校验失败');
}

const server = http.createServer(async (req, res) => {
  const requestId = randomUUID();
  const started = now();
  const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);

  if (!url.pathname.startsWith('/api/')) {
    return serveStatic(res, url.pathname);
  }

  try {
    checkHostAndOrigin(req);
    const session = loadSession(req);
    checkCsrf(req, session);

    const route = routes.find((candidate) => candidate.op.method === req.method && candidate.pattern.test(url.pathname));
    if (!route) throw httpError(404, 'not_found', `未登记的路由: ${req.method} ${url.pathname}`);

    const params = {};
    const match = route.pattern.exec(url.pathname);
    route.names.forEach((name, index) => {
      params[name] = decodeURIComponent(match[index + 1]);
    });

    const body = ['POST', 'PUT', 'PATCH'].includes(req.method ?? '') ? await readBody(req) : null;
    const ctx = {
      req,
      res,
      params,
      query: Object.fromEntries(url.searchParams.entries()),
      body,
      requestId,
      session,
      ip: req.socket.remoteAddress ?? 'unknown',
      setSession: (created) => {
        res.setHeader(
          'Set-Cookie',
          // 台架走明文 HTTP，所以这里没有 Secure；生产 webd 必须加 Secure（§5.6）。
          `edgeos_sid=${created.sid}; HttpOnly; SameSite=Strict; Path=/; Max-Age=${Math.floor(ABSOLUTE_TIMEOUT_MS / 1000)}`,
        );
      },
      clearSession: () => {
        res.setHeader('Set-Cookie', 'edgeos_sid=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0');
      },
    };

    const denial = authorize(ctx, route.op, session);
    if (denial) {
      audit(ctx, 'authz.denied', { operationId: route.op.operationId, code: denial.code });
      throw httpError(denial.status, denial.code, denial.message, denial.details);
    }

    const handler = handlers[`${route.op.method} ${route.op.path}`];
    if (!handler) throw httpError(501, 'not_implemented', `接口已登记但未实现: ${route.op.operationId}`);

    // 幂等：同 key 同摘要返回同一结果，不同摘要冲突（§7.1 规则 7）
    const idempotencyKey = req.headers['idempotency-key'];
    if (idempotencyKey && ['POST', 'PUT'].includes(req.method ?? '')) {
      const digest = createHash('sha256').update(JSON.stringify(body ?? {})).digest('hex');
      const previous = idempotency.get(String(idempotencyKey));
      if (previous) {
        if (previous.digest !== digest) throw httpError(409, 'idempotency_conflict', '相同幂等键的请求摘要不同');
        return sendJson(res, previous.status, previous.payload);
      }
      const result = handler(ctx);
      idempotency.set(String(idempotencyKey), { digest, status: 200, payload: result });
      return sendJson(res, 200, result);
    }

    const result = handler(ctx);
    const status = result === null ? 204 : route.op.successStatus ?? 200;
    if (result === null) {
      res.writeHead(204, { 'Cache-Control': 'no-store' });
      res.end();
      return;
    }
    sendJson(res, status, result);
  } catch (err) {
    const status = err.status ?? 500;
    if (status >= 500) console.error(`[mock] ${req.method} ${url.pathname} → ${status}`, err);
    sendError(res, status, err.code ?? 'internal_error', err.message ?? '内部错误', requestId, err.details);
  } finally {
    if (process.env.EDGEOS_MOCK_LOG) {
      console.log(`[mock] ${req.method} ${url.pathname} ${now() - started}ms`);
    }
  }
});

function serveStatic(res, pathname) {
  if (!STATIC_DIR) {
    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('未配置 --static，仅提供 API');
    return;
  }
  const safePath = pathname === '/' ? '/index.html' : pathname;
  const target = path.join(STATIC_DIR, path.normalize(safePath).replace(/^([/\\])+/, ''));
  if (!target.startsWith(STATIC_DIR) || !existsSync(target) || !statSync(target).isFile()) {
    // SPA 回退
    const indexFile = path.join(STATIC_DIR, 'index.html');
    if (!existsSync(indexFile)) {
      res.writeHead(404);
      res.end('not found');
      return;
    }
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' });
    createReadStream(indexFile).pipe(res);
    return;
  }
  const ext = path.extname(target);
  const type = ext === '.js' ? 'text/javascript' : ext === '.css' ? 'text/css' : ext === '.html' ? 'text/html' : 'application/octet-stream';
  res.writeHead(200, {
    'Content-Type': `${type}; charset=utf-8`,
    'Cache-Control': ext === '.html' ? 'no-store' : 'public, max-age=31536000, immutable',
  });
  createReadStream(target).pipe(res);
}

function legacyContract() {
  return {
    product_name: registry.productName,
    generated_at: new Date().toISOString(),
    active_tags: [
      { id: 'VOLTAGE_PHASE_A', name: 'VOLTAGE_PHASE_A', attributes: { ui: { label: 'A 相电压', unit: 'V', group: '电气量', precision: 1 } } },
      { id: 'CURRENT_PHASE_A', name: 'CURRENT_PHASE_A', attributes: { ui: { label: 'A 相电流', unit: 'A', group: '电气量', precision: 2 } } },
      { id: 'SOC', name: 'SOC', attributes: { ui: { label: 'SOC', unit: '%', group: '储能', precision: 1 } } },
    ],
    all_tags: [],
    ui_config: { branding: { title: registry.productName, theme: 'dark', logo: '/logo.svg' }, navigation: {} },
  };
}


// ---------------------------------------------------------------- 实时通道（WebSocket）
//
// §6.3：只做服务端 → 客户端的**只读**推送，客户端只能 subscribe / unsubscribe。
// 不引 ws 库（离线设备装不了），按 RFC 6455 手工处理最小子集：
//   - 握手：Sec-WebSocket-Accept
//   - 帧：只接受 text / close / ping，客户端帧必须带掩码
//   - 不支持分片与扩展（客户端发小 JSON，够用；遇到就断开，不猜）
const WS_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';
const wsClients = new Set();
let wsSeq = 0;

function wsAccept(key) {
  return createHashForWs('sha1').update(key + WS_GUID).digest('base64');
}

function wsEncode(text) {
  const payload = Buffer.from(text, 'utf8');
  const length = payload.length;
  let header;
  if (length < 126) {
    header = Buffer.from([0x81, length]);
  } else if (length < 65536) {
    header = Buffer.alloc(4);
    header[0] = 0x81;
    header[1] = 126;
    header.writeUInt16BE(length, 2);
  } else {
    header = Buffer.alloc(10);
    header[0] = 0x81;
    header[1] = 127;
    header.writeBigUInt64BE(BigInt(length), 2);
  }
  return Buffer.concat([header, payload]);
}

/** 从累积缓冲区里切出完整帧；返回 { messages, rest }。 */
function wsDecode(buffer) {
  const messages = [];
  let rest = buffer;
  while (rest.length >= 2) {
    const opcode = rest[0] & 0x0f;
    const masked = (rest[1] & 0x80) !== 0;
    let length = rest[1] & 0x7f;
    let offset = 2;
    if (length === 126) {
      if (rest.length < 4) break;
      length = rest.readUInt16BE(2);
      offset = 4;
    } else if (length === 127) {
      if (rest.length < 10) break;
      length = Number(rest.readBigUInt64BE(2));
      offset = 10;
    }
    if (!masked) {
      messages.push({ opcode, error: 'client frame must be masked' });
      return { messages, rest: Buffer.alloc(0) };
    }
    if (rest.length < offset + 4 + length) break;
    const mask = rest.subarray(offset, offset + 4);
    const payload = Buffer.from(rest.subarray(offset + 4, offset + 4 + length));
    for (let i = 0; i < payload.length; i += 1) payload[i] ^= mask[i % 4];
    messages.push({ opcode, text: payload.toString('utf8') });
    rest = rest.subarray(offset + 4 + length);
  }
  return { messages, rest };
}

function wsSend(client, message) {
  if (client.socket.destroyed) return;
  client.socket.write(wsEncode(JSON.stringify(message)));
}

/** 广播：seq 单调递增，客户端可据此判断是否漏消息（漏了就整体重取）。 */
function broadcast(topic, type, payload) {
  wsSeq += 1;
  const message = { type, seq: wsSeq, ts: new Date().toISOString(), topic, payload };
  for (const client of wsClients) {
    if (client.topics.has(topic)) wsSend(client, message);
  }
}

function handleWsUpgrade(req, socket) {
  const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
  const reject = (status, reason) => {
    socket.write(`HTTP/1.1 ${status} ${reason}\r\nConnection: close\r\n\r\n`);
    socket.destroy();
  };

  if (url.pathname !== '/api/v1/ws') return reject(404, 'Not Found');
  const host = (req.headers.host ?? '').split(':')[0];
  if (!HOST_ALLOWLIST.has(host)) return reject(421, 'Misdirected Request');
  const origin = req.headers.origin;
  if (origin && !HOST_ALLOWLIST.has(new URL(origin).hostname)) return reject(403, 'Forbidden');
  const session = loadSession(req);
  if (!session) return reject(401, 'Unauthorized');
  const key = req.headers['sec-websocket-key'];
  if (!key) return reject(400, 'Bad Request');

  socket.write(
    [
      'HTTP/1.1 101 Switching Protocols',
      'Upgrade: websocket',
      'Connection: Upgrade',
      `Sec-WebSocket-Accept: ${wsAccept(key)}`,
      '\r\n',
    ].join('\r\n'),
  );

  const client = { socket, session, topics: new Set(['system']), buffer: Buffer.alloc(0) };
  wsClients.add(client);
  wsSend(client, { type: 'welcome', seq: wsSeq, ts: new Date().toISOString(), topic: 'system', payload: { topics: [...client.topics] } });

  socket.on('data', (chunk) => {
    client.buffer = Buffer.concat([client.buffer, chunk]);
    const { messages, rest } = wsDecode(client.buffer);
    client.buffer = rest;
    for (const message of messages) {
      if (message.error) {
        socket.destroy();
        return;
      }
      if (message.opcode === 0x8) {
        socket.end();
        return;
      }
      if (message.opcode === 0x9) {
        socket.write(Buffer.from([0x8a, 0x00])); // pong（空载荷）
        continue;
      }
      if (message.opcode !== 0x1) continue;
      handleWsMessage(client, message.text);
    }
  });
  socket.on('close', () => wsClients.delete(client));
  socket.on('error', () => wsClients.delete(client));
}

/** 只接受 subscribe / unsubscribe：这个通道不承载任何写操作（§6.3 规则 4）。 */
function handleWsMessage(client, text) {
  let message;
  try {
    message = JSON.parse(text);
  } catch {
    wsSend(client, { type: 'error', code: 'invalid_json', message: '消息不是合法 JSON' });
    return;
  }
  const allowed = new Set(['alarm', 'task', 'config-transaction', 'telemetry']);
  const topic = message?.topic;
  if (!allowed.has(topic)) {
    wsSend(client, { type: 'error', code: 'unknown_topic', message: `未知订阅主题 ${topic}` });
    return;
  }
  if (message.type === 'subscribe') {
    client.topics.add(topic);
  } else if (message.type === 'unsubscribe') {
    client.topics.delete(topic);
  } else {
    wsSend(client, { type: 'error', code: 'unsupported_type', message: '只支持 subscribe / unsubscribe' });
    return;
  }
  wsSend(client, {
    type: 'subscribed',
    seq: wsSeq,
    ts: new Date().toISOString(),
    topic,
    payload: { topics: [...client.topics] },
  });
}

server.on('upgrade', handleWsUpgrade);

server.listen(PORT, HOST, () => {
  console.log(`[mock] product=${PRODUCT} listening http://${HOST}:${PORT}  registry=${registry.digests.registry.slice(0, 23)}…`);
  console.log(`[mock] routes=${routes.length}  static=${STATIC_DIR ?? '(none)'}`);
});

process.on('SIGTERM', () => server.close(() => process.exit(0)));
process.on('SIGINT', () => server.close(() => process.exit(0)));
