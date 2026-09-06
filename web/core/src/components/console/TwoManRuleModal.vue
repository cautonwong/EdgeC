<template>
  <div v-if="isOpen" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-md select-none font-mono text-xs">
    <!-- Modal Card Container -->
    <div class="w-full max-w-lg bg-[#080e1a] border-2 border-rose-500/80 rounded-3xl shadow-2xl overflow-hidden animate-in fade-in zoom-in-95 duration-150">
      <!-- Hazard Warning Striped Banner -->
      <div class="h-3 w-full bg-[repeating-linear-gradient(45deg,#f43f5e,#f43f5e_12px,#0f172a_12px,#0f172a_24px)]"></div>

      <!-- Header -->
      <div class="p-6 border-b border-rose-900/40 bg-rose-950/20 flex items-start gap-4">
        <div class="w-12 h-12 rounded-2xl bg-rose-500/20 border border-rose-500/40 text-rose-400 flex items-center justify-center text-2xl shrink-0">
          ⚠️
        </div>
        <div class="flex-1 min-w-0">
          <div class="flex items-center gap-2">
            <span class="px-2 py-0.5 rounded bg-rose-500 text-white font-bold text-[10px] uppercase tracking-wider">IEC 62443 SL-3</span>
            <span class="text-rose-400 font-bold text-sm">Two-Man Rule Dual Authorization</span>
          </div>
          <p class="text-slate-400 text-[11px] mt-1">
            High-risk operational command detected. Physical second-person supervisor authorization is mandatory before dispatch.
          </p>
        </div>
      </div>

      <!-- Command & Risk Inspection Box -->
      <div class="p-6 space-y-4">
        <div class="bg-slate-950 border border-rose-500/30 rounded-2xl p-4 space-y-2">
          <div class="flex items-center justify-between text-[11px]">
            <span class="text-slate-400">Target Command:</span>
            <span class="px-2 py-0.5 rounded bg-rose-500/20 text-rose-400 font-bold uppercase text-[10px]">
              {{ riskLevel || 'HIGH RISK' }}
            </span>
          </div>
          <div class="p-2.5 bg-black/60 rounded-xl border border-slate-800 text-rose-300 font-bold break-all">
            {{ targetCommand }}
          </div>
          <div v-if="riskReason" class="text-[11px] text-amber-400/90 flex items-center gap-1.5">
            <span>ℹ️</span>
            <span>{{ riskReason }}</span>
          </div>
        </div>

        <!-- Dual Authorization Form -->
        <div class="space-y-3">
          <!-- Initiator Display -->
          <div class="grid grid-cols-2 gap-3">
            <div>
              <label class="block text-slate-400 text-[10px] mb-1 uppercase font-bold">1st Operator (Initiator)</label>
              <input 
                :value="initiatorUser" 
                readonly 
                class="w-full bg-slate-900 border border-slate-800 rounded-xl px-3 py-2 text-slate-300 font-mono text-xs cursor-not-allowed opacity-80"
              />
            </div>
            <div>
              <label class="block text-slate-400 text-[10px] mb-1 uppercase font-bold">Initiator Role</label>
              <input 
                :value="initiatorRole" 
                readonly 
                class="w-full bg-slate-900 border border-slate-800 rounded-xl px-3 py-2 text-cyan-400 font-mono text-xs cursor-not-allowed opacity-80"
              />
            </div>
          </div>

          <!-- Supervisor Username -->
          <div>
            <label class="block text-slate-300 text-[10px] mb-1 uppercase font-bold flex items-center justify-between">
              <span>2nd Operator / Supervisor Username <span class="text-rose-400">*</span></span>
              <span class="text-slate-500 text-[9px]">(Must be different engineer/admin)</span>
            </label>
            <input 
              v-model="supervisorUser" 
              placeholder="e.g. supervisor_admin or lead_eng"
              class="w-full bg-slate-900/90 border border-slate-700 focus:border-rose-500 focus:ring-1 focus:ring-rose-500 rounded-xl px-3 py-2 text-white font-mono text-xs placeholder-slate-600 outline-none transition"
              autocomplete="off"
            />
          </div>

          <!-- Supervisor Passkey or 2FA Token -->
          <div>
            <label class="block text-slate-300 text-[10px] mb-1 uppercase font-bold flex items-center justify-between">
              <span>Supervisor Passkey / 6-Digit 2FA Token <span class="text-rose-400">*</span></span>
              <span class="text-slate-500 text-[9px]">(TOTP / 624430 / SUPERVISOR_KEY)</span>
            </label>
            <input 
              v-model="supervisorToken" 
              type="password"
              placeholder="Enter 6-digit TOTP code or supervisor passkey..."
              class="w-full bg-slate-900/90 border border-slate-700 focus:border-rose-500 focus:ring-1 focus:ring-rose-500 rounded-xl px-3 py-2 text-white font-mono text-xs placeholder-slate-600 outline-none transition"
              autocomplete="off"
            />
          </div>

          <!-- Operational Reason / Work Order -->
          <div>
            <label class="block text-slate-300 text-[10px] mb-1 uppercase font-bold">Operational Justification / Work Order</label>
            <input 
              v-model="changeReason" 
              placeholder="e.g. Scheduled maintenance WO-20260906-01"
              class="w-full bg-slate-900/90 border border-slate-700 focus:border-rose-500 focus:ring-1 focus:ring-rose-500 rounded-xl px-3 py-2 text-white font-mono text-xs placeholder-slate-600 outline-none transition"
            />
          </div>

          <!-- Error Alert Banner -->
          <div v-if="validationError" class="p-2.5 rounded-xl bg-rose-500/20 border border-rose-500/50 text-rose-300 text-[11px] flex items-center gap-2">
            <span>❌</span>
            <span>{{ validationError }}</span>
          </div>
        </div>
      </div>

      <!-- Footer & Actions with 60s Timeout -->
      <div class="p-6 bg-slate-950/80 border-t border-slate-900 flex items-center justify-between gap-4">
        <div class="flex items-center gap-2 text-slate-400 text-[11px]">
          <span class="w-2 h-2 rounded-full bg-amber-400 animate-ping"></span>
          <span>Timeout in <strong class="text-amber-400 font-bold">{{ remainingSeconds }}s</strong></span>
        </div>

        <div class="flex items-center gap-3">
          <button 
            @click="handleCancel"
            class="px-4 py-2 rounded-xl bg-slate-900 hover:bg-slate-800 text-slate-300 hover:text-white border border-slate-800 transition cursor-pointer"
          >
            Abort / Cancel
          </button>

          <button 
            @click="handleAuthorize"
            :disabled="!canSubmit"
            :class="canSubmit ? 'bg-rose-600 hover:bg-rose-500 text-white shadow-lg shadow-rose-900/50 cursor-pointer' : 'bg-rose-950/40 text-rose-400/40 border-rose-900/30 cursor-not-allowed'"
            class="px-5 py-2 rounded-xl border border-rose-500 font-bold transition flex items-center gap-2"
          >
            <span>🔐</span>
            <span>Dual Authorize & Execute</span>
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue';

const props = defineProps<{
  isOpen: boolean;
  targetCommand: string;
  initiatorUser?: string;
  initiatorRole?: string;
  riskLevel?: string;
  riskReason?: string;
}>();

const emit = defineEmits<{
  (e: 'confirm', payload: { supervisorUser: string; supervisorToken: string; reason: string }): void;
  (e: 'cancel'): void;
}>();

const supervisorUser = ref('');
const supervisorToken = ref('');
const changeReason = ref('Operational Maintenance');
const validationError = ref('');
const remainingSeconds = ref(60);

let timerInterval: any = null;

const canSubmit = computed(() => {
  return supervisorUser.value.trim().length > 0 && 
         supervisorToken.value.trim().length > 0;
});

function startCountdown() {
  remainingSeconds.value = 60;
  if (timerInterval) clearInterval(timerInterval);
  timerInterval = setInterval(() => {
    remainingSeconds.value--;
    if (remainingSeconds.value <= 0) {
      clearInterval(timerInterval);
      validationError.value = 'Authorization session timed out. Operation aborted.';
      setTimeout(() => {
        handleCancel();
      }, 1000);
    }
  }, 1000);
}

function handleAuthorize() {
  validationError.value = '';
  const op = (props.initiatorUser || 'operator').trim().toLowerCase();
  const sup = supervisorUser.value.trim().toLowerCase();

  if (op === sup) {
    validationError.value = `Two-Man Rule Error: Supervisor cannot be identical to the initiator (${props.initiatorUser}).`;
    return;
  }

  emit('confirm', {
    supervisorUser: supervisorUser.value.trim(),
    supervisorToken: supervisorToken.value.trim(),
    reason: changeReason.value.trim() || 'Two-man authorized execution'
  });
}

function handleCancel() {
  if (timerInterval) clearInterval(timerInterval);
  supervisorUser.value = '';
  supervisorToken.value = '';
  validationError.value = '';
  emit('cancel');
}

watch(() => props.isOpen, (newVal) => {
  if (newVal) {
    supervisorUser.value = '';
    supervisorToken.value = '';
    validationError.value = '';
    startCountdown();
  } else {
    if (timerInterval) clearInterval(timerInterval);
  }
});

onUnmounted(() => {
  if (timerInterval) clearInterval(timerInterval);
});
</script>
