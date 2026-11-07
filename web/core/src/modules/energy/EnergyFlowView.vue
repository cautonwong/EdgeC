<template>
  <div class="min-h-full bg-[var(--bg-page)] text-[var(--text-primary)] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[var(--bg-surface)] rounded-xl overflow-hidden shadow-2xl border border-[var(--border-default)] p-8 lg:p-10">
      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-blue-500/10 border border-blue-500/20 text-blue-400 text-xs font-black">
            <span class="w-1.5 h-1.5 rounded-full bg-blue-400 animate-pulse motion-reduce:animate-none"></span>
            微网调度运行中
          </div>
          <h1 class="text-3xl lg:text-4xl font-black tracking-tight leading-tight">能流图</h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            实时功率拓扑、分时四段套利与 100ms 防逆流 PID 控制
          </p>
        </div>

        <div class="flex flex-wrap items-center gap-4">
          <div class="bg-[var(--bg-elevated)] backdrop-blur-md border border-[var(--border-default)] px-5 py-3 rounded-2xl">
            <div class="text-xs font-black text-slate-400">PCS 状态</div>
            <div class="text-lg font-black text-emerald-400 font-mono mt-0.5">RUNNING / PQ</div>
          </div>
          <div class="bg-[var(--bg-elevated)] backdrop-blur-md border border-[var(--border-default)] px-5 py-3 rounded-2xl">
            <div class="text-xs font-black text-slate-400">协调器</div>
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
