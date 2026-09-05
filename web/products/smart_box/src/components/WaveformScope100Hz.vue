<template>
  <div 
    ref="scopeContainer"
    class="bg-slate-900 border-2 border-slate-800 rounded-3xl p-6 lg:p-8 shadow-2xl flex flex-col relative overflow-hidden backdrop-blur-md"
  >
    <!-- Header Controls -->
    <div class="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 mb-6 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2.5 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 text-emerald-400">
          <span class="text-xl">📈</span>
        </div>
        <div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono flex items-center gap-2">
            100Hz HARDWARE <span class="text-emerald-400 font-sans">OSCILLOSCOPE</span>
          </h2>
          <p class="text-xs font-mono text-slate-400">
            High-Precision Offscreen Canvas // 4K HDMI Output Ready
          </p>
        </div>
      </div>

      <!-- Scope Action Buttons -->
      <div class="flex flex-wrap items-center gap-2 font-mono text-xs">
        <span class="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 font-bold">
          <span class="w-2 h-2 rounded-full" :class="isRunning ? 'bg-emerald-400 animate-ping' : 'bg-amber-400'"></span>
          <span>{{ isRunning ? 'SAMPLING: 100Hz (10ms)' : 'FROZEN' }}</span>
        </span>

        <button 
          @click="toggleRun" 
          class="px-3.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded-xl font-bold transition-all"
        >
          {{ isRunning ? '⏸ FREEZE' : '▶ RUN' }}
        </button>

        <button 
          @click="toggleFullscreen" 
          class="px-3.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded-xl font-bold transition-all"
        >
          ⛶ 全屏投屏
        </button>
      </div>
    </div>

    <!-- Canvas Scope Screen Container -->
    <div class="relative w-full h-80 bg-slate-950 rounded-2xl overflow-hidden border border-slate-800 shadow-inner flex items-center justify-center">
      <!-- Grid Lines Background Overlay -->
      <div class="absolute inset-0 grid-scope-pattern pointer-events-none opacity-40"></div>

      <!-- Live Canvas Element -->
      <canvas 
        ref="canvasRef" 
        class="w-full h-full block relative z-10"
      ></canvas>

      <!-- Channel Overlay Legend on Canvas -->
      <div class="absolute top-3 left-4 z-20 flex gap-4 text-xs font-mono bg-slate-900/80 backdrop-blur px-3 py-1.5 rounded-xl border border-slate-800">
        <label class="flex items-center gap-1.5 cursor-pointer text-emerald-400">
          <input type="checkbox" v-model="showCH1" class="rounded border-slate-700 text-emerald-500 focus:ring-0" />
          <span>CH1: Gait (100Hz)</span>
        </label>
        <label class="flex items-center gap-1.5 cursor-pointer text-sky-400">
          <input type="checkbox" v-model="showCH2" class="rounded border-slate-700 text-sky-500 focus:ring-0" />
          <span>CH2: ECG (100Hz)</span>
        </label>
        <label class="flex items-center gap-1.5 cursor-pointer text-amber-400">
          <input type="checkbox" v-model="showCH3" class="rounded border-slate-700 text-amber-500 focus:ring-0" />
          <span>CH3: EMG (100Hz)</span>
        </label>
      </div>

      <!-- Scope Live Crosshair Measurements -->
      <div class="absolute bottom-3 right-4 z-20 text-[11px] font-mono text-slate-400 bg-slate-900/80 backdrop-blur px-3 py-1.5 rounded-xl border border-slate-800 space-x-3">
        <span>Vp-p: <strong class="text-slate-100">{{ vpp.toFixed(2) }}V</strong></span>
        <span>Freq: <strong class="text-emerald-400">100.00 Hz</strong></span>
        <span>Period: <strong class="text-sky-400">10.0 ms</strong></span>
      </div>
    </div>

    <!-- Scope Control Panel Bar -->
    <div class="mt-4 pt-4 border-t border-slate-800 grid grid-cols-2 sm:grid-cols-4 gap-4 text-xs font-mono">
      <div class="bg-slate-950/60 p-3 rounded-2xl border border-slate-800">
        <span class="text-[10px] text-slate-500 block mb-1 uppercase">Timebase</span>
        <select v-model="timebase" class="w-full bg-slate-900 border border-slate-700 rounded-lg px-2 py-1 text-slate-200 focus:outline-none focus:border-emerald-500">
          <option :value="10">10 ms/div</option>
          <option :value="20">20 ms/div</option>
          <option :value="50">50 ms/div</option>
          <option :value="100">100 ms/div</option>
        </select>
      </div>

      <div class="bg-slate-950/60 p-3 rounded-2xl border border-slate-800">
        <span class="text-[10px] text-slate-500 block mb-1 uppercase">Voltage Scale</span>
        <select v-model="gain" class="w-full bg-slate-900 border border-slate-700 rounded-lg px-2 py-1 text-slate-200 focus:outline-none focus:border-emerald-500">
          <option :value="1">1.0x (Standard)</option>
          <option :value="2">2.0x (Zoomed)</option>
          <option :value="5">5.0x (High-Gain)</option>
        </select>
      </div>

      <div class="bg-slate-950/60 p-3 rounded-2xl border border-slate-800">
        <span class="text-[10px] text-slate-500 block mb-1 uppercase">Trigger Mode</span>
        <select v-model="triggerMode" class="w-full bg-slate-900 border border-slate-700 rounded-lg px-2 py-1 text-slate-200 focus:outline-none focus:border-emerald-500">
          <option value="auto">AUTO (Edge)</option>
          <option value="normal">NORMAL</option>
          <option value="single">SINGLE</option>
        </select>
      </div>

      <div class="bg-slate-950/60 p-3 rounded-2xl border border-slate-800">
        <span class="text-[10px] text-slate-500 block mb-1 uppercase">Offscreen Buffer</span>
        <div class="text-sm font-black text-emerald-400 mt-1">
          DPI 2.0x (4K Synced)
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue';

const scopeContainer = ref<HTMLDivElement | null>(null);
const canvasRef = ref<HTMLCanvasElement | null>(null);

const isRunning = ref(true);
const showCH1 = ref(true);
const showCH2 = ref(true);
const showCH3 = ref(true);

const timebase = ref(20);
const gain = ref(1);
const triggerMode = ref('auto');
const vpp = ref(3.28);

let animationFrameId: number | null = null;
let sampleIntervalId: any = null;

// Ring buffer for 100Hz samples
const BUFFER_SIZE = 600;
const ch1Data: number[] = new Array(BUFFER_SIZE).fill(0);
const ch2Data: number[] = new Array(BUFFER_SIZE).fill(0);
const ch3Data: number[] = new Array(BUFFER_SIZE).fill(0);
let sampleIndex = 0;

function toggleRun() {
  isRunning.value = !isRunning.value;
}

function toggleFullscreen() {
  if (!scopeContainer.value) return;
  if (!document.fullscreenElement) {
    scopeContainer.value.requestFullscreen().catch(err => {
      console.warn('Fullscreen error:', err);
    });
  } else {
    document.exitFullscreen();
  }
}

// 100Hz high-frequency physical sampling loop (every 10ms)
function tickSampling() {
  if (!isRunning.value) return;
  const t = Date.now() / 1000;

  // CH1: Gait cadence force (approx 1.87Hz gait cycle + foot-strike impact)
  const gaitCycle = (t * 1.87 * 2 * Math.PI) % (2 * Math.PI);
  let gaitVal = Math.max(0, Math.sin(gaitCycle)) * 40;
  if (gaitCycle < 0.4) gaitVal += 20 * Math.sin(gaitCycle * 15); // foot strike peak
  ch1Data[sampleIndex] = gaitVal + (Math.random() - 0.5) * 2;

  // CH2: ECG QRS complex simulated waveform
  const ecgPhase = (t * 1.25 * 2 * Math.PI) % (2 * Math.PI);
  let ecgVal = 0;
  if (ecgPhase > 1.0 && ecgPhase < 1.3) {
    ecgVal = Math.sin((ecgPhase - 1.0) / 0.3 * Math.PI) * 45; // R peak
  } else if (ecgPhase > 1.4 && ecgPhase < 1.7) {
    ecgVal = -Math.sin((ecgPhase - 1.4) / 0.3 * Math.PI) * 12; // S peak
  }
  ch2Data[sampleIndex] = ecgVal + (Math.random() - 0.5) * 3;

  // CH3: sEMG muscle activity burst
  const emgBurst = Math.sin(t * 0.8) > 0 ? (Math.random() - 0.5) * 35 : (Math.random() - 0.5) * 4;
  ch3Data[sampleIndex] = emgBurst;

  sampleIndex = (sampleIndex + 1) % BUFFER_SIZE;
}

function renderScope() {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;
  const centerY = height / 2;

  // Clear with dark phosphor persistence effect
  ctx.fillStyle = 'rgba(2, 6, 23, 0.25)';
  ctx.fillRect(0, 0, width, height);

  const stepX = width / BUFFER_SIZE;
  const currentGain = gain.value;

  // Draw CH1 (Gait - Emerald)
  if (showCH1.value) {
    ctx.strokeStyle = '#10B981';
    ctx.lineWidth = 2.5;
    ctx.shadowColor = '#10B981';
    ctx.shadowBlur = 6;
    ctx.beginPath();
    for (let i = 0; i < BUFFER_SIZE; i++) {
      const idx = (sampleIndex + i) % BUFFER_SIZE;
      const x = i * stepX;
      const y = centerY - (ch1Data[idx] * currentGain);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Draw CH2 (ECG - Sky Blue)
  if (showCH2.value) {
    ctx.strokeStyle = '#38BDF8';
    ctx.lineWidth = 2.0;
    ctx.shadowColor = '#38BDF8';
    ctx.shadowBlur = 4;
    ctx.beginPath();
    for (let i = 0; i < BUFFER_SIZE; i++) {
      const idx = (sampleIndex + i) % BUFFER_SIZE;
      const x = i * stepX;
      const y = centerY + 40 - (ch2Data[idx] * currentGain);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Draw CH3 (EMG - Amber)
  if (showCH3.value) {
    ctx.strokeStyle = '#F59E0B';
    ctx.lineWidth = 1.5;
    ctx.shadowColor = '#F59E0B';
    ctx.shadowBlur = 4;
    ctx.beginPath();
    for (let i = 0; i < BUFFER_SIZE; i++) {
      const idx = (sampleIndex + i) % BUFFER_SIZE;
      const x = i * stepX;
      const y = centerY - 50 - (ch3Data[idx] * currentGain);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Reset shadow for next frame
  ctx.shadowBlur = 0;

  if (isRunning.value) {
    animationFrameId = requestAnimationFrame(renderScope);
  }
}

function handleResize() {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
}

onMounted(() => {
  handleResize();
  window.addEventListener('resize', handleResize);

  // 100Hz = 10ms sampling interval
  sampleIntervalId = setInterval(tickSampling, 10);
  animationFrameId = requestAnimationFrame(renderScope);
});

onBeforeUnmount(() => {
  if (sampleIntervalId) clearInterval(sampleIntervalId);
  if (animationFrameId) cancelAnimationFrame(animationFrameId);
  window.removeEventListener('resize', handleResize);
});
</script>

<style scoped>
.grid-scope-pattern {
  background-image: 
    linear-gradient(to right, rgba(255, 255, 255, 0.05) 1px, transparent 1px),
    linear-gradient(to bottom, rgba(255, 255, 255, 0.05) 1px, transparent 1px);
  background-size: 40px 40px;
}
</style>
