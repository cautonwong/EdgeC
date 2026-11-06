<template>
  <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative overflow-hidden backdrop-blur-md flex flex-col">
    <!-- Header -->
    <div class="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 mb-6 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2.5 rounded-2xl bg-rose-500/10 border border-rose-500/20 text-rose-400">
          <span class="text-xl">🛡️</span>
        </div>
        <div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono flex items-center gap-2">
            防逆流 <span class="text-rose-500 font-sans">100ms PID 调节</span>
          </h2>
          <p class="text-xs font-mono text-slate-400">
            零反送保护与死区功率调节回路
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <span 
          :class="[
            'inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-mono font-bold border',
            isNormal ? 'bg-emerald-500/10 border-emerald-500/20 text-emerald-400' : 
            (isDeadband ? 'bg-sky-500/10 border-sky-500/20 text-sky-400' : 'bg-rose-500/10 border-rose-500/20 text-rose-400 animate-pulse')
          ]"
        >
          <span class="w-2 h-2 rounded-full" :class="isNormal ? 'bg-emerald-400' : (isDeadband ? 'bg-sky-400' : 'bg-rose-400')"></span>
          <span>{{ statusText }}</span>
        </span>
        <span class="px-3 py-1 bg-slate-800 text-slate-300 border border-slate-700 rounded-xl text-xs font-mono font-bold">
          周期：100ms
        </span>
      </div>
    </div>

    <!-- Gauge Visualization Canvas (SVG Arc Meter) -->
    <div class="relative w-full h-[220px] flex items-center justify-center">
      <svg role="img" aria-label="数据图表" viewBox="0 0 300 180" class="w-full max-w-[340px] h-full">
        <!-- Background Track Arc -->
        <path
          d="M 40 150 A 110 110 0 0 1 260 150"
          fill="none"
          stroke="var(--bg-elevated)"
          stroke-width="16"
          stroke-linecap="round"
        />

        <!-- Reverse Danger Zone (Left Arc: -50kW to 0kW) -->
        <path
          d="M 40 150 A 110 110 0 0 1 120 49"
          fill="none"
          stroke="var(--color-danger)"
          stroke-width="16"
          stroke-dasharray="2 4"
          opacity="0.4"
        />

        <!-- Deadband Zone (Around 0kW: ±5kW) -->
        <path
          d="M 120 49 A 110 110 0 0 1 170 43"
          fill="none"
          stroke="var(--chart-1)"
          stroke-width="18"
          opacity="0.6"
        />

        <!-- Forward Safe Import Zone (Right Arc: 0kW to +100kW) -->
        <path
          d="M 170 43 A 110 110 0 0 1 260 150"
          fill="none"
          stroke="var(--chart-3)"
          stroke-width="16"
          opacity="0.3"
        />

        <!-- Active Measurement Arc -->
        <path
          :d="describeArc(150, 150, 110, -180, getAngle(gridPower))"
          fill="none"
          :stroke="gridPower < 0 ? 'var(--color-danger)' : (Math.abs(gridPower) < deadband ? 'var(--chart-1)' : 'var(--chart-3)')"
          stroke-width="16"
          stroke-linecap="round"
          class="transition duration-300"
        />

        <!-- Center Needle / Hub -->
        <circle cx="150" cy="150" r="12" fill="var(--bg-surface)" stroke="var(--text-disabled)" stroke-width="3" />
        <line
          x1="150"
          y1="150"
          :x2="150 + 90 * Math.cos(getAngleRad(gridPower))"
          :y2="150 + 90 * Math.sin(getAngleRad(gridPower))"
          :stroke="gridPower < 0 ? 'var(--color-danger)' : 'var(--text-primary)'"
          stroke-width="3.5"
          stroke-linecap="round"
          class="transition duration-300"
        />
        <circle cx="150" cy="150" r="5" fill="var(--chart-1)" />

        <!-- Scale Labels -->
        <text x="35" y="172" fill="var(--text-secondary)" font-family="Edge Mono, Edge Sans, monospace" font-size="12" text-anchor="middle">-50kW</text>
        <text x="150" y="32" fill="var(--chart-1)" font-family="Edge Mono, Edge Sans, monospace" font-size="12" font-weight="bold" text-anchor="middle">0（限值）</text>
        <text x="265" y="172" fill="var(--text-secondary)" font-family="Edge Mono, Edge Sans, monospace" font-size="12" text-anchor="middle">+100kW</text>

        <!-- Live Power Big Value Display -->
        <text x="150" y="115" fill="var(--text-primary)" font-family="Edge Mono, Edge Sans, monospace" font-size="22" font-weight="600" text-anchor="middle">
          {{ gridPower >= 0 ? '+' : '' }}{{ gridPower.toFixed(1) }} <tspan font-size="12" fill="var(--text-secondary)">kW</tspan>
        </text>
        <text x="150" y="132" fill="var(--text-disabled)" font-family="Edge Sans, sans-serif" font-size="12" font-weight="bold" text-anchor="middle">
          并网点功率
        </text>
      </svg>
    </div>

    <!-- PID Loop Control Telemetry Footer -->
    <div class="mt-2 pt-4 border-t border-slate-800 grid grid-cols-3 gap-3 text-xs font-mono">
      <div class="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
        <div class="text-xs text-slate-400">PID 输出 ΔP</div>
        <div class="text-base font-black text-white mt-0.5">
          {{ pidDeltaP.toFixed(1) }} <span class="text-xs text-slate-400">kW</span>
        </div>
      </div>
      <div class="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
        <div class="text-xs text-slate-400">死区</div>
        <div class="text-base font-black text-sky-400 mt-0.5">
          ±{{ deadband.toFixed(1) }} <span class="text-xs text-slate-400">kW</span>
        </div>
      </div>
      <div class="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
        <div class="text-xs text-slate-400">Kp / Ki / Kd</div>
        <div class="text-sm font-black text-amber-400 mt-1">
          0.85 / 0.12 / 0.04
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

const gridPower = ref(12.4); // Current active power at PCC point (kW)
const deadband = ref(2.5); // Anti-reverse deadband threshold (kW)
const pidDeltaP = ref(-0.0);
let pidTimer: any = null;

const isDeadband = computed(() => Math.abs(gridPower.value) <= deadband.value);
const isNormal = computed(() => gridPower.value > deadband.value);
const statusText = computed(() => {
  if (gridPower.value < -0.5) return 'CURTAILMENT ACTIVE';
  if (isDeadband.value) return '死区 LOCKED';
  return 'IMPORT NORMAL';
});

// Helper for SVG Arc calculation
function polarToCartesian(centerX: number, centerY: number, radius: number, angleInDegrees: number) {
  const angleInRadians = (angleInDegrees * Math.PI) / 180.0;
  return {
    x: centerX + (radius * Math.cos(angleInRadians)),
    y: centerY + (radius * Math.sin(angleInRadians))
  };
}

function describeArc(x: number, y: number, radius: number, startAngle: number, endAngle: number) {
  const start = polarToCartesian(x, y, radius, endAngle);
  const end = polarToCartesian(x, y, radius, startAngle);
  const largeArcFlag = endAngle - startAngle <= 180 ? '0' : '1';
  return [
    'M', start.x, start.y,
    'A', radius, radius, 0, largeArcFlag, 0, end.x, end.y
  ].join(' ');
}

function getAngle(kw: number): number {
  // Map [-50 kW, +100 kW] to [-180 deg, 0 deg]
  const clamped = Math.max(-50, Math.min(100, kw));
  const ratio = (clamped + 50) / 150;
  return -180 + ratio * 180;
}

function getAngleRad(kw: number): number {
  return (getAngle(kw) * Math.PI) / 180;
}

function tickPIDLoop() {
  // Simulate 100ms PID fast response loop
  const err = 0.0 - gridPower.value; // Target is 0.0 kW limit (prevent reverse export)
  if (gridPower.value < 0) {
    pidDeltaP.value = Math.max(-100, err * 0.85);
    gridPower.value += 1.2; // Rapid correction back towards deadband
  } else {
    // Normal small fluctuations
    gridPower.value += (Math.random() - 0.48) * 1.5;
    pidDeltaP.value = 0;
  }
}

onMounted(() => {
  pidTimer = setInterval(tickPIDLoop, 100);
});

onBeforeUnmount(() => {
  if (pidTimer) {
    clearInterval(pidTimer);
    pidTimer = null;
  }
});
</script>
