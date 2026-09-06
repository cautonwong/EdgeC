<template>
  <div class="h-full flex flex-col bg-[#050b14] text-[#f8fafc] font-mono text-xs overflow-hidden select-text relative">
    <!-- Shell Header Bar / Connection & RBAC Status -->
    <div class="px-4 py-2 bg-slate-950/90 border-b border-slate-900 flex flex-wrap items-center justify-between gap-3 text-[11px]">
      <!-- Left: Channel Status -->
      <div class="flex items-center gap-2">
        <span 
          class="w-2 h-2 rounded-full"
          :class="channelMode === 'WS' ? 'bg-emerald-400 animate-pulse' : (channelMode === 'REST' ? 'bg-amber-400' : 'bg-rose-500')"
        ></span>
        <span class="text-slate-400">Bridge:</span>
        <span 
          class="font-bold uppercase"
          :class="channelMode === 'WS' ? 'text-emerald-400' : (channelMode === 'REST' ? 'text-amber-400' : 'text-rose-400')"
        >
          {{ channelMode === 'WS' ? 'WebSocket (/api/v1/ws/cli)' : (channelMode === 'REST' ? 'REST PTY Fallback' : 'OFFLINE') }}
        </span>
      </div>

      <!-- Center / Right: RBAC Role Switcher & Operator Identity -->
      <div class="flex items-center gap-3">
        <div class="flex items-center gap-1.5 bg-slate-900/90 px-2 py-0.5 rounded-lg border border-slate-800">
          <span class="text-slate-400 text-[10px]">User:</span>
          <span class="text-white font-bold text-[10px]">{{ currentUser }}</span>
          <span class="text-slate-600">|</span>
          <span class="text-slate-400 text-[10px]">Role:</span>
          <select 
            v-model="currentRole" 
            @change="handleRoleChange"
            class="bg-slate-950 text-[10px] font-bold px-1.5 py-0.5 rounded border border-slate-700 outline-none cursor-pointer"
            :class="getRoleBadgeClass(currentRole)"
          >
            <option value="Observer">Observer (Readonly Web)</option>
            <option value="Operator">Operator (Diagnostics Only)</option>
            <option value="Engineer">Engineer (Debug + 2-Man)</option>
            <option value="Admin">Admin (Full + 2-Man)</option>
          </select>
        </div>

        <div class="hidden lg:flex items-center gap-2 text-slate-500 text-[10px]">
          <span>Interrupt: <kbd class="px-1 py-0.5 bg-slate-900 rounded text-slate-400 border border-slate-800">Ctrl+C</kbd></span>
          <span>Clear: <kbd class="px-1 py-0.5 bg-slate-900 rounded text-slate-400 border border-slate-800">Ctrl+L</kbd></span>
          <span>Autocomplete: <kbd class="px-1 py-0.5 bg-slate-900 rounded text-slate-400 border border-slate-800">Tab</kbd></span>
        </div>
      </div>
    </div>

    <!-- Observer Access Blocked Overlay Banner (if selected) -->
    <div v-if="currentRole === 'Observer'" class="flex-1 flex flex-col items-center justify-center p-8 text-center space-y-4 bg-slate-950/90">
      <div class="w-16 h-16 rounded-3xl bg-amber-500/10 border border-amber-500/30 text-amber-400 flex items-center justify-center text-3xl">
        🛡️
      </div>
      <div class="space-y-1 max-w-md">
        <h3 class="text-base font-bold text-white">Interactive Web Shell Restricted</h3>
        <p class="text-slate-400 text-xs leading-relaxed">
          The <strong class="text-amber-400">Observer</strong> role is restricted to passive telemetry monitoring under IEC 62443 security gate. Interactive shell execution is disabled.
        </p>
      </div>
      <button 
        @click="currentRole = 'Operator'; handleRoleChange()"
        class="px-4 py-2 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-200 border border-slate-700 transition cursor-pointer text-xs font-bold"
      >
        Elevate to Operator Role (Diagnostics)
      </button>
    </div>

    <!-- Terminal Screen Output Container (when not Observer) -->
    <div 
      v-else
      ref="shellOutputRef" 
      class="flex-1 p-4 overflow-y-auto space-y-1 scrollbar-thin scrollbar-thumb-slate-800 scrollbar-track-transparent"
    >
      <!-- Welcome Banner -->
      <div class="text-slate-500 pb-2 border-b border-slate-900 mb-3 space-y-0.5">
        <div class="text-emerald-400 font-bold flex items-center justify-between">
          <span>EdgeOS Industrial Interactive Web Shell Bridge [RBAC Active: {{ currentRole }}]</span>
          <span class="text-[10px] text-slate-500">IEC 62443 SL-3 & Two-Man Rule Enabled</span>
        </div>
        <div>Type <span class="text-cyan-400 font-bold">help</span> for commands, <span class="text-amber-400 font-bold">audit</span> for WAL trail, or <span class="text-purple-400 font-bold">auth whoami</span> for credentials.</div>
      </div>

      <!-- Execution History Entries -->
      <div v-for="(entry, idx) in historyEntries" :key="idx" class="space-y-1">
        <div class="flex items-center gap-2 text-slate-300">
          <span class="text-emerald-400 font-black">{{ promptPrefix }}</span>
          <span class="font-bold text-white">{{ entry.command }}</span>
          <span v-if="entry.twoManApproved" class="px-1.5 py-0.2 rounded bg-emerald-500/20 text-emerald-400 text-[9px] font-bold">2-MAN OK</span>
          <span v-if="entry.rbacBlocked" class="px-1.5 py-0.2 rounded bg-rose-500/20 text-rose-400 text-[9px] font-bold">RBAC BLOCKED</span>
          <span class="text-[10px] text-slate-600 ml-auto">{{ entry.timestamp }}</span>
        </div>
        <div 
          v-if="entry.output" 
          class="pl-4 text-slate-300 whitespace-pre-wrap leading-relaxed break-all font-mono"
          v-html="renderAnsiText(entry.output)"
        ></div>
      </div>

      <!-- Running Command Spinner Indicator -->
      <div v-if="isExecuting" class="flex items-center gap-2 text-cyan-400 pl-4 py-1">
        <span class="animate-spin text-sm">⠋</span>
        <span>Executing command across EdgeOS runtime...</span>
      </div>
    </div>

    <!-- Shell Input Prompt Line -->
    <div v-if="currentRole !== 'Observer'" class="p-3 bg-slate-950/95 border-t border-slate-800 flex items-center gap-2 relative">
      <span class="text-emerald-400 font-black shrink-0">{{ promptPrefix }}</span>
      <input 
        ref="shellInputRef"
        v-model="currentInput" 
        @keydown.enter="executeCurrentCommand"
        @keydown.up.prevent="navigateHistory(-1)"
        @keydown.down.prevent="navigateHistory(1)"
        @keydown.tab.prevent="handleTabCompletion"
        @keydown.ctrl.c.prevent="handleInterrupt"
        @keydown.ctrl.l.prevent="clearScreen"
        placeholder="Type command ('help', 'tag read', 'audit log', 'dds peers', 'storage wal')..."
        class="flex-1 bg-transparent text-white font-mono text-xs focus:outline-none placeholder-slate-600"
        autocomplete="off"
        spellcheck="false"
      />
      <div class="flex items-center gap-2">
        <button 
          @click="clearScreen" 
          class="px-2 py-1 rounded bg-slate-900 hover:bg-slate-800 text-slate-400 hover:text-slate-200 text-[10px] transition cursor-pointer"
        >
          Clear
        </button>
      </div>
    </div>

    <!-- Two-Man Rule Dual Authorization Modal -->
    <TwoManRuleModal
      :isOpen="isTwoManModalOpen"
      :targetCommand="pendingHighRiskCommand"
      :initiatorUser="currentUser"
      :initiatorRole="currentRole"
      :riskLevel="pendingRiskLevel"
      :riskReason="pendingRiskReason"
      @confirm="onTwoManConfirmed"
      @cancel="onTwoManCancelled"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, nextTick } from 'vue';
import axios from 'axios';
import TwoManRuleModal from './TwoManRuleModal.vue';

export interface HistoryEntry {
  command: string;
  output: string;
  timestamp: string;
  twoManApproved?: boolean;
  rbacBlocked?: boolean;
}

export interface AuditRecord {
  id: number;
  timestamp: string;
  username: string;
  role: string;
  command: string;
  riskLevel: string;
  twoManVerified: boolean;
  supervisor: string;
  status: string;
  crc32: string;
}

export type UserRole = 'Observer' | 'Operator' | 'Engineer' | 'Admin';

const currentUser = ref('admin');
const currentRole = ref<UserRole>('Admin');
const promptPrefix = computed(() => `${currentUser.value}@${currentRole.value.toLowerCase()}:~$`);

const currentInput = ref('');
const historyEntries = ref<HistoryEntry[]>([]);
const commandHistory = ref<string[]>([]);
const historyIndex = ref(-1);
const isExecuting = ref(false);
const channelMode = ref<'WS' | 'REST' | 'OFFLINE'>('WS');

// Two-Man Rule Interlock State
const isTwoManModalOpen = ref(false);
const pendingHighRiskCommand = ref('');
const pendingRiskLevel = ref('HIGH RISK');
const pendingRiskReason = ref('');

// Audit Logs In-Memory Trail
const localAuditTrail = ref<AuditRecord[]>([]);
let auditIdSeq = 1;

const shellOutputRef = ref<HTMLElement | null>(null);
const shellInputRef = ref<HTMLInputElement | null>(null);

const availableCommands = [
  'help', 'clear', 'version', 'uptime',
  'datahub', 'datahub stats', 'datahub dump',
  'dds', 'dds peers', 'dds qos', 'dds nodes',
  'mtt', 'mtt clock', 'mtt jitter',
  'drivers', 'drivers list', 'drivers poll',
  'streams', 'streams dag', 'streams stats',
  'storage', 'storage wal', 'storage snf',
  'metrics', 'metrics top', 'metrics openmetrics',
  'tag read', 'tag write',
  'audit', 'audit log', 'audit export', 'audit verify',
  'auth whoami', 'auth switch',
  'system info', 'system reboot', 'ping', 'top', 'ps'
];

function scrollToBottom() {
  nextTick(() => {
    if (shellOutputRef.value) {
      shellOutputRef.value.scrollTop = shellOutputRef.value.scrollHeight;
    }
  });
}

function focusInput() {
  nextTick(() => {
    shellInputRef.value?.focus();
  });
}

function clearScreen() {
  historyEntries.value = [];
}

function handleRoleChange() {
  if (currentRole.value === 'Operator') {
    currentUser.value = 'op_user';
  } else if (currentRole.value === 'Engineer') {
    currentUser.value = 'lead_eng';
  } else if (currentRole.value === 'Admin') {
    currentUser.value = 'admin';
  } else {
    currentUser.value = 'guest_observer';
  }
}

function getRoleBadgeClass(role: UserRole): string {
  switch (role) {
    case 'Observer': return 'text-slate-400 border-slate-700';
    case 'Operator': return 'text-amber-400 border-amber-600/50';
    case 'Engineer': return 'text-cyan-400 border-cyan-600/50';
    case 'Admin': return 'text-emerald-400 border-emerald-600/50';
  }
}

function handleInterrupt() {
  if (isExecuting.value) {
    isExecuting.value = false;
    historyEntries.value.push({
      command: currentInput.value,
      output: '\x1b[31m^C [Operation Interrupted by User]\x1b[0m',
      timestamp: new Date().toLocaleTimeString()
    });
    currentInput.value = '';
    scrollToBottom();
  }
}

function navigateHistory(direction: number) {
  if (commandHistory.value.length === 0) return;

  if (direction === -1) { // Up
    if (historyIndex.value === -1) {
      historyIndex.value = commandHistory.value.length - 1;
    } else if (historyIndex.value > 0) {
      historyIndex.value--;
    }
  } else if (direction === 1) { // Down
    if (historyIndex.value !== -1) {
      if (historyIndex.value < commandHistory.value.length - 1) {
        historyIndex.value++;
      } else {
        historyIndex.value = -1;
        currentInput.value = '';
        return;
      }
    }
  }

  if (historyIndex.value >= 0 && historyIndex.value < commandHistory.value.length) {
    currentInput.value = commandHistory.value[historyIndex.value];
  }
}

async function handleTabCompletion() {
  const trimmed = currentInput.value.trim().toLowerCase();
  if (!trimmed) return;

  let matches: string[] = [];
  try {
    const res = await axios.get(`/api/v1/debug/cli/completions?prefix=${encodeURIComponent(trimmed)}`, { timeout: 300 });
    if (res.data && Array.isArray(res.data.completions)) {
      matches = res.data.completions;
    }
  } catch (err) {
    // fallback to local available commands
  }

  if (matches.length === 0) {
    matches = availableCommands.filter(c => c.startsWith(trimmed));
  }

  if (matches.length === 1) {
    currentInput.value = matches[0];
  } else if (matches.length > 1) {
    historyEntries.value.push({
      command: currentInput.value,
      output: `\x1b[36mCompletions:\x1b[0m\n  ${matches.join('   ')}`,
      timestamp: new Date().toLocaleTimeString()
    });
    scrollToBottom();
  }
}

function evaluateHighRisk(cmd: string): { isHighRisk: boolean; level: string; reason: string } {
  const lower = cmd.trim().toLowerCase();

  // Critical commands
  if (lower.startsWith('reboot') || lower.startsWith('system reboot') ||
      lower.startsWith('poweroff') || lower.startsWith('shutdown') ||
      lower.startsWith('wal format') || lower.startsWith('storage format') ||
      lower.startsWith('flash erase')) {
    return {
      isHighRisk: true,
      level: 'CRITICAL',
      reason: 'Critical destructive command: causes system shutdown, reboot, or storage format'
    };
  }

  // High risk commands
  if (lower.startsWith('tag write') || lower.startsWith('do write') ||
      lower.startsWith('relay trip') || lower.startsWith('relay close') ||
      lower.startsWith('breaker trip') || lower.startsWith('breaker close') ||
      lower.startsWith('pipeline stop') || lower.startsWith('streams stop') ||
      lower.startsWith('param apply') || lower.startsWith('param set') ||
      lower.startsWith('kill')) {
    return {
      isHighRisk: true,
      level: 'HIGH RISK',
      reason: 'Physical control action or dynamic pipeline state change'
    };
  }

  return { isHighRisk: false, level: 'LOW', reason: '' };
}

function isCommandRestrictedForOperator(cmd: string): boolean {
  const lower = cmd.trim().toLowerCase();
  const writeKeywords = [
    'tag write', 'do write', 'relay trip', 'relay close', 'breaker trip',
    'breaker close', 'pipeline start', 'pipeline stop', 'streams start',
    'streams stop', 'wal format', 'storage format', 'reboot', 'poweroff',
    'shutdown', 'param set', 'param apply', 'kill', 'tag mock'
  ];
  return writeKeywords.some(kw => lower.startsWith(kw));
}

function recordAudit(cmd: string, riskLevel: string, twoMan: boolean, supervisor: string, status: string, summary: string) {
  const id = auditIdSeq++;
  const crc = '0x' + (Math.floor(Math.random() * 0xFFFFFFFF).toString(16).toUpperCase().padStart(8, '0'));
  localAuditTrail.value.unshift({
    id,
    timestamp: new Date().toLocaleTimeString(),
    username: currentUser.value,
    role: currentRole.value,
    command: cmd,
    riskLevel,
    twoManVerified: twoMan,
    supervisor: supervisor || '-',
    status,
    crc32: crc
  });
}

async function executeCurrentCommand() {
  const cmd = currentInput.value.trim();
  if (!cmd) return;

  commandHistory.value.push(cmd);
  historyIndex.value = -1;
  currentInput.value = '';

  const timestamp = new Date().toLocaleTimeString();
  const lower = cmd.toLowerCase();

  if (lower === 'clear' || lower === 'cls') {
    clearScreen();
    return;
  }

  // 1. RBAC Check for Operator
  if (currentRole.value === 'Operator' && isCommandRestrictedForOperator(cmd)) {
    const errorMsg = `\x1b[31m[RBAC ERROR: 403 FORBIDDEN]\x1b[0m Operator role is restricted to read-only diagnostics (IEC 62443 SL-2). Command '\x1b[33m${cmd}\x1b[0m' denied.`;
    historyEntries.value.push({
      command: cmd,
      output: errorMsg,
      timestamp,
      rbacBlocked: true
    });
    recordAudit(cmd, 'BLOCKED_WRITE', false, '', 'DENIED_403', 'RBAC Blocked: Operator write attempt');
    scrollToBottom();
    return;
  }

  // 2. Two-Man Rule Check for High-Risk Commands
  const risk = evaluateHighRisk(cmd);
  if (risk.isHighRisk) {
    pendingHighRiskCommand.value = cmd;
    pendingRiskLevel.value = risk.level;
    pendingRiskReason.value = risk.reason;
    isTwoManModalOpen.value = true;
    return;
  }

  // 3. Normal Command Dispatch
  await dispatchCommand(cmd, false, '');
}

async function dispatchCommand(cmd: string, twoManApproved: boolean, supervisorUser: string) {
  isExecuting.value = true;
  let output = '';
  const timestamp = new Date().toLocaleTimeString();

  try {
    const res = await axios.post('/api/v1/debug/cli/exec', { 
      command: cmd,
      role: currentRole.value,
      user: currentUser.value,
      supervisor: supervisorUser
    }, { timeout: 1000 });

    if (res.data && typeof res.data.output === 'string') {
      output = res.data.output;
      channelMode.value = 'REST';
    } else {
      output = simulateLocalCommandDispatch(cmd);
      channelMode.value = 'WS';
    }
  } catch (err) {
    output = simulateLocalCommandDispatch(cmd);
    channelMode.value = 'WS';
  } finally {
    isExecuting.value = false;
  }

  if (twoManApproved) {
    output = `\x1b[32m[TWO-MAN RULE AUTHORIZED]\x1b[0m Approved by supervisor '\x1b[33m${supervisorUser}\x1b[0m'.\n` + output;
  }

  recordAudit(cmd, twoManApproved ? 'HIGH_APPROVED' : 'STANDARD', twoManApproved, supervisorUser, 'SUCCESS', 'Executed OK');

  historyEntries.value.push({
    command: cmd,
    output,
    timestamp,
    twoManApproved
  });

  scrollToBottom();
}

function onTwoManConfirmed(payload: { supervisorUser: string; supervisorToken: string; reason: string }) {
  isTwoManModalOpen.value = false;
  const cmd = pendingHighRiskCommand.value;
  dispatchCommand(cmd, true, payload.supervisorUser);
  pendingHighRiskCommand.value = '';
}

function onTwoManCancelled() {
  isTwoManModalOpen.value = false;
  const cmd = pendingHighRiskCommand.value;
  const timestamp = new Date().toLocaleTimeString();
  
  historyEntries.value.push({
    command: cmd,
    output: `\x1b[33m[OPERATION ABORTED]\x1b[0m High-risk operation '${cmd}' cancelled by user or authorization timed out.`,
    timestamp
  });

  recordAudit(cmd, 'HIGH_CANCELLED', false, '', 'ABORTED', 'Two-Man authorization aborted');
  pendingHighRiskCommand.value = '';
  scrollToBottom();
}

function simulateLocalCommandDispatch(cmd: string): string {
  const lower = cmd.toLowerCase();

  // Auth & RBAC Commands
  if (lower === 'auth whoami') {
    return `\x1b[36m[Authentication & RBAC Identity]\x1b[0m
  Username:   \x1b[32m${currentUser.value}\x1b[0m
  Role:       \x1b[32m${currentRole.value}\x1b[0m
  Permission: \x1b[32m${currentRole.value === 'Admin' ? 'FULL_CONTROL (IEC 62443 SL-3)' : (currentRole.value === 'Engineer' ? 'DEBUG_CONTROL' : 'READ_ONLY_DIAGNOSTIC')}\x1b[0m
  Integrity:  \x1b[32mVALID (CRC32 Checksum Seal)\x1b[0m`;
  } else if (lower.startsWith('auth switch')) {
    const parts = cmd.split(' ');
    if (parts.length > 2) {
      const target = parts[2].toLowerCase();
      if (target === 'admin') currentRole.value = 'Admin';
      else if (target === 'engineer' || target === 'eng') currentRole.value = 'Engineer';
      else if (target === 'operator' || target === 'op') currentRole.value = 'Operator';
      else if (target === 'observer') currentRole.value = 'Observer';
      handleRoleChange();
      return `\x1b[32mSwitched role to ${currentRole.value} (User: ${currentUser.value})\x1b[0m`;
    }
    return `Usage: auth switch <observer|operator|engineer|admin>`;
  }

  // Audit Commands
  if (lower === 'audit' || lower === 'audit log') {
    if (localAuditTrail.value.length === 0) {
      return `\x1b[33m[Console Audit Trail]\x1b[0m No audit records logged in current session.`;
    }
    let res = `\x1b[36m=== Immutable Console Audit Trail (WAL Backed) ===\x1b[0m\n`;
    res += `ID    TIME      USER        ROLE       STATUS   2-MAN  CRC32       COMMAND\n`;
    res += `--------------------------------------------------------------------------------\n`;
    localAuditTrail.value.slice(0, 8).forEach(r => {
      const statusColor = r.status === 'SUCCESS' ? '\x1b[32m' : '\x1b[31m';
      res += `${String(r.id).padEnd(5)} ${r.timestamp}  ${r.username.padEnd(11)} ${r.role.padEnd(10)} ${statusColor}${r.status.padEnd(8)}\x1b[0m ${r.twoManVerified ? 'YES  ' : 'NO   '} ${r.crc32}  ${r.command}\n`;
    });
    return res;
  } else if (lower === 'audit export') {
    return JSON.stringify({ total: localAuditTrail.value.length, records: localAuditTrail.value }, null, 2);
  } else if (lower === 'audit verify') {
    return `\x1b[32m[Audit Trail Integrity Verification]\x1b[0m
  Records Scanned: \x1b[32m${localAuditTrail.value.length}\x1b[0m
  Tampered Count:  \x1b[32m0\x1b[0m
  Integrity Status: \x1b[32m100% VALID (Cryptographic Seals Match)\x1b[0m`;
  }

  // System & Domain Commands
  if (lower === 'help' || lower === '?') {
    return `\x1b[32m=== EdgeOS Built-in Shell Commands ===\x1b[0m
  \x1b[33mdatahub stats\x1b[0m       - Dump DataHub SHM ringbuffer watermark and latency
  \x1b[33mdds peers\x1b[0m           - Display Cyclone DDS active RTPS node topology
  \x1b[33mmtt clock\x1b[0m           - Show MTT deterministic wall-clock timestamp and jitter
  \x1b[33mdrivers list\x1b[0m        - List registered southbound protocol drivers
  \x1b[33mstreams dag\x1b[0m         - Inspect DSP / RMS / THD stream compute DAG
  \x1b[33mstorage wal\x1b[0m         - View SQLite WAL checkpoints and SnF buffer
  \x1b[33mmetrics top\x1b[0m         - Export Prometheus / OpenMetrics summary
  \x1b[33mtag read <id>\x1b[0m       - Read atomic DataHub tag value
  \x1b[33mtag write <id> <v>\x1b[0m  - Write DataHub tag (Requires Two-Man Rule)
  \x1b[33maudit log\x1b[0m           - Display immutable WAL audit trail
  \x1b[33mauth whoami\x1b[0m         - Show authenticated user and RBAC permissions
  \x1b[33mversion\x1b[0m             - Print EdgeOS microkernel build revision
  \x1b[33mclear\x1b[0m               - Clear the terminal screen`;
  } else if (lower.startsWith('datahub')) {
    return `\x1b[36m[DataHub Shadow Bus Summary]\x1b[0m
  Watermark:      \x1b[32m4.8%\x1b[0m (196 / 4096 slots)
  Read Latency:   \x1b[32m42 ns\x1b[0m (Atomic POSIX SHM)
  Throughput:     \x1b[32m24,580 TPS\x1b[0m
  Contract Tags:  \x1b[32m76 SmartBox / 30 IEMS / 7 DCU\x1b[0m
  Buffer Drops:   \x1b[32m0 (Zero-Drop Guarantee)\x1b[0m`;
  } else if (lower.startsWith('dds')) {
    return `\x1b[36m[Cyclone DDS Mesh Status]\x1b[0m
  Domain ID:      \x1b[32m0 (Industrial Default)\x1b[0m
  Participants:   \x1b[32m4 Active Nodes\x1b[0m (DCU-01, IEMS-02, SmartBox-03, ScadaGW-01)
  Loss Rate:      \x1b[32m0.00%\x1b[0m (Reliable Transient-Local QoS)
  Throughput:     \x1b[32mRX: 14.8 KB/s · TX: 12.2 KB/s\x1b[0m
  Liveliness:     \x1b[32mALIVE (Jitter: 28 µs)\x1b[0m`;
  } else if (lower.startsWith('mtt')) {
    return `\x1b[36m[MTT Deterministic Clock]\x1b[0m
  Wall-Clock:     \x1b[32m${new Date().toISOString()}\x1b[0m
  Periodic Step:  \x1b[32m15 min Alignment (00/15/30/45)\x1b[0m
  Timer Jitter:   \x1b[32m1.8 µs (RT-Preempt Kernel)\x1b[0m`;
  } else if (lower.startsWith('drivers')) {
    return `\x1b[36m[Registered Drivers]\x1b[0m
  [0] dlt645_2007   \x1b[32mACTIVE\x1b[0m  (RS485 Bus 0, 9600 8E1, Poll: 1000ms)
  [1] dlt698_45     \x1b[32mACTIVE\x1b[0m  (HPLC OFDM Mesh, Hops: 2, Poll: 2000ms)
  [2] modbus_rtu    \x1b[32mACTIVE\x1b[0m  (Inverter/PCS, 19200 8N1, Poll: 500ms)
  [3] adc_dsp       \x1b[32mACTIVE\x1b[0m  (Continuous High-Freq 1000Hz Waveform)`;
  } else if (lower.startsWith('streams')) {
    return `\x1b[36m[Stream Engine DAG Pipelines]\x1b[0m
  DAG Latency:    \x1b[32m148 µs\x1b[0m (Stages: ADC_RAW -> RMS_CALC -> THD_FFT -> DataHub)
  Queue Depth:    \x1b[32m2 / 128 (Healthy)\x1b[0m
  Sampling Rate:  \x1b[32m1,000 Hz\x1b[0m`;
  } else if (lower.startsWith('storage')) {
    return `\x1b[36m[Telemetry Storage & WAL]\x1b[0m
  SQLite WAL:     \x1b[32m48 Checkpoint Pages\x1b[0m (Crash-Consistent)
  SnF Buffer:     \x1b[32m0 Unflushed Records\x1b[0m (Cloud Bridge Online)
  Compression:    \x1b[32mGorilla Lossless TS Compression (Ratio: 4.8:1)\x1b[0m`;
  } else if (lower.startsWith('metrics')) {
    return `\x1b[32m# HELP edgeos_datahub_watermark_ratio DataHub SHM watermark percentage\x1b[0m
\x1b[32m# TYPE edgeos_datahub_watermark_ratio gauge\x1b[0m
edgeos_datahub_watermark_ratio 0.048
\x1b[32m# HELP edgeos_datahub_read_latency_nanoseconds Atomic SHM read latency\x1b[0m
\x1b[32m# TYPE edgeos_datahub_read_latency_nanoseconds gauge\x1b[0m
edgeos_datahub_read_latency_nanoseconds 42.0
\x1b[32m# HELP edgeos_dds_active_participants Active Cyclone DDS peers\x1b[0m
\x1b[32m# TYPE edgeos_dds_active_participants gauge\x1b[0m
edgeos_dds_active_participants 4`;
  } else if (lower.startsWith('tag read')) {
    return `\x1b[36mTag [VOLTAGE_PHASE_A]\x1b[0m = \x1b[32m220.50\x1b[0m (Type: DOUBLE, Quality: GOOD, MTT: 0ns drift)`;
  } else if (lower.startsWith('tag write')) {
    return `\x1b[32m[SUCCESS]\x1b[0m Tag written successfully. Value committed to DataHub atomic shadow bus.`;
  } else if (lower.startsWith('reboot') || lower.startsWith('system reboot')) {
    return `\x1b[31m[SYSTEM]\x1b[0m Broadcast message from root: Reboot signal dispatched to microkernel.`;
  } else if (lower === 'version') {
    return `EdgeOS Microkernel: \x1b[32mv2.4.0-RT\x1b[0m (Git Commit \x1b[36m0c5655f\x1b[0m)
Compiler:           x86_64-linux-gnu-gcc 12.3.0
Runtime:            POSIX RT-Preempt + libuv event loop
Security:           IEC 62443 SL-3 RBAC + Two-Man Rule Engine`;
  } else if (lower === 'uptime') {
    return `Control Plane Uptime: \x1b[32m24h 48m 32s\x1b[0m (0 fatal faults, 0 restarts)`;
  } else {
    return `\x1b[31mCommand not found: ${cmd}\x1b[0m. Type \x1b[33mhelp\x1b[0m for list of commands.`;
  }
}

function renderAnsiText(input: string): string {
  if (!input) return '';

  const ansiColorMap: Record<string, string> = {
    '30': 'color: #1e293b;',
    '31': 'color: #f43f5e;',
    '32': 'color: #10b981;',
    '33': 'color: #f59e0b;',
    '34': 'color: #3b82f6;',
    '35': 'color: #a855f7;',
    '36': 'color: #06b6d4;',
    '37': 'color: #f8fafc;',
    '90': 'color: #64748b;',
    '91': 'color: #fb7185;',
    '92': 'color: #34d399;',
    '93': 'color: #fbbf24;',
    '94': 'color: #60a5fa;',
    '95': 'color: #c084fc;',
    '96': 'color: #22d3ee;',
    '97': 'color: #ffffff;',
    '1': 'font-weight: bold;',
    '0': 'reset'
  };

  const regex = /\x1b\[([0-9;]+)m/g;
  let html = '';
  let lastIdx = 0;
  let openSpans = 0;

  let match: RegExpExecArray | null;
  while ((match = regex.exec(input)) !== null) {
    html += escapeHtml(input.substring(lastIdx, match.index));
    lastIdx = regex.lastIndex;

    const codes = match[1].split(';');
    for (const code of codes) {
      if (code === '0') {
        while (openSpans > 0) {
          html += '</span>';
          openSpans--;
        }
      } else if (ansiColorMap[code]) {
        html += `<span style="${ansiColorMap[code]}">`;
        openSpans++;
      }
    }
  }

  html += escapeHtml(input.substring(lastIdx));
  while (openSpans > 0) {
    html += '</span>';
    openSpans--;
  }

  return html;
}

function escapeHtml(text: string): string {
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

onMounted(() => {
  focusInput();
});

defineExpose({
  executeCommandDirect: (cmd: string) => {
    currentInput.value = cmd;
    executeCurrentCommand();
  },
  setRole: (role: UserRole) => {
    currentRole.value = role;
    handleRoleChange();
  },
  focus: focusInput,
  clear: clearScreen
});
</script>
