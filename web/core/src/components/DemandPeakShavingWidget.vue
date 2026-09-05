<template>
  <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative overflow-hidden backdrop-blur-md flex flex-col">
    <!-- Header -->
    <div class="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 mb-6 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2.5 rounded-2xl bg-sky-500/10 border border-sky-500/20 text-sky-400">
          <span class="text-xl">📉</span>
        </div>
        <div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono flex items-center gap-2">
            DEMAND <span class="text-sky-500 font-sans">PEAK SHAVING</span>
          </h2>
          <p class="text-xs font-mono text-slate-400">
            15-Min Rolling Demand Protection & 500kVA Transformer Redline Watch
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <span 
          :class="[
            'inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-mono font-bold border',
            isPeakShavingActive ? 'bg-amber-500/10 border-amber-500/20 text-amber-400 animate-pulse' : 'bg-emerald-500/10 border-emerald-500/20 text-emerald-400'
          ]"
        >
          <span class="w-2 h-2 rounded-full" :class="isPeakShavingActive ? 'bg-amber-400' : 'bg-emerald-400'"></span>
          <span>{{ isPeakShavingActive ? 'SHAVING ACTIVE (+65kW ESS)' : 'DEMAND NORMAL' }}</span>
        </span>
        <span class="px-3 py-1 bg-slate-800 text-slate-300 border border-slate-700 rounded-xl text-xs font-mono font-bold">
          Window: 15min MTT
        </span>
      </div>
    </div>

    <!-- Transformer Load Progress Bar & Redline Visualizer -->
    <div class="space-y-4 mb-6">
      <div class="flex justify-between items-end text-xs font-mono">
        <div>
          <div class="text-slate-400 text-[10px] uppercase font-bold">15-Min Rolling Demand</div>
          <div class="text-2xl font-black text-white font-mono mt-0.5">
            {{ rollingDemand.toFixed(1) }} <span class="text-xs text-slate-400">kW</span>
          </div>
        </div>
        <div class="text-right">
          <div class="text-slate-400 text-[10px] uppercase font-bold">Transformer Load Rate</div>
          <div 
            :class="[
              'text-2xl font-black font-mono mt-0.5',
              loadRate > 85 ? 'text-rose-400' : (loadRate > 75 ? 'text-amber-400' : 'text-emerald-400')
            ]"
          >
            {{ loadRate.toFixed(1) }}%
          </div>
        </div>
      </div>

      <!-- Linear Multi-Zone Gauge Bar -->
      <div class="relative w-full h-7 bg-slate-950 rounded-xl overflow-hidden border border-slate-800 p-1 flex items-center">
        <!-- Safe Zone (0 - 350kW: 70%) -->
        <div class="absolute left-0 top-0 bottom-0 w-[70%] bg-emerald-500/10 border-r border-emerald-500/20"></div>
        <!-- Warning Zone (350 - 400kW: 70% - 80%) -->
        <div class="absolute left-[70%] top-0 bottom-0 w-[10%] bg-amber-500/10 border-r border-amber-500/20"></div>
        <!-- Critical Zone (400 - 500kW: 80% - 100%) -->
        <div class="absolute left-[80%] top-0 bottom-0 w-[20%] bg-rose-500/10"></div>

        <!-- 400kW Redline Target Marker -->
        <div class="absolute left-[80%] top-0 bottom-0 w-0.5 bg-rose-500 shadow-[0_0_6px_#ef4444] z-10">
          <span class="absolute -top-6 -translate-x-1/2 bg-rose-950/80 border border-rose-500/40 text-rose-300 text-[9px] font-mono px-1 rounded">
            LIMIT: 400kW
          </span>
        </div>

        <!-- Active Load Level Bar -->
        <div 
          class="h-full rounded-lg transition-all duration-500 relative z-0 flex items-center justify-end pr-2"
          :style="{
            width: `${Math.min(100, (rollingDemand / 500) * 100)}%`,
            backgroundColor: loadRate > 80 ? '#F59E0B' : '#10B981'
          }"
        >
          <span class="w-1.5 h-1.5 rounded-full bg-white animate-ping"></span>
        </div>
      </div>

      <!-- Scale Markers -->
      <div class="flex justify-between text-[10px] font-mono text-slate-500 px-1">
        <span>0 kW</span>
        <span>250 kW (50%)</span>
        <span class="text-rose-400 font-bold">400 kW (80% REDLINE)</span>
        <span>500 kVA (MAX)</span>
      </div>
    </div>

    <!-- Mini History 15-min Intervals Histogram -->
    <div class="bg-slate-950/60 border border-slate-800 p-4 rounded-2xl">
      <div class="text-[10px] font-mono text-slate-400 font-bold uppercase mb-3 flex justify-between items-center">
        <span>Today's 15-Min Demand History (Peak Intervals)</span>
        <span class="text-emerald-400">Peak Shaved: 12 Intervals</span>
      </div>

      <div class="h-16 flex items-end gap-1.5 w-full">
        <div 
          v-for="(val, idx) in historyDemand" 
          :key="idx" 
          class="flex-1 bg-slate-800 hover:bg-sky-500 rounded-t transition-all group relative cursor-pointer"
          :style="{ 
            height: `${(val / 500) * 100}%`,
            backgroundColor: val > 390 ? '#F59E0B' : (val > 350 ? '#38BDF8' : '#334155')
          }"
        >
          <!-- Tooltip -->
          <div class="opacity-0 group-hover:opacity-100 transition-opacity absolute -top-7 left-1/2 -translate-x-1/2 bg-slate-900 border border-slate-700 px-1.5 py-0.5 rounded text-[9px] font-mono text-white whitespace-nowrap z-20 pointer-events-none">
            {{ val }}kW
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

const transformerCapacity = ref(500.0); // kVA
const peakLimit = ref(400.0); // kW (80% redline)
const rollingDemand = ref(378.4); // 15-min rolling average kW
let timer: any = null;

const loadRate = computed(() => (rollingDemand.value / transformerCapacity.value) * 100);
const isPeakShavingActive = computed(() => rollingDemand.value >= (peakLimit.value - 25));

const historyDemand = ref<number[]>([
  280, 310, 340, 360, 385, 392, 375, 340, 320, 350, 380, 395, 378, 360, 340, 330
]);

function updateRollingDemand() {
  // Simulate 15-min rolling demand variations
  rollingDemand.value = 370 + 20 * Math.sin(Date.now() / 15000) + (Math.random() - 0.5) * 6;
}

onMounted(() => {
  timer = setInterval(updateRollingDemand, 2000);
});

onBeforeUnmount(() => {
  if (timer) {
    clearInterval(timer);
    timer = null;
  }
});
</script>
