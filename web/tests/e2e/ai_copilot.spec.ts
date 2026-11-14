import { expect, test, type Page } from '@playwright/test';

/**
 * AI 副驾浏览器级 E2E。
 *
 * 判据落在服务端可见的事实上：快捷提示必须真的发出预期工具的 tools/call，
 * 工具目录必须来自服务端 tools/list，而不是前端写死的静态列表。
 *
 * 浏览器里没有 8089 的 edge-mcp-server，WebSocket 会失败，服务走 HTTP 回退；
 * 这里拦截 HTTP 回退并返回 JSON-RPC 响应，CORS 预检也一并处理。
 */

const VIEWER = { username: 'viewer', password: 'Viewer-Pass-2026' };

const CORS = {
  'access-control-allow-origin': '*',
  'access-control-allow-methods': 'POST, OPTIONS',
  'access-control-allow-headers': 'content-type',
};

const TOOLS = [
  { name: 'datahub_read_tags', description: 'Read measurements by tag id.', inputSchema: { type: 'object' } },
  { name: 'datahub_scan_abnormal', description: 'Scan all tags for abnormal quality.', inputSchema: { type: 'object' } },
  { name: 'dcu_analyze_power_quality', description: 'Analyze three-phase unbalance and THD.', inputSchema: { type: 'object' } },
  { name: 'smartbox_eval_transformer_life', description: 'Estimate transformer hot-spot life.', inputSchema: { type: 'object' } },
  { name: 'iems_optimize_tou_schedule', description: 'Optimize ESS charge and discharge.', inputSchema: { type: 'object' } },
  { name: 'security_simulate_interlock', description: 'Check five-anti interlock rules.', inputSchema: { type: 'object' } },
  { name: 'edge_mock_probe', description: 'Server-only catalog probe.', inputSchema: { type: 'object' } },
];

function toolResult(name: string): unknown {
  switch (name) {
    case 'datahub_read_tags':
      return {
        tags: [
          { tag_id: 0x1001, value: 220.5 },
          { tag_id: 0x1002, value: 219.8 },
          { tag_id: 0x1003, value: 221.2 },
        ],
      };
    case 'datahub_scan_abnormal':
      return { scanned: 128, abnormal: [] };
    case 'dcu_analyze_power_quality':
      return { unbalance_pct: 0.41, thd_pct: 3.5, advice: 'balanced' };
    case 'smartbox_eval_transformer_life':
      return { hot_spot_c: 61, remaining_life_years: 18.5 };
    case 'iems_optimize_tou_schedule':
      return { schedule: [{ hour: 2, action: 'charge' }, { hour: 18, action: 'discharge' }] };
    case 'security_simulate_interlock':
      return { allowed: true, rule: 'breaker-open-before-earth' };
    default:
      return { ok: true };
  }
}

async function mockMcp(page: Page): Promise<void> {
  await page.route('**/api/v1/mcp/call', async (route) => {
    if (route.request().method() === 'OPTIONS') {
      await route.fulfill({ status: 204, headers: CORS });
      return;
    }
    const body = route.request().postDataJSON();
    let result: unknown = {};
    if (body?.method === 'tools/list') {
      result = { tools: TOOLS };
    } else if (body?.method === 'tools/call') {
      result = toolResult(body?.params?.name);
    }
    await route.fulfill({
      status: 200,
      headers: { ...CORS, 'content-type': 'application/json' },
      body: JSON.stringify({ jsonrpc: '2.0', id: body?.id ?? 1, result }),
    });
  });
}

async function login(page: Page): Promise<void> {
  await page.goto('/#/login');
  await page.locator('[data-testid="login-username"] input').fill(VIEWER.username);
  await page.locator('[data-testid="login-password"] input').fill(VIEWER.password);
  await page.locator('[data-testid="login-submit"]').click();
  await expect(page.locator('[data-testid="topbar-user"]')).toBeVisible();
}

async function openCopilot(page: Page): Promise<void> {
  await page.locator('[data-testid="toggle-copilot"]').click();
  await expect(page.getByRole('dialog', { name: 'AI 副驾' })).toBeVisible();
}

async function runQuickPrompt(page: Page, label: string, tool: string): Promise<void> {
  await openCopilot(page);
  await page.locator('button', { hasText: label }).click();
  await expect(
    page.locator(`[data-testid="mcp-tool-call"][data-tool-name="${tool}"][data-status="success"]`),
  ).toBeVisible({ timeout: 15_000 });
}

test('快捷提示 1：全站异常扫描调用 datahub_scan_abnormal', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await runQuickPrompt(page, '全站异常扫描', 'datahub_scan_abnormal');
});

test('快捷提示 2：储能峰谷策略调用 iems_optimize_tou_schedule', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await runQuickPrompt(page, '储能峰谷策略', 'iems_optimize_tou_schedule');
});

test('快捷提示 3：变压器寿命评估调用 smartbox_eval_transformer_life', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await runQuickPrompt(page, '变压器寿命评估', 'smartbox_eval_transformer_life');
});

test('快捷提示 4：五防操作校验调用 security_simulate_interlock', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await runQuickPrompt(page, '五防操作校验', 'security_simulate_interlock');
});

test('快捷提示 5：三相电能质量调用 dcu_analyze_power_quality', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await runQuickPrompt(page, '三相电能质量', 'dcu_analyze_power_quality');
});

test('工具目录来自服务端 tools/list，刷新后显示服务端条目', async ({ page }) => {
  await mockMcp(page);
  await login(page);
  await openCopilot(page);
  await page.getByTitle('Provider & Model Settings').click();
  await page.locator('[data-testid="refresh-tools"]').click();
  await expect(page.locator('[data-testid="tool-catalog-source"]')).toHaveText('服务端 tools/list');
  await expect(page.locator('[data-testid="mcp-tool-catalog"]')).toContainText('edge_mock_probe');
});
