<template>
  <div class="min-h-full bg-slate-50/50 p-8 lg:p-12 space-y-16 animate-in fade-in duration-700">
    <!-- Hero Header: The Command Center Look -->
    <section class="relative bg-slate-900 rounded-[3rem] overflow-hidden shadow-2xl border border-white/5">
      <!-- Ambient light effect -->
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-blue-500/10 rounded-full blur-[120px] -mr-40 -mt-40"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-indigo-500/10 rounded-full blur-[80px] -ml-20 -mb-20"></div>
      
      <div class="relative z-10 p-12 lg:p-16 flex flex-col md:flex-row items-center justify-between gap-12">
        <div class="space-y-6 text-center md:text-left">
          <div class="inline-flex items-center px-4 py-1.5 rounded-full bg-blue-500/10 border border-blue-500/20 text-blue-400 text-[10px] font-black uppercase tracking-[0.2em]">
            System Operational
          </div>
          <h1 class="text-4xl lg:text-6xl font-black text-white tracking-tighter leading-tight">
            Operational<br/><span class="text-blue-500">Intelligence</span>
          </h1>
          <p class="text-slate-400 text-base lg:text-lg font-medium max-w-md">
            Advanced management interface for <span class="text-slate-200 underline decoration-blue-500/50 underline-offset-4">{{ contractStore.contract?.product_name }}</span>.
          </p>
        </div>

        <!-- System-wide Stats Wall -->
        <div class="grid grid-cols-2 gap-6 w-full md:w-auto">
          <div class="bg-white/5 backdrop-blur-md border border-white/10 p-8 rounded-[2rem] hover:bg-white/10 transition-colors">
            <div class="text-[10px] font-black text-blue-400 uppercase tracking-widest mb-3">Health Index</div>
            <div class="text-3xl font-black text-white font-mono uppercase tracking-tighter">99.2<span class="text-xs ml-1 opacity-40">%</span></div>
          </div>
          <div class="bg-white/5 backdrop-blur-md border border-white/10 p-8 rounded-[2rem] hover:bg-white/10 transition-colors">
            <div class="text-[10px] font-black text-indigo-400 uppercase tracking-widest mb-3">Sync Latency</div>
            <div class="text-3xl font-black text-white font-mono uppercase tracking-tighter">14<span class="text-xs ml-1 opacity-40">ms</span></div>
          </div>
        </div>
      </div>
    </section>

    <!-- Content columns -->
    <div class="grid grid-cols-1 lg:grid-cols-4 gap-12 items-start">
      <!-- Left Column: Telemetry groups and physical channels -->
      <div class="lg:col-span-3 space-y-16">
        <!-- Content: Grouped Telemetry Cards -->
        <div v-for="[groupName, tags] in contractStore.groups" :key="groupName" class="space-y-10">
          <div class="flex items-center gap-8">
            <h2 class="text-2xl font-black text-slate-800 tracking-tight uppercase italic">{{ groupName }}</h2>
            <div class="h-px flex-1 bg-gradient-to-r from-slate-200 to-transparent"></div>
            <div class="text-[10px] font-black text-slate-400 uppercase tracking-widest">{{ tags.length }} metrics</div>
          </div>

          <div class="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-8">
            <LiveTagCard 
              v-for="tag in tags" 
              :key="tag.id" 
              :meta="tag" 
              :refreshMs="1000"
            />
          </div>
        </div>

        <!-- 🟢 Channel Activity Grid (维度七：通道活动面板) -->
        <section v-if="!contractStore.loading && systemStats.buses && systemStats.buses.length > 0" class="space-y-6 bg-slate-900/5 backdrop-blur-sm border border-slate-100 p-8 rounded-[3rem]">
          <div class="flex items-center gap-8">
            <h2 class="text-xl font-black text-slate-800 tracking-tight uppercase italic flex items-center gap-2">
              <span class="text-blue-500">🎛️</span> Physical Channels Activity
            </h2>
            <div class="h-px flex-1 bg-gradient-to-r from-slate-200 to-transparent"></div>
            <span class="px-3 py-1 text-[9px] font-black bg-blue-500/10 text-blue-600 rounded-full border border-blue-500/20 font-mono">
              GATEWAY REVERSE PROXY ACTIVE
            </span>
          </div>

          <div class="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-8 gap-4">
            <div 
              v-for="bus in systemStats.buses" 
              :key="bus.id"
              class="bg-white border p-4 rounded-2xl flex flex-col justify-between transition-all duration-300 hover:shadow-lg hover:-translate-y-0.5"
              :class="bus.busy ? 'border-blue-500 bg-blue-50/50 shadow-md shadow-blue-500/5' : 'border-slate-100'"
            >
              <div class="flex justify-between items-center">
                <span class="text-[10px] font-black text-slate-400 font-mono">CH-{{ String(bus.id).padStart(2, '0') }}</span>
                <div 
                  class="w-2 h-2 rounded-full"
                  :class="bus.busy ? 'bg-blue-500 animate-pulse' : 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.4)]'"
                ></div>
              </div>
              <div class="mt-4">
                <div class="text-[9px] font-black uppercase tracking-wider" :class="bus.busy ? 'text-blue-600' : 'text-slate-400'">
                  {{ bus.busy ? 'Active' : 'Idle' }}
                </div>
                <div class="text-[11px] font-mono font-bold mt-0.5 text-slate-700" v-if="bus.busy">
                  Queue: {{ bus.queue }}
                </div>
                <div class="text-[9px] text-slate-300 font-bold" v-else>
                  Standby
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>

      <!-- Right Column: App Store Panel (维度八：WASM 应用商店面板) -->
      <div class="lg:col-span-1 space-y-8 bg-white border border-slate-100 rounded-[2.5rem] p-8 shadow-xl shadow-slate-200/50">
        <div>
          <h2 class="text-xl font-black text-slate-800 tracking-tight uppercase italic flex items-center gap-2">
            <span class="text-indigo-500">🚀</span> WASM App Store
          </h2>
          <p class="text-[9px] text-slate-400 font-bold uppercase tracking-widest mt-1">Micro-App Sandbox Deployment</p>
        </div>

        <!-- File Upload Form -->
        <div class="space-y-6">
          <!-- Drag-and-drop zone -->
          <div 
            @dragover.prevent="isDragging = true"
            @dragleave.prevent="isDragging = false"
            @drop.prevent="handleFileDrop"
            class="border-2 border-dashed rounded-2xl p-6 text-center cursor-pointer transition-all duration-300"
            :class="[
              isDragging ? 'border-indigo-500 bg-indigo-50/50' : 'border-slate-200 hover:border-indigo-400 bg-slate-50/50',
              appFile ? 'border-emerald-500 bg-emerald-50/10' : ''
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
            <div class="text-3xl mb-2">{{ appFile ? '📦' : '📤' }}</div>
            <div class="text-xs font-bold text-slate-700 truncate">
              {{ appFile ? appFile.name : 'Drag & drop logic.wasm here' }}
            </div>
            <div class="text-[9px] text-slate-400 font-semibold uppercase tracking-wider mt-1">
              {{ appFile ? `${(appFile.size / 1024).toFixed(1)} KB` : 'or click to browse' }}
            </div>
          </div>

          <!-- Metadata fields -->
          <div class="space-y-4">
            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">App Name</label>
              <input 
                v-model="appName"
                type="text"
                placeholder="e.g. power_smoothing"
                class="w-full bg-slate-50 border border-slate-200/80 rounded-xl px-4 py-2 text-xs font-bold text-slate-700 outline-none focus:ring-2 focus:ring-indigo-500/20"
              />
            </div>

            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">Trigger Tag ID</label>
              <input 
                v-model="triggerTag"
                type="number"
                placeholder="e.g. 1001"
                class="w-full bg-slate-50 border border-slate-200/80 rounded-xl px-4 py-2 text-xs font-bold text-slate-700 outline-none focus:ring-2 focus:ring-indigo-500/20 font-mono"
              />
            </div>

            <div class="space-y-1">
              <label class="text-[9px] font-black text-slate-400 uppercase tracking-wider">Entry Function</label>
              <input 
                v-model="entryFunc"
                type="text"
                placeholder="e.g. on_tick"
                class="w-full bg-slate-50 border border-slate-200/80 rounded-xl px-4 py-2 text-xs font-bold text-slate-700 outline-none focus:ring-2 focus:ring-indigo-500/20 font-mono"
              />
            </div>
          </div>

          <button 
            @click="handleUpload"
            :disabled="isUploading || !appFile"
            class="w-full py-3 rounded-2xl text-[10px] font-black uppercase tracking-widest text-white shadow-lg transition-all duration-300 flex items-center justify-center gap-2 active:scale-95 disabled:opacity-50 disabled:pointer-events-none"
            :class="appFile ? 'bg-gradient-to-r from-indigo-600 to-blue-600 hover:from-indigo-500 hover:to-blue-500 shadow-indigo-600/20' : 'bg-slate-300 shadow-none'"
          >
            <span v-if="isUploading" class="w-3.5 h-3.5 border-2 border-white border-t-transparent rounded-full animate-spin"></span>
            {{ isUploading ? 'Deploying...' : 'Deploy App' }}
          </button>

          <!-- Status indicator -->
          <div 
            v-if="uploadStatus.message" 
            class="p-4 rounded-2xl text-xs font-semibold leading-relaxed border"
            :class="[
              uploadStatus.type === 'success' ? 'bg-emerald-50 text-emerald-800 border-emerald-100' : 'bg-rose-50 text-rose-800 border-rose-100'
            ]"
          >
            {{ uploadStatus.message }}
          </div>
        </div>

        <div class="h-px bg-slate-100"></div>

        <!-- Dynamic Running Apps List -->
        <div class="space-y-4">
          <div class="text-[10px] font-black text-slate-500 uppercase tracking-wider">Running Apps</div>
          
          <div v-if="systemStats.engine?.pipelines?.length > 0" class="space-y-3">
            <div 
              v-for="app in systemStats.engine.pipelines" 
              :key="app.id" 
              class="p-4 bg-slate-50 border border-slate-100 rounded-2xl flex items-center justify-between"
            >
              <div class="space-y-1">
                <div class="text-xs font-bold text-slate-700 font-mono">{{ app.id }}</div>
                <div class="text-[9px] text-slate-400 font-bold uppercase tracking-wider">Trigger: Tag #{{ app.trigger_tag }}</div>
              </div>
              <span class="px-2.5 py-1 text-[8px] font-black uppercase bg-emerald-50 text-emerald-600 rounded-full font-mono">Running</span>
            </div>
          </div>
          <div v-else class="text-center py-6 text-slate-400 text-xs font-medium">
            No dynamic sandbox apps loaded.
          </div>
        </div>
      </div>
    </div>

    <!-- Loading & Empty States -->
    <div v-if="contractStore.loading" class="flex flex-col items-center justify-center py-32">
      <div class="w-16 h-16 border-4 border-slate-200 border-t-blue-600 rounded-full animate-spin"></div>
      <p class="mt-6 text-slate-400 font-bold uppercase tracking-widest text-xs">Accessing Data Constitution...</p>
    </div>

    <div v-if="contractStore.groups.size === 0 && !contractStore.loading" class="text-center py-40 bg-white rounded-[4rem] shadow-sm border border-slate-100">
      <div class="text-7xl mb-8 opacity-20">📡</div>
      <h3 class="text-2xl font-black text-slate-800 tracking-tight">No Active Streams Found</h3>
      <p class="text-slate-400 mt-2 max-w-sm mx-auto">Verify your <b>enabled_tags</b> in product configuration.</p>
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
    // Clear form except default entry_func
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