import { spawn } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';

/**
 * 起一个**独立的** watchdog 进程，盯着 iEMS 台架的状态文件。
 *
 * 为什么放在 globalSetup：它没有 HTTP 端口，Playwright 的 webServer 拉不起来。
 * 状态文件由 mock-server 在 webServer 阶段创建（命令里先清空），这里只负责看门狗本身。
 */
const STATE = path.resolve(process.cwd(), 'test-results/mock/iems-state.json');
const PID_FILE = path.resolve(process.cwd(), 'test-results/mock/watchdog.pid');

export default async function globalSetup(): Promise<void> {
  mkdirSync(path.dirname(STATE), { recursive: true });

  const child = spawn(process.execPath, ['mock-server/watchdog.mjs', '--state', STATE], {
    cwd: process.cwd(),
    // 300ms 一轮：E2E 要等"超时自动回滚"，窗口压到 8s，扫描也得跟着密一点
    env: { ...process.env, EDGEOS_WATCHDOG_INTERVAL_MS: '300' },
    stdio: 'ignore',
    detached: true,
  });
  child.unref();
  writeFileSync(PID_FILE, String(child.pid ?? ''));
  console.log(`[e2e] watchdog pid=${child.pid} state=${STATE}`);
}
