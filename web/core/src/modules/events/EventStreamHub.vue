<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-rose-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-cyan-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-rose-500/10 border border-rose-500/20 text-rose-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-rose-400 animate-ping"></span>
            Millisecond SOE & High-Frequency Stream Hub
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight font-mono">
            GLOBAL <span class="text-rose-400 font-sans">SOE EVENT STREAM</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            High-concurrency Sequence of Events (SOE) pipeline, multi-domain alarm waterfall, and acknowledgement audit center.
          </p>
        </div>

        <!-- Global Status Badges & Controls -->
        <div class="flex flex-wrap items-center gap-3 font-mono text-xs">
          <div class="bg-slate-950/90 border border-slate-800 px-3.5 py-2 rounded-2xl flex items-center gap-2.5 shadow-inner">
            <span class="text-slate-500">Stream Rate:</span>
            <span class="text-cyan-400 font-bold">{{ streamRateEps }} eps</span>
          </div>
          <div class="bg-slate-950/90 border border-slate-800 px-3.5 py-2 rounded-2xl flex items-center gap-2.5 shadow-inner">
            <span class="text-slate-500">Unacked:</span>
            <span :class="unackedCount > 0 ? 'text-rose-400 font-black animate-pulse' : 'text-emerald-400 font-bold'">
              {{ unackedCount }} ALARMS
            </span>
          </div>
          <button 
            @click="toggleStream" 
            :class="isStreaming ? 'bg-cyan-500 hover:bg-cyan-600 text-slate-950 font-black' : 'bg-slate-800 hover:bg-slate-700 text-slate-300'"
            class="px-4 py-2.5 rounded-2xl border border-slate-700 transition flex items-center gap-2 shadow-lg"
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
        <!-- Domain Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Domain:</span>
          <select v-model="filterDomain" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="ALL">ALL (全部域空间)</option>
            <option value="IEMS_EMS">IEMS (微电网能量管理)</option>
            <option value="SMARTBOX_SAFETY">SMARTBOX (智能箱变五防)</option>
            <option value="DCU_CHANNEL">DCU (台区采集网关)</option>
            <option value="DDS_TOPOLOGY">DDS (分布式网状通信)</option>
          </select>
        </div>

        <!-- Severity Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Severity:</span>
          <select v-model="filterSeverity" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="ALL">ALL (全部级别)</option>
            <option value="CRITICAL">💥 CRITICAL (严重告警/跳闸)</option>
            <option value="WARNING">⚠️ WARNING (越限预警)</option>
            <option value="INFO">ℹ️ INFO (遥信/状态变位)</option>
            <option value="ACTION">🔀 ACTION (控制/换相动作)</option>
          </select>
        </div>

        <!-- ACK Status Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Status:</span>
          <select v-model="filterAck" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer font-bold">
            <option value="ALL">ALL (全部状态)</option>
            <option value="UNACK">UNACKNOWLEDGED (待确认)</option>
            <option value="ACKED">ACKNOWLEDGED (已确认)</option>
          </select>
        </div>

        <!-- Search Box -->
        <div class="relative">
          <input 
            v-model="searchQuery" 
            type="text" 
            placeholder="Search Tag, OAD, Device..." 
            class="bg-slate-950 border border-slate-800 rounded-xl px-3 py-1.5 text-xs text-slate-200 placeholder-slate-600 focus:outline-none focus:border-rose-500 w-48"
          />
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="flex items-center gap-2.5">
        <button 
          @click="batchAcknowledgeAll" 
          :disabled="unackedCount === 0"
          :class="unackedCount > 0 ? 'bg-amber-600 hover:bg-amber-500 text-white' : 'bg-slate-950 text-slate-600 cursor-not-allowed'"
          class="px-3.5 py-2 rounded-xl border border-slate-700 transition font-bold flex items-center gap-1.5 shadow"
        >
          <span>✓ 批量全部确认</span>
        </button>
        <button 
          @click="exportEventsCSV" 
          class="px-3.5 py-2 bg-slate-950 hover:bg-slate-800 text-slate-300 border border-slate-800 rounded-xl transition flex items-center gap-1.5"
        >
          <span>📥 导出 CSV</span>
        </button>
        <button 
          @click="exportEventsJSON" 
          class="px-3.5 py-2 bg-slate-950 hover:bg-slate-800 text-slate-300 border border-slate-800 rounded-xl transition flex items-center gap-1.5"
        >
          <span>📄 JSON 报表</span>
        </button>
        <button 
          @click="clearAllEvents" 
          class="px-3 py-2 bg-slate-950 hover:bg-rose-950 text-slate-500 hover:text-rose-400 border border-slate-800 rounded-xl transition"
          title="清空当前事件缓存"
        >
          🗑️
        </button>
      </div>
    </div>

    <!-- Main SOE Event Stream Table -->
    <div class="bg-slate-900/90 border border-slate-800/90 rounded-3xl overflow-hidden shadow-2xl">
      <div class="overflow-x-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead>
            <tr class="bg-slate-950 border-b border-slate-800 text-slate-400 uppercase text-[10px] tracking-wider">
              <th class="p-4 w-12 text-center">#</th>
              <th class="p-4 w-32">SOE Timestamp (MTT)</th>
              <th class="p-4 w-28">Severity</th>
              <th class="p-4 w-36">Domain Source</th>
              <th class="p-4 w-48">Device / OAD</th>
              <th class="p-4">Event Description & Telemetry Delta</th>
              <th class="p-4 w-28 text-center">Status</th>
              <th class="p-4 w-32 text-center">Action</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-800/60">
            <tr 
              v-for="(evt, idx) in filteredEvents" 
              :key="evt.id"
              @click="openDetail(evt)"
              class="hover:bg-slate-800/50 cursor-pointer transition-colors group"
              :class="[
                evt.severity === 'CRITICAL' && !evt.acknowledged ? 'bg-rose-950/20 border-l-4 border-l-rose-500' : '',
                selectedEvent?.id === evt.id ? 'bg-slate-800/80' : ''
              ]"
            >
              <td class="p-4 text-slate-600 text-center">{{ idx + 1 }}</td>
              
              <!-- Timestamp (HH:mm:ss.SSS) -->
              <td class="p-4 whitespace-nowrap text-slate-300 font-bold">
                <div>{{ evt.timestamp }}</div>
                <div class="text-[9px] text-slate-500 font-normal mt-0.5">{{ evt.relativeDelta }}</div>
              </td>

              <!-- Severity Badge -->
              <td class="p-4 whitespace-nowrap">
                <span 
                  :class="[
                    'px-2.5 py-1 rounded-lg text-[10px] font-black border uppercase inline-flex items-center gap-1.5',
                    getSeverityBadgeClass(evt.severity)
                  ]"
                >
                  <span class="w-1.5 h-1.5 rounded-full" :class="getSeverityDotClass(evt.severity)"></span>
                  {{ evt.severity }}
                </span>
              </td>

              <!-- Domain -->
              <td class="p-4 whitespace-nowrap">
                <span class="text-slate-300 bg-slate-950 px-2 py-0.5 rounded border border-slate-800 text-[11px]">
                  {{ evt.domain }}
                </span>
              </td>

              <!-- Device / OAD -->
              <td class="p-4 whitespace-nowrap">
                <div class="text-white font-bold">{{ evt.deviceName }}</div>
                <div class="text-[10px] text-cyan-400 font-mono">{{ evt.oadCode }}</div>
              </td>

              <!-- Description & Value -->
              <td class="p-4 leading-relaxed">
                <div class="text-slate-200 font-medium">{{ evt.description }}</div>
                <div class="text-[11px] text-slate-400 font-mono mt-0.5 flex flex-wrap items-center gap-2">
                  <span v-if="evt.oldValue" class="text-slate-500 line-through">{{ evt.oldValue }}</span>
                  <span v-if="evt.oldValue" class="text-slate-600">&rarr;</span>
                  <span class="text-cyan-300 font-bold">{{ evt.currentValue }}</span>
                  <span v-if="evt.threshold" class="text-rose-400 text-[10px] bg-rose-500/10 px-1.5 py-0.5 rounded border border-rose-500/20">
                    [Limit: {{ evt.threshold }}]
                  </span>
                </div>
              </td>

              <!-- Status -->
              <td class="p-4 text-center whitespace-nowrap">
                <span 
                  class="px-2 py-0.5 rounded text-[10px] font-bold"
                  :class="evt.acknowledged ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30' : 'bg-rose-500/10 text-rose-400 border border-rose-500/30 animate-pulse'"
                >
                  {{ evt.acknowledged ? 'ACKED ✓' : 'UNACK ⚠️' }}
                </span>
              </td>

              <!-- Action -->
              <td class="p-4 text-center whitespace-nowrap">
                <div class="flex items-center justify-center gap-1.5" @click.stop>
                  <button 
                    v-if="!evt.acknowledged"
                    @click="acknowledgeEvent(evt)" 
                    class="px-2.5 py-1 bg-amber-500/20 hover:bg-amber-500/40 text-amber-300 border border-amber-500/30 rounded-lg text-[10px] font-bold transition"
                  >
                    确认
                  </button>
                  <button 
                    @click="openDetail(evt)" 
                    class="px-2 py-1 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg text-[10px] transition"
                  >
                    详情
                  </button>
                </div>
              </td>
            </tr>

            <tr v-if="filteredEvents.length === 0">
              <td colspan="8" class="text-center py-16 text-slate-500 font-mono">
                <div class="text-2xl mb-2">🔍</div>
                <div>No SOE events found matching the filter criteria.</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Event Detail Drawer / Modal -->
    <div v-if="selectedEvent" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-in fade-in duration-200">
      <div class="bg-slate-900 border border-slate-700 rounded-3xl p-6 lg:p-8 max-w-xl w-full shadow-2xl relative font-mono text-xs space-y-6">
        <div class="flex justify-between items-start border-b border-slate-800 pb-4">
          <div>
            <div class="text-[10px] text-cyan-400 uppercase font-bold tracking-widest">
              SOE Sequence of Events Record #{{ selectedEvent.id }}
            </div>
            <h2 class="text-xl font-black text-white mt-1">
              {{ selectedEvent.description }}
            </h2>
          </div>
          <button @click="selectedEvent = null" class="text-slate-400 hover:text-white text-lg">✕</button>
        </div>

        <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-2.5">
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">Timestamp (MTT High-Res):</span>
            <span class="text-slate-200 font-bold">{{ selectedEvent.timestamp }}</span>
          </div>
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">Domain / Subsystem:</span>
            <span class="text-cyan-400 font-bold">{{ selectedEvent.domain }}</span>
          </div>
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">Device ID / Name:</span>
            <span class="text-slate-200 font-bold">{{ selectedEvent.deviceName }}</span>
          </div>
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">DL/T 698 OAD / Tag Identifier:</span>
            <span class="text-amber-400 font-bold">{{ selectedEvent.oadCode }}</span>
          </div>
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">Severity Level:</span>
            <span :class="getSeverityTextClass(selectedEvent.severity)" class="font-bold">{{ selectedEvent.severity }}</span>
          </div>
          <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
            <span class="text-slate-500">Trigger Value:</span>
            <span class="text-rose-400 font-bold">{{ selectedEvent.currentValue }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-slate-500">Audit Status:</span>
            <span :class="selectedEvent.acknowledged ? 'text-emerald-400' : 'text-rose-400'" class="font-bold">
              {{ selectedEvent.acknowledged ? 'Acknowledged (已确认)' : 'Pending Operator Review (未确认)' }}
            </span>
          </div>
        </div>

        <div class="flex items-center justify-between pt-2">
          <button 
            v-if="!selectedEvent.acknowledged"
            @click="acknowledgeEvent(selectedEvent)" 
            class="px-5 py-2.5 bg-emerald-600 hover:bg-emerald-500 text-white font-bold rounded-xl shadow-lg transition"
          >
            ✓ 确认并消除告警
          </button>
          <div v-else class="text-slate-500 text-xs">
            该告警已由系统运维操作员确认。
          </div>

          <button @click="selectedEvent = null" class="px-5 py-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 font-bold">
            关闭
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

export interface SOEEvent {
  id: number;
  timestamp: string;
  relativeDelta: string;
  severity: 'CRITICAL' | 'WARNING' | 'INFO' | 'ACTION';
  domain: 'IEMS_EMS' | 'SMARTBOX_SAFETY' | 'DCU_CHANNEL' | 'DDS_TOPOLOGY';
  deviceName: string;
  oadCode: string;
  description: string;
  oldValue?: string;
  currentValue: string;
  threshold?: string;
  acknowledged: boolean;
}

const isStreaming = ref(true);
const streamRateEps = ref(124);
const filterDomain = ref('ALL');
const filterSeverity = ref('ALL');
const filterAck = ref('ALL');
const searchQuery = ref('');
const selectedEvent = ref<SOEEvent | null>(null);
let streamInterval: any = null;

const events = ref<SOEEvent[]>([
  {
    id: 1001,
    timestamp: '16:40:12.314',
    relativeDelta: 'just now',
    severity: 'CRITICAL',
    domain: 'DCU_CHANNEL',
    deviceName: 'METER_BRANCH_04',
    oadCode: '0x3011 (OUTAGE_LASTGASP)',
    description: '电表掉电事件：最后一公报告警触发 (Power Outage Last-Gasp)',
    oldValue: '223.5 V',
    currentValue: '0.0 V',
    threshold: '< 0.1 Un',
    acknowledged: false
  },
  {
    id: 1002,
    timestamp: '16:40:11.982',
    relativeDelta: '+332ms',
    severity: 'WARNING',
    domain: 'DCU_CHANNEL',
    deviceName: 'FEEDER_BUS_A',
    oadCode: '0x3005 (3PHASE_UNBALANCE)',
    description: '台区三相电流不平衡度越限 (GB/T 15543)',
    oldValue: '8.4%',
    currentValue: '38.5%',
    threshold: '> 15.0%',
    acknowledged: false
  },
  {
    id: 1003,
    timestamp: '16:40:10.150',
    relativeDelta: '+2.16s',
    severity: 'ACTION',
    domain: 'DCU_CHANNEL',
    deviceName: 'COMMUTATION_SWITCH_102',
    oadCode: '0x4002 (PHASE_SWITCH_EXEC)',
    description: '智能换相开关执行迁移遥控：A相 (24.2A) -> C相',
    oldValue: 'Phase A',
    currentValue: 'Phase C',
    acknowledged: true
  },
  {
    id: 1004,
    timestamp: '16:40:08.500',
    relativeDelta: '+3.81s',
    severity: 'CRITICAL',
    domain: 'SMARTBOX_SAFETY',
    deviceName: 'TRANSFORMER_WINDING_01',
    oadCode: '0x3015 (OVERTEMPERATURE)',
    description: '箱变高压侧绕组油温越限告警',
    oldValue: '88.5 ℃',
    currentValue: '96.2 ℃',
    threshold: '> 95.0 ℃',
    acknowledged: false
  },
  {
    id: 1005,
    timestamp: '16:40:05.120',
    relativeDelta: '+7.19s',
    severity: 'ACTION',
    domain: 'SMARTBOX_SAFETY',
    deviceName: 'SAFETY_INTERLOCK_BOX',
    oadCode: '0x2001 (FOUR_EYES_AUTH)',
    description: '电气五防双人复核指令下发通过 (Four-Eyes Principle)',
    currentValue: 'PIN Verified (Op: 101, Sup: 202)',
    acknowledged: true
  },
  {
    id: 1006,
    timestamp: '16:40:01.000',
    relativeDelta: '+11.31s',
    severity: 'INFO',
    domain: 'IEMS_EMS',
    deviceName: 'TOU_ARBITRAGE_ENGINE',
    oadCode: '0x4000 (TARIFF_SHIFT)',
    description: '分时电价时段切换：平段 -> 尖峰段 (TOU Arbitrage Schedule Shift)',
    oldValue: 'FLAT (0.75 ¥/kWh)',
    currentValue: 'PEAK (1.32 ¥/kWh)',
    acknowledged: true
  },
  {
    id: 1007,
    timestamp: '16:39:55.420',
    relativeDelta: '+16.89s',
    severity: 'WARNING',
    domain: 'IEMS_EMS',
    deviceName: 'GRID_TIE_POINT',
    oadCode: '0x1003 (DEMAND_PEAK_SHAVING)',
    description: '15分钟滚动需量削峰预警：变压器负荷率达 88.4%',
    oldValue: '380.0 kVA',
    currentValue: '442.0 kVA',
    threshold: '> 400.0 kVA',
    acknowledged: true
  },
  {
    id: 1008,
    timestamp: '16:39:48.810',
    relativeDelta: '+23.50s',
    severity: 'INFO',
    domain: 'DDS_TOPOLOGY',
    deviceName: 'DDS_MESH_DISCOVERY',
    oadCode: '0x5001 (NODE_JOIN)',
    description: 'Cyclone DDS 动态自发现新边缘节点加入网状总线',
    currentValue: 'Node GUID: 010f384a (IEMS_RACK_02)',
    acknowledged: true
  }
]);

const unackedCount = computed(() => events.value.filter(e => !e.acknowledged).length);

const filteredEvents = computed(() => {
  return events.value.filter(evt => {
    if (filterDomain.value !== 'ALL' && evt.domain !== filterDomain.value) return false;
    if (filterSeverity.value !== 'ALL' && evt.severity !== filterSeverity.value) return false;
    if (filterAck.value === 'UNACK' && evt.acknowledged) return false;
    if (filterAck.value === 'ACKED' && !evt.acknowledged) return false;
    if (searchQuery.value.trim() !== '') {
      const q = searchQuery.value.toLowerCase();
      const matchText = `${evt.deviceName} ${evt.oadCode} ${evt.description} ${evt.currentValue}`.toLowerCase();
      if (!matchText.includes(q)) return false;
    }
    return true;
  });
});

function getSeverityBadgeClass(sev: string): string {
  switch (sev) {
    case 'CRITICAL':
      return 'bg-rose-500/10 border-rose-500/30 text-rose-400';
    case 'WARNING':
      return 'bg-amber-500/10 border-amber-500/30 text-amber-400';
    case 'ACTION':
      return 'bg-purple-500/10 border-purple-500/30 text-purple-400';
    default:
      return 'bg-cyan-500/10 border-cyan-500/30 text-cyan-400';
  }
}

function getSeverityDotClass(sev: string): string {
  switch (sev) {
    case 'CRITICAL': return 'bg-rose-500 animate-ping';
    case 'WARNING': return 'bg-amber-400';
    case 'ACTION': return 'bg-purple-400';
    default: return 'bg-cyan-400';
  }
}

function getSeverityTextClass(sev: string): string {
  switch (sev) {
    case 'CRITICAL': return 'text-rose-400';
    case 'WARNING': return 'text-amber-400';
    case 'ACTION': return 'text-purple-400';
    default: return 'text-cyan-400';
  }
}

function toggleStream() {
  isStreaming.value = !isStreaming.value;
}

function openDetail(evt: SOEEvent) {
  selectedEvent.value = evt;
}

function acknowledgeEvent(evt: SOEEvent) {
  evt.acknowledged = true;
}

function batchAcknowledgeAll() {
  events.value.forEach(e => {
    e.acknowledged = true;
  });
}

function clearAllEvents() {
  events.value = [];
}

function exportEventsCSV() {
  const header = "ID,Timestamp,Severity,Domain,Device,OAD,Description,OldValue,CurrentValue,Status\n";
  const rows = events.value.map(e => 
    `"${e.id}","${e.timestamp}","${e.severity}","${e.domain}","${e.deviceName}","${e.oadCode}","${e.description}","${e.oldValue || ''}","${e.currentValue}","${e.acknowledged ? 'ACKED' : 'UNACK'}"`
  ).join("\n");

  const blob = new Blob([header + rows], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.setAttribute('href', url);
  link.setAttribute('download', `EdgeOS_SOE_Events_${Date.now()}.csv`);
  link.click();
}

function exportEventsJSON() {
  const blob = new Blob([JSON.stringify(events.value, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.setAttribute('href', url);
  link.setAttribute('download', `EdgeOS_SOE_Events_${Date.now()}.json`);
  link.click();
}

onMounted(() => {
  streamInterval = setInterval(() => {
    if (!isStreaming.value) return;
    // Simulate real-time stream jitter
    streamRateEps.value = 110 + Math.floor(Math.random() * 30);
  }, 1000);
});

onBeforeUnmount(() => {
  if (streamInterval) clearInterval(streamInterval);
});
</script>
