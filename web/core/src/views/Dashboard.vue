<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-12 animate-in fade-in duration-700 font-ui">
    <!-- Hero Header: The Industrial Command Center Look -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60">
      <!-- Ambient light effect -->
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-sky-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-emerald-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>
      
      <div class="relative z-10 p-8 lg:p-12 flex flex-col md:flex-row items-center justify-between gap-8">
        <div class="space-y-4 text-center md:text-left">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse"></span>
            System Operational
          </div>
          <h1 class="text-3xl lg:text-5xl font-black text-white tracking-tight leading-tight">
            {{ contractStore.branding.title }}<br/>
            <span class="text-sky-400 font-semibold text-2xl lg:text-3xl tracking-normal">{{ contractStore.branding.subtitle }}</span>
          </h1>
          <p class="text-slate-400 text-sm lg:text-base font-medium max-w-xl">
            Contract-driven telemetry & distributed orchestration for 
            <span class="text-slate-200 font-mono font-bold bg-[#1e293b] px-2 py-0.5 rounded border border-[#334155]">{{ contractStore.contract?.product_name || 'EdgeOS Target' }}</span>.
          </p>
        </div>

        <!-- System-wide Stats Wall -->
        <div class="grid grid-cols-2 sm:grid-cols-3 gap-4 w-full md:w-auto">
          <div class="bg-[#1e293b]/70 backdrop-blur-md border border-[#334155] p-5 rounded-2xl hover:border-sky-500/40 transition-colors">
            <div class="text-[9px] font-black text-sky-400 uppercase tracking-widest mb-1.5">Health Index</div>
            <div class="text-2xl font-black text-white font-mono uppercase tracking-tight metric-num">
              {{ health.status === 'ok' ? '99.8' : (health.status === 'error' ? '75.0' : '99.2') }}<span class="text-xs ml-0.5 text-slate-400">%</span>
            </div>
          </div>
          <div class="bg-[#1e293b]/70 backdrop-blur-md border border-[#334155] p-5 rounded-2xl hover:border-sky-500/40 transition-colors">
            <div class="text-[9px] font-black text-emerald-400 uppercase tracking-widest mb-1.5">Sync Latency</div>
            <div class="text-2xl font-black text-white font-mono uppercase tracking-tight metric-num">
              12<span class="text-xs ml-0.5 text-slate-400">ms</span>
            </div>
          </div>
          <div class="bg-[#1e293b]/70 backdrop-blur-md border border-[#334155] p-5 rounded-2xl hover:border-sky-500/40 transition-colors col-span-2 sm:col-span-1">
            <div class="text-[9px] font-black text-amber-400 uppercase tracking-widest mb-1.5">Live Metrics</div>
            <div class="text-2xl font-black text-white font-mono uppercase tracking-tight metric-num">
              {{ contractStore.totalMetrics }}<span class="text-xs ml-0.5 text-slate-400">pts</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <!-- Group Filter & Search Bar -->
    <div class="flex flex-col md:flex-row items-stretch md:items-center justify-between gap-4 bg-[#0f172a] p-3 rounded-2xl border border-[#334155]">
      <!-- Group Tabs -->
      <div class="flex items-center gap-2 overflow-x-auto pb-1 md:pb-0 scrollbar-none">
        <button
          @click="contractStore.setActiveGroup('all')"
          class="px-4 py-2 rounded-xl text-xs font-bold transition-all whitespace-nowrap flex items-center gap-2"
          :class="contractStore.activeGroup === 'all' 
            ? 'bg-sky-500 text-slate-950 shadow-md shadow-sky-500/20' 
            : 'bg-[#1e293b] text-slate-300 hover:text-white hover:bg-[#334155]'"
        >
          <span>All Metrics</span>
          <span class="px-1.5 py-0.2 rounded-full text-[10px] font-mono" :class="contractStore.activeGroup === 'all' ? 'bg-slate-950/20 text-slate-950 font-black' : 'bg-[#334155] text-slate-300'">
            {{ contractStore.totalMetrics }}
          </span>
        </button>

        <button
          v-for="groupName in contractStore.groupList"
          :key="groupName"
          @click="contractStore.setActiveGroup(groupName)"
          class="px-4 py-2 rounded-xl text-xs font-bold transition-all whitespace-nowrap flex items-center gap-2"
          :class="contractStore.activeGroup === groupName 
            ? 'bg-sky-500 text-slate-950 shadow-md shadow-sky-500/20' 
            : 'bg-[#1e293b] text-slate-300 hover:text-white hover:bg-[#334155]'"
        >
          <span>{{ groupName }}</span>
          <span class="px-1.5 py-0.2 rounded-full text-[10px] font-mono" :class="contractStore.activeGroup === groupName ? 'bg-slate-950/20 text-slate-950 font-black' : 'bg-[#334155] text-slate-300'">
            {{ (contractStore.groups.get(groupName) || []).length }}
          </span>
        </button>
      </div>

      <!-- Search Filter -->
      <div class="relative w-full md:w-72">
        <input 
          v-model="contractStore.searchQuery"
          type="text"
          placeholder="Filter point name / label..."
          class="w-full bg-[#1e293b] border border-[#334155] rounded-xl px-4 py-2 text-xs font-medium text-slate-200 placeholder-slate-500 outline-none focus:border-sky-500 focus:ring-1 focus:ring-sky-500 transition-all"
        />
        <span v-if="contractStore.searchQuery" @click="contractStore.setSearchQuery('')" class="absolute right-3 top-2.5 text-xs text-slate-400 hover:text-white cursor-pointer">✕</span>
      </div>
    </div>

    <!-- Content columns -->
    <div class="grid grid-cols-1 lg:grid-cols-4 gap-8 items-start">
      <!-- Left Column: Telemetry groups and physical channels -->
      <div class="lg:col-span-3 space-y-12">
        <!-- Render Grouped Telemetry Cards -->
        <template v-if="contractStore.activeGroup === 'all'">
          <div v-for="[groupName, tags] in contractStore.groups" :key="groupName" class="space-y-6">
            <div class="flex items-center gap-4">
              <h2 class="text-xl font-black text-slate-100 tracking-tight uppercase">{{ groupName }}</h2>
              <div class="h-px flex-1 bg-gradient-to-r from-[#334155] to-transparent"></div>
              <div class="text-[10px] font-black text-slate-400 font-mono tracking-widest">{{ tags.length }} metrics</div>
            </div>

            <div class="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-5">
              <LiveTagCard 
                v-for="tag in tags" 
                :key="tag.id" 
                :meta="tag" 
                :refreshMs="1000"
              />
            </div>
          </div>
        </template>

        <!-- Render Filtered Single Group -->
        <template v-else>
          <div class="space-y-6">
            <div class="flex items-center gap-4">
              <h2 class="text-xl font-black text-slate-100 tracking-tight uppercase">{{ contractStore.activeGroup }}</h2>
              <div class="h-px flex-1 bg-gradient-to-r from-[#334155] to-transparent"></div>
              <div class="text-[10px] font-black text-slate-400 font-mono tracking-widest">
                {{ contractStore.filteredTags.length }} metrics
              </div>
            </div>

            <div class="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-5">
              <LiveTagCard 
                v-for="tag in contractStore.filteredTags" 
                :key="tag.id" 
                :meta="tag" 
                :refreshMs="1000"
              />
            </div>
          </div>
        </template>

        <!-- Channel Activity Grid -->
        <section v-if="!contractStore.loading && systemStats.buses && systemStats.buses.length > 0" class="space-y-5 bg-[#0f172a] border border-[#334155] p-6 rounded-3xl">
          <div class="flex items-center gap-4">
            <h2 class="text-lg font-black text-slate-100 tracking-tight uppercase flex items-center gap-2">
              <span class="text-sky-400">🎛️</span> Physical Channels Activity
            </h2>
            <div class="h-px flex-1 bg-gradient-to-r from-[#334155] to-transparent"></div>
            <span class="px-2.5 py-0.5 text-[9px] font-black bg-sky-500/10 text-sky-400 rounded-full border border-sky-500/20 font-mono">
              GATEWAY PROXY ACTIVE
            </span>
          </div>

          <div class="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-8 gap-3">
            <div 
              v-for="bus in systemStats.buses" 
              :key="bus.id"
              class="bg-[#1e293b] border p-3.5 rounded-xl flex flex-col justify-between transition-all duration-300 hover:border-sky-500/50"
              :class="bus.busy ? 'border-sky-500 bg-sky-950/20 shadow-md shadow-sky-500/5' : 'border-[#334155]'"
            >
              <div class="flex justify-between items-center">
                <span class="text-[10px] font-black text-slate-400 font-mono">CH-{{ String(bus.id).padStart(2, '0') }}</span>
                <div 
                  class="w-2 h-2 rounded-full"
                  :class="bus.busy ? 'bg-sky-400 animate-pulse' : 'bg-emerald-400 shadow-[0_0_8px_rgba(16,185,129,0.4)]'"
                ></div>
              </div>
              <div class="mt-3">
                <div class="text-[9px] font-black uppercase tracking-wider" :class="bus.busy ? 'text-sky-400' : 'text-slate-400'">
                  {{ bus.busy ? 'Active' : 'Idle' }}
                </div>
                <div class="text-[10px] font-mono font-bold mt-0.5 text-slate-200" v-if="bus.busy">
                  Queue: {{ bus.queue }}
                </div>
                <div class="text-[9px] text-slate-500 font-semibold" v-else>
                  Standby
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>

      <!-- Right Column: WASM App Store Panel -->
      <div class="lg:col-span-1 space-y-6 bg-[#0f172a] border border-[#334155] rounded-3xl p-6 shadow-xl">
        <div>
          <h2 class="text-lg font-black text-slate-100 tracking-tight uppercase flex items-center gap-2">
            <span class="text-indigo-400">🚀</span> WASM App Sandbox
          </h2>
          <p class="text-[9px] text-slate-400 font-bold uppercase tracking-widest mt-1">Micro-App Hot Deployment</p>
        </div>

        <!-- File Upload Form -->
        <div class="space-y-5">
          <div 
            @dragover.prevent="isDragging = true"
            @dragleave.prevent="isDragging = false"
            @drop.prevent="handleFileDrop"
            class="border-2 border-dashed rounded-2xl p-5 text-center cursor-pointer transition-all duration-300"
            :class="[
              isDragging ? 'border-indigo-500 bg-indigo-950/30' : 'border-[#334155] hover:border-indigo-400 bg-[#1e293b]/50',
              appFile ? 'border-emerald-500 bg-emerald-950/20' : ''
            ]"
            @click="$refs.fileInput.click()"
          >
            <input 
              ref="fileInput"
              type="file"
              accept=".wasm"
              class="hidden"
              @change="handleFileChange"
            />
            <div class="text-2xl mb-1.5">{{ appFile ? '📦' : '📤' }}</div>
            <div class="text-xs font-bold text-slate-200 truncate">
              {{ appFile ? appFile.name : 'Drag & drop logic.wasm here' }}
            </div>
            <div class="text-[9px] text-slate-400 font-semibold uppercase tracking-wider mt-1">
              {{ appFile ? `${(appFile.size / 1024).toFixed(1)} KB` : 'or click to browse' }}
            </div>
          </div>

          <!-- Metadata fields -->
          <div class="space-y-3">
            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">App Name</label>
              <input 
                v-model="appName"
                type="text"
                placeholder="e.g. power_smoothing"
                class="w-full bg-[#1e293b] border border-[#334155] rounded-xl px-3.5 py-2 text-xs font-bold text-slate-200 outline-none focus:border-indigo-500"
              />
            </div>

            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">Trigger Tag ID</label>
              <input 
                v-model="triggerTag"
                type="number"
                placeholder="e.g. 1001"
                class="w-full bg-[#1e293b] border border-[#334155] rounded-xl px-3.5 py-2 text-xs font-bold text-slate-200 outline-none focus:border-indigo-500 font-mono"
              />
            </div>

            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">Entry Function</label>
              <input 
                v-model="entryFunc"
                type="text"
                placeholder="e.g. on_tick"
                class="w-full bg-[#1e293b] border border-[#334155] rounded-xl px-3.5 py-2 text-xs font-bold text-slate-200 outline-none focus:border-indigo-500 font-mono"
              />
            </div>
          </div>

          <button 
            @click="handleUpload"
            :disabled="isUploading || !appFile"
            class="w-full py-2.5 rounded-xl text-[10px] font-black uppercase tracking-widest text-white transition-all duration-300 flex items-center justify-center gap-2 active:scale-95 disabled:opacity-40 disabled:pointer-events-none shadow-lg"
            :class="appFile ? 'bg-gradient-to-r from-indigo-600 to-sky-600 hover:from-indigo-500 hover:to-sky-500 shadow-indigo-600/30' : 'bg-[#334155] shadow-none'"
          >
            <span v-if="isUploading" class="w-3.5 h-3.5 border-2 border-white border-t-transparent rounded-full animate-spin"></span>
            {{ isUploading ? 'Deploying...' : 'Deploy WASM App' }}
          </button>

          <!-- Status indicator -->
          <div 
            v-if="uploadStatus.message" 
            class="p-3.5 rounded-xl text-xs font-semibold leading-relaxed border"
            :class="[
              uploadStatus.type === 'success' ? 'bg-emerald-950/40 text-emerald-300 border-emerald-500/30' : 'bg-rose-950/40 text-rose-300 border-rose-500/30'
            ]"
          >
            {{ uploadStatus.message }}
          </div>
        </div>

        <div class="h-px bg-[#334155]"></div>

        <!-- Dynamic Running Apps List -->
        <div class="space-y-3">
          <div class="text-[10px] font-black text-slate-400 uppercase tracking-wider">Running Sandboxes</div>
          
          <div v-if="systemStats.engine?.pipelines?.length > 0" class="space-y-2.5">
            <div 
              v-for="app in systemStats.engine.pipelines" 
              :key="app.id" 
              class="p-3 bg-[#1e293b] border border-[#334155] rounded-xl flex items-center justify-between"
            >
              <div class="space-y-0.5">
                <div class="text-xs font-bold text-slate-200 font-mono">{{ app.id }}</div>
                <div class="text-[9px] text-slate-400 font-bold uppercase tracking-wider">Trigger: Tag #{{ app.trigger_tag }}</div>
              </div>
              <span class="px-2 py-0.5 text-[8px] font-black uppercase bg-emerald-500/10 text-emerald-400 rounded-full font-mono border border-emerald-500/20">Running</span>
            </div>
          </div>
          <div v-else class="text-center py-4 text-slate-500 text-xs font-medium">
            No dynamic WASM apps loaded.
          </div>
        </div>
      </div>
    </div>

    <!-- Loading & Empty States -->
    <div v-if="contractStore.loading" class="flex flex-col items-center justify-center py-24">
      <div class="w-12 h-12 border-4 border-[#334155] border-t-sky-400 rounded-full animate-spin"></div>
      <p class="mt-4 text-slate-400 font-bold uppercase tracking-widest text-xs">Syncing Contract Schema...</p>
    </div>

    <div v-if="contractStore.groups.size === 0 && !contractStore.loading" class="text-center py-28 bg-[#0f172a] rounded-[2.5rem] border border-[#334155]">
      <div class="text-5xl mb-4 opacity-30">📡</div>
      <h3 class="text-xl font-black text-slate-200 tracking-tight">No Active Streams Found</h3>
      <p class="text-slate-400 text-xs mt-1.5 max-w-sm mx-auto">Verify your <b>enabled_tags</b> in product contract configuration.</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import axios from 'axios';
import { useContractStore } from '../services/ContractLoader';
import LiveTagCard from '../components/LiveTagCard.vue';

const contractStore = useContractStore();
const health = ref<any>({});
const systemStats = ref<any>({});
let timer: any;
let statsTimer: any;

// Drag and drop state
const isDragging = ref(false);
const appFile = ref<File | null>(null);
const appName = ref('');
const triggerTag = ref('');
const entryFunc = ref('on_tick');
const uploadStatus = ref({ type: '', message: '' });
const isUploading = ref(false);

const handleFileChange = (e: Event) => {
  const target = e.target as HTMLInputElement;
  if (target.files && target.files.length > 0) {
    setFile(target.files[0]);
  }
};

const handleFileDrop = (e: DragEvent) => {
  isDragging.value = false;
  if (e.dataTransfer?.files && e.dataTransfer.files.length > 0) {
    setFile(e.dataTransfer.files[0]);
  }
};

const setFile = (file: File) => {
  appFile.value = file;
  if (!appName.value) {
    appName.value = file.name.replace(/\.[^/.]+$/, "");
  }
};

const handleUpload = async () => {
  if (!appFile.value || !appName.value || !triggerTag.value || !entryFunc.value) {
    uploadStatus.value = { type: 'error', message: 'All fields are required.' };
    return;
  }
  isUploading.value = true;
  uploadStatus.value = { type: '', message: '' };

  const formData = new FormData();
  formData.append('file', appFile.value);
  formData.append('name', appName.value);
  formData.append('trigger_tag', triggerTag.value);
  formData.append('entry_func', entryFunc.value);

  try {
    await axios.post('/api/v1/apps/upload', formData, {
      headers: {
        'Content-Type': 'multipart/form-data'
      }
    });
    uploadStatus.value = { type: 'success', message: 'App deployed successfully!' };
    appFile.value = null;
    appName.value = '';
    triggerTag.value = '';
    fetchStats();
  } catch (err: any) {
    const errMsg = err.response?.data?.message || err.message || 'Deployment failed.';
    uploadStatus.value = { type: 'error', message: errMsg };
  } finally {
    isUploading.value = false;
  }
};

const fetchHealth = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    health.value = res.data;
  } catch (e) { health.value = { status: 'error' }; }
};

const fetchStats = async () => {
  try {
    const res = await axios.get('/api/v1/system/stats');
    systemStats.value = res.data;
  } catch (e) { systemStats.value = {}; }
};

onMounted(() => {
  contractStore.loadContract();
  fetchHealth();
  fetchStats();
  timer = setInterval(fetchHealth, 3000);
  statsTimer = setInterval(fetchStats, 1000);
});

onUnmounted(() => {
  clearInterval(timer);
  clearInterval(statsTimer);
});
</script>