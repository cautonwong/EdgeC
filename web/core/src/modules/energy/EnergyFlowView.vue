<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <!-- Ambient light effect -->
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-blue-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-emerald-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-blue-500/10 border border-blue-500/20 text-blue-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-blue-400 animate-pulse"></span>
            IEMS Microgrid Dispatch Active
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight">
            Microgrid <span class="text-sky-400">Energy Flow & Arbitrage Hub</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            Real-time physical power topology, TOU 4-stage economic arbitrage, and 100ms zero-export PID control.
          </p>
        </div>

        <div class="flex flex-wrap items-center gap-4">
          <div class="bg-[#1e293b]/70 backdrop-blur-md border border-[#334155] px-5 py-3 rounded-2xl">
            <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">PCS Status</div>
            <div class="text-lg font-black text-emerald-400 font-mono mt-0.5">RUNNING / PQ</div>
          </div>
          <div class="bg-[#1e293b]/70 backdrop-blur-md border border-[#334155] px-5 py-3 rounded-2xl">
            <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">Coordinator</div>
            <div class="text-lg font-black text-sky-400 font-mono mt-0.5">AUTO / TOU+SHAVING</div>
          </div>
        </div>
      </div>
    </section>

    <!-- Main Sankey Power Flow Section (Top Hero) -->
    <SankeyEnergyFlow />

    <!-- Secondary Grids: TOU Arbitrage, Anti-Reverse PID Gauge, Demand Peak Shaving -->
    <div class="grid grid-cols-1 lg:grid-cols-3 gap-8 items-start">
      <!-- TOU Arbitrage Economic Engine -->
      <div class="lg:col-span-2">
        <TOUArbitrageWidget />
      </div>

      <!-- Anti-Reverse 100ms PID Gauge -->
      <div class="lg:col-span-1">
        <AntiReversePIDGauge />
      </div>

      <!-- 15-min Demand Peak Shaving Monitor (Full Width across bottom or 3-col span) -->
      <div class="lg:col-span-3">
        <DemandPeakShavingWidget />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import SankeyEnergyFlow from '../../components/SankeyEnergyFlow.vue';
import TOUArbitrageWidget from '../../components/TOUArbitrageWidget.vue';
import AntiReversePIDGauge from '../../components/AntiReversePIDGauge.vue';
import DemandPeakShavingWidget from '../../components/DemandPeakShavingWidget.vue';
</script>
