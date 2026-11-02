/**
 * 台架状态持久化（opt-in）。
 *
 * 为什么要文件：`mock-server` 是单进程内存态，重启就全没了，独立 watchdog 也没法跨进程看。
 * 只有设置 `EDGEOS_MOCK_STATE=<path>` 时才落盘——默认仍是纯内存，免得 39 条 E2E 之间互相污染。
 */
import { existsSync, readFileSync, writeFileSync } from 'node:fs';

export const STATE_VERSION = 1;

export function loadState(file) {
  if (!file || !existsSync(file)) return null;
  try {
    const data = JSON.parse(readFileSync(file, 'utf8'));
    if (data?.version !== STATE_VERSION) return null;
    return data;
  } catch {
    // 半截文件（上一轮被 kill）当作没有状态：宁可从头开始，也不要拿坏状态去回滚
    return null;
  }
}

export function saveState(file, snapshot) {
  if (!file) return;
  const payload = { version: STATE_VERSION, savedAt: new Date().toISOString(), ...snapshot };
  // 先写临时文件再 rename：watchdog 与 server 同时读写时不会读到半截 JSON
  const tmp = `${file}.tmp`;
  writeFileSync(tmp, JSON.stringify(payload, null, 2));
  writeFileSync(file, readFileSync(tmp));
}
