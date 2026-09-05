<template>
  <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative overflow-hidden backdrop-blur-md flex flex-col">
    <!-- Header -->
    <div class="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 mb-6 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2.5 rounded-2xl bg-amber-500/10 border border-amber-500/20 text-amber-400">
          <span class="text-xl">💰</span>
        </div>
        <div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono flex items-center gap-2">
            TOU ARBITRAGE <span class="text-amber-500 font-sans">ECONOMIC ENGINE</span>
          </h2>
          <p class="text-xs font-mono text-slate-400">
            Time-of-Use 4-Stage Tariff Schedule & Dynamic Profit Tracker
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <span class="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-purple-500/10 border border-purple-500/20 text-purple-400 text-xs font-mono font-bold">
          <span class="w-2 h-2 rounded-full bg-purple-400 animate-ping"></span>
          <span>STAGE: {{ currentStage.name }}</span>
        </span>
        <span class="px-3 py-1 bg-slate-800 text-slate-300 border border-slate-700 rounded-xl text-xs font-mono font-bold">
          {{ currentStage.price.toFixed(2) }} ¥/kWh
        </span>
      </div>
    </div>

    <!-- 24-Hour TOU Gantt Timeline Bar -->
    <div class="space-y-2 mb-6">
      <div class="flex justify-between items-center text-xs font-mono text-slate-400">
        <span class="flex items-center gap-1.5 font-bold text-slate-300">
          <span class="w-2 h-2 rounded-full bg-sky-400"></span>
          24H TOU Tariff Distribution
        </span>
        <span class="text-[11px] text-slate-500">Current Time: {{ currentHourStr }}</span>
      </div>

      <!-- Gantt Bar -->
      <div class="relative w-full h-8 bg-slate-950 rounded-xl overflow-hidden flex border border-slate-800">
        <div 
          v-for="slot in hourlySchedule" 
          :key="slot.hour"
          class="flex-1 h-full relative group transition-all"
          :style="{ backgroundColor: getTariffColor(slot.tier) }"
          :title="`${slot.hour}:00 - ${slot.hour + 1}:00 | ${slot.tier.toUpperCase()}: ${slot.price} ¥/kWh`"
        >
          <!-- Hover tooltip preview -->
          <div class="opacity-0 group-hover:opacity-100 transition-opacity absolute -top-8 left-1/2 -translate-x-1/2 bg-slate-900 border border-slate-700 px-2 py-0.5 rounded text-[10px] font-mono text-white whitespace-nowrap z-20 pointer-events-none shadow-lg">
            {{ slot.hour }}:00 {{ slot.tier.toUpperCase() }} {{ slot.price }}¥
          </div>
        </div>

        <!-- Current Time Indicator Needle -->
        <div 
          class="absolute top-0 bottom-0 w-1 bg-white shadow-[0_0_8px_#ffffff] z-10 transition-all pointer-events-none"
          :style="{ left: `${(currentMinutes / 1440) * 100}%` }"
        ></div>
      </div>

      <!-- Hour markers -->
      <div class="flex justify-between text-[10px] font-mono text-slate-500 px-1">
        <span>00:00</span>
        <span>04:00</span>
        <span>08:00</span>
        <span>12:00</span>
        <span>16:00</span>
        <span>20:00</span>
        <span>24:00</span>
      </div>

      <!-- Legend -->
      <div class="flex flex-wrap items-center gap-4 pt-2 text-xs font-mono">
        <div class="flex items-center gap-1.5">
          <span class="w-3 h-3 rounded-full bg-emerald-500"></span>
          <span class="text-slate-300">Valley (0.32 ¥)</span>
        </div>
        <div class="flex items-center gap-1.5">
          <span class="w-3 h-3 rounded-full bg-amber-500"></span>
          <span class="text-slate-300">Flat (0.68 ¥)</span>
        </div>
        <div class="flex items-center gap-1.5">
          <span class="w-3 h-3 rounded-full bg-rose-500"></span>
          <span class="text-slate-300">Peak (1.15 ¥)</span>
        </div>
        <div class="flex items-center gap-1.5">
          <span class="w-3 h-3 rounded-full bg-purple-500"></span>
          <span class="text-slate-300">Critical Peak (1.48 ¥)</span>
        </div>
      </div>
    </div>

    <!-- Arbitrage Metrics KPI Grid -->
    <div class="grid grid-cols-2 lg:grid-cols-4 gap-4">
      <div class="bg-slate-950/60 border border-slate-800 p-4 rounded-2xl">
        <div class="text-[10px] font-mono text-amber-400 uppercase tracking-wider font-bold mb-1">Daily Revenue</div>
        <div class="text-2xl font-black font-mono text-white">
          +¥{{ dailyRevenue.toFixed(2) }}
        </div>
        <div class="text-[11px] font-mono text-emerald-400 mt-1">
          ▲ +18.4% vs Baseline
        </div>
      </div>

      <div class="bg-slate-950/60 border border-slate-800 p-4 rounded-2xl">
        <div class="text-[10px] font-mono text-emerald-400 uppercase tracking-wider font-bold mb-1">Charged (Valley)</div>
        <div class="text-2xl font-black font-mono text-white">
          {{ chargedKwh.toFixed(1) }} <span class="text-xs text-slate-400">kWh</span>
        </div>
        <div class="text-[11px] font-mono text-slate-400 mt-1">
          Cost: ¥{{ (chargedKwh * 0.32).toFixed(2) }}
        </div>
      </div>

      <div class="bg-slate-950/60 border border-slate-800 p-4 rounded-2xl">
        <div class="text-[10px] font-mono text-purple-400 uppercase tracking-wider font-bold mb-1">Discharged (Peak)</div>
        <div class="text-2xl font-black font-mono text-white">
          {{ dischargedKwh.toFixed(1) }} <span class="text-xs text-slate-400">kWh</span>
        </div>
        <div class="text-[11px] font-mono text-slate-400 mt-1">
          Gain: ¥{{ (dischargedKwh * 1.42).toFixed(2) }}
        </div>
      </div>

      <div class="bg-slate-950/60 border border-slate-800 p-4 rounded-2xl">
        <div class="text-[10px] font-mono text-sky-400 uppercase tracking-wider font-bold mb-1">Round-Trip RTE</div>
        <div class="text-2xl font-black font-mono text-white">
          {{ ((dischargedKwh / (chargedKwh || 1)) * 100).toFixed(1) }}<span class="text-xs text-slate-400">%</span>
        </div>
        <div class="text-[11px] font-mono text-sky-400 mt-1">
          Optimal: > 92.0%
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

interface TOUSlot {
  hour: number;
  tier: 'valley' | 'flat' | 'peak' | 'critical';
  price: number;
}

const currentMinutes = ref(0);
const dailyRevenue = ref(842.50);
const chargedKwh = ref(420.0);
const dischargedKwh = ref(395.0);
let timer: any = null;

// 24-hour tariff profile definition
const hourlySchedule = ref<TOUSlot[]>([
  { hour: 0, tier: 'valley', price: 0.32 },
  { hour: 1, tier: 'valley', price: 0.32 },
  { hour: 2, tier: 'valley', price: 0.32 },
  { hour: 3, tier: 'valley', price: 0.32 },
  { hour: 4, tier: 'valley', price: 0.32 },
  { hour: 5, tier: 'valley', price: 0.32 },
  { hour: 6, tier: 'valley', price: 0.32 },
  { hour: 7, tier: 'valley', price: 0.32 },
  { hour: 8, tier: 'flat', price: 0.68 },
  { hour: 9, tier: 'peak', price: 1.15 },
  { hour: 10, tier: 'peak', price: 1.15 },
  { hour: 11, tier: 'valley', price: 0.32 },
  { hour: 12, tier: 'valley', price: 0.32 },
  { hour: 13, tier: 'flat', price: 0.68 },
  { hour: 14, tier: 'flat', price: 0.68 },
  { hour: 15, tier: 'flat', price: 0.68 },
  { hour: 16, tier: 'flat', price: 0.68 },
  { hour: 17, tier: 'peak', price: 1.15 },
  { hour: 18, tier: 'peak', price: 1.15 },
  { hour: 19, tier: 'critical', price: 1.48 },
  { hour: 20, tier: 'critical', price: 1.48 },
  { hour: 21, tier: 'peak', price: 1.15 },
  { hour: 22, tier: 'flat', price: 0.68 },
  { hour: 23, tier: 'flat', price: 0.68 },
]);

function getTariffColor(tier: string): string {
  switch (tier) {
    case 'valley': return '#10B981';
    case 'flat': return '#F59E0B';
    case 'peak': return '#EF4444';
    case 'critical': return '#8B5CF6';
    default: return '#64748B';
  }
}

const currentHourStr = computed(() => {
  const d = new Date();
  return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}:${String(d.getSeconds()).padStart(2, '0')}`;
});

const currentStage = computed(() => {
  const h = new Date().getHours();
  const slot = hourlySchedule.value.find(s => s.hour === h) || hourlySchedule.value[0];
  const names: Record<string, string> = {
    valley: 'VALLEY (谷)',
    flat: 'FLAT (平)',
    peak: 'PEAK (峰)',
    critical: 'CRITICAL PEAK (尖)',
  };
  return {
    name: names[slot.tier] || slot.tier.toUpperCase(),
    price: slot.price,
    tier: slot.tier
  };
});

function updateTime() {
  const now = new Date();
  currentMinutes.value = now.getHours() * 60 + now.getMinutes() + (now.getSeconds() / 60);
}

onMounted(() => {
  updateTime();
  timer = setInterval(updateTime, 1000);
});

onBeforeUnmount(() => {
  if (timer) {
    clearInterval(timer);
    timer = null;
  }
});
</script>
