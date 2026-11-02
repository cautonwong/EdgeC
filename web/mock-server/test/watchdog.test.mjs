import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import test from 'node:test';

import { sweep } from '../watchdog.mjs';
import { STATE_VERSION } from '../state.mjs';

function makeState(overrides = {}) {
  return {
    version: STATE_VERSION,
    configRevision: 8,
    interfaces: [
      { id: 'eth0', name: 'eth0', addresses: ['192.168.10.99/24'], gateway: '192.168.10.1', mtu: 1500, state: 'up' },
    ],
    sections: { 'system.time': { ntpEnabled: false } },
    alarms: [],
    transactions: [
      {
        id: 'tx_1',
        sectionId: 'network.interfaces',
        state: 'PENDING_CONFIRM',
        values: { interfaceId: 'eth0', addresses: ['192.168.10.99/24'] },
        deadline: new Date(Date.now() - 1000).toISOString(),
        snapshot: [
          { id: 'eth0', name: 'eth0', addresses: ['192.168.10.21/24'], gateway: '192.168.10.1', mtu: 1500, state: 'up' },
        ],
        sectionSnapshot: null,
      },
    ],
    ...overrides,
  };
}

test('watchdog：过期未确认 → 回滚、恢复快照、产生告警', () => {
  const state = makeState();
  const handled = sweep(state);

  assert.equal(handled, 1);
  assert.equal(state.transactions[0].state, 'ROLLED_BACK');
  assert.equal(state.transactions[0].rolledBackBy, 'watchdog');
  assert.deepEqual(state.interfaces[0].addresses, ['192.168.10.21/24'], '设备状态必须回到应用前');
  assert.equal(state.alarms.length, 1);
  assert.equal(state.alarms[0].code, 'CFG-WATCHDOG-ROLLBACK');
  assert.equal(state.alarms[0].level, 'major');
});

test('watchdog：未到 deadline 的事务不动', () => {
  const state = makeState();
  state.transactions[0].deadline = new Date(Date.now() + 60_000).toISOString();
  assert.equal(sweep(state), 0);
  assert.equal(state.transactions[0].state, 'PENDING_CONFIRM');
  assert.deepEqual(state.interfaces[0].addresses, ['192.168.10.99/24']);
});

test('watchdog：已确认的事务即使过了 deadline 也不动', () => {
  const state = makeState();
  state.transactions[0].state = 'CONFIRMED';
  assert.equal(sweep(state), 0);
  assert.equal(state.transactions[0].state, 'CONFIRMED');
});

test('watchdog：分区快照同样能回滚（非网络分区）', () => {
  const state = makeState({
    sections: { 'system.time': { ntpEnabled: false } },
    transactions: [
      {
        id: 'tx_2',
        sectionId: 'system.time',
        state: 'PENDING_CONFIRM',
        values: { ntpEnabled: false },
        deadline: new Date(Date.now() - 500).toISOString(),
        snapshot: null,
        sectionSnapshot: { ntpEnabled: true, servers: ['ntp.aliyun.com'] },
      },
    ],
  });
  assert.equal(sweep(state), 1);
  assert.equal(state.sections['system.time'].ntpEnabled, true, '分区值必须回到应用前');
});

test('watchdog --once：真跑一个进程，读写同一个状态文件', () => {
  const dir = mkdtempSync(path.join(tmpdir(), 'edgeos-wd-'));
  const file = path.join(dir, 'state.json');
  writeFileSync(file, JSON.stringify(makeState(), null, 2));

  const out = execFileSync(
    process.execPath,
    [path.resolve(import.meta.dirname, '../watchdog.mjs'), '--state', file, '--once'],
    { encoding: 'utf8' },
  );
  assert.match(out, /once: 1 transaction\(s\) rolled back/);

  const after = JSON.parse(readFileSync(file, 'utf8'));
  assert.equal(after.transactions[0].state, 'ROLLED_BACK');
  assert.equal(after.alarms[0].code, 'CFG-WATCHDOG-ROLLBACK');
});

test('watchdog --once：状态文件不存在时不报错（还没人写过状态）', () => {
  const dir = mkdtempSync(path.join(tmpdir(), 'edgeos-wd-'));
  const file = path.join(dir, 'missing.json');
  const out = execFileSync(
    process.execPath,
    [path.resolve(import.meta.dirname, '../watchdog.mjs'), '--state', file, '--once'],
    { encoding: 'utf8' },
  );
  assert.match(out, /once: 0 transaction\(s\) rolled back/);
});
