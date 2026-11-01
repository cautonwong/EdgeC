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
