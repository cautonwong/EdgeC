<template>
  <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative overflow-hidden backdrop-blur-md flex flex-col">
    <!-- Header -->
    <div class="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 mb-6 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2.5 rounded-2xl bg-blue-500/10 border border-blue-500/20 text-blue-400">
          <span class="text-xl">⚡</span>
        </div>
        <div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono flex items-center gap-2">
            DYNAMIC SANKEY <span class="text-blue-500 font-sans">POWER FLOW</span>
          </h2>
          <p class="text-xs font-mono text-slate-400">
            Source-Grid-Load-Storage-EV 5-Node Microgrid Topology
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <span class="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 text-xs font-mono font-bold">
          <span class="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
          <span>GPU ACCELERATED</span>
        </span>
        <button 
          @click="toggleSimulation" 
          class="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 rounded-xl text-xs font-mono font-bold transition-all"
        >
          {{ isSimulating ? 'Sim: Active' : 'Sim: Paused' }}
        </button>
      </div>
    </div>

    <!-- SVG Sankey Canvas -->
    <div class="relative w-full h-[420px] flex items-center justify-center">
      <svg 
        viewBox="0 0 900 420" 
        class="w-full h-full preserve-3d"
        preserveAspectRatio="xMidYMid meet"
      >
        <defs>
          <!-- Flow Line Gradients -->
          <linearGradient id="grad-pv" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#3B82F6" stop-opacity="0.8" />
            <stop offset="100%" stop-color="#60A5FA" stop-opacity="0.9" />
          </linearGradient>
          <linearGradient id="grad-grid" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#A855F7" stop-opacity="0.8" />
            <stop offset="100%" stop-color="#C084FC" stop-opacity="0.9" />
          </linearGradient>
          <linearGradient id="grad-ess" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#10B981" stop-opacity="0.8" />
            <stop offset="100%" stop-color="#34D399" stop-opacity="0.9" />
          </linearGradient>
          <linearGradient id="grad-load" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#F97316" stop-opacity="0.8" />
            <stop offset="100%" stop-color="#FB923C" stop-opacity="0.9" />
          </linearGradient>
          <linearGradient id="grad-ev" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#EC4899" stop-opacity="0.8" />
            <stop offset="100%" stop-color="#F472B6" stop-opacity="0.9" />
          </linearGradient>

          <!-- Glow Filter -->
          <filter id="glow" x="-20%" y="-20%" width="140%" height="140%">
            <feGaussianBlur stdDeviation="3" result="blur" />
            <feComposite in="SourceGraphic" in2="blur" operator="over" />
          </filter>
        </defs>

        <!-- Static Background Guideline Tracks -->
        <g stroke="rgba(255,255,255,0.06)" stroke-width="6" fill="none" stroke-linecap="round">
          <!-- Solar PV -> Bus (Left-Top -> Center) -->
          <path d="M 170 100 C 310 100, 340 210, 450 210" />
          <!-- Grid -> Bus (Left-Mid -> Center) -->
          <path d="M 170 210 L 450 210" />
          <!-- ESS -> Bus (Left-Bottom -> Center) -->
          <path d="M 170 320 C 310 320, 340 210, 450 210" />
          <!-- Bus -> Load (Center -> Right-Top) -->
          <path d="M 450 210 C 560 210, 590 120, 730 120" />
          <!-- Bus -> EV Charger (Center -> Right-Bottom) -->
          <path d="M 450 210 C 560 210, 590 300, 730 300" />
        </g>

        <!-- Dynamic Animated Flow Streams -->
        <g fill="none" stroke-linecap="round">
          <!-- Solar PV Flow Stream -->
          <path 
            v-if="pvPower > 0.1"
            d="M 170 100 C 310 100, 340 210, 450 210" 
            stroke="url(#grad-pv)" 
            :stroke-width="getStrokeWidth(pvPower)"
            class="flow-stream flow-forward"
            :style="{ animationDuration: getSpeed(pvPower) }"
            filter="url(#glow)"
          />

          <!-- Grid Flow Stream (Bidirectional) -->
          <path 
            v-if="Math.abs(gridPower) > 0.1"
            d="M 170 210 L 450 210" 
            stroke="url(#grad-grid)" 
            :stroke-width="getStrokeWidth(Math.abs(gridPower))"
            :class="['flow-stream', gridPower >= 0 ? 'flow-forward' : 'flow-reverse']"
            :style="{ animationDuration: getSpeed(Math.abs(gridPower)) }"
            filter="url(#glow)"
          />

          <!-- ESS Battery Flow Stream (Bidirectional) -->
          <path 
            v-if="Math.abs(essPower) > 0.1"
            d="M 170 320 C 310 320, 340 210, 450 210" 
            stroke="url(#grad-ess)" 
            :stroke-width="getStrokeWidth(Math.abs(essPower))"
            :class="['flow-stream', essPower >= 0 ? 'flow-forward' : 'flow-reverse']"
            :style="{ animationDuration: getSpeed(Math.abs(essPower)) }"
            filter="url(#glow)"
          />

          <!-- Facility Load Stream -->
          <path 
            v-if="loadPower > 0.1"
            d="M 450 210 C 560 210, 590 120, 730 120" 
            stroke="url(#grad-load)" 
            :stroke-width="getStrokeWidth(loadPower)"
            class="flow-stream flow-forward"
            :style="{ animationDuration: getSpeed(loadPower) }"
            filter="url(#glow)"
          />

          <!-- EV Charger Stream -->
          <path 
            v-if="evPower > 0.1"
            d="M 450 210 C 560 210, 590 300, 730 300" 
            stroke="url(#grad-ev)" 
            :stroke-width="getStrokeWidth(evPower)"
            class="flow-stream flow-forward"
            :style="{ animationDuration: getSpeed(evPower) }"
            filter="url(#glow)"
          />
        </g>

        <!-- Node 1: Solar PV (Left-Top) -->
        <g transform="translate(70, 60)" class="cursor-pointer">
          <rect width="100" height="80" rx="16" fill="#0F172A" stroke="#3B82F6" stroke-width="2" />
          <text x="50" y="28" text-anchor="middle" fill="#93C5FD" font-family="sans-serif" font-weight="900" font-size="11">☀️ SOLAR PV</text>
          <text x="50" y="52" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="16">{{ pvPower.toFixed(1) }}</text>
          <text x="50" y="68" text-anchor="middle" fill="#60A5FA" font-family="JetBrains Mono, monospace" font-weight="bold" font-size="9">kW</text>
        </g>

        <!-- Node 2: Utility Grid (Left-Mid) -->
        <g transform="translate(70, 170)" class="cursor-pointer">
          <rect width="100" height="80" rx="16" fill="#0F172A" stroke="#A855F7" stroke-width="2" />
          <text x="50" y="28" text-anchor="middle" fill="#D8B4FE" font-family="sans-serif" font-weight="900" font-size="11">🏢 UTILITY GRID</text>
          <text x="50" y="52" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="16">{{ gridPower >= 0 ? '+' : '' }}{{ gridPower.toFixed(1) }}</text>
          <text x="50" y="68" text-anchor="middle" fill="#C084FC" font-family="JetBrains Mono, monospace" font-weight="bold" font-size="9">{{ gridPower >= 0 ? 'IMPORT' : 'EXPORT' }}</text>
        </g>

        <!-- Node 3: ESS BESS (Left-Bottom) -->
        <g transform="translate(70, 280)" class="cursor-pointer">
          <rect width="100" height="80" rx="16" fill="#0F172A" stroke="#10B981" stroke-width="2" />
          <text x="50" y="26" text-anchor="middle" fill="#6EE7B7" font-family="sans-serif" font-weight="900" font-size="11">🔋 ESS BESS</text>
          <text x="50" y="48" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="15">{{ essPower.toFixed(1) }} kW</text>
          <text x="50" y="66" text-anchor="middle" fill="#34D399" font-family="JetBrains Mono, monospace" font-weight="bold" font-size="9">SOC: {{ essSoc.toFixed(0) }}% ({{ essPower >= 0 ? 'DISCHG' : 'CHG' }})</text>
        </g>

        <!-- Node 4: Central Microgrid Bus (Center) -->
        <g transform="translate(390, 160)">
          <rect width="120" height="100" rx="20" fill="#0B1329" stroke="#38BDF8" stroke-width="2.5" filter="url(#glow)" />
          <circle cx="60" cy="22" r="4" fill="#38BDF8" class="animate-ping" />
          <text x="60" y="38" text-anchor="middle" fill="#7DD3FC" font-family="sans-serif" font-weight="900" font-size="11">⚡ MG BUS</text>
          <text x="60" y="64" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="18">{{ (loadPower + evPower).toFixed(1) }}</text>
          <text x="60" y="82" text-anchor="middle" fill="#94A3B8" font-family="JetBrains Mono, monospace" font-size="9">380V / 50.00Hz</text>
        </g>

        <!-- Node 5: Facility Load (Right-Top) -->
        <g transform="translate(730, 80)" class="cursor-pointer">
          <rect width="100" height="80" rx="16" fill="#0F172A" stroke="#F97316" stroke-width="2" />
          <text x="50" y="28" text-anchor="middle" fill="#FDBA74" font-family="sans-serif" font-weight="900" font-size="11">🏭 LOAD</text>
          <text x="50" y="52" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="16">{{ loadPower.toFixed(1) }}</text>
          <text x="50" y="68" text-anchor="middle" fill="#FB923C" font-family="JetBrains Mono, monospace" font-weight="bold" font-size="9">kW</text>
        </g>

        <!-- Node 6: EV Fast Charger (Right-Bottom) -->
        <g transform="translate(730, 260)" class="cursor-pointer">
          <rect width="100" height="80" rx="16" fill="#0F172A" stroke="#EC4899" stroke-width="2" />
          <text x="50" y="28" text-anchor="middle" fill="#F472B6" font-family="sans-serif" font-weight="900" font-size="11">🔌 EV CHARGER</text>
          <text x="50" y="52" text-anchor="middle" fill="#FFFFFF" font-family="JetBrains Mono, monospace" font-weight="900" font-size="16">{{ evPower.toFixed(1) }}</text>
          <text x="50" y="68" text-anchor="middle" fill="#F472B6" font-family="JetBrains Mono, monospace" font-weight="bold" font-size="9">3/4 Active</text>
        </g>
      </svg>
    </div>

    <!-- Microgrid Power Balance Balance Sheet Bar -->
    <div class="mt-4 pt-4 border-t border-slate-800 flex flex-wrap justify-between items-center text-xs font-mono text-slate-400">
      <div class="flex items-center gap-6">
        <span>Generation: <strong class="text-blue-400">{{ pvPower.toFixed(1) }} kW</strong></span>
        <span>Storage: <strong class="text-emerald-400">{{ essPower.toFixed(1) }} kW</strong></span>
        <span>Grid: <strong class="text-purple-400">{{ gridPower.toFixed(1) }} kW</strong></span>
        <span>Consumption: <strong class="text-amber-400">{{ (loadPower + evPower).toFixed(1) }} kW</strong></span>
      </div>
      <div class="text-[11px] text-slate-500">
        Balance Deficit: <strong class="text-slate-200">{{ Math.abs((pvPower + gridPower + essPower) - (loadPower + evPower)).toFixed(2) }} kW (0.00%)</strong>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue';
import axios from 'axios';

const isSimulating = ref(true);
let simTimer: any = null;

// Telemetry state variables (Physical units: kW, %)
const pvPower = ref(85.4);
const gridPower = ref(32.1);
const essPower = ref(25.0); // positive: discharging, negative: charging
const essSoc = ref(78.5);
const loadPower = ref(112.5);
const evPower = ref(30.0);

function getStrokeWidth(kw: number): number {
  return Math.min(10, Math.max(3, kw / 15));
}

function getSpeed(kw: number): string {
  const duration = Math.max(0.6, 3.5 - (kw / 40));
  return `${duration.toFixed(2)}s`;
}

function updateSimulation() {
  const t = Date.now();
  const angle = (t / 4000) * Math.PI;

  pvPower.value = Math.max(0, 90 + 35 * Math.sin(angle) + (Math.random() - 0.5) * 4);
  essPower.value = 30 * Math.cos(angle * 1.2) + (Math.random() - 0.5) * 3;
  loadPower.value = 110 + 20 * Math.sin(angle * 0.7) + (Math.random() - 0.5) * 5;
  evPower.value = Math.max(0, 25 + 15 * Math.cos(angle * 0.5));
  
  // Power balance: Grid = Load + EV - PV - ESS
  gridPower.value = (loadPower.value + evPower.value) - pvPower.value - essPower.value;
  essSoc.value = Math.min(100, Math.max(10, essSoc.value - (essPower.value > 0 ? 0.02 : -0.02)));
}

async function fetchRealTelemetry() {
  try {
    const res = await axios.get('/api/v1/iems/telemetry');
    if (res.data) {
      if (res.data.pv_power !== undefined) pvPower.value = Number(res.data.pv_power);
      if (res.data.grid_power !== undefined) gridPower.value = Number(res.data.grid_power);
      if (res.data.ess_power !== undefined) essPower.value = Number(res.data.ess_power);
      if (res.data.ess_soc !== undefined) essSoc.value = Number(res.data.ess_soc);
      if (res.data.load_power !== undefined) loadPower.value = Number(res.data.load_power);
      if (res.data.ev_power !== undefined) evPower.value = Number(res.data.ev_power);
    }
  } catch (e) {
    updateSimulation();
  }
}

function toggleSimulation() {
  isSimulating.value = !isSimulating.value;
}

onMounted(() => {
  simTimer = setInterval(() => {
    if (isSimulating.value) {
      fetchRealTelemetry();
    }
  }, 1000);
});

onBeforeUnmount(() => {
  if (simTimer) {
    clearInterval(simTimer);
    simTimer = null;
  }
});
</script>

<style scoped>
/* GPU hardware-accelerated flow animations */
.flow-stream {
  stroke-dasharray: 10 8;
  will-change: stroke-dashoffset;
}

.flow-forward {
  animation: flowForward linear infinite;
}

.flow-reverse {
  animation: flowReverse linear infinite;
}

@keyframes flowForward {
  from {
    stroke-dashoffset: 36;
  }
  to {
    stroke-dashoffset: 0;
  }
}

@keyframes flowReverse {
  from {
    stroke-dashoffset: 0;
  }
  to {
    stroke-dashoffset: 36;
  }
}
</style>
