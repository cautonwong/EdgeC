import { expect, test } from '@playwright/test';

/**
 * DCU 平台 E2E：与 iEMS 同一套 Core，只换 Pack 与 release-scope。
 *
 * 这里刻意验证与 iEMS **相反**的一侧：DCU 的 release-scope 启用了 firmwareUpgrade，
 * 所以升级接口在 Registry 里存在——同样一个 POST，iEMS 返回 404（路由不存在），
 * DCU 返回 403（路由存在但没权限）。这一对差异证明裁剪真的按 release-scope 生效。
 */
const VIEWER = { username: 'viewer', password: 'Viewer-Pass-2026' };

async function login(page, account: { username: string; password: string }) {
  await page.goto('/#/login');
  await page.locator('[data-testid="login-username"] input').fill(account.username);
  await page.locator('[data-testid="login-password"] input').fill(account.password);
  await page.locator('[data-testid="login-submit"]').click();
  await expect(page.locator('[data-testid="topbar-user"]')).toBeVisible();
}

test('DCU 菜单按 Pack 与权限生成，电表档案可见', async ({ page }) => {
  await login(page, VIEWER);
  await expect(page.locator('[data-testid="nav-item-dcu.meters"]')).toBeVisible();
  await page.locator('[data-testid="nav-item-dcu.meters"]').click();
  await expect(page.locator('[data-testid="dcu-meters"]')).toBeVisible();
  await expect(page.locator('[data-testid="dcu-meters"]')).toContainText('1# 进线表');
});

test('DCU 启用了升级功能：接口存在但需要 execute 权限', async ({ page }) => {
  await login(page, VIEWER);
  const me = await (await page.request.get('/api/v1/auth/me')).json();
  const response = await page.request.post('/api/v1/system/upgrade', {
    data: {},
    headers: { Origin: 'http://127.0.0.1:5174', 'X-CSRF-Token': me.csrfToken },
  });
  expect(response.status()).toBe(403);
  expect((await response.json()).code).toBe('permission_denied');
});

test('DCU 有蜂窝模块：蜂窝页面可进入（与 iEMS 相反）', async ({ page }) => {
  await login(page, VIEWER);
  // viewer 没有 network.cellular:read，所以仍被权限挡住 —— 但原因必须是 forbidden 而非 unsupported
  await page.goto('/#/network/cellular');
  await expect(page.locator('[data-testid="denied-state"]')).toHaveText('forbidden');
});
