<template>
  <div class="h-full flex flex-col bg-[#050b14] text-[#f8fafc] font-mono text-xs overflow-hidden select-text">
    <!-- Terminal Output Screen -->
    <div 
      ref="termOutputRef" 
      class="flex-1 p-4 overflow-y-auto space-y-1 scrollbar-thin scrollbar-thumb-slate-800 scrollbar-track-transparent"
    >
      <!-- Welcome Banner -->
      <div class="text-slate-500 pb-2 border-b border-slate-900 mb-3 space-y-0.5">
        <div class="text-emerald-400 font-bold">EdgeOS Industrial Real-Time Web Interactive CLI [Version 2.4.0-RT]</div>
        <div>Type <span class="text-cyan-400 font-bold">help</span> or <span class="text-cyan-400 font-bold">?</span> for available commands. Press <kbd class="px-1 py-0.5 bg-slate-900 rounded text-slate-400 text-[10px]">Tab</kbd> for auto-completion, <kbd class="px-1 py-0.5 bg-slate-900 rounded text-slate-400 text-[10px]">↑/↓</kbd> for history.</div>
      </div>

      <!-- Command History Output Lines -->
      <div v-for="(entry, index) in historyEntries" :key="index" class="space-y-1">
        <!-- Input Command Echo -->
        <div class="flex items-center gap-2 text-slate-300">
          <span class="text-emerald-400 font-black">{{ promptPrefix }}</span>
          <span class="font-bold text-white">{{ entry.command }}</span>
          <span class="text-[10px] text-slate-600 ml-auto">{{ entry.timestamp }}</span>
        </div>
        <!-- Command Output (ANSI color formatted) -->
        <div 
          v-if="entry.output" 
          class="pl-4 text-slate-300 whitespace-pre-wrap leading-relaxed break-all font-mono"
          v-html="renderAnsiText(entry.output)"
        ></div>
      </div>
    </div>

    <!-- Terminal Command Input Bar -->
    <div class="p-3 bg-slate-950/95 border-t border-slate-800 flex items-center gap-2 relative">
      <span class="text-emerald-400 font-black shrink-0">{{ promptPrefix }}</span>
      <input 
        ref="cmdInputRef"
        v-model="currentInput" 
        @keydown.enter="executeCommand"
        @keydown.up.prevent="navigateHistory(-1)"
        @keydown.down.prevent="navigateHistory(1)"
        @keydown.tab.prevent="handleTabCompletion"
        placeholder="Enter EdgeOS CLI command (e.g. 'help', 'datahub stats', 'dds peers', 'metrics')..."
        class="flex-1 bg-transparent text-white font-mono text-xs focus:outline-none placeholder-slate-600"
        autocomplete="off"
        spellcheck="false"
      />
      <div class="flex items-center gap-2">
        <button 
          @click="clearScreen" 
          class="px-2 py-1 rounded bg-slate-900 hover:bg-slate-800 text-slate-400 hover:text-slate-200 text-[10px] transition cursor-pointer"
          title="Clear Screen (Ctrl+L)"
        >
          Clear
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, nextTick } from 'vue';

export interface HistoryEntry {
  command: string;
  output: string;
  timestamp: string;
}

const promptPrefix = ref('edgeos@control-plane:~$');
const currentInput = ref('');
const historyEntries = ref<HistoryEntry[]>([]);
const commandHistory = ref<string[]>([]);
const historyIndex = ref(-1);

const termOutputRef = ref<HTMLElement | null>(null);
const cmdInputRef = ref<HTMLInputElement | null>(null);

// Available CLI Commands for Auto-Completion
const availableCommands = [
  'help', 'clear', 'version', 'uptime',
  'datahub', 'datahub stats', 'datahub dump',
  'dds', 'dds peers', 'dds qos',
  'mtt', 'mtt clock', 'mtt jitter',
  'drivers', 'drivers list', 'drivers poll',
  'streams', 'streams dag', 'streams stats',
  'storage', 'storage wal', 'storage snf',
  'metrics', 'metrics openmetrics',
  'ping', 'top', 'ps', 'reboot'
];

function scrollToBottom() {
  nextTick(() => {
    if (termOutputRef.value) {
      termOutputRef.value.scrollTop = termOutputRef.value.scrollHeight;
    }
  });
}

function focusInput() {
  nextTick(() => {
    cmdInputRef.value?.focus();
  });
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

function handleTabCompletion() {
  const trimmed = currentInput.value.trim().toLowerCase();
  if (!trimmed) return;

  const matches = availableCommands.filter(c => c.startsWith(trimmed));
  if (matches.length === 1) {
    currentInput.value = matches[0];
  } else if (matches.length > 1) {
    // Show available completions
    historyEntries.value.push({
      command: currentInput.value,
      output: `\x1b[36mCompletions:\x1b[0m\n  ${matches.join('   ')}`,
      timestamp: new Date().toLocaleTimeString()
    });
    scrollToBottom();
  }
}

function clearScreen() {
  historyEntries.value = [];
}

// Built-in EdgeOS Command Dispatcher
function executeCommand() {
  const cmd = currentInput.value.trim();
  if (!cmd) return;

  commandHistory.value.push(cmd);
  historyIndex.value = -1;
  currentInput.value = '';

  const timestamp = new Date().toLocaleTimeString();
  const lower = cmd.toLowerCase();

  let output = '';

  if (lower === 'clear' || lower === 'cls') {
    clearScreen();
    return;
  } else if (lower === 'help' || lower === '?') {
    output = `\x1b[32m=== EdgeOS Built-in Shell Commands ===\x1b[0m
  \x1b[33mdatahub stats\x1b[0m       - Dump DataHub SHM ringbuffer watermark and latency
  \x1b[33mdds peers\x1b[0m           - Display Cyclone DDS active RTPS node topology
  \x1b[33mmtt clock\x1b[0m           - Show MTT deterministic wall-clock timestamp and jitter
  \x1b[33mdrivers list\x1b[0m        - List registered southbound protocol drivers
  \x1b[33mstreams dag\x1b[0m         - Inspect DSP / RMS / THD stream compute DAG
  \x1b[33mstorage wal\x1b[0m         - View SQLite WAL checkpoints and SnF buffer
  \x1b[33mmetrics\x1b[0m             - Export Prometheus / OpenMetrics summary
  \x1b[33mversion\x1b[0m             - Print EdgeOS and Zephyr OS kernel revision
  \x1b[33muptime\x1b[0m              - Display control plane process uptime
  \x1b[33mclear\x1b[0m               - Clear the terminal screen`;
  } else if (lower.startsWith('datahub')) {
    output = `\x1b[36m[DataHub Shadow Bus Summary]\x1b[0m
  Watermark:      \x1b[32m4.8%\x1b[0m (196 / 4096 slots)
  Read Latency:   \x1b[32m42 ns\x1b[0m (Atomic POSIX SHM)
  Throughput:     \x1b[32m24,580 TPS\x1b[0m
  Contract Tags:  \x1b[32m76 SmartBox / 30 IEMS / 7 DCU\x1b[0m
  Buffer Drops:   \x1b[32m0 (Zero-Drop Guarantee)\x1b[0m`;
  } else if (lower.startsWith('dds')) {
    output = `\x1b[36m[Cyclone DDS Mesh Status]\x1b[0m
  Domain ID:      \x1b[32m0 (Industrial Default)\x1b[0m
  Participants:   \x1b[32m4 Active Nodes\x1b[0m (DCU-01, IEMS-02, SmartBox-03, ScadaGW-01)
  Loss Rate:      \x1b[32m0.00%\x1b[0m (Reliable Transient-Local QoS)
  Throughput:     \x1b[32mRX: 14.8 KB/s · TX: 12.2 KB/s\x1b[0m
  Liveliness:     \x1b[32mALIVE (Jitter: 28 µs)\x1b[0m`;
  } else if (lower.startsWith('mtt')) {
    output = `\x1b[36m[MTT Deterministic Clock]\x1b[0m
  Wall-Clock:     \x1b[32m${new Date().toISOString()}\x1b[0m
  Periodic Step:  \x1b[32m15 min Alignment (00/15/30/45)\x1b[0m
  Timer Jitter:   \x1b[32m1.8 µs (RT-Preempt Kernel)\x1b[0m`;
  } else if (lower.startsWith('drivers')) {
    output = `\x1b[36m[Registered Drivers]\x1b[0m
  [0] dlt645_2007   \x1b[32mACTIVE\x1b[0m  (RS485 Bus 0, 9600 8E1, Poll: 1000ms)
  [1] dlt698_45     \x1b[32mACTIVE\x1b[0m  (HPLC OFDM Mesh, Hops: 2, Poll: 2000ms)
  [2] modbus_rtu    \x1b[32mACTIVE\x1b[0m  (Inverter/PCS, 19200 8N1, Poll: 500ms)
  [3] adc_dsp       \x1b[32mACTIVE\x1b[0m  (Continuous High-Freq 1000Hz Waveform)`;
  } else if (lower.startsWith('streams')) {
    output = `\x1b[36m[Stream Engine DAG Pipelines]\x1b[0m
  DAG Latency:    \x1b[32m148 µs\x1b[0m (Stages: ADC_RAW -> RMS_CALC -> THD_FFT -> DataHub)
  Queue Depth:    \x1b[32m2 / 128 (Healthy)\x1b[0m
  Sampling Rate:  \x1b[32m1,000 Hz\x1b[0m`;
  } else if (lower.startsWith('storage')) {
    output = `\x1b[36m[Telemetry Storage & WAL]\x1b[0m
  SQLite WAL:     \x1b[32m48 Checkpoint Pages\x1b[0m (Crash-Consistent)
  SnF Buffer:     \x1b[32m0 Unflushed Records\x1b[0m (Cloud Bridge Online)
  Compression:    \x1b[32mGorilla Lossless TS Compression (Ratio: 4.8:1)\x1b[0m`;
  } else if (lower.startsWith('metrics')) {
    output = `\x1b[32m# HELP edgeos_datahub_watermark_ratio DataHub SHM watermark percentage\x1b[0m
\x1b[32m# TYPE edgeos_datahub_watermark_ratio gauge\x1b[0m
edgeos_datahub_watermark_ratio 0.048
\x1b[32m# HELP edgeos_datahub_read_latency_nanoseconds Atomic SHM read latency\x1b[0m
\x1b[32m# TYPE edgeos_datahub_read_latency_nanoseconds gauge\x1b[0m
edgeos_datahub_read_latency_nanoseconds 42.0
\x1b[32m# HELP edgeos_dds_active_participants Active Cyclone DDS peers\x1b[0m
\x1b[32m# TYPE edgeos_dds_active_participants gauge\x1b[0m
edgeos_dds_active_participants 4`;
  } else if (lower === 'version') {
    output = `EdgeOS Microkernel: \x1b[32mv2.4.0-RT\x1b[0m (Git Commit \x1b[36m0c5655f\x1b[0m)
Compiler:           x86_64-linux-gnu-gcc 12.3.0
Runtime:            POSIX RT-Preempt + libuv event loop
Web Console:        Vue 3.4 + Tailwind CSS + Ansi Engine`;
  } else if (lower === 'uptime') {
    output = `Control Plane Uptime: \x1b[32m24h 38m 12s\x1b[0m (0 crashes, 0 restarts)`;
  } else {
    output = `\x1b[31mCommand not found: ${cmd}\x1b[0m. Type \x1b[33mhelp\x1b[0m for list of commands.`;
  }

  historyEntries.value.push({
    command: cmd,
    output,
    timestamp
  });

  scrollToBottom();
}

// Lightweight ANSI SGR 16-color HTML formatter
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
    executeCommand();
  },
  focus: focusInput,
  clear: clearScreen
});
</script>
