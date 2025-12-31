<template>
  <div class="p-6 max-w-7xl mx-auto">
    <div class="flex justify-between items-center mb-6">
      <h2 class="text-2xl font-bold text-gray-800">Southbound Devices</h2>
      <button @click="fetchDevices" class="p-2 text-gray-500 hover:text-indigo-600 transition-colors">
        <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.001 0 01-15.357-2m15.357 2H15" />
        </svg>
      </button>
    </div>

    <!-- Device Table -->
    <div class="bg-white rounded-xl shadow-sm border border-gray-100 overflow-hidden">
      <table class="min-w-full divide-y divide-gray-200">
        <thead class="bg-gray-50">
          <tr>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Device ID</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Protocol</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Status</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Last Active</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Comm Errors</th>
          </tr>
        </thead>
        <tbody class="bg-white divide-y divide-gray-200">
          <tr v-for="device in devices" :key="device.id" class="hover:bg-gray-50 transition-colors">
            <td class="px-6 py-4 whitespace-nowrap">
              <div class="text-sm font-medium text-gray-900">{{ device.id }}</div>
            </td>
            <td class="px-6 py-4 whitespace-nowrap">
              <span class="px-2 inline-flex text-xs leading-5 font-semibold rounded-full bg-blue-100 text-blue-800">
                {{ device.protocol }}
              </span>
            </td>
            <td class="px-6 py-4 whitespace-nowrap">
              <div class="flex items-center">
                <span :class="device.status === 'online' ? 'bg-green-400' : 'bg-red-400'" class="flex-shrink-0 h-2.5 w-2.5 rounded-full mr-2"></span>
                <span class="text-sm text-gray-700 capitalize">{{ device.status }}</span>
              </div>
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
              {{ formatTime(device.last_active) }}
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="device.errors > 0 ? 'text-red-600 font-bold' : 'text-gray-500'">
              {{ device.errors }}
            </td>
          </tr>
        </tbody>
      </table>
      
      <div v-if="devices.length === 0" class="text-center py-12">
        <p class="text-gray-400">No southbound devices detected.</p>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import axios from 'axios';

interface Device {
  id: string;
  protocol: string;
  status: 'online' | 'offline';
  last_active: number;
  errors: number;
}

const devices = ref<Device[]>([]);
let timer: number;

const fetchDevices = async () => {
  try {
    const res = await axios.get('/api/v1/devices');
    devices.value = res.data;
  } catch (e) {
    console.error('Failed to fetch device list:', e);
  }
};

function formatTime(val: number) {
  if (!val) return 'Never';
  // 假设后端返回的是 libuv 的 uv_now() 毫秒值，
  // 此处简单演示格式化逻辑
  return new Date().toLocaleTimeString();
}

onMounted(() => {
  fetchDevices();
  timer = setInterval(fetchDevices, 5000);
});

onUnmounted(() => clearInterval(timer));
</script>
