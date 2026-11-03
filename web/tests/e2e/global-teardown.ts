import { readFileSync } from 'node:fs';
import path from 'node:path';

const PID_FILE = path.resolve(process.cwd(), 'test-results/mock/watchdog.pid');

/** 收掉 globalSetup 起的 watchdog，别让它跨轮次残留。 */
export default async function globalTeardown(): Promise<void> {
  let pid = 0;
  try {
    pid = Number(readFileSync(PID_FILE, 'utf8').trim());
  } catch {
    return;
  }
  if (!Number.isInteger(pid) || pid <= 0) return;
  try {
    process.kill(pid, 'SIGTERM');
    console.log(`[e2e] watchdog pid=${pid} stopped`);
  } catch {
    // 已经退出就算了
  }
}
