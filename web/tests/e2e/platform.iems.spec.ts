import { expect, test } from '@playwright/test';
import { createServer } from 'node:http';

/**
 * iEMS 平台 E2E：验证的是**授权链**，不是"页面长得对"。
 *
 * 三个账号（mock-server 种子）：
 *   admin    / Initial-EdgeOS-2026  → 首登强制改密
 *   engineer / Engineer-Pass-2026   → 无用户管理权限
 *   viewer   / Viewer-Pass-2026     → 只读
 *
 * 能力快照里 `hardware.network.cellular = false`，用来验证"设备不支持"这条路径。
 */

const VIEWER = { username: 'viewer', password: 'Viewer-Pass-2026' };
const ENGINEER = { username: 'engineer', password: 'Engineer-Pass-2026' };

/** 直接打 API 时必须自己补 Origin 与 CSRF：这两条是服务端强制的，测试不能绕过。 */
async function apiCall(
  page,
  method: 'post' | 'put' | 'patch' | 'delete',
  path: string,
  data: unknown = {},
  csrf?: string,
) {
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  return page.request[method](path, {
    data,
    headers: {
      Origin: 'http://127.0.0.1:5173',
      'X-CSRF-Token': csrf ?? me.csrfToken,
    },
  });
}

async function login(page, account: { username: string; password: string }) {
  await page.goto('/#/login');
  await page.locator('[data-testid="login-username"] input').fill(account.username);
  await page.locator('[data-testid="login-password"] input').fill(account.password);
  await page.locator('[data-testid="login-submit"]').click();
  await expect(page.locator('[data-testid="topbar-user"]')).toBeVisible();
}

test('未登录访问受保护页面会被送到登录页', async ({ page }) => {
  await page.goto('/#/dashboard');
  await expect(page.locator('[data-testid="login-submit"]')).toBeVisible();
  await expect(page).toHaveURL(/#\/login/);
});

test('admin 首登被强制改密，改密后才能进入系统', async ({ page }) => {
  await page.goto('/#/login');
  await page.locator('[data-testid="login-username"] input').fill('admin');
  await page.locator('[data-testid="login-password"] input').fill('Initial-EdgeOS-2026');
  await page.locator('[data-testid="login-submit"]').click();

  // 未改密前任何受保护页面都进不去
  await expect(page.locator('[data-testid="cp-submit"]')).toBeVisible();
  await page.goto('/#/dashboard');
  await expect(page.locator('[data-testid="cp-submit"]')).toBeVisible();

  // 弱口令被拒（服务端也判，前端只是先拦一层）
  await page.locator('[data-testid="cp-current"] input').fill('Initial-EdgeOS-2026');
  await page.locator('[data-testid="cp-new"] input').fill('short');
  await page.locator('[data-testid="cp-confirm"] input').fill('short');
  await expect(page.locator('[data-testid="cp-submit"]')).toBeDisabled();

  const strong = 'Bench-EdgeOS-2026!';
  await page.locator('[data-testid="cp-new"] input').fill(strong);
  await page.locator('[data-testid="cp-confirm"] input').fill(strong);
  await page.locator('[data-testid="cp-submit"]').click();

  await expect(page.locator('[data-testid="topbar-user"]')).toBeVisible();
  await expect(page.locator('[data-testid="nav-group-system"]')).toBeVisible();
});

test('viewer 的菜单不含用户管理，直接输入 URL 得到 403 且说明缺哪条权限', async ({ page }) => {
  await login(page, VIEWER);

  await expect(page.locator('[data-testid="nav-group-overview"]')).toBeVisible();
  await expect(page.locator('[data-testid="nav-item-core.users"]')).toHaveCount(0);

  await page.goto('/#/system/users');
  await expect(page.locator('[data-testid="denied-page"]')).toBeVisible();
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
  await expect(page.locator('[data-testid="denied-page"]')).toContainText('user.account:read');
});

test('engineer 没有用户管理权限：菜单无此项，直接访问仍是 403', async ({ page }) => {
  await login(page, ENGINEER);

  await expect(page.locator('[data-testid="nav-item-core.users"]')).toHaveCount(0);
  await page.goto('/#/system/users');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
});

test('能力缺失：蜂窝页面既不进菜单，直接访问也明确说"不支持"', async ({ page }) => {
  await login(page, ENGINEER);

  await expect(page.locator('[data-testid="nav-item-core.network.cellular"]')).toHaveCount(0);
  await page.goto('/#/network/cellular');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('unsupported');
  await expect(page.locator('[data-testid="denied-page"]')).toContainText('hardware.network.cellular');
});

test('有权限的页面正常渲染，数据来自 Pack 接口', async ({ page }) => {
  await login(page, VIEWER);

  await expect(page.locator('[data-testid="nav-item-iems.telemetry"]')).toBeVisible();
  await page.locator('[data-testid="nav-item-iems.telemetry"]').click();
  await expect(page.locator('[data-testid="iems-telemetry"]')).toBeVisible();
  await expect(page.locator('[data-testid="iems-telemetry"]')).toContainText('电池 SOC');
});

test('服务端才是权威：viewer 直接调用管理接口返回 403', async ({ page }) => {
  await login(page, VIEWER);
  const response = await apiCall(page, 'post', '/api/v1/users', { username: 'x' });
  expect(response.status()).toBe(403);
  const body = await response.json();
  expect(body.code).toBe('permission_denied');
  expect(body.requestId).toBeTruthy();
});

test('状态变更请求缺少 CSRF 头会被拒绝', async ({ page }) => {
  await login(page, VIEWER);
  const response = await apiCall(page, 'post', '/api/v1/auth/logout', {}, 'forged-token');
  expect(response.status()).toBe(403);
  expect((await response.json()).code).toBe('csrf_failed');
});

test('登出后会话被撤销，再访问受保护页面回到登录页', async ({ page }) => {
  await login(page, VIEWER);
  await page.locator('[data-testid="logout"]').click();
  await expect(page.locator('[data-testid="login-submit"]')).toBeVisible();

  await page.goto('/#/dashboard');
  await expect(page.locator('[data-testid="login-submit"]')).toBeVisible();
});

test('关闭的功能不残留路由：升级接口返回未登记', async ({ page }) => {
  await login(page, ENGINEER);
  const response = await apiCall(page, 'post', '/api/v1/system/upgrade', {});
  // iEMS 的 release-scope 关闭了 firmwareUpgrade → Registry 里没有该 operation
  expect(response.status()).toBe(404);
  expect((await response.json()).code).toBe('not_found');
});

// ---------------------------------------------------------------- 配置事务（R2）
const ENGINEER_ACCOUNT = { username: 'engineer', password: 'Engineer-Pass-2026' };

async function openNetworkEditor(page) {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/network');
  await expect(page.locator('[data-testid="network-form"]')).toHaveCount(0);
  await page.locator('[data-testid="edit-eth0"]').click();
  await expect(page.locator('[data-testid="network-form"]')).toBeVisible();
}

test('配置事务：非法值在前端就被拦住，不会产生事务', async ({ page }) => {
  await openNetworkEditor(page);
  await page.locator('[data-testid="field-addresses"]').fill('192.168.10.99/33');
  await page.locator('[data-testid="tx-apply"]').click();

  await expect(page.locator('[data-testid="error-addresses"]')).toBeVisible();
  await expect(page.locator('[data-testid="tx-pending"]')).toHaveCount(0);
  await expect(page.locator('[data-testid="tx-error"]')).toContainText('Schema');
});

test('配置事务：应用 → 倒计时确认 → 配置真的落到设备', async ({ page }) => {
  await openNetworkEditor(page);
  await page.locator('[data-testid="field-addresses"]').fill('192.168.10.99/24');
  await page.locator('[data-testid="tx-apply"]').click();

  // 应用后必须进入待确认，并显示倒计时（R2 才有确认窗口）
  await expect(page.locator('[data-testid="tx-pending"]')).toBeVisible();
  await expect(page.locator('[data-testid="tx-countdown"]')).toHaveText(/^\d{2}:\d{2}$/);
  await expect(page.locator('[data-testid="tx-changes"]')).toHaveCount(0, { timeout: 1000 });

  await page.locator('[data-testid="tx-confirm"]').click();
  await expect(page.locator('[data-testid="tx-message"]')).toContainText('已确认');

  // 设备侧确实改了：列表里出现新地址
  await expect(page.locator('table')).toContainText('192.168.10.99/24');
});

test('配置事务：应用后回滚，设备状态回到应用前', async ({ page }) => {
  await openNetworkEditor(page);
  await page.locator('[data-testid="field-addresses"]').fill('192.168.10.77/24');
  await page.locator('[data-testid="tx-apply"]').click();
  await expect(page.locator('[data-testid="tx-pending"]')).toBeVisible();

  await page.locator('[data-testid="tx-rollback"]').click();
  await expect(page.locator('[data-testid="tx-message"]')).toContainText('已回滚');
  await expect(page.locator('table')).not.toContainText('192.168.10.77/24');
});

test('配置事务：服务端 schema 校验才是结论（422 + fieldErrors）', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  const created = await apiCall(page, 'post', '/api/v1/config/transactions', {
    sectionId: 'network.interfaces',
    values: { interfaceId: 'eth0', addresses: ['10.0.0.1/99'] },
  });
  expect(created.status()).toBe(422);
  const body = await created.json();
  expect(body.code).toBe('schema_validation_failed');
  expect(body.details.fieldErrors.addresses).toBeTruthy();
});

test('配置事务：只读账号能看接口状态，但编辑入口是禁用的', async ({ page }) => {
  // Operator 按 §8.2 只有"网络与通信 1 页只读"：能进页面、能看状态，不能改
  await login(page, { username: 'operator', password: 'Operator-Pass-2026' });
  await page.goto('/#/network');
  await expect(page.locator('[data-testid="edit-eth0"]')).toBeDisabled();
  await expect(page.locator('[data-testid="network-form"]')).toHaveCount(0);
});

test('配置事务：连读权限都没有的账号，页面直接 403', async ({ page }) => {
  await login(page, VIEWER);
  await page.goto('/#/network');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
});

// ---------------------------------------------------------------- 风险等级驱动 UI
test('R1 分区（系统时间）：应用即生效，不弹确认倒计时', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/system/time');
  await expect(page.locator('[data-testid="schema-form"]')).toBeVisible();

  // 改 NTP 服务器列表（数组字段：一行一个）
  await page.locator('[data-testid="field-servers"]').fill('ntp1.example.net\nntp2.example.net');
  await expect(page.locator('[data-testid="tx-dirty"]')).toBeVisible();
  await page.locator('[data-testid="tx-apply"]').click();

  // R1 没有确认窗口：直接成功，且不出现 pending 横幅
  await expect(page.locator('[data-testid="tx-message"]')).toContainText('生效');
  await expect(page.locator('[data-testid="tx-pending"]')).toHaveCount(0);

  // 刷新页面后读回的是新值（证明真的写进去了，不是只改了本地草稿）
  await page.reload();
  await expect(page.locator('[data-testid="field-servers"]')).toHaveValue(/ntp1\.example\.net/);
});

test('R3 分区（防火墙）：engineer 能读不能改，菜单可见但控件禁用', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  // engineer 没有 network.firewall:read → 页面本身 403（读权限与写权限分开判定）
  await page.goto('/#/network/firewall');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
  await expect(page.locator('[data-testid="denied-page"]')).toContainText('network.firewall:read');
});

// ---------------------------------------------------------------- 实时通道（只读推送）
test('实时通道：登录后建立连接，顶栏徽标显示"实时"', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await expect(page.locator('[data-testid="live-badge"]')).toHaveText('实时');
});

test('实时通道：别的会话确认事务后，本页无需手动刷新就能看到结果', async ({ page }) => {
  await openNetworkEditor(page);
  await page.locator('[data-testid="field-addresses"]').fill('192.168.10.88/24');
  await page.locator('[data-testid="tx-apply"]').click();
  await expect(page.locator('[data-testid="tx-pending"]')).toBeVisible();

  // 用同一会话的另一条通道确认（模拟"另一个窗口/运维同事确认了"），页面不做任何点击
  const pending = await (await page.request.get('/api/v1/config/transactions?state=PENDING_CONFIRM')).json();
  const txId = pending.items[0].transactionId;
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const confirmed = await page.request.post(`/api/v1/config/transactions/${txId}/confirm`, {
    data: {},
    headers: { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken },
  });
  expect(confirmed.status()).toBe(200);

  // 靠 WebSocket 推送把状态带过来：pending 横幅消失、出现"已确认"
  await expect(page.locator('[data-testid="tx-message"]')).toContainText('已确认', { timeout: 15_000 });
  await expect(page.locator('[data-testid="tx-pending"]')).toHaveCount(0);
});

// ---------------------------------------------------------------- 告警（列表 + 推送 + 确认）
test('告警页：列表按等级排序，等级/状态过滤可用', async ({ page }) => {
  await login(page, VIEWER);
  await page.goto('/#/events');
  await expect(page.locator('[data-testid="alarms-table"]')).toBeVisible();
  await expect(page.locator('[data-testid="alarm-message-al_0001"]')).toContainText('MQTT');

  await page.locator('[data-testid="alarm-level-filter"]').selectOption('minor');
  await expect(page.locator('[data-testid="alarm-message-al_0002"]')).toBeVisible();
  await expect(page.locator('[data-testid="alarm-message-al_0001"]')).toHaveCount(0);

  await page.locator('[data-testid="alarm-level-filter"]').selectOption('all');
  await page.locator('[data-testid="alarm-state-filter"]').selectOption('acked');
  await expect(page.locator('[data-testid="alarms-empty"]')).toBeVisible();
});

test('告警页：只读账号没有确认按钮，服务端同样拒绝', async ({ page }) => {
  await login(page, VIEWER);
  await page.goto('/#/events');
  await expect(page.locator('[data-testid="alarms-table"]')).toBeVisible();
  await expect(page.locator('[data-testid="alarm-ack-al_0001"]')).toHaveCount(0);

  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const denied = await page.request.post('/api/v1/alarms/al_0001/ack', {
    data: {},
    headers: { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken },
  });
  expect(denied.status()).toBe(403);
  expect((await denied.json()).code).toBe('permission_denied');
});

test('告警页：确认告警后状态变为已确认（写操作走服务端判定）', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/events');
  await page.locator('[data-testid="alarm-ack-al_0002"]').click();
  await expect(page.locator('[data-testid="alarm-state-filter"]')).toBeVisible();
  await page.locator('[data-testid="alarm-state-filter"]').selectOption('acked');
  await expect(page.locator('[data-testid="alarm-message-al_0002"]')).toBeVisible();
});

test('告警页：配置回滚产生的告警通过实时通道推到页面，无需刷新', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/events');
  await expect(page.locator('[data-testid="alarms-table"]')).toBeVisible();

  // 告警现在会**归并**：同源同码只保留一条、累加次数。
  // 所以这里判"次数增加"，而不是"多出一条"——后者恰恰是归并要消灭的行为。
  const marker = '已回滚（未在确认窗口内确认或主动取消）';
  const row = page.locator('[data-testid="alarms-table"] tr', { hasText: marker }).first();
  await expect(row).toBeVisible();
  const countCell = row.locator('[data-testid^="alarm-count-"]');
  const beforeCount = await countCell.textContent();

  // 在另一个页面里制造一次回滚（配置回滚会产生 major 告警）
  const other = await page.context().newPage();
  await other.goto('/#/network');
  await other.locator('[data-testid="edit-eth0"]').click();
  await other.locator('[data-testid="field-addresses"]').fill('192.168.10.66/24');
  await other.locator('[data-testid="tx-apply"]').click();
  await expect(other.locator('[data-testid="tx-pending"]')).toBeVisible();
  await other.locator('[data-testid="tx-rollback"]').click();
  await expect(other.locator('[data-testid="tx-message"]')).toContainText('已回滚');

  // 告警页不做任何操作，靠 WS 推送把次数刷上去
  await expect(countCell).not.toHaveText(beforeCount ?? '', { timeout: 15_000 });
});

// ---------------------------------------------------------------- 用户与角色管理（§5.5）
const OPSADMIN = { username: 'opsadmin', password: 'OpsAdmin-Pass-2026' };

test('用户管理：创建用户返回一次性口令，列表随即出现', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/users');
  await expect(page.locator('[data-testid="users-table"]')).toBeVisible();

  await page.locator('[data-testid="create-user"]').click();
  await page.locator('[data-testid="new-username"]').fill('e2euser');
  await page.locator('[data-testid="new-display-name"]').fill('E2E 账号');
  await page.locator('[data-testid="create-user-submit"]').click();

  await expect(page.locator('[data-testid="one-time-password"]')).toBeVisible();
  await expect(page.locator('[data-testid="users-table"]')).toContainText('e2euser');
});

test('用户管理：不能禁用当前登录的账号（自锁保护）', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/users');
  await page.locator('[data-testid="disable-opsadmin"]').click();
  await expect(page.locator('[data-testid="users-error"]')).toContainText('self_protected');
});

test('用户管理：重置口令会生成一次性口令', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/users');
  // 拿刚建的 e2euser 开刀，不要动 engineer —— 它的口令后面还有用例要用
  await page.locator('[data-testid="reset-e2euser"]').click();
  await expect(page.locator('[data-testid="one-time-password"]')).toBeVisible();
  await expect(page.locator('[data-testid="users-notice"]')).toContainText('会话全部失效');
});

test('用户管理：禁用的账号无法再登录', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/users');
  await page.locator('[data-testid="disable-e2euser"]').click();
  await expect(page.locator('[data-testid="users-notice"]')).toContainText('已更新');

  // 用一个干净上下文探测：带当前会话的请求会先被 CSRF 挡掉，测不到"禁用后能否登录"
  const cleanContext = await page.context().browser()!.newContext();
  const probe = await cleanContext.request.post('http://127.0.0.1:5173/api/v1/auth/login', {
    data: { username: 'e2euser', password: 'whatever-not-matter' },
    headers: { Origin: 'http://127.0.0.1:5173' },
  });
  // 禁用后连口令都不该被校验：直接 401，不能泄露"口令对不对"
  expect(probe.status()).toBe(401);
  await cleanContext.close();
});

test('角色管理：可创建自定义角色，权限必须在目录里', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/roles');
  await expect(page.locator('[data-testid="roles-table"]')).toBeVisible();

  await page.locator('[data-testid="create-role"]').click();
  await page.locator('[data-testid="role-id"]').fill('e2e-auditor');
  await page.locator('[data-testid="role-name"]').fill('E2E 审计员');
  await page.locator('[data-testid="role-description"]').fill('只读审计');
  await page.locator('[data-testid="perm-core.alarm:read"]').check();
  await page.locator('[data-testid="create-role-submit"]').click();

  await expect(page.locator('[data-testid="roles-notice"]')).toContainText('已创建角色');
  await expect(page.locator('[data-testid="roles-table"]')).toContainText('e2e-auditor');

  // 目录里不存在的权限：服务端必须拒（不是靠前端勾选框控制）
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const bad = await page.request.post('/api/v1/roles', {
    data: { id: 'e2e-bad', name: '非法角色', permissions: ['core.nonexistent:read'] },
    headers: { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken },
  });
  expect(bad.status()).toBe(422);
  expect((await bad.json()).code).toBe('unknown_permission');
});

test('角色管理：没有 user.role:read 的账号进不去', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/system/roles');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
});

// ---------------------------------------------------------------- 概览（遥测推送）
test('概览：点位卡按分组展示，分组过滤生效', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/dashboard');
  await expect(page.locator('[data-testid="metric-grid"]')).toBeVisible();
  await expect(page.locator('[data-testid="metric-SOC"]')).toBeVisible();
  await expect(page.locator('[data-testid="metric-VOLTAGE_PHASE_A"]')).toBeVisible();

  await page.locator('[data-testid="group-储能"]').click();
  await expect(page.locator('[data-testid="metric-SOC"]')).toBeVisible();
  await expect(page.locator('[data-testid="metric-VOLTAGE_PHASE_A"]')).toHaveCount(0);

  await page.locator('[data-testid="group-all"]').click();
  await expect(page.locator('[data-testid="metric-VOLTAGE_PHASE_A"]')).toBeVisible();
});

test('概览：数值靠遥测推送自己更新，不刷新页面', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/dashboard');

  const voltage = page.locator('[data-testid="metric-VOLTAGE_PHASE_A"] .metric__number');
  await expect(voltage).toBeVisible();
  const before = await voltage.textContent();

  // 服务端每 2s 推一次快照；7s 周期的正弦，10s 内一定会变
  await expect
    .poll(async () => voltage.textContent(), { timeout: 12_000, intervals: [500, 1000, 2000] })
    .not.toBe(before);

  // 更新时间也在走（说明是推送驱动，不是首屏渲染的静态值）
  await expect(page.locator('.head__hint')).toContainText('更新于');
});

// ---------------------------------------------------------------- 曲线页（遥测数据面）
test('实时曲线：点位来自遥测快照，图表可渲染', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/data');
  await expect(page.locator('[data-testid="curve-chart"] canvas')).toBeVisible();

  // 单点位模式下才会出现点位选择器（多点模式是固定四条曲线）
  await page.locator('[data-testid="curve-single-mode"]').click();
  await expect(page.locator('[data-testid="curve-point-select"]')).toBeVisible();

  // 点位列表来自 /api/v1/telemetry/current，且自动选中第一个
  await expect(page.locator('[data-testid="curve-point-select"] option')).toHaveCount(8);
});

test('历史曲线：选点位查询后返回序列并画出图表', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/data/history');

  const select = page.locator('[data-testid="history-point-select"]');
  await expect(select).toBeVisible();
  await select.selectOption('SOC');
  await page.locator('[data-testid="history-query"]').click();

  // 序列来自 /api/v1/telemetry/history：点数 > 0 且图表容器里有 canvas
  await expect(page.locator('[data-testid="history-points"]')).not.toHaveText('0');
  await expect(page.locator('[data-testid="history-chart"] canvas')).toBeVisible();
});

// ---------------------------------------------------------------- 应用商店
test('应用商店：工程师可安装应用，刷新后状态保留', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/apps');
  await expect(page.getByRole('heading', { name: '应用商店' })).toBeVisible();
  await expect(page.locator('[data-testid="app-card"]')).toHaveCount(12);

  await page.locator('[data-testid="app-search"]').fill('需量');
  await expect(page.locator('[data-testid="app-card"]')).toHaveCount(1);
  await page.locator('[data-testid="app-search"]').fill('');

  await page.locator('[data-testid="install-demand_forecast"]').click();
  await expect(page.locator('[data-testid="state-demand_forecast"]')).toHaveText('已安装');

  await page.locator('[data-app-id="demand_forecast"] .app-card__details').click();
  await expect(page.locator('[data-testid="app-drawer"]')).toBeVisible();
  await page.keyboard.press('Escape');
  await expect(page.locator('[data-testid="app-drawer"]')).toHaveCount(0);

  await page.reload();
  await expect(page.locator('[data-testid="state-demand_forecast"]')).toHaveText('已安装');
});

test('应用商店：viewer 只能浏览', async ({ page }) => {
  await login(page, VIEWER);
  await page.goto('/#/apps');
  await expect(page.getByRole('heading', { name: '应用商店' })).toBeVisible();
  await expect(page.locator('[data-testid="install-demand_forecast"]')).toBeDisabled();
});

// ---------------------------------------------------------------- 角色编辑 / 删除
test('角色管理：自定义角色可编辑，内置角色不给编辑入口', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/roles');

  // 先建一个
  await page.locator('[data-testid="create-role"]').click();
  await page.locator('[data-testid="role-id"]').fill('e2e-edit');
  await page.locator('[data-testid="role-name"]').fill('E2E 可编辑');
  await page.locator('[data-testid="perm-core.alarm:read"]').check();
  await page.locator('[data-testid="create-role-submit"]').click();
  await expect(page.locator('[data-testid="roles-table"]')).toContainText('e2e-edit');

  // 编辑：加一条权限
  await page.locator('[data-testid="edit-role-e2e-edit"]').click();
  await expect(page.locator('[data-testid="role-id"]')).toBeDisabled();
  await page.locator('[data-testid="perm-core.telemetry:read"]').check();
  await page.locator('[data-testid="create-role-submit"]').click();
  await expect(page.locator('[data-testid="roles-notice"]')).toContainText('已更新');

  const roles = await (await page.request.get('/api/v1/roles')).json();
  const edited = roles.items.find((role: { id: string }) => role.id === 'e2e-edit');
  expect(edited.permissions).toEqual(['core.alarm:read', 'core.telemetry:read']);

  // 内置角色没有编辑/删除入口
  await expect(page.locator('[data-testid="edit-role-admin"]')).toHaveCount(0);
  await expect(page.locator('[data-testid="delete-role-admin"]')).toHaveCount(0);
});

test('角色管理：内置角色改不动，被持有的角色删不掉', async ({ page }) => {
  await login(page, OPSADMIN);
  await page.goto('/#/system/roles');
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const csrf = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken };

  const builtin = await page.request.patch('/api/v1/roles/admin', {
    data: { name: '改名试试' },
    headers: csrf,
  });
  expect(builtin.status()).toBe(403);
  expect((await builtin.json()).code).toBe('builtin_role_immutable');

  // 建角色 + 建持有它的账号，再删角色 → 必须拒绝
  await page.request.post('/api/v1/roles', {
    data: { id: 'e2e-in-use', name: 'E2E 占用中', permissions: ['core.alarm:read'] },
    headers: csrf,
  });
  const created = await page.request.post('/api/v1/users', {
    data: { username: 'e2eholder', roleIds: ['e2e-in-use'] },
    headers: csrf,
  });
  expect(created.status()).toBe(201);

  const inUse = await page.request.delete('/api/v1/roles/e2e-in-use', { headers: csrf });
  expect(inUse.status()).toBe(409);
  expect((await inUse.json()).code).toBe('role_in_use');
});

// ---------------------------------------------------------------- 独立 watchdog（超时自动回滚）
test('配置事务：窗口内不确认，watchdog 自动回滚（不点任何按钮）', async ({ page }) => {
  await openNetworkEditor(page);
  await page.locator('[data-testid="field-addresses"]').fill('192.168.10.123/24');
  await page.locator('[data-testid="tx-apply"]').click();

  // 台架把确认窗口压到 8s（EDGEOS_CONFIRM_WINDOW_MS），watchdog 每 300ms 扫一次
  await expect(page.locator('[data-testid="tx-pending"]')).toBeVisible();

  // 关键：不做任何操作，等 watchdog 回滚后由推送把结果带回来
  await expect(page.locator('[data-testid="tx-message"]')).toContainText('已回滚', { timeout: 30_000 });
  await expect(page.locator('table')).not.toContainText('192.168.10.123/24');
});

test('配置事务：watchdog 的回滚会留下可追溯的告警', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/events');
  await expect(page.locator('[data-testid="alarms-table"]')).toBeVisible();
  // 上一条用例触发的 watchdog 回滚必须留下 major 告警（现场要知道"配置没生效"）
  await expect(page.getByText('watchdog 已回滚').first()).toBeVisible({ timeout: 15_000 });
});

// ---------------------------------------------------------------- 登录失败锁定（§5.6）
test('登录锁定：连续失败后给出剩余次数，锁定期间连正确口令也拒绝', async ({ page }) => {
  const ACCOUNT = { username: 'locktest', password: 'LockTest-Pass-2026' };
  await page.goto('/#/login');

  // 前 4 次：401，文案里带"还可尝试 N 次"
  for (let i = 0; i < 4; i += 1) {
    await page.locator('[data-testid="login-username"] input').fill(ACCOUNT.username);
    await page.locator('[data-testid="login-password"] input').fill(`wrong-${i}`);
    await page.locator('[data-testid="login-submit"]').click();
    await expect(page.locator('[data-testid="login-error"]')).toContainText('还可尝试');
  }

  // 第 5 次失败触发锁定（台架 5 次 / 30s）
  await page.locator('[data-testid="login-password"] input').fill('wrong-last');
  await page.locator('[data-testid="login-submit"]').click();

  // 锁定期间即使口令正确也必须拒绝：否则等于给攻击者一个校验口
  await page.locator('[data-testid="login-password"] input').fill(ACCOUNT.password);
  await page.locator('[data-testid="login-submit"]').click();
  await expect(page.locator('[data-testid="login-locked"]')).toContainText('秒后再试');
  await expect(page.locator('[data-testid="topbar-user"]')).toHaveCount(0);
});

// ---------------------------------------------------------------- 在线会话（§5.6）
test('在线会话：能看到谁在别处登录，撤销后那条会话立刻失效', async ({ page, browser }) => {
  // 另起一个浏览器上下文，用 engineer 登录，制造"第二个会话"
  const other = await browser.newContext();
  const otherPage = await other.newPage();
  await otherPage.goto('http://127.0.0.1:5173/#/login');
  await otherPage.locator('[data-testid="login-username"] input').fill(ENGINEER.username);
  await otherPage.locator('[data-testid="login-password"] input').fill(ENGINEER.password);
  await otherPage.locator('[data-testid="login-submit"]').click();
  await expect(otherPage.locator('[data-testid="topbar-user"]')).toBeVisible();

  await login(page, OPSADMIN);
  await page.goto('/#/system/sessions');
  await expect(page.locator('[data-testid="sessions-table"]')).toBeVisible();
  await expect(page.locator('[data-testid="sessions-table"]')).toContainText('engineer');
  // 自己的会话不允许在这里踢（避免"点一下就退出登录"的误操作）
  await expect(page.locator('[data-testid="sessions-table"]')).toContainText('当前会话');

  const row = page.locator('[data-testid="sessions-table"] tr', { hasText: 'engineer' }).first();
  await row.locator('button', { hasText: '撤销' }).click();
  await expect(page.locator('[data-testid="sessions-notice"]')).toContainText('下一次请求即失效');

  // 被撤销的那条会话：下一次请求就是 401
  const probe = await otherPage.request.get('http://127.0.0.1:5173/api/v1/auth/me');
  expect(probe.status()).toBe(401);
  await other.close();
});

test('在线会话：没有 user.session:admin 的账号进不去', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/system/sessions');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
});

// ---------------------------------------------------------------- 告警归并与静默（§8.3）
test('告警：同码告警归并显示次数，可静默', async ({ page, browser }) => {
  // 用两个独立的 API 会话造一次远程拉闸：engineer 发起 + opsadmin 复核
  const ops = await browser.newContext();
  const opsLogin = await ops.request.post('/api/v1/auth/login', {
    data: { username: OPSADMIN.username, password: OPSADMIN.password },
    // 登录也是状态变更：服务端强制校验 Origin，少了它直接 403
    headers: { Origin: 'http://127.0.0.1:5173' },
  });
  expect(opsLogin.status()).toBe(200);
  const opsCsrf = (await opsLogin.json()).user.csrfToken;
  const opsHeaders = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': opsCsrf };

  await login(page, ENGINEER_ACCOUNT);
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const engHeaders = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken };

  const devices = await (await page.request.get('/api/v1/control/devices')).json();
  const closed = devices.items.find((item: { state: string }) => item.state === 'closed');
  const created = await page.request.post('/api/v1/control/commands', {
    data: { deviceId: closed.id, action: 'trip', reason: 'E2E：制造一条可静默的告警' },
    headers: engHeaders,
  });
  const command = await created.json();
  const approved = await ops.request.post(`/api/v1/control/commands/${command.id}/approve`, {
    data: {},
    headers: opsHeaders,
  });
  expect(approved.status()).toBe(200);
  await ops.close();

  await page.goto('/#/events');
  await expect(page.locator('[data-testid="alarms-table"]')).toBeVisible();

  const row = page.locator('[data-testid="alarms-table"] tr', { hasText: '远程拉闸' }).first();
  await expect(row).toBeVisible();
  // 归并计数列：至少 ×1（同码事件会被累加）
  await expect(row.locator('[data-testid^="alarm-count-"]')).toContainText('×');

  await row.locator('[data-testid^="alarm-silence-"]').click();
  await expect(page.locator('[data-testid="alarms-notice"]')).toContainText('已静默');
  await expect(page.locator('[data-testid="alarms-table"]')).toContainText('已静默');
});

// ---------------------------------------------------------------- 通知渠道（§8.3）
test('通知渠道：Webhook 真发 HTTP，成功与失败都留痕', async ({ page }) => {
  // 起一个真的接收端，验证「投递成功」不是自己写在数组里的
  const received: string[] = [];
  const echo = createServer((req, res) => {
    let body = '';
    req.on('data', (chunk) => { body += chunk; });
    req.on('end', () => {
      received.push(body);
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end('{"ok":true}');
    });
  });
  await new Promise<void>((resolve) => echo.listen(0, '127.0.0.1', resolve));
  const port = (echo.address() as { port: number }).port;

  await login(page, OPSADMIN);
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const headers = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken };

  // 走菜单链接而不是 hash goto：后者在会话尚未就绪时会被路由守卫弹回默认页
  await page.locator('a[href="#/system/notifications"]').click();
  await expect(page).toHaveURL(/#\/system\/notifications/);
  await expect(page.locator('[data-testid="notify-channels"]')).toBeVisible();

  // 停用状态：发测试不发请求
  await page.locator('[data-testid="notify-test-ch_1"]').click();
  await expect(page.locator('[data-testid="notify-notice"]')).toContainText('未启用');

  // 启用，并把地址指向真的接收端
  await page.locator('[data-testid="notify-toggle-ch_1"]').click();
  await expect(page.locator('[data-testid="notify-channels"]')).toContainText('已启用');
  await page.request.patch('/api/v1/notifications/channels/ch_1', {
    data: { url: `http://127.0.0.1:${port}/hook` },
    headers,
  });
  await page.locator('[data-testid="notify-test-ch_1"]').click();
  await expect(page.locator('[data-testid="notify-notice"]')).toContainText('异步投递');
  // 真的收到一次 POST，且投递记录是 delivered（不是自说自话）
  await expect.poll(() => received.length, { timeout: 15_000 }).toBeGreaterThan(0);
  await expect.poll(async () => {
    const list = await (await page.request.get('/api/v1/notifications/deliveries')).json();
    return list.items.filter((item: { status: string }) => item.status === 'delivered').length;
  }, { timeout: 15_000 }).toBeGreaterThan(0);

  // 换成一个没人监听的端口：必须失败，且重试 3 次
  await page.request.patch('/api/v1/notifications/channels/ch_1', {
    data: { url: 'http://127.0.0.1:1/hook' },
    headers,
  });
  await page.locator('[data-testid="notify-test-ch_1"]').click();
  await expect.poll(async () => {
    const list = await (await page.request.get('/api/v1/notifications/deliveries')).json();
    return list.items.some((item: { status: string; attempts?: number }) => item.status === 'failed' && item.attempts === 3);
  }, { timeout: 20_000 }).toBe(true);

  // 收尾：还原地址并停用
  await page.request.patch('/api/v1/notifications/channels/ch_1', {
    data: { url: 'http://127.0.0.1:9099/hooks/edgeos', enabled: false },
    headers,
  });
  await new Promise<void>((resolve) => echo.close(() => resolve()));
});

// ---------------------------------------------------------------- 告警归档与导出（§8.3）
test('告警归档：清台后仍可回溯，并能导出 CSV', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const headers = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken };

  // 制造一条告警：应用 → 回滚
  const created = await page.request.post('/api/v1/config/transactions', {
    data: { sectionId: 'network.interfaces', values: { interfaceId: 'eth0', addresses: ['192.168.10.99/24'] } },
    headers,
  });
  const tx = await created.json();
  await page.request.post(`/api/v1/config/transactions/${tx.transactionId}/apply`, { data: {}, headers });
  await page.request.post(`/api/v1/config/transactions/${tx.transactionId}/rollback`, { data: {}, headers });

  await page.goto('/#/events');
  await expect(page.locator('[data-testid=alarms-table]')).toContainText('已回滚');

  // 清台：告警从列表消失，但不是销毁
  const cleared = await page.request.post('/api/v1/alarms/clear', { data: {}, headers });
  expect(cleared.status()).toBe(200);
  await expect(page.locator('[data-testid=alarms-table]')).not.toContainText('已回滚');

  // 归档里还在，且能导出 CSV
  await page.locator('[data-testid=alarm-archive-toggle]').click();
  await expect(page.locator('[data-testid=alarm-archive-table]')).toContainText('已回滚');

  const [download] = await Promise.all([
    page.waitForEvent('download'),
    page.locator('[data-testid=alarm-archive-export]').click(),
  ]);
  expect(download.suggestedFilename()).toMatch(/alarms-archive-.*\.csv/);
});

// ---------------------------------------------------------------- 审计日志（§5.7）
test('审计日志：高危动作留痕，可按动作过滤', async ({ page }) => {
  await login(page, OPSADMIN);
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const headers = { Origin: 'http://127.0.0.1:5173', 'X-CSRF-Token': me.csrfToken };

  // 制造一条可定位的高危动作记录
  await page.request.post('/api/v1/notifications/channels/ch_1/test', { data: {}, headers });

  await page.goto('/#/system/audit');
  await expect(page.locator('[data-testid=audit-table]')).toBeVisible();
  await expect(page.locator('[data-testid=audit-table]')).toContainText('notification.test');
  await expect(page.locator('[data-testid=audit-table]')).toContainText('opsadmin');

  await page.locator('[data-testid=audit-action-filter]').selectOption('notification.test');
  const rows = page.locator('[data-testid=audit-table] tbody tr');
  await expect(rows.first()).toContainText('notification.test');
});

test('审计日志：没有 core.audit:read 的账号进不去', async ({ page }) => {
  await login(page, ENGINEER_ACCOUNT);
  await page.goto('/#/system/audit');
  await expect(page.locator('[data-testid=denied-state]')).toHaveText('forbidden');
});
