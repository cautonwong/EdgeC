<template>
  <div v-if="isOpen" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/85 backdrop-blur-md animate-in fade-in duration-300">
    <!-- Modal Container with Alert Red Glow -->
    <div 
      class="bg-slate-900 border-2 border-rose-500/80 rounded-3xl p-6 lg:p-8 max-w-lg w-full shadow-[0_0_50px_rgba(239,68,68,0.35)] relative overflow-hidden"
    >
      <!-- Red Warning Top Accent Bar -->
      <div class="absolute top-0 left-0 right-0 h-1.5 bg-gradient-to-r from-rose-600 via-amber-500 to-rose-600 animate-pulse"></div>

      <!-- Header -->
      <div class="flex items-start gap-4 mb-6">
        <div class="p-3 rounded-2xl bg-rose-500/10 border border-rose-500/30 text-rose-400 text-2xl flex-shrink-0 animate-bounce">
          ⚠️
        </div>
        <div>
          <div class="inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full bg-rose-500/10 border border-rose-500/30 text-rose-400 text-[10px] font-black uppercase tracking-widest mb-1 font-mono">
            CRITICAL SAFETY INTERLOCK
          </div>
          <h2 class="text-xl font-black text-white tracking-tight font-mono">
            {{ title || 'HIGH-RISK ACTION CONFIRMATION' }}
          </h2>
          <p class="text-xs text-slate-400 font-mono mt-1">
            {{ actionDescription || 'Physical hardware power cutoff requires dual-person credential authorization.' }}
          </p>
        </div>
      </div>

      <!-- Action Details Card -->
      <div class="bg-slate-950 border border-slate-800 rounded-2xl p-4 mb-6 text-xs font-mono space-y-2">
        <div class="flex justify-between">
          <span class="text-slate-500">Target Node:</span>
          <span class="text-slate-200 font-bold">SmartBox-X001 (0x8001)</span>
        </div>
        <div class="flex justify-between">
          <span class="text-slate-500">Physical Action:</span>
          <span class="text-rose-400 font-bold">HARDWARE_RELAY_CUTOFF (0xFF01)</span>
        </div>
        <div class="flex justify-between">
          <span class="text-slate-500">Safety Policy:</span>
          <span class="text-amber-400 font-bold">Four-Eyes Principle (双人复核)</span>
        </div>
      </div>

      <!-- Dual Authorization Form -->
      <div class="space-y-4 mb-6 font-mono text-xs">
        <!-- Operator Credential -->
        <div>
          <label class="block text-slate-300 font-bold mb-1.5 flex items-center justify-between">
            <span>👤 操作员凭证 (Operator PIN / RFID):</span>
            <span v-if="opValid" class="text-emerald-400 text-[10px]">✓ VERIFIED</span>
          </label>
          <input 
            v-model="operatorPin"
            type="password"
            maxlength="16"
            placeholder="Enter Operator PIN (e.g. 1234)"
            class="w-full bg-slate-950 border border-slate-700 rounded-xl px-4 py-2.5 text-slate-100 placeholder-slate-600 focus:outline-none focus:border-rose-500 focus:ring-1 focus:ring-rose-500 transition-all text-sm tracking-widest"
          />
        </div>

        <!-- Supervisor Credential -->
        <div>
          <label class="block text-slate-300 font-bold mb-1.5 flex items-center justify-between">
            <span>🛡️ 监护主管凭证 (Supervisor PIN / Key):</span>
            <span v-if="supValid" class="text-emerald-400 text-[10px]">✓ VERIFIED</span>
          </label>
          <input 
            v-model="supervisorPin"
            type="password"
            maxlength="16"
            placeholder="Enter Supervisor PIN (e.g. 8888)"
            class="w-full bg-slate-950 border border-slate-700 rounded-xl px-4 py-2.5 text-slate-100 placeholder-slate-600 focus:outline-none focus:border-rose-500 focus:ring-1 focus:ring-rose-500 transition-all text-sm tracking-widest"
          />
        </div>
      </div>

      <!-- Error Message -->
      <div v-if="errorMessage" class="mb-4 p-3 rounded-xl bg-rose-500/10 border border-rose-500/30 text-rose-400 text-xs font-mono flex items-center gap-2">
        <span>❌</span>
        <span>{{ errorMessage }}</span>
      </div>

      <!-- Countdown Warning & Actions -->
      <div class="flex flex-col sm:flex-row items-center justify-between gap-3 pt-4 border-t border-slate-800">
        <button 
          @click="handleCancel"
          class="w-full sm:w-auto px-5 py-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 font-mono text-xs font-bold transition-all border border-slate-700"
        >
          取消 (Cancel)
        </button>

        <button 
          @click="handleConfirm"
          :disabled="countdown > 0 || !canSubmit"
          :class="[
            'w-full sm:w-auto px-6 py-2.5 rounded-xl font-mono text-xs font-black transition-all flex items-center justify-center gap-2 shadow-lg',
            countdown > 0 || !canSubmit 
              ? 'bg-rose-900/40 text-rose-400/50 border border-rose-800/40 cursor-not-allowed' 
              : 'bg-rose-600 hover:bg-rose-500 text-white shadow-rose-600/30 cursor-pointer animate-pulse'
          ]"
        >
          <span>{{ countdown > 0 ? `防误触等待 (${countdown}s)` : '确认执行硬件断电' }}</span>
          <span v-if="countdown === 0">⚡</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onBeforeUnmount } from 'vue';

const props = defineProps<{
  isOpen: boolean;
  title?: string;
  actionDescription?: string;
}>();

const emit = defineEmits<{
  (e: 'confirm', payload: { operator: string; supervisor: string }): void;
  (e: 'cancel'): void;
}>();

const operatorPin = ref('');
const supervisorPin = ref('');
const errorMessage = ref('');
const countdown = ref(3);
let timer: any = null;

const opValid = computed(() => operatorPin.value.length >= 4);
const supValid = computed(() => supervisorPin.value.length >= 4 && supervisorPin.value !== operatorPin.value);
const canSubmit = computed(() => opValid.value && supValid.value);

function startCountdown() {
  countdown.value = 3;
  if (timer) clearInterval(timer);
  timer = setInterval(() => {
    if (countdown.value > 0) {
      countdown.value--;
    } else {
      clearInterval(timer);
      timer = null;
    }
  }, 1000);
}

watch(() => props.isOpen, (newVal) => {
  if (newVal) {
    operatorPin.value = '';
    supervisorPin.value = '';
    errorMessage.value = '';
    startCountdown();
  } else {
    if (timer) clearInterval(timer);
  }
});

function handleCancel() {
  emit('cancel');
}

function handleConfirm() {
  if (countdown.value > 0) return;
  if (!operatorPin.value || !supervisorPin.value) {
    errorMessage.value = '双方必须均输入授权凭证 (PIN 码)';
    return;
  }
  if (operatorPin.value === supervisorPin.value) {
    errorMessage.value = '双人复核安全违规：操作员与监护人不能为同一人！';
    return;
  }

  errorMessage.value = '';
  emit('confirm', {
    operator: operatorPin.value,
    supervisor: supervisorPin.value
  });
}

onBeforeUnmount(() => {
  if (timer) {
    clearInterval(timer);
    timer = null;
  }
});
</script>
