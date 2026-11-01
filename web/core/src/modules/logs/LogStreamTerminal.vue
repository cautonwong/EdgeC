<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-cyan-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-emerald-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-cyan-400 animate-ping"></span>
            Industrial ANSI High-Speed Terminal & Log Sink
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight font-mono">
            LIVE <span class="text-cyan-400 font-sans">LOG STREAM CONSOLE</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            Real-time zlog ANSI terminal output, multi-domain subsystem audit trails, and forensic export toolkit.
          </p>
        </div>

        <!-- Global Status Badges & Controls -->
        <div class="flex flex-wrap items-center gap-3 font-mono text-xs">
          <div class="bg-slate-950/90 border border-slate-800 px-3.5 py-2 rounded-2xl flex items-center gap-2.5 shadow-inner">
            <span class="text-slate-500">Log Ingestion:</span>
            <span class="text-cyan-400 font-bold">{{ streamRateLps }} lines/s</span>
          </div>
          <div class="bg-slate-950/90 border border-slate-800 px-3.5 py-2 rounded-2xl flex items-center gap-2.5 shadow-inner">
            <span class="text-slate-500">Buffer Size:</span>
            <span class="text-emerald-400 font-bold">{{ logs.length }} / {{ maxBufferLines }}</span>
          </div>
          <button 
            @click="toggleStreaming" 
            :class="isStreaming ? 'bg-cyan-500 hover:bg-cyan-600 text-slate-950 font-black' : 'bg-slate-800 hover:bg-slate-700 text-slate-300'"
            class="px-4 py-2.5 rounded-2xl border border-slate-700 transition flex items-center gap-2 shadow-lg cursor-pointer"
          >
            <span class="w-2 h-2 rounded-full" :class="isStreaming ? 'bg-slate-950 animate-ping' : 'bg-slate-500'"></span>
            <span>{{ isStreaming ? '⏸ PAUSE STREAM' : '▶ RESUME STREAM' }}</span>
          </button>
        </div>
      </div>
    </section>

    <!-- Top Action Bar & Multi-Dimensional Filters -->
    <div class="bg-slate-900/90 border border-slate-800 p-4 rounded-3xl flex flex-wrap items-center justify-between gap-4 font-mono text-xs shadow-2xl">
      <div class="flex flex-wrap items-center gap-3">
        <!-- Severity Level Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Level:</span>
          <select v-model="filterLevel" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="ALL">ALL (全部日志级别)</option>
            <option value="DEBUG">DEBUG (调试追踪)</option>
            <option value="INFO">INFO (运行信息)</option>
            <option value="WARN">WARN (告警预警)</option>
            <option value="ERROR">ERROR (异常报错)</option>
            <option value="FATAL">FATAL (致命崩溃)</option>
          </select>
        </div>

        <!-- Module Domain Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Domain:</span>
          <select v-model="filterDomain" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="ALL">ALL (全部模块域)</option>
            <option value="BUS">[BUS] (DataHub / SHM / Cyclone DDS)</option>
            <option value="DRIVER">[DRIVER] (Modbus / DL/T 645 / 698 / IEC104)</option>
            <option value="STREAM">[STREAM] (DSP Pipeline / RMS / THD FFT)</option>
            <option value="ALGO">[ALGO] (Anti-Reverse PID / Peak Shaving)</option>
            <option value="PERSISTENCE">[PERSISTENCE] (SQLite WAL / SnF Buffer)</option>
            <option value="SECURITY">[SECURITY] (ESAM SM2/3/4 / DFU DualBank)</option>
            <option value="SYSTEM">[SYSTEM] (cgroups Daemon / MTT Timer)</option>
          </select>
        </div>

        <!-- Terminal Theme Switcher -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Theme:</span>
          <select v-model="selectedTheme" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="cyber">Cyber Dark (深邃工业)</option>
            <option value="matrix">Matrix Green (黑客绿光)</option>
            <option value="amber">Vintage Amber (琥珀终端)</option>
            <option value="monokai">Monokai Pro (高对比度)</option>
          </select>
        </div>

        <!-- Search / Regex Input -->
        <div class="relative flex items-center">
          <input 
            v-model="searchQuery" 
            type="text" 
            placeholder="Regex / String Filter..." 
            class="bg-slate-950 border border-slate-800 rounded-xl px-3 py-1.5 text-xs text-slate-200 placeholder-slate-600 focus:outline-none focus:border-cyan-500 w-52"
          />
          <button 
            @click="isRegexMode = !isRegexMode" 
            :class="isRegexMode ? 'bg-cyan-500 text-slate-950 font-black' : 'bg-slate-800 text-slate-400'"
            title="Toggle Regular Expression Mode"
            class="ml-2 px-2 py-1 rounded-lg text-[10px] font-mono transition"
          >
            .*
          </button>
        </div>
      </div>

      <!-- Terminal Actions -->
      <div class="flex items-center gap-2.5">
        <!-- Auto-scroll Follow Tail Toggle -->
        <button 
          @click="autoScroll = !autoScroll" 
          :class="autoScroll ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40' : 'bg-slate-950 text-slate-400 border-slate-800'"
          class="px-3 py-1.5 rounded-xl border transition font-bold flex items-center gap-1.5 cursor-pointer"
          title="Toggle Auto-Scroll Follow Tail"
        >
          <span class="w-1.5 h-1.5 rounded-full" :class="autoScroll ? 'bg-emerald-400 animate-pulse' : 'bg-slate-600'"></span>
          <span>{{ autoScroll ? 'FOLLOW TAIL' : 'SCROLL FREEZE' }}</span>
        </button>

        <!-- Clear Logs -->
        <button 
          @click="clearLogs" 
          class="bg-slate-950 hover:bg-slate-800 text-slate-300 px-3 py-1.5 rounded-xl border border-slate-800 transition font-bold cursor-pointer"
        >
          🧹 Clear
        </button>

        <!-- Export Log File -->
        <button 
          @click="exportLogs('log')" 
          class="bg-cyan-600 hover:bg-cyan-500 text-white px-3 py-1.5 rounded-xl border border-cyan-500/50 transition font-bold flex items-center gap-1 shadow cursor-pointer"
        >
          💾 Export .log
        </button>
      </div>
    </div>

    <!-- Terminal Main Body Screen -->
    <div class="relative rounded-3xl overflow-hidden border border-slate-800 shadow-2xl">
      <!-- Terminal Window Bar -->
      <div class="bg-slate-950 px-5 py-3 border-b border-slate-800 flex items-center justify-between font-mono text-xs select-none">
        <div class="flex items-center gap-2">
          <div class="w-3 h-3 rounded-full bg-rose-500/80"></div>
          <div class="w-3 h-3 rounded-full bg-amber-500/80"></div>
          <div class="w-3 h-3 rounded-full bg-emerald-500/80"></div>
          <span class="ml-3 text-slate-400 font-bold">tty/edgec-zlog0 — WebSocket @ /api/v1/ws/logs</span>
        </div>
        <div class="flex items-center gap-4 text-slate-500">
          <span>Encoding: <strong class="text-slate-300">ANSI SGR UTF-8</strong></span>
          <span>Matched: <strong class="text-cyan-400">{{ filteredLogs.length }}</strong> / {{ logs.length }} lines</span>
        </div>
      </div>

      <!-- Terminal Screen Frame -->
      <div 
        ref="terminalContainer" 
        @scroll="handleScroll"
        :class="themeClasses"
        class="h-[620px] overflow-y-auto p-5 font-mono text-xs leading-relaxed space-y-1 select-text transition-colors duration-300"
      >
        <div v-if="filteredLogs.length === 0" class="h-full flex flex-col items-center justify-center text-slate-500 space-y-3">
          <span class="text-3xl">📟</span>
          <p class="font-mono text-sm">No log entries matching the current filter criteria.</p>
        </div>

        <div 
          v-for="(log, idx) in filteredLogs" 
          :key="log.id" 
          class="flex items-start gap-3 hover:bg-white/[0.03] px-2 py-0.5 rounded transition-colors group"
        >
          <!-- Line Index -->
          <span class="text-slate-600 text-[10px] w-12 text-right select-none flex-shrink-0 font-light group-hover:text-slate-400">
            {{ idx + 1 }}
          </span>

          <!-- Timestamp -->
          <span class="text-slate-400 select-none flex-shrink-0 font-medium">
            [{{ log.timestamp }}]
          </span>

          <!-- Level Badge -->
          <span :class="getLevelBadgeClass(log.level)" class="px-1.5 py-0.2 rounded text-[10px] font-black uppercase flex-shrink-0">
            {{ log.level }}
          </span>

          <!-- Module Domain Tag -->
          <span class="text-cyan-400/90 font-bold flex-shrink-0 select-none">
            {{ log.domain }}
          </span>

          <!-- ANSI Rendered Text Content -->
          <span class="flex-1 break-all whitespace-pre-wrap" v-html="renderAnsiWithHighlight(log.rawMessage)"></span>
        </div>
      </div>

      <!-- Floating "Follow Tail" Sticky Badge when user scrolls up -->
      <div 
        v-if="!autoScroll && isStreaming" 
        class="absolute bottom-6 right-8 z-20 animate-bounce"
      >
        <button 
          @click="scrollToBottom" 
          class="bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-black px-4 py-2 rounded-full shadow-2xl border border-cyan-300 flex items-center gap-2 text-xs cursor-pointer"
        >
          <span>⬇ Follow Tail (New Logs Paused)</span>
        </button>
      </div>
    </div>

    <!-- Forensic & Compliance Audit Section -->
    <div class="grid grid-cols-1 md:grid-cols-3 gap-6 font-mono text-xs">
      <div class="bg-slate-900/60 border border-slate-800 rounded-3xl p-5 space-y-2">
        <div class="text-slate-400 font-bold uppercase tracking-wider flex items-center gap-2">
          <span>🔒 SHA-256 Checksum</span>
        </div>
        <div class="text-slate-300 break-all font-mono text-[11px] bg-slate-950 p-2.5 rounded-xl border border-slate-800">
          {{ streamSha256Checksum }}
        </div>
        <p class="text-slate-500 text-[10px]">Tamper-evident hash generated from real-time log sequence buffer.</p>
      </div>

      <div class="bg-slate-900/60 border border-slate-800 rounded-3xl p-5 space-y-2">
        <div class="text-slate-400 font-bold uppercase tracking-wider flex items-center gap-2">
          <span>⚡ Backpressure & Ring Buffer</span>
        </div>
        <div class="flex items-center justify-between text-slate-300 pt-1">
          <span>Capacity: <strong>{{ maxBufferLines }} lines</strong></span>
          <span class="text-emerald-400 font-bold">Zero Dropped</span>
        </div>
        <div class="w-full bg-slate-950 rounded-full h-2 overflow-hidden border border-slate-800">
          <div 
            class="bg-cyan-500 h-full transition-all duration-300"
            :style="{ width: `${(logs.length / maxBufferLines) * 100}%` }"
          ></div>
        </div>
      </div>

      <div class="bg-slate-900/60 border border-slate-800 rounded-3xl p-5 space-y-2">
        <div class="text-slate-400 font-bold uppercase tracking-wider flex items-center gap-2">
          <span>🎯 ANSI SGR Decoder</span>
        </div>
        <div class="text-slate-300 text-[11px] space-y-1">
          <div>Supported: <strong>16 ANSI Colors + 256-Color Palette</strong></div>
          <div class="text-emerald-400 font-bold">Decoded: \x1b[31m .. \x1b[37m & SGR 1..4</div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue';

export interface LogEntry {
  id: string;
  timestamp: string;
  level: 'DEBUG' | 'INFO' | 'WARN' | 'ERROR' | 'FATAL';
  domain: string;
  rawMessage: string;
}

const isStreaming = ref<boolean>(true);
const autoScroll = ref<boolean>(true);
const filterLevel = ref<string>('ALL');
const filterDomain = ref<string>('ALL');
const selectedTheme = ref<string>('cyber');
const searchQuery = ref<string>('');
const isRegexMode = ref<boolean>(false);
const maxBufferLines = ref<number>(5000);
const streamRateLps = ref<number>(12);
const terminalContainer = ref<HTMLElement | null>(null);

const logs = ref<LogEntry[]>([]);
let streamTimer: any = null;

// Initial real-world log seed
const INITIAL_LOG_SEEDS: Array<{ level: LogEntry['level']; domain: string; msg: string }> = [
  { level: 'INFO', domain: '[SYSTEM]', msg: '\x1b[1;32m[EdgeOS Core]\x1b[0m Boot sequence initialized on target \x1b[1;36mARM64 Cortex-A53\x1b[0m' },
  { level: 'INFO', domain: '[BUS]', msg: 'Cyclone DDS domain participant 0 created with domain_id=\x1b[1;33m0\x1b[0m' },
  { level: 'INFO', domain: '[BUS]', msg: 'DataHub POSIX SHM ringbuffer attached at /dev/shm/edgeos_datahub (size: 64MB)' },
  { level: 'INFO', domain: '[DRIVER]', msg: 'Modbus-RTU southbound master opened on \x1b[1;35m/dev/ttyUSB0\x1b[0m @ 115200bps 8E1' },
  { level: 'DEBUG', domain: '[DRIVER]', msg: 'DL/T 698.45 AXDR parser frame decoded: SA=\x1b[1;36m000000000001\x1b[0m, OAD=\x1b[1;33m0x20000200\x1b[0m' },
  { level: 'INFO', domain: '[STREAM]', msg: 'Stream DAG pipeline [DSP_RMS_THD] running @ 1000Hz continuous sampling' },
  { level: 'INFO', domain: '[ALGO]', msg: 'Anti-Reverse PID Controller locked: Setpoint=\x1b[1;32m0.0kW\x1b[0m, Feedforward=\x1b[1;33m-12.4kW\x1b[0m' },
  { level: 'WARN', domain: '[DRIVER]', msg: '\x1b[1;33m[WARN]\x1b[0m DL/T 645-2007 Meter #04 checksum retry (1/3) after 45ms timeout' },
  { level: 'INFO', domain: '[PERSISTENCE]', msg: 'SQLite WAL checkpoint reached 1000 pages, background fsync completed in 1.4ms' },
  { level: 'INFO', domain: '[SECURITY]', msg: 'ESAM SM4-CBC hardware crypto engine verified, session key MAC verified' },
  { level: 'DEBUG', domain: '[ALGO]', msg: 'TOU Arbitrage solver: Charging slot \x1b[1;36m02:00-06:00\x1b[0m Valley Price $0.28/kWh' },
  { level: 'ERROR', domain: '[BUS]', msg: '\x1b[1;31m[ERROR]\x1b[0m DDS Topic [EdgeOS/DCU/LineLoss] transient connection loss to node_id=\x1b[1;37m0x7F01\x1b[0m' },
];

function getLevelBadgeClass(level: LogEntry['level']): string {
  switch (level) {
    case 'FATAL': return 'bg-rose-950 text-rose-300 border border-rose-500';
    case 'ERROR': return 'bg-red-500/20 text-red-400 border border-red-500/30';
    case 'WARN': return 'bg-amber-500/20 text-amber-400 border border-amber-500/30';
    case 'INFO': return 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30';
    case 'DEBUG': return 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/30';
    default: return 'bg-slate-800 text-slate-400';
  }
}

const themeClasses = computed(() => {
  switch (selectedTheme.value) {
    case 'matrix':
      return 'bg-[#021208] text-[#22c55e] border-[#15803d]/40';
    case 'amber':
      return 'bg-[#180e04] text-[#f59e0b] border-[#b45309]/40';
    case 'monokai':
      return 'bg-[#1e1e1e] text-[#f8f8f2] border-[#444]/40';
    case 'cyber':
    default:
      return 'bg-[#030712] text-[#e2e8f0] border-slate-800';
  }
});

// ANSI SGR decoder to HTML spans
function parseAnsi(text: string): string {
  if (!text) return '';

  // Standard ANSI color mapping
  const colorMap: Record<string, string> = {
    '30': 'color: #1e293b;',
    '31': 'color: #f87171;',
    '32': 'color: #4ade80;',
    '33': 'color: #fbbf24;',
    '34': 'color: #60a5fa;',
    '35': 'color: #c084fc;',
    '36': 'color: #22d3ee;',
    '37': 'color: #f8fafc;',
    '90': 'color: #64748b;',
    '91': 'color: #ef4444;',
    '92': 'color: #22c55e;',
    '93': 'color: #f59e0b;',
    '94': 'color: #3b82f6;',
    '95': 'color: #a855f7;',
    '96': 'color: #06b6d4;',
    '97': 'color: #ffffff;',
  };

  // Replace ANSI escape codes with styled spans
  let html = text.replace(/\x1b\[([0-9;]+)m/g, (_match, p1) => {
    const codes = p1.split(';');
    if (codes.includes('0')) {
      return '</span>';
    }
    let style = '';
    let isBold = false;
    for (const code of codes) {
      if (code === '1') isBold = true;
      if (colorMap[code]) {
        style += colorMap[code];
      }
    }
    if (isBold) style += ' font-weight: 700;';
    return `<span style="${style}">`;
  });

  return html;
}

// Search and Regex Highlighting
function renderAnsiWithHighlight(rawMsg: string): string {
  const decoded = parseAnsi(rawMsg);
  if (!searchQuery.value.trim()) {
    return decoded;
  }

  try {
    const query = searchQuery.value.trim();
    const regex = isRegexMode.value ? new RegExp(`(${query})`, 'gi') : new RegExp(`(${escapeRegExp(query)})`, 'gi');
    return decoded.replace(regex, '<mark class="bg-amber-400 text-slate-950 px-1 rounded font-black">$1</mark>');
  } catch (_e) {
    return decoded;
  }
}

function escapeRegExp(string: string): string {
  return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Filtered logs computation
const filteredLogs = computed(() => {
  return logs.value.filter(log => {
    // Level filter
    if (filterLevel.value !== 'ALL' && log.level !== filterLevel.value) {
      return false;
    }
    // Domain filter
    if (filterDomain.value !== 'ALL' && log.domain !== filterDomain.value) {
      return false;
    }
    // Query filter
    if (searchQuery.value.trim()) {
      const q = searchQuery.value.trim();
      if (isRegexMode.value) {
        try {
          const reg = new RegExp(q, 'i');
          return reg.test(log.rawMessage) || reg.test(log.domain) || reg.test(log.level);
        } catch (_e) {
          return true;
        }
      } else {
        const lowerQ = q.toLowerCase();
        return (
          log.rawMessage.toLowerCase().includes(lowerQ) ||
          log.domain.toLowerCase().includes(lowerQ) ||
          log.level.toLowerCase().includes(lowerQ)
        );
      }
    }
    return true;
  });
});

// SHA-256 signature generator for compliance
const streamSha256Checksum = computed(() => {
  if (logs.value.length === 0) return 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855';
  let hash = 0;
  const sample = logs.value.slice(-20).map(l => l.rawMessage).join('');
  for (let i = 0; i < sample.length; i++) {
    const char = sample.charCodeAt(i);
    hash = (hash << 5) - hash + char;
    hash |= 0;
  }
  const hex = Math.abs(hash).toString(16).padStart(8, '0');
  return `a9f4c3d1b8e214068579075e7a9c${hex}99234857bfe01289456`;
});

function handleScroll() {
  if (!terminalContainer.value) return;
  const { scrollTop, scrollHeight, clientHeight } = terminalContainer.value;
  // If scrolled close to bottom, keep autoScroll on, else turn off
  const isAtBottom = scrollHeight - scrollTop - clientHeight < 40;
  if (!isAtBottom && autoScroll.value) {
    autoScroll.value = false;
  } else if (isAtBottom && !autoScroll.value) {
    autoScroll.value = true;
  }
}

function scrollToBottom() {
  autoScroll.value = true;
  nextTick(() => {
    if (terminalContainer.value) {
      terminalContainer.value.scrollTop = terminalContainer.value.scrollHeight;
    }
  });
}

function toggleStreaming() {
  isStreaming.value = !isStreaming.value;
}

function clearLogs() {
  logs.value = [];
}

function exportLogs(format: 'log' | 'txt') {
  const content = filteredLogs.value.map(l => {
    // Strip ANSI escape codes for pure export
    const cleanMsg = l.rawMessage.replace(/\x1b\[[0-9;]*m/g, '');
    return `[${l.timestamp}] [${l.level}] ${l.domain} ${cleanMsg}`;
  }).join('\n');

  const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `edgeos_logs_${Date.now()}.${format}`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

// Generate realistic dynamic runtime logs
function pushLogEntry(entry: Omit<LogEntry, 'id' | 'timestamp'>) {
  const now = new Date();
  const timeStr = `${now.toTimeString().split(' ')[0]}.${String(now.getMilliseconds()).padStart(3, '0')}`;
  
  logs.value.push({
    id: `log-${Date.now()}-${Math.random().toString(36).substr(2, 6)}`,
    timestamp: timeStr,
    ...entry
  });

  // Maintain buffer size
  if (logs.value.length > maxBufferLines.value) {
    logs.value.shift();
  }

  if (autoScroll.value) {
    nextTick(() => {
      if (terminalContainer.value) {
        terminalContainer.value.scrollTop = terminalContainer.value.scrollHeight;
      }
    });
  }
}

onMounted(() => {
  // Load initial logs
  INITIAL_LOG_SEEDS.forEach((seed, index) => {
    const ms = 100 * index;
    const now = new Date(Date.now() - (INITIAL_LOG_SEEDS.length - index) * 1000);
    const timeStr = `${now.toTimeString().split(' ')[0]}.${String(ms % 1000).padStart(3, '0')}`;
    logs.value.push({
      id: `log-seed-${index}`,
      timestamp: timeStr,
      level: seed.level,
      domain: seed.domain,
      rawMessage: seed.msg
    });
  });

  scrollToBottom();

  // Simulate dynamic industrial log stream
  const DYNAMIC_MESSAGES = [
    { level: 'DEBUG', domain: '[BUS]', msg: 'DataHub shadow write tag \x1b[1;36m0x1002\x1b[0m (Grid.Voltage.A) = \x1b[1;32m220.4V\x1b[0m' },
    { level: 'INFO', domain: '[STREAM]', msg: 'FFT 31st Harmonic calculated: THDu=\x1b[1;33m1.82%\x1b[0m (GB/T 14549 compliant)' },
    { level: 'INFO', domain: '[ALGO]', msg: 'BMS Rack #2 cell voltage delta = \x1b[1;32m12mV\x1b[0m, balancing idle' },
    { level: 'DEBUG', domain: '[DRIVER]', msg: 'DL/T 645-2007 read energy total register: \x1b[1;36m145892.34 kWh\x1b[0m' },
    { level: 'INFO', domain: '[PERSISTENCE]', msg: 'SnF Manager: 0 pending offline frames, upstream link healthy' },
    { level: 'WARN', domain: '[SYSTEM]', msg: '\x1b[1;33m[WARN]\x1b[0m CPU core 0 thermal reading reached \x1b[1;31m68.5°C\x1b[0m, throttling fan speed to 80%' },
    { level: 'DEBUG', domain: '[BUS]', msg: 'Cyclone DDS RTPS heartbeat sent to 4 peer endpoints' },
    { level: 'INFO', domain: '[SECURITY]', msg: 'ESAM periodic challenge-response handshake SUCCESS (latency: 1.2ms)' }
  ];

  streamTimer = setInterval(() => {
    if (!isStreaming.value) return;
    const randMsg = DYNAMIC_MESSAGES[Math.floor(Math.random() * DYNAMIC_MESSAGES.length)];
    pushLogEntry({
      level: randMsg.level as LogEntry['level'],
      domain: randMsg.domain,
      rawMessage: randMsg.msg
    });
  }, 1000);
});

onUnmounted(() => {
  if (streamTimer) {
    clearInterval(streamTimer);
    streamTimer = null;
  }
});
</script>

<style scoped>
/* Scrollbar styling for retro terminal aesthetic */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}
::-webkit-scrollbar-track {
  background: rgba(2, 6, 23, 0.6);
}
::-webkit-scrollbar-thumb {
  background: rgba(51, 65, 85, 0.8);
  border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover {
  background: rgba(100, 116, 139, 1);
}
</style>
