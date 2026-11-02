#!/usr/bin/env node
/**
 * 配置事务的**独立** watchdog（§6.4）。
 *
 * 为什么必须独立：确认窗口的承诺是"超时一定回滚"，而回滚不能依赖
 *   - 浏览器（用户可能直接关掉），
 *   - API 进程（可能被重启/OOM 杀掉）。
 * 所以它只看状态文件里的 deadline，进程自己跑自己的；谁还活着不重要，到点就回滚。
 *
 * 用法：
 *   node mock-server/watchdog.mjs --state /tmp/edgeos-state.json            # 常驻，1s 一轮
 *   node mock-server/watchdog.mjs --state /tmp/edgeos-state.json --once     # 只跑一轮（测试用）
 *
 * 退出码：0 正常；2 状态文件不存在（还没人写过状态，不算错误）；1 其他异常。
 */
import { existsSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import { loadState, saveState } from './state.mjs';

function rollback(state, transaction, nowIso) {
  // 恢复 apply 前的快照：接口与其它分区各一份
  if (Array.isArray(transaction.snapshot) && Array.isArray(state.interfaces)) {
    state.interfaces = transaction.snapshot;
  }
  if (transaction.sectionSnapshot && state.sections) {
    state.sections[transaction.sectionId] = transaction.sectionSnapshot;
  }
  transaction.state = 'ROLLED_BACK';
  transaction.rolledBackAt = nowIso;
  transaction.rolledBackBy = 'watchdog';

  state.alarms = state.alarms ?? [];
  const seq = (state.alarms[0]?.id?.replace(/\D/g, '') ?? '0');
  state.alarms.unshift({
    id: `al_wd_${String(Number(seq) + 1).padStart(4, '0')}`,
    ts: nowIso,
    level: 'major',
    source: 'watchdog',
    code: 'CFG-WATCHDOG-ROLLBACK',
    message: `配置分区 ${transaction.sectionId} 在确认窗口内未确认，watchdog 已回滚`,
    state: 'active',
    ackBy: null,
    ackAt: null,
  });
}

/** 一轮：只处理"已过 deadline 且仍在 PENDING_CONFIRM"的事务。返回处理条数。 */
export function sweep(state, nowMs = Date.now()) {
  const transactions = state?.transactions ?? [];
  let handled = 0;
  for (const transaction of transactions) {
    if (transaction.state !== 'PENDING_CONFIRM') continue;
    const deadline = transaction.deadline ? Date.parse(transaction.deadline) : NaN;
    if (!Number.isFinite(deadline) || deadline > nowMs) continue;
    rollback(state, transaction, new Date(nowMs).toISOString());
    handled += 1;
    console.log(`[watchdog] rolled back ${transaction.id} (${transaction.sectionId}) at ${new Date(nowMs).toISOString()}`);
  }
  return handled;
}

function main(argv) {
  const args = argv.slice(2);
  const stateFile = (() => {
    const index = args.indexOf('--state');
    if (index >= 0 && args[index + 1]) return args[index + 1];
    return process.env.EDGEOS_MOCK_STATE ?? '';
  })();
  const once = args.includes('--once');
  const intervalMs = Number(process.env.EDGEOS_WATCHDOG_INTERVAL_MS ?? 1000);

  if (!stateFile) {
    console.error('[watchdog] 缺少 --state <path> 或 EDGEOS_MOCK_STATE');
    process.exit(1);
  }

  const tick = () => {
    if (!existsSync(stateFile)) return 0;
    const state = loadState(stateFile);
    if (!state) return 0;
    const handled = sweep(state);
    if (handled > 0) saveState(stateFile, state);
    return handled;
  };

  if (once) {
    const handled = tick();
    console.log(`[watchdog] once: ${handled} transaction(s) rolled back`);
    process.exit(0);
  }

  console.log(`[watchdog] watching ${stateFile} every ${intervalMs}ms`);
  setInterval(() => {
    try {
      tick();
    } catch (err) {
      // 单轮失败不该让 watchdog 死掉：下一轮继续（日志留痕）
      console.error('[watchdog] tick failed:', err?.message ?? err);
    }
  }, intervalMs);
}

// 只有直接执行时才跑 CLI：被测试 import 时不能去读 argv
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main(process.argv);
}
