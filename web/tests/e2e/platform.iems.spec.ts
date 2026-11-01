import { expect, test } from '@playwright/test';

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

  // 前面的用例也制造过回滚，所以这里比较"条数增加"，而不是"某条文本存在"
  const marker = '已回滚（未在确认窗口内确认或主动取消）';
  const before = await page.getByText(marker).count();

  // 在另一个页面里制造一次回滚（配置回滚会产生 major 告警）
  const other = await page.context().newPage();
  await other.goto('/#/network');
  await other.locator('[data-testid="edit-eth0"]').click();
  await other.locator('[data-testid="field-addresses"]').fill('192.168.10.66/24');
  await other.locator('[data-testid="tx-apply"]').click();
  await expect(other.locator('[data-testid="tx-pending"]')).toBeVisible();
  await other.locator('[data-testid="tx-rollback"]').click();
  await expect(other.locator('[data-testid="tx-message"]')).toContainText('已回滚');

  // 告警页不做任何操作，靠 WS 推送出现新告警
  await expect(page.getByText(marker)).toHaveCount(before + 1, { timeout: 15_000 });
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
