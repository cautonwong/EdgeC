import { defineConfig, devices } from '@playwright/test';

/**
 * WebUI 平台 E2E。
 *
 * 跑的是**真实浏览器 + 真实 mock-server（webd 的行为替身）**：登录、强制改密、
 * 菜单按权限/能力呈现、直接 URL 的 403 / 不支持、登出撤销、release-scope 裁剪。
 * 判据落在服务端可见的事实上（页面状态 + 接口拒绝码），不靠"元素看起来对"。
 *
 * 旧页面的历史用例放在 tests/e2e/legacy/ 并在 testIgnore 中排除：它们依赖已删除的
 * AdminLayout 引导流程，随 WBUI-072 页面迁移逐条重录，不在这里假装通过。
 */
export default defineConfig({
  testDir: './tests/e2e',
  // 独立 watchdog 没有 HTTP 端口，只能自己起（见 global-setup）
  globalSetup: './tests/e2e/global-setup.ts',
  globalTeardown: './tests/e2e/global-teardown.ts',
  testIgnore: ['legacy/**'],
  timeout: 60_000,
  fullyParallel: false,
  forbidOnly: !!process.env.CI,
  retries: 0,
  workers: 1,
  reporter: [['list'], ['html', { outputFolder: 'playwright-report', open: 'never' }]],
  use: {
    // 台架只装了完整 chromium，没有 headless shell；固定 channel 避免"环境缺浏览器"被当成用例失败
    channel: 'chromium',
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
    video: 'retain-on-failure',
    viewport: { width: 1440, height: 900 },
  },
  projects: [
    {
      name: 'iems',
      use: { ...devices['Desktop Chrome'], baseURL: 'http://127.0.0.1:5173' },
      testMatch: /platform\.iems\.spec\.ts/,
    },
    {
      name: 'ai',
      use: { ...devices['Desktop Chrome'], baseURL: 'http://127.0.0.1:5173' },
      testMatch: /ai_copilot\.spec\.ts/,
    },
    {
      name: 'dcu',
      use: { ...devices['Desktop Chrome'], baseURL: 'http://127.0.0.1:5175' },
      testMatch: /platform\.dcu\.spec\.ts/,
    },
    {
      name: 'smartbox',
      use: { ...devices['Desktop Chrome'], baseURL: 'http://127.0.0.1:5174' },
      testMatch: /smartbox\.spec\.ts/,
    },
  ],
  webServer: [
    {
      // 状态落盘 + 压短确认窗口：这样 E2E 能真的等一次"超时自动回滚"（8s），
      // 而不是只验"点了确认按钮"。状态文件每轮先清掉，避免上一轮的事务残留。
      command:
        'mkdir -p test-results/mock && rm -f test-results/mock/iems-state.json && ' +
        'EDGEOS_MOCK_STATE=test-results/mock/iems-state.json EDGEOS_CONFIRM_WINDOW_MS=8000 ' +
        'node mock-server/server.mjs --product iems --port 8081',
      url: 'http://127.0.0.1:8081/api/v1/system/info',
      reuseExistingServer: false,
      timeout: 60_000,
    },
    {
      command: 'node mock-server/server.mjs --product dcu --port 8082',
      url: 'http://127.0.0.1:8082/api/v1/system/info',
      reuseExistingServer: false,
      timeout: 60_000,
    },
    {
      command: 'npm run dev --prefix products/iems -- --port 5173 --host 127.0.0.1',
      url: 'http://127.0.0.1:5173',
      reuseExistingServer: !process.env.CI,
      timeout: 120_000,
    },
    {
      command: 'EDGEOS_API_TARGET=http://127.0.0.1:8082 npm run dev --prefix products/dcu -- --port 5175 --host 127.0.0.1',
      url: 'http://127.0.0.1:5175',
      reuseExistingServer: !process.env.CI,
      timeout: 120_000,
    },
    {
      command: 'npm run dev --prefix products/smart_box -- --port 5174 --host 127.0.0.1',
      url: 'http://127.0.0.1:5174',
      reuseExistingServer: !process.env.CI,
      timeout: 120_000,
    },
  ],
});
