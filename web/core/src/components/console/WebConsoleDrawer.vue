<template>
  <div v-if="isOpen" class="relative z-50 select-none">
    <!-- Semi-transparent Backdrop when in full-screen or dimmed mode -->
    <div 
      v-if="drawerMode === 'fullscreen'" 
      class="fixed inset-0 bg-slate-950/80 backdrop-blur-sm transition-opacity"
      @click="closeDrawer"
    ></div>

    <!-- Main Console Container -->
    <div 
      :class="[
        drawerMode === 'fullscreen' ? 'fixed inset-4 lg:inset-8 rounded-3xl' : 
        drawerMode === 'floating' ? 'fixed bottom-6 right-6 w-[720px] max-w-[calc(100vw-3rem)] h-[480px] rounded-3xl shadow-2xl' : 
        'fixed inset-x-0 bottom-0 rounded-t-3xl shadow-2xl'
      ]"
      :style="drawerMode === 'docked' ? { height: `${drawerHeightVh}vh` } : {}"
      class="bg-[#050b14] border border-slate-800 text-[#f8fafc] flex flex-col overflow-hidden shadow-2xl transition-all duration-200"
    >
      <!-- Top Draggable Resize Bar (for Docked Mode) -->
      <div 
        v-if="drawerMode === 'docked'"
        @mousedown="startResize"
        class="h-2 w-full bg-slate-900/60 hover:bg-emerald-500/30 transition-colors cursor-row-resize flex items-center justify-center group"
      >
        <div class="w-12 h-1 bg-slate-700 group-hover:bg-emerald-400 rounded-full transition-colors"></div>
      </div>

      <!-- Header Navigation Bar -->
      <header class="bg-slate-950/90 border-b border-slate-800 px-4 lg:px-6 py-2.5 flex items-center justify-between gap-4 shrink-0 font-mono text-xs">
        <!-- Left: Product Brand & Active Tabs -->
        <div class="flex items-center gap-3">
          <div class="flex items-center gap-2 pr-3 border-r border-slate-800 text-emerald-400 font-bold">
            <span class="w-2 h-2 rounded-full bg-emerald-400 animate-ping"></span>
            <span class="hidden sm:inline">EdgeOS Global Web Console</span>
            <span class="sm:hidden">Console</span>
          </div>

          <!-- Tabs -->
          <div class="flex items-center gap-1 bg-slate-900/90 p-1 rounded-xl border border-slate-800">
            <button 
              v-for="tab in consoleTabs" 
              :key="tab.id"
              @click="activeTab = tab.id"
              :class="activeTab === tab.id ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 font-bold' : 'text-slate-400 hover:text-slate-200 border-transparent'"
              class="px-3 py-1 rounded-lg border transition flex items-center gap-1.5 cursor-pointer text-xs"
            >
              <span>{{ tab.icon }}</span>
              <span>{{ tab.name }}</span>
            </button>
          </div>
        </div>

        <!-- Right: Window Controls & Hotkey Hint -->
        <div class="flex items-center gap-2">
          <span class="hidden md:inline text-slate-500 text-[10px]">Toggle: <kbd class="px-1 py-0.5 bg-slate-900 rounded border border-slate-800 text-slate-400">Ctrl + `</kbd></span>

          <!-- Mode Toggles -->
          <button 
            @click="cycleDrawerMode" 
            class="p-1.5 rounded-lg bg-slate-900 hover:bg-slate-800 text-slate-400 hover:text-slate-200 border border-slate-800 transition cursor-pointer"
            :title="`Current mode: ${drawerMode}. Click to switch mode.`"
          >
            <span v-if="drawerMode === 'docked'">🗖 Fullscreen</span>
            <span v-else-if="drawerMode === 'fullscreen'">🗗 Floating</span>
            <span v-else>🗕 Docked</span>
          </button>

          <!-- Close Button -->
          <button 
            @click="closeDrawer" 
            class="w-7 h-7 rounded-lg bg-slate-900 hover:bg-rose-900/40 hover:text-rose-400 text-slate-400 flex items-center justify-center transition cursor-pointer border border-slate-800"
          >
            ✕
          </button>
        </div>
      </header>

      <!-- Main Tab Content Area -->
      <main class="flex-1 min-h-0 relative overflow-hidden">
        <!-- TAB 1: CLI Shell Terminal -->
        <div v-show="activeTab === 'cli'" class="h-full">
          <InteractiveShell ref="terminalViewRef" />
        </div>

        <!-- TAB 2: Live Log Stream Console -->
        <div v-show="activeTab === 'logs'" class="h-full flex flex-col bg-[#050b14] p-4 font-mono text-xs overflow-hidden">
          <!-- Log Filter Bar -->
          <div class="flex items-center justify-between pb-3 border-b border-slate-800 text-xs">
            <div class="flex items-center gap-2">
              <span class="text-slate-500">Filter Level:</span>
              <select v-model="logLevelFilter" class="bg-slate-900 text-slate-200 px-2 py-1 rounded border border-slate-800">
                <option value="ALL">ALL (Debug..Fatal)</option>
                <option value="INFO">INFO & Above</option>
                <option value="WARN">WARN & Above</option>
                <option value="ERROR">ERROR / FATAL Only</option>
              </select>
            </div>
            <div class="flex items-center gap-2">
              <span class="text-emerald-400 font-bold">● Streaming (50Hz)</span>
              <button @click="liveLogs = []" class="px-2 py-1 rounded bg-slate-900 hover:bg-slate-800 text-slate-400 text-[10px]">Clear</button>
            </div>
          </div>
          <!-- Log Lines Container -->
          <div class="flex-1 overflow-y-auto py-2 space-y-1 scrollbar-thin">
            <div 
              v-for="(log, idx) in filteredLogs" 
              :key="idx" 
              class="flex items-start gap-2 text-[11px] leading-relaxed"
            >
              <span class="text-slate-600 shrink-0">{{ log.time }}</span>
              <span 
                class="px-1 rounded text-[9px] font-bold uppercase shrink-0"
                :class="getLogLevelClass(log.level)"
              >
                {{ log.level }}
              </span>
              <span class="text-cyan-400 font-bold shrink-0">[{{ log.module }}]</span>
              <span class="text-slate-300 break-all">{{ log.message }}</span>
            </div>
          </div>
        </div>

        <!-- TAB 3: Protocol Packet Sniffer -->
        <div v-show="activeTab === 'sniffer'" class="h-full">
          <ProtocolSnifferView />
        </div>

        <!-- TAB 4: Quick Macros -->
        <div v-show="activeTab === 'macros'" class="h-full bg-[#050b14] p-6 font-mono text-xs overflow-y-auto space-y-6">
          <div class="text-slate-400">
            <h3 class="text-sm font-bold text-white mb-1">Industrial Quick Diagnosis Macros</h3>
            <p class="text-[11px]">Click a macro to execute pre-compiled diagnosis and control actions across EdgeOS four planes.</p>
          </div>

          <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
            <div 
              v-for="macro in quickMacros" 
              :key="macro.id"
              @click="runMacro(macro)"
              class="bg-slate-950 border border-slate-800 hover:border-emerald-500/50 p-4 rounded-2xl cursor-pointer transition-all hover:scale-[1.02] shadow-lg group space-y-2"
            >
              <div class="flex items-center justify-between">
                <span class="text-lg">{{ macro.icon }}</span>
                <span class="text-[10px] px-2 py-0.5 rounded bg-slate-900 text-slate-400 font-bold uppercase">{{ macro.plane }}</span>
              </div>
              <div class="font-bold text-white group-hover:text-emerald-400 transition-colors">{{ macro.name }}</div>
              <p class="text-[11px] text-slate-500 leading-relaxed">{{ macro.description }}</p>
              <div class="text-[10px] text-cyan-400 font-mono pt-1">CMD: {{ macro.command }}</div>
            </div>
          </div>
        </div>
      </main>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import InteractiveShell from './InteractiveShell.vue';
import ProtocolSnifferView from './ProtocolSnifferView.vue';
import AnsiTerminalView from './AnsiTerminalView.vue';

export interface ConsoleTab {
  id: 'cli' | 'logs' | 'sniffer' | 'macros';
  name: string;
  icon: string;
}

export interface QuickMacro {
  id: string;
  name: string;
  icon: string;
  plane: string;
  description: string;
  command: string;
}

export interface LiveLog {
  time: string;
  level: 'DEBUG' | 'INFO' | 'WARN' | 'ERROR' | 'FATAL';
  module: string;
  message: string;
}

export interface PacketItem {
  time: string;
  direction: 'TX' | 'RX';
  protocol: string;
  channel: string;
  hexData: string;
  decodedSummary: string;
}

const props = defineProps<{
  isOpen: boolean;
}>();

const emit = defineEmits<{
  (e: 'update:isOpen', value: boolean): void;
}>();

const activeTab = ref<'cli' | 'logs' | 'sniffer' | 'macros'>('cli');
const drawerMode = ref<'docked' | 'fullscreen' | 'floating'>('docked');
const drawerHeightVh = ref(45);
const logLevelFilter = ref('ALL');

const terminalViewRef = ref<InstanceType<typeof InteractiveShell> | null>(null);

const consoleTabs: ConsoleTab[] = [
  { id: 'cli', name: 'CLI Terminal', icon: '🖥️' },
  { id: 'logs', name: 'Live Logs', icon: '📟' },
  { id: 'sniffer', name: 'Packet Sniffer', icon: '📡' },
  { id: 'macros', name: 'Quick Macros', icon: '⚡' }
];

const quickMacros: QuickMacro[] = [
  {
    id: 'macro-datahub',
    name: 'Dump DataHub Ringbuffer',
    icon: '📊',
    plane: 'DataHub',
    description: 'Inspect SPSC slots watermark, atomic nanosecond read latency, and zero-drop metrics.',
    command: 'datahub stats'
  },
  {
    id: 'macro-dds',
    name: 'Check Cyclone DDS Mesh',
    icon: '🌐',
    plane: 'DDS Mesh',
    description: 'Query discovered RTPS participants, network throughput, and QoS liveliness state.',
    command: 'dds peers'
  },
  {
    id: 'macro-mtt',
    name: 'Query MTT Clock Precision',
    icon: '🕒',
    plane: 'Control',
    description: 'Verify 15min deterministic wall-clock alignment and RT-Preempt kernel jitter.',
    command: 'mtt clock'
  },
  {
    id: 'macro-drivers',
    name: 'Inspect Southbound Drivers',
    icon: '🔌',
    plane: 'Execution',
    description: 'Scan active DL/T 645, DL/T 698, and Modbus serial bus polling loops.',
    command: 'drivers list'
  },
  {
    id: 'macro-storage',
    name: 'Check SQLite WAL & SnF',
    icon: '💾',
    plane: 'Storage',
    description: 'Audit crash-consistent WAL page checkpoints and offline buffering storage.',
    command: 'storage wal'
  },
  {
    id: 'macro-metrics',
    name: 'Export OpenMetrics Payload',
    icon: '🎛️',
    plane: 'Observability',
    description: 'Fetch Prometheus exposition format for external Grafana / Cloud scada scrape.',
    command: 'metrics'
  }
];

const liveLogs = ref<LiveLog[]>([
  { time: '21:05:01.120', level: 'INFO', module: 'BUS', message: 'Cyclone DDS RTPS participant discovered node IEMS-02 on domain 0' },
  { time: '21:05:01.240', level: 'INFO', module: 'DRIVER', message: 'DLT698.45 HPLC carrier polling loop executed for 16 meters (Latency: 42ms)' },
  { time: '21:05:01.500', level: 'DEBUG', module: 'STREAM', message: 'DSP RMS/THD pipeline completed 1000Hz frame computation' },
  { time: '21:05:02.100', level: 'WARN', module: 'CHANNEL', message: 'Phase B to C capacitive coupling crosstalk at 21.6dB exceeds 18dB caution threshold' }
]);

const packetStream = ref<PacketItem[]>([
  {
    time: '21:05:02.100',
    direction: 'TX',
    protocol: 'DLT645-2007',
    channel: 'RS485-0',
    hexData: '68 11 4D 3C 2B 1A 00 68 11 04 33 33 34 33 58 16',
    decodedSummary: 'Read Tag: Phase A Voltage (0x02010100)'
  },
  {
    time: '21:05:02.142',
    direction: 'RX',
    protocol: 'DLT645-2007',
    channel: 'RS485-0',
    hexData: '68 11 4D 3C 2B 1A 00 68 91 06 33 33 34 33 85 45 B2 16',
    decodedSummary: 'Response: 220.5 V (Success)'
  }
]);

const filteredLogs = computed(() => {
  if (logLevelFilter.value === 'ALL') return liveLogs.value;
  if (logLevelFilter.value === 'INFO') return liveLogs.value.filter(l => l.level !== 'DEBUG');
  if (logLevelFilter.value === 'WARN') return liveLogs.value.filter(l => l.level === 'WARN' || l.level === 'ERROR' || l.level === 'FATAL');
  return liveLogs.value.filter(l => l.level === 'ERROR' || l.level === 'FATAL');
});

function closeDrawer() {
  emit('update:isOpen', false);
}

function cycleDrawerMode() {
  if (drawerMode.value === 'docked') drawerMode.value = 'fullscreen';
  else if (drawerMode.value === 'fullscreen') drawerMode.value = 'floating';
  else drawerMode.value = 'docked';
}

function runMacro(macro: QuickMacro) {
  activeTab.value = 'cli';
  terminalViewRef.value?.executeCommandDirect(macro.command);
}

function getLogLevelClass(level: LiveLog['level']) {
  switch (level) {
    case 'DEBUG': return 'bg-slate-800 text-slate-400';
    case 'INFO': return 'bg-blue-500/20 text-blue-400';
    case 'WARN': return 'bg-amber-500/20 text-amber-400';
    case 'ERROR': return 'bg-rose-500/20 text-rose-400';
    case 'FATAL': return 'bg-purple-500/20 text-purple-400';
  }
}

// Drag to Resize Docked Drawer Height
let isResizing = false;
function startResize(e: MouseEvent) {
  isResizing = true;
  document.addEventListener('mousemove', handleResize);
  document.addEventListener('mouseup', stopResize);
}

function handleResize(e: MouseEvent) {
  if (!isResizing) return;
  const newVh = ((window.innerHeight - e.clientY) / window.innerHeight) * 100;
  drawerHeightVh.value = Math.min(85, Math.max(25, Math.round(newVh)));
}

function stopResize() {
  isResizing = false;
  document.removeEventListener('mousemove', handleResize);
  document.removeEventListener('mouseup', stopResize);
}

onMounted(() => {
  // Global Hotkey Listener: Ctrl + ` or Alt + D
  const onKeyDown = (e: KeyboardEvent) => {
    if ((e.ctrlKey && (e.key === '`' || e.key === '~')) || (e.altKey && (e.key === 'd' || e.key === 'D'))) {
      e.preventDefault();
      emit('update:isOpen', !props.isOpen);
    }
  };
  window.addEventListener('keydown', onKeyDown);
});
</script>
