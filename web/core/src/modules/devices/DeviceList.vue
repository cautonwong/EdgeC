<template>
  <div class="p-8 lg:p-12 space-y-10 animate-in fade-in duration-500">
    <div class="flex justify-between items-end">
      <div>
        <h1 class="text-3xl font-black tracking-tight">设备列表</h1>
        <p class="text-slate-400 text-sm font-medium mt-1">执行层设备清单</p>
      </div>
      <button type="button" @click="fetchDevices" class="px-6 py-2 bg-white border border-slate-200 rounded-xl text-xs font-black text-slate-600 hover:bg-slate-50 transition shadow-sm active:scale-95">
        扫描总线
      </button>
    </div>

    <!-- Device Grid -->
    <div class="grid grid-cols-1 gap-6">
      <div class="bg-white rounded-xl shadow-xl shadow-slate-200/50 border border-slate-100 overflow-hidden">
        <table class="w-full text-left border-collapse">
          <thead>
            <tr class="bg-slate-50/50 border-b border-slate-100">
              <th class="px-8 py-5 text-xs font-black text-slate-400">设备标识</th>
              <th class="px-8 py-5 text-xs font-black text-slate-400">协议栈</th>
              <th class="px-8 py-5 text-xs font-black text-slate-400">通信状态</th>
              <th class="px-8 py-5 text-xs font-black text-slate-400 text-right">指标</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-50">
            <tr v-for="dev in devices" :key="dev.id" class="hover:bg-blue-50/30 transition-colors group">
              <td class="px-8 py-6">
                <div class="flex items-center gap-4">
                  <div class="w-10 h-10 bg-slate-100 rounded-xl flex items-center justify-center text-lg shadow-inner">🔌</div>
                  <div>
                    <div class="text-sm font-bold text-slate-700">{{ dev.id }}</div>
                    <div class="text-xs text-slate-400 font-mono">地址：{{ dev.address || '自动' }}</div>
                  </div>
                </div>
              </td>
              <td class="px-8 py-6">
                <span class="px-3 py-1 rounded-lg bg-blue-50 text-blue-600 text-xs font-black tracking-tighter border border-blue-100">
                  {{ dev.protocol }}
                </span>
              </td>
              <td class="px-8 py-6">
                <div class="flex items-center gap-2">
                  <span class="w-2 h-2 rounded-full" :class="dev.status === 'online' ? 'bg-emerald-500 shadow-[0_0_8px_var(--color-success)]' : 'bg-rose-500'"></span>
                  <span class="text-xs font-bold tracking-tight" :class="dev.status === 'online' ? 'text-emerald-600' : 'text-rose-500'">
                    {{ dev.status === 'online' ? '在线' : '离线' }}
                  </span>
                </div>
              </td>
              <td class="px-8 py-6 text-right">
                <div class="text-xs font-bold text-slate-600 font-mono">{{ dev.errors }} 错误 / {{ dev.success || 0 }} 成功</div>
                <div class="text-xs text-slate-300 mt-1">延迟：{{ dev.latency || 12 }}ms</div>
              </td>
            </tr>
          </tbody>
        </table>
        
        <div v-if="error" class="px-8 py-6">
          <UiBanner tone="danger" title="读取设备失败">{{ error }}</UiBanner>
        </div>
        <div v-else-if="devices.length === 0" class="py-20 text-center">
          <p class="text-slate-400 font-medium text-sm">{{ loading ? '正在扫描总线…' : '总线上没有发现设备' }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import UiBanner from '../../ui/components/Banner.vue';

interface DeviceRow {
  id: string;
  protocol: string;
  status: string;
  address?: string;
  errors?: number;
  success?: number;
  latency?: number;
}

const devices = ref<DeviceRow[]>([]);
const loading = ref(false);
const error = ref('');

async function fetchDevices(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    devices.value = await request<DeviceRow[]>('/api/v1/devices');
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
}

onMounted(fetchDevices);
</script>